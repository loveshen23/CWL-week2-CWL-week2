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
      // `GOT.func` and `GOT.mem` don't actually exist anywhere but reference
      // other symbols.  For this reason we treat an import from one of these
      // namespaces as an import from the `env` namespace.  (i.e.  any usage of
      // a `GOT.mem` or `GOT.func` import requires the corresponding env import
      // to be kept alive.
      module = ENV;
    }
    return std::string(module.str) + " (*) " + std::string(base.str);
  }

  ImportId getFunctionImportId(Name name) {
    auto* imp = wasm.getFunction(name);
    return getImportId(imp->module, imp->base);
  }

  ImportId getGlobalImportId(Name name) {
    auto* imp = wasm.getGlobal(name);
    return getImportId(imp->module, imp->base);
  }

  ImportId getTagImportId(Name name) {
    auto* imp = wasm.getTag(name);
    return getImportId(imp->module, imp->base);
  }

  // import module.base => DCE name
  std::unordered_map<Name, Name> importIdToDCENode;

  Module& wasm;

  MetaDCEGraph(Module& wasm) : wasm(wasm) {}

  // populate the graph with info from the wasm, integrating with
  // potentially-existing nodes for imports and exports that the graph may
  // already contain.
  void scanWebAssembly() {
    // Add an entry for everything we might need ahead of time, so parallel work
    // does not alter parent state, just adds to things pointed by it,
    // independently (each thread will add for one function, etc.)
    ModuleUtils::iterDefinedFunctions(wasm, [&](Function* func) {
      auto dceName = getName("func", func->name.toString());
      DCENodeToFunction[dceName] = func->name;
      functionToDCENode[func->name] = dceName;
      nodes[dceName] = DCENode(dceName);
    });
    ModuleUtils::iterDefinedGlobals(wasm, [&](Global* global) {
      auto dceName = getName("global", global->name.toString());
      DCENodeToGlobal[dceName] = global->name;
      globalToDCENode[global->name] = dceName;
      nodes[dceName] = DCENode(dceName);
    });
    ModuleUtils::iterDefinedTags(wasm, [&](Tag* tag) {
      auto dceName = getName("tag", tag->name.toString());
      DCENodeToTag[dceName] = tag->name;
      tagToDCENode[tag->name] = dceName;
      nodes[dceName] = DCENode(dceName);
    });
    // only process function, global, and tag imports - the table and memory are
    // always there
    ModuleUtils::iterImportedFunctions(wasm, [&](Function* import) {
      auto id = getImportId(import->module, import->base);
      if (importIdToDCENode.find(id) == importIdToDCENode.end()) {
        auto dceName = getName("importId", import->name.toString());
        importIdToDCENode[id] = dceName;
      }
    });
    ModuleUtils::iterImportedGlobals(wasm, [&](Global* import) {
      auto id = getImportId(import->module, import->base);
      if (importIdToDCENode.find(id) == importIdToDCENode.end()) {
        auto dceName = getName("importId", import->name.toString());
        importIdToDCENode[id] = dceName;
      }
    });
    ModuleUtils::iterImportedTags(wasm, [&](Tag* import) {
      auto id = getImportId(import->module, import->base);
      if (importIdToDCENode.find(id) == importIdToDCENode.end()) {
        auto dceName = getName("importId", import->name.toString());
        importIdToDCENode[id] = dceName;
      }
    });
    for (auto& exp : wasm.exports) {
      if (exportToDCENode.find(exp->name) == exportToDCENode.end()) {
        auto dceName = getName("export", exp->name.toString());
        DCENodeToExport[dceName] = exp->name;
        exportToDCENode[exp->name] = dceName;
        nodes[dceName] = DCENode(dceName);
      }
      // we can also link the export to the thing being exported
      auto& node = nodes[exportToDCENode[exp->name]];
      if (exp->kind == ExternalKind::Function) {
        if (!wasm.getFunction(exp->value)->imported()) {
          node.reaches.push_back(functionToDCENode[exp->value]);
        } else {
          node.reaches.push_back(
            importIdToDCENode[getFunctionImportId(exp->value)]);
        }
      } else if (exp->kind == ExternalKind::Global) {
        if (!wasm.getGlobal(exp->value)->imported()) {
          node.reaches.push_back(globalToDCENode[exp->value]);
        } else {
          node.reaches.push_back(
            importIdToDCENode[getGlobalImportId(exp->value)]);
        }
      } else if (exp->kind == ExternalKind::Tag) {
        if (!wasm.getTag(exp->value)->imported()) {
          node.reaches.push_back(tagToDCENode[exp->value]);
        } else {
          node.reaches.push_back(importIdToDCENode[getTagImportId(exp->value)]);
        }
      }
    }
    // Add initializer dependencies
    // if we provide a parent DCE name, that is who can reach what we see
    // if none is provided, then it is something we must root
    struct InitScanner : public PostWalker<InitScanner> {
      InitScanner(MetaDCEGraph* parent, Name parentDceName)
        : parent(parent), parentDceName(parentDceName) {}

      void visitGlobalGet(GlobalGet* curr) { handleGlobal(curr->name); }
      void visitGlobalSet(GlobalSet* curr) { handleGlobal(curr->name); }

    private:
      MetaDCEGraph* parent;
      Name parentDceName;

      void handleGlobal(Name name) {
        Name dceName;
        if (!getModule()->getGlobal(name)->imported()) {
          // its a defined global
          dceName = parent->globalToDCENode[name];
        } else {
          // it's an import.
          dceName = parent->importIdToDCENode[parent->getGlobalImportId(name)];
        }
        if (!parentDceName.isNull()) {
          parent->nodes[parentDceName].reaches.push_back(dceName);
        }
      }
    };
    ModuleUtils::iterDefinedGlobals(wasm, [&](Global* global) {
      InitScanner scanner(this, globalToDCENode[global->name]);
      scanner.setModule(&wasm);
      scanner.walk(global->init);
    });
    // we can't remove segments, so root what they need
    InitScanner rooter(this, Name());
    rooter.setModule(&wasm);
    ModuleUtils::iterActiveElementSegments(wasm, [&](ElementSegment* segment) {
      // TODO: currently, all functions in the table are roots, but we
      //       should add an option to refine that
      ElementUtils::iterElementSegmentFunctionNames(
        segment, [&](Name name, Index) {
          if (!wasm.getFunction(name)->imported()) {
            roots.insert(functionToDCENode[name]);
          } else {
            roots.insert(importIdToDCENode[getFunctionImportId(name)]);
          }
        });
      rooter.walk(segment->offset);
    });
    ModuleUtils::iterActiveDataSegments(
      wasm, [&](DataSegment* segment) { rooter.walk(segment->offset); });

    // A parallel scanner for function bodies
    struct Scanner : public WalkerPass<PostWalker<Scanner>> {
      bool isFunctionParallel() override { return true; }

      Scanner(MetaDCEGraph* parent) : parent(parent) {}

      std::unique_ptr<Pass> create() override {
        return std::make_unique<Scanner>(parent);
      }

      void visitCall(Call* curr) {
        if (!getModule()->getFunction(curr->target)->imported()) {
          parent->nodes[parent->functionToDCENode[getFunction()->name]]
            .reaches.push_back(parent->functionToDCENode[curr->target]);
        } else {
          assert(parent->functionToDCENode.count(getFunction()->name) > 0);
          parent->nodes[parent->functionToDCENode[getFunction()->name]]
            .reaches.push_back(
              parent
                ->importIdToDCENode[parent->getFunctionImportId(curr->target)]);
        }
      }
      void visitGlobalGet(GlobalGet* curr) { handleGlobal(curr->name); }
      void visitGlobalSet(GlobalSet* curr) { handleGlobal(curr->name); }
      void visitThrow(Throw* curr) { handleTag(curr->tag); }
      void visitTry(Try* curr) {
        for (auto tag : curr->catchTags) {
          handleTag(tag);
        }
      }

    private:
      MetaDCEGraph* parent;

      void handleGlobal(Name name) {
        if (!getFunction()) {
          return; // non-function stuff (initializers) are handled separately
        }
        Name dceName;
        if (!getModule()->getGlobal(name)->imported()) {
          // its a global
          dceName = parent->globalToDCENode[name];
        } else {
          // it's an import.
          dceName = parent->importIdToDCENode[parent->getGlobalImportId(name)];
        }
        parent->nodes[parent->functionToDCENode[getFunction()->name]]
          .reaches.push_back(dceName);
      }

      void handleTag(Name name) {
        Name dceName;
        if (!getModule()->getTag(name)->imported()) {
          dceName = parent->tagToDCENode[name];
        } else {
          dceName = parent->importIdToDCENode[parent->getTagImportId(name)];
        }
        parent->nodes[parent->functionToDCENode[getFunction()->name]]
          .reaches.push_back(dceName);
      }
    };

    PassRunner runner(&wasm);
    Scanner(this).run(&runner, &wasm);
  }

