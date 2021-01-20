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

// Avoids reinterprets by using more loads: if we load a value and
// reinterpret it, we could have loaded it with the other type
// anyhow. This uses more locals and loads, so it is not generally
// beneficial, unless reinterprets are very costly (which is the case
// with wasm2js).

#include <ir/local-graph.h>
#include <ir/properties.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>

namespace wasm {

static bool canReplaceWithReinterpret(Load* load) {
  // We can replace a full-size load with a valid pointer with
  // a reinterpret of the same address. A partial load would see
  // more bytes and possibly invalid data, and an unreachable
  // pointer is just not interesting to handle.
  return load->type != Type::unreachable &&
         load->bytes == load->type.getByteSize();
}

static Load* getSingleLoad(LocalGraph* localGraph,
                           LocalGet* get,
                           const PassOptions& passOptions,
                           Module& module) {
  std::set<LocalGet*> seen;
  seen.insert(get);
  while (1) {
    auto& sets = localGraph->getSetses[get];
    if (sets.size() != 1) {
      return nullptr;
    }
    auto* set = *sets.begin();
    if (!set) {
      return nullptr;
    }
    auto* value = Properties::getFallthrough(set->value, passOptions, module);
    if (auto* parentGet = value->dynCast<LocalGet>()) {
      if (seen.emplace(parentGet).second) {
        get = parentGet;
        continue;
      }
      // We are in a cycle of gets, in unreachable code.
      return nullptr;
    }
    if (auto* load = value->dynCast<Load>()) {
      return load;
    }
    return nullptr;
  }
}

static bool isReinterpret(Unary* curr) {
  return curr->op == ReinterpretInt32 || curr->op == ReinterpretInt64 ||
         curr->op == ReinterpretFloat32 || curr->op == ReinterpretFloat64;
}

struct AvoidReinterprets : public WalkerPass<PostWalker<AvoidReinterprets>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<AvoidReinterprets>();
  }

  struct Info {
    // Info used when analyzing.
    bool reinterpreted;
    // Info used when optimizing.
    Index ptrLocal;
    Index reinterpretedLocal;
  };
  std::map<Load*, Info> infos;

  LocalGraph* localGraph;

  void doWalkFunction(Function* func) {
    // prepare
    LocalGraph localGraph_(func);
    localGraph = &localGraph_;
    // walk
    PostWalker<AvoidReinterprets>::doWalkFunction(func);
    // optimize
    optimize(func);
  }

  void visitUnary(Unary* curr) {
    if (isReinterpret(curr)) {
      if (auto* get = Properties::getFallthrough(
                        curr->value, getPassOptions(), *getModule())
                        ->dynCast<LocalGet>()) {
        if (auto* load =
              getSingleLoad(localGraph, get, getPassOptions(), *getModule())) {
          auto& info = infos[load];
          info.reinterpreted = true;
        }
      }
    }
  }

  void optimize(Function* func) {
    std::set<Load*> unoptimizables;
    for (auto& [load, info] : infos) {
      if (info.reinterpreted && canReplaceWithRein