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
    // Loop on further changes.
    inReplaceCurrent = true;
    do {
      changed = false;
      visit(getCurrent());
    } while (changed);
    inReplaceCurrent = false;
  }

  EffectAnalyzer effects(Expression* expr) {
    return EffectAnalyzer(getPassOptions(), *getModule(), expr);
  }

  decltype(auto) pure(Expression** binder) {
    using namespace Match::Internal;
    return Matcher<PureMatcherKind<OptimizeInstructions>>(binder, this);
  }

  bool canReorder(Expression* a, Expression* b) {
    return EffectAnalyzer::canReorder(getPassOptions(), *getModule(), a, b);
  }

  void visitBinary(Binary* curr) {
    // If this contains dead code, don't bother trying to optimize it, the type
    // might change (if might not be unreachable if just one arm is, for
    // example). This optimization pass focuses on actually executing code.
    if (curr->type == Type::unreachable) {
      return;
    }

    if (shouldCanonicalize(curr)) {
      canonicalize(curr);
    }

    {
      // TODO: It is an ongoing project to port more transformations to the
      // match API. Once most of the transformations have been ported, the
      // `using namespace Match` can be hoisted to function scope and this extra
      // block scope can be removed.
      using namespace Match;
      using namespace Abstract;
      Builder builder(*getModule());
      {
        // try to get rid of (0 - ..), that is, a zero only used to negate an
        // int. an add of a subtract can be flipped in order to remove it:
        //   (ival.add
        //     (ival.sub
        //       (ival.const 0)
        //       X
        //     )
        //     Y
        //   )
        // =>
        //   (ival.sub
        //     Y
        //     X
        //   )
        // Note that this reorders X and Y, so we need to be careful about that.
        Expression *x, *y;
        Binary* sub;
        if (matches(
              curr,
              binary(Add, binary(&sub, Sub, ival(0), any(&x)), any(&y))) &&
            canReorder(x, y)) {
          sub->left = y;
          sub->right = x;
          return replaceCurrent(sub);
        }
      }
      {
        // The flip case is even easier, as no reordering occurs:
        //   (ival.add
        //     Y
        //     (ival.sub
        //       (ival.const 0)
        //       X
        //     )
        //   )
        // =>
        //   (ival.sub
        //     Y
        //     X
        //   )
        Expression* y;
        Binary* sub;
        if (matches(curr,
                    binary(Add, any(&y), binary(&sub, Sub, ival(0), any())))) {
          sub->left = y;
          return replaceCurrent(sub);
        }
      }
      {
        // try de-morgan's AND law,
        //  (eqz X) and (eqz Y) === eqz (X or Y)
        // Note that the OR and XOR laws do not work here, as these
        // are not booleans (we could check if they are, but a boolean
        // would already optimize with the eqz anyhow, unless propagating).
        // But for AND, the left is true iff X and Y are each all zero bits,
        // and the right is true if the union of their bits is zero; same.
        Unary* un;
        Binary* bin;
        Expression *x, *y;
        if (matches(curr,
                    binary(&bin,
                           AndInt32,
                           unary(&un, EqZInt32, any(&x)),
                           unary(EqZInt32, any(&y))))) {
          bin->op = OrInt32;
          bin->left = x;
          bin->right = y;
          un->value = bin;
          return replaceCurrent(un);
        }
      }
      {
        // x <<>> (C & (31 | 63))   ==>   x <<>> C'
        // x <<>> (y & (31 | 63))   ==>   x <<>> y
        // x <<>> (y & (32 | 64))   ==>   x
        // where '<<>>':
        //   '<<', '>>', '>>>'. 'rotl' or 'rotr'
        BinaryOp op;
        Const* c;
        Expression *x, *y;

        // x <<>> C
        if (matches(curr, binary(&op, any(&x), ival(&c))) &&
            Abstract::hasAnyShift(op)) {
          // truncate RHS constant to effective size as:
          // i32(x) <<>> const(C & 31))
          // i64(x) <<>> const(C & 63))
          c->value = c->value.and_(
            Literal::makeFromInt32(c->type.getByteSize() * 8 - 1, c->type));
          // x <<>> 0   ==>   x
          if (c->value.isZero()) {
            return replaceCurrent(x);
          }
        }
        if (matches(curr,
                    binary(&op, any(&x), binary(And, any(&y), ival(&c)))) &&
            Abstract::hasAnyShift(op)) {
          // i32(x) <<>> (y & 31)   ==>   x <<>> y
          // i64(x) <<>> (y & 63)   ==>   x <<>> y
          if ((c->type == Type::i32 && (c->value.geti32() & 31) == 31) ||
              (c->type == Type::i64 && (c->value.geti64() & 63LL) == 63LL)) {
            curr->cast<Binary>()->right = y;
            return replaceCurrent(curr);
          }
          // i32(x) <<>> (y & C)   ==>   x,  where (C & 31) == 0
          // i64(x) <<>> (y & C)   ==>   x,  where (C & 63) == 0
          if (((c->type == Type::i32 && (c->value.geti32() & 31) == 0) ||
               (c->type == Type::i64 && (c->value.geti64() & 63LL) == 0LL)) &&
              !effects(y).hasSideEffects()) {
            return replaceCurrent(x);
          }
        }
      }
      {
        // -x + y   ==>   y - x
        //   where  x, y  are floating points
        Expression *x, *y;
        if (matches(curr, binary(Add, unary(Neg, any(&x)), any(&y))) &&
            canReorder(x, y)) {
          curr->op = Abstract::getBinary(curr->type, Sub);
          curr->left = x;
          std::swap(curr->left, curr->right);
          return replaceCurrent(curr);
        }
      }
      {
        // x + (-y)   ==>   x - y
        // x - (-y)   ==>   x + y
        //   where  x, y  are floating points
        Expression* y;
        if (matches(curr, binary(Add, any(), unary(Neg, any(&y)))) ||
            matches(curr, binary(Sub, any(), unary(Neg, any(&y))))) {
          curr->op = Abstract::getBinary(
            curr->type,
            curr->op == Abstract::getBinary(curr->type, Add) ? Sub : Add);
          curr->right = y;
          return replaceCurrent(curr);
        }
      }
      {
        // -x * -y   ==>   x * y
        //   where  x, y  are integers
        Binary* bin;
        Expression *x, *y;
        if (matches(curr,
                    binary(&bin,
                           Mul,
                           binary(Sub, ival(0), any(&x)),
                           binary(Sub, ival(0), any(&y))))) {
          bin->left = x;
          bin->right = y;
          return replaceCurrent(curr);
        }
      }
      {
        // -x * y   ==>   -(x * y)
        // x * -y   ==>   -(x * y)
        //   where  x, y  are integers
        Expression *x, *y;
        if ((matches(curr,
                     binary(Mul, binary(Sub, ival(0), any(&x)), any(&y))) ||
             matches(curr,
                     binary(Mul, any(&x), binary(Sub, ival(0), any(&y))))) &&
            !x->is<Const>() && !y->is<Const>()) {
          Builder builder(*getModule());
          return replaceCurrent(
            builder.makeBinary(Abstract::getBinary(curr->type, Sub),
                               builder.makeConst(Literal::makeZero(curr->type)),
                               builder.makeBinary(curr->op, x, y)));
        }
      }
      {
        if (getModule()->features.hasSignExt()) {
          Const *c1, *c2;
          Expression* x;
          // i64(x) << 56 >> 56   ==>   i64.extend8_s(x)
          // i64(x) << 48 >> 48   ==>   i64.extend16_s(x)
          // i64(x) << 32 >> 32   ==>   i64.extend32_s(x)
          if (matches(curr,
                      binary(ShrSInt64,
                             binary(ShlInt64, any(&x), i64(&c1)),
                             i64(&c2))) &&
              Bits::getEffectiveShifts(c1) == Bits::getEffectiveShifts(c2)) {
            switch (64 - Bits::getEffectiveShifts(c1)) {
              case 8:
                return replaceCurrent(builder.makeUnary(ExtendS8Int64, x));
              case 16:
                return replaceCurrent(builder.makeUnary(ExtendS16Int64, x));
              case 32:
                return replaceCurrent(builder.makeUnary(ExtendS32Int64, x));
              default:
                break;
            }
          }
          // i32(x) << 24 >> 24   ==>   i32.extend8_s(x)
          // i32(x) << 16 >> 16   ==>   i32.extend16_s(x)
          if (matches(curr,
                      binary(ShrSInt32,
                             binary(ShlInt32, any(&x), i32(&c1)),
                             i32(&c2))) &&
              Bits::getEffectiveShifts(c1) == Bits::getEffectiveShifts(c2)) {
            switch (32 - Bits::getEffectiveShifts(c1)) {
              case 8:
                return replaceCurrent(builder.makeUnary(ExtendS8Int32, x));
              case 16:
                return replaceCurrent(builder.makeUnary(ExtendS16Int32, x));
              default:
                break;
            }
          }
        }
      }
      {
        // unsigned(x) >= 0   =>   i32(1)
        // TODO: Use getDroppedChildrenAndAppend() here, so we can optimize even
        //       if pure.
        Const* c;
        Expression* x;
        if (matches(curr, binary(GeU, pure(&x), ival(&c))) &&
            c->value.isZero()) {
          c->value = Literal::makeOne(Type::i32);
          c->type = Type::i32;
          return replaceCurrent(c);
        }
        // unsigned(x) < 0   =>   i32(0)
        if (matches(curr, binary(LtU, pure(&x), ival(&c))) &&
            c->value.isZero()) {
          c->value = Literal::makeZero(Type::i32);
          c->type = Type::i32;
          return replaceCurrent(c);
        }
      }
    }
    if (auto* ext = Properties::getAlmostSignExt(curr)) {
      Index extraLeftShifts;
      auto bits = Properties::getAlmostSignExtBits(curr, extraLeftShifts);
      if (extraLeftShifts == 0) {
        if (auto* load =
              Properties::getFallthrough(ext, getPassOptions(), *getModule())
                ->dynCast<Load>()) {
          // pattern match a load of 8 bits and a sign extend using a shl of
          // 24 then shr_s of 24 as well, etc.
          if (LoadUtils::canBeSigned(load) &&
              ((load->bytes == 1 && bits == 8) ||
               (load->bytes == 2 && bits == 16))) {
            // if the value falls through, we can't alter the load, as it
            // might be captured in a tee
            if (load->signed_ == true || load == ext) {
              load->signed_ = true;
              return replaceCurrent(ext);
            }
          }
        }
      }
      // We can in some cases remove part of a sign extend, that is,
      //   (x << A) >> B   =>   x << (A - B)
      // If the sign-extend input cannot have a sign bit, we don't need it.
      if (Bits::getMaxBits(ext, this) + extraLeftShifts < bits) {
        return replaceCurrent(removeAlmostSignExt(curr));
      }
      // We also don't need it if it already has an identical-sized sign
      // extend applied to it. That is, if it is already a sign-extended
      // value, then another sign extend will do nothing. We do need to be
      // careful of the extra shifts, though.
      if (isSignExted(ext, bits) && extraLeftShifts == 0) {
        return replaceCurrent(removeAlmostSignExt(curr));
      }
    } else if (curr->op == EqInt32 || curr->op == NeInt32) {
      if (auto* c = curr->right->dynCast<Const>()) {
        if (auto* ext = Properties::getSignExtValue(curr->left)) {
          // We are comparing a sign extend to a constant, which means we can
          // use a cheaper zero-extend in some cases. That is,
          //  (x << S) >> S ==/!= C    =>    x & T ==/!= C
          // where S and T are the matching values for sign/zero extend of the
          // same size. For example, for an effective 8-bit value:
          //  (x << 24) >> 24 ==/!= C    =>    x & 255 ==/!= C
          //
          // The key thing to track here are the upper bits plus the sign bit;
          // call those the "relevant bits". This is crucial because x is
          // sign-extended, that is, its effective sign bit is spread to all
          // the upper bits, which means that the relevant bits on the left
          // side are either all 0, or all 1.
          auto bits = Properties::getSignExtBits(curr->left);
          uint32_t right = c->value.geti32();
          uint32_t numRelevantBits = 32 - bits + 1;
          uint32_t setRelevantBits =
            Bits::popCount(right >> uint32_t(bits - 1));
          // If all the relevant bits on C are zero
          // then we can mask off the high bits instead of sign-extending x.
          // This is valid because if x is negative, then the comparison was
          // false before (negative vs positive), and will still be false
          // as the sign bit will remain to cause a difference. And if x is
          // positive then the upper bits would be zero anyhow.
          if (setRelevantBits == 0) {
            curr->left = makeZeroExt(ext, bits);
            return replaceCurrent(curr);
          } else if (setRelevantBits == numRelevantBits) {
            // If all those bits are one, then we can do something similar if
            // we also zero-extend on the right as well. This is valid
            // because, as in the previous case, the sign bit differentiates
            // the two sides when they are different, and if the sign bit is
            // identical, then the upper bits don't matter, so masking them
            // off both sides is fine.
            curr->left = makeZeroExt(ext, bits);
            c->value = c->value.and_(Literal(Bits::lowBitMask(bits)));
            return replaceCurrent(curr);
          } else {
            // Otherwise, C's relevant bits are mixed, and then the two sides
            // can never be equal, as the left side's bits cannot be mixed.
            Builder builder(*getModule());
            // The result is either always true, or always false.
            c->value = Literal::makeFromInt32(curr->op == NeInt32, c->type);
            return replaceCurrent(
              builder.makeSequence(builder.makeDrop(ext), c));
          }
        }
      } else if (auto* left = Properties::getSignExtValue(curr->left)) {
        if (auto* right = Properties::getSignExtValue(curr->right)) {
          auto bits = Properties::getSignExtBits(curr->left);
          if (Properties::getSignExtBits(curr->right) == bits) {
            // we are comparing two sign-exts with the same bits, so we may as
            // well replace both with cheaper zexts
            curr->left = makeZeroExt(left, bits);
            curr->right = makeZeroExt(right, bits);
            return replaceCurrent(curr);
          }
        } else if (auto* load = curr->right->dynCast<Load>()) {
          // we are comparing a load to a sign-ext, we may be able to switch
          // to zext
          auto leftBits = Properties::getSignExtBits(curr->left);
          if (load->signed_ && leftBits == load->bytes * 8) {
            load->signed_ = false;
            curr->left = makeZeroExt(left, leftBits);
            return replaceCurrent(curr);
          }
        }
      } else if (auto* load = curr->left->dynCast<Load>()) {
        if (auto* right = Properties::getSignExtValue(curr->right)) {
          // we are comparing a load to a sign-ext, we may be able to switch
          // to zext
          auto rightBits = Properties::getSignExtBits(curr->right);
          if (load->signed_ && rightBits == load->bytes * 8) {
            load->signed_ = false;
            curr->right = makeZeroExt(right, rightBits);
            return replaceCurrent(curr);
          }
        }
      }
      // note that both left and right may be consts, but then we let
      // precompute compute the constant result
    } else if (curr->op == AddInt32 || curr->op == AddInt64 ||
               curr->op == SubInt32 || curr->op == SubInt64) {
      if (auto* ret = optimizeAddedConstants(curr)) {
        return replaceCurrent(ret);
      }
    } else if (curr->op == MulFloat32 || curr->op == MulFloat64 ||
               curr->op == DivFloat32 || curr->op == DivFloat64) {
      if (curr->left->type == curr->right->type) {
        if (auto* leftUnary = curr->left->dynCast<Unary>()) {
          if (leftUnary->op == Abstract::getUnary(curr->type, Abstract::Abs)) {
            if (auto* rightUnary = curr->right->dynCast<Unary>()) {
              if (leftUnary->op == rightUnary->op) { // both are abs ops
                // abs(x) * abs(y)   ==>   abs(x * y)
                // abs(x) / abs(y)   ==>   abs(x / y)
                curr->left = leftUnary->value;
                curr->right = rightUnary->value;
                leftUnary->value = curr;
                return replaceCurrent(leftUnary);
              }
            }
          }
        }
      }
    }
    // a bunch of operations on a constant right side can be simplified
    if (auto* right = curr->right->dynCast<Const>()) {
      if (curr->op == AndInt32) {
        auto mask = right->value.geti32();
        // and with -1 does nothing (common in asm.js output)
        if (mask == -1) {
          return replaceCurrent(curr->left);
        }
        // small loads do not need to be masked, the load itself masks
        if (auto* load = curr->left->dynCast<Load>()) {
          if ((load->bytes == 1 && mask == 0xff) ||
              (load->bytes == 2 && mask == 0xffff)) {
            load->signed_ = false;
            return replaceCurrent(curr->left);
          }
        } else if (auto maskedBits = Bits::getMaskedBits(mask)) {
          if (Bits::getMaxBits(curr->left, this) <= maskedBits) {
            // a mask of lower bits is not needed if we are already smaller
            return replaceCurrent(curr->left);
          }
        }
      }
      // some math operations have trivial results
      if (auto* ret = optimizeWithConstantOnRight(curr)) {
        return replaceCurrent(ret);
      }
      if (auto* ret = optimizeDoubletonWithConstantOnRight(curr)) {
        return replaceCurrent(ret);
      }
      if (right->type == Type::i32) {
        BinaryOp op;
        int32_t c = right->value.geti32();
        // First, try to lower signed operations to unsigned if that is
        // possible. Some unsigned operations like div_u or rem_u are usually
        // faster on VMs. Also this opens more possibilities for further
        // simplifications afterwards.
        if (c >= 0 && (op = makeUnsignedBinaryOp(curr->op)) != InvalidBinary &&
            Bits::getMaxBits(curr->left, this) <= 31) {
          curr->op = op;
        }
        if (c < 0 && c > std::numeric_limits<int32_t>::min() &&
            curr->op == DivUInt32) {
          // u32(x) / C   ==>   u32(x) >= C  iff C > 2^31
          // We avoid applying this for C == 2^31 due to conflict
          // with other rule which transform to more prefereble
          // right shift operation.
          curr->op = c == -1 ? EqInt32 : GeUInt32;
          return replaceCurrent(curr);
        }
        if (Bits::isPowerOf2((uint32_t)c)) {
          switch (curr->op) {
            case MulInt32:
              return replaceCurrent(optimizePowerOf2Mul(curr, (uint32_t)c));
            case RemUInt32:
              return replaceCurrent(optimizePowerOf2URem(curr, (uint32_t)c));
            case DivUInt32:
              return replaceCurrent(optimizePowerOf2UDiv(curr, (uint32_t)c));
            default:
              break;
          }
        }
      }
      if (right->type == Type::i64) {
        BinaryOp op;
        int64_t c = right->value.geti64();
        // See description above for Type::i32
        if (c >= 0 && (op = makeUnsignedBinaryOp(curr->op)) != InvalidBinary &&
            Bits::getMaxBits(curr->left, this) <= 63) {
          curr->op = op;
        }
        if (getPassOptions().shrinkLevel == 0 && c < 0 &&
            c > std::numeric_limits<int64_t>::min() && curr->op == DivUInt64) {
          // u64(x) / C   ==>   u64(u64(x) >= C)  iff C > 2^63
          // We avoid applying this for C == 2^31 due to conflict
          // with other rule which transform to more prefereble
          // right shift operation.
          // And apply this only for shrinkLevel == 0 due to it
          // increasing size by one byte.
          curr->op = c == -1LL ? EqInt64 : GeUInt64;
          curr->type = Type::i32;
          return replaceCurrent(
            Builder(*getModule()).makeUnary(ExtendUInt32, curr));
        }
        if (Bits::isPowerOf2((uint64_t)c)) {
          switch (curr->op) {
            case MulInt64:
              return replaceCurrent(optimizePowerOf2Mul(curr, (uint64_t)c));
            case RemUInt64:
              return replaceCurrent(optimizePowerOf2URem(curr, (uint64_t)c));
            case DivUInt64:
              return replaceCurrent(optimizePowerOf2UDiv(curr, (uint64_t)c));
            default:
              break;
          }
        }
      }
      if (curr->op == DivFloat32) {
        float c = right->value.getf32();
        if (Bits::isPowerOf2InvertibleFloat(c)) {
          return replaceCurrent(optimizePowerOf2FDiv(curr, c));
        }
      }
      if (curr->op == DivFloat64) {
        double c = right->value.getf64();
        if (Bits::isPowerOf2InvertibleFloat(c)) {
          return replaceCurrent(optimizePowerOf2FDiv(curr, c));
        }
      }
    }
    // a bunch of operations on a constant left side can be simplified
    if (curr->left->is<Const>()) {
      if (auto* ret = optimizeWithConstantOnLeft(curr)) {
        return replaceCurrent(ret);
      }
    }
    if (curr->op == AndInt32 || curr->op == OrInt32) {
      if (curr->op == AndInt32) {
        if (auto* ret = combineAnd(curr)) {
          return replaceCurrent(ret);
        }
      }
      // for or, we can potentially combine
      if (curr->op == OrInt32) {
        if (auto* ret = combineOr(curr)) {
          return replaceCurrent(ret);
        }
      }
      // bitwise operations
      // for and and or, we can potentially conditionalize
      if (auto* ret = conditionalizeExpensiveOnBitwise(curr)) {
        return replaceCurrent(ret);
      }
    }
    // relation/comparisons allow for math optimizations
    if (curr->isRelational()) {
      if (auto* ret = optimizeRelational(curr)) {
        return replaceCurrent(ret);
      }
    }
    // finally, try more expensive operations on the curr in
    // the case that they have no side effects
    if (!effects(curr->left).hasSideEffects()) {
      if (ExpressionAnalyzer::equal(curr->left, curr->right)) {
        if (auto* ret = optimizeBinaryWithEqualEffectlessChildren(curr)) {
          return replaceCurrent(ret);
        }
      }
    }

    if (auto* ret = deduplicateBinary(curr)) {
      return replaceCurrent(ret);
    }
  }

  void visitUnary(Unary* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }

    {
      using namespace Match;
      using namespace Abstract;
      Builder builder(*getModule());
      {
        // eqz(x - y)  =>  x == y
        Binary* inner;
        if (matches(curr, unary(EqZ, binary(&inner, Sub, any(), any())))) {
          inner->op = Abstract::getBinary(inner->left->type, Eq);
          inner->type = Type::i32;
          return replaceCurrent(inner);
        }
      }
      {
        // eqz(x + C)  =>  x == -C
        Const* c;
        Binary* inner;
        if (matches(curr, unary(EqZ, binary(&inner, Add, any(), ival(&c))))) {
          c->value = c->value.neg();
          inner->op = Abstract::getBinary(c->type, Eq);
          inner->type = Type::i32;
          return replaceCurrent(inner);
        }
      }
      {
        // eqz((signed)x % C_pot)  =>  eqz(x & (abs(C_pot) - 1))
        Const* c;
        Binary* inner;
        if (matches(curr, unary(EqZ, binary(&inner, RemS, any(), ival(&c)))) &&
            (c->value.isSignedMin() ||
             Bits::isPowerOf2(c->value.abs().getInteger()))) {
          inner->op = Abstract::getBinary(c->type, And);
          if (c->value.isSignedMin()) {
            c->value = Literal::makeSignedMax(c->type);
          } else {
            c->value = c->value.abs().sub(Literal::makeOne(c->type));
          }
          return replaceCurrent(curr);
        }
      }
      {
        // i32.wrap_i64 can be removed if the operations inside it do not
        // actually require 64 bits, e.g.:
        //
        // i32.wrap_i64(i64.extend_i32_u(x))  =>  x
        if (matches(curr, unary(WrapInt64, any()))) {
          if (auto* ret = optimizeWrappedResult(curr)) {
            return replaceCurrent(ret);
          }
        }
      }
      {
        // i32.eqz(i32.wrap_i64(x))  =>  i64.eqz(x)
        //   where maxBits(x) <= 32
        Unary* inner;
        Expression* x;
        if (matches(curr, unary(EqZInt32, unary(&inner, WrapInt64, any(&x)))) &&
            Bits::getMaxBits(x, this) <= 32) {
          inner->op = EqZInt64;
          return replaceCurrent(inner);
        }
      }
      {
        // i32.eqz(i32.eqz(x))  =>  i32(x) != 0
        // i32.eqz(i64.eqz(x))  =>  i64(x) != 0
        //   iff shinkLevel == 0
        // (1 instruction instead of 2, but 1 more byte)
        if (getPassRunner()->options.shrinkLevel == 0) {
          Expression* x;
          if (matches(curr, unary(EqZInt32, unary(EqZ, any(&x))))) {
            Builder builder(*getModule());
            return replaceCurrent(builder.makeBinary(
              getBinary(x->type, Ne),
              x,
              builder.makeConst(Literal::makeZero(x->type))));
          }
        }
      }
      {
        // i64.extend_i32_s(i32.wrap_i64(x))  =>  x
        //   where maxBits(x) <= 31
        //
        // i64.extend_i32_u(i32.wrap_i64(x))  =>  x
        //   where maxBits(x) <= 32
        Expression* x;
        UnaryOp unaryOp;
        if (matches(curr, unary(&unaryOp, unary(WrapInt64, any(&x))))) {
          if (unaryOp == ExtendSInt32 || unaryOp == ExtendUInt32) {
            auto maxBits = Bits::getMaxBits(x, this);
            if ((unaryOp == ExtendSInt32 && maxBits <= 31) ||
                (unaryOp == ExtendUInt32 && maxBits <= 32)) {
              return replaceCurrent(x);
            }
          }
        }
      }
      if (getModule()->features.hasSignExt()) {
        // i64.extend_i32_s(i32.wrap_i64(x))  =>  i64.extend32_s(x)
        Unary* inner;
        if (matches(curr,
                    unary(ExtendSInt32, unary(&inner, WrapInt64, any())))) {
          inner->op = ExtendS32Int64;
          inner->type = Type::i64;
          return replaceCurrent(inner);
        }
      }
    }

    if (curr->op == ExtendUInt32 || curr->op == ExtendSInt32) {
      if (auto* load = curr->value->dynCast<Load>()) {
        // i64.extend_i32_s(i32.load(_8|_16)(_u|_s)(x))  =>
        //    i64.load(_8|_16|_32)(_u|_s)(x)
        //
        // i64.extend_i32_u(i32.load(_8|_16)(_u|_s)(x))  =>
        //    i64.load(_8|_16|_32)(_u|_s)(x)
        //
        // but we can't do this in following cases:
        //
        //    i64.extend_i32_u(i32.load8_s(x))
        //    i64.extend_i32_u(i32.load16_s(x))
        //
        // this mixed sign/zero extensions can't represent in single
        // signed or unsigned 64-bit load operation. For example if `load8_s(x)`
        // return i8(-1) (0xFF) than sign extended result will be
        // i32(-1) (0xFFFFFFFF) and with zero extension to i64 we got
        // finally 0x00000000FFFFFFFF. However with `i64.load8_s` in this
        // situation we got `i64(-1)` (all ones) and with `i64.load8_u` it
        // will be 0x00000000000000FF.
        //
        // Another limitation is atomics which only have unsigned loads.
        // So we also avoid this only case:
        //
        //   i64.extend_i32_s(i32.atomic.load(x))

        // Special case for i32.load. In this case signedness depends on
        // extend operation.
        bool willBeSigned = curr->op == ExtendSInt32 && load->bytes == 4;
        if (!(curr->op == ExtendUInt32 && load->bytes <= 2 && load->signed_) &&
            !(willBeSigned && load->isAtomic)) {
          if (willBeSigned) {
            load->signed_ = true;
          }
          load->type = Type::i64;
          return replaceCurrent(load);
        }
      }
    }

    if (Abstract::hasAnyReinterpret(curr->op)) {
      // i32.reinterpret_f32(f32.reinterpret_i32(x))  =>  x
      // i64.reinterpret_f64(f64.reinterpret_i64(x))  =>  x
      // f32.reinterpret_i32(i32.reinterpret_f32(x))  =>  x
      // f64.reinterpret_i64(i64.reinterpret_f64(x))  =>  x
      if (auto* inner = curr->value->dynCast<Unary>()) {
        if (Abstract::hasAnyReinterpret(inner->op)) {
          if (inner->value->type == curr->type) {
            return replaceCurrent(inner->value);
          }
        }
      }
      // f32.reinterpret_i32(i32.load(x))  =>  f32.load(x)
      // f64.reinterpret_i64(i64.load(x))  =>  f64.load(x)
      // i32.reinterpret_f32(f32.load(x))  =>  i32.load(x)
      // i64.reinterpret_f64(f64.load(x))  =>  i64.load(x)
      if (auto* load = curr->value->dynCast<Load>()) {
        if (!load->isAtomic && load->bytes == curr->type.getByteSize()) {
          load->type = curr->type;
          return replaceCurrent(load);
        }
      }
    }

    if (curr->op == EqZInt32) {
      if (auto* inner = curr->value->dynCast<Binary>()) {
        // Try to invert a relational operation using De Morgan's law
        auto op = invertBinaryOp(inner->op);
        if (op != InvalidBinary) {
          inner->op = op;
          return replaceCurrent(inner);
        }
      }
      // eqz of a sign extension can be of zero-extension
      if (auto* ext = Properties::getSignExtValue(curr->value)) {
        // we are comparing a sign extend to a constant, which means we can
        // use a cheaper zext
        auto bits = Properties::getSignExtBits(curr->value);
        curr->value = makeZeroExt(ext, bits);
        return replaceCurrent(curr);
      }
    } else if (curr->op == AbsFloat32 || curr->op == AbsFloat64) {
      // abs(-x)   ==>   abs(x)
      if (auto* unaryInner = curr->value->dynCast<Unary>()) {
        if (unaryInner->op ==
            Abstract::getUnary(unaryInner->type, Abstract::Neg)) {
          curr->value = unaryInner->value;
          return replaceCurrent(curr);
        }
      }
      // abs(x * x)   ==>   x * x
      // abs(x / x)   ==>   x / x
      if (auto* binary = curr->value->dynCast<Binary>()) {
        if ((binary->op == Abstract::getBinary(binary->type, Abstract::Mul) ||
             binary->op == Abstract::getBinary(binary->type, Abstract::DivS)) &&
            areConsecutiveInputsEqual(binary->left, binary->right)) {
          return replaceCurrent(binary);
        }
        // abs(0 - x)   ==>   abs(x),
        // only for fast math
        if (fastMath &&
            binary->op == Abstract::getBinary(binary->type, Abstract::Sub)) {
          if (auto* c = binary->left->dynCast<Const>()) {
            if (c->value.isZero()) {
              curr->value = binary->right;
              return replaceCurrent(curr);
            }
          }
        }
      }
    }

    if (auto* ret = deduplicateUnary(curr)) {
      return replaceCurrent(ret);
    }

    if (auto* ret = simplifyRoundingsAndConversions(curr)) {
      return replaceCurrent(ret);
    }
  }

  void visitSelect(Select* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }
    if (auto* ret = optimizeSelect(curr)) {
      return replaceCurrent(ret);
    }
    optimizeTernary(curr);
  }

  void visitGlobalSet(GlobalSet* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }
    // optimize out a set of a get
    auto* get = curr->value->dynCast<GlobalGet>();
    if (get && get->name == curr->name) {
      ExpressionManipulator::nop(curr);
      return replaceCurrent(curr);
    }
  }

  void visitBlock(Block* curr) {
    if (getModule()->features.hasGC()) {
      optimizeHeapStores(curr->list);
    }
  }

  void visitIf(If* curr) {
    curr->condition = optimizeBoolean(curr->condition);
    if (curr->ifFalse) {
      if (auto* unary = curr->condition->dynCast<Unary>()) {
        if (unary->op == EqZInt32) {
          // flip if-else arms to get rid of an eqz
          curr->condition = unary->value;
          std::swap(curr->ifTrue, curr->ifFalse);
        }
      }
      if (curr->condition->type != Type::unreachable &&
          ExpressionAnalyzer::equal(curr->ifTrue, curr->ifFalse)) {
        // The sides are identical, so fold. If we can replace the If with one
        // arm and there are no side effects in the condition, replace it. But
        // make sure not to change a concrete expression to an unreachable
        // expression because we want to avoid having to refinalize.
        bool needCondition = effects(curr->condition).hasSideEffects();
        bool wouldBecomeUnreachable =
          curr->type.isConcrete() && curr->ifTrue->type == Type::unreachable;
        Builder builder(*getModule());
        if (!wouldBecomeUnreachable && !needCondition) {
          return replaceCurrent(curr->ifTrue);
        } else if (!wouldBecomeUnreachable) {
          return replaceCurrent(builder.makeSequence(
            builder.makeDrop(curr->condition), curr->ifTrue));
        } else {
          // Emit a block with the original concrete type.
          auto* ret = builder.makeBlock();
          if (needCondition) {
            ret->list.push_back(builder.makeDrop(curr->condition));
          }
          ret->list.push_back(curr->ifTrue);
          ret->finalize(curr->type);
          return replaceCurrent(ret);
        }
      }
      optimizeTernary(curr);
    }
  }

  void visitLocalSet(LocalSet* curr) {
    // Interactions between local.set/tee and ref.as_non_null can be optimized
    // in some cases, by removing or moving the ref.as_non_null operation. In
    // all cases, we only do this when we do *not* allow non-nullable locals. If
    // we do allow such locals, then (1) this local might be non-nullable, so we
    // can't remove or move a ref.as_non_null flowing into a local.set/tee, and
    // (2) even if the local were nullable, if we change things we might prevent
    // the LocalSubtyping pass from turning it into a non-nullable local later.
    // Note that we must also check if this local is nullable regardless, as a
    // parameter might be non-nullable even if nullable locals are disallowed
    // (as that just affects vars, and not params).
    if (auto* as = curr->value->dynCast<RefAs>()) {
      if (as->op == RefAsNonNull && !getModule()->features.hasGCNNLocals() &&
          getFunction()->getLocalType(curr->index).isNullable()) {
        //   (local.tee (ref.as_non_null ..))
        // =>
        //   (ref.as_non_null (local.tee ..))
        //
        // The reordering allows the ref.as to be potentially optimized further
        // based on where the value flows to.
        if (curr->isTee()) {
          curr->value = as->value;
          curr->finalize();
          as->value = curr;
          as->finalize();
          replaceCurrent(as);
          return;
        }

        // Otherwise, if this is not a tee, then no value falls through. The
        // ref.as_non_null acts as a null check here, basically. If we are
        // ignoring such traps, we can remove it.
        auto& passOptions = getPassOptions();
        if (passOptions.ignoreImplicitTraps || passOptions.trapsNeverHappen) {
          curr->value = as->value;
        }
      }
    }
  }

  void visitBreak(Break* curr) {
    if (curr->condition) {
      curr->condition = optimizeBoolean(curr->condition);
    }
  }

  void visitLoad(Load* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }
    optimizeMemoryAccess(curr->ptr, curr->offset, curr->memory);
  }

  void visitStore(Store* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }
    optimizeMemoryAccess(curr->ptr, curr->offset, curr->memory);
    optimizeStoredValue(curr->value, curr->bytes);
    if (auto* unary = curr->value->dynCast<Unary>()) {
      if (unary->op == WrapInt64) {
        // instead of wrapping to 32, just store some of the bits in the i64
        curr->valueType = Type::i64;
        curr->value = unary->value;
      } else if (!curr->isAtomic && Abstract::hasAnyReinterpret(unary->op) &&
                 curr->bytes == curr->valueType.getByteSize()) {
        // f32.store(y, f32.reinterpret_i32(x))  =>  i32.store(y, x)
        // f64.store(y, f64.reinterpret_i64(x))  =>  i64.store(y, x)
        // i32.store(y, i32.reinterpret_f32(x))  =>  f32.store(y, x)
        // i64.store(y, i64.reinterpret_f64(x))  =>  f64.store(y, x)
        curr->valueType = unary->value->type;
        curr->value = unary->value;
      }
    }
  }

  void optimizeStoredValue(Expression*& value, Index bytes) {
    if (!value->type.isInteger()) {
      return;
    }
    // truncates constant values during stores
    // (i32|i64).store(8|16|32)(p, C)   ==>
    //    (i32|i64).store(8|16|32)(p, C & mask)
    if (auto* c = value->dynCast<Const>()) {
      if (value->type == Type::i64 && bytes == 4) {
        c->value = c->value.and_(Literal(uint64_t(0xffffffff)));
      } else {
        c->value = c->value.and_(
          Literal::makeFromInt32(Bits::lowBitMask(bytes * 8), value->type));
      }
    }
    // stores of fewer bits truncates anyhow
    if (auto* binary = value->dynCast<Binary>()) {
      if (binary->op == AndInt32) {
        if (auto* right = binary->right->dynCast<Const>()) {
          if (right->type == Type::i32) {
            auto mask = right->value.geti32();
            if ((bytes == 1 && mask == 0xff) ||
                (bytes == 2 && mask == 0xffff)) {
              value = binary->left;
            }
          }
        }
      } else if (auto* ext = Properties::getSignExtValue(binary)) {
        // if sign extending the exact bit size we store, we can skip the
        // extension if extending something bigger, then we just alter bits we
        // don't save anyhow
        if (Properties::getSignExtBits(binary) >= Index(bytes) * 8) {
          value = ext;
        }
      }
    }
  }

  void visitMemoryCopy(MemoryCopy* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }
    assert(getModule()->features.hasBulkMemory());
    if (auto* ret = optimizeMemoryCopy(curr)) {
      return replaceCurrent(ret);
    }
  }

  void visitMemoryFill(MemoryFill* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }
    assert(getModule()->features.hasBulkMemory());
    if (auto* ret = optimizeMemoryFill(curr)) {
      return replaceCurrent(ret);
    }
  }

  void visitCallRef(CallRef* curr) {
    skipNonNullCast(curr->target, curr);
    if (trapOnNull(curr, curr->target)) {
      return;
    }
    if (curr->target->type == Type::unreachable) {
      // The call_ref is not reached; leave this for DCE.
      return;
    }

    if (auto* ref = curr->target->dynCast<RefFunc>()) {
      // We know the target!
      replaceCurrent(
        Builder(*getModule())
          .makeCall(ref->func, curr->operands, curr->type, curr->isReturn));
      return;
    }

    if (auto* get = curr->target->dynCast<TableGet>()) {
      // (call_ref ..args.. (table.get $table (index))
      //   =>
      // (call_indirect $table ..args.. (index))
      replaceCurrent(Builder(*getModule())
                       .makeCallIndirect(get->table,
                                         get->index,
                                         curr->operands,
                                         get->type.getHeapType(),
                                         curr->isReturn));
      return;
    }

    auto features = getModule()->features;

    // It is possible the target is not a function reference, but we can infer
    // the fallthrough value there. It takes more work to optimize this case,
    // but it is pretty important to allow a call_ref to become a fast direct
    // call, so make the effort.
    if (auto* ref = Properties::getFallthrough(
                      curr->target, getPassOptions(), *getModule())
                      ->dynCast<RefFunc>()) {
      // Check if the fallthrough make sense. We may have cast it to a different
      // type, which would be a problem - we'd be replacing a call_ref to one
      // type with a direct call to a function of another type. That would trap
      // at runtime; be careful not to emit invalid IR here.
      if (curr->target->type.getHeapType() != ref->type.getHeapType()) {
        return;
      }
      Builder builder(*getModule());
      if (curr->operands.empty()) {
        // No operands, so this is simple and there is nothing to reorder: just
        // emit:
        //
        // (block
        //  (drop curr->target)
        //  (call ref.func-from-curr->target)
        // )
        replaceCurrent(builder.makeSequence(
          builder.makeDrop(curr->target),
          builder.makeCall(ref->func, {}, curr->type, curr->isReturn)));
        return;
      }

      // In the presence of operands, we must execute the code in curr->target
      // after the last operand and before the call happens. Interpose at the
      // last operand:
      //
      // (call ref.func-from-curr->target)
      //  (operand1)
      //  (..)
      //  (operandN-1)
      //  (block
      //   (local.set $temp (operandN))
      //   (drop curr->target)
      //   (local.get $temp)
      //  )
      // )
      auto* lastOperand = curr->operands.back();
      auto lastOperandType = lastOperand->type;
      if (lastOperandType == Type::unreachable) {
        // The call_ref is not reached; leave this for DCE.
        return;
      }
      if (!TypeUpdating::canHandleAsLocal(lastOperandType)) {
        // We cannot create a local, so we must give up.
        return;
      }
      Index tempLocal = builder.addVar(
        getFunction(),
        TypeUpdating::getValidLocalType(lastOperandType, features));
      auto* set = builder.makeLocalSet(tempLocal, lastOperand);
      auto* drop = builder.makeDrop(curr->target);
      auto* get = TypeUpdating::fixLocalGet(
        builder.makeLocalGet(tempLocal, lastOperandType), *getModule());
      curr->operands.back() = builder.makeBlock({set, drop, get});
      replaceCurrent(builder.makeCall(
        ref->func, curr->operands, curr->type, curr->isReturn));
      return;
    }

    // If the target is a select of two different constants, we can emit an if
    // over two direct calls.
    if (auto* calls = CallUtils::convertToDirectCalls(
          curr,
          [](Expression* target) -> CallUtils::IndirectCallInfo {
            if (auto* refFunc = target->dynCast<RefFunc>()) {
              return CallUtils::Known{refFunc->func};
            }
            return CallUtils::Unknown{};
          },
          *getFunction(),
          *getModule())) {
      replaceCurrent(calls);
    }
  }

  // Note on removing casts (which the following utilities, skipNonNullCast and
  // skipCast do): removing a cast is potentially dangerous, as it removes
  // information from the IR. For example:
  //
  //  (ref.is_func
  //    (ref.as_func
  //      (local.get $anyref)))
  //
  // The local has no useful type info here (it is anyref). The cast forces it
  // to be a function, so we know that if we do not trap then the ref.is will
  // definitely be 1. But if we removed the ref.as first (which we can do in
  // traps-never-happen mode) then we'd not have the type info we need to
  // optimize that way.
  //
  // To avoid such risks we should keep in mind the following:
  //
  //  * Before removing a cast we should use its type information in the best
  //    way we can. Only after doing so should a cast be removed. In the exmaple
  //    above, that means first seeing that the ref.is must return 1, and only
  //    then possibly removing the ref.as.
  //  * Do not remove a cast if removing it might remove useful information for
  //    others. For example,
  //
  //      (ref.cast $A
  //        (ref.as_non_null ..))
  //
  //    If we remove the inner cast then the outer cast becomes nullable. That
  //    means we'd be throwing away useful information, which we should not do,
  //    even in traps-never-happen mode and even if the wasm would validate
  //    without the cast. Only if we saw that the parents of the outer cast
  //    cannot benefit from non-nullability should we remove it.
  //    Another example:
  //
  //      (struct.get $A 0
  //        (ref.cast $B ..))
  //
  //    The cast only changes the type of the reference, which is consumed in
  //    this expression and so we don't have more parents to consider. But it is
  //    risky to remove this cast, since e.g. GUFA benefits from such info:
  //    it tells GUFA that we are reading from a $B here, and not the supertype
  //    $A. If $B may contain fewer values in field 0 than $A, then GUFA might
  //    be able to optimize better with this cast. Now, in traps-never-happen
  //    mode we can assume that only $B can arrive here, which means GUFA might
  //    be able to infer that even without the cast - but it might not, if we
  //    hit a limitation of GUFA. Some code patterns simply cannot be expected
  //    to be always inferred, say if a data structure has a tagged variant:
  //
  //      {
  //        tag: i32,
  //        ref: anyref
  //      }
  //
  //    Imagine that if tag == 0 then the reference always contains struct $A,
  //    and if tag == 1 then it always contains a struct $B, and so forth. We
  //    can't expect GUFA to figure out such invariants in general. But by
  //    having casts in the right places we can help GUFA optimize:
  //
  //      (if
  //        (tag == 1)
  //        (struct.get $A 0
  //          (ref.cast $B ..))
  //
  //    We know it must be a $B due to the tag. By keeping the cast there we can
  //    make sure that optimizations can benefit from that.
  //
  //    Given the large amount of potential benefit we can get from a successful
  //    optimization in GUFA, any reduction there may be a bad idea, so we
  //    should be very careful and probably *not* remove such casts.

  // If an instruction traps on a null input, there is no need for a
  // ref.as_non_null on that input: we will trap either way (and the binaryen
  // optimizer does not differentiate traps).
  //
  // See "notes on removing casts", above. However, in most cases removing a
  // non-null cast is obviously safe to do, since we only remove one if another
  // check will happen later.
  //
  // We also pass in the parent, because we need to be careful about ordering:
  // if the parent has other children than |input| then we may not be able to
  // remove the trap. For example,
  //
  //  (struct.set
  //   (ref.as_non_null X)
  //   (call $foo)
  //  )
  //
  // If X is null we'd trap before the call to $foo. If we remove the
  // ref.as_non_null then the struct.set will still trap, of course, but that
  // will only happen *after* the call, which is wrong.
  void skipNonNullCast(Expression*& input, Expression* parent) {
    // Check the other children for the ordering problem only if we find a
    // possible optimization, to avoid wasted work.
    bool checkedSiblings = false;
    auto& options = getPassOptions();
    while (1) {
      if (auto* as = input->dynCast<RefAs>()) {
        if (as->op == RefAsNonNull) {
          // The problem with effect ordering that is described above is not an
          // issue if traps are assumed to never happen anyhow.
          if (!checkedSiblings && !options.trapsNeverHappen) {
            // We need to see if a child with side effects exists after |input|.
            // If there is such a child, it is a problem as mentioned above (it
            // is fine for such a child to appear *before* |input|, as then we
            // wouldn't be reordering effects). Thus, all we need to do is
            // accumulate the effects in children after |input|, as we want to
            // move the trap across those.
            bool seenInput = false;
            EffectAnalyzer crossedEffects(options, *getModule());
            for (auto* child : ChildIterator(parent)) {
              if (child == input) {
                seenInput = true;
              } else if (seenInput) {
                crossedEffects.walk(child);
              }
            }

            // Check if the effects we cross interfere with the effects of the
            // trap we want to move. (We use a shallow effect analyzer since we
            // will only move the ref.as_non_null itself.)
            ShallowEffectAnalyzer movingEffects(options, *getModule(), input);
            if (crossedEffects.invalidates(movingEffects)) {
              return;
            }

            // If we got here, we've checked the siblings and found no problem.
            checkedSiblings = true;
          }
          input = as->value;
          continue;
        }
      }
      break;
    }
  }

  // As skipNonNullCast, but skips all casts if we can do so. This is useful in
  // cases where we don't actually care about the type but just the value, that
  // is, if casts of the type do not affect our behavior (which is the case in
  // ref.eq for example).
  //
  // |requiredType| is the required supertype of the final output. We will not
  // remove a cast that would leave something that would break that. If
  // |requiredType| is not provided we will accept any type there.
  //
  // See "notes on removing casts", above, for when this is safe to do.
  void skipCast(Expression*& input, Type requiredType = Type::none) {
    // Traps-never-happen mode is a requirement for us to optimize here.
    if (!getPassOptions().trapsNeverHappen) {
      return;
    }
    while (1) {
      if (auto* as = input->dynCast<RefAs>()) {
        if (requiredType == Type::none ||
            Type::isSubType(as->value->type, requiredType)) {
          input = as->value;
          continue;
        }
      } else if (auto* cast = input->dynCast<RefCast>()) {
        if (requiredType == Type::none ||
            Type::isSubType(cast->ref->type, requiredType)) {
          input = cast->ref;
          continue;
        }
      }
      break;
    }
  }

  // Appends a result after the dropped children, if we need them.
  Expression* getDroppedChildrenAndAppend(Expression* curr,
                                          Expression* result) {
    return wasm::getDroppedChildrenAndAppend(
      curr, *getModule(), getPassOptions(), result);
  }

  Expression* getDroppedChildrenAndAppend(Expression* curr, Literal value) {
    auto* result = Builder(*getModule()).makeConst(value);
    return getDroppedChildrenAndAppend(curr, result);
  }

  Expression* getResultOfFirst(Expression* first, Expression* second) {
    return wasm::getResultOfFirst(
      first, second, getFunction(), getModule(), getPassOptions());
  }

  // Optimize an instruction and the reference it operates on, under the
  // assumption that if the reference is a null then we will trap. Returns true
  // if we replaced the expression with something simpler. Returns false if we
  // found nothing to optimize, or if we just modified or replaced the ref (but
  // not the expression itself).
  bool trapOnNull(Expression* curr, Expression*& ref) {
    Builder builder(*getModule());

    if (getPassOptions().trapsNeverHappen) {
      // We can ignore the possibility of the reference being an input, so
      //
      //    (if
      //      (condition)
      //      (null)
      //      (other))
      // =>
      //    (drop
      //      (condition))
      //    (other)
      //
      // That is, we will by assumption not read from the null, so remove that
      // arm.
      //
      // TODO We could recurse here.
      // TODO We could do similar things for casts (rule out an impossible arm).
      // TODO Worth thinking about an 'assume' instrinsic of some form that
      //      annotates knowledge about a value, or another mechanism to allow
      //      that information to be passed around.
      if (auto* iff = ref->dynCast<If>()) {
        if (iff->ifFalse) {
          if (iff->ifTrue->type.isNull()) {
            if (ref->type != iff->ifFalse->type) {
              refinalize = true;
            }
            ref = builder.makeSequence(builder.makeDrop(iff->condition),
                                       iff->ifFalse);
            return false;
          }
          if (iff->ifFalse->type.isNull()) {
            if (ref->type != iff->ifTrue->type) {
              refinalize = true;
            }
            ref = builder.makeSequence(builder.makeDrop(iff->condition),
                                       iff->ifTrue);
            return false;
          }
        }
      }

      if (auto* select = ref->dynCast<Select>()) {
        if (select->ifTrue->type.isNull()) {
          ref = builder.makeSequence(
            builder.makeDrop(select->ifTrue),
            getResultOfFirst(select->ifFalse,
                             builder.makeDrop(select->condition)));
          return false;
        }
        if (select->ifFalse->type.isNull()) {
          ref = getResultOfFirst(
            select->ifTrue,
            builder.makeSequence(builder.makeDrop(select->ifFalse),
                                 builder.makeDrop(select->condition)));
          return false;
        }
      }
    }

    // A nullable cast can be turned into a non-nullable one:
    //
    //    (struct.get ;; or something else that traps on a null ref
    //      (ref.cast null
    // =>
    //    (struct.get
    //      (ref.cast      ;; now non-nullable
    //
    // Either way we trap here, but refining the type may have benefits later.
    if (ref->type.isNullable()) {
      if (auto* cast = ref->dynCast<RefCast>()) {
        // Note that we must be the last child of the parent, otherwise effects
        // in the middle may need to remain:
        //
        //    (struct.set
        //      (ref.cast null
        //      (call ..
        //
        // The call here must execute before the trap in the struct.set. To
        // avoid that problem, inspect all children after us. If there are no
        // such children, then there is no problem; if there are, see below.
        auto canOptimize = true;
        auto seenRef = false;
        for (auto* child : ChildIterator(curr)) {
          if (child == ref) {
            seenRef = true;
          } else if (seenRef) {
            // This is a child after the reference. Check it for effects. For
            // simplicity, focus on the case of traps-never-happens: if we can
            // assume no trap occurs in the parent, then there must not be a
            // trap in the child either, unless control flow transfers and we
            // might not reach the parent.
            // TODO: handle more cases.
            if (!getPassOptions().trapsNeverHappen ||
                effects(child).transfersControlFlow()) {
              canOptimize = false;
              break;
            }
          }
        }
        if (canOptimize) {
          cast->type = Type(cast->type.getHeapType(), NonNullable);
        }
      }
    }

    auto fallthrough =
      Properties::getFallthrough(ref, getPassOptions(), *getModule());

    if (fallthrough->type.isNull()) {
      replaceCurrent(
        getDroppedChildrenAndAppend(curr, builder.makeUnreachable()));
      // Propagate the unreachability.
      refinalize = true;
      return true;
    }
    return false;
  }

  void visitRefEq(RefEq* curr) {
    // The types may prove that the same reference cannot appear on both sides.
    auto leftType = curr->left->type;
    auto rightType = curr->right->type;
    if (leftType == Type::unreachable || rightType == Type::unreachable) {
      // Leave this for DCE.
      return;
    }
    auto leftHeapType = leftType.getHeapType();
    auto rightHeapType = rightType.getHeapType();
    auto leftIsHeapSubtype = HeapType::isSubType(leftHeapType, rightHeapType);
    auto rightIsHeapSubtype = HeapType::isSubType(rightHeapType, leftHeapType);
    if (!leftIsHeapSubtype && !rightIsHeapSubtype &&
        (leftType.isNonNullable() || rightType.isNonNullable())) {
      // The heap types have no intersection, so the only thing that can
      // possibly appear on both sides is null, but one of the two is non-
      // nullable, which rules that out. So there is no way that the same
      // reference can appear on both sides.
      replaceCurrent(
        getDroppedChildrenAndAppend(curr, Literal::makeZero(Type::i32)));
      return;
    }

    // Equality does not depend on the type, so casts may be removable.
    //
    // This is safe to do first because nothing farther down cares about the
    // type, and we consume the two input references, so removing a cast could
    // not help our parents (see "notes on removing casts").
    Type nullableEq = Type(HeapType::eq, Nullable);
    skipCast(curr->left, nullableEq);
    skipCast(curr->right, nullableEq);

    // Identical references compare equal.
    // (Technically we do not need to check if the inputs are also foldable into
    // a single one, but we do not have utility code to handle non-foldable
    // cases yet; the foldable case we do handle is the common one of the first
    // child being a tee and the second a get of that tee. TODO)
    if (areConsecutiveInputsEqualAndFoldable(curr->left, curr->right)) {
      replaceCurrent(
        getDroppedChildrenAndAppend(curr, Literal::makeOne(Type::i32)));
      return;
    }

    // Canonicalize to the pattern of a null on the right-hand side, if there is
    // one. This makes pattern matching simpler.
    if (curr->left->is<RefNull>()) {
      std::swap(curr->left, curr->right);
    }

    // RefEq of a value to Null can be replaced with RefIsNull.
    if (curr->right->is<RefNull>()) {
      replaceCurrent(Builder(*getModule()).makeRefIsNull(curr->left));
    }
  }

  void visitStructGet(StructGet* curr) {
    skipNonNullCast(curr->ref, curr);
    trapOnNull(curr, curr->ref);
  }

  void visitStructSet(StructSet* curr) {
    skipNonNullCast(curr->ref, curr);
    if (trapOnNull(curr, curr->ref)) {
      return;
    }

    if (curr->ref->type != Type::unreachable && curr->value->type.isInteger()) {
      // We must avoid the case of a null type.
      auto heapType = curr->ref->type.getHeapType();
      if (heapType.isStruct()) {
        const auto& fields = heapType.getStruct().fields;
        optimizeStoredValue(curr->value, fields[curr->index].getByteSize());
      }
    }

    // If our reference is a tee of a struct.new, we may be able to fold the
    // stored value into the new itself:
    //
    //  (struct.set (local.tee $x (struct.new X Y Z)) X')
    // =>
    //  (local.set $x (struct.new X' Y Z))
    //
    if (auto* tee = curr->ref->dynCast<LocalSet>()) {
      if (auto* new_ = tee->value->dynCast<StructNew>()) {
        if (optimizeSubsequentStructSet(new_, curr, tee->index)) {
          // Success, so we do not need the struct.set any more, and the tee
          // can just be a set instead of us.
          tee->makeSet();
          replaceCurrent(tee);
        }
      }
    }
  }

  // Similar to the above with struct.set whose reference is a tee of a new, we
  // can do the same for subsequent sets in a list:
  //
  //  (local.set $x (struct.new X Y Z))
  //  (struct.set (local.get $x) X')
  // =>
  //  (local.set $x (struct.new X' Y Z))
  //
  // We also handle other struct.sets immediately after this one, but we only
  // handle the case where they are all in sequence and right after the
  // local.set (anything in the middle of this pattern will stop us from
  // optimizing later struct.sets, which might be improved later but would
  // require an analysis of effects TODO).
  void optimizeHeapStores(ExpressionList& list) {
    for (Index i = 0; i < list.size(); i++) {
      auto* localSet = list[i]->dynCast<LocalSet>();
      if (!localSet) {
        continue;
      }
      auto* new_ = localSet->value->dynCast<StructNew>();
      if (!new_) {
        continue;
      }

      // This local.set of a struct.new looks good. Find struct.sets after it
      // to optimize.
      for (Index j = i + 1; j < list.size(); j++) {
        auto* structSet = list[j]->dynCast<StructSet>();
        if (!structSet) {
          // Any time the pattern no longer matches, stop optimizing possible
          // struct.sets for this struct.new.
          break;
        }
        auto* localGet = structSet->ref->dynCast<LocalGet>();
        if (!localGet || localGet->index != localSet->index) {
          break;
        }
        if (!optimizeSubsequentStructSet(new_, structSet, localGet->index)) {
          break;
        } else {
          // Success. Replace the set with a nop, and continue to
          // perhaps optimize more.
          ExpressionManipulator::nop(structSet);
        }
      }
    }
  }

  // Given a struct.new and a struct.set that occurs right after it, and that
  // applies to the same data, try to apply the set during the new. This can be
  // either with a nested tee:
  //
  //  (struct.set
  //    (local.tee $x (struct.new X Y Z))
  //    X'
  //  )
  // =>
  //  (local.set $x (struct.new X' Y Z))
  //
  // or without:
  //
  //  (local.set $x (struct.new X Y Z))
  //  (struct.set (local.get $x) X')
  // =>
  //  (local.set $x (struct.new X' Y Z))
  //
  // Returns true if we succeeded.
  bool optimizeSubsequentStructSet(StructNew* new_,
                                   StructSet* set,
                                   Index refLocalIndex) {
    // Leave unreachable code for DCE, to avoid updating types here.
    if (new_->type == Type::unreachable || set->type == Type::unreachable) {
      return false;
    }

    if (new_->isWithDefault()) {
      // Ignore a new_default for now. If the fields are defaultable then we
      // could add them, in principle, but that might increase code size.
      return false;
    }

    auto index = set->index;
    auto& operands = new_->operands;

    // Check for effects that prevent us moving the struct.set's value (X' in
    // the function comment) into its new position in the struct.new. First, it
    // must be ok to move it past the local.set (otherwise, it might read from
    // memory using that local, and depend on the struct.new having already
    // occurred; or, if it writes to that local, then it would cross another
    // write).
    auto setValueEffects = effects(set->value);
    if (setValueEffects.localsRead.count(refLocalIndex) ||
        setValueEffects.localsWritten.count(refLocalIndex)) {
      return false;
    }

    // We must move the set's value past indexes greater than it (Y and Z in
    // the example in the comment on this function).
    // TODO When this function is called repeatedly in a sequence this can
    //      become quadratic - perhaps we should memoize (though, struct sizes
    //      tend to not be ridiculously large).
    for (Index i = index + 1; i < operands.size(); i++) {
      auto operandEffects = effects(operands[i]);
      if (operandEffects.invalidates(setValueEffects)) {
        // TODO: we could use locals to reorder everything
        return false;
      }
    }

    Builder builder(*getModule());

    // See if we need to keep the old value.
    if (effects(operands[index]).hasUnremovableSideEffects()) {
      operands[index] =
        builder.makeSequence(builder.makeDrop(operands[index]), set->value);
    } else {
      operands[index] = set->value;
    }

    return true;
  }

  void visitArrayGet(ArrayGet* curr) {
    skipNonNullCast(curr->ref, curr);
    trapOnNull(curr, curr->ref);
  }

  void visitArraySet(ArraySet* curr) {
    skipNonNullCast(curr->ref, curr);
    if (trapOnNull(curr, curr->ref)) {
      return;
    }

    if (curr->ref->type != Type::unreachable && curr->value->type.isInteger()) {
      auto element = curr->ref->type.getHeapType().getArray().element;
      optimizeStoredValue(curr->value, element.getByteSize());
    }
  }

  void visitArrayLen(ArrayLen* curr) {
    skipNonNullCast(curr->ref, curr);
    trapOnNull(curr, curr->ref);
  }

  void visitArrayCopy(ArrayCopy* curr) {
    skipNonNullCast(curr->destRef, curr);
    skipNonNullCast(curr->srcRef, curr);
    trapOnNull(curr, curr->destRef) || trapOnNull(curr, curr->srcRef);
  }

  void visitRefCast(RefCast* curr) {
    // Note we must check the ref's type here and not our own, since we only
    // refinalize at the end, which means our type may not have been updated yet
    // after a change in the child.
    // TODO: we could update unreachability up the stack perhaps, or just move
    //       all patterns that can add unreachability to a pass that does so
    //       already like vacuum or dce.
    if (curr->ref->type == Type::unreachable) {
      return;
    }

    if (curr->type.isNonNullable() && trapOnNull(curr, curr->ref)) {
      return;
    }

    // Check whether the cast will definitely fail (or succeed). Look not just
    // at the fallthrough but all intermediatary fallthrough values as well, as
    // if any of them has a type that cannot be cast to us, then we will trap,
    // e.g.
    //
    //   (ref.cast $struct-A
    //     (ref.cast $struct-B
    //       (ref.cast $array
    //         (local.get $x)
    //
    // The fallthrough is the local.get, but the array cast in the middle
    // proves a trap must happen.
    Builder builder(*getModule());
    auto nullType = curr->type.getHeapType().getBottom();
    {
      auto** refp = &curr->ref;
      while (1) {
        auto* ref = *refp;

        auto result = GCTypeUtils::evaluateCastCheck(ref->type, curr->type);

        if (result == GCTypeUtils::Success) {
          // The cast will succeed. This can only happen if the ref is a subtype
          // of the cast instruction, which means we can replace the cast with
          // the ref.
          assert(Type::isSubType(ref->type, curr->type));
          if (curr->type != ref->type) {
            refinalize = true;
          }
          // If there were no intermediate expressions, we can just skip the
          // cast.
          if (ref == curr->ref) {
            replaceCurrent(ref);
            return;
          }
          // Otherwise we can't just remove the cast and replace it with `ref`
          // because the intermediate expressions might have had side effects.
          // We can replace the cast with a drop followed by a direct return of
          // the value, though.
          if (ref->type.isNull()) {
            // We can materialize the resulting null value directly.
            replaceCurrent(builder.makeSequence(builder.makeDrop(curr->ref),
                                                builder.makeRefNull(nullType)));
            return;
          }
          // We need to use a tee to return the value since we can't materialize
          // it directly.
          auto scratch = builder.addVar(getFunction(), ref->type);
          *refp = builder.makeLocalTee(scratch, ref, ref->type);
          replaceCurrent(
            builder.makeSequence(builder.makeDrop(curr->ref),
                                 builder.makeLocalGet(scratch, ref->type)));
          return;
        } else if (result == GCTypeUtils::Failure) {
          // This cast cannot succeed, so it will trap.
          // Make sure to emit a block with the same type as us; leave updating
          // types for other passes.
          replaceCurrent(builder.makeBlock(
            {builder.makeDrop(curr->ref), builder.makeUnreachable()},
            curr->type));
          return;
        } else if (result == GCTypeUtils::SuccessOnlyIfNull) {
          // If either cast or ref types were non-nullable then the cast could
          // never succeed, and we'd have reached |Failure|, above.
          assert(curr->type.isNullable() && curr->ref->type.isNullable());

          // The cast either returns null, or traps. In trapsNeverHappen mode
          // we know the result, since it by assumption will not trap.
          if (getPassOptions().trapsNeverHappen) {
            replaceCurrent(builder.makeBlock(
              {builder.makeDrop(curr->ref), builder.makeRefNull(nullType)},
              curr->type));
            return;
          }

          // Without trapsNeverHappen we can at least sharpen the type here, if
          // it is not already a null type.
          auto newType = Type(nullType, Nullable);
          if (curr->type != newType) {
            curr->type = newType;
            // Call replaceCurrent() to make us re-optimize this node, as we
            // may have just unlocked further opportunities. (We could just
            // continue down to the rest, but we'd need to do more work to
            // make sure all the local state in this function is in sync
            // which this change; it's easier to just do another clean pass
            // on this node.)
            replaceCurrent(curr);
            return;
          }
        }

        auto** last = refp;
        refp = Properties::getImmediateFallthroughPtr(
          refp, getPassOptions(), *getModule());
        if (refp == last) {
          break;
        }
      }
    }

    // See what we know about the cast result.
    //
    // Note that we could look at the fallthrough for the ref, but that would
    // require additional work to make sure we emit something that validates
    // properly. TODO
    auto result = GCTypeUtils::evaluateCastCheck(curr->ref->type, curr->type);

    if (result == GCTypeUtils::Success) {
      replaceCurrent(curr->ref);

      // We must refinalize here, as we may be returning a more specific
      // type, which can alter the parent. For example:
      //
      //  (struct.get $parent 0
      //   (ref.cast_static $parent
      //    (local.get $child)
      //   )
      //  )
      //
      // Try to cast a $child to its parent, $parent. That always works,
      // so the cast can be removed.
      // Then once the cast is removed, the outer struct.get
      // will have a reference with a different type, making it a
      // (struct.get $child ..) instead of $parent.
      // But if $parent and $child have different types on field 0 (the
      // child may have a more refined one) then the struct.get must be
      // refinalized so the IR node has the expected type.
      refinalize = true;
      return;
    } else if (result == GCTypeUtils::SuccessOnlyIfNonNull) {
      // All we need to do is check for a null here.
      //
      // As above, we must refinalize as we may now be emitting a more refined
      // type (specifically a more refined heap type).
      replaceCurrent(builder.makeRefAs(RefAsNonNull, curr->ref));
      refinalize = true;
      return;
    }

    if (auto* child = curr->ref->dynCast<RefCast>()) {
      // Repeated casts can be removed, leaving just the most demanding of
      // them. Note that earlier we already checked for the cast of the ref's
      // type being more refined, so all we need to handle is the opposite, that
      // is, something like this:
      //
      //   (ref.cast $B
      //     (ref.cast $A
      //
      // where $B is a subtype of $A. We don't need to cast to $A here; we can
      // just cast all the way to $B immediately. To check this, see if the
      // parent's type would succeed if cast by the child's; if it must then the
      // child's is redundant.
      auto result = GCTypeUtils::evaluateCastCheck(curr->type, child->type);
      if (result == GCTypeUtils::Success) {
        curr->ref = child->ref;
        return;
      } else if (result == GCTypeUtils::SuccessOnlyIfNonNull) {
        // Similar to above, but we must also trap on null.
        curr->ref = child->ref;
        curr->type = Type(curr->type.getHeapType(), NonNullable);
        return;
      }
    }

    // ref.cast can be combined with ref.as_non_null,
    //
    //   (ref.cast null (ref.as_non_null ..))
    // =>
    //   (ref.cast ..)
    //
    if (auto* as = curr->ref->dynCast<RefAs>()) {
      if (as->op == RefAsNonNull) {
        curr->ref = as->value;
        curr->type = Type(curr->type.getHeapType(), NonNullable);
      }
    }
  }

  void visitRefTest(RefTest* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }

    Builder builder(*getModule());

    if (curr->ref->type.isNull()) {
      // The input is null, so we know whether this will succeed or fail.
      int32_t result = curr->castType.isNullable() ? 1 : 0;
      replaceCurrent(builder.makeBlock(
        {builder.makeDrop(curr->ref), builder.makeConst(int32_t(result))}));
      return;
    }

    // Parallel to the code in visitRefCast
    switch (GCTypeUtils::evaluateCastCheck(curr->ref->type, curr->castType)) {
      case GCTypeUtils::Unknown:
        break;
      case GCTypeUtils::Success:
        replaceCurrent(builder.makeBlock(
          {builder.makeDrop(curr->ref), builder.makeConst(int32_t(1))}));
        break;
      case GCTypeUtils::Failure:
        replaceCurrent(builder.makeSequence(builder.makeDrop(curr->ref),
                                            builder.makeConst(int32_t(0))));
        break;
      case GCTypeUtils::SuccessOnlyIfNull:
        replaceCurrent(builder.makeRefIsNull(curr->ref));
        break;
      case GCTypeUtils::SuccessOnlyIfNonNull:
        // This adds an EqZ, but code size does not regress since ref.test also
        // encodes a type, and ref.is_null does not. The EqZ may also add some
        // work, but a cast is likely more expensive than a null check + a fast
        // int operation.
        replaceCurrent(
          builder.makeUnary(EqZInt32, builder.makeRefIsNull(curr->ref)));
        break;
    }
  }

  void visitRefIsNull(RefIsNull* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }

    // Optimizing RefIsNull is not that obvious, since even if we know the
    // result evaluates to 0 or 1 then the replacement may not actually save
    // code size, since RefIsNull is a single byte while adding a Const of 0
    // would be two bytes. Other factors are that we can remove the input and
    // the added drop on it if it has no side effects, and that replacing with a
    // constant may allow further optimizations later. For now, replace with a
    // constant, but this warrants more investigation. TODO

    Builder builder(*getModule());
    if (curr->value->type.isNonNullable()) {
      replaceCurrent(
        builder.makeSequence(builder.makeDrop(curr->value),
                             builder.makeConst(Literal::makeZero(Type::i32))));
    } else {
      skipCast(curr->value);
    }
  }

  void visitRefAs(RefAs* curr) {
    if (curr->type == Type::unreachable) {
      return;
    }

    if (curr->op == ExternExternalize || curr->op == ExternInternalize) {
      // We can't optimize these. Even removing a non-null cast is not valid as
      // they allow nulls to filter through, unlike other RefAs*.
      return;
    }

    assert(curr->op == RefAsNonNull);
    skipNonNullCast(curr->value, curr);
    if (!curr->value->type.isNullable()) {
      replaceCurrent(curr->value);
      return;
    }

    // As we do in visitRefCast, ref.cast can be combined with ref.as_non_null.
    // This code handles the case where the ref.as is on the outside:
    //
    //   (ref.as_non_null (ref.cast null ..))
    // =>
    //   (ref.cast ..)
    //
    if (auto* cast = curr->value->dynCast<RefCast>()) {
      // The cast cannot be non-nullable, or we would have handled this right
      // above by just removing the ref.as, since it would not be needed.
      assert(!cast->type.isNonNullable());
      cast->type = Type(cast->type.getHeapType(), NonNullable);
      replaceCurrent(cast);
    }
  }

  Index getMaxBitsForLocal(LocalGet* get) {
    // check what we know about the local
    return localInfo[get->index].maxBits;
  }

