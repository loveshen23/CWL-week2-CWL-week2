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

#include <cmath>

#include "wat-lexer.h"
#include "gtest/gtest.h"

using namespace wasm::WATParser;
using namespace std::string_view_literals;

TEST(LexerTest, LexWhitespace) {
  Token one{"1"sv, IntTok{1, NoSign}};
  Token two{"2"sv, IntTok{2, NoSign}};
  Token three{"3"sv, IntTok{3, NoSign}};
  Token four{"4"sv, IntTok{4, NoSign}};
  Token five{"5"sv, IntTok{5, NoSign}};

  Lexer lexer(" 1\t2\n3\r4 \n\n\t 5 "sv);

  auto it = lexer.begin();
  ASSERT_NE(it, lexer.end());
  Token t1 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t2 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t3 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t4 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t5 = *it++;
  EXPECT_EQ(it, lexer.end());

  EXPECT_EQ(t1, one);
  EXPECT_EQ(t2, two);
  EXPECT_EQ(t3, three);
  EXPECT_EQ(t4, four);
  EXPECT_EQ(t5, five);

  EXPECT_EQ(lexer.position(t1), (TextPos{1, 1}));
  EXPECT_EQ(lexer.position(t2), (TextPos{1, 3}));
  EXPECT_EQ(lexer.position(t3), (TextPos{2, 0}));
  EXPECT_EQ(lexer.position(t4), (TextPos{2, 2}));
  EXPECT_EQ(lexer.position(t5), (TextPos{4, 2}));
}

TEST(LexerTest, LexLineComment) {
  Token one{"1"sv, IntTok{1, NoSign}};
  Token six{"6"sv, IntTok{6, NoSign}};

  Lexer lexer("1;; whee! 2 3\t4\r5\n6"sv);

  auto it = lexer.begin();
  Token t1 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t2 = *it++;
  EXPECT_EQ(it, lexer.end());

  EXPECT_EQ(t1, one);
  EXPECT_EQ(t2, six);

  EXPECT_EQ(lexer.position(t1), (TextPos{1, 0}));
  EXPECT_EQ(lexer.position(t2), (TextPos{2, 0}));
}

TEST(LexerTest, LexBlockComment) {
  Token one{"1"sv, IntTok{1, NoSign}};
  Token six{"6"sv, IntTok{6, NoSign}};

  Lexer lexer("1(; whoo! 2\n (; \n3\n ;) 4 (;) 5 ;) \n;)6"sv);

  auto it = lexer.begin();
  Token t1 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t2 = *it++;
  EXPECT_EQ(it, lexer.end());

  EXPECT_EQ(t1, one);
  EXPECT_EQ(t2, six);

  EXPECT_EQ(lexer.position(t1), (TextPos{1, 0}));
  EXPECT_EQ(lexer.position(t2), (TextPos{5, 2}));
}

TEST(LexerTest, LexParens) {
  Token left{"("sv, LParenTok{}};
  Token right{")"sv, RParenTok{}};

  Lexer lexer("(())"sv);

  auto it = lexer.begin();
  ASSERT_NE(it, lexer.end());
  Token t1 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t2 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t3 = *it++;
  ASSERT_NE(it, lexer.end());
  Token t4 = *it++;
  EXPECT_EQ(it, lexer.end());

  EXPECT_EQ(t1, left);
  EXPECT_EQ(t2, left);
  EXPECT_EQ(t3, right);
  EXPECT_EQ(t4, right);
  EXPECT_TRUE(left.isLParen());
  EXPECT_TRUE(right.isRParen());
}

