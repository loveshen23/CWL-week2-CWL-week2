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
               "instrument the build with logging of where exec