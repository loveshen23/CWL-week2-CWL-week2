/*
 * Copyright 2015 WebAssembly Community Group participants
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
// wasm2js console tool
//

#include "wasm2js.h"
#include "optimization-options.h"
#include "pass.h"
#include "support/colors.h"
#include "support/command-line.h"
#include "support/file.h"
#include "wasm-s-parser.h"

using namespace cashew;
using namespace wasm;

// helpers

namespace {

static void optimizeWasm(Module& wasm, PassOptions options) {
  // Perform various optimizations that will be good for JS, but would not be
  // great for wasm in general
  struct OptimizeForJS : public WalkerPass<PostWalker<OptimizeForJS>> {
    bool isFunctionParallel() override { return true; }

    std::unique_ptr<Pass> create() override {
      return std::make_unique<OptimizeForJS>();
    }

    void visitBinary(Binary* curr) {
      // x - -c (where c is a constant) is larger than x + c, in js (but not
      // necessarily in wasm, where LEBs prefer negatives).
      if (curr->op == SubInt32) {
        if (auto* c = curr->right->dynCast<Const>()) {
          if (c->value.geti32() < 0) {
            curr->op = AddInt32;
            c->value = c->value.neg();
          }
        }
      }
    }
  };

  PassRunner runner(&wasm, options);
  OptimizeForJS().run(&runner, &wasm);
}

template<typename T> static void printJS(Ref ast, T& output) {
  JSPrinter jser(true, true, ast);
  jser.printAst();
  output << jser.buffer << '\n';
}

// Traversals

struct TraverseInfo {
  TraverseInfo() = default;
  TraverseInfo(Ref node) : node(node) {
    assert(node.get());
    if (node->isArray()) {
      for (size_t i = 0; i < node->size(); i++) {
        maybeAdd(node[i]);
      }
    } else if (node->isAssign()) {
      auto assign = node->asAssign();
      maybeAdd(assign->target());
      maybeAdd(assign->value());
    } else if (node->isAssignName()) {
      auto assign = node->asAssignName();
      maybeAdd(assign->value());
    } else {
      // no children
    }
  }
  Ref node;
  bool scanned = false;
  std::vector<Ref> children;

private:
  void maybeAdd(Ref child) {
    if (child.get()) {
      children.push_back(child);
 