/*
 * Copyright 2022 WebAssembly Community Group participants
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

#include <optional>
#include <variant>

#include "ir/branch-utils.h"
#include "ir/eh-utils.h"
#include "ir/local-graph.h"
#include "ir/module-utils.h"
#include "ir/possible-contents.h"
#include "support/insert_ordered.h"
#include "wasm.h"

namespace std {

std::ostream& operator<<(std::ostream& stream,
                         const wasm::PossibleContents& contents) {
  contents.dump(stream);
  return stream;
}

} // namespace std

namespace wasm {

PossibleContents PossibleContents::combine(const PossibleContents& a,
                                           const PossibleContents& b) {
  auto aType = a.getType();
  auto bType = b.getType();
  // First handle the trivial cases of them being equal, or one of them is
  // None or Many.
  if (a == b) {
    return a;
  }
  if (b.isNone()) {
    return a;
  }
  if (a.isNone()) {
    return b;
  }
  if (a.isMany()) {
    return a;
  }
  if (b.isMany()) {
    return b;
  }

  if (!aType.isRef() || !bType.isRef()) {
    // At least one is not a reference. The only possibility left for a useful
    // combination here is if they have the same type (since we've already ruled
    // out the case of them being equal). If they have the same type then
    // neither is a reference and we can emit an exact type (since subtyping is
    // not relevant for non-references).
    if (aType == bType) {
      return ExactType(aType);
    } else {
      return Many();
    }
  }

  // Special handling for references from here.

  if (a.isNull() && b.isNull()) {
    // These must be nulls in different hierarchies, otherwise a would have
    // been handled by the `a == b` case above.
    assert(aType != bType);
    return Many();
  }

  auto lub = Type::getLeastUpperBound(aType, bType);
  if (lub == Type::none) {
    // The types are not in the same hierarchy.
    return Many();
  }

  // From here we can assume there is a useful LUB.

  // Nulls can be combined in by just adding nullability to a type.
  if (a.isNull() || b.isNull()) {
    // Only one of them can be null here, since we already handled the case
    // where they were both null.
    assert(!a.isNull() || !b.isNull());
    // If only one is a null then we can use the type info from the b, and
    // just add in nullability. For example, a literal of type T and a null
    // becomes an exact type of T that allows nulls, and so forth.
    auto mixInNull = [](ConeType cone) {
      cone.type = Type(cone.type.getHeapType(), Nullable);
      return cone;
    };
    if (!a.isNull()) {
      return mixInNull(a.getCone());
    } else if (!b.isNull()) {
      return mixInNull(b.getCone());
    }
  }

  // Find a ConeType that describes both inputs, using the shared ancestor which
  // is the LUB. We need to find how big a cone we need: the cone must be big
  // enough to contain both the inputs.
  auto aDepth = a.getCone().depth;
  auto bDepth = b.getCone().depth;
  Index newDepth;
  if (aDepth == FullDepth || bDepth == FullDepth) {
    // At least one has full (infinite) depth, so we know the new depth must
    // be the same.
    newDepth = FullDepth;
  } else {
    // The depth we need under the lub is how far from the lub we are, plus
    // the depth of our cone.
    // TODO: we could make a single loop that also does the LUB, at the same
    // time, and also avoids calling getDepth() which loops once more?
    auto aDepthFromRoot = aType.getHeapType().getDepth();
    auto bDepthFromRoot = bType.getHeapType().getDepth();
    auto lubDepthFromRoot = lub.getHeapType().getDepth();
    assert(lubDepthFromRoot <= aDepthFromRoot);
    assert(lubDepthFromRoot <= bDepthFromRoot);
    Index aDepthUnderLub = aDepthFromRoot - lubDepthFromRoot + aDepth;
    Index bDepthUnderLub = bDepthFromRoot - lubDepthFromRoot + bDepth;

    // The total cone must be big enough to contain all the above.
    newDepth = std::max(aDepthUnderLub, bDepthUnderLub);
  }

  return ConeType{lub, newDepth};
}

void PossibleContents::intersectWithFullCone(const PossibleContents& other) {
  assert(other.isFullConeType());

  if (isSubContents(other, *this)) {
    // The intersection is just |other|.
    // Note that this code path handles |this| being Many.
    value = other.value;
    return;
  }

  if (!haveIntersection(*this, other)) {
    // There is no intersection at all.
    // Note that this code path handles |this| being None.
    value = None();
    return;
  }

  // There is an intersection here. Note that this implies |this| is a reference
  // type, as it has an intersection with |other| which is a full cone type
  // (which must be a reference type).
  auto type = getType();
  auto otherType = other.getType();
  auto heapType = type.getHeapType();
  auto otherHeapType = otherType.getHeapType();

  // If both inputs are nullable then the intersection is nullable as well.
  auto nullability =
    type.isNullable() && otherType.isNullable() ? Nullable : NonNullable;

  auto setNoneOrNull = [&]() {
    if (nullability == Nullable) {
      value = Literal::makeNull(otherHeapType);
    } else {
      value = None();
    }
  };

  if (isNull()) {
    // The intersection is either this null itself, or nothing if a null is not
    // allowed.
    setNoneOrNull();
    return;
  }

  // If the heap types are not compatible then they are in separate hierarchies
  // and there is no intersection, aside from possibly a null of the bottom
  // type.
  auto isSubType = HeapType::isSubType(heapType, otherHeapType);
  auto otherIsSubType = HeapType::isSubType(otherHeapType, heapType);
  if (!isSubType && !otherIsSubType) {
    if (nullability == Nullable &&
        heapType.getBottom() == otherHeapType.getBottom()) {
      value = Literal::makeNull(heapType.getBottom());
    } else {
      value = None();
    }
    return;
  }

  if (isLiteral() || isGlobal()) {
    // The information about the value being identical to a particular literal
    // or immutable global is not removed by intersection, if the type is in the
    // cone we are intersecting with.
    if (isSubType) {
      return;
    }

    // The type must change, so continue down to the generic code path.
    // TODO: for globals we could perhaps refine the type here, but then the
    //       type on GlobalInfo would not match the module, so that needs some
    //       refactoring.
  }

  // Intersect the cones, as there is no more specific information we can use.
  auto depthFromRoot = heapType.getDepth();
  auto otherDepthFromRoot = otherHeapType.getDepth();

  // To compute the new cone, find the new heap type for it, and to compute its
  // depth, consider the adjustments to the existing depths that stem from the
  // choice of new heap type.
  HeapType newHeapType;

  if (depthFromRoot < otherDepthFromRoot) {
    newHeapType = otherHeapType;
  } else {
    newHeapType = heapType;
  }

  auto newType = Type(newHeapType, nullability);

  // By assumption |other| has full depth. Consider the other cone in |this|.
  if (hasFullCone()) {
    // Both are full cones, so the result is as well.
    value = FullConeType(newType);
  } else {
    // The result is a partial cone. If the cone starts in |otherHeapType| then
    // we need to adjust the depth down, since it will be smaller than the
    // original cone:
    /*
    //                             ..
    //                            /
    //              otherHeapType
    //            /               \
    //   heapType                  ..
    //            \
    */
    // E.g. if |this| is a cone of depth 10, and |otherHeapType| is an immediate
    // subtype of |this|, then the new cone must be of depth 9.
    auto newDepth = getCone().depth;
    if (newHeapType == otherHeapType) {
      assert(depthFromRoot <= otherDepthFromRoot);
      auto reduction = otherDepthFromRoot - depthFromRoot;
      if (reduction > newDepth) {
        // The cone on heapType does not even reach the cone on otherHeapType,
        // so the result is not a cone.
        setNoneOrNull();
        return;
      }
      newDepth -= reduction;
    }

    value = ConeType{newType, newDepth};
  }
}

