//===-- llvm/BinaryFormat/COFF.h --------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains an definitions used in Windows COFF Files.
//
// Structures and enums defined within this file where created using
// information from Microsoft's publicly available PE/COFF format document:
//
// Microsoft Portable Executable and Common Object File Format Specification
// Revision 8.1 - February 15, 2008
//
// As of 5/2/2010, hosted by Microsoft at:
// http://www.microsoft.com/whdc/system/platform/firmware/pecoff.mspx
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BINARYFORMAT_COFF_H
#define LLVM_BINARYFORMAT_COFF_H

#include "llvm/Support/DataTypes.h"
#include <cassert>
#include <cstring>

namespace llvm {
namespace COFF {

// The maximum number of sections that a COFF object can have (inclusive).
const int32_t MaxNumberOfSections16 = 65279;

// The PE signature bytes that follows the DOS stub header.
static const char PEMagic[] = {'P', 'E', '\0', '\0'};

static const char BigObjMagic[] = {
    '\xc7', '\xa1', '\xba', '\xd1', '\xee', '\xba', '\xa9', '\x4b',
    '\xaf', '\x20', '\xfa', '\xf6', '\x6a', '\xa4', '\xdc', '\xb8',
};

static const char ClGlObjMagic[] = {
    '\x38', '\xfe', '\xb3', '\x0c', '\xa5', '\xd9', '\xab', '\x4d',
    '\xac', '\x9b', '\xd6', '\xb6', '\x22', '\x26', '\x53', '\xc2',
};

// The signature bytes that start a .res file.
static const char WinResMagic[] = {
    '\x00', '\x00', '\x00', '\x00', '\x20', '\x00', '\x00', '\x00',
    '\xff', '\xff', '\x00', '\x00', '\xff', '\xff', '\x00', '\x00',
};

// Sizes in bytes of various things in the COFF format.
enum {
  Header16Size = 20,
  Header32Size = 56,
  NameSize = 8,
  Symbol16Size = 18,
  Symbol32Size = 20,
  SectionSize = 40,
  RelocationSize = 10
};

struct header {
  uint16_t Machine;
  int32_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
};

struct BigObjHeader {
  enum : uint16_t { MinBigObjectVersion = 2 };

  uint16_t Sig1; ///< Must be IMAGE_FILE_MACHINE_UNKNOWN (0).
  uint16_t Sig2; ///< Must be 0xFFFF.
  uint16_t Version;
  uint16_t Machine;
  uint32_t TimeDateStamp;
  uint8_t UUID[16];
  uint32_t unused1;
  uint32_t unused2;
  uint32_t unused3;
  uint32_t unused4;
  uint32_t NumberOfSections;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
};

enum MachineTypes : unsigned {
  MT_Invalid = 0xffff,

  IMAGE_FILE_MACHINE_UNKNOWN = 0x0,
  IMAGE_FILE_MACHINE_AM33 = 0x1D3,
  IMAGE_FILE_MACHINE_AMD64 = 0x8664,
  IMAGE_FILE_MACHINE_ARM = 0x1C0,
  IMAGE_FILE_MACHINE_ARMNT = 0x1C4,
  IMAGE_FILE_MACHINE_ARM64 = 0xAA64,
  IMAGE_FILE_MACHINE_EBC = 0xEBC,
  IMAGE_FILE_MACHINE_I386 = 0x14C,
  IMAGE_FILE_MACHINE_IA64 = 0x200,
  IMAGE_FILE_MACHINE_M32R = 0x9041,
  IMAGE_FILE_MACHINE_MIPS16 = 0x266,
  IMAGE_FILE_MACHINE_MIPSFPU = 0x366,
  IMAGE_FILE_MACHINE_MIPSFPU16 = 0x466,
  IMAGE_FILE_MACHINE_POWERPC = 0x1F0,
  IMAGE_FILE_MACHINE_POWERPCFP = 0x1F1,
  IMAGE_FILE_MACHINE_R4000 = 0x166,
  IMAGE_FILE_MACHINE_RISCV32 = 0x5032,
  IMAGE_FILE_MACHINE_RISCV64 = 0x5064,
  IMAGE_FILE_MACHINE_RISCV128 = 0x5128,
  IMAGE_FILE_MACHINE_SH3 = 0x1A2,
  IMAGE_FILE_MACHINE_SH3DSP = 0x1A3,
  IMAGE_FILE_MACHINE_SH4 = 0x1A6,
  IMAGE_FILE_MACHINE_SH5 = 0x1A8,
  IMAGE_FILE_MACHINE_THUMB = 0x1C2,
  IMAGE_FILE_MACHINE_WCEMIPSV2 = 0x169
};

enum Characteristics : unsigned {
  C_Invalid = 0,

