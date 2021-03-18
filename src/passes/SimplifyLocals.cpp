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
// Locals-related optimizations
//
// This "sinks" local.sets, pushing them to the next local.get where possible,
// and removing the set if there are no gets remaining (the latter is
// particularly useful in ssa mode, but not only).
//
// We also note where local.sets coalesce: if all breaks of a block set
// a specific local, we can use a block return value for it, in effect
// removing multiple local.sets and replacing them with one that the
// block returns to. Further optimization rounds then have the opportunity
// to remove that local.set as well. TODO: support partial traces; right
// now, whenever control flow splits, we invalidate everything.
//
// After this pass, some locals may be completely unused. reorder-locals
// can get rid of those (the operation is trivial there after it sorts by use
// frequency).
//
// This pass has options:
//
//   * Tee: allow teeing, i.e., sinking a local with more than one use,
//          and so after sinking we have a tee for the first use.
//   * Structure: create block and if return values, by merging the
//                internal local.sets into one on the outside,
//                that can itself then be sunk further.
//
// There is also an option to disallow nesting entirely, which disallows
// Tee and Structure from those 2 options, and also disallows any sinking
// operation that would create nesting. This keeps the IR flat while
// removing redundant locals.
//

#include "ir/equivalent_sets.h"
#include <ir/branch-utils.h>
#include <ir/effects.h>
#include <ir/find_all.h>
#include <ir/linear-execution.h>
#include <ir/local-utils.h>
#include <ir/manipulation.h>
#include <ir/utils.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm-traversal.h>
#include <wasm.h>

