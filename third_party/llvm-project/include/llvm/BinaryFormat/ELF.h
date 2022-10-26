//===- llvm/BinaryFormat/ELF.h - ELF constants and structures ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This header contains common, non-processor-specific data structures and
// constants for the ELF file format.
//
// The details of the ELF32 bits in this file are largely based on the Tool
// Interface Standard (TIS) Executable and Linking Format (ELF) Specification
// Version 1.2, May 1995. The ELF64 stuff is based on ELF-64 Object File Format
// Version 1.5, Draft 2, May 1998 as well as OpenBSD header files.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BINARYFORMAT_ELF_H
#define LLVM_BINARYFORMAT_ELF_H

#include <cstdint>
#include <cstring>

namespace llvm {
namespace ELF {

using Elf32_Addr = uint32_t; // Program address
using Elf32_Off = uint32_t;  // File offset
using Elf32_Half = uint16_t;
using Elf32_Word = uint32_t;
using Elf32_Sword = int32_t;

using Elf64_Addr = uint64_t;
using Elf64_Off = uint64_t;
using Elf64_Half = uint16_t;
using Elf64_Word = uint32_t;
using Elf64_Sword = int32_t;
using Elf64_Xword = uint64_t;
using Elf64_Sxword = int64_t;

// Object file magic string.
static const char ElfMagic[] = {0x7f, 'E', 'L', 'F', '\0'};

// e_ident size and indices.
enum {
  EI_MAG0 = 0,       // File identification index.
  EI_MAG1 = 1,       // File identification index.
  EI_MAG2 = 2,       // File identification index.
  EI_MAG3 = 3,       // File identification index.
  EI_CLASS = 4,      // File class.
  EI_DATA = 5,       // Data encoding.
  EI_VERSION = 6,    // File version.
  EI_OSABI = 7,      // OS/ABI identification.
  EI_ABIVERSION = 8, // ABI version.
  EI_PAD = 9,        // Start of padding bytes.
  EI_NIDENT = 16     // Number of bytes in e_ident.
};

struct Elf32_Ehdr {
  unsigned char e_ident[EI_NIDENT]; // ELF Identification bytes
  Elf32_Half e_type;                // Type of file (see ET_* below)
  Elf32_Half e_machine;   // Required architecture for this file (see EM_*)
  Elf32_Word e_version;   // Must be equal to 1
  Elf32_Addr e_entry;     // Address to jump to in order to start program
  Elf32_Off e_phoff;      // Program header table's file offset, in bytes
  Elf32_Off e_shoff;      // Section header table's file offset, in bytes
  Elf32_Word e_flags;     // Processor-specific flags
  Elf32_Half e_ehsize;    // Size of ELF header, in bytes
  Elf32_Half e_phentsize; // Size of an entry in the program header table
  Elf32_Half e_phnum;     // Number of entries in the program header table
  Elf32_Half e_shentsize; // Size of an entry in the section header table
  Elf32_Half e_shnum;     // Number of entries in the section header table
  Elf32_Half e_shstrndx;  // Sect hdr table index of sect name string table

  bool checkMagic() const {
    return (memcmp(e_ident, ElfMagic, strlen(ElfMagic))) == 0;
  }

  unsigned char getFileClass() const { return e_ident[EI_CLASS]; }
  unsigned char getDataEncoding() const { return e_ident[EI_DATA]; }
};

// 64-bit ELF header. Fields are the same as for ELF32, but with different
// types (see above).
struct Elf64_Ehdr {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;

  bool checkMagic() const {
    return (memcmp(e_ident, ElfMagic, strlen(ElfMagic))) == 0;
  }

  unsigned char getFileClass() const { return e_ident[EI_CLASS]; }
  unsigned char getDataEncoding() const { return e_ident[EI_DATA]; }
};

// File types
enum {
  ET_NONE = 0,        // No file type
  ET_REL = 1,         // Relocatable file
  ET_EXEC = 2,        // Executable file
  ET_DYN = 3,         // Shared object file
  ET_CORE = 4,        // Core file
  ET_LOPROC = 0xff00, // Beginning of processor-specific codes
  ET_HIPROC = 0xffff  // Processor-specific
};

// Versioning
enum { EV_NONE = 0, EV_CURRENT = 1 };

// Machine architectures
// See current registered ELF machine architectures at:
//    http://www.uxsglobal.com/developers/gabi/latest/ch4.eheader.html
enum {
  EM_NONE = 0,           // No machine
  EM_M32 = 1,            // AT&T WE 32100
  EM_SPARC = 2,          // SPARC
  EM_386 = 3,            // Intel 386
  EM_68K = 4,            // Motorola 68000
  EM_88K = 5,            // Motorola 88000
  EM_IAMCU = 6,          // Intel MCU
  EM_860 = 7,            // Intel 80860
  EM_MIPS = 8,           // MIPS R3000
  EM_S370 = 9,           // IBM System/370
  EM_MIPS_RS3_LE = 10,   // MIPS RS3000 Little-endian
  EM_PARISC = 15,        // Hewlett-Packard PA-RISC
  EM_VPP500 = 17,        // Fujitsu VPP500
  EM_SPARC32PLUS = 18,   // Enhanced instruction set SPARC
  EM_960 = 19,           // Intel 80960
  EM_PPC = 20,           // PowerPC
  EM_PPC64 = 21,         // PowerPC64
  EM_S390 = 22,          // IBM System/390
  EM_SPU = 23,           // IBM SPU/SPC
  EM_V800 = 36,          // NEC V800
  EM_FR20 = 37,          // Fujitsu FR20
  EM_RH32 = 38,          // TRW RH-32
  EM_RCE = 39,           // Motorola RCE
  EM_ARM = 40,           // ARM
  EM_ALPHA = 41,         // DEC Alpha
  EM_SH = 42,            // Hitachi SH
  EM_SPARCV9 = 43,       // SPARC V9
  EM_TRICORE = 44,       // Siemens TriCore
  EM_ARC = 45,           // Argonaut RISC Core
  EM_H8_300 = 46,        // Hitachi H8/300
  EM_H8_300H = 47,       // Hitachi H8/300H
  EM_H8S = 48,           // Hitachi H8S
  EM_H8_500 = 49,        // Hitachi H8/500
  EM_IA_64 = 50,         // Intel IA-64 processor architecture
  EM_MIPS_X = 51,        // Stanford MIPS-X
  EM_COLDFIRE = 52,      // Motorola ColdFire
  EM_68HC12 = 53,        // Motorola M68HC12
  EM_MMA = 54,           // Fujitsu MMA Multimedia Accelerator
  EM_PCP = 55,           // Siemens PCP
  EM_NCPU = 56,          // Sony nCPU 