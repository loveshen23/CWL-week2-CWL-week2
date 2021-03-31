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
// Performs DCE on a graph containing a wasm module. The caller provides
// the graph, this tool fills in the wasm module's parts. It can then
// remove exports that are unused, for example, which is impossible
// otherwise (since we wouldn't know if the outside needs them).
//
// TODO: Currently all functions in the table are considered roots,
//       as the outside may call them. In the future we probably want
//       to refine that.

#include <memory>

#include "asmjs/shared-constants.h"
#include "ir/element-utils.h"
#include "ir/module-utils.h"
#include "pass.h"
#include "support/colors.h"
#include "support/file.h"
#include "support/json.h"
#include "tool-options.h"
#include "wasm-builder.h"
#include "wasm-io.h"
#include "wasm-validator.h"

using namespace wasm;

// Generic reachability graph of abstract nodes

struct DCENode {
  Name name;
  std::vector<Name> reaches; // the other nodes this one can reach
  DCENode() = default;
  DCENode(Name name) : name(name) {}
};

// A meta DCE graph with wasm integration
struct MetaDCEGraph {
  std::unordered_map<Name, DCENode> nodes;
  std::unordered_set<Name> roots;

  // export exported name => DCE name
  std::unordered_map<Name, Name> exportToDCENode;
  std::unordered_map<Name, Name> functionToDCENode; // function name => DCE name
  std::unordered_map<Name, Name> globalToDCENode;   // global name => DCE name
  std::unordered_map<Name, Name> tagToDCENode;      // tag name => DCE name

  std::unordered_map<Name, Name> DCENodeToExport; // reverse maps
  std::unordered_map<Name, Name> DCENodeToFunction;
  std::unordered_map<Name, Name> DCENodeToGlobal;
  std::unordered_map<Name, Name> DCENodeToTag;

  // imports are not mapped 1:1 to DCE nodes in the wasm, since env.X might
  // be imported twice, for example. So we don't map a DCE node to an Import,
  // but rather the module.base pair ("id") for the import.
  // TODO: implement this in a safer way, not a string with a magic separator
  using ImportId = Name;

  ImportId getImportId(Name module, Name base) {
    if (module == "GOT.func" || module == "GOT.mem") {
      // If a module imports a symbol from `GOT.func` of `GOT.mem` that
      // corresponds to the address of a symbol in the `env` namespace.  The
      // `GOT.func` and `GOT.mem` don't act