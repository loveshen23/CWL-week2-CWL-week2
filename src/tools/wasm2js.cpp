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
    }
  }
};

// Traverse, calling visit after the children
static void traversePrePost(Ref node,
                            std::function<void(Ref)> visitPre,
                            std::function<void(Ref)> visitPost) {
  std::vector<TraverseInfo> stack;
  stack.push_back(TraverseInfo(node));
  while (!stack.empty()) {
    TraverseInfo& back = stack.back();
    if (!back.scanned) {
      back.scanned = true;
      // This is the first time we see this.
      visitPre(back.node);
      for (auto child : back.children) {
        stack.emplace_back(child);
      }
      continue;
    }
    // Time to post-visit the node itself
    auto node = back.node;
    stack.pop_back();
    visitPost(node);
  }
}

static void traversePost(Ref node, std::function<void(Ref)> visit) {
  traversePrePost(
    node, [](Ref node) {}, visit);
}

static void replaceInPlace(Ref target, Ref value) {
  assert(target->isArray() && value->isArray());
  target->setSize(value->size());
  for (size_t i = 0; i < value->size(); i++) {
    target[i] = value[i];
  }
}

static void replaceInPlaceIfPossible(Ref target, Ref value) {
  if (target->isArray() && value->isArray()) {
    replaceInPlace(target, value);
  }
}

