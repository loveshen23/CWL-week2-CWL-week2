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
// Simple WebAssembly interpreter. This operates directly on the AST,
// for simplicity and clarity. A goal is for it to be possible for
// people to read this code and understand WebAssembly semantics.
//

#ifndef wasm_wasm_interpreter_h
#define wasm_wasm_interpreter_h

#include <cmath>
#include <limits.h>
#include <sstream>
#include <variant>

#include "ir/intrinsics.h"
#include "ir/module-utils.h"
#include "support/bits.h"
#include "support/safe_integer.h"
#include "wasm-builder.h"
#include "wasm-traversal.h"
#include "wasm.h"

namespace wasm {

struct WasmException {
  Name tag;
  Literals values;
};
std::ostream& operator<<(std::ostream& o, const WasmException& exn);

// Utilities

extern Name WASM, RETURN_FLOW, NONCONSTANT_FLOW;

// Stuff that flows around during executing expressions: a literal, or a change
// in control flow.
class Flow {
public:
  Flow() : values() {}
  Flow(Literal value) : values{value} { assert(value.type.isConcrete()); }
  Flow(Literals& values) : values(values) {}
  Flow(Literals&& values) : values(std::move(values)) {}
  Flow(Name breakTo) : values(), breakTo(breakTo) {}
  Flow(Name breakTo, Literal value) : values{value}, breakTo(breakTo) {}

  Literals values;
  Name breakTo; // if non-null, a break is going on

  // A helper function for the common case where there is only one value
  const Literal& getSingleValue() {
    assert(values.size() == 1);
    return values[0];
  }

  Type getType() { return values.getType(); }

  Expression* getConstExpression(Module& module) {
    assert(values.size() > 0);
    Builder builder(module);
    return builder.makeConstantExpression(values);
  }

  bool breaking() { return breakTo.is(); }

  void clearIf(Name target) {
    if (breakTo == target) {
      breakTo = Name{};
    }
  }

  friend std::ostream& operator<<(std::ostream& o, const Flow& flow) {
    o << "(flow " << (flow.breakTo.is() ? flow.breakTo.str : "-") << " : {";
    for (size_t i = 0; i < flow.values.size(); ++i) {
      if (i > 0) {
        o << ", ";
      }
      o << flow.values[i];
    }
    o << "})";
    return o;
  }
};

// Debugging helpers
#ifdef WASM_INTERPRETER_DEBUG
class Indenter {
  static int indentLevel;

  const char* entryName;

public:
  Indenter(const char* entry);
  ~Indenter();

  static void print();
};

#define NOTE_ENTER(x)                                                          \
  Indenter _int_blah(x);                                                       \
  {                                                                            \
    Indenter::print();                                                         \
    std::cout << "visit " << x << " : " << curr << "\n";                       \
  }
#define NOTE_ENTER_(x)                                                         \
  Indenter _int_blah(x);                                                       \
  {                                                                            \
    Indenter::print();                                                         \
    std::cout << "visit " << x << "\n";                                        \
  }
#define NOTE_NAME(p0)                                                          \
  {                                                                            \
    Indenter::print();                                                         \
    std::cout << "name " << '(' << Name(p0) << ")\n";                          \
  }
#define NOTE_EVAL1(p0)                                                         \
  {                                                                            \
    Indenter::print();                                                         \
    std::cout << "eval " #p0 " (" << p0 << ")\n";                              \
  }
#define NOTE_EVAL2(p0, p1)                                                     \
  {                                                                            \
    Indenter::print();                                                         \
    std::cout << "eval " #p0 " (" << p0 << "), " #p1 " (" << p1 << ")\n";      \
  }
#else // WASM_INTERPRETER_DEBUG
#define NOTE_ENTER(x)
#define NOTE_ENTER_(x)
#define NOTE_NAME(p0)
#define NOTE_EVAL1(p0)
#define NOTE_EVAL2(p0, p1)
#endif // WASM_INTERPRETER_DEBUG

// Execute an expression
template<typename SubType>
class ExpressionRunner : public OverriddenVisitor<SubType, Flow> {
  SubType* self() { return static_cast<SubType*>(this); }

protected:
  // Optional module context to search for globals and called functions. NULL if
  // we are not interested in any context.
  Module* module = nullptr;

  // Maximum depth before giving up.
  Index maxDepth;
  Index depth = 0;

  // Maximum iterations before giving up on a loop.
  Index maxLoopIterations;

  Flow generateArguments(const ExpressionList& operands, Literals& arguments) {
    NOTE_ENTER_("generateArguments");
    arguments.reserve(operands.size());
    for (auto expression : operands) {
      Flow flow = self()->visit(expression);
      if (flow.breaking()) {
        return flow;
      }
      NOTE_EVAL1(flow.values);
      arguments.push_back(flow.getSingleValue());
    }
    return Flow();
  }

public:
  // Indicates no limit of maxDepth or maxLoopIterations.
  static const Index NO_LIMIT = 0;

  ExpressionRunner(Module* module = nullptr,
                   Index maxDepth = NO_LIMIT,
                   Index maxLoopIterations = NO_LIMIT)
    : module(module), maxDepth(maxDepth), maxLoopIterations(maxLoopIterations) {
  }
  virtual ~ExpressionRunner() = default;

  Flow visit(Expression* curr) {
    depth++;
    if (maxDepth != NO_LIMIT && depth > maxDepth) {
      hostLimit("interpreter recursion limit");
    }
    auto ret = OverriddenVisitor<SubType, Flow>::visit(curr);
    if (!ret.breaking()) {
      Type type = ret.getType();
      if (type.isConcrete() || curr->type.isConcrete()) {
#if 1 // def WASM_INTERPRETER_DEBUG
        if (!Type::isSubType(type, curr->type)) {
          std::cerr << "expected " << curr->type << ", seeing " << type
                    << " from\n"
                    << *curr << '\n';
        }
#endif
        assert(Type::isSubType(type, curr->type));
      }
    }
    depth--;
    return ret;
  }

  // Gets the module this runner is operating on.
  Module* getModule() { return module; }

