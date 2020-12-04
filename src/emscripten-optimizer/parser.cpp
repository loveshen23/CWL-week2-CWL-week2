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

#include <unordered_map>

#include "parser.h"

namespace cashew {

// common strings

IString TOPLEVEL("toplevel");
IString DEFUN("defun");
IString BLOCK("block");
IString VAR("var");
IString CONST("const");
IString CONDITIONAL("conditional");
IString BINARY("binary");
IString RETURN("return");
IString IF("if");
IString ELSE("else");
IString WHILE("while");
IString DO("do");
IString FOR("for");
IString SEQ("seq");
IString SUB("sub");
IString CALL("call");
IString LABEL("label");
IString BREAK("break");
IString CONTINUE("continue");
IString SWITCH("switch");
IString STRING("string");
IString TRY("try");
IString INF("inf");
IString NaN("nan");
IString LLVM_CTTZ_I32("_llvm_cttz_i32");
IString UDIVMODDI4("___udivmoddi4");
IString UNARY_PREFIX("unary-prefix");
IString UNARY_POSTFIX("unary-postfix");
IString MATH_FROUND("Math_fround");
IString MATH_CLZ32("Math_clz32");
IString INT64("i64");
IString INT64_CONST("i64_const");
IString SIMD_FLOAT32X4("SIMD_Float32x4");
IString SIMD_FLOAT64X2("SIMD_Float64x2");
IString SIMD_INT8X16("SIMD_Int8x16");
IString SIMD_INT16X8("SIMD_Int16x8");
IString SIMD_INT32X4("SIMD_Int32x4");
IString PLUS("+");
IString MINUS("-");
IString OR("|");
IString AND("&");
IString XOR("^");
IString L_NOT("!");
IString B_NOT("~");
IString LT("<");
IString GE(">=");
IString LE("<=");
IString GT(">");
IString EQ("==");
IString NE("!=");
IString DIV("/");
IString MOD("%");
IString MUL("*");
IString RSHIFT(">>");
IString LSHIFT("<<");
IString TRSHIFT(">>>");
IString HEAP8("HEAP8");
IString HEAP16("HEAP16");
IString HEAP32("HE