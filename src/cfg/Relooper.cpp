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

#include "Relooper.h"

#include <stdlib.h>
#include <string.h>

#include <list>
#include <stack>
#include <string>

#include "ir/branch-utils.h"
#include "ir/utils.h"
#include "parsing.h"

namespace CFG {

template<class T, class U>
static bool contains(const T& container, const U& contained) {
  return !!container.count(contained);
}

#ifdef RELOOPER_DEBUG
static void PrintDebug(const char* Format, ...);
#define DebugDump(x, ...) Debugging::Dump(x, __VA_ARGS__)
#else
#define PrintDebug(x, ...)
#define DebugDump(x, ...)
#endif

// Rendering utilities

static wasm::Expression* HandleFollowupMultiples(wasm::Expression* Ret,
                                     