  Flow visitBlock(Block* curr) {
    NOTE_ENTER("Block");
    // special-case Block, because Block nesting (in their first element) can be
    // incredibly deep
    std::vector<Block*> stack;
    stack.push_back(curr);
    while (curr->list.size() > 0 && curr->list[0]->is<Block>()) {
      curr = curr->list[0]->cast<Block>();
      stack.push_back(curr);
    }
    Flow flow;
    auto* top = stack.back();
    while (stack.size() > 0) {
      curr = stack.back();
      stack.pop_back();
      if (flow.breaking()) {
        flow.clearIf(curr->name);
        continue;
      }
      auto& list = curr->list;
      for (size_t i = 0; i < list.size(); i++) {
        if (curr != top && i == 0) {
          // one of the block recursions we already handled
          continue;
        }
        flow = visit(list[i]);
        if (flow.breaking()) {
          flow.clearIf(curr->name);
          break;
        }
      }
    }
    return flow;
  }
  Flow visitIf(If* curr) {
    NOTE_ENTER("If");
    Flow flow = visit(curr->condition);
    if (flow.breaking()) {
      return flow;
    }
    NOTE_EVAL1(flow.values);
    if (flow.getSingleValue().geti32()) {
      Flow flow = visit(curr->ifTrue);
      if (!flow.breaking() && !curr->ifFalse) {
        flow = Flow(); // if_else returns a value, but if does not
      }
      return flow;
    }
    if (curr->ifFalse) {
      return visit(curr->ifFalse);
    }
    return Flow();
  }
  Flow visitLoop(Loop* curr) {
    NOTE_ENTER("Loop");
    Index loopCount = 0;
    while (1) {
      Flow flow = visit(curr->body);
      if (flow.breaking()) {
        if (flow.breakTo == curr->name) {
          if (maxLoopIterations != NO_LIMIT &&
              ++loopCount >= maxLoopIterations) {
            return Flow(NONCONSTANT_FLOW);
          }
          continue; // lol
        }
      }
      // loop does not loop automatically, only continue achieves that
      return flow;
    }
  }
  Flow visitBreak(Break* curr) {
    NOTE_ENTER("Break");
    bool condition = true;
    Flow flow;
    if (curr->value) {
      flow = visit(curr->value);
      if (flow.breaking()) {
        return flow;
      }
    }
    if (curr->condition) {
      Flow conditionFlow = visit(curr->condition);
      if (conditionFlow.breaking()) {
        return conditionFlow;
      }
      condition = conditionFlow.getSingleValue().getInteger() != 0;
      if (!condition) {
        return flow;
      }
    }
    flow.breakTo = curr->name;
    return flow;
  }
  Flow visitSwitch(Switch* curr) {
    NOTE_ENTER("Switch");
    Flow flow;
    Literals values;
    if (curr->value) {
      flow = visit(curr->value);
      if (flow.breaking()) {
        return flow;
      }
      values = flow.values;
    }
    flow = visit(curr->condition);
    if (flow.breaking()) {
      return flow;
    }
    int64_t index = flow.getSingleValue().getInteger();
    Name target = curr->default_;
    if (index >= 0 && (size_t)index < curr->targets.size()) {
      target = curr->targets[(size_t)index];
    }
    flow.breakTo = target;
    flow.values = values;
    return flow;
  }

  Flow visitConst(Const* curr) {
    NOTE_ENTER("Const");
    NOTE_EVAL1(curr->value);
    return Flow(curr->value); // heh
  }

  // Unary and Binary nodes, the core math computations. We mostly just
  // delegate to the Literal::* methods, except we handle traps here.

