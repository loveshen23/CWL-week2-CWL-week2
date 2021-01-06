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

#include "module-utils.h"
#include "ir/intrinsics.h"
#include "support/insert_ordered.h"
#include "support/topological_sort.h"

namespace wasm::ModuleUtils {

namespace {

// Helper for collecting HeapTypes and their frequencies.
struct Counts : public InsertOrderedMap<HeapType, size_t> {
  void note(HeapType type) {
    if (!type.isBasic()) {
      (*this)[type]++;
    }
  }
  void note(Type type) {
    for (HeapType ht : type.getHeapTypeChildren()) {
      note(ht);
    }
  }
  // Ensure a type is included without increasing its count.
  void include(HeapType type) {
    if (!type.isBasic()) {
      (*this)[type];
    }
  }
  void include(Type type) {
    for (HeapType ht : type.getHeapTypeChildren()) {
      include(ht);
    }
  }
};

struct CodeScanner
  : PostWalker<CodeScanner, UnifiedExpressionVisitor<CodeScanner>> {
  Counts& counts;

  CodeScanner(Module& wasm, Counts& counts) : counts(counts) {
    setModule(&wasm);
  }

  void visitExpression(Expression* curr) {
    if (auto* call = curr->dynCast<CallIndirect>()) {
      counts.note(call->heapType);
    } else if (auto* call = curr->dynCast<CallRef>()) {
      counts.note(call->target->type);
    } else if (curr->is<RefNull>()) {
      counts.note(curr->type);
    } else if (curr->is<Select>() && curr->type.isRef()) {
      // This select will be annotated in the binary, so note it.
      counts.note(curr->type);
    } else if (curr->is<StructNew>()) {
      counts.note(curr->type);
    } else if (curr->is<ArrayNew>()) {
      counts.note(curr->type);
    } else if (curr->is<ArrayNewSeg>()) {
      counts.note(curr->type);
    } else if (curr->is<ArrayNewFixed>()) {
      counts.note(curr->type);
    } else if (auto* cast = curr->dynCast<RefCast>()) {
      counts.note(cast->type);
    } else if (auto* cast = curr->dynCast<RefTest>()) {
      counts.note(cast->castType);
    } else if (auto* cast = curr->dynCast<BrOn>()) {
      if (cast->op == BrOnCast || cast->op == BrOnCastFail) {
        counts.note(cast->castType);
      }
    } else if (auto* get = curr->dynCast<StructGet>()) {
      counts.note(get->ref->type);
      // If the type we read is a reference type then we must include it. It is
      // not written in the binary format, so it doesn't need to be counted, but
      // it does need to be taken into account in the IR (this may be the only
      // place this type appears in the entire binary, and we must scan all
      // types as the analyses that use us depend on that). TODO: This is kind
      // of a hack, so it would be nice to remove. If we could remove it, we
      // could also remove some of the pruning logic in getHeapTypeCounts below.
      counts.include(get->type);
    } else if (auto* set = curr->dynCast<StructSet>()) {
      counts.note(set->ref->type);
    } else if (auto* get = curr->dynCast<ArrayGet>()) {
      counts.note(get->ref->type);
      // See note on StructGet above.
      counts.include(get->type);
    } else if (auto* set = curr->dynCast<ArraySet>()) {
      counts.note(set->ref->type);
    } else if (Properties::isControlFlowStructure(curr)) {
      if (curr->type.isTuple()) {
        // TODO: Allow control flow to have input types as well
        counts.note(Signature(Type::none, curr->type));
      } else {
        counts.note(curr->type);
      }
    }
  }
};

// Count the number of times each heap type that would appear in the binary is
// referenced. If `prune`, exclude types that are never referenced, even though
// a binary would be invalid without them.
Counts getHeapTypeCounts(Module& wasm, bool prune = false) {
  // Collect module-level info.
  Counts counts;
  CodeScanner(wasm, counts).walkModuleCode(&wasm);
  for (auto& curr : wasm.globals) {
    counts.note(curr->type);
  }
  for (auto& curr : wasm.tags) {
    counts.note(curr->sig);
  }
  for (auto& curr : wasm.tables) {
    counts.note(curr->type);
  }
  for (auto& curr : wasm.elementSegments) {
    counts.note(curr->type);
  }

  // Collect info from functions in parallel.
  ModuleUtils::ParallelFunctionAnalysis<Counts, Immutable, InsertOrderedMap>
    analysis(wasm, [&](Function* func, Counts& counts) {
      counts.note(func->type);
      for (auto type : func->vars) {
        counts.note(type);
      }
      if (!func->imported()) {
        CodeScanner(wasm, counts).walk(func->body);
      }
    });

  // Combine the function info with the module info.
  for (auto& [_, functionCounts] : analysis.map) {
    for (auto& [sig, count] : functionCounts) {
      counts[sig] += count;
    }
  }

  if (prune) {
    // Remove types that are not actually used.
    auto it = counts.begin();
    while (it != counts.end()) {
      if (it->second == 0) {
        auto deleted = it++;
        counts.erase(deleted);
      } else {
        ++it;
      }
    }
  }

  // Recursively traverse each reference type, which may have a child type that
  // is itself a reference type. This reflects an appearance in the binary
  // format that is in the type section itself. As we do this we may find more
  // and more types, as nested children of previous ones. Each such type will
  // appear in the type section once, so we just need to visit it once. Also
  // track which recursion groups we've already processed to avoid quadratic
  // behavior when there is a single large group.
  InsertOrderedSet<He