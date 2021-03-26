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

#ifndef wasm_passes_h
#define wasm_passes_h

namespace wasm {

class Pass;

// Normal passes:
Pass* createAbstractTypeRefiningPass();
Pass* createAlignmentLoweringPass();
Pass* createAsyncifyPass();
Pass* createAvoidReinterpretsPass();
Pass* createCoalesceLocalsPass();
Pass* createCoalesceLocalsWithLearningPass();
Pass* createCodeFoldingPass();
Pass* createCodePushingPass();
Pass* createConstHoistingPass();
Pass* createConstantFieldPropagationPass();
Pass* createDAEPass();
Pass* createDAEOptimizingPass();
Pass* createDataFlowOptsPass();
Pass* createDeadCodeEliminationPass();
Pass* createDeNaNPass();
Pass* createDeAlignPass();
Pass* createDirectizePass();
Pass* createDiscardGlobalEffectsPass();
Pass* createDWARFDumpPass();
Pass* createDuplicateImportEliminationPass();
Pass* createDuplicateFunctionEliminationPass();
Pass* createEmitTargetFeaturesPass();
Pass* createExtractFunctionPass();
Pass* createE