  Flow visitUnary(Unary* curr) {
    NOTE_ENTER("Unary");
    Flow flow = visit(curr->value);
    if (flow.breaking()) {
      return flow;
    }
    Literal value = flow.getSingleValue();
    NOTE_EVAL1(value);
    switch (curr->op) {
      case ClzInt32:
      case ClzInt64:
        return value.countLeadingZeroes();
      case CtzInt32:
      case CtzInt64:
        return value.countTrailingZeroes();
      case PopcntInt32:
      case PopcntInt64:
        return value.popCount();
      case EqZInt32:
      case EqZInt64:
        return value.eqz();
      case ReinterpretInt32:
        return value.castToF32();
      case ReinterpretInt64:
        return value.castToF64();
      case ExtendSInt32:
        return value.extendToSI64();
      case ExtendUInt32:
        return value.extendToUI64();
      case WrapInt64:
        return value.wrapToI32();
      case ConvertUInt32ToFloat32:
      case ConvertUInt64ToFloat32:
        return value.convertUIToF32();
      case ConvertUInt32ToFloat64:
      case ConvertUInt64ToFloat64:
        return value.convertUIToF64();
      case ConvertSInt32ToFloat32:
      case ConvertSInt64ToFloat32:
        return value.convertSIToF32();
      case ConvertSInt32ToFloat64:
      case ConvertSInt64ToFloat64:
        return value.convertSIToF64();
      case ExtendS8Int32:
      case ExtendS8Int64:
        return value.extendS8();
      case ExtendS16Int32:
      case ExtendS16Int64:
        return value.extendS16();
      case ExtendS32Int64:
        return value.extendS32();
      case NegFloat32:
      case NegFloat64:
        return value.neg();
      case AbsFloat32:
      case AbsFloat64:
        return value.abs();
      case CeilFloat32:
      case CeilFloat64:
        return value.ceil();
      case FloorFloat32:
      case FloorFloat64:
        return value.floor();
      case TruncFloat32:
      case TruncFloat64:
        return value.trunc();
      case NearestFloat32:
      case NearestFloat64:
        return value.nearbyint();
      case SqrtFloat32:
      case SqrtFloat64:
        return value.sqrt();
      case TruncSFloat32ToInt32:
      case TruncSFloat64ToInt32:
      case TruncSFloat32ToInt64:
      case TruncSFloat64ToInt64:
        return truncSFloat(curr, value);
      case TruncUFloat32ToInt32:
      case TruncUFloat64ToInt32:
      case TruncUFloat32ToInt64:
      case TruncUFloat64ToInt64:
        return truncUFloat(curr, value);
      case TruncSatSFloat32ToInt32:
      case TruncSatSFloat64ToInt32:
        return value.truncSatToSI32();
      case TruncSatSFloat32ToInt64:
      case TruncSatSFloat64ToInt64:
        return value.truncSatToSI64();
      case TruncSatUFloat32ToInt32:
      case TruncSatUFloat64ToInt32:
        return value.truncSatToUI32();
      case TruncSatUFloat32ToInt64:
      case TruncSatUFloat64ToInt64:
        return value.truncSatToUI64();
      case ReinterpretFloat32:
        return value.castToI32();
      case PromoteFloat32:
        return value.extendToF64();
      case ReinterpretFloat64:
        return value.castToI64();
      case DemoteFloat64:
        return value.demote();
      case SplatVecI8x16:
        return value.splatI8x16();
      case SplatVecI16x8:
        return value.splatI16x8();
      case SplatVecI32x4:
        return value.splatI32x4();
      case SplatVecI64x2:
        return value.splatI64x2();
      case SplatVecF32x4:
        return value.splatF32x4();
      case SplatVecF64x2:
        return value.splatF64x2();
      case NotVec128:
        return value.notV128();
      case AnyTrueVec128:
        return value.anyTrueV128();
      case AbsVecI8x16:
        return value.absI8x16();
      case NegVecI8x16:
        return value.negI8x16();
      case AllTrueVecI8x16:
        return value.allTrueI8x16();
      case BitmaskVecI8x16:
        return value.bitmaskI8x16();
      case PopcntVecI8x16:
        return value.popcntI8x16();
      case AbsVecI16x8:
        return value.absI16x8();
      case NegVecI16x8:
        return value.negI16x8();
      case AllTrueVecI16x8:
        return value.allTrueI16x8();
      case BitmaskVecI16x8:
        return value.bitmaskI16x8();
      case AbsVecI32x4:
        return value.absI32x4();
      case NegVecI32x4:
        return value.negI32x4();
      case AllTrueVecI32x4:
        return value.allTrueI32x4();
      case BitmaskVecI32x4:
        return value.bitmaskI32x4();
      case AbsVecI64x2:
        return value.absI64x2();
      case NegVecI64x2:
        return value.negI64x2();
      case AllTrueVecI64x2:
        return value.allTrueI64x2();
      case BitmaskVecI64x2:
        return value.bitmaskI64x2();
      case AbsVecF32x4:
        return value.absF32x4();
      case NegVecF32x4:
        return value.negF32x4();
      case SqrtVecF32x4:
        return value.sqrtF32x4();
      case CeilVecF32x4:
        return value.ceilF32x4();
      case FloorVecF32x4:
        return value.floorF32x4();
      case TruncVecF32x4:
        return value.truncF32x4();
      case NearestVecF32x4:
        return value.nearestF32x4();
      case AbsVecF64x2:
        return value.absF64x2();
      case NegVecF64x2:
        return value.negF64x2();
      case SqrtVecF64x2:
        return value.sqrtF64x2();
      case CeilVecF64x2:
        return value.ceilF64x2();
      case FloorVecF64x2:
        return value.floorF64x2();
      case TruncVecF64x2:
        return value.truncF64x2();
      case NearestVecF64x2:
        return value.nearestF64x2();
      case ExtAddPairwiseSVecI8x16ToI16x8:
        return value.extAddPairwiseToSI16x8();
      case ExtAddPairwiseUVecI8x16ToI16x8:
        return value.extAddPairwiseToUI16x8();
      case ExtAddPairwiseSVecI16x8ToI32x4:
        return value.extAddPairwiseToSI32x4();
      case ExtAddPairwiseUVecI16x8ToI32x4:
        return value.extAddPairwiseToUI32x4();
      case TruncSatSVecF32x4ToVecI32x4:
      case RelaxedTruncSVecF32x4ToVecI32x4:
        return value.truncSatToSI32x4();
      case TruncSatUVecF32x4ToVecI32x4:
      case RelaxedTruncUVecF32x4ToVecI32x4:
        return value.truncSatToUI32x4();
      case ConvertSVecI32x4ToVecF32x4:
        return value.convertSToF32x4();
      case ConvertUVecI32x4ToVecF32x4:
        return value.convertUToF32x4();
      case ExtendLowSVecI8x16ToVecI16x8:
        return value.extendLowSToI16x8();
      case ExtendHighSVecI8x16ToVecI16x8:
        return value.extendHighSToI16x8();
      case ExtendLowUVecI8x16ToVecI16x8:
        return value.extendLowUToI16x8();
      case ExtendHighUVecI8x16ToVecI16x8:
        return value.extendHighUToI16x8();
      case ExtendLowSVecI16x8ToVecI32x4:
        return value.extendLowSToI32x4();
      case ExtendHighSVecI16x8ToVecI32x4:
        return value.extendHighSToI32x4();
      case ExtendLowUVecI16x8ToVecI32x4:
        return value.extendLowUToI32x4();
      case ExtendHighUVecI16x8ToVecI32x4:
        return value.extendHighUToI32x4();
      case ExtendLowSVecI32x4ToVecI64x2:
        return value.extendLowSToI64x2();
      case ExtendHighSVecI32x4ToVecI64x2:
        return value.extendHighSToI64x2();
      case ExtendLowUVecI32x4ToVecI64x2:
        return value.extendLowUToI64x2();
      case ExtendHighUVecI32x4ToVecI64x2:
        return value.extendHighUToI64x2();
      case ConvertLowSVecI32x4ToVecF64x2:
        return value.convertLowSToF64x2();
      case ConvertLowUVecI32x4ToVecF64x2:
        return value.convertLowUToF64x2();
      case TruncSatZeroSVecF64x2ToVecI32x4:
      case RelaxedTruncZeroSVecF64x2ToVecI32x4:
        return value.truncSatZeroSToI32x4();
      case TruncSatZeroUVecF64x2ToVecI32x4:
      case RelaxedTruncZeroUVecF64x2ToVecI32x4:
        return value.truncSatZeroUToI32x4();
      case DemoteZeroVecF64x2ToVecF32x4:
        return value.demoteZeroToF32x4();
      case PromoteLowVecF32x4ToVecF64x2:
        return value.promoteLowToF64x2();
      case InvalidUnary:
        WASM_UNREACHABLE("invalid unary op");
    }
    WASM_UNREACHABLE("invalid op");
  }
  Flow visitBinary(Binary* curr) {
    NOTE_ENTER("Binary");
    Flow flow = visit(curr->left);
    if (flow.breaking()) {
      return flow;
    }
    Literal left = flow.getSingleValue();
    flow = visit(curr->right);
    if (flow.breaking()) {
      return flow;
    }
    Literal right = flow.getSingleValue();
    NOTE_EVAL2(left, right);
    assert(curr->left->type.isConcrete() ? left.type == curr->left->type
                                         : true);
    assert(curr->right->type.isConcrete() ? right.type == curr->right->type
                                          : true);
    switch (curr->op) {
      case AddInt32:
      case AddInt64:
      case AddFloat32:
      case AddFloat64:
        return left.add(right);
      case SubInt32:
      case SubInt64:
      case SubFloat32:
      case SubFloat64:
        return left.sub(right);
      case MulInt32:
      case MulInt64:
      case MulFloat32:
      case MulFloat64:
        return left.mul(right);
      case DivSInt32: {
        if (right.getInteger() == 0) {
          trap("i32.div_s by 0");
        }
        if (left.getInteger() == std::numeric_limits<int32_t>::min() &&
            right.getInteger() == -1) {
          trap("i32.div_s overflow"); // signed division overflow
        }
        return left.divS(right);
      }
      case DivUInt32: {
        if (right.getInteger() == 0) {
          trap("i32.div_u by 0");
        }
        return left.divU(right);
      }
      case RemSInt32: {
        if (right.getInteger() == 0) {
          trap("i32.rem_s by 0");
        }
        if (left.getInteger() == std::numeric_limits<int32_t>::min() &&
            right.getInteger() == -1) {
          return Literal(int32_t(0));
        }
        return left.remS(right);
      }
      case RemUInt32: {
        if (right.getInteger() == 0) {
          trap("i32.rem_u by 0");
        }
        return left.remU(right);
      }
      case DivSInt64: {
        if (right.getInteger() == 0) {
          trap("i64.div_s by 0");
        }
        if (left.getInteger() == LLONG_MIN && right.getInteger() == -1LL) {
          trap("i64.div_s overflow"); // signed division overflow
        }
        return left.divS(right);
      }
      case DivUInt64: {
        if (right.getInteger() == 0) {
          trap("i64.div_u by 0");
        }
        return left.divU(right);
      }
      case RemSInt64: {
        if (right.getInteger() == 0) {
          trap("i64.rem_s by 0");
        }
        if (left.getInteger() == LLONG_MIN && right.getInteger() == -1LL) {
          return Literal(int64_t(0));
        }
        return left.remS(right);
      }
      case RemUInt64: {
        if (right.getInteger() == 0) {
          trap("i64.rem_u by 0");
        }
        return left.remU(right);
      }
      case DivFloat32:
      case DivFloat64:
        return left.div(right);
      case AndInt32:
      case AndInt64:
        return left.and_(right);
      case OrInt32:
      case OrInt64:
        return left.or_(right);
      case XorInt32:
      case XorInt64:
        return left.xor_(right);
      case ShlInt32:
      case ShlInt64:
        return left.shl(right);
      case ShrUInt32:
      case ShrUInt64:
        return left.shrU(right);
      case ShrSInt32:
      case ShrSInt64:
        return left.shrS(right);
      case RotLInt32:
      case RotLInt64:
        return left.rotL(right);
      case RotRInt32:
      case RotRInt64:
        return left.rotR(right);

      case EqInt32:
      case EqInt64:
      case EqFloat32:
      case EqFloat64:
        return left.eq(right);
      case NeInt32:
      case NeInt64:
      case NeFloat32:
      case NeFloat64:
        return left.ne(right);
      case LtSInt32:
      case LtSInt64:
        return left.ltS(right);
      case LtUInt32:
      case LtUInt64:
        return left.ltU(right);
      case LeSInt32:
      case LeSInt64:
        return left.leS(right);
      case LeUInt32:
      case LeUInt64:
        return left.leU(right);
      case GtSInt32:
      case GtSInt64:
        return left.gtS(right);
      case GtUInt32:
      case GtUInt64:
        return left.gtU(right);
      case GeSInt32:
      case GeSInt64:
        return left.geS(right);
      case GeUInt32:
      case GeUInt64:
        return left.geU(right);
      case LtFloat32:
      case LtFloat64:
        return left.lt(right);
      case LeFloat32:
      case LeFloat64:
        return left.le(right);
      case GtFloat32:
      case GtFloat64:
        return left.gt(right);
      case GeFloat32:
      case GeFloat64:
        return left.ge(right);

      case CopySignFloat32:
      case CopySignFloat64:
        return left.copysign(right);
      case MinFloat32:
      case MinFloat64:
        return left.min(right);
      case MaxFloat32:
      case MaxFloat64:
        return left.max(right);

      case EqVecI8x16:
        return left.eqI8x16(right);
      case NeVecI8x16:
        return left.neI8x16(right);
      case LtSVecI8x16:
        return left.ltSI8x16(right);
      case LtUVecI8x16:
        return left.ltUI8x16(right);
      case GtSVecI8x16:
        return left.gtSI8x16(right);
      case GtUVecI8x16:
        return left.gtUI8x16(right);
      case LeSVecI8x16:
        return left.leSI8x16(right);
      case LeUVecI8x16:
        return left.leUI8x16(right);
      case GeSVecI8x16:
        return left.geSI8x16(right);
      case GeUVecI8x16:
        return left.geUI8x16(right);
      case EqVecI16x8:
        return left.eqI16x8(right);
      case NeVecI16x8:
        return left.neI16x8(right);
      case LtSVecI16x8:
        return left.ltSI16x8(right);
      case LtUVecI16x8:
        return left.ltUI16x8(right);
      case GtSVecI16x8:
        return left.gtSI16x8(right);
      case GtUVecI16x8:
        return left.gtUI16x8(right);
      case LeSVecI16x8:
        return left.leSI16x8(right);
      case LeUVecI16x8:
        return left.leUI16x8(right);
      case GeSVecI16x8:
        return left.geSI16x8(right);
      case GeUVecI16x8:
        return left.geUI16x8(right);
      case EqVecI32x4:
        return left.eqI32x4(right);
      case NeVecI32x4:
        return left.neI32x4(right);
      case LtSVecI32x4:
        return left.ltSI32x4(right);
      case LtUVecI32x4:
        return left.ltUI32x4(right);
      case GtSVecI32x4:
        return left.gtSI32x4(right);
      case GtUVecI32x4:
        return left.gtUI32x4(right);
      case LeSVecI32x4:
        return left.leSI32x4(right);
      case LeUVecI32x4:
        return left.leUI32x4(right);
      case GeSVecI32x4:
        return left.geSI32x4(right);
      case GeUVecI32x4:
        return left.geUI32x4(right);
      case EqVecI64x2:
        return left.eqI64x2(right);
      case NeVecI64x2:
        return left.neI64x2(right);
      case LtSVecI64x2:
        return left.ltSI64x2(right);
      case GtSVecI64x2:
        return left.gtSI64x2(right);
      case LeSVecI64x2:
        return left.leSI64x2(right);
      case GeSVecI64x2:
        return left.geSI64x2(right);
      case EqVecF32x4:
        return left.eqF32x4(right);
      case NeVecF32x4:
        return left.neF32x4(right);
      case LtVecF32x4:
        return left.ltF32x4(right);
      case GtVecF32x4:
        return left.gtF32x4(right);
      case LeVecF32x4:
        return left.leF32x4(right);
      case GeVecF32x4:
        return left.geF32x4(right);
      case EqVecF64x2:
        return left.eqF64x2(right);
      case NeVecF64x2:
        return left.neF64x2(right);
      case LtVecF64x2:
        return left.ltF64x2(right);
      case GtVecF64x2:
        return left.gtF64x2(right);
      case LeVecF64x2:
        return left.leF64x2(right);
      case GeVecF64x2:
        return left.geF64x2(right);

      case AndVec128:
        return left.andV128(right);
      case OrVec128:
        return left.orV128(right);
      case XorVec128:
        return left.xorV128(right);
      case AndNotVec128:
        return left.andV128(right.notV128());

      case AddVecI8x16:
        return left.addI8x16(right);
      case AddSatSVecI8x16:
        return left.addSaturateSI8x16(right);
      case AddSatUVecI8x16:
        return left.addSaturateUI8x16(right);
      case SubVecI8x16:
        return left.subI8x16(right);
      case SubSatSVecI8x16:
        return left.subSaturateSI8x16(right);
      case SubSatUVecI8x16:
        return left.subSaturateUI8x16(right);
      case MinSVecI8x16:
        return left.minSI8x16(right);
      case MinUVecI8x16:
        return left.minUI8x16(right);
      case MaxSVecI8x16:
        return left.maxSI8x16(right);
      case MaxUVecI8x16:
        return left.maxUI8x16(right);
      case AvgrUVecI8x16:
        return left.avgrUI8x16(right);
      case AddVecI16x8:
        return left.addI16x8(right);
      case AddSatSVecI16x8:
        return left.addSaturateSI16x8(right);
      case AddSatUVecI16x8:
        return left.addSaturateUI16x8(right);
      case SubVecI16x8:
        return left.subI16x8(right);
      case SubSatSVecI16x8:
        return left.subSaturateSI16x8(right);
      case SubSatUVecI16x8:
        return left.subSaturateUI16x8(right);
      case MulVecI16x8:
        return left.mulI16x8(right);
      case MinSVecI16x8:
        return left.minSI16x8(right);
      case MinUVecI16x8:
        return left.minUI16x8(right);
      case MaxSVecI16x8:
        return left.maxSI16x8(right);
      case MaxUVecI16x8:
        return left.maxUI16x8(right);
      case AvgrUVecI16x8:
        return left.avgrUI16x8(right);
      case Q15MulrSatSVecI16x8:
      case RelaxedQ15MulrSVecI16x8:
        return left.q15MulrSatSI16x8(right);
      case ExtMulLowSVecI16x8:
        return left.extMulLowSI16x8(right);
      case ExtMulHighSVecI16x8:
        return left.extMulHighSI16x8(right);
      case ExtMulLowUVecI16x8:
        return left.extMulLowUI16x8(right);
      case ExtMulHighUVecI16x8:
        return left.extMulHighUI16x8(right);
      case AddVecI32x4:
        return left.addI32x4(right);
      case SubVecI32x4:
        return left.subI32x4(right);
      case MulVecI32x4:
        return left.mulI32x4(right);
      case MinSVecI32x4:
        return left.minSI32x4(right);
      case MinUVecI32x4:
        return left.minUI32x4(right);
      case MaxSVecI32x4:
        return left.maxSI32x4(right);
      case MaxUVecI32x4:
        return left.maxUI32x4(right);
      case DotSVecI16x8ToVecI32x4:
        return left.dotSI16x8toI32x4(right);
      case ExtMulLowSVecI32x4:
        return left.extMulLowSI32x4(right);
      case ExtMulHighSVecI32x4:
        return left.extMulHighSI32x4(right);
      case ExtMulLowUVecI32x4:
        return left.extMulLowUI32x4(right);
      case ExtMulHighUVecI32x4:
        return left.extMulHighUI32x4(right);
      case AddVecI64x2:
        return left.addI64x2(right);
      case SubVecI64x2:
        return left.subI64x2(right);
      case MulVecI64x2:
        return left.mulI64x2(right);
      case ExtMulLowSVecI64x2:
        return left.extMulLowSI64x2(right);
      case ExtMulHighSVecI64x2:
        return left.extMulHighSI64x2(right);
      case ExtMulLowUVecI64x2:
        return left.extMulLowUI64x2(right);
      case ExtMulHighUVecI64x2:
        return left.extMulHighUI64x2(right);

      case AddVecF32x4:
        return left.addF32x4(right);
      case SubVecF32x4:
        return left.subF32x4(right);
      case MulVecF32x4:
        return left.mulF32x4(right);
      case DivVecF32x4:
        return left.divF32x4(right);
      case MinVecF32x4:
      case RelaxedMinVecF32x4:
        return left.minF32x4(right);
      case MaxVecF32x4:
      case RelaxedMaxVecF32x4:
        return left.maxF32x4(right);
      case PMinVecF32x4:
        return left.pminF32x4(right);
      case PMaxVecF32x4:
        return left.pmaxF32x4(right);
      case AddVecF64x2:
        return left.addF64x2(right);
      case SubVecF64x2:
        return left.subF64x2(right);
      case MulVecF64x2:
        return left.mulF64x2(right);
      case DivVecF64x2:
        return left.divF64x2(right);
      case MinVecF64x2:
      case RelaxedMinVecF64x2:
        return left.minF64x2(right);
      case MaxVecF64x2:
      case RelaxedMaxVecF64x2:
        return left.maxF64x2(right);
      case PMinVecF64x2:
        return left.pminF64x2(right);
      case PMaxVecF64x2:
        return left.pmaxF64x2(right);

      case NarrowSVecI16x8ToVecI8x16:
        return left.narrowSToI8x16(right);
      case NarrowUVecI16x8ToVecI8x16:
        return left.narrowUToI8x16(right);
      case NarrowSVecI32x4ToVecI16x8:
        return left.narrowSToI16x8(right);
      case NarrowUVecI32x4ToVecI16x8:
        return left.narrowUToI16x8(right);

      case SwizzleVecI8x16:
      case RelaxedSwizzleVecI8x16:
        return left.swizzleI8x16(right);

      case DotI8x16I7x16SToVecI16x8:
        return left.dotSI8x16toI16x8(right);

      case InvalidBinary:
        WASM_UNREACHABLE("invalid binary op");
    }
    WASM_UNREACHABLE("invalid op");
  }
  Flow visitSIMDExtract(SIMDExtract* curr) {
    NOTE_ENTER("SIMDExtract");
    Flow flow = self()->visit(curr->vec);
    if (flow.breaking()) {
      return flow;
    }
    Literal vec = flow.getSingleValue();
    switch (curr->op) {
      case ExtractLaneSVecI8x16:
        return vec.extractLaneSI8x16(curr->index);
      case ExtractLaneUVecI8x16:
        return vec.extractLaneUI8x16(curr->index);
      case ExtractLaneSVecI16x8:
        return vec.extractLaneSI16x8(curr->index);
      case ExtractLaneUVecI16x8:
        return vec.extractLaneUI16x8(curr->index);
      case ExtractLaneVecI32x4:
        return vec.extractLaneI32x4(curr->index);
      case ExtractLaneVecI64x2:
        return vec.extractLaneI64x2(curr->index);
      case ExtractLaneVecF32x4:
        return vec.extractLaneF32x4(curr->index);
      case ExtractLaneVecF64x2:
        return vec.extractLaneF64x2(curr->index);
    }
    WASM_UNREACHABLE("invalid op");
  }
  Flow visitSIMDReplace(SIMDReplace* curr) {
    NOTE_ENTER("SIMDReplace");
    Flow flow = self()->visit(curr->vec);
    if (flow.breaking()) {
      return flow;
    }
    Literal vec = flow.getSingleValue();
    flow = self()->visit(curr->value);
    if (flow.breaking()) {
      return flow;
    }
    Literal value = flow.getSingleValue();
    switch (curr->op) {
      case ReplaceLaneVecI8x16:
        return vec.replaceLaneI8x16(value, curr->index);
      case ReplaceLaneVecI16x8:
        return vec.replaceLaneI16x8(value, curr->index);
      case ReplaceLaneVecI32x4:
        return vec.replaceLaneI32x4(value, curr->index);
      case ReplaceLaneVecI64x2:
        return vec.replaceLaneI64x2(value, curr->index);
      case ReplaceLaneVecF32x4:
        return vec.replaceLaneF32x4(value, curr->index);
      case ReplaceLaneVecF64x2:
        return vec.replaceLaneF64x2(value, curr->index);
    }
    WASM_UNREACHABLE("invalid op");
  }
  Flow visitSIMDShuffle(SIMDShuffle* curr) {
    NOTE_ENTER("SIMDShuffle");
    Flow flow = self()->visit(curr->left);
    if (flow.breaking()) {
      return flow;
    }
    Literal left = flow.getSingleValue();
    flow = self()->visit(curr->right);
    if (flow.breaking()) {
      return flow;
    }
    Literal right = flow.getSingleValue();
    return left.shuffleV8x16(right, curr->mask);
  }
  Flow visitSIMDTernary(SIMDTernary* curr) {
    NOTE_ENTER("SIMDBitselect");
    Flow flow = self()->visit(curr->a);
    if (flow.breaking()) {
      return flow;
    }
    Literal a = flow.getSingleValue();
    flow = self()->visit(curr->b);
    if (flow.breaking()) {
      return flow;
    }
    Literal b = flow.getSingleValue();
    flow = self()->visit(curr->c);
    if (flow.breaking()) {
      return flow;
    }
    Literal c = flow.getSingleValue();
    switch (curr->op) {
      case Bitselect:
      case LaneselectI8x16:
      case LaneselectI16x8:
      case LaneselectI32x4:
      case LaneselectI64x2:
        return c.bitselectV128(a, b);

      case RelaxedFmaVecF32x4:
        return a.relaxedFmaF32x4(b, c);
      case RelaxedFmsVecF32x4:
        return a.relaxedFmsF32x4(b, c);
      case RelaxedFmaVecF64x2:
        return a.relaxedFmaF64x2(b, c);
      case RelaxedFmsVecF64x2:
        return a.relaxedFmsF64x2(b, c);
      default:
        // TODO: implement signselect and dot_add
        WASM_UNREACHABLE("not implemented");
    }
  }
  Flow visitSIMDShift(SIMDShift* curr) {
    NOTE_ENTER("SIMDShift");
    Flow flow = self()->visit(curr->vec);
    if (flow.breaking()) {
      return flow;
    }
    Literal vec = flow.getSingleValue();
    flow = self()->visit(curr->shift);
    if (flow.breaking()) {
      return flow;
    }
    Literal shift = flow.getSingleValue();
    switch (curr->op) {
      case ShlVecI8x16:
        return vec.shlI8x16(shift);
      case ShrSVecI8x16:
        return vec.shrSI8x16(shift);
      case ShrUVecI8x16:
        return vec.shrUI8x16(shift);
      case ShlVecI16x8:
        return vec.shlI16x8(shift);
      case ShrSVecI16x8:
        return vec.shrSI16x8(shift);
      case ShrUVecI16x8:
        return vec.shrUI16x8(shift);
      case ShlVecI32x4:
        return vec.shlI32x4(shift);
      case ShrSVecI32x4:
        return vec.shrSI32x4(shift);
      case ShrUVecI32x4:
        return vec.shrUI32x4(shift);
      case ShlVecI64x2:
        return vec.shlI64x2(shift);
      case ShrSVecI64x2:
        return vec.shrSI64x2(shift);
      case ShrUVecI64x2:
        return vec.shrUI64x2(shift);
    }
    WASM_UNREACHABLE("invalid op");
  }
  Flow visitSelect(Select* curr) {
    NOTE_ENTER("Select");
    Flow ifTrue = visit(curr->ifTrue);
    if (ifTrue.breaking()) {
      return ifTrue;
    }
    Flow ifFalse = visit(curr->ifFalse);
    if (ifFalse.breaking()) {
      return ifFalse;
    }
    Flow condition = visit(curr->condition);
    if (condition.breaking()) {
      return condition;
    }
    NOTE_EVAL1(condition.getSingleValue());
    return condition.getSingleValue().geti32() ? ifTrue : ifFalse; // ;-)
  }
  Flow visitDrop(Drop* curr) {
    NOTE_ENTER("Drop");
    Flow value = visit(curr->value);
    if (value.breaking()) {
      return value;
    }
    return Flow();
  }
  Flow visitReturn(Return* curr) {
    NOTE_ENTER("Return");
    Flow flow;
    if (curr->value) {
      flow = visit(curr->value);
      if (flow.breaking()) {
        return flow;
      }
      NOTE_EVAL1(flow.getSingleValue());
    }
    flow.breakTo = RETURN_FLOW;
    return flow;
  }
  Flow visitNop(Nop* curr) {
    NOTE_ENTER("Nop");
    return Flow();
  }
  Flow visitUnreachable(Unreachable* curr) {
    NOTE_ENTER("Unreachable");
    trap("unreachable");
    WASM_UNREACHABLE("unreachable");
  }

