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

//
// Merge unneeded types: types that are not needed for validation, and have no
// detectable runtime effect. Completely unused types are removed anyhow during
// binary writing, so this handles the case of used types that can be merged
// into others. Specifically we merge a type into its super, which is possible
// when it has no extra fields, no refined fields, and no casts.
//
// Note that such "redundant" types may help the optimizer, so merging them can
// have a negative effect later. For that reason this may be best run near the
// very end of the optimization pipeline, when nothing else is expected to do
// type-based optimizations later. However, you also do not want to merge at the
// very end, as e.g. type merging may open up function merging opportunities.
// One possible sequence:
//
//   --type-ssa -Os --type-merging -Os
//
// That is, running TypeSSA early makes sense, as it provides more type info.
// Then we hope the optimizer benefits from that, and after that we merge types
// and then optimize a final time. You can experiment with more optimization
// passes in between.
//

#include "ir/module-utils.h"
#include "ir/type-updating.h"
#include "pass.h"
#include "support/dfa_minimization.h"
#include "support/small_set.h"
#include "support/topological_sort.h"
#include "wasm-builder.h"
#include "wasm-type-ordering.h"
#include "wasm.h"

#define TYPE_MERGING_DEBUG 0

#if TYPE_MERGING_DEBUG
#include "wasm-type-printing.h"
#endif

namespace wasm {

namespace {

// We need to find all types that are distinguished from their supertypes by
// some kind of cast instruction. Merging these types with their supertypes
// would be an observable change. In contrast, types that are never used in
// casts are never distinguished from their supertypes.

// Most functions do no casts, or perhaps cast |this| and perhaps a few others.
using CastTypes = SmallUnorderedSet<HeapType, 5>;

struct CastFinder : public PostWalker<CastFinder> {
  CastTypes castTypes;

  // If traps never happen, then ref.cast and call_indirect can never
  // differentiate between types since they always succeed. Take advantage of
  // that by not having those instructions inhibit merges in TNH mode.
  // mode.
  bool trapsNeverHappen;

  CastFinder(const PassOptions& options)
    : trapsNeverHappen(options.trapsNeverHappen) {}

  template<typename T> void visitCast(T* curr) {
    if (auto type = curr->getCastType(); type != Type::unreachable) {
      castTypes.insert(type.getHeapType());
    }
  }

  void visitRefCast(RefCast* curr) {
    if (!trapsNeverHappen) {
      visitCast(curr);
    }
  }

  void visitRefTest(RefTest* curr) { visitCast(curr); }

  void visitBrOn(BrOn* curr) {
    if (curr->op == BrOnCast || curr->op == BrOnCastFail) {
      visitCast(curr);
    }
  }

  void visitCallIndirect(CallIndirect* curr) {
    if (!trapsNeverHappen) {
      castTypes.insert(curr->heapType);
    }
  }
};

// We are going to treat the type graph as a partitioned DFA where each type is
// a state with transitions to its children. We will partition the DFA states so
// that types that may be mergeable will be in the same partition and types that
// we know are not mergeable will be in different partitions. Then we will
// refine the partitions so that types that turn out to not be mergeable will be
// split out into separate partitions.
struct TypeMerging : public Pass {
  // Only modifies types.
  bool requiresNonNullableLocalFixups() override { return false; }

  Module* module;

  std::unordered_set<HeapType> privateTypes;
  CastTypes castTypes;

  void run(Module* module_) override;

