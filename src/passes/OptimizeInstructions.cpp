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

  void vis