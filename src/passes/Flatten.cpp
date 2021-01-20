/*
 * Copyright 2017 WebAssembly Community Group participants
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
// Flattens code into "Flat IR" form. See ir/flat.h.
//
// TODO: handle non-nullability. for example:
//
//      (module
//       (type $none (func))
//       (func $foo
//        (drop
//         (block (result funcref (ref $none))
//          (tuple.make
//           (ref.null func)
//           (ref.func $foo)
//          )
//         )
//        )
//       )
//      )
//
// The tuple has a non-nullable type, and so it cannot be set to a local. We
// would need to split up the tuple and reconstruct it later, but that would
// require allowing tuple operations in more nested places than Flat IR allows
// today. For now, error on this; eventually changes in the spec regarding
// null-nullability may make this easier.

#include <ir/branch-utils.h>
#include <ir/effects.h>
#include <ir/eh-utils.h>
#include <ir/flat.h>
#include <ir/properties.h>
#include <ir/utils.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>

namespace wasm {

// We use the following algorithm: we maintain a list of "preludes", code
// that runs right before an expression. When we visit an expression we
// must handle it and its preludes. If the expression has side effects,
// we reduce it to a local.get and add a prelude for that. We then handle
// the preludes, by moving them to the parent or handling them directly.
// we can move them to the parent if the parent is not a control flow
// structure. Otherwise, if the parent is a control flow structure, it
// will incorporate the preludes of its children accordingly.
// As a result, when we reach a node, we know its children have no
// side effects (they have been moved to a prelude), or we are a
// control flow structure (which allows children with side effects,
// e.g. a return as a block element).
// Once exception is that we allow an (unreachable) node, which is used
// when we move something unreachable to another place, and need a
// placeholder. We will never reach that (unreachable) anyhow
struct Flatten
  : public WalkerPass<
      ExpressionStackWalker<Flatten, UnifiedExpressionVisitor<Flatten>>> {
  bool isFunctionParallel() override { return true; }

  // Flattening splits the original locals into a great many other ones, losing
  // track of the originals that DWARF refers to.
  // FIXME DWARF updating does not handle local changes yet.
  bool invalidatesDWARF() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<Flatten>();
  }

  // For each expression, a bunch of expressions that should execute right
  // before it
  std::unordered_map<Expression*, std::vector<Expression*>> preludes;

  // Break values are sent through a temp local
  std::unordered_map<Name, Index> breakTemps;

  void visitExpression(Expression* curr) {
    std::vector<Expression*> ourPreludes;
    Builder builder(*getModule());

    // Nothing to do for constants and nop.
    if (Properties::isConstantExpression(curr) || curr->is<Nop>()) {
      return;
    }

    if (Properties::isControlFlowStructure(curr)) {
      // handle control flow explicitly. our children do not have control flow,
      // but they do have preludes which we need to set up in the right place

      // no one should have given us preludes, they are on the children
      assert(preludes.find(curr) == preludes.end());

      if (auto* block = curr->dynCast<Block>()) {
        // make a new list, where each item's preludes are added before it
        ExpressionList newList(getModule()->allocator);
        for (auto* item : block->list) {
          auto iter = preludes.find(item);
          if (iter != preludes.end()) {
            auto& itemPreludes = iter->second;
            for (auto* prelude : itemPreludes) {
              newList.push_back(prelude);
            }
            itemPreludes.clear();
          }
          newList.push_back(item);
        }
        block->list.swap(newList);
        // remove a block return value
        auto type = block->type;
        if (type.isConcrete()) {
          // if there is a temp index for breaking to the block, use that
          Index temp;
          auto iter = breakTemps.find(block->name);
          if (iter !=