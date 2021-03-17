/*
 * Copyright 2015 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Removes branches for which we go to where they go anyhow
//

#include <ir/branch-utils.h>
#include <ir/cost.h>
#include <ir/effects.h>
#include <ir/gc-type-utils.h>
#include <ir/literal-utils.h>
#include <ir/utils.h>
#include <parsing.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>

namespace wasm {

// Grab a slice out of a block, replacing it with nops, and returning
// either another block with the contents (if more than 1) or a single
// expression.
// This does not finalize the input block; it leaves that for the caller.
static Expression*
stealSlice(Builder& builder, Block* input, Index from, Index to) {
  Expression* ret;
  if (to == from + 1) {
    // just one
    ret = input->list[from];
  } else {
    auto* block = builder.makeBlock();
    for (Index i = from; i < to; i++) {
      block->list.push_back(input->list[i]);
    }
    block->finalize();
    ret = block;
  }
  if (to == input->list.size()) {
    input->list.resize(from);
  } else {
    for (Index i = from; i < to; i++) {
      input->list[i] = builder.makeNop();
    }
  }
  return ret;
}

// to turn an if into a br-if, we must be able to reorder the
// condition and possible value, and the possible value must
// not have side effects (as they would run unconditionally)
static bool canTurnIfIntoBrIf(Expression* ifCondition,
                              Expression* brValue,
                              PassOptions& options,
                              Module& wasm) {
  // if the if isn't even reached, this is all dead code anyhow
  if (ifCondition->type == Type::unreachable) {
    return false;
  }
  if (!brValue) {
    return true;
  }
  EffectAnalyzer value(options, wasm, brValue);
  if (value.hasSideEffects()) {
    return false;
  }
  return !EffectAnalyzer(options, wasm, ifCondition).invalidates(value);
}

// This leads to similar choices as LLVM does.
// See https://github.com/WebAssembly/binaryen/pull/4228
// It can be tuned more later.
const Index TooCostlyToRunUnconditionally = 9;

static_assert(TooCostlyToRunUnconditionally < CostAnalyzer::Unacceptable,
              "We never run code unconditionally if it has unacceptable cost");

// Check if it is not worth it to run code unconditionally. This
// assumes we are trying to run two expressions where previously
// only one of the two might have executed. We assume here that
// executing both is good for code size.
static bool tooCostlyToRunUnconditionally(const PassOptions& passOptions,
                                          Expression* one,
                                          Expression* two) {
  // If we care mostly about code size, just do it for that reason.
  if (passOptions.shrinkLevel) {
    return false;
  }
  // Consider the cost of executing all the code unconditionally.
  auto total = CostAnalyzer(one).cost + CostAnalyzer(two).cost;
  return total >= TooCostlyToRunUnconditionally;
}

// As above, but a single expression that we are considering moving to a place
// where it executes unconditionally.
static bool tooCostlyToRunUnconditionally(const PassOptions& passOptions,
                                          Expression* curr) {
  if (passOptions.shrinkLevel) {
    return false;
  }
  return CostAnalyzer(curr).cost >= TooCostlyToRunUnconditionally;
}

struct RemoveUnusedBrs : public WalkerPass<PostWalker<RemoveUnusedBrs>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<RemoveUnusedBrs>();
  }

  bool anotherCycle;

  using Flows = std::vector<Expression**>;

  // list of breaks that are currently flowing. if they reach their target
  // without interference, they can be removed (or their value forwarded TODO)
  Flows flows;

  // a stack for if-else contents, we merge their outputs
  std::vector<Flows> ifStack;

  // list of all loops, so we can optimize them
  std::vector<Loop*> loops;

  static void visitAny(RemoveUnusedBrs* self, Expression** currp) {
    auto* curr = *currp;
    auto& flows = self->flows;

    if (curr->is<Break>()) {
      flows.clear();
      auto* br = curr->cast<Break>();
      if (!br->condition) { // TODO: optimize?
        // a break, let's see where it flows to
        flows.push_back(currp);
      } else {
        self->stopValueFlow();
      }
    } else if (curr->is<Return>()) {
      flows.clear();
      flows.push_back(currp);
    } else if (curr->is<If>()) {
      auto* iff = curr->cast<If>();
      if (iff->condition->type == Type::unreachable) {
        // avoid trying to optimize this, we never reach it anyhow
        self->stopFlow();
        return;
      }
      if (iff->ifFalse) {
        assert(self->ifStack.size() > 0);
        auto ifTrueFlows = std::move(self->ifStack.back());
        self->ifStack.pop_back();
        // we can flow values out in most cases, except if one arm
        // has the none type - we will update the types later, but
        // there is no way to emit a proper type for one arm being
        // none and the other flowing a value; and there is no way
        // to flow a value from a none.
        if (iff->ifTrue->type == Type::none ||
            iff->ifFalse->type == Type::none) {
          self->removeValueFlow(ifTrueFlows);
          self->stopValueFlow();
        }
        for (auto* flow : ifTrueFlows) {
          flows.push_back(flow);
        }
      } else {
        // if without else stops the flow of values
        self->stopValueFlow();
      }
    } else if (auto* block = curr->dynCast<Block>()) {
      // any breaks flowing to here are unnecessary, as we get here anyhow
      auto name = block->name;
      auto& list = block->list;
      if (name.is()) {
        Index size = flows.size();
        Index skip = 0;
        for (Index i = 0; i < size; i++) {
          auto* flow = (*flows[i])->dynCast<Break>();
          if (flow && flow->name == name) {
            if (!flow->value) {
              // br => nop
              ExpressionManipulator::nop<Break>(flow);
            } else {
              // br with value => value
              *flows[i] = flow->value;
            }
            skip++;
            self->anotherCycle = true;
          } else if (skip > 0) {
            flows[i - skip] = flows[i];
          }
        }
        if (skip > 0) {
          flows.resize(size - skip);
        }
      }
      // Drop a nop at the end of a block, which prevents a value flowing. Note
      // that this is worth doing regardless of whether we have a name on this
      // block or not (which the if right above us checks) - such a nop is
      // always unneeded and can limit later optimizations.
      while (list.size() > 0 && list.back()->is<Nop>()) {
        list.resize(list.size() - 1);
        self->anotherCycle = true;
      }
      // A value flowing is only valid if it is a value that the block actually
      // flows out. If it is never reached, it does not flow out, and may be
      // invalid to represent as such.
      auto size = list.size();
      for (Index i = 0; i < size; i++) {
        if (i != size - 1 && list[i]->type == Type::unreachable) {
          // No value flows out of this block.
          self->stopValueFlow();
          break;
        }
      }
    } else if (curr->is<Nop>()) {
      // ignore (could be result of a previous cycle)
      self->stopValueFlow();
    } else if (curr->is<Loop>()) {
      // do nothing - it's ok for values to flow out
    } else if (auto* sw = curr->dynCast<Switch>()) {
      self->stopFlow();
      self->optimizeSwitch(sw);
    } else {
      // anything else stops the flow
      self->stopFlow();
    }
  }

  void stopFlow() { flows.clear(); }

  void removeValueFlow(Flows& currFlows) {
    currFlows.erase(std::remove_if(currFlows.begin(),
                                   currFlows.end(),
                                   [&](Expression** currp) {
                                     auto* curr = *currp;
                                     if (auto* ret = curr->dynCast<Return>()) {
                                       return ret->value;
                                     }
                                     return curr->cast<Break>()->value;
                                   }),
                    currFlows.end());
  }

  void stopValueFlow() { removeValueFlow(flows); }

  static void clear(RemoveUnusedBrs* self, Expression** currp) {
    self->flows.clear();
  }

  static void saveIfTrue(RemoveUnusedBrs* self, Expression** currp) {
    self->ifStack.push_back(std::move(self->flows));
  }

  void visitLoop(Loop* curr) { loops.push_back(curr); }

  void optimizeSwitch(Switch* curr) {
    // if the final element is the default, we don't need it
    while (!curr->targets.empty() && curr->targets.back() == curr->default_) {
      curr->targets.pop_back();
    }
    // if the first element is the default, we can remove it by shifting
    // everything (which does add a subtraction of a constant, but often that is
    // worth it as the constant can be folded away and/or we remove multiple
    // elements here)
    Index removable = 0;
    while (removable < curr->targets.size() &&
           curr->targets[removable] == curr->default_) {
      removable++;
    }
    if (removable > 0) {
      for (Index i = removable; i < curr->targets.size(); i++) {
        curr->targets[i - removable] = curr->targets[i];
      }
      curr->targets.resize(curr->targets.size() - removable);
      Builder builder(*getModule());
      curr->condition = builder.makeBinary(
        SubInt32, curr->condition, builder.makeConst(int32_t(removable)));
    }
    // when there isn't a value, we can do some trivial optimizations without
    // worrying about the value being executed before the condition
    if (curr->value) {
      return;
    }
    if (curr->targets.size() == 0) {
      // a switch with just a default always goes there
      Builder builder(*getModule());
      replaceCurrent(builder.makeSequence(builder.makeDrop(curr->condition),
                                          builder.makeBreak(curr->default_)));
    } else if (curr->targets.size() == 1) {
      // a switch with two options is basically an if
      Builder builder(*getModule());
      replaceCurrent(builder.makeIf(curr->condition,
                                    builder.makeBreak(curr->default_),
                                    builder.makeBreak(curr->targets.front())));
    } else {
      // there are also some other cases where we want to convert a switch into
      // ifs, especially if the switch is large and we are focusing on size. an
      // especially egregious case is a switch like this: [a b b [..] b b c]
      // with default b (which may be arrived at after we trim away excess
      // default values on both sides). in this case, we really have 3 values in
      // a simple form, so it is the next logical case after handling 1 and 2
      // values right above here. to optimize this, we must add a local + a
      // bunch of nodes (if*2, tee, eq, get, const, break*3), so the table must
      // be big enough for it to make sense

      // How many targets we need when shrinking. This is literally the size at
      // which the transformation begins to be smaller.
      const uint32_t MIN_SHRINK = 13;
      // How many targets we need when not shrinking, in which case, 2 ifs may
      // be slower, so we do this when the table is ridiculously large for one
      // with just 3 values in it.
      const uint32_t MIN_GENERAL = 128;

      auto shrink = getPassRunner()->options.shrinkLevel > 0;
      if ((curr->targets.size() >= MIN_SHRINK && shrink) ||
          (curr->targets.size() >= MIN_GENERAL && !shrink)) {
        for (Index i = 1; i < curr->targets.size() - 1; i++) {
          if (curr->targets[i] != curr->default_) {
            return;
          }
        }
        // great, we are in that case, optimize
        Builder builder(*getModule());
        auto temp = builder.addVar(getFunction(), Type::i32);
        Expression* z;
        replaceCurrent(z = builder.makeIf(
                         builder.makeLocalTee(temp, curr->condition, Type::i32),
                         builder.makeIf(builder.makeBinary(
                                          EqInt32,
                                          builder.makeLocalGet(temp, Type::i32),
                                          builder.makeConst(
                                            int32_t(curr->targets.size() - 1))),
                                        builder.makeBreak(curr->targets.back()),
                                        builder.makeBreak(curr->default_)),
                         builder.makeBreak(curr->targets.front())));
      }
    }
  }

  void visitIf(If* curr) {
    if (!curr->ifFalse) {
      // if without an else. try to reduce
      //    if (condition) br  =>  br_if (condition)
      if (Break* br = curr->ifTrue->dynCast<Break>()) {
        if (canTurnIfIntoBrIf(
              curr->condition, br->value, getPassOptions(), *getModule())) {
          if (!br->condition) {
            br->condition = curr->condition;
          } else {
            // In this case we can replace
            //   if (condition1) br_if (condition2)
            // =>
            //   br_if select (condition1) (condition2) (i32.const 0)
            // In other words, we replace an if (3 bytes) with a select and a
            // zero (also 3 bytes). The size is unchanged, but the select may
            // be further optimizable, and if select does not branch we also
            // avoid one branch.
            // Multivalue selects are not supported
            if (br->value && br->value->type.isTuple()) {
              return;
            }
            // If running the br's condition unconditionally is too expensive,
            // give up.
            auto* zero = LiteralUtils::makeZero(Type::i32, *getModule());
            if (tooCostlyToRunUnconditionally(
                  getPassOptions(), br->condition, zero)) {
              return;
            }
            // Of course we can't do this if the br's condition has side
            // effects, as we would then execute those unconditionally.
            if (EffectAnalyzer(getPassOptions(), *getModule(), br->condition)
                  .hasSideEffects()) {
              return;
            }
            Builder builder(*getModule());
            // Note that we use the br's condition as the select condition.
            // That keeps the order of the two conditions as it was originally.
            br->condition =
              builder.makeSelect(br->condition, curr->condition, zero);
          }
          br->finalize();
          replaceCurrent(Builder(*getModule()).dropIfConcretelyTyped(br));
          anotherCycle = true;
        }
      }

      // if (condition-A) { if (condition-B) .. }
      //   =>
      // if (condition-A ? condition-B : 0) { .. }
      //
      // This replaces an if, which is 3 bytes, with a select plus a zero, which
      // is also 3 bytes. The benefit is that the select may be faster, and also
      // further optimizations may be possible on the select.
      if (auto* child = curr->ifTrue->dynCast<If>()) {
        if (child->ifFalse) {
          return;
        }
        // If running the child's condition unconditionally is too expensive,
        // give up.
        if (tooCostlyToRunUnconditionally(getPassOptions(), child->condition)) {
          return;
        }
        // Of course we can't do this if the inner if's condition has side
        // effects, as we would then execute those unconditionally.
        if (EffectAnalyzer(getPassOptions(), *getModule(), child->condition)
              .hasSideEffects()) {
          return;
        }
        Builder builder(*getModule());
        curr->condition = builder.makeSelect(
          child->condition, curr->condition, builder.makeConst(int32_t(0)));
        curr->ifTrue = child->ifTrue;
      }
    }
    // TODO: if-else can be turned into a br_if as well, if one of the sides is
    //       a dead end we handle the case of a returned value to a local.set
    //       later down, see visitLocalSet.
  }

  // override scan to add a pre and a post check task to all nodes
  static void scan(RemoveUnusedBrs* self, Expression** currp) {
    self->pushTask(visitAny, currp);

    auto* iff = (*currp)->dynCast<If>();

    if (iff) {
      if (iff->condition->type == Type::unreachable) {
        // avoid trying to optimize this, we never reach it anyhow
        return;
      }
      self->pushTask(doVisitIf, currp);
      if (iff->ifFalse) {
        // we need to join up if-else control flow, and clear after the
        // condition
        self->pushTask(scan, &iff->ifFalse);
        // safe the ifTrue flow, we'll join it later
        self->pushTask(saveIfTrue, currp);
      }
      self->pushTask(scan, &iff->ifTrue);
      self->pushTask(clear, currp); // clear all flow after the condition
      self->pushTask(scan, &iff->condition);
    } else {
      super::scan(self, currp);
    }
  }

  // optimizes a loop. returns true if we made changes
  bool optimizeLoop(Loop* loop) {
    // if a loop ends in
    // (loop $in
    //   (block $out
    //     if (..) br $in; else br $out;
    //   )
    // )
    // then our normal opts can remove the break out since it flows directly out
    // (and later passes make the if one-armed). however, the simple analysis
    // fails on patterns like
    //     if (..) br $out;
    //     br $in;
    // which is a common way to do a while (1) loop (end it with a jump to the
    // top), so we handle that here. Specifically we want to conditionalize
    // breaks to the loop top, i.e., put them behind a condition, so that other
    // code can flow directly out and thus brs out can be removed. (even if
    // the change is to let a break somewhere else flow out, that can still be
    // helpful, as it shortens the logical loop. it is also good to generate
    // an if-else instead of an if, as it might allow an eqz to be removed
    // by flipping arms)
    if (!loop->name.is()) {
      return false;
    }
    auto* block = loop->body->dynCast<Block>();
    if (!block) {
      return false;
    }
    // does the last element break to the top of the loop?
    auto& list = block->list;
    if (list.size() <= 1) {
      return false;
    }
    auto* last = list.back()->dynCast<Break>();
    if (!last || !ExpressionAnalyzer::isSimple(last) ||
        last->name != loop->name) {
      return false;
    }
    // last is a simple break to the top of the loop. if we can conditionalize
    // it, it won't block things from flowing out and not needing breaks to do
    // so.
    Index i = list.size() - 2;
    Builder builder(*getModule());
    while (1) {
      auto* curr = list[i];
      if (auto* iff = curr->dynCast<If>()) {
        // let's try to move the code going to the top of the loop into the
        // if-else
        if (!iff->ifFalse) {
          // we need the ifTrue to break, so it cannot reach the code we want to
          // move
          if (iff->ifTrue->type == Type::unreachable) {
            iff->ifFalse = stealSlice(builder, block, i + 1, list.size());
            iff->finalize();
            block->finalize();
            return true;
          }
        } else {
          // this is already an if-else. if one side is a dead end, we can
          // append to the other, if there is no returned value to concern us

          // can't be, since in the middle of a block
          assert(!iff->type.isConcrete());

          // ensures the first node is a block, if it isn't already, and merges
          // in the second, either as a single element or, if a block, by
          // appending to the first block. this keeps the order of operations in
          // place, that is, the appended element will be executed after the
          // first node's elements
          auto blockifyMerge = [&](Expression* any,
                                   Expression* append) -> Block* {
            Block* block = nullptr;
            if (any) {
              block = any->dynCast<Block>();
            }
            // if the first isn't a block, or it's a block with a name (so we
            // might branch to the end, and so can't append to it, we might skip
            // that code!) then make a new block
            if (!block || block->name.is()) {
              block = builder.makeBlock(any);
            } else {
              assert(!block->type.isConcrete());
            }
            auto* other = append->dynCast<Block>();
            if (!other) {
              block->list.push_back(append);
            } else {
              for (auto* item : other->list) {
                block->list.push_back(item);
              }
            }
            block->finalize();
            return block;
          };

          if (iff->ifTrue->type == Type::unreachable) {
            iff->ifFalse = blockifyMerge(
              iff->ifFalse, stealSlice(builder, block, i + 1, list.size()));
            iff->finalize();
            block->finalize();
            return true;
          } else if (iff->ifFalse->type == Type::unreachable) {
            iff->ifTrue = blockifyMerge(
              iff->ifTrue, stealSlice(builder, block, i + 1, list.size()));
            iff->finalize();
            block->finalize();
            return true;
          }
        }
        return false;
      } else if (auto* brIf = curr->dynCast<Break>()) {
        // br_if is similar to if.
        if (brIf->condition && !brIf->value && brIf->name != loop->name) {
          if (i == list.size() - 2) {
            // there is the br_if, and then the br to the top, so just flip them
            // and the condition
            brIf->condition = builder.makeUnary(EqZInt32, brIf->condition);
            last->name = brIf->name;
            brIf->name = loop->name;
            return true;
          } else {
            // there are elements in the middle,
            //   br_if $somewhere (condition)
            //   (..more..)
            //   br $in
            // we can convert the br_if to an if. this has a cost, though,
            // so only do it if it looks useful, which it definitely is if
            //  (a) $somewhere is straight out (so the br out vanishes), and
            //  (b) this br_if is the only branch to that block (so the block
            //      will vanish)
            if (brIf->name == block->name &&
                BranchUtils::BranchSeeker::count(block, block->name) == 1) {
              // note that we could drop the last element here, it is a br we
              // know for sure is removable, but telling stealSlice to steal all
              // to the end is more efficient, it can just truncate.
              list[i] =
                builder.makeIf(brIf->condition,
                               builder.makeBreak(brIf->name),
                               stealSlice(builder, block, i + 1, list.size()));
              block->finalize();
              return true;
            }
          }
        }
        return false;
      }
      // if there is control flow, we must stop looking
      if (EffectAnalyzer(getPassOptions(), *getModule(), curr)
            .transfersControlFlow()) {
        return false;
      }
      if (i == 0) {
        return false;
      }
      i--;
    }
  }

  bool sinkBlocks(Function* func) {
    struct Sinker : public PostWalker<Sinker> {
      bool worked = false;

      void visitBlock(Block* curr) {
        // If the block has a single child which is a loop, and the block is
        // named, then it is the exit for the loop. It's better to move it into
        // the loop, where it can be better optimized by other passes. Similar
        // logic for ifs: if the block is an exit for the if, we can move the
        // block in, consider for example:
        //    (block $label
        //     (if (..condition1..)
        //      (block
        //       (br_if $label (..condition2..))
        //       (..code..)
        //      )
        //     )
        //    )
        // After also merging the blocks, we have
        //    (if (..condition1..)
        //     (block $label
        //      (br_if $label (..condition2..))
        //      (..code..)
        //     )
        //    )
        // which can be further optimized later.
        if (curr->name.is() && curr->list.size() == 1) {
          if (auto* loop = curr->list[0]->dynCast<Loop>()) {
            curr->list[0] = loop->body;
            loop->body = curr;
            curr->finalize(curr->type);
            loop->finalize();
            replaceCurrent(loop);
            worked = true;
          } else if (auto* iff = curr->list[0]->dynCast<If>()) {
            // The label can't be used in the condition.
            if (BranchUtils::BranchSeeker::count(iff->condition, curr->name) ==
                0) {
              // We can move the block into either arm, if there are no uses in
              // the other.
              Expression** target = nullptr;
              if (!iff->ifFalse || BranchUtils::BranchSeeker::count(
                                     iff->ifFalse, curr->name) == 0) {
                target = &iff->ifTrue;
              } else if (BranchUtils::BranchSeeker::count(iff->ifTrue,
                                                          curr->name) == 0) {
                target = &iff->ifFalse;
              }
              if (target) {
                curr->list[0] = *target;
                *target = curr;
                // The block used to contain the if, and may have changed type
                // from unreachable to none, for example, if the if has an
                // unreachable condition but the arm is not unreachable.
                curr->finalize();
                iff->finalize();
                replaceCurrent(iff);
                worked = true;
                // Note that the type might change, e.g. if the if condition is
                // unreachable but the block that was on the outside had a
                // break.
              }
            }
          }
        }
      }
    } sinker;

    sinker.doWalkFunction(func);
    if (sinker.worked) {
      ReFinalize().walkFunctionInModule(func, getModule());
      return true;
    }
    return false;
  }

  // GC-specific optimizations. These are split out from the main code to keep
  // things as simple as possible.
  bool optimizeGC(Function* func) {
    if (!getModule()->features.hasGC()) {
      return false;
    }

    struct Optimizer : public PostWalker<Optimizer> {
      bool worked = false;

      void visitBrOn(BrOn* curr) {
        // Ignore unreachable BrOns which we cannot improve anyhow. Note that
        // we must check the ref field manually, as we may be changing types as
        // we go here. (Another option would be to use a TypeUpdater here
        // instead of calling ReFinalize at the very end, but that would be more
        // complex and slower.)
        if (curr->type == Type::unreachable ||
            curr->ref->type == Type::unreachable) {
          return;
        }

        // First, check for a possible null which would prevent optimizations on
        // null checks.
        // TODO: Look into using BrOnNonNull here, to replace a br_on_func whose
        // input is (ref null func) with br_on_non_null (as only the null check
        // would be needed).
        // TODO: Use the fallthrough to determine in more cases that we
        // definitely have a null.
        auto refType = curr->ref->type;
        if (refType.isNullable() &&
            (curr->op == BrOnNull || curr->op == BrOnNonNull)) {
          return;
        }

        if (curr->op == BrOnNull) {
          assert(refType.isNonNullable());
          // This cannot be null, so the br is never taken, and the non-null
          // value flows through.
          replaceCurrent(curr->ref);
          worked = true;
          return;
        }
        if (curr->op == BrOnNonNull) {
          assert(refType.isNonNullable());
          // This cannot be null, so the br is always taken.
          replaceCurrent(
            Builder(*getModule()).makeBreak(curr->name, curr->ref));
          worked = true;
          return;
        }

        // Check if the type is the kind we are checking for.
        auto result = GCTypeUtils::evaluateCastCheck(refType, curr->castType);
        if (curr->op == BrOnCastFail) {
          result = GCTypeUtils::flipEvaluationResult(result);
        }

        if (result == GCTypeUtils::Success) {
          // The cast succeeds, so we can switch from BrOn to a simple br that
          // is always taken.
          replaceCurrent(
            Builder(*getModule()).makeBreak(curr->name, curr->ref));
          worked = true;
        } else if (result == GCTypeUtils::Failure) {
          // The cast fails, so the branch is never taken, and the value just
          // flows through.
          replaceCurrent(curr->ref);
          worked = true;
        }
        // TODO: Handle SuccessOnlyIfNull and SuccessOnlyIfNonNull.
      }
    } optimizer;

    optimizer.setModule(getModule());
    optimizer.doWalkFunction(func);

    // If we removed any BrOn instructions, that might affect the reachability
    // of the things they used to break to, so update types.
    if (optimizer.worked) {
      ReFinalize().walkFunctionInModule(func, getModule());
      return true;
    }
    return false;
  }

  void doWalkFunction(Function* func) {
    // multiple cycles may be needed
    do {
      anotherCycle = false;
      super::doWalkFunction(func);
      assert(ifStack.empty());
      // flows may contain returns, which are flowing out and so can be
      // optimized
      for (Index i = 0; i < flows.size(); i++) {
        auto* flow = (*flows[i])->dynCast<Return>();
        if (!flow) {
          continue;
        }
        if (!flow->value) {
          // return => nop
          ExpressionManipulator::nop(flow);
        } else {
          // return with value => value
          *flows[i] = flow->value;
        }
        anotherCycle = true;
      }
      flows.clear();
      // optimize loops (we don't do it while tracking flows, as they can
      // interfere)
      for (auto* loop : loops) {
        anotherCycle |= optimizeLoop(loop);
      }
      loops.clear();
      if (anotherCycle) {
        ReFinalize().walkFunctionInModule(func, getModule());
      }
      if (sinkBlocks(func)) {
        anotherCycle = true;
      }
      if (optimizeGC(func)) {
        anotherCycle = true;
      }
    } while (anotherCycle);

    // thread trivial jumps
    struct JumpThreader : public ControlFlowWalker<JumpThreader> {
      // map of all value-less breaks and switches going to a block (and not a
      // loop)
      std::map<Block*, std::vector<Expression*>> branchesToBlock;

      bool worked = false;

      void visitBreak(Break* curr) {
        if (!curr->value) {
          if (auto* target = findBreakTarget(curr->name)->dynCast<Block>()) {
            branchesToBlock[target].push_back(curr);
          }
        }
      }
      void visitSwitch(Switch* curr) {
        if (!curr->value) {
          auto names = BranchUtils::getUniqueTargets(curr);
          for (auto name : names) {
            if (auto* target = findBreakTarget(name)->dynCast<Block>()) {
              branchesToBlock[target].push_back(curr);
            }
          }
        }
      }
      void visitBlock(Block* curr) {
        auto& list = curr->list;
        if (list.size() == 1 && curr->name.is()) {
          // if this block has just one child, a sub-block, then jumps to the
          // former are jumps to us, really
          if (auto* child = list[0]->dynCast<Block>()) {
            // the two blocks must have the same type for us to update the
            // branch, as otherwise one block may be unreachable and the other
            // concrete, so one might lack a value
            if (child->name.is() && child->name != curr->name &&
                child->type == curr->type) {
              redirectBranches(child, curr->name);
            }
          }
        } else if (list.size() == 2) {
          // if this block has two children, a child-block and a simple jump,
          // then jumps to child-block can be replaced with jumps to the new
          // target
          auto* child = list[0]->dynCast<Block>();
          auto* jump = list[1]->dynCast<Break>();
          if (child && child->name.is() && jump &&
              ExpressionAnalyzer::isSimple(jump)) {
            redirectBranches(child, jump->name);
          }
        }
      }

      void redirectBranches(Block* from, Name to) {
        auto& branches = branchesToBlock[from];
        for (auto* branch : branches) {
          if (BranchUtils::replacePossibleTarget(branch, from->name, to)) {
            worked = true;
          }
        }
        // if the jump is to another block then we can update the list, and
        // maybe push it even more later
        if (auto* newTarget = findBreakTarget(to)->dynCast<Block>()) {
          for (auto* branch : branches) {
            branchesToBlock[newTarget].push_back(branch);
          }
        }
      }

      void finish(Function* func) {
        if (worked) {
          // by changing where brs go, we may change block types etc.
          ReFinalize().walkFunctionInModule(func, getModule());
        }
      }
    };
    JumpThreader jumpThreader;
    jumpThreader.setModule(getModule());
    jumpThreader.walkFunction(func);
    jumpThreader.finish(func);

    // perform some final optimizations
    struct FinalOptimizer : public PostWalker<FinalOptimizer> {
      bool shrink;
      PassOptions& passOptions;

      bool needUniqify = false;

      FinalOptimizer(PassOptions& passOptions) : passOptions(passOptions) {}

      void visitBlock(Block* curr) {
        // if a block has an if br else br, we can un-conditionalize the latter,
        // allowing the if to become a br_if.
        // * note that if not in a block already, then we need to create a block
        //   for this, so not useful otherwise
        // * note that this only happens at the end of a block, as code after
        //   the if is dead
        // * note that we do this at the end, because un-conditionalizing can
        //   interfere with optimizeLoop()ing.
        auto& list = curr->list;
        for (Index i = 0; i < list.size(); i++) {
          auto* iff = list[i]->dynCast<If>();
          if (!iff || !iff->ifFalse) {
            // if it lacked an if-false, it would already be a br_if, as that's
            // the easy case
            continue;
          }
          auto* ifTrueBreak = iff->ifTrue->dynCast<Break>();
          if (ifTrueBreak && !ifTrueBreak->condition &&
              canTurnIfIntoBrIf(iff->condition,
                                ifTrueBreak->value,
                                passOptions,
                                *getModule())) {
            // we are an if-else where the ifTrue is a break without a
            // condition, so we can do this
            ifTrueBreak->condition = iff->condition;
            ifTrueBreak->finalize();
            list[i] = Builder(*getModule()).dropIfConcretelyTyped(ifTrueBreak);
            ExpressionManipulator::spliceIntoBlock(curr, i + 1, iff->ifFalse);
            continue;
          }
          // otherwise, perhaps we can flip the if
          auto* ifFalseBreak = iff->ifFalse->dynCast<Break>();
          if (ifFalseBreak && !ifFalseBreak->condition &&
              canTurnIfIntoBrIf(iff->condition,
                                ifFalseBreak->value,
                                passOptions,
                                *getModule())) {
            ifFalseBreak->condition =
              Builder(*getModule()).makeUnary(EqZInt32, iff->condition)