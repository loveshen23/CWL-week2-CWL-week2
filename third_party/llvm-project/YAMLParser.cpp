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
  /// of the token in the input.
  StringRef Range;

  /// The value of a block scalar node.
  std::string Value;

  Token() = default;
};

} // end namespace yaml
} // end namespace llvm

using TokenQueueT = BumpPtrList<Token>;

namespace {

/// This struct is used to track simple keys.
///
/// Simple keys are handled by creating an entry in SimpleKeys for each Token
/// which could legally be the start of a simple key. When peekNext is called,
/// if the Token To be returned is referenced by a SimpleKey, we continue
/// tokenizing until that potential simple key has either been found to not be
/// a simple key (we moved on to the next line or went further than 1024 chars).
/// Or when we run into a Value, and then insert a Key token (and possibly
/// others) before the SimpleKey's Tok.
struct SimpleKey {
  TokenQueueT::iterator Tok;
  unsigned Column = 0;
  unsigned Line = 0;
  unsigned FlowLevel = 0;
  bool IsRequired = false;

  bool operator ==(const SimpleKey &Other) {
    return Tok == Other.Tok;
  }
};

} // end anonymous namespace

/// The Unicode scalar value of a UTF-8 minimal well-formed code unit
///        subsequence and the subsequence's length in code units (uint8_t).
///        A length of 0 represents an error.
using UTF8Decoded = std::pair<uint32_t, unsigned>;

static UTF8Decoded decodeUTF8(StringRef Range) {
  StringRef::iterator Position= Range.begin();
  StringRef::iterator End = Range.end();
  // 1 byte: [0x00, 0x7f]
  // Bit pattern: 0xxxxxxx
  if ((*Position & 0x80) == 0) {
     return std::make_pair(*Position, 1);
  }
  // 2 bytes: [0x80, 0x7ff]
  // Bit pattern: 110xxxxx 10xxxxxx
  if (Position + 1 != End &&
      ((*Position & 0xE0) == 0xC0) &&
      ((*(Position + 1) & 0xC0) == 0x80)) {
    uint32_t codepoint = ((*Position & 0x1F) << 6) |
                          (*(Position + 1) & 0x3F);
    if (codepoint >= 0x80)
      return std::make_pair(codepoint, 2);
  }
  // 3 bytes: [0x8000, 0xffff]
  // Bit pattern: 1110xxxx 10xxxxxx 10xxxxxx
  if (Position + 2 != End &&
      ((*Position & 0xF0) == 0xE0) &&
      ((*(Position + 1) & 0xC0) == 0x80) &&
      ((*(Position + 2) & 0xC0) == 0x80)) {
    uint32_t codepoint = ((*Position & 0x0F) << 12) |
                         ((*(Position + 1) & 0x3F) << 6) |
                          (*(Position + 2) & 0x3F);
    // Codepoints between 0xD800 and 0xDFFF are invalid, as
    // they are high / low surrogate halves used by UTF-16.
    if (codepoint >= 0x800 &&
        (codepoint < 0xD800 || codepoint > 0xDFFF))
      return std::make_pair(codepoint, 3);
  }
  // 4 bytes: [0x10000, 0x10FFFF]
  // Bit pattern: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  if (Position + 3 != End &&
      ((*Position & 0xF8) == 0xF0) &&
      ((*(Position + 1) & 0xC0) == 0x80) &&
      ((*(Position + 2) & 0xC0) == 0x80) &&
      ((*(Position + 3) & 0xC0) == 0x80)) {
    uint32_t codepoint = ((*Position & 0x07) << 18) |
                         ((*(Position + 1) & 0x3F) << 12) |
                         ((*(Position + 2) & 0x3F) << 6) |
                          (*(Position + 3) & 0x3F);
    if (codepoint >= 0x10000 && codepoint <= 0x10FFFF)
      return std::make_pair(codepoint, 4);
  }
  return std::make_pair(0, 0);
}

namespace llvm {
namespace yaml {

/// Scans YAML tokens from a MemoryBuffer.
class Scanner {
public:
  Scanner(StringRef Input, SourceMgr &SM, bool ShowColors = true,
          std::error_code *EC = nullptr);
  Scanner(MemoryBufferRef Buffer, SourceMgr &SM_, bool ShowColors = true,
          std::error_code *EC = nullptr);

  /// Parse the next token and return it without popping it.
  Token &peekNext();

  /// Parse the next token and pop it from the queue.
  Token getNext();