private:
  // Information about our locals
  std::vector<LocalInfo> localInfo;

  // Check if two consecutive inputs to an instruction are equal. As they are
  // consecutive, no code can execeute in between them, which simplies the
  // problem here (and which is the case we care about in this pass, which does
  // simple peephole optimizations - all we care about is a single instruction
  // at a time, and its inputs).
  //
  // This also checks that the inputs are removable (but we do not assume the
  // caller will always remove them).
  bool areConsecutiveInputsEqualAndRemovable(Expression* left,
                                             Expression* right) {
    // First, check for side effects. If there are any, then we can't even
    // assume things like local.get's of the same index being identical. (It is
    // also ok to have removable side effects here, see the function
    // description.)
    auto& passOptions = getPassOptions();
    if (EffectAnalyzer(passOptions, *getModule(), left)
          .hasUnremovableSideEffects() ||
        EffectAnalyzer(passOptions, *getModule(), right)
          .hasUnremovableSideEffects()) {
      return false;
    }

    // Ignore extraneous things and compare them structurally.
    left = Properties::getFallthrough(left, passOptions, *getModule());
    right = Properties::getFallthrough(right, passOptions, *getModule());
    if (!ExpressionAnalyzer::equal(left, right)) {
      return false;
    }
    // To be equal, they must also be known to return the same result
    // deterministically.
    if (Properties::isGenerative(left, getModule()->features)) {
      return false;
    }
    return true;
  }

  // Check if two consecutive inputs to an instruction are equal and can also be
  // folded into the first of the two (but we do not assume the caller will
  // always fold them). This is similar to areConsecutiveInputsEqualAndRemovable
  // but also identifies reads from the same local variable when the first of
  // them is a "tee" operation and the second is a get (in which case, it is
  // fine to remove the get, but not the tee).
  //
  // The inputs here must be consecutive, but it is also ok to have code with no
  // side effects at all in the middle. For example, a Const in between is ok.
  bool areConsecutiveInputsEqualAndFoldable(Expression* left,
                                            Expression* right) {
    if (auto* set = left->dynCast<LocalSet>()) {
      if (auto* get = right->dynCast<LocalGet>()) {
        if (set->isTee() && get->index == set->index) {
          return true;
        }
      }
    }
    // stronger property than we need - we can not only fold
    // them but remove them entirely.
    return areConsecutiveInputsEqualAndRemovable(left, right);
  }

  // Similar to areConsecutiveInputsEqualAndFoldable, but only checks that they
  // are equal (and not that they are foldable).
  bool areConsecutiveInputsEqual(Expression* left, Expression* right) {
    // TODO: optimize cases that must be equal but are *not* foldable.
    return areConsecutiveInputsEqualAndFoldable(left, right);
  }

  // Canonicalizing the order of a symmetric binary helps us
  // write more concise pattern matching code elsewhere.
  void canonicalize(Binary* binary) {
    assert(shouldCanonicalize(binary));
    auto swap = [&]() {
      assert(canReorder(binary->left, binary->right));
      if (binary->isRelational()) {
        binary->op = reverseRelationalOp(binary->op);
      }
      std::swap(binary->left, binary->right);
    };
    auto maybeSwap = [&]() {
      if (canReorder(binary->left, binary->right)) {
        swap();
      }
    };
    // Prefer a const on the right.
    if (binary->left->is<Const>() && !binary->right->is<Const>()) {
      swap();
    }
    if (auto* c = binary->right->dynCast<Const>()) {
      // x - C   ==>   x + (-C)
      // Prefer use addition if there is a constant on the right.
      if (binary->op == Abstract::getBinary(c->type, Abstract::Sub)) {
        c->value = c->value.neg();
        binary->op = Abstract::getBinary(c->type, Abstract::Add);
        return;
      }
      // Prefer to compare to 0 instead of to -1 or 1.
      // (signed)x > -1   ==>   x >= 0
      if (binary->op == Abstract::getBinary(c->type, Abstract::GtS) &&
          c->value.getInteger() == -1LL) {
        binary->op = Abstract::getBinary(c->type, Abstract::GeS);
        c->value = Literal::makeZero(c->type);
        return;
      }
      // (signed)x <= -1   ==>   x < 0
      if (binary->op == Abstract::getBinary(c->type, Abstract::LeS) &&
          c->value.getInteger() == -1LL) {
        binary->op = Abstract::getBinary(c->type, Abstract::LtS);
        c->value = Literal::makeZero(c->type);
        return;
      }
      // (signed)x < 1   ==>   x <= 0
      if (binary->op == Abstract::getBinary(c->type, Abstract::LtS) &&
          c->value.getInteger() == 1LL) {
        binary->op = Abstract::getBinary(c->type, Abstract::LeS);
        c->value = Literal::makeZero(c->type);
        return;
      }
      // (signed)x >= 1   ==>   x > 0
      if (binary->op == Abstract::getBinary(c->type, Abstract::GeS) &&
          c->value.getInteger() == 1LL) {
        binary->op = Abstract::getBinary(c->type, Abstract::GtS);
        c->value = Literal::makeZero(c->type);
        return;
      }
      // (unsigned)x < 1   ==>   x == 0
      if (binary->op == Abstract::getBinary(c->type, Abstract::LtU) &&
          c->value.getInteger() == 1LL) {
        binary->op = Abstract::getBinary(c->type, Abstract::Eq);
        c->value = Literal::makeZero(c->type);
        return;
      }
      // (unsigned)x >= 1   ==>   x != 0
      if (binary->op == Abstract::getBinary(c->type, Abstract::GeU) &&
          c->value.getInteger() == 1LL) {
        binary->op = Abstract::getBinary(c->type, Abstract::Ne);
        c->value = Literal::makeZero(c->type);
        return;
      }
      // Prefer compare to signed min (s_min) instead of s_min + 1.
      // (signed)x < s_min + 1   ==>   x == s_min
      if (binary->op == LtSInt32 && c->value.geti32() == INT32_MIN + 1) {
        binary->op = EqInt32;
        c->value = Literal::makeSignedMin(Type::i32);
        return;
      }
      if (binary->op == LtSInt64 && c->value.geti64() == INT64_MIN + 1) {
        binary->op = EqInt64;
        c->value = Literal::makeSignedMin(Type::i64);
        return;
      }
      // (signed)x >= s_min + 1   ==>   x != s_min
      if (binary->op == GeSInt32 && c->value.geti32() == INT32_MIN + 1) {
        binary->op = NeInt32;
        c->value = Literal::makeSignedMin(Type::i32);
        return;
      }
      if (binary->op == GeSInt64 && c->value.geti64() == INT64_MIN + 1) {
        binary->op = NeInt64;
        c->value = Literal::makeSignedMin(Type::i64);
        return;
      }
      // Prefer compare to signed max (s_max) instead of s_max - 1.
      // (signed)x > s_max - 1   ==>   x == s_max
      if (binary->op == GtSInt32 && c->value.geti32() == INT32_MAX - 1) {
        binary->op = EqInt32;
        c->value = Literal::makeSignedMax(Type::i32);
        return;
      }
      if (binary->op == GtSInt64 && c->value.geti64() == INT64_MAX - 1) {
        binary->op = EqInt64;
        c->value = Literal::makeSignedMax(Type::i64);
        return;
      }
      // (signed)x <= s_max - 1   ==>   x != s_max
      if (binary->op == LeSInt32 && c->value.geti32() == INT32_MAX - 1) {
        binary->op = NeInt32;
        c->value = Literal::makeSignedMax(Type::i32);
        return;
      }
      if (binary->op == LeSInt64 && c->value.geti64() == INT64_MAX - 1) {
        binary->op = NeInt64;
        c->value = Literal::makeSignedMax(Type::i64);
        return;
      }
      // Prefer compare to unsigned max (u_max) instead of u_max - 1.
      // (unsigned)x <= u_max - 1   ==>   x != u_max
      if (binary->op == Abstract::getBinary(c->type, Abstract::LeU) &&
          c->value.getInteger() == (int64_t)(UINT64_MAX - 1)) {
        binary->op = Abstract::getBinary(c->type, Abstract::Ne);
        c->value = Literal::makeUnsignedMax(c->type);
        return;
      }
      // (unsigned)x > u_max - 1   ==>   x == u_max
      if (binary->op == Abstract::getBinary(c->type, Abstract::GtU) &&
          c->value.getInteger() == (int64_t)(UINT64_MAX - 1)) {
        binary->op = Abstract::getBinary(c->type, Abstract::Eq);
        c->value = Literal::makeUnsignedMax(c->type);
        return;
      }
      return;
    }
    // Prefer a get on the right.
    if (binary->left->is<LocalGet>() && !binary->right->is<LocalGet>()) {
      return maybeSwap();
    }
    // Sort by the node id type, if different.
    if (binary->left->_id != binary->right->_id) {
      if (binary->left->_id > binary->right->_id) {
        return maybeSwap();
      }
      return;
    }
    // If the children have the same node id, we have to go deeper.
    if (auto* left = binary->left->dynCast<Unary>()) {
      auto* right = binary->right->cast<Unary>();
      if (left->op > right->op) {
        return maybeSwap();
      }
    }
    if (auto* left = binary->left->dynCast<Binary>()) {
      auto* right = binary->right->cast<Binary>();
      if (left->op > right->op) {
        return maybeSwap();
      }
    }
    if (auto* left = binary->left->dynCast<LocalGet>()) {
      auto* right = binary->right->cast<LocalGet>();
      if (left->index > right->index) {
        return maybeSwap();
      }
    }
  }

  // Optimize given that the expression is flowing into a boolean context
  Expression* optimizeBoolean(Expression* boolean) {
    // TODO use a general getFallthroughs
    if (auto* unary = boolean->dynCast<Unary>()) {
      if (unary) {
        if (unary->op == EqZInt32) {
          auto* unary2 = unary->value->dynCast<Unary>();
          if (unary2 && unary2->op == EqZInt32) {
            // double eqz
            return unary2->value;
          }
          if (auto* binary = unary->value->dynCast<Binary>()) {
            // !(x <=> y)   ==>   x <!=> y
            auto op = invertBinaryOp(binary->op);
            if (op != InvalidBinary) {
              binary->op = op;
              return binary;
            }
          }
        }
      }
    } else if (auto* binary = boolean->dynCast<Binary>()) {
      if (binary->op == SubInt32) {
        if (auto* c = binary->left->dynCast<Const>()) {
          if (c->value.geti32() == 0) {
            // bool(0 - x)   ==>   bool(x)
            return binary->right;
          }
        }
      } else if (binary->op == OrInt32) {
        // an or flowing into a boolean context can consider each input as
        // boolean
        binary->left = optimizeBoolean(binary->left);
        binary->right = optimizeBoolean(binary->right);
      } else if (binary->op == NeInt32) {
        if (auto* c = binary->right->dynCast<Const>()) {
          // x != 0 is just x if it's used as a bool
          if (c->value.geti32() == 0) {
            return binary->left;
          }
          // TODO: Perhaps use it for separate final pass???
          // x != -1   ==>    x ^ -1
          // if (num->value.geti32() == -1) {
          //   binary->op = XorInt32;
          //   return binary;
          // }
        }
      } else if (binary->op == RemSInt32) {
        // bool(i32(x) % C_pot)  ==>  bool(x & (C_pot - 1))
        // bool(i32(x) % min_s)  ==>  bool(x & max_s)
        if (auto* c = binary->right->dynCast<Const>()) {
          if (c->value.isSignedMin() ||
              Bits::isPowerOf2(c->value.abs().geti32())) {
            binary->op = AndInt32;
            if (c->value.isSignedMin()) {
              c->value = Literal::makeSignedMax(Type::i32);
            } else {
              c->value = c->value.abs().sub(Literal::makeOne(Type::i32));
            }
            return binary;
          }
        }
      }
      if (auto* ext = Properties::getSignExtValue(binary)) {
        // use a cheaper zero-extent, we just care about the boolean value
        // anyhow
        return makeZeroExt(ext, Properties::getSignExtBits(binary));
      }
    } else if (auto* block = boolean->dynCast<Block>()) {
      if (block->type == Type::i32 && block->list.size() > 0) {
        block->list.back() = optimizeBoolean(block->list.back());
      }
    } else if (auto* iff = boolean->dynCast<If>()) {
      if (iff->type == Type::i32) {
        iff->ifTrue = optimizeBoolean(iff->ifTrue);
        iff->ifFalse = optimizeBoolean(iff->ifFalse);
      }
    } else if (auto* select = boolean->dynCast<Select>()) {
      select->ifTrue = optimizeBoolean(select->ifTrue);
      select->ifFalse = optimizeBoolean(select->ifFalse);
    } else if (auto* tryy = boolean->dynCast<Try>()) {
      if (tryy->type == Type::i32) {
        tryy->body = optimizeBoolean(tryy->body);
        for (Index i = 0; i < tryy->catchBodies.size(); i++) {
          tryy->catchBodies[i] = optimizeBoolean(tryy->catchBodies[i]);
        }
      }
    }
    // TODO: recurse into br values?
    return boolean;
  }

  Expression* optimizeSelect(Select* curr) {
    using namespace Match;
    using namespace Abstract;
    Builder builder(*getModule());
    curr->condition = optimizeBoolean(curr->condition);
    {
      // Constant condition, we can just pick the correct side (barring side
      // effects)
      Expression *ifTrue, *ifFalse;
      if (matches(curr, select(pure(&ifTrue), any(&ifFalse), i32(0)))) {
        return ifFalse;
      }
      if (matches(curr, select(any(&ifTrue), any(&ifFalse), i32(0)))) {
        return builder.makeSequence(builder.makeDrop(ifTrue), ifFalse);
      }
      int32_t cond;
      if (matches(curr, select(any(&ifTrue), pure(&ifFalse), i32(&cond)))) {
        // The condition must be non-zero because a zero would have matched one
        // of the previous patterns.
        assert(cond != 0);
        return ifTrue;
      }
      // Don't bother when `ifFalse` isn't pure - we would need to reverse the
      // order using a temp local, which would be bad
    }
    {
      // TODO: Remove this after landing SCCP pass. See: #4161

      // i32(x) ? i32(x) : 0  ==>  x
      Expression *x, *y;
      if (matches(curr, select(any(&x), i32(0), any(&y))) &&
          areConsecutiveInputsEqualAndFoldable(x, y)) {
        return curr->ifTrue;
      }
      // i32(x) ? 0 : i32(x)  ==>  { x, 0 }
      if (matches(curr, select(i32(0), any(&x), any(&y))) &&
          areConsecutiveInputsEqualAndFoldable(x, y)) {
        return builder.makeSequence(builder.makeDrop(x), curr->ifTrue);
      }

      // i64(x) == 0 ? 0 : i64(x)  ==>  x
      // i64(x) != 0 ? i64(x) : 0  ==>  x
      if ((matches(curr, select(i64(0), any(&x), unary(EqZInt64, any(&y)))) ||
           matches(
             curr,
             select(any(&x), i64(0), binary(NeInt64, any(&y), i64(0))))) &&
          areConsecutiveInputsEqualAndFoldable(x, y)) {
        return curr->condition->is<Unary>() ? curr->ifFalse : curr->ifTrue;
      }

      // i64(x) == 0 ? i64(x) : 0  ==>  { x, 0 }
      // i64(x) != 0 ? 0 : i64(x)  ==>  { x, 0 }
      if ((matches(curr, select(any(&x), i64(0), unary(EqZInt64, any(&y)))) ||
           matches(
             curr,
             select(i64(0), any(&x), binary(NeInt64, any(&y), i64(0))))) &&
          areConsecutiveInputsEqualAndFoldable(x, y)) {
        return builder.makeSequence(
          builder.makeDrop(x),
          curr->condition->is<Unary>() ? curr->ifFalse : curr->ifTrue);
      }
    }
    {
      // Simplify selects between 0 and 1
      Expression* c;
      bool reversed = matches(curr, select(ival(0), ival(1), any(&c)));
      if (reversed || matches(curr, select(ival(1), ival(0), any(&c)))) {
        if (reversed) {
          c = optimizeBoolean(builder.makeUnary(EqZInt32, c));
        }
        if (!Properties::emitsBoolean(c)) {
          // cond ? 1 : 0 ==> !!cond
          c = builder.makeUnary(EqZInt32, builder.makeUnary(EqZInt32, c));
        }
        return curr->type == Type::i64 ? builder.makeUnary(ExtendUInt32, c) : c;
      }
    }
    // Flip the arms if doing so might help later optimizations here.
    if (auto* binary = curr->condition->dynCast<Binary>()) {
      auto inv = invertBinaryOp(binary->op);
      if (inv != InvalidBinary) {
        // For invertible binary operations, we prefer to have non-zero values
        // in the ifTrue, and zero values in the ifFalse, due to the
        // optimization right after us. Even if this does not help there, it is
        // a nice canonicalization. (To ensure convergence - that we don't keep
        // doing work each time we get here - do nothing if both are zero, or
        // if both are nonzero.)
        Const* c;
        if ((matches(curr->ifTrue, ival(0)) &&
             !matches(curr->ifFalse, ival(0))) ||
            (!matches(curr->ifTrue, ival()) &&
             matches(curr->ifFalse, ival(&c)) && !c->value.isZero())) {
          binary->op = inv;
          std::swap(curr->ifTrue, curr->ifFalse);
        }
      }
    }
    if (curr->type == Type::i32 &&
        Bits::getMaxBits(curr->condition, this) <= 1 &&
        Bits::getMaxBits(curr->ifTrue, this) <= 1 &&
        Bits::getMaxBits(curr->ifFalse, this) <= 1) {
      // The condition and both arms are i32 booleans, which allows us to do
      // boolean optimizations.
      Expression* x;
      Expression* y;

      // x ? y : 0   ==>   x & y
      if (matches(curr, select(any(&y), ival(0), any(&x)))) {
        return builder.makeBinary(AndInt32, y, x);
      }

      // x ? 1 : y   ==>   x | y
      if (matches(curr, select(ival(1), any(&y), any(&x)))) {
        return builder.makeBinary(OrInt32, y, x);
      }
    }
    {
      // Simplify x < 0 ? -1 : 1 or x >= 0 ? 1 : -1 to
      // i32(x) >> 31 | 1
      // i64(x) >> 63 | 1
      Binary* bin;
      if (matches(
            curr,
            select(ival(-1), ival(1), binary(&bin, LtS, any(), ival(0)))) ||
          matches(
            curr,
            select(ival(1), ival(-1), binary(&bin, GeS, any(), ival(0))))) {
        auto c = bin->right->cast<Const>();
        auto type = curr->ifTrue->type;
        if (type == c->type) {
          bin->type = type;
          bin->op = Abstract::getBinary(type, ShrS);
          c->value = Literal::makeFromInt32(type.getByteSize() * 8 - 1, type);
          curr->ifTrue->cast<Const>()->value = Literal::makeOne(type);
          return builder.makeBinary(
            Abstract::getBinary(type, Or), bin, curr->ifTrue);
        }
      }
    }
    {
      // Flip select to remove eqz if we can reorder
      Select* s;
      Expression *ifTrue, *ifFalse, *c;
      if (matches(
            curr,
            select(
              &s, any(&ifTrue), any(&ifFalse), unary(EqZInt32, any(&c)))) &&
          canReorder(ifTrue, ifFalse)) {
        s->ifTrue = ifFalse;
        s->ifFalse = ifTrue;
        s->condition = c;
        return s;
      }
    }
    {
      // Sides are identical, fold
      Expression *ifTrue, *ifFalse, *c;
      if (matches(curr, select(any(&ifTrue), any(&ifFalse), any(&c))) &&
          ExpressionAnalyzer::equal(ifTrue, ifFalse)) {
        auto value = effects(ifTrue);
        if (value.hasSideEff