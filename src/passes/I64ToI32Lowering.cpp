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
// Lowers i64s to i32s by splitting variables and arguments
// into pairs of i32s. i64 return values are lowered by
// returning the low half and storing the high half into a
// global.
//

#include "abi/js.h"
#include "ir/flat.h"
#include "ir/iteration.h"
#include "ir/memory-utils.h"
#include "ir/module-utils.h"
#include "ir/names.h"
#include "pass.h"
#include "support/istring.h"
#include "support/name.h"
#include "wasm-builder.h"
#include "wasm.h"
#include <algorithm>

namespace wasm {

static Name makeHighName(Name n) { return n.toString() + "$hi"; }

struct I64ToI32Lowering : public WalkerPass<PostWalker<I64ToI32Lowering>> {
  struct TempVar {
    TempVar(Index idx, Type ty, I64ToI32Lowering& pass)
      : idx(idx), pass(pass), moved(false), ty(ty) {}

    TempVar(TempVar&& other)
      : idx(other), pass(other.pass), moved(false), ty(other.ty) {
      assert(!other.moved);
      other.moved = true;
    }

    TempVar& operator=(TempVar&& rhs) {
      assert(!rhs.moved);
      // free overwritten idx
      if (!moved) {
        freeIdx();
      }
      idx = rhs.idx;
      rhs.moved = true;
      moved = false;
      return *this;
    }

    ~TempVar() {
      if (!moved) {
        freeIdx();
      }
    }

    bool operator==(const TempVar& rhs) {
      assert(!moved && !rhs.moved);
      return idx == rhs.idx;
    }

    operator Index() {
      assert(!moved);
      return idx;
    }

    // disallow copying
    TempVar(const TempVar&) = delete;
    TempVar& operator=(const TempVar&) = delete;

  private:
    void freeIdx() {
      auto& freeList = pass.freeTemps[ty.getBasic()];
      assert(std::find(freeList.begin(), freeList.end(), idx) ==
             freeList.end());
      freeList.push_back(idx);
    }

    Index idx;
    I64ToI32Lowering& pass;
    bool moved; // since C++ will still destruct moved-from values
    Type ty;
  };

  // false since function types need to be lowered
  // TODO: allow module-level transformations in parallel passes
  bool isFunctionParallel() override { return false; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<I64ToI32Lowering>();
  }

  void doWalkModule(Module* module) {
    if (!builder) {
      builder = make_unique<Builder>(*module);
    }
    // add new globals for high bits
    for (size_t i = 0, globals = module->globals.size(); i < globals; ++i) {
      auto* curr = module->globals[i].get();
      if (curr->type != Type::i64) {
        continue;
      }
      originallyI64Globals.insert(curr->name);
      curr->type = Type::i32;
      auto high = builder->makeGlobal(makeHighName(curr->name),
                                      Type::i32,
                                      builder->makeConst(int32_t(0)),
                                      Builder::Mutable);
      if (curr->imported()) {
        Fatal() << "TODO: imported i64 globals";
      } else {
        if (auto* c = curr->init->dynCast<Const>()) {
          uint64_t value = c->value.geti64();
          c->value = Literal(uint32_t(value));
          c->type = Type::i32;
          high->init = builder->makeConst(uint32_t(value >> 32));
        } else if (auto* get = curr->init->dynCast<GlobalGet>()) {
          high->init =
            builder->makeGlobalGet(makeHighName(get->name), Type::i32);
        } else {
          WASM_UNREACHABLE("unexpected expression type");
        }
        curr->init->type = Type::i32;
      }
      module->addGlobal(std::move(high));
    }

    // For functions that return 64-bit values, we use this global variable
    // to return the high 32 bits.
    auto* highBits = new Global();
    highBits->type = Type::i32;
    highBits->name = INT64_TO_32_HIGH_BITS;
    highBits->init = builder->makeConst(int32_t(0));
    highBits->mutable_ = true;
    module->addGlobal(highBits);
    PostWalker<I64ToI32Lowering>::doWalkModule(module);
  }

  void doWalkFunction(Function* func) {
    Flat::verifyFlatness(func);
    // create builder here if this is first entry to module for this object
    if (!builder) {
      builder = make_unique<Builder>(*getModule());
    }
    indexMap.clear();
    highBitVars.clear();
    freeTemps.clear();
    Module temp;
    auto* oldFunc = ModuleUtils::copyFunction(func, temp);
    func->setParams(Type::none);
    func->vars.clear();
    func->localNames.clear();
    func->localIndices.clear();
    Index newIdx = 0;
    Names::ensureNames(oldFunc);
    for (Index i = 0; i < oldFunc->getNumLocals(); ++i) {
      assert(oldFunc->hasLocalName(i));
      Name lowName = oldFunc->get