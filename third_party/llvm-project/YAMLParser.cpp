//===- YAMLParser.cpp - Simple YAML parser --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements a YAML parser.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/YAMLParser.h"
#include "llvm/ADT/AllocatorList.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Unicode.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

using namespace llvm;
using namespace yaml;

enum UnicodeEncodingForm {
  UEF_UTF32_LE, ///< UTF-32 Little Endian
  UEF_UTF32_BE, ///< UTF-32 Big Endian
  UEF_UTF16_LE, ///< UTF-16 Little Endian
  UEF_UTF16_BE, ///< UTF-16 Big Endian
  UEF_UTF8,     ///< UTF-8 or ascii.
  UEF_Unknown   ///< Not a valid Unicode encoding.
};

/// EncodingInfo - Holds the encoding type and length of the byte order mark if
///                it exists. Length is in {0, 2, 3, 4}.
using EncodingInfo = std::pair<UnicodeEncodingForm, unsigned>;

/// getUnicodeEncoding - Reads up to the first 4 bytes to determine the Unicode
///                      encoding form of \a Input.
///
/// @param Input A string of length 0 or more.
/// @returns An EncodingInfo indicating the Unicode encoding form of the input
///          and how long the byte order mark is if one exists.
static EncodingInfo getUnicodeEncoding(StringRef Input) {
  if (Input.empty())
    return std::make_pair(UEF_Unknown, 0);

  switch (uint8_t(Input[0])) {
  case 0x00:
    if (Input.size() >= 4) {
      if (  Input[1] == 0
         && uint8_t(Input[2]) == 0xFE
         && uint8_t(Input[3]) == 0xFF)
        return std::make_pair(UEF_UTF32_BE, 4);
      if (Input[1] == 0 && Input[2] == 0 && Input[3] != 0)
        return std::make_pair(UEF_UTF32_BE, 0);
    }

    if (Input.size() >= 2 && Input[1] != 0)
      return std::make_pair(UEF_UTF16_BE, 0);
    return std::make_pair(UEF_Unknown, 0);
  case 0xFF:
    if (  Input.size() >= 4
       && uint8_t(Input[1]) == 0xFE
       && Input[2] == 0
       && Input[3] == 0)
      return std::make_pair(UEF_UTF32_LE, 4);

    if (Input.size() >= 2 && uint8_t(Input[1]) == 0xFE)
      return std::make_pair(UEF_UTF16_LE, 2);
    return std::make_pair(UEF_Unknown, 0);
  case 0xFE:
    if (Input.size() >= 2 && uint8_t(Input[1]) == 0xFF)
      return std::make_pair(UEF_UTF16_BE, 2);
    return std::make_pair(UEF_Unknown, 0);
  case 0xEF:
    if (  Input.size() >= 3
       && uint8_t(Input[1]) == 0xBB
       && uint8_t(Input[2]) == 0xBF)
      return std::make_pair(UEF_UTF8, 3);
    return std::make_pair(UEF_Unknown, 0);
  }

  // It could still be utf-32 or utf-16.
  if (Input.size() >= 4 && Input[1] == 0 && Input[2] == 0 && Input[3] == 0)
    return std::make_pair(UEF_UTF32_LE, 0);

  if (Input.size() >= 2 && Input[1] == 0)
    return std::make_pair(UEF_UTF16_LE, 0);

  return std::make_pair(UEF_UTF8, 0);
}

/// Pin the vtables to this file.
void Node::anchor() {}
void NullNode::anchor() {}
void ScalarNode::anchor() {}
void BlockScalarNode::anchor() {}
void KeyValueNode::anchor() {}
void MappingNode::anchor() {}
void SequenceNode::anchor() {}
void AliasNode::anchor() {}

namespace llvm {
namespace yaml {

/// Token - A single YAML token.
struct Token {
  enum TokenKind {
    TK_Error, // Uninitialized token.
    TK_StreamStart,
    TK_StreamEnd,
    TK_VersionDirective,
    TK_TagDirective,
    TK_DocumentStart,
    TK_DocumentEnd,
    TK_BlockEntry,
    TK_BlockEnd,
    TK_BlockSequenceStart,
    TK_BlockMappingStart,
    TK_FlowEntry,
    TK_FlowSequenceStart,
    TK_FlowSequenceEnd,
    TK_FlowMappingStart,
    TK_FlowMappingEnd,
    TK_Key,
    TK_Value,
    TK_Scalar,
    TK_BlockScalar,
    TK_Alias,
    TK_Anchor,
    TK_Tag
  } Kind = TK_Error;

  /// A string of length 0 or more whose begin() points to the logical location
  /// of the t