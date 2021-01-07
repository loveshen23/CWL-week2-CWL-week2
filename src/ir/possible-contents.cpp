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
 