static void optimizeJS(Ref ast, Wasm2JSBuilder::Flags flags) {
  // Helpers

  auto isBinary = [](Ref node, IString op) {
    return node->isArray() && !node->empty() && node[0] == BINARY &&
           node[1] == op;
  };

  auto isConstantBinary = [&](Ref node, IString op, int num) {
    return isBinary(node, op) && node[3]->isNumber() &&
           node[3]->getNumber() == num;
  };

  auto isOrZero = [&](Ref node) { return isConstantBinary(node, OR, 0); };

  auto isTrshiftZero = [&](Ref node) {
    return isConstantBinary(node, TRSHIFT, 0);
  };

  auto isPlus = [](Ref node) {
    return node->isArray() && !node->empty() && node[0] == UNARY_PREFIX &&
           node[1] == PLUS;
  };

  auto isFround = [](Ref node) {
    return node->isArray() && !node->empty() && node[0] == cashew::CALL &&
           node[1] == MATH_FROUND;
  };

  auto isBitwise = [](Ref node) {
    if (node->isArray() && !node->empty() && node[0] == BINARY) {
      auto op = node[1];
      return op == OR || op == AND || op == XOR || op == RSHIFT ||
             op == TRSHIFT || op == LSHIFT;
    }
    return false;
  };

  auto isSignedBitwise = [](Ref node) {
    if (node->isArray() && !node->empty() && node[0] == BINARY) {
      auto op = node[1];
      return op == OR || op == AND || op == XOR || op == RSHIFT || op == LSHIFT;
    }
    return false;
  };

  auto isUnary = [](Ref node, IString op) {
    return node->isArray() && !node->empty() && node[0] == UNARY_PREFIX &&
           node[1] == op;
  };

  auto isWhile = [](Ref node) {
    return node->isArray() && !node->empty() && node[0] == WHILE;
  };

  auto isDo = [](Ref node) {
    return node->isArray() && !node->empty() && node[0] == DO;
  };

  auto isIf = [](Ref node) {
    return node->isArray() && !node->empty() && node[0] == IF;
  };

  auto removeOrZero = [&](Ref node) {
    while (isOrZero(node)) {
      node = node[2];
    }
    return node;
  };

  auto removePlus = [&](Ref node) {
    while (isPlus(node)) {
      node = node[2];
    }
    return node;
  };

  auto removePlusAndFround = [&](Ref node) {
    while (1) {
      if (isFround(node)) {
        node = node[2][0];
      } else if (isPlus(node)) {
        node = node[2];
      } else {
        break;
      }
    }
    return node;
  };

  auto getHeapFromAccess = [](Ref node) { return node[1]->getIString(); };

  auto setHeapOnAccess = [](Ref node, IString heap) {
    node[1] = ValueBuilder::makeName(heap);
  };

  auto isIntegerHeap = [](IString heap) {
    return heap == HEAP8 || heap == HEAPU8 || heap == HEAP16 ||
           heap == HEAPU16 || heap == HEAP32 || heap == HEAPU32;
  };

  auto isFloatHeap = [](IString heap) {
    return heap == HEAPF32 || heap == HEAPF64;
  };

  auto isHeapAccess = [&](Ref node) {
    if (node->isArray() && !node->empty() && node[0] == SUB &&
        node[1]->isString()) {
      auto heap = getHeapFromAccess(node);
      return isIntegerHeap(heap) || isFloatHeap(heap);
    }
    return false;
  };

  // Optimize given that the expression is flowing into a boolean context
  auto optimizeBoolean = [&](Ref node) {
    // TODO: in some cases it may be possible to turn
    //
    //   if (x | 0)
    //
    // into
    //
    //   if (x)
    //
    // In general this is unsafe if e.g. x is -2147483648 + -2147483648 (which
    // the | 0 turns into 0, but without it is a truthy value).
    //
    // Another issue is that in deterministic mode we care about corner cases
    // that would trap in wasm, like an integer divide by zero:
    //
    //  if ((1 / 0) | 0)  =>  condition is Infinity | 0 = 0 which is falsey
    //
    // while
    //
    //  if (1 / 0)  =>  condition is Infinity which is truthy
    //
    // Thankfully this is not common, and does not occur on % (1 % 0 is a NaN
    // which has the right truthiness), so we could perhaps do
    //
    //   if (!(flags.deterministic && isBinary(node[2], DIV))) return node[2];
    //
    // (but there is still the first issue).
    return node;
  };

  // Optimizations

  // Pre-simplification
  traversePost(ast, [&](Ref node) {
    // x >> 0  =>  x | 0
    if (isConstantBinary(node, RSHIFT, 0)) {
      node[1]->setString(OR);
    }
  });

  traversePost(ast, [&](Ref node) {
    if (isBitwise(node)) {
      // x | 0 going into a bitwise op => skip the | 0
      node[2] = removeOrZero(node[2]);
      node[3] = removeOrZero(node[3]);
      // (x | 0 or similar) | 0  =>  (x | 0 or similar)
      if (isOrZero(node)) {
        if (isSignedBitwise(node[2])) {
          replaceInPlace(node, node[2]);
        }
      }
      if (isHeapAccess(node[2])) {
        auto heap = getHeapFromAccess(node[2]);
        IString replacementHeap;
        // We can avoid a cast of a load by using the load to do it instead.
        if (isOrZero(node)) {
          if (isIntegerHeap(heap)) {
            replacementHeap = heap;
          }
        } else if (isTrshiftZero(node)) {
          // For signed or unsigned loads smaller than 32 bits, doing an | 0
          // was safe either way - they aren't in the range an | 0 can affect.
          // For >>> 0 however, a negative value would change, so we still
          // need the cast.
          if (heap == HEAP32 || heap == HEAPU32) {
            replacementHeap = HEAPU32;
          } else if (heap == HEAPU16) {
            replacementHeap = HEAPU16;
          } else if (heap == HEAPU8) {
            replacementHeap = HEAPU8;
          }
        }
        if (!replacementHeap.isNull()) {
          setHeapOnAccess(node[2], replacementHeap);
          replaceInPlace(node, node[2]);
          return;
        }
        // A load into an & may allow using a simpler heap, e.g. HEAPU8[..] & 1
        // (a load of a boolean) may be HEAP8[..] & 1. The signed heaps are more
        // commonly used, so it compresses better, and also they seem to have
        // better performance (perhaps since HEAPU32 is at risk of not being a
        // smallint).
        if (node[1] == AND) {
          if (isConstantBinary(node, AND, 1)) {
            if (heap == HEAPU8) {
              setHeapOnAccess(node[2], HEAP8);
            } else if (heap == HEAPU16) {
              setHeapOnAccess(node[2], HEAP16);
            }
          }
        }
      }
      // Pre-compute constant [op] constant, which the lowering can generate
      // in loads etc.
      if (node[2]->isNumber() && node[3]->isNumber()) {
        int32_t left = node[2]->getNumber();
        int32_t right = node[3]->getNumber();
        if (node[1] == OR) {
          node->setNumber(left | right);
        } else if (node[1] == AND) {
          node->setNumber(left & right);
        } else if (node[1] == XOR) {
          node->setNumber(left ^ right);
        } else if (node[1] == LSHIFT) {
          node->setNumber(left << (right & 31));
        } else if (node[1] == RSHIFT) {
          node->setNumber(int32_t(left) >> int32_t(right & 31));
        } else if (node[1] == TRSHIFT) {
          node->setNumber(uint32_t(left) >> uint32_t(right & 31));
        }
        return;
      }
    }
    // +(+x) => +x
    else if (isPlus(node)) {
      node[2] = removePlus(node[2]);
    }
    // +(+x) => +x
    else if (isFround(node)) {
      node[2] = removePlusAndFround(node[2]);
    } else if (isUnary(node, L_NOT)) {
      node[2] = optimizeBoolean(node[2]);
    }
    // Add/subtract can merge coercions up, except when a child is a division,
    // which needs to be eagerly truncated to remove fractional results.
    else if (isBinary(node, PLUS) || isBinary(node, MINUS)) {
      auto left = node[2];
      auto right = node[3];
      if (isOrZero(left) && isOrZero(right) && !isBinary(left[2], DIV) &&
          !isBinary(right[2], DIV)) {
        auto op = node[1]->getIString();
        // Add a coercion on top.
        node[1]->setString(OR);
        node[2] = left;
        node[3] = ValueBuilder::makeNum(0);
        // Add/subtract the inner uncoerced values.
        left[1]->setString(op);
        left[3] = right[2];
      }
    }
    // Assignment into a heap coerces.
    else if (node->isAssign()) {
      auto assign = node->asAssign();
      auto target = assign->target();
      if (isHeapAccess(target)) {
        auto heap = getHeapFromAccess(target);
        if (isIntegerHeap(heap)) {
          if (heap == HEAP8 || heap == HEAPU8) {
            while (isOrZero(assign->value()) ||
                   isConstantBinary(assign->value(), AND, 255)) {
              assign->value() = assign->value()[2];
            }
          } else if (heap == HEAP16 || heap == HEAPU16) {
            while (isOrZero(assign->value()) ||
                   isConstantBinary(assign->value(), AND, 65535)) {
              assign->value() = assign->value()[2];
            }
          } else {
            assert(heap == HEAP32 || heap == HEAPU32);
            assign->value() = removeOrZero(assign->value());
          }
        } else {
          assert(isFloatHeap(heap));
          if (heap == HEAPF32) {
            assign->value() = removePlusAndFround(assign->value());
          } else {
            assign->value() = removePlus(assign->value());
          }
        }
      }
    } else if (isWhile(node) || isDo(node) || isIf(node)) {
      node[1] = optimizeBoolean(node[1]);
    }
  });

  // Remove unnecessary break/continue labels, when the name is that of the
  // highest target anyhow, which we would reach without the name.

  std::vector<Ref> breakCapturers;
  std::vector<Ref> continueCapturers;
  std::unordered_map<IString, Ref>
    labelToValue;                      // maps the label to the loop/etc.
  std::unordered_set<Value*> labelled; // all things with a label on them.
  Value INVALID;
  traversePrePost(
    ast,
    [&](Ref node) {
      if (node->isArray() && !node->empty()) {
        if (node[0] == LABEL) {
          auto label = node[1]->getIString();
          labelToValue[label] = node[2];
          labelled.insert(node[2].get());
        } else if (node[0] == WHILE || node[0] == DO || node[0] == FOR) {
          breakCapturers.push_back(node);
          continueCapturers.push_back(node);
        } else if (node[0] == cashew::BLOCK) {
          if (labelled.count(node.get())) {
            // Cannot break to a block without the label.
            breakCapturers.push_back(Ref(&INVALID));
          }
        } else if (node[0] == SWITCH) {
          breakCapturers.push_back(node);
        }
      }
    },
    [&](Ref node) {
      if (node->isArray() && !node->empty()) {
        if (node[0] == LABEL) {
          auto lab