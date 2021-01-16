/*
 * Copyright 2016 WebAssembly Community Group participants
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

#ifndef wasm_ir_properties_h
#define wasm_ir_properties_h

#include "ir/bits.h"
#include "ir/effects.h"
#include "ir/match.h"
#include "wasm.h"

namespace wasm::Properties {

inline bool isSymmetric(Binary* binary) {
  switch (binary->op) {
    case AddInt32:
    case MulInt32:
    case AndInt32:
    case OrInt32:
    case XorInt32:
    case EqInt32:
    case NeInt32:

    case AddInt64:
    case MulInt64:
    case AndInt64:
    case OrInt64:
    case XorInt64:
    case EqInt64:
    case NeInt64:

    case MinFloat32:
    case MaxFloat32:
    case EqFloat32:
    case NeFloat32:
    case MinFloat64:
    case MaxFloat64:
    case EqFloat64:
    case NeFloat64:
      return true;

    default:
      return false;
  }
}

inline bool isControlFlowStructure(Expression* curr) {
  return curr->is<Block>() || curr->is<If>() || curr->is<Loop>() ||
         curr->is<Try>();
}

// Check if an expression is a control flow construct with a name, which implies
// it may have breaks or delegates to it.
inline bool isNamedControlFlow(Expression* curr) {
  if (auto* block = curr->dynCast<Block>()) {
    return block->name.is();
  } else if (auto* loop = curr->dynCast<Loop>()) {
    return loop->name.is();
  } else if (auto* try_ = curr->dynCast<Try>()) {
    return try_->name.is();
  }
  return false;
}

// A constant expression is something like a Const: it has a fixed value known
// at compile time, and passes that propagate constants can try to propagate it.
// Constant expressions are also allowed in global initializers in wasm. Also
// when two constant expressions compare equal at compile time, their values at
// runtime will be equal as well. TODO: combine this with
// isValidInConstantExpression or find better names(#4845)
inline bool isSingleConstantExpression(const Expression* 