  /// The file does not contain base relocations and must be loaded at its
  /// preferred base. If this cannot be done, the loader will error.
  IMAGE_FILE_RELOCS_STRIPPED = 0x0001,
  /// The file is valid and can be run.
  IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002,
  /// COFF line numbers have been stripped. This is deprecated and should be
  /// 0.
  IMAGE_FILE_LINE_NUMS_STRIPPED = 0x0004,
  /// COFF symbol table entries for local symbols have been removed. This is
  /// deprecated and should be 0.
  IMAGE_FILE_LOCAL_SYMS_STRIPPED = 0x0008,
  /// Aggressively trim working set. This is deprecated and must be 0.
  IMAGE_FILE_AGGRESSIVE_WS_TRIM = 0x0010,
  /// Image can handle > 2GiB addresses.
  IMAGE_FILE_LARGE_ADDRESS_AWARE = 0x0020,
  /// Little endian: the LSB precedes the MSB in memory. This is deprecated
  /// and should be 0.
  IMAGE_FILE_BYTES_REVERSED_LO = 0x0080,
  /// Machine is based on a 32bit word architecture.
  IMAGE_FILE_32BIT_MACHINE = 0x0100,
  /// Debugging info has been removed.
  IMAGE_FILE_DEBUG_STRIPPED = 0x0200,
  /// If the image is on removable media, fully load it and copy it to swap.
  IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400,
  /// If the image is on network media, fully load it and copy it to swap.
  IMAGE_FILE_NET_RUN_FROM_SWAP = 0x0800,
  /// The image file is a system file, not a user program.
  IMAGE_FILE_SYSTEM = 0x1000,
  /// The image file is a DLL.
  IMAGE_FILE_DLL = 0x2000,
  /// This file should only be run on a uniprocessor machine.
  IMAGE_FILE_UP_SYSTEM_ONLY = 0x4000,
  /// Big endian: the MSB precedes the LSB in memory. This is deprecated
  /// and should be 0.
  IMAGE_FILE_BYTES_REVERSED_HI = 0x8000
};

enum ResourceTypeID : unsigned {
  RID_Cursor = 1,
  RID_Bitmap = 2,
  RID_Icon = 3,
  RID_Menu = 4,
  RID_Dialog = 5,
  RID_String = 6,
  RID_FontDir = 7,
  RID_Font = 8,
  RID_Accelerator = 9,
  RID_RCData = 10,
  RID_MessageTable = 11,
  RID_Group_Cursor = 12,
  RID_Group_Icon = 14,
  RID_Version = 16,
  RID_DLGInclude = 17,
  RID_PlugPlay = 19,
  RID_VXD = 20,
  RID_AniCursor = 21,
  RID_AniIcon = 22,
  RID_HTML = 23,
  RID_Manifest = 24,
};

struct symbol {
  char Name[NameSize];
  uint32_t Value;
  int32_t SectionNumber;
  uint16_t Type;
  uint8_t StorageClass;
  uint8_t NumberOfAuxSymbols;
};

enum SymbolSectionNumber : int32_t {
  IMAGE_SYM_DEBUG = -2,
  IMAGE_SYM_ABSOLUTE = -1,
  IMAGE_SYM_UNDEFINED = 0
};

/// Storage class tells where and what the symbol represents
enum SymbolStorageClass {
  SSC_Invalid = 0xff,

  IMAGE_SYM_CLASS_END_OF_FUNCTION = -1,  ///< Physical end of function
  IMAGE_SYM_CLASS_NULL = 0,              ///< No symbol
  IMAGE_SYM_CLASS_AUTOMATIC = 1,         ///< Stack variable
  IMAGE_SYM_CLASS_EXTERNAL = 2,          ///< External symbol
  IMAGE_SYM_CLASS_STATIC = 3,            ///< Static
  IMAGE_SYM_CLASS_REGISTER = 4,          ///< Register variable
  IMAGE_SYM_CLASS_EXTERNAL_DEF = 5,      ///< External definition
  IMAGE_SYM_CLASS_LABEL = 6,             ///< Label
  IMAGE_SYM_CLASS_UNDEFINED_LABEL = 7,   ///< Undefined label
  IMAGE_SYM_CLASS_MEMBER_OF_STRUCT = 8,  ///< Member of structure
  IMAGE_SYM_CLASS_ARGUMENT = 9,          ///< Function argument
  IMAGE_SYM_CLASS_STRUCT_TAG = 10,       ///< Structure tag
  IMAGE_SYM_CLASS_MEMBER_OF_UNION = 11,  ///< Member of union
  IMAGE_SYM_CLASS_UNION_TAG = 12,        ///< Union tag
  IMAGE_SYM_CLASS_TYPE_DEFINITION = 13,  ///< Type definition
  IMAGE_SYM_CLASS_UNDEFINED_STATIC = 14, ///< Undefined static
  IMAGE_SYM_CLASS_ENUM_TAG = 15,         ///< Enumeration tag
  IMAGE_SYM_CLASS_MEMBER_OF_ENUM = 16,   ///< Member of enumeration
  IMAGE_SYM_CLASS_REGISTER_PARAM = 17,   ///< Register parameter
  IMAGE_SYM_CLASS_BIT_FIELD = 18,        ///< Bit field
  /// ".bb" or ".eb" - beginning or end of block
  IMAGE_SYM_CLASS_BLOCK = 100,
  /// ".bf" or ".ef" - beginning or end of function
  IMAGE_SYM_CLASS_FUNCTION = 101,
  IMAGE_SYM_CLASS_END_OF_STRUCT = 102, ///< End of structure
  IMAGE_SYM_CLASS_FILE = 103,          ///< File name
  /// Line number, reformatted as symbol
  IMAGE_SYM_CLASS_SECTION = 104,
  IMAGE_S