bool PossibleContents::haveIntersection(const PossibleContents& a,
                                        const PossibleContents& b) {
  if (a.isNone() || b.isNone()) {
    // One is the empty set, so nothing can intersect here.
    return false;
  }

  if (a.isMany() || b.isMany()) {
    // One is the set of all things, so definitely something can intersect since
    // we've ruled out an empty set for both.
    return true;
  }

  auto aType = a.getType();
  auto bType = b.getType();

  if (!aType.isRef() || !bType.isRef()) {
    // At least one is not a reference. The only way they can intersect is if
    // the type is identical.
    return aType == bType;
  }

  // From here on we focus on references.

  auto aHeapType = aType.getHeapType();
  auto bHeapType = bType.getHeapType();

  if (aType.isNullable() && bType.isNullable() &&
      aHeapType.getBottom() == bHeapType.getBottom()) {
    // A compatible null is possible on both sides.
    return true;
  }

  // We ruled out having a compatible null on both sides. If one is simply a
  // null then no chance for an intersection remains.
  if (a.isNull() || b.isNull()) {
    return false;
  }

  auto aSubB = HeapType::isSubType(aHeapType, bHeapType);
  auto bSubA = HeapType::isSubType(bHeapType, aHeapType);
  if (!aSubB && !bSubA) {
    // No type can appear in both a and b, so the types differ, so the values
    // do not overlap.
    return false;
  }

  // From here on we focus on references and can ignore the case of null - any
  // intersection must be of a non-null value, so we can focus on the heap
  // types.

  auto aDepthFromRoot = aHeapType.getDepth();
  auto bDepthFromRoot = bHeapType.getDepth();

  if (aSubB) {
    // A is a subtype of B. For there to be an intersection we need their cones
    // to intersect, that is, to rule out the case where the cone from B is not
    // deep enough to reach A.
    assert(aDepthFromRoot >= bDepthFromRoot);
    return aDepthFromRoot - bDepthFromRoot <= b.getCone().depth;
  } else if (bSubA) {
    assert(bDepthFromRoot >= aDepthFromRoot);
    return bDepthFromRoot - aDepthFromRoot <= a.getCone().depth;
  } else {
    WASM_UNREACHABLE("we ruled out no subtyping before");
  }

  // TODO: we can also optimize things like different Literals, but existing
  //       passes do such things already so it is low priority.
}

