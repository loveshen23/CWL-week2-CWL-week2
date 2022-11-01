//===- DWARFContext.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/

#ifndef LLVM_DEBUGINFO_DWARF_DWARFCONTEXT_H
#define LLVM_DEBUGINFO_DWARF_DWARFCONTEXT_H

#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/DebugInfo/DWARF/DWARFAcceleratorTable.h"
#include "llvm/DebugInfo/DWARF/DWARFCompileUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugAbbrev.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugAranges.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugFrame.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugLine.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugLoc.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugMacro.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/DebugInfo/DWARF/DWARFGdbIndex.h"
#include "llvm/DebugInfo/DWARF/DWARFObject.h"
#include "llvm/DebugInfo/DWARF/DWARFSection.h"
#include "llvm/DebugInfo/DWARF/DWARFTypeUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFUnitIndex.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/DataExtractor.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Host.h"
#include <cstdint>
#include <deque>
#include <map>
#include <memory>

namespace llvm {

class MCRegisterInfo;
class MemoryBuffer;
class raw_ostream;

/// Used as a return value for a error callback passed to DWARF context.
/// Callback should return Halt if client application wants to stop
/// object parsing, or should return Continue otherwise.
enum class ErrorPolicy { Halt, Continue };

/// DWARFContext
/// This data structure is the top level entity that deals with dwarf debug
/// information parsing. The actual data is supplied through DWARFObj.
class DWARFContext : public DIContext {
  DWARFUnitVector NormalUnits;
  std::unique_ptr<DWARFUnitIndex> CUIndex;
  std::unique_ptr<DWARFGdbIndex> GdbIndex;
  std::unique_ptr<DWARFUnitIndex> TUIndex;
  std::unique_ptr<DWARFDebugAbbrev> Abbrev;
  std::unique_ptr<DWARFDebugLoc> Loc;
  std::unique_ptr<DWARFDebugAranges> Aranges;
  std::unique_ptr<DWARFDebugLine> Line;
  std::unique_ptr<DWARFDebugFrame> DebugFrame;
  std::unique_ptr<DWARFDebugFrame> EHFrame;
  std::unique_ptr<DWARFDebugMacro> Macro;
  std::unique_ptr<DWARFDebugNames> Names;
  std::unique_ptr<AppleAcceleratorTable> AppleNames;
  std::unique_ptr<AppleAcceleratorTable> AppleTypes;
  std::unique_ptr<AppleAcceleratorTable> AppleNamespaces;
  std::unique_ptr<AppleAcceleratorTable> AppleObjC;

  DWARFUnitVector DWOUnits;
  std::unique_ptr<DWARFDebugAbbrev> AbbrevDWO;

  /// The maximum DWARF version of all units.
  unsigned MaxVersion = 0;

  struct DWOFile {
    object::OwningBinary<object::ObjectFile> File;
    std::unique_ptr<DWARFContext> Context;
  };
  StringMap<std::weak_ptr<DWOFile>> DWOFiles;
  std::weak_ptr<DWOFile> DWP;
  bool CheckedForDWP = false;
  std::string DWPName;

  std::unique_ptr<MCRegisterInfo> RegInfo;

  /// Read compile units from the debug_info section (if necessary)
  /// and type units from the debug_types sections (if necessary)
  /// and store them in NormalUnits.
  void parseNormalUnits();

  /// Read compile units from the debug_info.dwo section (if necessary)
  /// and type units from the debug_types.dwo section (if necessary)
  /// and store them in DWOUnits.
  /// If \p Lazy is true, set up to parse but don't actually parse them.
  enum { EagerParse = false, LazyParse = true };
  void parseDWOUnits(bool Lazy = false);

  std::unique_ptr<const DWARFObject> DObj;

public:
  DWARFContext(std::unique_ptr<const DWARFObject> DObj,
               std::string DWPName = "");
  ~DWAR