  Literal truncSFloat(Unary* curr, Literal value) {
    double val = value.getFloat();
    if (std::isnan(val)) {
      trap("truncSFloat of nan");
    }
    if (curr->type == Type::i32) {
      if (value.type == Type::f32) {
        if (!isInRangeI32TruncS(value.reinterpreti32())) {
          trap("i32.truncSFloat overflow");
        }
      } else {
        if (!isInRangeI32TruncS(value.reinterpreti64())) {
          trap("i32.truncSFloat overflow");
        }
      }
      return Literal(int32_t(val));
    } else {
      if (value.type == Type::f32) {
        if (!isInRangeI64TruncS(value.reinterpreti32())) {
          trap("i64.truncSFloat overflow");
        }
      } else {
        if (!isInRangeI64TruncS(value.reinterpreti64())) {
          trap("i64.truncSFloat overflow");
        }
      }
      return Literal(int64_t(val));
    }
  }

  Literal truncUFloat(Unary* curr, Literal value) {
    double val = value.getFloat();
    if (std::isnan(val)) {
      trap("truncUFloat of nan");
    }
    if (curr->type == Type::i32) {
      if (value.type == Type::f32) {
        if (!isInRangeI32TruncU(value.reinterpreti32())) {
          trap("i32.truncUFloat overflow");
        }
      } else {
        if (!isInRangeI32TruncU(value.reinterpreti64())) {
          trap("i32.truncUFloat overflow");
        }
      }
      return Literal(uint32_t(val));
    } else {
      if (value.type == Type::f32) {
        if (!isInRangeI64TruncU(value.reinterpreti32())) {
          trap("i64.truncUFloat overflow");
        }
      } else {
        if (!isInRangeI64TruncU(value.reinterpreti64())) {
          trap("i64.truncUFloat overflow");
        }
      }
      return Literal(uint64_t(val));
    }
  }
  Flow visitAtomicFence(AtomicFence* curr) {
    // Wasm currently supports only sequentially consistent atomics, in which
    // case atomic_fence can be lowered to nothing.
    NOTE_ENTER("AtomicFence");
    return Flow();
  }
  Flow visitTupleMake(TupleMake* curr) {
    NOTE_ENTER("tuple.make");
    Literals arguments;
    Flow flow = generateArguments(curr->operands, arguments);
    if (flow.breaking()) {
      return flow;
    }
    for (auto arg : arguments) {
      assert(arg.type.isConcrete());
      flow.values.push_back(arg);
    }
    return flow;
  }
  Flow visitTupleExtract(TupleExtract* curr) {
    NOTE_ENTER("tuple.extract");
    Flow flow = visit(curr->tuple);
    if (flow.breaking()) {
      return flow;
    }
    assert(flow.values.size() > curr->index);
    return Flow(flow.values[curr->index]);
  }
  Flow visitLocalGet(LocalGet* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitLocalSet(LocalSet* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitGlobalGet(GlobalGet* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitGlobalSet(GlobalSet* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitCall(Call* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitCallIndirect(CallIndirect* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitLoad(Load* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitStore(Store* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitMemorySize(MemorySize* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitMemoryGrow(MemoryGrow* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitMemoryInit(MemoryInit* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitDataDrop(DataDrop* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitMemoryCopy(MemoryCopy* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitMemoryFill(MemoryFill* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitAtomicRMW(AtomicRMW* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitAtomicCmpxchg(AtomicCmpxchg* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitAtomicWait(AtomicWait* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitAtomicNotify(AtomicNotify* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitSIMDLoad(SIMDLoad* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitSIMDLoadSplat(SIMDLoad* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitSIMDLoadExtend(SIMDLoad* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitSIMDLoadZero(SIMDLoad* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitSIMDLoadStoreLane(SIMDLoadStoreLane* curr) {
    WASM_UNREACHABLE("unimp");
  }
  Flow visitPop(Pop* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitCallRef(CallRef* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitRefNull(RefNull* curr) {
    NOTE_ENTER("RefNull");
    return Literal::makeNull(curr->type.getHeapType());
  }
  Flow visitRefIsNull(RefIsNull* curr) {
    NOTE_ENTER("RefIsNull");
    Flow flow = visit(curr->value);
    if (flow.breaking()) {
      return flow;
    }
    const auto& value = flow.getSingleValue();
    NOTE_EVAL1(value);
    return Literal(int32_t(value.isNull()));
  }
  Flow visitRefFunc(RefFunc* curr) {
    NOTE_ENTER("RefFunc");
    NOTE_NAME(curr->func);
    return Literal::makeFunc(curr->func, curr->type.getHeapType());
  }
  Flow visitRefEq(RefEq* curr) {
    NOTE_ENTER("RefEq");
    Flow flow = visit(curr->left);
    if (flow.breaking()) {
      return flow;
    }
    auto left = flow.getSingleValue();
    flow = visit(curr->right);
    if (flow.breaking()) {
      return flow;
    }
    auto right = flow.getSingleValue();
    NOTE_EVAL2(left, right);
    return Literal(int32_t(left == right));
  }
  Flow visitTableGet(TableGet* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitTableSet(TableSet* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitTableSize(TableSize* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitTableGrow(TableGrow* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitTry(Try* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitThrow(Throw* curr) {
    NOTE_ENTER("Throw");
    Literals arguments;
    Flow flow = generateArguments(curr->operands, arguments);
    if (flow.breaking()) {
      return flow;
    }
    NOTE_EVAL1(curr->tag);
    WasmException exn;
    exn.tag = curr->tag;
    for (auto item : arguments) {
      exn.values.push_back(item);
    }
    throwException(exn);
    WASM_UNREACHABLE("throw");
  }
  Flow visitRethrow(Rethrow* curr) { WASM_UNREACHABLE("unimp"); }
  Flow visitI31New(I31New* curr) {
    NOTE_ENTER("I31New");
    Flow flow = visit(curr->value);
    if (flow.breaking()) {
      return flow;
    }
    const auto& value = flow.getSingleValue();
    NOTE_EVAL1(value);
    return Literal::makeI31(value.geti32());
  }
  Flow visitI31Get(I31Get* curr) {
    NOTE_ENTER("I31Get");
    Flow flow = visit(curr->i31);
    if (flow.breaking()) {
      return flow;
    }
    const auto& value = flow.getSingleValue();
    NOTE_EVAL1(value);
    if (value.isNull()) {
      trap("null ref");
    }
    return Literal(value.geti31(curr->signed_));
  }

  // Helper for ref.test, ref.cast, and br_on_cast, which share almost all their
  // logic except for what they return.
  struct Cast {
    // The control flow that preempts the cast.
    struct Breaking : Flow {
      Breaking(Flow breaking) : Flow(breaking) {}
    };
    // The result of the successful cast.
    struct Success : Literal {
      Success(Literal result) : Literal(result) {}
    };
    // The input to a failed cast.
    struct Failure : Literal {
      Failure(Literal original) : Literal(original) {}
    };

    std::variant<Breaking, Success, Failure> state;

    template<class T> Cast(T state) : state(state) {}
    Flow* getBreaking() { return std::get_if<Breaking>(&state); }
    Literal* getSuccess() { return std::get_if<Success>(&state); }
    Literal* getFailure() { return std::get_if<Failure>(&state); }
  };

  template<typename T> Cast doCast(T* curr) {
    Flow ref = self()->visit(curr->ref);
    if (ref.breaking()) {
      return typename Cast::Breaking{ref};
    }
    Literal val = ref.getSingleValue();
    Type castType = curr->getCastType();
    if (val.isNull()) {
      if (castType.isNullable()) {
        return typename Cast::Success{val};
      } else {
        return typename Cast::Failure{val};
      }
    }
    if (HeapType::isSubType(val.type.getHeapType(), castType.getHeapType())) {
      return typename Cast::Success{val};
    } else {
      return typename Cast::Failure{val};
    }
  }

  Flow visitRefTest(RefTest* curr) {
    NOTE_ENTER("RefTest");
    auto cast = doCast(curr);
    if (auto* breaking = cast.getBreaking()) {
      return *breaking;
    } else {
      return Literal(int32_t(bool(cast.getSuccess())));
    }
  }
  Flow visitRefCast(RefCast* curr) {
    NOTE_ENTER("RefCast");
    auto cast = doCast(curr);
    if (auto* breaking = cast.getBreaking()) {
      return *breaking;
    } else if (auto* result = cast.getSuccess()) {
      return *result;
    }
    assert(cast.getFailure());
    trap("cast error");
    WASM_UNREACHABLE("unreachable");
  }
  Flow visitBrOn(BrOn* curr) {
    NOTE_ENTER("BrOn");
    // BrOnCast* uses the casting infrastructure, so handle them first.
    if (curr->op == BrOnCast || curr->op == BrOnCastFail) {
      auto cast = doCast(curr);
      if (auto* breaking = cast.getBreaking()) {
        return *breaking;
      } else if (auto* original = cast.getFailure()) {
        if (curr->op == BrOnCast) {
          return *original;
        } else {
          return Flow(curr->name, *original);
        }
      } else {
        auto* result = cast.getSuccess();
        assert(result);
        if (curr->op == BrOnCast) {
          return Flow(curr->name, *result);
        } else {
          return *result;
        }
      }
    }
    // Otherwise we are just checking for null.
    Flow flow = visit(curr->ref);
    if (flow.breaking()) {
      return flow;
    }
    const auto& value = flow.getSingleValue();
    NOTE_EVAL1(value);
    if (curr->op == BrOnNull) {
      // BrOnNull does not propagate the value if it takes the branch.
      if (value.isNull()) {
        return Flow(curr->name);
      }
      // If the branch is not taken, we return the non-null value.
      return {value};
    } else {
      // BrOnNonNull does not return a value if it does not take the branch.
      if (value.isNull()) {
        return Flow();
      }
      // If the branch is taken, we send the non-null value.
      return Flow(curr->name, value);
    }
  }
  Flow visitStructNew(StructNew* curr) {
    NOTE_ENTER("StructNew");
    if (curr->type == Type::unreachable) {
      // We cannot proceed to compute the heap type, as there isn't one. Just
      // find why we are unreachable,