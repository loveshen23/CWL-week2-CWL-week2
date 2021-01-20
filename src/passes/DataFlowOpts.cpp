/*
 * Copyright 2018 WebAssembly Community Group participants
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
// Optimize using the DataFlow SSA IR.
//
// This needs 'flatten' to be run before it, and you should run full
// regular opts afterwards to clean up the flattening. For example,
// you might use it like this:
//
//    --flatten --dfo -Os
//

#include "dataflow/graph.h"
#include "dataflow/node.h"
#include "dataflow/users.h"
#include "dataflow/utils.h"
#include "ir/flat.h"
#include "ir/utils.h"
#include "pass.h"
#include "wasm-builder.h"
#include "wasm.h"

namespace wasm {

struct DataFlowOpts : public WalkerPass<PostWalker<DataFlowOpts>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<DataFlowOpts>();
  }

  DataFlow::Users nodeUsers;

  // The optimization work left to do: nodes that we need to look at.
  std::unordered_set<DataFlow::Node*> workLeft;

  DataFlow::Graph graph;

  void doWalkFunction(Function* func) {
    Flat::verifyFlatness(func);
    // Build the data-flow IR.
    graph.build(func, getModule());
    nodeUsers.build(graph);
    // Propagate optimizations through the graph.
    std::unordered_set<DataFlow::Node*> optimized; // which nodes we optimized
    for (auto& node : graph.nodes) {
      workLeft.insert(node.get()); // we should try to optimize each node
    }
    while (!workLeft.empty()) {
      // std::cout << "\n\ndump before work iter\n";
      // dump(graph, std::cout);
      auto iter = workLeft.begin();
      auto* node = *iter;
      workLeft.erase(iter);
      workOn(node);
    }
    // After updating the DataFlow IR, we can update the sets in
    // the wasm.
    // TODO: we also need phis, as a phi can flow directly into say
    //       a return or a call parameter.
    for (auto* set : graph.sets) {
      auto* node = graph.setNodeMap[set];
      auto iter = optimized.find(node);
      if (iter != optimized.end()) {
        assert(node->isExpr()); // this is a set, where the node is defined
        set->value = node->expr;
      }
    }
  }

  void workOn(DataFlow::Node* node) {
    if (node->isConst()) {
      return;
    }
    // If there are no uses, there is no point to work.
    if (nodeUsers.getNumUses(node) == 0) {
      return;
    }
    // Optimize: Look for nodes that we can easily convert into
    // something simpler.
    // TODO: we can expressionify and run full normal opts on that,
    //       then copy the result if it's smaller.
    if (node->isPhi() && DataFlow::allInputsIdentical(node)) {
      // Note we don't need to check for effects when replacing, as in
      // flattened IR expression children are local.gets or consts.
      auto* value = node->getValue(1);
      if (value->isConst()) {
        replaceAllUsesWith(node, value);
      }
    } else if (node->isExpr() && DataFlow::allInputsConstant(node)) {
      assert(!node->isConst());
      // If this is a concrete value (not e.g. an eqz of unreachable),
      // it can definitely be precomputed into a constant.
      if (node->expr->type.isConcrete()) {
        // This can be precomputed.
        // TODO not just all-constant inputs? E.g. i32.mul of 0 and X.
        optimizeExprToConstant(node);
      }
    }
  }

  void optimizeExprToConstant(DataFlow::Node* node) {
    assert(node->isExpr());
    assert(!node->isConst());
    // std::cout << "will optimize an Expr of all constant inputs. before" <<
    //              '\n';
    // dump(node, std::cout);
    auto* expr = node->expr;
    // First, note that some of the expression's children may be
    // local.gets that we inferred during SSA analysis as constant.
    // We can apply those now.
    for (Index i = 0; i < node->values.size(); i++) {
      if (node->values[i]->isConst()) {
        auto* currp = getIndexPointer(expr, i);
        // Directly represent it as a constant. (Note that it may already be
        // a constant, but for now to avoid corner cases just replace them
        // all here.)
        auto* c = node->values[i]->expr->dynCast<Const>();
        *currp = Builder(*getModule()).makeConst(c->value);
      }
    }
    // Now we know that all our DataFlow inputs are constant, and all
    // our Binaryen IR representations of them are constant too. RUn
    // precompute, which will transform the expression into a constanat.
    Module temp;
    // XXX we should copy expr here, in principle, and definitely will need to
    //     when we do arbitrarily regenerated expressions
    std::unique_ptr<Function> tempFunc(Builder(temp).makeFunction(
      "temp", Signature(Type::none, Type::none), {}, expr));
    PassRunner runner(&temp);
    runner.setIsNested(true);
    runner.add("precompute");
    runner.runOnFunction(tempFunc.get());
    // Get the optimized thing
    auto* result = tempFunc->body;
    // It may not be a constant, e.g. 0 / 0 does not optimize to 0
    if (!result->is<Const>()) {
      return;
    }
    // All good, copy it.
    node->expr = Builder(*getModule()).makeConst(result->cast<Const>()->value);
    assert(node->isConst());
    // We no longer have values, and so do not use anything.
    nodeUsers.stopUsingValues(node);
    nod