  void printError(SMLoc Loc, SourceMgr::DiagKind Kind, const Twine &Message,
                  ArrayRef<SMRange> Ranges = None) {
    SM.PrintMessage(Loc, Kind, Message, Ranges, /* FixIts= */ None, ShowColors);
  }

  void setError(const Twine &Message, StringRef::iterator Position) {
    if (Current >= End)
      Current = End - 1;

    // propagate the error if possible
    if (EC)
      *EC = make_error_code(std::errc::invalid_argument);

    // Don't print out more errors after the first one we encounter. The rest
    // are just the result of the first, and have no meaning.
    if (!Failed)
      printError(SMLoc::getFromPointer(Current), SourceMgr::DK_Error, Message);
    Failed = true;
  }

  void setError(const Twine &Message) {
    setError(Message, Current);
  }

  /// Returns true if an error occurred while parsing.
  bool failed() {
    return Failed;
  }

private:
  void init(MemoryBufferRef Buffer);

  StringRef currentInput() {
    return StringRef(Current, End - Current);
  }

  /// Decode a UTF-8 minimal well-formed code unit subsequence starting
  ///        at \a Position.
  ///
  /// If the UTF-8 code units starting at Position do not form a well-formed
  /// code unit subsequence, then the Unicode scalar value is 0, and the length
  /// is 0.
  UTF8Decoded decodeUTF8(StringRef::iterator Position) {
    return ::decodeUTF8(StringRef(Position, End - Position));
  }

  // The following functions are based on the gramar rules in the YAML spec. The
  // style of the function names it meant to closely match how they are written
  // in the spec. The number within the [] is the number of the grammar rule in
  // the spec.
  //
  // See 4.2 [Production Naming Conventions] for the meaning of the prefixes.
  //
  // c-
  //   A production starting and ending with a special character.
  // b-
  //   A production matching a single line break.
  // nb-
  //   A production starting and ending with a non-break character.
  // s-
  //   A production starting and ending with a white space character.
  // ns-
  //   A production starting and ending with a non-space character.
  // l-
  //   A production matching complete line(s).

  /// Skip a single nb-char[27] starting at Position.
  ///
  /// A nb-char is 0x9 | [0x20-0x7E] | 0x85 | [0xA0-0xD7FF] | [0xE000-0xFEFE]
  ///                  | [0xFF00-0xFFFD] | [0x10000-0x10FFFF]
  ///
  /// @returns The code unit after the nb-char, or Position if it's not an
  ///          nb-char.
  StringRef::iterator skip_nb_char(StringRef::iterator Position);

  /// Skip a single b-break[28] starting at Position.
  ///
  /// A b-break is 0xD 0xA | 0xD | 0xA
  ///
  /// @returns The code unit after the b-break, or Position if it's not a
  ///          b-break.
  StringRef::iterator skip_b_break(StringRef::iterator Position);

  /// Skip a single s-space[31] starting at Position.
  ///
  /// An s-space is 0x20
  ///
  /// @returns The code unit after the s-space, or Position if it's not a
  ///          s-space.
  StringRef::iterator skip_s_space(StringRef::iterator Position);

  /// Skip a single s-white[33] starting at Position.
  ///
  /// A s-white is 0x20 | 0x9
  ///
  /// @returns The code unit after the s-white, or Position if it's not a
  ///          s-white.
  StringRef::iterator skip_s_white(StringRef::iterator Position);

  /// Skip a single ns-char[34] starting at Position.
  ///
  /// A ns-char is nb-char - s-white
  ///
  /// @returns The code unit after the ns-char, or Position if it's not a
  ///          ns-char.
  StringRef::iterator skip_ns_char(StringRef::iterator Position);

  using SkipWhileFunc = StringRef::iterator (Scanner::*)(StringRef::iterator);

  /// Skip minimal well-formed code unit subsequences until Func
  ///        returns its input.
  ///
  /// @returns The code unit after the last minimal well-formed code unit
  ///          subsequence that Func accepted.
  StringRef::iterator skip_while( SkipWhileFunc Func
                                , StringRef::iterator Position);

  /// Skip minimal well-formed code unit subsequences until Func returns its
  /// input.
  void advanceWhile(SkipWhileFunc Func);

  /// Scan ns-uri-char[39]s starting at Cur.
  ///
  /// This updates Cur and Column while scanning.
  void scan_ns_uri_char();

  /// Consume a minimal well-formed code unit subsequence starting at
  ///        \a Cur. Return false if it is not the same Unicode scalar value as
  ///        \a Expected. This updates \a Column.
  bool consume(uint32_t Expected);

