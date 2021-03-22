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
// Apply more specific subtypes to type fields where possible, where all the
// writes to that field in the entire program allow doing so.
//

#include "ir/lubs.h"
#include "ir/struct-utils.h"
#include "ir/type-updating.h"
#include "ir/utils.h"
#include "pass.h"
#include "support/unique_deferring_queue.h"
#include "wasm-type.h"
#include "wasm.h"

namespace wasm {

namespace {

// We use a LUBFinder to track field info, which includes the best LUB possible
// as well as relevant nulls (nulls force us to keep the type nullable).
using FieldInfo = LUBFinder;

struct FieldInfoScanner
  : public StructUtils::StructScanner<FieldInfo, FieldInfoScanner> {
  std::unique_ptr<Pass> create() override {
    return std::make_unique<FieldInfoScanner>(functionNewInfos,
                                              functionSetGetInfos);
  }

  FieldInfoScanner(
    StructUtils::FunctionStructValuesMap<FieldInfo>& functionNewInfos,
    StructUtils::FunctionStructValuesMap<FieldInfo>& functionSetGetInfos)
    : StructUtils::StructScanner<FieldInfo, FieldInfoScanner>(
        functionNewInfos, functionSetGetInfos) {}

  void noteExpression(Expression* expr,
                      HeapType type,
                      Index index,
                      FieldInfo& info) {
    info.note(expr->type);
  }

  void
  noteDefault(Type fieldType, HeapType type, Index index, FieldInfo& info) {
    // Default values do not affect what the heap type of a field can be turned
    // into. Note them, however, as they force us to keep the type nullable.
    if (fieldType.isRef()) {
      info.note(Type(fieldType.getHeapType().getBottom(), Nullable));
    }
  }

  void noteCopy(HeapType type, Index index, FieldInfo& info) {
    // Copies do not add any type requirements at all: the type will always be
    // read and written to a place with the same type.
  }

  void noteRead(HeapType type, Index index, FieldInfo& info) {
    // Nothing to do for a read, we just care about written values.
  }

  Properties::FallthroughBehavior getFallthroughBehavior() {
    // Looking at fallthrough values may be dangerous here, because it ignores
    // intermediate steps. Consider this:
    //
    // (struct.set $T 0
    //   (local.tee
    //     (struct.get $T 0)))
    //
    // This is a copy of a field to itself - normally something we can ignore
    // (see above). But in this case, if we refine the field then that will
    // update the struct.get, but the local.tee will still have the old type,
    // which may not be refined enough. We could in theory always fix this up
    // using casts later, but those casts may be expensive (especially ref.casts
    // as opposed to ref.as_non_null), so for now just ignore tee fallthroughs.
    // TODO: investigate more
    return Properties::FallthroughBehavior::NoTeeBrIf;
  }
};

struct TypeRefining : public Pass {
  // Only affects GC type declarations and struct.gets.
  bool requiresNonNullableLocalFixups() override { return false; }

  StructUtils::StructValuesMap<FieldInfo> finalInfos;

  void run(Module*