private:
  // gets a unique name for the graph
  Name getName(std::string prefix1, std::string prefix2) {
    while (1) {
      auto curr =
        Name(prefix1 + '$' + prefix2 + '$' + std::to_string(nameIndex++));
      if (nodes.find(curr) == nodes.end()) {
        return curr;
      }
    }
  }

  Index nameIndex = 0;

  std::unordered_set<Name> reached;

public:
  // Perform the DCE: simple marking from the roots
  void deadCodeElimination() {
    std::vector<Name> queue;
    for (auto root : roots) {
      reached.insert(root);
      queue.push_back(root);
    }
    while (queue.size() > 0) {
      auto name = queue.back();
      queue.pop_back();
      auto& node = nodes[name];
      for (auto target : node.reaches) {
        if (reached.emplace(target).second) {
          queue.push_back(target);
        }
      }
    }
  }

  // Apply to the wasm
  void apply() {
    // Remove the unused exports
    std::vector<Name> toRemove;
    for (auto& exp : wasm.exports) {
      auto name = exp->name;
      auto dceName = exportToDCENode[name];
      if (reached.find(dceName) == reached.end()) {
        toRemove.push_back(name);
      }
    }
    for (auto name : toRemove) {
      wasm.removeExport(name);
    }
    // Now they are gone, standard optimization passes can do the rest!
    PassRunner passRunner(&wasm);
    passRunner.add("remove-unused-module-elements");
    // removing functions may alter the optimum order, as # of calls can change
    passRunner.add("reorder-functions");
    passRunner.run();
  }

  // Print out everything we found is not used, and so can be
  // removed, including on the outside
  void printAllUnused() {
    std::set<std::string> unused;
    for (auto& [name, _] : nodes) {
      if (reached.find(name) == reached.end()) {
        unused.insert(name.toString());
      }
    }
    for (auto& name : unused) {
      std::cout << "unused: " << name << '\n';
    }
  }

  // A debug utility, prints out the graph
  void dump() {
    std::cout << "=== graph ===\n";
    for (auto root : roots) {
      std::cout << "root: " << root << '\n';
    }
    std::map<Name, ImportId> importMap;
    for (auto& [id, dceName] : importIdToDCENode) {
      importMap[dceName] = id;
    }
    for (auto& [name, node] : nodes) {
      std::cout << "node: " << name << '\n';
      if (importMap.find(name) != importMap.end()) {
        std::cout << "  is import " << importMap[name] << '\n';
      }
      if (DCENodeToExport.find(name) != DCENodeToExport.end()) {
        std::cout << "  is export " << DCENodeToExport[name] << ", "
                  << wasm.getExport(DCENodeToExport[name])->value << '\n';
      }
      if (DCENodeToFunction.find(name) != DCENodeToFunction.end()) {
        std::cout << "  is function " << DCENodeToFunction[name] << '\n';
      }
      if (DCENodeToGlobal.find(name) != DCENodeToGlobal.end()) {
        std::cout << "  is global " << DCENodeToGlobal[name] << '\n';
      }
      if (DCENodeToTag.fi