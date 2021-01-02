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

#ifndef wasm_ir_effects_h
#define wasm_ir_effects_h

#include "ir/intrinsics.h"
#include "pass.h"
#include "wasm-traversal.h"

namespace wasm {

// Analyze various possible effects.

class EffectAnalyzer {
public:
  EffectAnalyzer(const PassOptions& passOptions, Module& module)
    : ignoreImplicitTraps(passOptions.ignoreImplicitTraps),
      trapsNeverHappen(passOptions.trapsNeverHappen),
      funcEffectsMap(passOptions.funcEffectsMap), module(module),
      features(module.features) {}

  EffectAnalyzer(const PassOptions& passOptions,
                 Module& module,
                 Expression* ast)
    : EffectAnalyzer(passOptions, module) {
    walk(ast);
  }

  EffectAnalyzer(const PassOptions& passOptions, Module& module, Function* func)
    : EffectAnalyzer(passOptions, module) {
    walk(func);
  }

  bool ignoreImplicitTraps;
  bool trapsNeverHappen;
  std::shared_ptr<FuncEffectsMap> funcEffectsMap;
  Module& module;
  FeatureSet features;

  // Walk an expression and all its children.
  void walk(Expression* ast) {
    pre();
    InternalAnalyzer(*this).walk(ast);
    post();
  }

  // Visit an expression, without any children.
  void visit(Expression* ast) {
    pre();
    InternalAnalyzer(*this).visit(ast);
    post();
  }

  // Walk an entire function body. This will ignore effects that are not
  // noticeable from the perspective of the caller, that is, effects that are
  // only noticeable during the call, but "vanish" when the call stack is
  // unwound.
  void walk(Function* func) {
    walk(func->body);

    // We can ignore branching out of the function body - this can only be
    // a return, and that is only noticeable in the function, not outside.
    branchesOut = false;

    // When the function exits, changes to locals cannot be noticed any more.
    localsWritten.clear();
    localsRead.clear();
  }

  // Core effect tracking

  // Definitely branches out of this expression, or does a return, etc.
  // breakTargets tracks individual targets, which we may eventually see are
  // internal, while this is set when we see something that will definitely
  // not be internal, or is otherwise special like an infinite loop (which
  // does not technically branch "out", but it does break the normal assumption
  // of control flow proceeding normally).
  bool branchesOut = false;
  bool calls = false;
  std::set<Index> localsRead;
  std::set<Index> localsWritten;
  std::set<Name> mutableGlobalsRead;
  std::set<Name> globalsWritten;
  bool readsMemory = false;
  bool writesMemory = false;
  bool readsTable = false;
  bool writesTable = false;
  // TODO: More specific type-based ali