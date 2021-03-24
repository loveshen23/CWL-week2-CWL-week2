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

#include <chrono>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#endif

#include "ir/hashed.h"
#include "ir/module-utils.h"
#include "ir/type-updating.h"
#include "pass.h"
#include "passes/passes.h"
#include "support/colors.h"
#include "wasm-debug.h"
#include "wasm-io.h"
#include "wasm-validator.h"

namespace wasm {

// PassRegistry

PassRegistry::PassRegistry() { registerPasses(); }

static PassRegistry singleton;

PassRegistry* PassRegistry::get() { return &singleton; }

void PassRegistry::registerPass(const char* name,
                                const char* description,
                                Creator create) {
  assert(passInfos.find(name) == passInfos.end());
  passInfos[name] = PassInfo(description, create);
}

void PassRegistry::registerTestPass(const char* name,
                                    const char* description,
                                    Creator create) {
  assert(passInfos.find(name) == passInfos.end());
  passInfos[name] = PassInfo(description, create, true);
}

std::unique_ptr<Pass> PassRegistry::createPass(std::string name) {
  if (passInfos.find(name) == passInfos.end()) {
    Fatal() << "Could not find pass: " << name << "\n";
  }
  std::unique_ptr<Pass> ret;
  ret.reset(passInfos[name].create());
  ret->name = name;
  return ret;
}

std::vector<std::string> PassRegistry::getRegisteredNames() {
  std::vector<std::string> ret;
  for (auto& [name, _] : passInfos) {
    ret.push_back(name);
  }
  return ret;
}

std::string PassRegistry::getPassDescription(std::string name) {
  assert(passInfos.find(name) != passInfos.end());
  return passInfos[name].description;
}

bool PassRegistry::isPassHidden(std::string name) {
  assert(passInfos.find(name) != passInfos.end());
  return passInfos[name].hidden;
}

// PassRunner

void PassRegistry::registerPasses() {
  registerPass("alignment-lowering",
               "lower unaligned loads and stores to smaller aligned ones",
               createAlignmentLoweringPass);
  registerPass("asyncify",
               "async/await style transform, allowing pausing and resuming",
               createAsyncifyPass);
  registerPass("avoid-reinterprets",
               "Tries to avoid reinterpret operations via more loads",
               createAvoidReinterpretsPass);
  registerPass(
    "dae", "removes arguments to calls in an lto-like manner", createDAEPass);
  registerPass("dae-optimizing",
               "removes arguments to calls in an lto-like manner, and "
               "optimizes where we removed",
               createDAEOptimizingPass);
  registerPass("abstract-type-refining",
               "refine and merge abstract (never-created) types",
               createAbstractTypeRefiningPass);
  registerPass("coalesce-locals",
               "reduce # of locals by coalescing",
               createCoalesceLocalsPass);
  registerPass("coalesce-locals-learning",
               "reduce # of locals by coalescing and learning",
               createCoalesceLocalsWithLearningPass);
  registerPass("code-pushing",
               "push code forward, potentially making it not always execute",
               createCodePushingPass);
  registerPass(
    "code-folding", "fold code, merging duplicates", createCodeFoldingPass);
  registerPass("const-hoisting",
               "hoist repeated constants to a local",
               createConstHoistingPass);
  registerPass("cfp",
               "propagate constant struct field values",
               createConstantFieldPropagationPass);
  registerPass(
    "dce", "removes unreachable code", createDeadCodeEliminationPass);
  registerPass("dealign",
               "forces all loads and stores to have alignment 1",
               createDeAlignPass);
  registerPass("denan",
               "instrument the wasm to convert NaNs into 0 at runtime",
               createDeNaNPass);
  registerPass(
    "directize", "turns indirect calls into direct ones", createDirectizePass);
  registerPass("discard-global-effects",
               "discards global effect info",
               createDiscardGlobalEffectsPass);
  registerPass(
    "dfo", "optimizes using the DataFlow SSA IR", createDataFlowOptsPass);
  registerPass("dwarfdump",
               "dump DWARF debug info sections from the read binary",
               createDWARFDumpPass);
  registerPass("duplicate-import-elimination",
               "removes duplicate imports",
               createDuplicateImportEliminationPass);
  registerPass("duplicate-function-elimination",
               "removes duplicate functions",
               createDuplicateFunctionEliminationPass);
  registerPass("emit-target-features",
               "emit the target features section in the output",
               createEmitTargetFeaturesPass);
  registerPass("extract-function",
               "leaves just one function (useful for debugging)",
               createExtractFunctionPass);
  registerPass("extract-function-index",
               "leaves just one function selected by index",
               createExtractFunctionIndexPass);
  registerPass(
    "flatten", "flattens out code, removing nesting", createFlattenPass);
  registerPass("fpcast-emu",
               "emulates function pointer casts, allowing incorrect indirect "
               "calls to (sometimes) work",
               createFuncCastEmulationPass);
  registerPass(
    "func-metrics", "reports function metrics", createFunctionMetricsPass);
  registerPass("generate-dyncalls",
               "generate dynCall fuctions used by emscripten ABI",
               createGenerateDynCallsPass);
  registerPass(
    "generate-i64-dyncalls",
    "generate dynCall functions used by emscripten ABI, but only for "
    "functions with i64 in their signature (which cannot be invoked "
    "via the wasm table without JavaScript BigInt support).",
    createGenerateI64DynCallsPass);
  registerPass("generate-global-effects",
               "generate global effect info (helps later passes)",
               createGenerateGlobalEffectsPass);
  registerPass(
    "generate-stack-ir", "generate Stack IR", createGenerateStackIRPass);
  registerPass(
    "global-refining", "refine the types of globals", createGlobalRefiningPass);
  registerPass(
    "gsi", "globally optimize struct values", createGlobalStructInferencePass);
  registerPass(
    "gto", "globally optimize GC types", createGlobalTypeOptimizationPass);
  registerPass("gufa",
               "Grand Unified Flow Analysis: optimize the entire program using "
               "information about what content can actually appear in each "
               "location",
               createGUFAPass);
  registerPass("gufa-optimizing",
               "GUFA plus local optimizations in functions we modified",
               createGUFAOptimizingPass);
  registerPass("type-refining",
               "apply more specific subtypes to type fields where possible",
               createTypeRefiningPass);
  registerPass(
    "heap2local", "replace GC allocations with locals", createHeap2LocalPass);
  registerPass(
    "inline-main", "inline __original_main into main", createInlineMainPass);
  registerPass("inlining",
               "inline functions (you probably want inlining-optimizing)",
               createInliningPass);
  registerPass("inlining-optimizing",
               "inline functions and optimizes where we inlined",
               createInliningOptimizingPass);
  registerPass("intrinsic-lowering",
               "lower away binaryen intrinsics",
               createIntrinsicLoweringPass);
  registerPass("jspi",
               "wrap imports and exports for JavaScript promise integration",
               createJSPIPass);
  registerPass("legalize-js-interface",
               "legalizes i64 types on the import/export boundary",
               createLegalizeJSInterfacePass);
  registerPass("legalize-js-interface-minimally",
               "legalizes i64 types on the import/export boundary in a minimal "
               "manner, only on things only JS will call",
               createLegalizeJSInterfaceMinimallyPass);
  registerPass("local-cse",
               "common subexpression elimination inside basic blocks",
               createLocalCSEPass);
  registerPass("local-subtyping",
               "apply more specific subtypes to locals where possible",
               createLocalSubtypingPass);
  registerPass("log-execution",
               "instrument the build with logging of where execution goes",
               createLogExecutionPass);
  registerPass("i64-to-i32-lowering",
               "lower all uses of i64s to use i32s instead",
               createI64ToI32LoweringPass);
  registerPass(
    "instrument-locals",
    "instrument the build with code to intercept all loads and stores",
    createInstrumentLocalsPass);
  registerPass(
    "instrument-memory",
    "instrument the build with code to intercept all loads and stores",
    createInstrumentMemoryPass);
  registerPass(
    "licm", "loop invariant code motion", createLoopInvariantCodeMotionPass);
  registerPass("limit-segments",
               "attempt to merge segments to fit within web limits",
               createLimitSegmentsPass);
  registerPass("memory64-lowering",
               "lower loads and stores to a 64-bit memory to instead use a "
               "32-bit one",
               createMemory64LoweringPass);
  registerPass("memory-packing",
               "packs memory into separate segments, skipping zeros",
               createMemoryPackingPass);
  registerPass(
    "merge-blocks", "merges blocks to their parents", createMergeBlocksPass);
  registerPass("merge-similar-functions",
               "merges similar functions when benefical",
               createMergeSimilarFunctionsPass);
  registerPass(
    "merge-locals", "merges locals when beneficial", createMergeLocalsPass);
  registerPass("metrics", "reports metrics", createMetricsPass);
  registerPass("minify-imports",
               "minifies import names (only those, and not export names), and "
               "emits a mapping to the minified ones",
               createMinifyImportsPass);
  registerPass("minify-imports-and-exports",
               "minifies both import and export names, and emits a mapping to "
               "the minified ones",
               createMinifyImportsAndExportsPass);
  registerPass("minify-imports-and-exports-and-modules",
               "minifies both import and export names, and emits a mapping to "
               "the minified ones, and minifies the modules as well",
               createMinifyImportsAndExportsAndModulesPass);
  registerPass("mod-asyncify-always-and-only-unwind",
               "apply the assumption that asyncify imports always unwind, "
               "and we never rewind",
               createModAsyncifyAlwaysOnlyUnwindPass);
  registerPass("mod-asyncify-never-unwind",
               "apply the assumption that asyncify never unwinds",
               createModAsyncifyNeverUnwindPass);
  registerPass("monomorphize",
               "creates specialized versions of functions",
               createMonomorphizePass);
  registerPass("monomorphize-always",
               "creates specialized versions of functions (even if unhelpful)",
               createMonomorphizeAlwaysPass);
  registerPass("multi-memory-lowering",
               "combines multiple memories into a single memory",
               createMultiMemoryLoweringPass);
  registerPass(
    "multi-memory-lowering-with-bounds-checks",
    "combines multiple memories into a single memory, trapping if the read or "
    "write is larger than the length of the memory's data",
    createMultiMemoryLoweringWithBoundsChecksPass);
  registerPass("nm", "name list", createNameListPass);
  registerPass("name-types", "(re)name all heap types", createNameTypesPass);
  registerPass("once-reduction",
               "reduces calls to code that only runs once",
               createOnceReductionPass);
  registerPass("optimize-added-constants",
               "optimizes added constants into load/store offsets",
               createOptimizeAddedConstantsPass);
  registerPass("optimize-added-constants-propagate",
               "optimizes added constants into load/store offsets, propagating "
               "them across locals too",
               createOptimizeAddedConstantsPropagatePass);
  registerPass(
    "optimize-casts", "eliminate and reuse casts", createOptimizeCastsPass);
  registerPass("optimize-instructions",
               "optimizes instruction combinations",
               createOptimizeInstructionsPass);
  registerPass(
    "optimize-stack-ir", "optimize Stack IR", createOptimizeStackIRPass);
  registerPass("pick-load-signs",
               "pick load signs based on their uses",
               createPickLoadSignsPass);
  registerPass(
    "poppify", "Tranform Binaryen IR into Poppy IR", createPoppifyPass);
  registerPass("post-emscripten",
               "miscellaneous optimizations for Emscripten-generated code",
               createPostEmscriptenPass);
  registerPass("optimize-for-js",
               "early optimize of the instruction combinations for js",
               createOptimizeForJSPass);
  registerPass("precompute",
               "computes compile-time evaluatable expressions",
               createPrecomputePass);
  registerPass("precompute-propagate",
               "computes compile-time evaluatable expressions and propagates "
               "them through locals",
               createPrecomputePropagatePass);
  registerPass("print", "print in s-expression format", createPrinterPass);
  registerPass("print-minified",
               "print in minified s-expression format",
               createMinifiedPrinterPass);
  registerPass("print-features",
               "print options for enabled features",
               createPrintFeaturesPass);
  registerPass(
    "print-full", "print in full s-expression format", createFullPrinterPass);
  registerPass(
    "print-call-graph", "print call graph", createPrintCallGraphPass);

  // Register PrintFunctionMap using its normal name.
  registerPass("print-function-map",
               "print a map of function indexes to names",
               createPrintFunctionMapPass);
  // Also register it as "symbolmap" so that  wasm-opt --symbolmap=foo  is the
  // same as  wasm-as --symbolmap=foo  even though the latter is not a pass
  // (wasm-as cannot run arbitrary passes).
  // TODO: switch emscripten to this name, then remove the old one
  registerPass(
    "symbolmap", "(alias for print-function-map)", createPrintFunctionMapPass);

  registerPass("print-stack-ir",
               "print out Stack IR (useful for internal debugging)",
               createPrintStackIRPass);
  registerPass("remove-non-js-ops",
               "removes operations incompatible with js",
               createRemoveNonJSOpsPass);
  registerPass("remove-imports",
               "removes imports and replaces them with nops",
               createRemoveImportsPass);
  registerPass(
    "remove-memory", "removes memory segments", createRemoveMemoryPass);
  registerPass("remove-unused-brs",
               "removes breaks from locations that are not needed",
               createRemoveUnusedBrsPass);
  registerPass("remove-unused-module-elements",
               "removes unused module elements",
               createRemoveUnusedModuleElementsPass);
  registerPass("remove-unused-nonfunction-module-elements",
               "removes unused module elements that are not functions",
               createRemoveUnusedNonFunctionModuleElementsPass);
  registerPass("remove-unused-names",
               "removes names from locations that are never branched to",
               createRemoveUnusedNamesPass);
  registerPass("remove-unused-types",
               "remove unused private GC types",
               createRemoveUnusedTypesPass);
  registerPass("reorder-functions",
               "sorts functions by access frequency",
               createReorderFunctionsPass);
  registerPass("reorder-globals",
               "sorts globals by access frequency",
               createReorderGlobalsPass);
  registerTestPass("reorder-globals-always",
                   "sorts globals by access frequency (even if there are few)",
                   createReorderGlobalsAlwaysPass);
  registerPass("reorder-locals",
               "sorts locals by access frequency",
               createReorderLocalsPass);
  registerPass("rereloop",
               "re-optimize control flow using the relooper algorithm",
               createReReloopPass);
  registerPass(
    "rse", "remove redundant local.sets", createRedundantSetEliminationPass);
  registerPass("roundtrip",
               "write the module to binary, then read it",
               createRoundTripPass);
  registerPass("safe-heap",
               "instrument loads and stores to check for invalid behavior",
               createSafeHeapPass);
  registerPass("set-globals",
               "sets specified globals to specified values",
               createSetGlobalsPass);
  registerPass("signature-pruning",
               "remove params from function signature types where possible",
               createSignaturePruningPass);
  registerPass("signature-refining",
               "apply more specific subtypes to signature types where possible",
               createSignatureRefiningPass);
  registerPass("signext-lowering",
               "lower sign-ext operations to wasm mvp",
               createSignExtLoweringPass);
  registerPass("simplify-globals",
               "miscellaneous globals-related optimizations",
               createSimplifyGlobalsPass);
  registerPass("simplify-globals-optimizing",
               "miscellaneous globals-related optimizations, and optimizes "
               "where we replaced global.gets with constants",
               createSimplifyGlobalsOptimizingPass);
  registerPass("simplify-locals",
               "miscellaneous locals-related optimizations",
               createSimplifyLocalsPass);
  registerPass("simplify-locals-nonesting",
               "miscellaneous locals-related optimizations (no nesting at all; "
               "preserves flatness)",
               createSimplifyLocalsNoNestingPass);
  registerPass("simplify-locals-notee",
               "miscellaneous locals-related optimizations (no tees)",
               createSimplifyLocalsNoTeePass);
  registerPass("simplify-locals-nostructure",
               "miscellaneous locals-related optimizations (no structure)",
               createSimplifyLocalsNoStructurePass);
  registerPass(
    "simplify-locals-notee-nostructure",
    "miscellaneous locals-related optimizations (no tees or structure)",
    createSimplifyLocalsNoTeeNoStructurePass);
  registerPass("souperify", "emit Souper IR in text form", createSouperifyPass);
  registerPass("souperify-single-use",
               "emit Souper IR in text form (single-use nodes only)",
               createSouperifySingleUsePass);
  registerPass("spill-pointers",
               "spill pointers to the C stack (useful for Boehm-style GC)",
               createSpillPointersPass);
  registerPass("stub-unsupported-js",
               "stub out unsupported JS operations",
               createStubUnsupportedJSOpsPass);
  registerPass("ssa",
               "ssa-ify variables so that they have a single assignment",
               createSSAifyPass);
  registerPass(
    "ssa-nomerge",
    "ssa-ify variables so that they have a single assignment, ignoring merges",
    createSSAifyNoMergePass);
  registerPass(
    "strip", "deprecated; same as strip-debug", createStripDebugPass);
  registerPass("stack-check",
               "enforce limits on llvm's __stack_pointer global",
               createStackCheckPass);
  registerPass("strip-debug",
               "strip debug info (including the names section)",
               createStripDebugPass);
  registerPass("strip-dwarf", "strip dwarf debug info", createStripDWARFPass);
  registerPass("strip-producers",
               "strip the wasm producers section",
               createStripProducersPass);
  registerPass("strip-target-features",
               "strip the wasm target features section",
               createStripTargetFeaturesPass);
  registerPass("trap-mode-clamp",
               "replace trapping operations with clamping semantics",
               createTrapModeClamp);
  registerPass("trap-mode-js",
          