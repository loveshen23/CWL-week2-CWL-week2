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

// Poppify.cpp - Transform Binaryen IR to Poppy IR.
//
// Poppy IR represents stack machine code using normal Binaryen IR types by
// imposing the following constraints:
//
//  1. Function bodies and children of control flow (except If conditions) must
//     be blocks.
//
//  2. Blocks may have any Expressions as children. The sequence of instructions
//     in a block follows the same validation rules as in WebAssembly. That
//     means that any expression may have a concrete type, not just the final
//     expression in the block.
//
//  3. All other children must be Pops, which are used to determine the input
//     stack type of each instruction. Pops may not have `unreachable` type.
//     Pops must correspond to the results of previous expressions or block
//     inputs in a stack discipline.
//
//  4. Only control flow structures and instructions that have polymorphic
//     unreachable behavior in WebAssembly may have unreachable type. Blocks may
//     be unreachable when they are not branch targets and when they have an
//     unreachable child. Note that this means a block may be unreachable even
//     if it would otherwise have a concrete type, unlike in Binaryen IR. For
//     example, this block could have unreachable type in Poppy IR but would
//     have to have type i32 in Binaryen IR:
//
//       (block
//         (unreachable)
//         (i32.const 1)
//       )
//
// As an example of Poppification, the following Binaryen IR Function:
//
//   (func $foo (result i32)
//    (i32.add
//     (i32.const 42)
//     (i32.const 5)
//    )
//   )
//
// would look like this in Poppy IR:
//
//   (func $foo (result i32)
//    (block
//     (i32.const 42)
//     (i32.const 5)
//     (i32.add
//      (pop i32)
//      (pop i32)
//     )
//    )
//   )
//
// Notice that the sequence of instructions in the block is now identical to the
// sequence of instructions in a WebAssembly binary. Also note that Poppy IR's
// validation rules are largely additional on top of the normal Binaryen IR
// validation rules, with the only exceptions being block body validation and
// block unreachability rules.
//

#include "ir/names.h"
#include "ir/properties.h"
#include "ir/stack-utils.h"
#include "ir/utils.h"
#include "pass.h"
#include "wasm-stack.h"

namespace wasm {

namespace {

// Generate names for the elements of tuple globals
Name getGlobalElem(Module* module, Name global, Index i) {
  return Names::getValidGlobalName(*module,
                                   global.toString() + '$' + std::to_string(i));
}

struct Poppifier : BinaryenIRWriter<Poppifier> {
  // Collects instructions to be inserted into a block at a certain scope, as
  // well as what kind of scope it is, which determines how the instructions are
  // inserted.
  struct Scope {
    enum Kind { Func, Block, Loop, If, Else, Try, Catch } kind;
    std::vector<Expression*> instrs;
    Scope(Kind kind) : kind(kind) {}
  };

  Module* module;
  Builder builder;
  std::vector<Scope> scopeStack;

  // Maps tuple locals to the new locals that will hold their elements
  std::unordered_map<Index, std::vector<Index>> tupleVars;

  // Records the scratch local to be used for tuple.extracts of each type
  std::unordered_map<Type, Index> scratchLocals;

  Poppifier(Function* func, Module* module);

  Index getScratchLocal(Type type);

  // Replace `expr`'s children with Pops of the correct type.
  void poppify(Expression* expr);

  // Pops the current scope off the scope stack and replaces `expr` with a block
  // co