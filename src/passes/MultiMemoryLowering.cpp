/*
 * Copyright 2022 WebAssembly Community Group participants
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
// Condensing a module with multiple memories into a module with a single memory
// for browsers that don’t support multiple memories.
//
// This pass also disables multi-memories so that the target features section in
// the emitted module does not report the use of MultiMemories. Disabling the
// multi-memories feature also prevents later passes from adding additional
// memories.
//
// The offset computation in function maybeMakeBoundsCheck is not precise
// according to the spec. In the spec offsets do not overflow as
// twos-complement, but i32.add does. Concretely, a load from address 1000 with
// offset 0xffffffff should actually trap, as the combined number is greater
// than 32 bits. But with an add, 1000 + 0xffffffff = 999 due to overflow, which
// would not trap. In theory we could compute like the spec, by expanding the
// i32s to i64s and adding there (where we won't overflow), but we don't have
// i128s to handle i64 overflow.
//
// The Atomic instructions memory.atomic.wait and memory.atomic.notify, have
// browser engine implementations that predate the still-in-progress threads
// spec (https://github.com/WebAssembly/threads). And whether or not
// atomic.notify should trap for out-of-bounds addresses remains an open issue
// (https://github.com/WebAssembly/threads/issues/105). For now, we are using
// the same semantics as v8, which is to bounds check all Atomic instructions
// the same way and trap for out-of-bounds.

#include "ir/module-utils.h"
#include "ir/names.h"
#include "wasm-builder.h"
#include <pass.h>
#include <wasm.h>

namespace wasm {

struct MultiMemoryLowering : public Pass {
  Module* wasm = nullptr;
  // The name of the single memory that exists after this pass is run
  Name combinedMemory;
  // The type of the single memory
  Type pointerType;
  // Used to indicate the type of the single memory when creating instructions
  // (memory.grow, memory.size) for that memory
  Builder::MemoryInfo memoryInfo;
  // If the combined memory is shared
  bool isShared;
  // If the combined memory is imported
  bool isImported;
  // If the combined memory is exported
  bool isExported = false;
  // If the combined memory should be imported, the following two
  // properties will be set
  Name module;
  Name base;
  // The initial page size of the combined memory
  Address totalInitialPages;
  // The max page size of the combined memory
  Address totalMaxPages;
  // There is no offset for the first memory, so offsetGlobalNames will always
  // have a size that is one less than the count of memories at the time this
  // pass is run. Use helper getOffsetGlobal(Index) to index the vector
  // conveniently without having to manipulate the index directly
  std::vector<Name> offsetGlobalNames;
  // Maps from the name of the memory to its index as seen in the
  // module->memories vector
  std::unordered_map<Name, Index> memoryIdxMap;
  // A vector of the memory size function names that were created proactively
  // for each memory
  std::vector<Name> memorySizeNames;
  // A vector of the memory grow functions that were created proactively for
  // each memory
  std::vector<Name> memoryGrowNames;

  bool checkBounds = false;

  MultiMemoryLowering(bool checkBounds) : checkBounds(checkBounds) {}

  struct Replacer : public WalkerPass<PostWalker<Replacer>> {
    MultiMemoryLowering& parent;
    Builder builder;
    Replacer(MultiMemoryLowering& parent, Module& wasm)
      : parent(parent), builder(wasm) {}
    // Avoid visiting the custom functions added by the parent pass
    // MultiMemoryLowering
    void walkFunction(Function* func) {
      for (Name funcName : parent.memorySizeNames) {
        if (funcName == func->name) {
          return;
        }
      }
      for (Name funcName : parent.memoryGrowNames) {
        if (funcName == func->name) {
          return;
        }
      }
      super::walkFunction(func);
    }

    void visitMemoryGrow(MemoryGrow* curr) {
      auto idx = parent.memoryIdxMap.at(curr->memory);
      Name funcName = parent.memoryGrowNames[idx];
      replaceCurrent(builder.makeCall(funcName, {curr->delta}, curr->type));
    }

    void visitMemorySize(MemorySize* curr) {
      auto idx = parent.memoryIdxMap.at(curr->memory);
      Name funcName = parent.memorySizeNames[idx];
      replaceCurrent(builder.makeCall(funcName, {}, curr->type));
    }

    Expression* addOffsetGlobal(Expression* toExpr, Name memory) {
      auto memoryIdx = parent.memoryIdxMap.at(memory);
      auto offsetGlobal = parent.getOffsetGlobal(memoryIdx);
      Expression* returnExpr;
      if (offsetGlobal) {
        returnExpr = builder.makeBinary(
          Abstract::getBinary(parent.pointerType, Abstract::Add),
          builder.makeGlobalGet(offsetGlobal, parent.pointerType),
          toExpr);
      } else {
        returnExpr = toExpr;
      }
      return returnExpr;
    }

    Expression* makeAddGtuTrap(Expression* leftOperand,
                               Expression* rightOperand,
                               Expression* limit) {
      Expression* gtuTrap = builder.makeIf(
        builder.makeBinary(
          Abstract::getBinary(parent.pointerType, Abstract::GtU),
          builder.makeBinary(
            Abstract::getBinary(parent.pointerType, Abstract::Add),
            leftOperand,
            rightOperand),
          limit),
        builder.makeUnreachable());
      return gtuTrap;
    }

    Expression* makeAddGtuMemoryTrap(Expression* leftOperand,
                                     Expression* rightOperand,
                                     Name memory) {
      auto memoryIdx = parent.memoryIdxMap.at(memory);
      Name memorySizeFunc = parent.memorySizeNames[memoryIdx];
      Expression* gtuMemoryTrap = makeAddGtuTrap(
        leftOperand,
        rightOperand,
        builder.makeCall(memorySizeFunc, {}, parent.pointerType));
      return gtuMemoryTrap;
    }

    template<typename T>
    Expression* makePtrBoundsCheck(T* curr, Index ptrIdx, Index bytes) {
      Expression* boundsCheck = makeAddGtuMemoryTrap(
        builder.makeBinary(
          // ptr + offset (ea from wasm spec) + bit width
          Abstract::getBinary(parent.pointerType, Abstract::Add),
          builder.makeLocalGet(ptrIdx, parent.pointerType),
          builder.makeConstPtr(curr->offset, parent.pointerType)),
        builder.makeConstPtr(bytes, parent.pointerType),
        curr->memory);
      return boundsCheck;
    }

    Expression* makeDataSegmentBoundsCheck(MemoryInit* curr,
                                           Index sizeIdx,
                                           Index offsetIdx) {
      auto& segment = parent.wasm->dataSegments[curr->segment];
      Expression* addGtuTrap = makeAddGtuTrap(
        builder.makeLocalGet(offsetIdx, parent.pointerType),
        builder.makeLocalGet(sizeIdx, parent.pointerType),
        builder.makeConstPtr(segment->data.size(), parent.pointerType));
      return addGtuTrap;
    }

    template<typename T> Expression* getPtr(T* curr, Index bytes) {
      Expression* ptrValue = addOffsetGlobal(curr->ptr, curr->memory);
      if (parent.checkBounds) {
        Index ptrIdx = Builder::addVar(getFunction(), parent.pointerType);
        Expression* ptrSet = builder.makeLocalSet(ptrIdx, ptrValue);
        Expression* boundsCheck = makePtrBoundsCheck(curr, ptrIdx, bytes);
        Expression* ptrGet = builder.makeLocalGet(ptrIdx, parent.pointerType);
        return builder.makeBlock({ptrSet, boundsCheck, ptrGet});
      }

      return ptrValue;
    }

    template<typename T>
    Expression* getDest(T* curr,
                        Name memory,
                        Index sizeIdx = Index(-1),
                        Expression* localSet = nullptr,
                        Expression* additionalCheck = nullptr) {
      Expression* destValue = addOffsetGlobal(curr->dest, memory);

      if (parent.checkBounds) {
        Expression* sizeSet = builder.makeLocalSet(sizeIdx, curr->size);
        Index destIdx = Builder::addVar(getFunction(), parent.pointerType);
        Expression* destSet = builder.makeLocalSet(destIdx, destValue);
        Expression* boundsCheck = makeAddGtuMemoryTrap(
          builder.makeLocalGet(destIdx, parent.pointerType),
          builder.makeLocalGet(sizeIdx, parent.pointerType),
          memory);
        std::vector<Expression*> exprs = {
          destSet, localSet, sizeSet, boundsCheck};
        if (additionalCheck) {
          exprs.push_back(additionalCheck);
        }
        Expression* destGet = builder.makeLocalGet(destIdx, parent.pointerType);
        exprs.push_back(destGet);
        return builder.makeBlock(exprs);
      }

      return destValue;
    }

    Expression* getSource(MemoryCopy* curr,
                          Index sizeIdx = Index(-1),
                          Index sourceIdx = Index(-1)) {
      Expression* sourceValue =
        addOffsetGlobal(curr->source, curr->sourceMemory);

      if (parent.checkBounds) {
        Expression* boundsCheck = makeAddGtuMemoryTrap(
          builder.makeLocalGet(sourceIdx, parent.pointerType),
          builder.makeLocalGet(sizeIdx, parent.pointerType),
          curr->sourceMemory);
        Expression* sourceGet =
          builder.makeLocalGet(sourceIdx, parent.pointerType);
        std::vector<Expression*> exprs = {boundsCheck, sourceGet};
        return builder.makeBlock(exprs);
      }

      return sourceValue;
    }

    void visitMemoryInit(MemoryInit* curr) {
      if (parent.checkBounds) {
        Index offsetIdx = Builder::addVar(getFunction(), parent.pointerType);
        Index sizeIdx = Builder::addVar(getFunction(), parent.pointerType);
        curr->dest =
          getDest(curr,
                  curr->memory,
                  sizeIdx,
                  builder.makeLocalSet(offsetIdx, curr->offset),
                  makeDataSegmentBoundsCheck(curr, sizeIdx, offsetIdx));
        curr->offset = builder.makeLocalGet(offsetIdx, parent.pointerType);
        curr->size = builder.makeLocalGet(sizeIdx, parent.pointerType);
      } else {
        curr->dest = getDest(curr, curr->memory);
      }
      setMemory(curr);
    }

    void visitMemoryCopy(MemoryCopy* curr) {
      if (parent.checkBounds) {
        Index sourceIdx = Builder::addVar(getFunction(), parent.pointerType);
        Index sizeIdx = Builder::addVar(getFunction(), parent.pointerType);
        curr->dest = getDest(curr,
                             curr->destMemory,
                             sizeIdx,
                             builder.makeLocalSet(sourceIdx, curr->source));
        curr->source = getSource(curr, sizeIdx, sourceIdx);
        curr->size = builder.makeLocalGet(sizeIdx, parent.pointerType);
      } else {
        curr->dest = getDest(curr, curr->destMemory);
        curr->source = getSource(curr);
      }
      curr->destMemory = parent.combinedMemory;
      curr->sourceMemory = parent.combinedMemory;
    }

    void visitMemoryFill(MemoryFill* curr) {
      if (parent.checkBounds) {
        Index valueIdx = Builder::addVar(getFunction(), parent.pointerType);
        Index sizeIdx = Builder::addVar(getFunction(), parent.pointerType);
        curr->dest = getDest(curr,
                             curr->memory,
                             sizeIdx,
                             builder.makeLocalSet(valueIdx, curr->value));
        curr->value = builder.makeLocalGet(valueIdx, parent.pointerType);
        curr->size = builder.makeLocalGet(sizeIdx, parent.pointerType);
      } else {
        curr->dest = getDest(curr, curr->memory);
      }
      setMemory(curr);
    }

    template<typename T> void setMemory(T* curr) {
      curr->memory = parent.combinedMemory;
    }

    void visitLoad(Load* curr) {
      curr->ptr = getPtr(curr, curr->bytes);
      setMemory(curr);
    }

    void visitStore(Store* curr) {
      curr->ptr = getPtr(curr, curr->bytes);
      setMemory(curr);
    }

    void visitSIMDLoad(SIMDLoad* curr) {
      curr->ptr = getPtr(curr, curr->getMemBytes());
      setMemory(curr);
    }

    void visitSIMDLoadSplat(SIMDLoad* curr) {
      curr->ptr = getPtr(curr, curr->getMemBytes());
      setMemory(curr);
    }

    void visitSIMDLoadExtend(SIMDLoad* curr) {
      curr->ptr = getPtr(curr, curr->getMemBytes());
      setMemory(curr);
    }

    void visitSIMDLoadZero(SIMDLoad* curr) {
      curr->ptr = getPtr(curr, curr->getMemBytes());
      setMemory(curr);
    }

    void visitSIMDLoadStoreLane(SIMDLoadStoreLane* curr) {
      curr->ptr = getPtr(curr, curr->getMemBytes());
      setMemory(curr);
    }

    void visitAtomicRMW(AtomicRMW* curr) {
      curr->ptr = getPtr(curr, curr->bytes);
      setMemory(curr);
    }

    void visitAtomicCmpxchg(AtomicCmpxchg* curr) {
      curr->ptr = getPtr(curr, curr->bytes);
      setMemory(