  CastTypes findCastTypes();
  std::vector<HeapType> getPublicChildren(HeapType type);
  DFA::State<HeapType> makeDFAState(HeapType type);
  void applyMerges(const TypeMapper::TypeUpdates& merges);
};

// Hash and equality-compare HeapTypes based on their top-level structure (i.e.
// "shape"), ignoring nontrivial heap type children that will not be
// differentiated between until we run the DFA partition refinement.
bool shapeEq(HeapType a, HeapType b);
bool shapeEq(const Struct& a, const Struct& b);
bool shapeEq(Array a, Array b);
bool shapeEq(Signature a, Signature b);
bool shapeEq(Field a, Field b);
bool shapeEq(Type a, Type b);
bool shapeEq(const Tuple& a, const Tuple& b);

size_t shapeHash(HeapType a);
size_t shapeHash(const Struct& a);
size_t shapeHash(Array a);
size_t shapeHash(Signature a);
size_t shapeHash(Field a);
size_t shapeHash(Type a);
size_t shapeHash(const Tuple& a);

struct ShapeEq {
  bool operator()(const HeapType& a, const HeapType& b) const {
    return shapeEq(a, b);
  }
};

struct ShapeHash {
  size_t operator()(const HeapType& type) const { return shapeHash(type); }
};

void TypeMerging::run(Module* module_) {
  module = module_;

  if (!module->features.hasGC()) {
    return;
  }

  if (!getPassOptions().closedWorld) {
    Fatal() << "TypeMerging requires --closed-world";
  }

  // First, find all the cast types and private types. We will need these to
  // determine whether types are eligible to be merged.
  auto privates = ModuleUtils::getPrivateHeapTypes(*module);
  privateTypes = std::unordered_set<HeapType>(privates.begin(), privates.end());
  castTypes = findCastTypes();

  // Initial partitions are formed by grouping types with their structurally
  // similar supertypes. Starting with the topmost types and working down the
  // subtype trees, add each type to its supertype's partition if they are
  // structurally compatible.

  // A list of partitions with stable iterators.
  using Partition = std::vector<DFA::State<HeapType>>;
  using Partitions = std::list<Partition>;
  Partitions partitions;

  // Map each type to its partition in the list.
  std::unordered_map<HeapType, Partitions::iterator> typePartitions;

  // Map optional supertypes and the top-level structures of their refined
  // children to partitions so that different children that refine the supertype
  // in the same way can be assigned to the same partition and potentially
  // merged.
  // TODO: This is not fully general because it still prevents types from being
  // merged if they are identical subtypes of two other types that end up being
  // merged. Fixing this would require 1) merging such input partitions and
  // re-running DFA minimization until convergence or 2) treating supertypes as
  // special transitions in the DFA and augmenting Valmari-Lehtinen DFA
  // minimization so that these special transitions cannot be used to split a
  // partition if they are self-transitions.
  std::unordered_map<
    std::optional<HeapType>,
    std::unordered_map<HeapType, Partitions::iterator, ShapeHash, ShapeEq>>
    shapePartitions;

#if TYPE_MERGING_DEBUG
  using Fallback = IndexedTypeNameGenerator<DefaultTypeNameGenerator>;
  Fallback printPrivate(privates, "private.");
  ModuleTypeNameGenerator<Fallback> print(*module, printPrivate);
  auto dumpPartitions = [&]() {
    size_t i = 0;
    for (auto& partition : partitions) {
      std::cerr << i++ << ": " << print(partition[0].val) << "\n";
      for (size_t j = 1; j < partition.size(); ++j) {
        std::cerr << "   " << print(partition[j].val) << "\n";
      }
      std::cerr << "\n";
    }
  };
#endif // TYPE_MERGING_DEBUG

  // Ensure the type has a partition and return a reference to it. Since we
  // merge up the type tree and visit supertypes first, the partition usually
  // already exists. The exception is when the supertype is public, in which
  // case we might not have created a partition for it yet.
  auto ensurePartition = [&](HeapType type) -> Partitions::iterator {
    auto [it, inserted] = typePartitions.insert({type, partitions.end()});
    if (inserted) {
      it->second = partitions.insert(partitions.end(), {makeDFAState(type)});
    }
    return it->second;
  };

  // Similar to the above, but look up or create a partition associated with the
  // type's supertype and top-level shape rather than its identity.
  auto ensureShapePartition = [&](HeapType type) -> Partitions::iterator {
    auto [it, inserted] =
      shapePartitions[type.getSuperType()].insert({type, partition