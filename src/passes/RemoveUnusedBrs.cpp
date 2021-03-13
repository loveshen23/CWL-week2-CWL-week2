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
                                         