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
// Locals-related optimizations
//
// This "sinks" local.sets, pushing them to the next local.get where possible,
// and removing the set if there are no gets remaining (the latter is
// particularly useful in ssa mode, but not only).
//
// We also note where local.sets coalesce: if all breaks of a block set
// a specific local, we can use a block return value for it, in effect
// removing multiple local.sets and replacing them with one that the
// block returns to. Further optimization rounds then have the opportunity
// to remove that local.set as well. TODO: support partial traces; right
// now, whenever control flow splits, we invalidate everything.
//
// After this pass, some locals may be completely unused. reorder-locals
// can get rid of those (the operation is trivial there after it sorts by use
// frequency).
//
// This pass has options:
//
//   * Tee: allow teeing, i.e., sinking a local with more than one use,
//          and so after sinking we have a tee for the first use.
//   * Structure: create block and if return values, by merging the
//                internal local.sets into one on the outside,
//                that can itself then be sunk further.
//
// There is also an option to disallow nesting entirely, which disallows
// Tee and Structure from those 2 options, and also disallows any sinking
// operation that would create nesting. This keeps the IR flat while
// removing redundant locals.
//

#include "ir/equivalent_sets.h"
#include <ir/branch-utils.h>
#include <ir/effects.h>
#include <ir/find_all.h>
#include <ir/linear-execution.h>
#include <ir/local-utils.h>
#include <ir/manipulation.h>
#include <ir/utils.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm-traversal.h>
#include <wasm.h>

namespace wasm {

// Main class

template<bool allowTee = true,
         bool allowStructure = true,
         bool allowNesting = true>
struct SimplifyLocals
  : public WalkerPass<LinearExecutionWalker<
      SimplifyLocals<allowTee, allowStructure, allowNesting>>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<
      SimplifyLocals<allowTee, allowStructure, allowNesting>>();
  }

  // information for a local.set we can sink
  struct SinkableInfo {
    Expression** item;
    EffectAnalyzer effects;

    SinkableInfo(Expression** item, PassOptions& passOptions, Module& module)
      : item(item), effects(passOptions, module, *item) {}
  };

  // a list of sinkables in a linear execution trace
  using Sinkables = std::map<Index, SinkableInfo>;

  // locals in current linear execution trace, which we try to sink
  Sinkables sinkables;

  // Information about an exit from a block: the break, and the
  // sinkables. For the final exit from a block (falling off)
  // exitter is null.
  struct BlockBreak {
    Expression** brp;
    Sinkables sinkables;
  };

  // a list of all sinkable traces that exit a block. the last
  // is falling off the end, others are branches. this is used for
  // block returns
  std::map<Name, std::vector<BlockBreak>> blockBreaks;

  // blocks that we can't produce a block return value for them.
  // (switch target, or some o