  /// Skip \a Distance UTF-8 code units. Updates \a Cur and \a Column.
  void skip(uint32_t Distance);

  /// Return true if the minimal well-formed code unit subsequence at
  ///        Pos is whitespace or a new line
  bool isBlankOrBreak(StringRef::iterator Position);

  /// Consume a single b-break[28] if it's present at the current position.
  ///
  /// Return false if the code unit at the current position isn't a line break.
  bool consumeLineBreakIfPresent();

  /// If IsSimpleKeyAllowed, create and push_back a new SimpleKey.
  void saveSimpleKeyCandidate( TokenQueueT::iterator Tok
                             , unsigned AtColumn
                             , bool IsRequired);

  /// Remove simple keys that can no longer be valid simple keys.
  ///
  /// Invalid simple keys are not on the current line or are further than 1024
  /// columns back.
  void removeStaleSimpleKeyCandidates();

  /// Remove all simple keys on FlowLevel \a Level.
  void removeSimpleKeyCandidatesOnFlowLevel(unsigned Level);

  /// Unroll indentation in \a Indents back to \a Col. Creates BlockEnd
  ///        tokens if needed.
  bool unrollIndent(int ToColumn);

  /// Increase indent to \a Col. Creates \a Kind token at \a InsertPoint
  ///        if needed.
  bool rollIndent( int ToColumn
                 , Token::TokenKind Kind
                 , TokenQueueT::iterator InsertPoint);

  /// Skip a single-line comment when the comment starts at the current
  /// position of the scanner.
  void skipComment();

  /// Skip whitespace and comments until the start of the next token.
  void scanToNextToken();

  /// Must be the first token generated.
  bool scanStreamStart();

  /// Generate tokens needed to close out the stream.
  bool scanStreamEnd();

  /// Scan a %BLAH directive.
  bool scanDirective();

  /// Scan a ... or ---.
  bool scanDocumentIndicator(bool IsStart);

  /// Scan a [ or { and generate the proper flow collection start token.
  bool scanFlowCollectionStart(bool IsSequence);

  /// Scan a ] or } and generate the proper flow collection end token.
  bool scanFlowCollectionEnd(bool IsSequence);

  /// Scan the , that separates entries in a flow collection.
  bool scanFlowEntry();

  /// Scan the - that starts block sequence entries.
  bool scanBlockEntry();

  /// Scan an explicit ? indicating a key.
  bool scanKey();

  /// Scan an explicit : indicating a value.
  bool scanValue();

  /// Scan a quoted scalar.
  bool scanFlowScalar(bool IsDoubleQuoted);

  /// Scan an unquoted scalar.
  bool scanPlainScalar();

  /// Scan an Alias or Anchor starting with * or &.
  bool scanAliasOrAnchor(bool IsAlias);

  /// Scan a block scalar starting with | or >.
  bool scanBlockScalar(bool IsLiteral);

  /// Scan a chomping indicator in a block scalar header.
  char scanBlockChompingIndicator();

  /// Scan an indentation indicator in a block scalar header.
  unsigned scanBlockIndentationIndicator();

  /// Scan a block scalar header.
  ///
  /// Return false if an error occurred.
  bool scanBlockScalarHeader(char &ChompingIndicator, unsigned &IndentIndicator,
                             bool &IsDone);

  /// Look for the indentation level of a block scalar.
  ///
  /// Return false if an error occurred.
  bool findBlockScalarIndent(unsigned &BlockIndent, unsigned BlockExitIndent,
                             unsigned &LineBreaks, bool &IsDone);

  /// Scan the indentation of a text line in a block scalar.
  ///
  /// Return false if an error occurred.
  bool scanBlockScalarIndent(unsigned BlockIndent, unsigned BlockExitIndent,
                             bool &IsDone);

  /// Scan a tag of the form !stuff.
  bool scanTag();

  /// Dispatch to the next scanning function based on \a *Cur.
  bool fetchMoreTokens();

  /// The SourceMgr used for diagnostics and buffer management.
  SourceMgr &SM;

  /// The original input.
  MemoryBufferRef InputBuffer;

  /// The current position of the scanner.
  StringRef::iterator Current;

  /// The end of the input (one past the last character).
  StringRef::iterator End;

  /// Current YAML indentation level in spaces.
  int Indent;

  /// Current column number in Unicode code points.
  unsigned Column;

  /// Current line number.
  unsigned Line;

