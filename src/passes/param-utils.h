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

#ifndef wasm_ir_function_h
#define wasm_ir_function_h

#include "pass.h"
#include "support/sorted_vector.h"
#include "wasm.h"

// Helper code for passes that manipulate function parameters, specifically
// checking if they are used and removing them if so. This is closely tied to
// the internals of those passes, and so is not in /ir/ (it would be inside the
// pass .cpp file, but there is more than one).

namespace wasm::ParamUtils {

// Find which parameters are actually used in the function, that is, that the
// values arriving in the parameter are read. This ignores values set in the
// function, like this:
//
// function foo(x) {
//   x = 10;
//   bar(x); // read of a param index, but not the param value passed in.
// }
//
// This is an actual use:
//
// function foo(x) {
//   bar(x); // read of a param value
// }
std::unordered_set<Index> getUsedParams(Function* func);

// Try to remove a parameter from a set of funct