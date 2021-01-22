/*
 * Copyright 2021 WebAssembly Community Group participants
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
// Find heap allocations that never escape the current function, and lower the
// allocation's data into locals. That is, avoid allocating a GC object, and
// instead use one local for each of its fields.
//
// To get a sense for what this pass does, here is an example to clarify. First,
// in pseudocode:
//
//   ref = new Int(42)
//   do {
//     ref.set(ref.get() + 1)
//   } while (import(ref.get())
//
// That is, we allocate an int on the heap and use it as a counter.
// Unnecessarily, as it could be a normal int on the stack.
//
// Wat:
//
//   (module
//    ;; A boxed integer: an entire struct just to hold an int.
//    (type $boxed-int (struct (field (mut i32))))
//
//    (import "env" "import" (func $import (param i32) (result i32)))
//
//    (func "example"
//     (local $ref (ref null $boxed-int))
//
//     ;; Allocate a boxed integer of 42 and save the reference to it.
//     (local.set $ref
//      (struct.new $boxed-int
//       (i32.const 42)
//      )
//     )
//
//     ;; Increment the integer in a loop, looking for some condition.
//     (loop $loop
//      (struct.set $boxed-int 0
//       (local.get $ref)
//       (i32.add
//        (struct.get $boxed-int 0
//         (local.get $ref)
//        )
//        (i32.const 1)
//       )
//      )
//      (br_if $loop
//       (call $import
//        (struct.get $boxed-int 0
//         (local.get $ref)
//        )
//       )
//      )
//     )
//    )
//   )
//
// Before this pass, the optimizer could do essentially nothing with this. Even
// with this pass, running -O1 has no effect, as the pass is only used in -O2+.
// However, running --heap2local -O1 leads to this:
//
//    (func $0
//     (local $0 i32)
//     (local.set $0
//      (i32.const 42)
//     )
//     (loop $loop
//      (br_if $loop
//       (call $import
//        (local.tee $0
//         (i32.add
//          (local.get $0)
//          (i32.const 1)
//         )
//        )
//       )
//      )
//     )
//    )
//
// All the GC heap operations have been removed, and we just have a plain int
// now, allowing a bunch of other opts to run.
//
// For us to replace an allocation with locals, we need to prove two things:
//
//  * It must not escape from the function. If it escapes, we must pass out a
//    reference anyhow. (In theory we could do a whole-program transformation
//    to replace the reference with parameters in some cases, but inlining can
//    hopefully let us optimize most cases.)
//  * It must be used "exclusively", without overlap. That is, we cannot
//    handle the case where a local.get might return our allocation, but might
//    also get some other value. We also cannot handle a select where one arm
//    is our allocation and another is something else. If the use is exclusive
//    then we have a simple guarantee of being able to replace the heap
//    allocation with the locals.
//
// Non-exclusive uses are optimizable too, but they require more work and add
// overhead. For example, here is a non-exclusive use:
//
//   var x;
//   if (..) {
//     x = new Something(); // the allocation we want to optimize
//   } else {
//     x = something_else;
//   }
//   // 'x' here is not used exclusively by our allocation
//   return x.field0;
//
// To optimize x.field0, we'd need to check if it contains our allocation or
// not, perhaps marking a boolean as true when it is, then doing an if on that
// local, etc.:
//
//   var x_is_our_alloc; // whether x is our allocation
//   var x; // keep x around for when it is not our allocation
//   var x_field0; // the value of field0 on x, when x is our allocation
//   if (..) {
//     x_field0 = ..default value for the type..
//     x_is_our_alloc = true;
//   } else {
//     x = something_else;
//     x_is_our_alloc = false;
//   }
//   return x_is_our_alloc ? x_field0 : x.field0;
//
// (node splitting/code duplication is another possible approach). On the other
// hand, if the allocation is used exclusively in all places (the if-else above
// does not have an else any more) then we can do this:
//
//   var x_field0; // the value of field0 on x
//   if (..) {
//     x_field0 = ..default value for the type..
//   }
//   return x_field0;
//
// This optimization focuses on such cases.
//

#include "ir/branch-utils.h"
#include "ir/find_all.h"
#include "ir/local-graph.h"
#include "ir/parents.h"
#include "ir/properties.h"
#include "ir/type-updating.h"
#include "ir/utils.h"
#include "pass.h"
#include "support/unique_deferring_queue.h"
#include "wasm-builder.h"
#include "wasm.h"

namespace wasm {

namespace {

struct Heap2LocalOptimizer {
  Function* func;
  Module* module;
  const PassOptions& passOptions;

  // To find allocations that do not escape, we must track locals so that we
  // can see how allocations flow from sets to gets and so forth.
  // TODO: only scan reference types
  LocalGraph localGraph;

  // To find what escapes, we need to follow where values flow, both up to
  // parents, and via branches.
  Parents parents;
  BranchUtils::BranchTargets branchTargets;

  Heap2LocalOptimizer(Function* func,
                      Module* module,
                      const PassOptions& passOptions)
    : func(func), module(module), passOptions(passOptions), localGraph(func),
      parents(func->body), branchTargets(func->body) {
    // We need to track what each set influences, to see where its value can
    // flow to.
    localGraph.computeSetInfluences();

    // All the allocations in the function.
    // TODO: Arrays (of constant size) as well.
    FindAll<StructNew> allocations(func->body);

    for (auto* allocation : allocations.list) {
      // The point of this optimization is to replace heap allocations with
      // locals, so we must be able to place the data in locals.
      if (!canHandleAsLocals(allocation->type)) {
        continue;
      }

      convertToLocals(allocation);
    }
  }

  bool canHandleAsLocals(Type type) {
    if (type == Type::unreachable) {
      return false;
    }
    auto& fields = type.getHeapType().getStruct().fields;
    for (auto field : fields) {
      if (!TypeUpdating::canHandleAsLocal(field.type)) {
        return false;
      }
      if (field.isPacked()) {
        // TODO: support packed fields by adding coercions/truncations.
        return false;
      }
    }
    return true;
  }

  // Handles the rewriting that we do to perform the optimization. We store the
  // data that rewriting will need here, while we analyze, and then if we can do
  // the optimization, we tell it to run.
  //
  // TODO: Doing a single rewrite walk at the end would be more efficient, but
  //       it would need to be more complex.
  struct Rewriter : PostWalker<Rewriter> {
    StructNew* allocation;
    Function* func;
    Builder builder;
    const FieldList& fields;

    Rewriter(StructNew* allocation, Function* func, Module* module)
      : allocation(allocation), func(func), builder(*module),
        fields(allocation->type.getHeapType().getStruct().fields) {}

    // We must track all the local.sets that write the allocation, to verify
    // exclusivity.
    std::unordered_set<LocalSet*> sets;

    // All the expressions we reached during the flow analysis. That is exactly
    // all the places where our allocation is used. We track these so that we
    // can fix them up at the end, if the optimization ends up possible.
    std::unordered_set<Expression*> reached;

    // Maps indexes in the struct to the local index that will replace them.
    std::vector<Index> localIndexes;

    void applyOptimization() {
      // Allocate locals to store the allocation's fields in.
      for (auto field : fields) {
        localIndexes.push_back(builder.addVar(func, field.type));
      }

      // Replace the things we need to using the visit* methods.
      walk(func->body);
    }

    // Rewrite the code in visit* methods. The general approach taken is to
    // replace the allocation with a null reference (which may require changing
    // types in some places, like making a block return value nullable), and to
    // remove all uses of it as much as possible, using the information we have
    // (for example, when our allocation reaches a RefAsNonNull we can simply
    // remove that operation as we know it would not throw). Some things are
    // left to other passes, like getting rid of dropped code without side
    // effects.

    // Adjust the type that flows through an expression, updating that type as
    // necessary.
    void adjustTypeFlowingThrough(Expression* cu