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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <ostream>
#include <string_view>
#include <variant>

#ifndef wasm_wat_lexer_h
#define wasm_wat_lexer_h

namespace wasm::WATParser {

struct TextPos {
  size_t line;
  size_t col;

  bool operator==(const TextPos& other) const;
  bool operator!=(const TextPos& other) const { return !(*this == other); }

  friend std::ostream& operator<<(std::ostream& os, const TextPos& pos);
};

// ======
// Tokens
// ======

struct LParenTok {
  bool operator==(const LParenTok&) const { return true; }
  friend std::ostream& operator<<(std::ostream&, const LParenTok&);
};

struct RParenTok {
  bool operator==(const RParenTok&) const { return true; }
  friend std::ostream& operator<<(std::ostream&, const RParenTok&);
};

struct IdTok {
  bool operator==(const IdTok&) const { return true; }
  friend std::ostream& operator<<(std::ostream&, const IdTok&);
};

enum Sign { NoSign, Pos, Neg };

struct IntTok {
  uint64_t n;
  Sign sign;

  bool operator==(const IntTok&) const