namespace wasm {

// Main class

template<bool allowTee = true,
         bool allowStructure = true,
         bool allowNesting = true>
struct SimplifyLocals
  : public WalkerPass<LinearExecutionWalker<
      SimplifyLocals<allowTee, allowStructure, allowNesting>>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<
      SimplifyLocals<allowTee, allowStructure, allowNesting>>();
  }

  // information for a local.set we can sink
  struct SinkableInfo {
    Expression** item;
    EffectAnalyzer effects;

    SinkableInfo(Expression** item, PassOptions& passOptions, Module& module)
      : item(item), effects(passOptions, module, *item) {}
  };

  // a list of sinkables in a linear execution trace
  using Sinkables = std::map<Index, SinkableInfo>;

  // locals in current linear execution trace, which we try to sink
  Sinkables sinkables;

  // Information about an exit from a block: the break, and the
  // sinkables. For the final exit from a block (falling off)
  // exitter is null.
  struct BlockBreak {
    Expression** brp;
    Sinkables sinkables;
  };

  // a list of all sinkable traces that exit a block. the last
  // is falling off the end, others are branches. this is used for
  // block returns
  std::map<Name, std::vector<BlockBreak>> blockBreaks;

  // blocks that we can't produce a block return value for them.
  // (switch target, or some other reason)
  std::set<Name> unoptimizableBlocks;

  // A stack of sinkables from the current traversal state. When
  // execution reaches an if-else, it splits, and can then
  // be merged on return.
  std::vector<Sinkables> ifStack;

  // whether we need to run an additional cycle
  bool anotherCycle;

  // whether this is the first cycle, in which we always disallow teeing
  bool firstCycle;

  // local => # of local.gets for it
  LocalGetCounter getCounter;

  // In rare cases we make a change to a type that requires a refinalize.
  bool refinalize = false;

  static void
  doNoteNonLinear(SimplifyLocals<allowTee, allowStructure, allowNesting>* self,
                  Expression** currp) {
    // Main processing.
    auto* curr = *currp;
    if (auto* br = curr->dynCast<Break>()) {
      if (br->value) {
        // value means the block already has a return value
        self->unoptimizableBlocks.insert(br->name);
      } else {
        self->blockBreaks[br->name].push_back(
          {currp, std::move(self->sinkables)});
      }
    } else if (curr->is<Block>()) {
      return; // handled in visitBlock
    } else if (curr->is<If>()) {
      assert(!curr->cast<If>()
                ->ifFalse); // if-elses are handled by doNoteIf* methods
    } else {
      // Not one of the recognized instructions, so do not optimize here: mark
      // all the targets as unoptimizable.
      // TODO optimize BrOn, Switch, etc.
      auto targets = BranchUtils::getUniqueTargets(curr);
      for (auto target : targets) {
        self->unoptimizableBlocks.insert(target);
      }
      // TODO: we could use this info to stop gathering data on these blocks
    }
    self->sinkables.clear();
  }

  static void doNoteIfCondition(
    SimplifyLocals<allowTee, allowStructure, allowNesting>* self,
    Expression** currp) {
    // we processed the condition of this if-else, and now control flow branches
    // into either the true or the false sides
    self->sinkables.clear();
  }

  static void
  doNoteIfTrue(SimplifyLocals<allowTee, allowStructure, allowNesting>* self,
               Expression** currp) {
    auto* iff = (*currp)->cast<If>();
    if (iff->ifFalse) {
      // We processed the ifTrue side of this if-else, save it on the stack.
      self->ifStack.push_back(std::move(self->sinkables));
    } else {
      // This is an if without an else.
      if (allowStructure) {
        self->optimizeIfReturn(iff, currp);
      }
      self->sinkables.clear();
    }
  }

  static void
  doNoteIfFalse(SimplifyLocals<allowTee, allowStructure, allowNesting>* self,
                Expression** currp) {
    // we processed the ifFalse side of this if-else, we can now try to
    // mere with the ifTrue side and optimize a return value, if possible
    auto* iff = (*currp)->cast<If>();
    assert(iff->ifFalse);
    if (allowStructure) {
      self->optimizeIfElseReturn(iff, currp, self->ifStack.back());
    }
    self->ifStack.pop_back();
    self->sinkables.clear();
  }

  void visitBlock(Block* curr) {
    bool hasBreaks = curr->name.is() && blockBreaks[curr->name].size() > 0;

    if (allowStructure) {
      optimizeBlockReturn(curr); // can modify blockBreaks
    }

    // post-block cleanups
    if (curr->name.is()) {
      if (unoptimizableBlocks.count(curr->name)) {
        sinkables.clear();
        unoptimizableBlocks.erase(curr->name);
      }

      if (hasBreaks) {
        // more than one path to here, so nonlinear
        sinkables.clear();
        blockBreaks.erase(curr->name);
      }
    }
  }

  void visitLoop(Loop* curr) {
    if (allowStructure) {
      optimizeLoopReturn(curr);
    }
  }

  void optimizeLocalGet(LocalGet* curr) {
    auto found = sinkables.find(curr->index);
    if (found != sinkables.end()) {
      auto* set = (*found->second.item)
                    ->template cast<LocalSet>(); // the set we may be sinking
      bool oneUse = firstCycle || getCounter.num[curr->index] == 1;
      // the set's value may be a get (i.e., the set is a copy)
      auto* get = set->value->template dynCast<LocalGet>();
      // if nesting is not allowed, and this might cause nesting, check if the
      // sink would cause such a thing
      if (!allowNesting) {
        // a get is always ok to sink
        if (!get) {
          assert(expressionStack.size() >= 2);
          assert(expressionStack[expressionStack.size() - 1] == curr);
          auto* parent = expressionStack[expressionStack.size() - 2];
          bool parentIsSet = parent->template is<LocalSet>();
          // if the parent of this get is a set, we can sink into the set's
          // value, it would not be nested.
          if (!parentIsSet) {
            return;
          }
        }
      }
      // we can optimize here
      if (!allowNesting && get && !oneUse) {
        // if we can't nest 's a copy with multiple uses, then we can't create
        // a tee, and we can't nop the origin, but we can at least switch to
        // the copied index, which may make the origin unneeded eventually.
        curr->index = get->index;
        anotherCycle = true;
        return;
      }
      // sink it, and nop the origin
      if (oneUse) {
        // with just one use, we can sink just the value
        this->replaceCurrent(set->value);

        // We are replacing a local.get with the value of the local.set. That
        // may require a refinalize in certain cases, like this:
        //
        //  (struct.get $X 0
        //    (local.get $x)
        //  )
        //
        // If we replace the local.get with a more refined type then the
        // struct.get may read a more refined type (if the subtype has a more
        // refined type for that particular field). Note that this cannot happen
        // in the other arm of this if-else, where we replace the local.get with
        // a tee, since tees have the type of the local, so no types change
        // there.
        if (set->value->type != curr->type) {
          refinalize = true;
        }
      } else {
        this->replaceCurrent(set);
        assert(!set->isTee());
        set->makeTee(this->getFunction()->getLocalType(set->index));
      }
      // reuse the local.get that is dying
      *found->second.item = curr;
      ExpressionManipulator::nop(curr);
      sinkables.erase(found);
      anotherCycle = true;
    }
  }

  void visitDrop(Drop* curr) {
    // collapse drop-tee into set, which can occur if a get was sunk into a tee
    auto* set = curr->value->dynCast<LocalSet>();
    if (set) {
      assert(set->isTee());
      set->makeSet();
      this->replaceCurrent(set);
    }
  }

  void checkInvalidations(EffectAnalyzer& effects) {
    // TODO: this is O(bad)
    std::vector<Index> invalidated;
    for (auto& [index, info] : sinkables) {
      if (effects.invalidates(info.effects)) {
        invalidated.push_back(index);
      }
    }
    for (auto index : invalidated) {
      sinkables.erase(index);
    }
  }

  // a full expression stack is used when !allowNesting, so that we can check if
  // a sink would cause nesting
  ExpressionStack expressionStack;

  static void
  visitPre(SimplifyLocals<allowTee, allowStructure, allowNesting>* self,
           Expression** currp) {
    Expression* curr = *currp;

    // Certain expressions cannot be sinked into 'try', and so at the start of
    // 'try' we forget about them.
    if (curr->is<Try>()) {
      std::vector<Index> invalidated;
      for (auto& [index, info] : self->sinkables) {
        // Expressions that may throw cannot be moved into a try (which might
        // catch them, unlike before the move).
        if (info.effects.throws()) {
          invalidated.push_back(index);
          continue;
        }
      }
      for (auto index : invalidated) {
        self->sinkables.erase(index);
      }
    }

    EffectAnalyzer effects(self->getPassOptions(), *self->getModule());
    if (effects.checkPre(curr)) {
      self->checkInvalidations(effects);
    }

    if (!allowNesting) {
      self->expressionStack.push_back(curr);
    }
  }

  static void
  visitPost(SimplifyLocals<allowTee, allowStructure, allowNesting>* self,
            Expression** currp) {
    // Handling invalidations in the case where the current node is a get
    // that we sink into is not trivial in general. In the simple case,
    // all current sinkables are compatible with each other (otherwise one
    // would have invalidated a previous one, and removed it). Given that, if
    // we sink one of the sinkables, then that new code cannot invalidate any
    // other sinkable - we've already compared them. However, a tricky case
    // is when a sinkable contains another sinkable,
    //
    //  (local.set $x
    //   (block (result i32)
    //    (A (local.get $y))
    //    (local.set $y B)
    //   )
    //  )
    //  (C (local.get $y))
    //  (D (local.get $x))
    //
    // If we sink the set of $y, we have
    //
    //  (local.set $x
    //   (block (result i32)
    //    (A (local.get $y))
    //    (nop)
    //   )
    //  )
    //  (C B)
    //  (D (local.get $x))
    //
    // There is now a risk that the set of $x should be invalidated, because
    // if we sink it then A may happen after B (imagine that B contains
    // something dangerous for that). To verify the risk, we could recursively
    // scan all of B, but that is less efficient. Instead, the key thing is
    // that if we sink out an inner part of a set, we should just leave further
    // work on it to a later iteration. This is achieved by checking for
    // invalidation on the original node, the local.get $y, which is guaranteed
    // to invalidate the parent whose inner part was removed (since the inner
    // part has a set, and the original node is a get of that same local).
    //
    // To implement this, if the current node is a get, note it and use it
    // for invalidations later down. We must note it since optimizing the get
    // may perform arbitrary changes to the graph, including reuse the get.

    Expression* original = *currp;

    LocalGet originalGet;

    if (auto* get = (*currp)->dynCast<LocalGet>()) {
      // Note: no visitor for LocalGet, so that we can handle it here.
      originalGet = *get;
      original = &originalGet;
      self->optimizeLocalGet(get);
    }

    // perform main LocalSet processing here, since we may be the result of
    // replaceCurrent, i.e., no visitor for LocalSet, like LocalGet above.
    auto* set = (*currp)->dynCast<LocalSet>();

    if (set) {
      // if we see a set that was already potentially-sinkable, then the
      // previous store is dead, leave just the value
      auto found = self->sinkables.find(set->index);
      if (found != self->sinkables.end()) {
        auto* previous = (*found->second.item)->template cast<LocalSet>();
        assert(!previous->isTee());
        auto* previousValue = previous->value;
        Drop* drop = ExpressionManipulator::convert<LocalSet, Drop>(previous);
        drop->value = previousValue;
        drop->finalize();
        self->sinkables.erase(found);
        self->anotherCycle = true;
      }
    }

    EffectAnalyzer effects(self->getPassOptions(), *self->getModule());
    if (effects.checkPost(original)) {
      self->checkInvalidations(effects);
    }

    if (set && self->canSink(set)) {
      Index index = set->index;
      assert(self->sinkables.count(index) == 0);
      self->sinkables.emplace(std::pair{
        index,
        SinkableInfo(currp, self->getPassOptions(), *self->getModule())});
    }

    if (!allowNesting) {
      self->expressionStack.pop_back();
    }
  }

  bool canSink(LocalSet* set) {
    // we can never move a tee
    if (set->isTee()) {
      return false;
    }
    // We cannot move expressions containing pops that are not enclosed in
    // 'catch', because 'pop' should follow right after 'catch'.
    F