  /// How deep we are in flow style containers. 0 Means at block level.
  unsigned FlowLevel;

  /// Are we at the start of the stream?
  bool IsStartOfStream;

  /// Can the next token be the start of a simple key?
  bool IsSimpleKeyAllowed;

  /// True if an error has occurred.
  bool Failed;

  /// Should colors be used when printing out the diagnostic messages?
  bool ShowColors;

  /// Queue of tokens. This is required to queue up tokens while looking
  ///        for the end of a simple key. And for cases where a single character
  ///        can produce multiple tokens (e.g. BlockEnd).
  TokenQueueT TokenQueue;

  /// Indentation levels.
  SmallVector<int, 4> Indents;

  /// Potential simple keys.
  SmallVector<SimpleKey, 4> SimpleKeys;

  std::error_code *EC;
};

} // end namespace yaml
} // end namespace llvm

/// encodeUTF8 - Encode \a UnicodeScalarValue in UTF-8 and append it to result.
static void encodeUTF8( uint32_t UnicodeScalarValue
                      , SmallVectorImpl<char> &Result) {
  if (UnicodeScalarValue <= 0x7F) {
    Result.push_back(UnicodeScalarValue & 0x7F);
  } else if (UnicodeScalarValue <= 0x7FF) {
    uint8_t FirstByte = 0xC0 | ((UnicodeScalarValue & 0x7C0) >> 6);
    uint8_t SecondByte = 0x80 | (UnicodeScalarValue & 0x3F);
    Result.push_back(FirstByte);
    Result.push_back(SecondByte);
  } else if (UnicodeScalarValue <= 0xFFFF) {
    uint8_t FirstByte = 0xE0 | ((UnicodeScalarValue & 0xF000) >> 12);
    uint8_t SecondByte = 0x80 | ((UnicodeScalarValue & 0xFC0) >> 6);
    uint8_t ThirdByte = 0x80 | (UnicodeScalarValue & 0x3F);
    Result.push_back(FirstByte);
    Result.push_back(SecondByte);
    Result.push_back(ThirdByte);
  } else if (UnicodeScalarValue <= 0x10FFFF) {
    uint8_t FirstByte = 0xF0 | ((UnicodeScalarValue & 0x1F0000) >> 18);
    uint8_t SecondByte = 0x80 | ((UnicodeScalarValue & 0x3F000) >> 12);
    uint8_t ThirdByte = 0x80 | ((UnicodeScalarValue & 0xFC0) >> 6);
    uint8_t FourthByte = 0x80 | (UnicodeScalarValue & 0x3F);
    Result.push_back(FirstByte);
    Result.push_back(SecondByte);
    Result.push_back(ThirdByte);
    Result.push_back(FourthByte);
  }
}

bool yaml::dumpTokens(StringRef Input, raw_ostream &OS) {
  SourceMgr SM;
  Scanner scanner(Input, SM);
  while (true) {
    Token T = scanner.getNext();
    switch (T.Kind) {
    case Token::TK_StreamStart:
      OS << "Stream-Start: ";
      break;
    case Token::TK_StreamEnd:
      OS << "Stream-End: ";
      break;
    case Token::TK_VersionDirective:
      OS << "Version-Directive: ";
      break;
    case Token::TK_TagDirective:
      OS << "Tag-Directive: ";
      break;
    case Token::TK_DocumentStart:
      OS << "Document-Start: ";
      break;
    case Token::TK_DocumentEnd:
      OS << "Document-End: ";
      break;
    case Token::TK_BlockEntry:
      OS << "Block-Entry: ";
      break;
    case Token::TK_BlockEnd:
      OS << "Block-End: ";
      break;
    case Token::TK_BlockSequenceStart:
      OS << "Block-Sequence-Start: ";
      break;
    case Token::TK_BlockMappingStart:
      OS << "Block-Mapping-Start: ";
      break;
    case Token::TK_FlowEntry:
      OS << "Flow-Entry: ";
      break;
    case Token::TK_FlowSequenceStart:
      OS << "Flow-Sequence-Start: ";
      break;
    case Token::TK_FlowSequenceEnd:
      OS << "Flow-Sequence-End: ";
      break;
    case Token::TK_FlowMappingStart:
      OS << "Flow-Mapping-Start: ";
      break;
    case Token::TK_FlowMappingEnd:
      OS << "Flow-Mapping-End: ";
      break;
    case Token::TK_Key:
      OS << "Key: ";
      break;
    case Token::TK_Value:
      OS << "Value: ";
      break;
    case Token::TK_Scalar:
      OS << "Scalar: ";
      break;
    case Token::TK_BlockScalar:
      OS << "Block Scalar: ";
      break;
    case Token::TK_Alias:
      OS << "Alias: ";
      break;
    case Token::TK_Anchor:
      OS << "Anchor: ";
      break;
    case Token::TK_Tag:
      OS << "Tag: ";
      break;
    case Token::TK_Error:
      break;
    }
    OS << T.Range << "\n";
    if (T.Kind == Token::TK_StreamEnd)
      break;
    else if (T.Kind == Token::TK_Error)
      return false;
  }
  return true;
}