bool PossibleContents::isSubContents(const PossibleContents& a,
                                     const PossibleContents& b) {
  // TODO: Everything else. For now we only call this when |a| or |b| is a full
  //       cone type.
  if (b.isFullConeType()) {
    if (a.isNone()) {
      return true;
    }
    if (a.isMany()) {
      return false;
    }
    if (a.isNull()) {
      return b.getType().isNullable();
    }
    return Type::isSubType(a.getType(), b.getType());
  }

  if (a.isFullConeType()) {
    // We've already ruled out b being a full cone type before, so the only way
    // |a| can be contained in |b| is if |b| is everything.
    return b.isMany();
  }

  WASM_UNREACHABLE("a or b must be a full cone");
}

namespace {

// We are going to do a very large flow operation, potentially, as we create
// a Location for every interesting part in the entire wasm, and some of those
// places will have lots of links (like a struct field may link out to every
// single struct.get of that type), so we must make the data structures here
// as efficient as possible. Towards that goal, we work with location
// *indexes* where possible, which are small (32 bits) and do not require any
// complex hashing when we use them in sets or maps.
//
// Note that we do not use indexes everywhere, since the initial analysis is
// done in parallel, and we do not have a fixed indexing of locations yet. When
// we merge the parallel data we create that indexing, and use indexes from then
// on.
using LocationIndex = uint32_t;

#ifndef NDEBUG
// Assert on not having duplicates in a vector.
template<typename T> void disallowDuplicates(const T& targets) {
#if defined(POSSIBLE_CONTENTS_DEBUG) && POSSIBLE_CONTENTS_DEBUG >= 2
  std::unordered_set<LocationIndex> uniqueTargets;
  for (const auto& target : targets) {
    uniqueTargets.insert(target);
  }
  assert(uniqueTargets.size() == targets.size());
#endif
}
#endif

// A link indicates a flow of content from one location to another. For
// example, if we do a local.get and return that value from a function, then
// we have a link from the ExpressionLocation of that local.get to a
// ResultLocation.
template<typename T> struct Link {
  T from;
  T to;

  bool operator==(const Link<T>& other) const {
    return from == other.from && to == other.to;
  }
};

using LocationLink = Link<Location>;
using IndexLink = Link<LocationIndex>;

} // anonymous namespace

} // namespace wasm

namespace std {

template<> struct hash<wasm::LocationLink> {
  size_t operator()(const wasm::LocationLink& loc) const {
    return std::hash<std::pair<wasm::Location, wasm::Location>>{}(
      {loc.from, loc.to});
  }
};

template<> struct hash<wasm::IndexLink> {
  size_t operator()(const wasm::IndexLink& loc) const {
    return std::hash<std::pair<wasm::LocationIndex, wasm::LocationIndex>>{}(
      {loc.from, loc.to});
  }
};

} // namespace std

namespace wasm {

namespace {

// The data we gather from each function, as we process them in parallel. Later
// this will be merged into a single big graph.
struct CollectedFuncInfo {
  // All the links we found in this function. Rarely are there duplicates
  // in this list (say when writing to the same global location from another
  // global location), and we do not try to deduplicate here, just store them in
  // a plain array for now, which is faster (later, when we merge all the info
  // from the functions, we need to deduplicate anyhow).
  std::vector<LocationLink> links;

