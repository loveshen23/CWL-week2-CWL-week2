//===- MCSymbolWasm.h -  ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_MC_MCSYMBOLWASM_H
#define LLVM_MC_MCSYMBOLWASM_H

#include "llvm/BinaryFormat/Wasm.h"
#include "llvm/MC/MCSymbol.h"

namespace llvm {

class MCSymbolWasm : public MCSymbol {
  wasm::WasmSymbolType Type = wasm::WASM_SYMBOL_TYPE_DATA;
  bool IsWeak = false;
  bool IsHidden = false;
  bool IsComdat = false;
  mutable bool IsUsedInGOT = false;
  Optional<std::string> ImportModule;
  Optional<std::string> ImportName;
  wasm::WasmSignature *Signature = nullptr;
  Optional<wasm::WasmGlobalType> GlobalType;
  Optional<wasm::WasmEventType> EventType;

  /// An expression descr