TEST(LexerTest, LexInt) {
  {
    Lexer lexer("0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0"sv, IntTok{0, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+0"sv, IntTok{0, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-0"sv, IntTok{0, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("1"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"1"sv, IntTok{1, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+1"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+1"sv, IntTok{1, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-1"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-1"sv, IntTok{-1ull, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("0010"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0010"sv, IntTok{10, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+0010"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+0010"sv, IntTok{10, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-0010"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-0010"sv, IntTok{-10ull, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("9999"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"9999"sv, IntTok{9999, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+9999"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+9999"sv, IntTok{9999, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-9999"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-9999"sv, IntTok{-9999ull, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("12_34"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"12_34"sv, IntTok{1234, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("1_2_3_4"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"1_2_3_4"sv, IntTok{1234, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("_1234"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("1234_"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("12__34"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("12cd56"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("18446744073709551615"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"18446744073709551615"sv, IntTok{-1ull, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    // 64-bit unsigned overflow!
    Lexer lexer("18446744073709551616"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"18446744073709551616"sv,
                   FloatTok{{}, 18446744073709551616.}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+9223372036854775807"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+9223372036854775807"sv, IntTok{INT64_MAX, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+9223372036854775808"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+9223372036854775808"sv,
                   IntTok{uint64_t(INT64_MAX) + 1, Pos}};
    ;
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-9223372036854775808"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-9223372036854775808"sv, IntTok{uint64_t(INT64_MIN), Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-9223372036854775809"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-9223372036854775809"sv,
                   IntTok{uint64_t(INT64_MIN) - 1, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
}

TEST(LexerTest, LexHexInt) {
  {
    Lexer lexer("0x0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0x0"sv, IntTok{0, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+0x0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+0x0"sv, IntTok{0, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-0x0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-0x0"sv, IntTok{0, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("0x1"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0x1"sv, IntTok{1, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+0x1"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+0x1"sv, IntTok{1, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-0x1"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-0x1"sv, IntTok{-1ull, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("0x0010"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0x0010"sv, IntTok{16, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+0x0010"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+0x0010"sv, IntTok{16, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-0x0010"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-0x0010"sv, IntTok{-16ull, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("0xabcdef"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0xabcdef"sv, IntTok{0xabcdef, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("+0xABCDEF"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"+0xABCDEF"sv, IntTok{0xabcdef, Pos}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("-0xAbCdEf"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"-0xAbCdEf"sv, IntTok{-0xabcdefull, Neg}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("0x12_34"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0x12_34"sv, IntTok{0x1234, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("0x1_2_3_4"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"0x1_2_3_4"sv, IntTok{0x1234, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("_0x1234"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("0x_1234"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("0x1234_"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("0x12__34"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("0xg"sv);
    EXPECT_TRUE(lexer.empty());
  }
  {
    Lexer lexer("0x120x34"sv);
    EXPECT_TRUE(lexer.empty());
  }
}

TEST(LexerTest, ClassifyInt) {
  {
    Lexer lexer("0"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    ASSERT_TRUE(lexer->getU32());
    ASSERT_TRUE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0ull);
    EXPECT_EQ(*lexer->getS64(), 0ll);
    EXPECT_EQ(*lexer->getI64(), 0ull);
    EXPECT_EQ(*lexer->getU32(), 0u);
    EXPECT_EQ(*lexer->getS32(), 0);
    EXPECT_EQ(*lexer->getI32(), 0u);
    EXPECT_EQ(*lexer->getF64(), 0.0);
    EXPECT_EQ(*lexer->getF32(), 0.0);
    EXPECT_FALSE(std::signbit(*lexer->getF64()));
    EXPECT_FALSE(std::signbit(*lexer->getF32()));
  }
  {
    Lexer lexer("+0"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    ASSERT_TRUE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0ll);
    EXPECT_EQ(*lexer->getI64(), 0ull);
    EXPECT_EQ(*lexer->getS32(), 0);
    EXPECT_EQ(*lexer->getI32(), 0u);
    EXPECT_EQ(*lexer->getF64(), 0.0);
    EXPECT_EQ(*lexer->getF32(), 0.0);
    EXPECT_FALSE(std::signbit(*lexer->getF64()));
    EXPECT_FALSE(std::signbit(*lexer->getF32()));
  }
  {
    Lexer lexer("-0"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    ASSERT_TRUE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0ll);
    EXPECT_EQ(*lexer->getI64(), 0ull);
    EXPECT_EQ(*lexer->getS32(), 0);
    EXPECT_EQ(*lexer->getI32(), 0u);
    EXPECT_EQ(*lexer->getF64(), -0.0);
    EXPECT_EQ(*lexer->getF32(), -0.0);
    ASSERT_TRUE(std::signbit(*lexer->getF64()));
    ASSERT_TRUE(std::signbit(*lexer->getF32()));
  }
  {
    Lexer lexer("0x7fff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    ASSERT_TRUE(lexer->getU32());
    ASSERT_TRUE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0x7fffffffull);
    EXPECT_EQ(*lexer->getS64(), 0x7fffffffll);
    EXPECT_EQ(*lexer->getI64(), 0x7fffffffull);
    EXPECT_EQ(*lexer->getU32(), 0x7fffffffu);
    EXPECT_EQ(*lexer->getS32(), 0x7fffffff);
    EXPECT_EQ(*lexer->getI32(), 0x7fffffffu);
    EXPECT_EQ(*lexer->getF64(), 0x7fffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0x7fffffff.p0f);
  }
  {
    Lexer lexer("0x8000_0000"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    ASSERT_TRUE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0x80000000ull);
    EXPECT_EQ(*lexer->getS64(), 0x80000000ll);
    EXPECT_EQ(*lexer->getI64(), 0x80000000ull);
    EXPECT_EQ(*lexer->getU32(), 0x80000000u);
    EXPECT_EQ(*lexer->getI32(), 0x80000000u);
    EXPECT_EQ(*lexer->getF64(), 0x80000000.p0);
    EXPECT_EQ(*lexer->getF32(), 0x80000000.p0f);
  }
  {
    Lexer lexer("+0x7fff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    ASSERT_TRUE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0x7fffffffll);
    EXPECT_EQ(*lexer->getI64(), 0x7fffffffull);
    EXPECT_EQ(*lexer->getS32(), 0x7fffffff);
    EXPECT_EQ(*lexer->getI32(), 0x7fffffffu);
    EXPECT_EQ(*lexer->getF64(), 0x7fffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0x7fffffff.p0f);
  }
  {
    Lexer lexer("+0x8000_0000"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0x80000000ll);
    EXPECT_EQ(*lexer->getI64(), 0x80000000ull);
    EXPECT_EQ(*lexer->getF64(), 0x80000000.p0);
    EXPECT_EQ(*lexer->getF32(), 0x80000000.p0f);
  }
  {
    Lexer lexer("-0x8000_0000"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    ASSERT_TRUE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), -0x80000000ll);
    EXPECT_EQ(*lexer->getI64(), -0x80000000ull);
    EXPECT_EQ(*lexer->getS32(), -0x7fffffffll - 1);
    EXPECT_EQ(*lexer->getI32(), -0x80000000u);
    EXPECT_EQ(*lexer->getF64(), -0x80000000.p0);
    EXPECT_EQ(*lexer->getF32(), -0x80000000.p0f);
  }
  {
    Lexer lexer("-0x8000_0001"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), -0x80000001ll);
    EXPECT_EQ(*lexer->getI64(), -0x80000001ull);
    EXPECT_EQ(*lexer->getF64(), -0x80000001.p0);
    EXPECT_EQ(*lexer->getF32(), -0x80000001.p0f);
  }
  {
    Lexer lexer("0xffff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    ASSERT_TRUE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    ASSERT_TRUE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0xffffffffull);
    EXPECT_EQ(*lexer->getS64(), 0xffffffffll);
    EXPECT_EQ(*lexer->getI64(), 0xffffffffull);
    EXPECT_EQ(*lexer->getU32(), 0xffffffffu);
    EXPECT_EQ(*lexer->getI32(), 0xffffffffu);
    EXPECT_EQ(*lexer->getF64(), 0xffffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0xffffffff.p0f);
  }
  {
    Lexer lexer("0x1_0000_0000"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0x100000000ull);
    EXPECT_EQ(*lexer->getS64(), 0x100000000ll);
    EXPECT_EQ(*lexer->getI64(), 0x100000000ull);
    EXPECT_EQ(*lexer->getF64(), 0x100000000.p0);
    EXPECT_EQ(*lexer->getF32(), 0x100000000.p0f);
  }
  {
    Lexer lexer("+0xffff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0xffffffffll);
    EXPECT_EQ(*lexer->getI64(), 0xffffffffull);
    EXPECT_EQ(*lexer->getF64(), 0xffffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0xffffffff.p0f);
  }
  {
    Lexer lexer("+0x1_0000_0000"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0x100000000ll);
    EXPECT_EQ(*lexer->getI64(), 0x100000000ull);
    EXPECT_EQ(*lexer->getF64(), 0x100000000.p0);
    EXPECT_EQ(*lexer->getF32(), 0x100000000.p0f);
  }
  {
    Lexer lexer("0x7fff_ffff_ffff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0x7fffffffffffffffull);
    EXPECT_EQ(*lexer->getS64(), 0x7fffffffffffffffll);
    EXPECT_EQ(*lexer->getI64(), 0x7fffffffffffffffull);
    EXPECT_EQ(*lexer->getF64(), 0x7fffffffffffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0x7fffffffffffffff.p0f);
  }
  {
    Lexer lexer("+0x7fff_ffff_ffff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), 0x7fffffffffffffffll);
    EXPECT_EQ(*lexer->getI64(), 0x7fffffffffffffffull);
    EXPECT_EQ(*lexer->getF64(), 0x7fffffffffffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0x7fffffffffffffff.p0f);
  }
  {
    Lexer lexer("-0x8000_0000_0000_0000"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    ASSERT_TRUE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getS64(), -0x7fffffffffffffffll - 1);
    EXPECT_EQ(*lexer->getI64(), -0x8000000000000000ull);
    EXPECT_EQ(*lexer->getF64(), -0x8000000000000000.p0);
    EXPECT_EQ(*lexer->getF32(), -0x8000000000000000.p0f);
  }
  {
    Lexer lexer("0xffff_ffff_ffff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    ASSERT_TRUE(lexer->getU64());
    EXPECT_FALSE(lexer->getS64());
    ASSERT_TRUE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getU64(), 0xffffffffffffffffull);
    EXPECT_EQ(*lexer->getI64(), 0xffffffffffffffffull);
    EXPECT_EQ(*lexer->getF64(), 0xffffffffffffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0xffffffffffffffff.p0f);
  }
  {
    Lexer lexer("+0xffff_ffff_ffff_ffff"sv);
    ASSERT_FALSE(lexer.empty());

    EXPECT_FALSE(lexer->getU64());
    EXPECT_FALSE(lexer->getS64());
    EXPECT_FALSE(lexer->getI64());
    EXPECT_FALSE(lexer->getU32());
    EXPECT_FALSE(lexer->getS32());
    EXPECT_FALSE(lexer->getI32());
    ASSERT_TRUE(lexer->getF64());
    ASSERT_TRUE(lexer->getF32());

    EXPECT_EQ(*lexer->getF64(), 0xffffffffffffffff.p0);
    EXPECT_EQ(*lexer->getF32(), 0xffffffffffffffff.p0f);
  }
}

TEST(LexerTest, LexFloat) {
  {
    Lexer lexer("42"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"42"sv, IntTok{42, NoSign}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("42."sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"42."sv, FloatTok{{}, 42.}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("42.5"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"42.5"sv, FloatTok{{}, 42.5}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexer("42e0"sv);
    ASSERT_FALSE(lexer.empty());
    Token expected{"42e0"sv, FloatTok{{}, 42e0}};
    EXPECT_EQ(*lexer, expected);
  }
  {
    Lexer lexe