bool yaml::scanTokens(StringRef Input) {
  SourceMgr SM;
  Scanner scanner(Input, SM);
  while (true) {
    Token T = scanner.getNext();
    if (T.Kind == Token::TK_StreamEnd)
      break;
    else if (T.Kind == Token::TK_Error)
      return false;
  }
  return true;
}

std::string yaml::escape(StringRef Input, bool EscapePrintable) {
  llvm_unreachable("BYN yaml::escape");
}

Scanner::Scanner(StringRef Input, SourceMgr &sm, bool ShowColors,
                 std::error_code *EC)
    : SM(sm), ShowColors(ShowColors), EC(EC) {
  init(MemoryBufferRef(Input, "YAML"));
}

Scanner::Scanner(MemoryBufferRef Buffer, SourceMgr &SM_, bool ShowColors,
                 std::error_code *EC)
    : SM(SM_), ShowColors(ShowColors), EC(EC) {
  init(Buffer);
}

void Scanner::init(MemoryBufferRef Buffer) {
  InputBuffer = Buffer;
  Current = InputBuffer.getBufferStart();
  End = InputBuffer.getBufferEnd();
  Indent = -1;
  Column = 0;
  Line = 0;
  FlowLevel = 0;
  IsStartOfStream = true;
  IsSimpleKeyAllowed = true;
  Failed = false;
  std::unique_ptr<MemoryBuffer> InputBufferOwner =
      MemoryBuffer::getMemBuffer(Buffer);
  SM.AddNewSourceBuffer(std::move(InputBufferOwner), SMLoc());
}

Token &Scanner::peekNext() {
  // If the current token is a possible simple key, keep parsing until we
  // can confirm.
  bool NeedMore = false;
  while (true) {
    if (TokenQueue.empty() || NeedMore) {
      if (!fetchMoreTokens()) {
        TokenQueue.clear();
        SimpleKeys.clear();
        TokenQueue.push_back(Token());
        return TokenQueue.front();
      }
    }
    assert(!TokenQueue.empty() &&
            "fetchMoreTokens lied about getting tokens!");

    removeStaleSimpleKeyCandidates();
    SimpleKey SK;
    SK.Tok = TokenQueue.begin();
    if (!is_contained(SimpleKeys, SK))
      break;
    else
      NeedMore = true;
  }
  return TokenQueue.front();
}

Token Scanner::getNext() {
  Token Ret = peekNext();
  // TokenQueue can be empty if there was an error getting the next token.
  if (!TokenQueue.empty())
    TokenQueue.pop_front();

  // There cannot be any referenced Token's if the TokenQueue is empty. So do a
  // quick deallocation of them all.
  if (TokenQueue.empty())
    TokenQueue.resetAlloc();

  return Ret;
}

StringRef::iterator Scanner::skip_nb_char(StringRef::iterator Position) {
  if (Position == End)
    return Position;
  // Check 7 bit c-printable - b-char.
  if (   *Position == 0x09
      || (*Position >= 0x20 && *Position <= 0x7E))
    return Position + 1;

  // Check for valid UTF-8.
  if (uint8_t(*Position) & 0x80) {
    UTF8Decoded u8d = decodeUTF8(Position);
    if (   u8d.second != 0
        && u8d.first != 0xFEFF
        && ( u8d.first == 0x85
          || ( u8d.first >= 0xA0
            && u8d.first <= 0xD7FF)
          || ( u8d.first >= 0xE000
            && u8d.first <= 0xFFFD)
          || ( u8d.first >= 0x10000
            && u8d.first <= 0x10FFFF)))
      return Position + u8d.second;
  }
  return Position;
}

StringRef::iterator Scanner::skip_b_break(StringRef::iterator Position) {
  if (Position == End)
    return Position;
  if (*Position == 0x0D) {
    if (Position + 1 != End && *(Position + 1) == 0x0A)
      return Position + 2;
    return Position + 1;
  }

  if (*Position == 0x0A)
    return Position + 1;
  return Position;
}

