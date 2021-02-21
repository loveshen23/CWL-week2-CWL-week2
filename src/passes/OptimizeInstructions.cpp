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

//
// Optimize combinations of instructions
//

#include <algorithm>
#include <cmath>
#include <type_traits>

#include <ir/abstract.h>
#include <ir/bits.h>
#include <ir/boolean.h>
#include <ir/cost.h>
#include <ir/drop.h>
#include <ir/effects.h>
#include <ir/eh-utils.h>
#include <ir/find_all.h>
#include <ir/gc-type-utils.h>
#include <ir/iteration.h>
#include <ir/literal-utils.h>
#include <ir/load-utils.h>
#include <ir/manipulation.h>
#include <ir/match.h>
#include <ir/ordering.h>
#include <ir/properties.h>
#include <ir/type-updating.h>
#include <ir/utils.h>
#include <pass.h>
#include <support/threads.h>
#include <wasm.h>

#include "call-utils.h"

// TODO: Use the new sign-extension opcodes where appropriate. This needs to be
// conditionalized on the availability of atomics.

namespace wasm {

static Index getBitsForType(Type type) {
  if (!type.isNumber()) {
    return -1;
  }
  return type.getByteSize() * 8;
}

static bool isSignedOp(BinaryOp op) {
  switch (op) {
    case LtSInt32:
    case LeSInt32:
    case GtSInt32:
    case GeSInt32:
    case LtSInt64:
    case LeSInt64:
    case GtSInt64:
    case GeSInt64:
      return true;
    default:
      return false;
  }
}

// Useful information about locals
struct LocalInfo {
  static const Index kUnknown = Index(-1);

  Index maxBits;
  Index signExtedBits;
};

struct LocalScanner : PostWalker<LocalScanner> {
  std::vector<LocalInfo>& localInfo;
  const PassOptions& passOptions;

  LocalScanner(std::vector<LocalInfo>& localInfo,
               const PassOptions& passOptions)
    : localInfo(localInfo), passOptions(passOptions) {}

  void doWalkFunction(Function* func) {
    // prepare
    localInfo.resize(func->getNumLocals());
    for (Index i = 0; i < func->getNumLocals(); i++) {
      auto& info = localInfo[i];
      if (func->isParam(i)) {
        info.maxBits = getBitsForType(func->getLocalType(i)); // worst-case
        info.signExtedBits = LocalInfo::kUnknown; // we will never know anything
      } else {
        info.maxBits = info.signExtedBits = 0; // we are open to learning
      }
    }
    // walk
    PostWalker<LocalScanner>::doWalkFunction(func);
    // finalize
    for (Index i = 0; i < func->getNumLocals(); i++) {
      auto& info = localInfo[i];
      if (info.signExtedBits == LocalInfo::kUnknown) {
        info.signExtedBits = 0;
      }
    }
  }

  void visitLocalSet(LocalSet* curr) {
    auto* func = getFunction();
    if (func->isParam(curr->index)) {
      return;
    }
    auto type = getFunction()->getLocalType(curr->index);
    if (type != Type::i32 && type != Type::i64) {
      return;
    }
    // an integer var, worth processing
    auto* value =
      Properties::getFallthrough(curr->value, passOptions, *getModule());
    auto& info = localInfo[curr->index];
    info.maxBits = std::max(info.maxBits, Bits::getMaxBits(value, this));
    auto signExtBits = LocalInfo::kUnknown;
    if (Properties::getSignExtValue(value)) {
      signExtBits = Properties::getSignExtBits(value);
    } else if (auto* load = value->dynCast<Load>()) {
      if (LoadUtils::isSignRelevant(load) && load->signed_) {
        signExtBits = load->bytes * 8;
      }
    }
    if (info.signExtedBits == 0) {
      info.signExtedBits = signExtBits; // first info we see
    } else if (info.signExtedBits != signExtBits) {
      // contradictory information, give up
      info.signExtedBits = LocalInfo::kUnknown;
    }
  }

  // define this for the templated getMaxBits method. we know nothing here yet
  // about locals, so return the maxes
  Index getMaxBitsForLocal(LocalGet* get) { return getBitsForType(get->type); }
};

namespace {
// perform some final optimizations
struct FinalOptimizer : public PostWalker<FinalOptimizer> {
  const PassOptions& passOptions;

