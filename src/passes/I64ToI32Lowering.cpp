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
      Name lowName = oldFunc->getLocalName(i);
      Name highName = makeHighName(lowName);
      Type paramType = oldFunc->getLocalType(i);
      auto builderFunc =
        (i < oldFunc->getVarIndexBase())
          ? Builder::addParam
          : static_cast<Index (*)(Function*, Name, Type)>(Builder::addVar);
      if (paramType == Type::i64) {
        builderFunc(func, lowName, Type::i32);
        builderFunc(func, highName, Type::i32);
        indexMap[i] = newIdx;
        newIdx += 2;
      } else {
        builderFunc(func, lowName, paramType);
        indexMap[i] = newIdx++;
      }
    }
    nextTemp = func->getNumLocals();
    PostWalker<I64ToI32Lowering>::doWalkFunction(func);
  }

  void visitFunction(Function* func) {
    if (func->imported()) {
      return;
    }
    if (func->getResults() == Type::i64) {
      func->setResults(Type::i32);
      // body may not have out param if it ends with control flow
      if (hasOutParam(func->body)) {
        TempVar highBits = fetchOutParam(func->body);
        TempVar lowBits = getTemp();
        LocalSet* setLow = builder->makeLocalSet(lowBits, func->body);
        GlobalSet* setHigh = builder->makeGlobalSet(
          INT64_TO_32_HIGH_BITS, builder->makeLocalGet(highBits, Type::i32));
        LocalGet* getLow = builder->makeLocalGet(lowBits, Type::i32);
        func->body = builder->blockify(setLow, setHigh, getLow);
      }
    }
    int idx = 0;
    for (size_t i = func->getNumLocals(); i < nextTemp; i++) {
      Name tmpName("i64toi32_i32$" + std::to_string(idx++));
      builder->addVar(func, tmpName, tempTypes[i]);
    }
  }

  template<typename T>
  using BuilderFunc = std::function<T*(std::vector<Expression*>&, Type)>;

  // Fixes up a call. If we performed fixups, returns the call; otherwise
  // returns nullptr;
  template<typename T>
  T* visitGenericCall(T* curr, BuilderFunc<T> callBuilder) {
    if (handleUnreachable(curr)) {
      return nullptr;
    }
    bool fixed = false;
    std::vector<Expression*> args;
    for (auto* e : curr->operands) {
      args.push_back(e);
      if (hasOutParam(e)) {
        TempVar argHighBits = fetchOutParam(e);
        args.push_back(builder->makeLocalGet(argHighBits, Type::i32));
        fixed = true;
      }
    }
    if (curr->type != Type::i64) {
      auto* ret = callBuilder(args, curr->type);
      replaceCurrent(ret);
      return fixed ? ret : nullptr;
    }
    TempVar lowBits = getTemp();
    TempVar highBits = getTemp();
    auto* call = callBuilder(args, Type::i32);
    LocalSet* doCall = builder->makeLocalSet(lowBits, call);
    LocalSet* setHigh = builder->makeLocalSet(
      highBits, builder->makeGlobalGet(INT64_TO_32_HIGH_BITS, Type::i32));
    LocalGet* getLow = builder->makeLocalGet(lowBits, Type::i32);
    Block* result = builder->blockify(doCall, setHigh, getLow);
    setOutParam(result, std::move(highBits));
    replaceCurrent(result);
    return call;
  }
  void visitCall(Call* curr) {
    if (curr->isReturn &&
        getModule()->getFunction(curr->target)->getResults() == Type::i64) {
      Fatal()
        << "i64 to i32 lowering of return_call values not yet implemented";
    }
    auto* fixedCall = visitGenericCall<Call>(
      curr, [&](std::vector<Expression*>& args, Type results) {
        return builder->makeCall(curr->target, args, results, curr->isReturn);
      });
    // If this was to an import, we need to call the legal version. This assumes
    // that legalize-js-interface has been run before.
    if (fixedCall && getModule()->getFunction(fixedCall->target)->imported()) {
      fixedCall->target =
        std::string("legalfunc$") + fixedCall->target.toString();
      return;
    }
  }

  void visitCallIndirect(CallIndirect* curr) {
    if (curr->isReturn && curr->heapType.getSignature().results == Type::i64) {
      Fatal()
        << "i64 to i32 lowering of return_call values not yet implemented";
    }
    visitGenericCall<CallIndirect>(
      curr, [&](std::vector<Expression*>& args, Type results) {
        std::vector<Type> params;
        for (const auto& param : curr->heapType.getSignature().params) {
          if (param == Type::i64) {
            params.push_back(Type::i32);
            params.push_back(Type::i32);
          } else {
            params.push_back(param);
          }
        }
        return builder->makeCallIndirect(curr->table,
                                         curr->target,
                                         args,
                                         Signature(Type(params), results),
                                         curr->isReturn);
      });
  }

  void visitLocalGet(LocalGet* curr) {
    const auto mappedIndex = indexMap[curr->index];
    // Need to remap the local into the new naming scheme, regardless of
    // the type of the local.
    curr->index = mappedIndex;
    if (curr->type != Type::i64) {
      return;
    }
    curr->type = Type::i32;
    TempVar highBits = getTemp();
    LocalSet* setHighBits = builder->makeLocalSet(
      highBits, builder->makeLocalGet(mappedIndex + 1, Type::i32));
    Block* result = builder->blockify(setHighBits, curr);
    replaceCurrent(result);
    setOutParam(result, std::move(highBits));
  }

  void lowerTee(LocalSet* curr) {
    TempVar highBits = fetchOutParam(curr->value);
    TempVar tmp = getTemp();
    curr->type = Type::i32;
    LocalSet* setLow = builder->makeLocalSet(tmp, curr);
    LocalSet* setHigh = builder->makeLocalSet(
      curr->index + 1, builder->makeLocalGet(highBits, Type::i32));
    LocalGet* getLow = builder->makeLocalGet(tmp, Type::i32);
    Block* result = builder->blockify(setLow, setHigh, getLow);
    replaceCurrent(result);
    setOutParam(result, std::move(highBits));
  }

  void visitLocalSet(LocalSet* curr) {
    const auto mappedIndex = indexMap[curr->index];
    // Need to remap the local into the new naming scheme, regardless of
    // the type of the local.  Note that lowerTee depends on this happening.
    curr->index = mappedIndex;
    if (!hasOutParam(curr->value)) {
      return;
    }
    if (curr->isTee()) {
      lowerTee(curr);
      return;
    }
    TempVar highBits = fetchOutParam(curr->value);
    auto* setHigh = builder->makeLocalSet(
      mappedIndex + 1, builder->makeLocalGet(highBits, Type::i32));
    Block* result = builder->blockify(curr, setHigh);
    replaceCurrent(result);
  }

  void visitGlobalGet(GlobalGet* curr) {
    if (!getFunction()) {
      return; // if in a global init, skip - we already handled that.
    }
    if (!originallyI64Globals.count(curr->name)) {
      return;
    }
    curr->type = Type::i32;
    TempVar highBits = getTemp();
    LocalSet* setHighBits = builder->makeLocalSet(
      highBits, builder->makeGlobalGet(makeHighName(curr->name), Type::i32));
    Block* result = builder->blockify(setHighBits, curr);
    replaceCurrent(result);
    setOutParam(result, std::move(highBits));
  }

  void visitGlobalSet(GlobalSet* curr) {
    if (!originallyI64Globals.count(curr->name)) {
      return;
    }
    if (handleUnreachable(curr)) {
      return;
    }
    TempVar highBits = fetchOutParam(curr->value);
    auto* setHigh = builder->makeGlobalSet(
      makeHighName(curr->name), builder->makeLocalGet(highBits, Type::i32));
    replaceCurrent(builder->makeSequence(curr, setHigh));
  }

  void visitLoad(Load* curr) {
    if (curr->type != Type::i64) {
      return;
    }
    assert(!curr->isAtomic && "64-bit atomic load not implemented");
    TempVar lowBits = getTemp();
    TempVar highBits = getTemp();
    TempVar ptrTemp = getTemp();
    LocalSet* setPtr = builder->makeLocalSet(ptrTemp, curr->ptr);
    LocalSet* loadHigh;
    if (curr->bytes == 8) {
      loadHigh = builder->makeLocalSet(
        highBits,
        builder->makeLoad(4,
                          curr->signed_,
                          curr->offset + 4,
                          std::min(uint32_t(curr->align), uint32_t(4)),
                          builder->makeLocalGet(ptrTemp, Type::i32),
                          Type::i32,
                          curr->memory));
    } else if (curr->signed_) {
      loadHigh = builder->makeLocalSet(
        highBits,
        builder->makeBinary(ShrSInt32,
                            builder->makeLocalGet(lowBits, Type::i32),
                            builder->makeConst(int32_t(31))));
    } else {
      loadHigh =
        builder->makeLocalSet(highBits, builder->makeConst(int32_t(0)));
    }
    curr->type = Type::i32;
    curr->bytes = std::min(curr->bytes, uint8_t(4));
    curr->align = std::min(uint32_t(curr->align), uint32_t(4));
    curr->ptr = builder->makeLocalGet(ptrTemp, Type::i32);
    Block* result =
      builder->blockify(setPtr,
                        builder->makeLocalSet(lowBits, curr),
                        loadHigh,
                        builder->makeLocalGet(lowBits, Type::i32));
    replaceCurrent(result);
    setOutParam(result, std::move(highBits));
  }

  void visitStore(Store* curr) {
    if (!hasOutParam(curr->value)) {
      return;
    }
    assert(curr->offset + 4 > curr->offset);
    assert(!curr->isAtomic && "atomic store not implemented");
    TempVar highBits = fetchOutParam(curr->value);
    uint8_t bytes = curr->bytes;
    curr->bytes = std::min(curr->bytes, uint8_t(4));
    curr->align = std::min(uint32_t(curr->align), uint32_t(4));
    curr->valueType = Type::i32;
    if (bytes == 8) {
      TempVar ptrTemp = getTemp();
      LocalSet* setPtr = builder->makeLocalSet(ptrTemp, curr->ptr);
      curr->ptr = builder->makeLocalGet(ptrTemp, Type::i32);
      curr->finalize();
      Store* storeHigh =
        builder->makeStore(4,
                           curr->offset + 4,
                           std::min(uint32_t(curr->align), uint32_t(4)),
                           builder->makeLocalGet(ptrTemp, Type::i32),
                           builder->makeLocalGet(highBits, Type::i32),
                           Type::i32,
                           curr->memory);
      replaceCurrent(builder->blockify(setPtr, curr, storeHigh));
    }
  }

  void visitAtomicRMW(AtomicRMW* curr) {
    if (handleUnreachable(curr)) {
      return;
    }
    if (curr->type != Type::i64) {
      return;
    }
    // We cannot break this up into smaller operations as it must be atomic.
    // Lower to an instrinsic function that wasm2js will implement.
    TempVar lowBits = getTemp();
    TempVar highBits = getTemp();
    auto* getLow = builder->makeCall(
      ABI::wasm2js::ATOMIC_RMW_I64,
      {builder->makeConst(int32_t(curr->op)),
       builder->makeConst(int32_t(curr->bytes)),
       builder->makeConst(int32_t(curr->offset)),
       curr->ptr,
       curr->value,
       builder->makeLocalGet(fetchOutParam(curr->value), Type::i32)},
      Type::i32);
    auto* getHigh =
      builder->makeCall(ABI::wasm2js::GET_STASHED_BITS, {}, Type::i32);
    auto* setLow = builder->makeLocalSet(lowBits, getLow);
    auto* setHigh = builder->makeLocalSet(highBits, getHigh);
    auto* finalGet = builder->makeLocalGet(lowBits, Type::i32);
    auto* result = builder->makeBlock({setLow, setHigh, finalGet});
    setOutParam(result, std::move(highBits));
    replaceCurrent(result);
  }

  void visitAtomicCmpxchg(AtomicCmpxchg* curr) {
    assert(curr->type != Type::i64 && "64-bit AtomicCmpxchg not implemented");
  }

  void visitAtomicWait(AtomicWait* curr) {
    // The last parameter is an i64, so we cannot leave it as it is
    replaceCurrent(builder->makeCall(
      ABI::wasm2js::ATOMIC_WAIT_I32,
      {builder->makeConst(int32_t(curr->offset)),
       curr->ptr,
       curr->expected,
       curr->timeout,
       builder->makeLocalGet(fetchOutParam(curr->timeout), Type::i32)},
      Type::i32));
  }

  void visitConst(Const* curr) {
    if (!getFunction()) {
      return; // if in a global init, skip - we already handled that.
    }
