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
// Optimize types at the global level, altering fields etc. on the set of heap
// types defined in the module.
//
//  * Immutability: If a field has no struct.set, it can become immutable.
//  * Fields that are never read from can be removed entirely.
//

#include "ir/effects.h"
#include "ir/localize.h"
#include "ir/ordering.h"
#include "ir/struct-utils.h"
#include "ir/subtypes.h"
#include "ir/type-updating.h"
#include "ir/utils.h"
#include "pass.h"
#include "wasm-builder.h"
#include "wasm-type.h"
#include "wasm.h"

namespace wasm {

namespace {

// Information about usage of a field.
struct FieldInfo {
  bool hasWrite = false;
  bool hasRead = false;

  void noteWrite() { hasWrite = true; }
  void noteRead() { hasRead = true; }

  bool combine(const FieldInfo& other) {
    bool changed = false;
    if (!hasWrite && other.hasWrite) {
      hasWrite = true;
      changed = true;
    }
    if (!hasRead && other.hasRead) {
      hasRead = true;
      changed = true;
    }
    return changed;
  }
};

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
      