  // All the roots of the graph, that is, places that begin by containing some
  // particular content. That includes i32.const, ref.func, struct.new, etc. All
  // possible contents in the rest of the graph flow from such places.
  //
  // The vector here is of the location of the root and then its contents.
  std::vector<std::pair<Location, PossibleContents>> roots;

  // In some cases we need to know the parent of the expression. Consider this:
  //
  //  (struct.set $A k
  //    (local.get $ref)
  //    (local.get $value)
  //  )
  //
  // Imagine that the first local.get, for $ref, receives a new value. That can
  // affect where the struct.set sends values: if previously that local.get had
  // no possible contents, and now it does, then we have DataLocations to
  // update. Likewise, when the second local.get is updated we must do the same,
  // but again which DataLocations we update depends on the ref passed to the
  // struct.set. To handle such things, we set add a childParent link, and then
  // when we update the child we can find the parent and handle any special
  // behavior we need there.
  std::unordered_map<Expression*, Expression*> childParents;
};

// Walk the wasm and find all the links we need to care about, and the locations
// and roots related to them. This builds up a CollectedFuncInfo data structure.
// After all InfoCollectors run, those data structures will be merged and the
// main flow will begin.
struct InfoCollector
  : public PostWalker<InfoCollector, OverriddenVisitor<InfoCollector>> {
  CollectedFuncInfo& info;

  InfoCollector(CollectedFuncInfo& info) : info(info) {}

  // Check if a type is relevant for us. If not, we can ignore it entirely.
  bool isRelevant(Type type) {
    if (type == Type::unreachable || type == Type::none) {
      return false;
    }
    if (type.isTuple()) {
      for (auto t : type) {
        if (isRelevant(t)) {
          return true;
        }
      }
    }
    return true;
  }

  bool isRelevant(Signature sig) {
    return isRelevant(sig.params) || isRelevant(sig.results);
  }

  bool isRelevant(Expression* curr) { return curr && isRelevant(curr->type); }

  template<typename T> bool isRelevant(const T& vec) {
    for (auto* expr : vec) {
      if (isRelevant(expr->type)) {
        return true;
      }
    }
    return false;
  }

  // Each visit*() call is responsible for connecting the children of a node to
  // that node. Responsibility for connecting the node's output to anywhere
  // else (another expression or the function itself, if we are at the top
  // level) is the responsibility of the outside.

  void visitBlock(Block* curr) {
    if (curr->list.empty()) {
      return;
    }

    // Values sent to breaks to this block must be received here.
    handleBreakTarget(curr);

    // The final item in the block can flow a value to here as well.
    receiveChildValue(curr->list.back(), curr);
  }
  void visitIf(If* curr) {
    // Each arm may flow out a value.
    receiveChildValue(curr->ifTrue, curr);
    receiveChildValue(curr->ifFalse, curr);
  }
  void visitLoop(Loop* curr) { receiveChildValue(curr->body, curr); }
  void visitBreak(Break* curr) {
    // Connect the value (if present) to the break target.
    handleBreakValue(curr);

    // The value may also flow through in a br_if (the type will indicate that,
    // which receiveChildValue will notice).
    receiveChildValue(curr->value, curr);
  }
  void visitSwitch(Switch* curr) { handleBreakValue(curr); }
  void visitLoad(Load* curr) {
    // We could infer the exact type here, but as no subtyping is possible, it
    // would have no benefit, so just add a generic root (which will be "Many").
    // See the comment on the ContentOracle class.
    addRoot(curr);
  }
  void visitStore(Store* curr) {}
  void visitAtomicRMW(AtomicRMW* curr) { addRoot(curr); }
  void visitAtomicCmpxchg(AtomicCmpxchg* curr) { addRoot(curr); }
  void visitAtomicWait(AtomicWait* curr) { addRoot(curr); }
  void visitAtomicNotify(AtomicNotify* curr) { addRoot(curr); }
  void visitAtomicFence(AtomicFence* curr) {}
  void visitSIMDExtract(SIMDExtract* curr) { addRoot(curr); }
  void visitSIMDReplace(SIMDReplace* curr) { addRoot(curr); }
  void visitSIMDShuffle(SIMDShuffle* curr) { addRoot(curr); }
  void visitSIMDTernary(SIMDTernary* curr) { addRoot(curr); }
  void visitSIMDShift(SIMDShift* curr) { addRoot(curr); }
  void visitSIMDLoad(SIMDLoad* curr) { addRoot(curr); }
  void visitSIMDLoadStoreLane(SIMDLoadStoreLane* curr) { addRoot(curr); }
  void visitMemoryInit(MemoryInit* curr) {}
  void visitDataDrop(DataDrop* curr) {}
  void visitMemoryCopy(MemoryCopy* curr) {}
  void visitMemoryFill(MemoryFill* curr) {}
  void visitConst(Const* curr) {
    addRoot(curr, PossibleContents::literal(curr->value));
  }
  void visitUnary(Unary* curr) {
    // We could optimize cases like this using interpreter integration: if the
    // input is a Literal, we could interpret the Literal result. However, if
    // the input is a literal then the GUFA pass will emit a Const there, and
    // the Precompute pass can use that later to interpret a result. That is,
    // the input we need here, a constant, is already something GUFA can emit as
    // an output. As a result, integrating the interpreter here would perhaps
    // make compilation require fewer steps, but it wouldn't let us optimize
    // more than we could before.
    addRoot(curr);
  }
  void visitBinary(Binary* curr) { addRoot(curr); }
  void visitSelect(Select* curr) {
    receiveChildValue(curr->ifTrue, curr);
    receiveChildValue(curr->ifFalse, curr);
  }
  void visitDrop(Drop* curr) {}
  void visitMemorySize(MemorySize* curr) { addRoot(curr); }
  void visitMemoryGrow(MemoryGrow* curr) { addRoot(curr); }
  void visitRefNull(RefNull* curr) {
    addRoot(
      curr,
      PossibleContents::literal(Literal::makeNull(curr->type.getHeapType())));
  }
  void visitRefIsNull(RefIsNull* curr) {
    // TODO: Optimize when possible. For example, if we can infer an exact type
    //       here which allows us to know the result then we should do so. This
    //       is unlike the case in visitUnary, above: the information that lets
    //       us optimize *cannot* be written into Binaryen IR (unlike a Literal)
    //       so using it during this pass allows us to optimize new things.
    addRoot(curr);
  }
  void visitRefFunc(RefFunc* curr) {
    addRoot(
      curr,
      PossibleContents::literal(Literal(curr->func, curr->type.getHeapType())));

    // The presence of a RefFunc indicates the function may be called
    // indirectly, so add the relevant connections for this particular function.
    // We do so here in the RefFunc so that we only do it for functions that
    // actually have a RefFunc.
    auto* func = getModule()->getFunction(curr->func);
    for (Index i = 0; i < func->getParams().size(); i++) {
      info.links.push_back(
        {SignatureParamLocation{func->type, i}, ParamLocation{func, i}});
    }
    for (Index i = 0; i < func->getResults().size(); i++) {
      info.links.push_back(
        {ResultLocation{func, i}, SignatureResultLocation{func->type, i}});
    }
  }
  void visitRefEq(RefEq* curr) {
    addRoot(curr);
  }
  void visitTableGet(TableGet* curr) {
    addRoot(curr);
  }
  void visitTableSet(TableSet* curr) {}
  void visitTableSize(TableSize* curr) { addRoot(curr); }
  void visitTableGrow(TableGrow* curr) { addRoot(curr); }

  void visitNop(Nop* curr) {}
  void visitUnreachable(Unreachable* curr) {}

#ifndef NDEBUG
  // For now we only handle pops in a catch body, see visitTry(). To check for
  // errors, use counter of the pops we handled and all the pops; those sums
  // must agree at the end, or else we've seen something we can't handle.
  Index totalPops = 0;
  Index handledPops = 0;
#endif

  void visitPop(Pop* curr) {
#ifndef NDEBUG
    totalPops++;
#endif
  }
  void visitI31New(I31New* curr) {
    // TODO: optimize like struct references
    addRoot(curr);
  }
  void visitI31Get(I31Get* curr) {
    // TODO: optimize like struct references
    addRoot(curr);
  }

  void visitRefCast(RefCast* curr) { receiveChildValue(curr->ref, curr); }
  void visitRefTest(RefTest* curr) { addRoot(curr); }
  void visitBrOn(BrOn* curr) {
    // TODO: optimize when possible
    handleBreakValue(curr);
    receiveChildValue(curr-