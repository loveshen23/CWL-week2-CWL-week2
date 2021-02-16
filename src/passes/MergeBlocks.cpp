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
// Merges blocks to their parents.
//
// We merge both entire blocks when possible, as well as loop tails, like
//  (block
//   (loop $child
//    (br_if $child (..)
//    (call $foo)
//   )
//  )
// Here we can move the call into the outer block. Doing so may let
// the inner block become a single expression, and usually outer
// blocks are larger anyhow. (This also helps readability.)
//
// We also restructure blocks in order to enable such merging. For
// example,
//
//  (i32.store
//    (block
//      (call $foo)
//      (i32.load (i32.const 100))
//    )
//    (i32.const 0)
//  )
//
// can be transformed into
//
//  (block
//    (call $foo)
//    (i32.store
//      (block
//        (i32.load (i32.const 100))
//      )
//      (i32.const 0)
//    )
//  )
//
// after which the internal block can go away, and
// the new external block might be mergeable. This is always
// worth it if the internal block ends up with 1 item.
// For the second operand,
//
//  (i32.store
//    (i32.const 100)
//    (block
//      (call $foo)
//      (i32.load (i32.const 200))
//    )
//  )
//
// The order of operations requires that the first execute
// before. We can do the same operation, but only if the
// first has no side effects, or the code we are moving out
// has no side effects.
// If we can do this to both operands, we can generate a
// single outside block.
//

#include <ir/branch-utils.h>
#include <ir/effects.h>
#include <ir/iteration.h>
#include <ir/utils.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>

namespace wasm {

// Looks for reasons we can't remove the values from breaks to an origin
// For example, if there is a switch targeting us, we can't do it - we can't
// remove the value from other targets
struct ProblemFinder
  : public ControlFlowWalker<ProblemFinder,
                             UnifiedExpressionVisitor<ProblemFinder>> {
  Name origin;
  bool foundProblem = false;
  // count br_ifs, and dropped br_ifs. if they don't match, then a br_if flow
  // value is used, and we can't drop it
  Index brIfs = 0;
  Index droppedBrIfs = 0;
  PassOptions& passOptions;

  ProblemFinder(PassOptions& passOptions) : passOptions(passOptions) {}

  void visitExpression(Expression* curr) {
    if (auto* drop = curr->dynCast<Drop>()) {
      if (auto* br = drop->value->dynCast<Break>()) {
        if (br->name == origin && br->condition) {
          droppedBrIfs++;
        }
      }
      return;
    }

    if (auto* br = curr->dynCast<Break>()) {
      if (br->name == origin) {
        if (br->condition) {
          brIfs++;
        }
        // if the value has side effects, we can't remove it
        if (EffectAnalyzer(passOptions, *getModule(), br->value)
              .hasSideEffects()) {
          foundProblem = true;
        }
      }
      return;
    }

    // Any other branch type - switch, br_on, etc. - is not handled yet.
    BranchUtils::operateOnScopeNameUses(curr, [&](Name& name) {
      if (name == origin) {
        foundProblem = true;
      }
    });
  }

  bool found() {
    assert(brIfs >= droppedBrIfs);
    return foundProblem || brIfs > droppedBrIfs;
  }
};

// Drops values from breaks to an origin.
// While doing so it can create new blocks, so optimize blocks as well.
struct BreakValueDropper : public ControlFlowWalker<BreakValueDropper> {
  Name origin;
  PassOptions& passOptions;
  BranchUtils::BranchSeekerCache& branchInfo;

  BreakValueDropper(PassOptions& passOptions,
                    BranchUtils::BranchSeekerCache& branchInfo)
    : passOptions(passOptions), branchInfo(branchInfo) {}

  void visitBlock(Block* curr);

  void visitBreak(Break* curr) {
    if (curr->value && curr->name == origin) {
      Builder builder(*getModule());
      auto* value = curr->value;
      if (value->type == Type::unreachable) {
        // the break isn't even reached
        replaceCurrent(value);
        return;
      }
      curr->value = nullptr;
      curr->finalize();
      replaceCurrent(builder.makeSequence(builder.makeDrop(value), curr));
    }
  }

  void visitDrop(Drop* curr) {
    // if we dropped a br_if whose value we removed, then we are now dropping a
    // (block (drop value) (br_if)) with type none, which does not need a drop
    // likewise, unreachable does not need to be dropped, so we just leave drops
    // of concrete values
    if (!curr->value->type.isConcrete()) {
      replaceCurrent(curr->value);
    }
  }
};

static bool hasUnreachableChild(Block* block) {
  for (auto* test : block->list) {
    if (test->type == Type::unreachable) 