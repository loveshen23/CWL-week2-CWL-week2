/*
 * Copyright 2020 WebAssembly Community Group participants
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

// wasm-split: Split a module in two or instrument a module to inform future
// splitting.

#include <fstream>

#include "ir/module-splitting.h"
#include "ir/names.h"
#include "support/file.h"
#include "support/name.h"
#include "support/path.h"
#include "support/utilities.h"
#include "wasm-binary.h"
#include "wasm-builder.h"
#include "wasm-io.h"
#include "wasm-validator.h"

#include "instrumenter.h"
#include "split-options.h"

using namespace wasm;

namespace {

void parseInput(Module& wasm, const WasmSplitOptions& options) {
  options.applyFeatures(wasm);
  ModuleReader reader;
  reader.setProfile(options.profile);
  try {
    reader.read(options.inputFiles[0], wasm);
  } catch (ParseException& p) {
    p.dump(std::cerr);
    std::cerr << '\n';
    Fatal() << "error parsing wasm";
  } catch (std::bad_alloc&) {
    Fatal() << "error building module, std::bad_alloc (possibly invalid "
               "request for silly amounts of memory)";
  }

  if (options.passOptions.validate && !WasmValidator().validate(wasm)) {
    Fatal() << "error validating input";
  }
}

uint64_t hashFile(const std::string& filename) {
  auto contents(read_file<std::vector<char>>(filename, Flags::Binary));
  size_t digest = 0;
  // Don't use `hash` or `rehash` - they aren't deterministic between executions
  for (char c : contents) {
    hash_combine(digest, c);
  }
  return uint64_t(digest);
}

void adjustTableSize(Module& wasm, int initialSize) {
  if (initialSize < 0) {
    return;
  }
  if (wasm.tables.empty()) {
    Fatal() << "--initial-table used but there is no table";
  }

  auto& table = wasm.tables.front();

  if ((uint64_t)initialSize < table->initial) {
    Fatal() << "Specified initial table size too small, should be at least "
            << table->initial;
  }
  if ((uint64_t)initialSize > table->max) {
    Fatal() << "Specified initial table size larger than max table size "
            << table->max;
  }
  table->initial = initialSize;
}

void writeModule(Module& wasm,
                 std::string filename,
                 const WasmSplitOptions& options) {
  ModuleWriter writer;
  writer.setBinary(options.emitBinary);
  writer.setDebugInfo(options.passOptions.debugInfo);
  if (options.emitModuleNames) {
    writer.setEmitModuleName(true);
  }
  writer.write(wasm, filename);
}

void instrumentModule(const WasmSplitOptions& options) {
  Module wasm;
  parseInput(wasm, options);

  // Check that the profile export name is not already taken
  if (wasm.getExportOrNull(options.profileExport) != nullptr) {
    Fatal() << "error: Export " << options.profileExport << " already exists.";
  }

  uint64_t moduleHash = hashFile(options.inputFiles[0]);
  InstrumenterConfig config;
  if (options.importNamespace.size()) {
    config.importNamespace = options.importNamespace;
  }
  if (options.secondaryMemoryName.size()) {
    config.secondaryMemoryName = options.secondaryMemoryName;
  }
  config.storageKind = options.storageKind;
  config.profileExport = options.profileExport;

  PassRunner runner(&wasm, options.passOptions);
  runner.add(std::make_unique<Instrumenter>(config, moduleHash));
  runner.run();

  adjustTableSize(wasm, options.initialTableSize);

  // Write the output modules
  writeModule(wasm, options.output, options);
}

struct ProfileData {
  uint64_t hash;
  std::vector<size_t> timestamps;
};

// See "wasm-split profile format" in instrumenter.cpp for more information.
ProfileData readProfile(const std::string& file) {
  auto profileData = read_file<std::vector<char>>(file, Flags::Binary);
  size_t i = 0;
  auto readi32 = [&]() {
    if (i + 4 > profileData.size()) {
      Fatal() << "Unexpected end of profile data in " << file;
    }
    uint32_t i32 = 0;
    i32 |= uint32_t(uint8_t(profileData[i++]));
    i32 |= uint32_t(uint8_t(profileData[i++])) << 8;
    i32 |= uint32_t(uint8_t(profileData[i++])) << 16;
    i32 |= uint32_t(uint8_t(profileData[i++])) << 24;
    return i32;
  };

  uint64_t hash = readi32();
  hash |= uint64_t(readi32()) << 32;

  std::vector<size_t> timestamps;
  while (i < profileData.size()) {
    timestamps.push_back(readi32());
  }

  return {hash, timestamps};
}

void getFunctionsToKeepAndSplit(Module& wasm,
                                uint64_t wasmHash,
                                const std::string& profileFile,
                                std::set<Name>& keepFuncs,
                                std::set<Name>& splitFuncs) {
  ProfileData profile = readProfile(profileFile);
  if (profile.hash != wasmHash) {
    Fatal() << "error: checksum in profile does not match module checksum. "
            << "The module to split must be the original, uninstrumented "
               "module, not the module used to generate the profile.";
  }

  size_t i = 0;
  ModuleUtils::iterDefinedFunctions(wasm, [&](Function* func) {
    if (i >= profile.timestamps.size()) {
      Fatal() << "Unexpected end of profile data";
    }
    if (profile.timestamps[i++] > 0) {
      keepFuncs.insert(func->name);
    } else {
      splitFuncs.insert(func->name);
    }
  });
  if (i != profile.timestamps.size()) {
    Fatal() << "Unexpected extra profile data";
  }
}

void writeSymbolMap(Module& wasm, std::string filename) {
  PassOptions options;
  options.arguments["symbolmap"] = filename;
  PassRunner runner(&wasm, options);
  runner.add("symbolmap");
  runner.run();
}

void writePlaceholderMap(const std::map<size_t, Name> placeholderMap,
                         std::string filename) {
  Output out