  FinalOptimizer(const PassOptions& passOptions) : passOptions(passOptions) {}

  void visitBinary(Binary* curr) {
    if (auto* replacement = optimize(curr)) {
      replaceCurrent(replacement);
    }
  }

  Binary* optimize(Binary* curr) {
    using namespace Abstract;
    using namespace Match;
    {
      Const* c;
      if (matches(curr, binary(Add, any(), ival(&c)))) {
        // normalize x + (-C)  ==>   x - C
        if (c->value.isNegative()) {
          c->value = c->value.neg();
          curr->op = Abstract::getBinary(c->type, Sub);
        }
        // Wasm binary encoding uses signed LEBs, which slightly favor negative
        // numbers: -64 is more efficient than +64 etc., as well as other powers
        // of two 7 bits etc. higher. we therefore prefer x - -64 over x + 64.
        // in theory we could just prefer negative numbers over positive, but
        // that can have bad effects on gzip compression (as it would mean more
        // subtractions than the more common additions).
        int64_t value = c->value.getInteger();
        if (value == 0x40LL || value == 0x2000LL || value == 0x100000LL ||
            value == 0x8000000LL || value == 0x400000000LL ||
            value == 0x20000000000LL || value == 0x1000000000000LL ||
            value == 0x80000000000000LL || value == 0x4000000000000000LL) {
          c->value = c->value.neg();
          if (curr->op == Abstract::getBinary(c->type, Add)) {
            curr->op = Abstract::getBinary(c->type, Sub);
          } else {
            curr->op = Abstract::getBinary(c->type, Add);
          }
        }
        return curr;
      }
    }
    return nullptr;
  }
};

} // anonymous namespace

// Create a custom matcher for checking side effects
template<class Opt> struct PureMatcherKind {};
template<class Opt>
struct Match::Internal::KindTypeRegistry<PureMatcherKind<Opt>> {
  using matched_t = Expression*;
  using data_t = Opt*;
};
template<class Opt> struct Match::Internal::MatchSelf<PureMatcherKind<Opt>> {
  bool operator()(Expression* curr, Opt* opt) {
    return !opt->effects(curr).hasSideEffects();
  }
};

// Main pass class
struct OptimizeInstructions
  : public WalkerPass<PostWalker<OptimizeInstructions>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<OptimizeInstructions>();
  }

  bool fastMath;

  // In rare cases we make a change to a type, and will do a refinalize.
  bool refinalize = false;

  void doWalkFunction(Function* func) {
    fastMath = getPassOptions().fastMath;

    // First, scan locals.
    {
      LocalScanner scanner(localInfo, getPassOptions());
      scanner.setModule(getModule());
      scanner.walkFunction(func);
    }

    // Main walk.
    super::doWalkFunction(func);

    if (refinalize) {
      ReFinalize().walkFunctionInModule(func, getModule());
    }

    // Final optimizations.
    {
      FinalOptimizer optimizer(getPassOptions());
      optimizer.walkFunction(func);
    }

    // Some patterns create blocks that can interfere 'catch' and 'pop', nesting
    // the 'pop' into a block making it invalid.
    EHUtils::handleBlockNestedPops(func, *getModule());
  }

  // Set to true when one of the visitors makes a change (either replacing the
  // node or modifying it).
  bool changed;

  // Used to avoid recursion in replaceCurrent, see below.
  bool inReplaceCurrent = false;

  void replaceCurrent(Expression* rep) {
    WalkerPass<PostWalker<OptimizeInstructions>>::replaceCurrent(rep);
    // We may be able to apply multiple patterns as one may open opportunities
    // for others. NB: patterns must not have cycles

    // To avoid recursion, this uses the following pattern: the initial call to
    // this method comes from one of the visit*() methods. We then loop in here,
    // and if we are called again we set |changed| instead of recursing, so that
    // we can loop on that value.
    if (inReplaceCurrent) {
      // We are in the loop below so just note a change and return to there.
      changed = true;
      return;
    }
    // L