/*
 * Copyright 2021 WebAssembly Community Group participants
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

#include "split-options.h"
#include <fstream>

namespace wasm {

namespace {

std::set<Name> parseNameListFromLine(const std::string& line) {
  std::set<Name> names;
  std::istringstream stream(line);
  for (std::string name; std::getline(stream, name, ',');) {
    names.insert(name);
  }
  return names;
}

std::set<Name> parseNameListFromFile(const std::string& filename) {
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    std::cerr << "Failed openin