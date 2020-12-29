/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include "ir/branch-utils.h"
#include "ir/find_all.h"
#include "ir/utils.h"

namespace wasm {

static Type getValueType(Expression* value) {
  return value ? value->type : Type::none;
}

namespace {

// Handles a branch fixup for visitBlock: if the branch goes to the
// target name, give it a value which is unreachable.
template<typename T>
void handleBranchForVisitBlock(T* curr, Name name, Module* module) {
  if (BranchUtils::getUniqueTargets(curr).count(name)) {
    assert(!curr->value);
    Builder builder(*module);
    curr->value = builder.makeUnreachable();
  }
}

} // anonymous namespace

void ReFinalize::visitBlock(Block* curr) {
  if (curr->list.size() == 0) {
    curr->type = Type::none;
    return;
  }
  if (curr->name.is()) {
    auto iter = breakTypes.find(curr->name);
    if (iter != breakTypes.end()) {
      // Set the type to be a supertype of the branch types and the flowed-out
      // type. TODO: calculate proper LUBs to compute a new correct type in this
      // situation.
      auto& types = iter->second;
      types.insert(curr->list.back()->type);
      curr->type = Type::getLeastUpperBound(types);
      return;
    }
  }
  curr->type = curr->list.back()->type;
  if (curr->type == Type::unreachable) {
    return;
  }
  // type is none, but we might be unreachable
  if (curr->type == Type::none) {
    for (auto* child : curr->list) {
      if (child->type == Type::unreachable) {
        curr->type = Type::unreachable;
        break;
      }
    }
  }
}
void ReFinalize::visitIf(If* curr) { curr->finalize(); }
void ReFinalize::visitLoop(Loop* curr) { curr->finalize(); }
void ReFinalize::visitBreak(Break* curr) {
  curr->finalize();
  auto valueType = getValueType(curr->value);
  if (valueType == Type::unreachable) {
    replaceUntaken(curr->value, curr->condition);
  } else {
    updateBreakValueType(curr->name, valueType);
  }
}
void ReFinalize::visitSwitch(Switch* curr) {
  curr->finalize();
  auto valueType = getValueType(curr->value);
  if (valueType == Type::unreachable) {
    replaceUntaken(curr->value, curr->condition);
  } else {
    for (auto target : curr->targets) {
      updateBreakValueType(target, valueType);
    }
    updateBreakValueType(curr->default_, valueType);
  }
}
void ReFinalize::visitCall(Call* curr) { curr->finalize(); }
void ReFinalize::visitCallIndirect(CallIndirect* curr) { curr->finalize(); }
void ReFinalize::visitLocalGet(LocalGet* curr) { curr->finalize(); }
void ReFinalize::visitLocalSet(LocalSet* curr) { curr->finalize(); }
void ReFinalize::visitGlobalGet(GlobalGet* curr) { curr->finalize(); }
void ReFinalize::visitGlobalSet(GlobalSet* curr) { curr->f