StringRef::iterator Scanner::skip_s_space(StringRef::iterator Position) {
  if (Position == End)
    return Position;
  if (*Position == ' ')
    return Position + 1;
  return Position;
}

StringRef::iterator Scanner::skip_s_white(StringRef::iterator Position) {
  if (Position == End)
    return Position;
  if (*Position == ' ' || *Position == '\t')
    return Position + 1;
  return Position;
}

StringRef::iterator Scanner::skip_ns_char(StringRef::iterator Position) {
  if (Position == End)
    return Position;
  if (*Position == ' ' || *Position == '\t')
    return Position;
  return skip_nb_char(Position);
}

StringRef::iterator Scanner::skip_while( SkipWhileFunc Func
                                       , StringRef::iterator Position) {
  while (true) {
    StringRef::iterator i = (this->*Func)(Position);
    if (i == Position)
      break;
    Position = i;
  }
  return Position;
}

void Scanner::advanceWhile(SkipWhileFunc Func) {
  auto Final = skip_while(Func, Current);
  Column += Final - Current;
  Current = Final;
}

static bool is_ns_hex_digit(const char C) {
  return    (C >= '0' && C <= '9')
         || (C >= 'a' && C <= 'z')
         || (C >= 'A' && C <= 'Z');
}

static bool is_ns_word_char(const char C) {
  return    C == '-'
         || (C >= 'a' && C <= 'z')
         || (C >= 'A' && C <= 'Z');
}

void Scanner::scan_ns_uri_char() {
  while (true) {
    if (Current == End)
      break;
    if ((   *Current == '%'
          && Current + 2 < End
          && is_ns_hex_digit(*(Current + 1))
          && is_ns_hex_digit(*(Current + 2)))
        || is_ns_word_char(*Current)
        || StringRef(Current, 1).find_first_of("#;/?:@&=+$,_.!~*'()[]")
          != StringRef::npos) {
      ++Current;
      ++Column;
    } else
      break;
  }
}

bool Scanner::consume(uint32_t Expected) {
  if (Expected >= 0x80) {
    setError("Cannot consume non-ascii characters");
    return false;
  }
  if (Current == End)
    return false;
  if (uint8_t(*Current) >= 0x80) {
    setError("Cannot consume non-ascii characters");
    return false;
  }
  if (uint8_t(*Current) == Expected) {
    ++Current;
    ++Column;
    return true;
  }
  return false;
}

void Scanner::skip(uint32_t Distance) {
  Current += Distance;
  Column += Distance;
  assert(Current <= End && "Skipped past the end");
}

bool Scanner::isBlankOrBreak(StringRef::iterator Position) {
  if (Position == End)
    return false;
  return *Position == ' ' || *Position == '\t' || *Position == '\r' ||
         *Position == '\n';
}

bool Scanner::consumeLineBreakIfPresent() {
  auto Next = skip_b_break(Current);
  if (Next == Current)
    return false;
  Column = 0;
  ++Line;
  Current = Next;
  return true;
}

void Scanner::saveSimpleKeyCandidate( TokenQueueT::iterator Tok
                                    , unsigned AtColumn
                                    , bool IsRequired) {
  if (IsSimpleKeyAllowed) {
    SimpleKey SK;
    SK.Tok = Tok;
    SK.Line = Line;
    SK.Column = AtColumn;
    SK.IsRequired = IsRequired;
    SK.FlowLevel = FlowLevel;
    SimpleKeys.push_back(SK);
  }
}

void Scanner::removeStaleSimpleKeyCandidates() {
  for (SmallVectorImpl<SimpleKey>::iterator i = SimpleKeys.begin();
                                            i != SimpleKeys.end();) {
    if (i->Line != Line || i->Column + 1024 < Column) {
      if (i->IsRequired)
        setError( "Could not find expected : for simple key"
                , i->Tok->Range.begin());
      i = SimpleKeys.erase(i);
    } else
      ++i;
  }
}

void Scanner::removeSimpleKeyCandidatesOnFlowLevel(unsigned Level) {
  if (!SimpleKeys.empty() && (SimpleKeys.end() - 1)->FlowLevel == Level)
    SimpleKeys.pop_back();
}

bool Scanner::unrollIndent(int ToColumn) {
  Token T;
  // Indentation is ignored in flow.
  if (FlowLevel != 0)
    return true;

  while (Indent > ToColumn) {
    T.Kind 