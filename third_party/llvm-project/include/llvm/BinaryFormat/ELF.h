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
  EM_NCPU = 56,          // Sony nCPU embedded RISC processor
  EM_NDR1 = 57,          // Denso NDR1 microprocessor
  EM_STARCORE = 58,      // Motorola Star*Core processor
  EM_ME16 = 59,          // Toyota ME16 processor
  EM_ST100 = 60,         // STMicroelectronics ST100 processor
  EM_TINYJ = 61,         // Advanced Logic Corp. TinyJ embedded processor family
  EM_X86_64 = 62,        // AMD x86-64 architecture
  EM_PDSP = 63,          // Sony DSP Processor
  EM_PDP10 = 64,         // Digital Equipment Corp. PDP-10
  EM_PDP11 = 65,         // Digital Equipment Corp. PDP-11
  EM_FX66 = 66,          // Siemens FX66 microcontroller
  EM_ST9PLUS = 67,       // STMicroelectronics ST9+ 8/16 bit microcontroller
  EM_ST7 = 68,           // STMicroelectronics ST7 8-bit microcontroller
  EM_68HC16 = 69,        // Motorola MC68HC16 Microcontroller
  EM_68HC11 = 70,        // Motorola MC68HC11 Microcontroller
  EM_68HC08 = 71,        // Motorola MC68HC08 Microcontroller
  EM_68HC05 = 72,        // Motorola MC68HC05 Microcontroller
  EM_SVX = 73,           // Silicon Graphics SVx
  EM_ST19 = 74,          // STMicroelectronics ST19 8-bit microcontroller
  EM_VAX = 75,           // Digital VAX
  EM_CRIS = 76,          // Axis Communications 32-bit embedded processor
  EM_JAVELIN = 77,       // Infineon Technologies 32-bit embedded processor
  EM_FIREPATH = 78,      // Element 14 64-bit DSP Processor
  EM_ZSP = 79,           // LSI Logic 16-bit DSP Processor
  EM_MMIX = 80,          // Donald Knuth's educational 64-bit processor
  EM_HUANY = 81,         // Harvard University machine-independent object files
  EM_PRISM = 82,         // SiTera Prism
  EM_AVR = 83,           // Atmel AVR 8-bit microcontroller
  EM_FR30 = 84,          // Fujitsu FR30
  EM_D10V = 85,          // Mitsubishi D10V
  EM_D30V = 86,          // Mitsubishi D30V
  EM_V850 = 87,          // NEC v850
  EM_M32R = 88,          // Mitsubishi M32R
  EM_MN10300 = 89,       // Matsushita MN10300
  EM_MN10200 = 90,       // Matsushita MN10200
  EM_PJ = 91,            // picoJava
  EM_OPENRISC = 92,      // OpenRISC 32-bit embedded processor
  EM_ARC_COMPACT = 93,   // ARC International ARCompact processor (old
                         // spelling/synonym: EM_ARC_A5)
  EM_XTENSA = 94,        // Tensilica Xtensa Architecture
  EM_VIDEOCORE = 95,     // Alphamosaic VideoCore processor
  EM_TMM_GPP = 96,       // Thompson Multimedia General Purpose Processor
  EM_NS32K = 97,         // National Semiconductor 32000 series
  EM_TPC = 98,           // Tenor Network TPC processor
  EM_SNP1K = 99,         // Trebia SNP 1000 processor
  EM_ST200 = 100,        // STMicroelectronics (www.st.com) ST200
  EM_IP2K = 101,         // Ubicom IP2xxx microcontroller family
  EM_MAX = 102,          // MAX Processor
  EM_CR = 103,           // National Semiconductor CompactRISC microprocessor
  EM_F2MC16 = 104,       // Fujitsu F2MC16
  EM_MSP430 = 105,       // Texas Instruments embedded microcontroller msp430
  EM_BLACKFIN = 106,     // Analog Devices Blackfin (DSP) processor
  EM_SE_C33 = 107,       // S1C33 Family of Seiko Epson processors
  EM_SEP = 108,          // Sharp embedded microprocessor
  EM_ARCA = 109,         // Arca RISC Microprocessor
  EM_UNICORE = 110,      // Microprocessor series from PKU-Unity Ltd. and MPRC
                         // of Peking University
  EM_EXCESS = 111,       // eXcess: 16/32/64-bit configurable embedded CPU
  EM_DXP = 112,          // Icera Semiconductor Inc. Deep Execution Processor
  EM_ALTERA_NIOS2 = 113, // Altera Nios II soft-core processor
  EM_CRX = 114,          // National Semiconductor CompactRISC CRX
  EM_XGATE = 115,        // Motorola XGATE embedded processor
  EM_C166 = 116,         // Infineon C16x/XC16x processor
  EM_M16C = 117,         // Renesas M16C series microprocessors
  EM_DSPIC30F = 118,     // Microchip Technology dsPIC30F Digital Signal
                         // Controller
  EM_CE = 119,           // Freescale Communication Engine RISC core
  EM_M32C = 120,         // Renesas M32C series microprocessors
  EM_TSK3000 = 131,      // Altium TSK3000 core
  EM_RS08 = 132,         // Freescale RS08 embedded processor
  EM_SHARC = 133,        // Analog Devices SHARC family of 32-bit DSP
                         // processors
  EM_ECOG2 = 134,        // Cyan Technology eCOG2 microprocessor
  EM_SCORE7 = 135,       // Sunplus S+core7 RISC processor
  EM_DSP24 = 136,        // New Japan Radio (NJR) 24-bit DSP Processor
  EM_VIDEOCORE3 = 137,   // Broadcom VideoCore III processor
  EM_LATTICEMICO32 = 138, // RISC processor for Lattice FPGA architecture
  EM_SE_C17 = 139,        // Seiko Epson C17 family
  EM_TI_C6000 = 140,      // The Texas Instruments TMS320C6000 DSP family
  EM_TI_C2000 = 141,      // The Texas Instruments TMS320C2000 DSP family
  EM_TI_C5500 = 142,      // The Texas Instruments TMS320C55x DSP family
  EM_MMDSP_PLUS = 160,    // STMicroelectronics 64bit VLIW Data Signal Processor
  EM_CYPRESS_M8C = 161,   // Cypress M8C microprocessor
  EM_R32C = 162,          // Renesas R32C series microprocessors
  EM_TRIMEDIA = 163,      // NXP Semiconductors TriMedia architecture family
  EM_HEXAGON = 164,       // Qualcomm Hexagon processor
  EM_8051 = 165,          // Intel 8051 and variants
  EM_STXP7X = 166,        // STMicroelectronics STxP7x family of configurable
                          // and extensible RISC processors
  EM_NDS32 = 167,         // Andes Technology compact code size embedded RISC
                          // processor family
  EM_ECOG1 = 168,         // Cyan Technology eCOG1X family
  EM_ECOG1X = 168,        // Cyan Technology eCOG1X family
  EM_MAXQ30 = 169,        // Dallas Semiconductor MAXQ30 Core Micro-controllers
  EM_XIMO16 = 170,        // New Japan Radio (NJR) 16-bit DSP Processor
  EM_MANIK = 171,         // M2000 Reconfigurable RISC Microprocessor
  EM_CRAYNV2 = 172,       // Cray Inc. NV2 vector architecture
  EM_RX = 173,            // Renesas RX family
  EM_METAG = 174,         // Imagination Technologies META processor
                          // architecture
  EM_MCST_ELBRUS = 175,   // MCST Elbrus general purpose hardware architecture
  EM_ECOG16 = 176,        // Cyan Technology eCOG16 family
  EM_CR16 = 177,          // National Semiconductor CompactRISC CR16 16-bit
                          // microprocessor
  EM_ETPU = 178,          // Freescale Extended Time Processing Unit
  EM_SLE9X = 179,         // Infineon Technologies SLE9X core
  EM_L10M = 180,          // Intel L10M
  EM_K10M = 181,          // Intel K10M
  EM_AARCH64 = 183,       // ARM AArch64
  EM_AVR32 = 185,         // Atmel Corporation 32-bit microprocessor family
  EM_STM8 = 186,          // STMicroeletronics STM8 8-bit microcontroller
  EM_TILE64 = 187,        // Tilera TILE64 multicore architecture family
  EM_TILEPRO = 188,       // Tilera TILEPro multicore architecture family
  EM_CUDA = 190,          // NVIDIA CUDA architecture
  EM_TILEGX = 191,        // Tilera TILE-Gx multicore architecture family
  EM_CLOUDSHIELD = 192,   // CloudShield architecture family
  EM_COREA_1ST = 193,     // KIPO-KAIST Core-A 1st generation processor family
  EM_COREA_2ND = 194,     // KIPO-KAIST Core-A 2nd generation processor family
  EM_ARC_COMPACT2 = 195,  // Synopsys ARCompact V2
  EM_OPEN8 = 196,         // Open8 8-bit RISC soft processor core
  EM_RL78 = 197,          // Renesas RL78 family
  EM_VIDEOCORE5 = 198,    // Broadcom VideoCore V processor
  EM_78KOR = 199,         // Renesas 78KOR family
  EM_56800EX = 200,       // Freescale 56800EX Digital Signal Controller (DSC)
  EM_BA1 = 201,           // Beyond BA1 CPU architecture
  EM_BA2 = 202,           // Beyond BA2 CPU architecture
  EM_XCORE = 203,         // XMOS xCORE processor family
  EM_MCHP_PIC = 204,      // Microchip 8-bit PIC(r) family
  EM_INTEL205 = 205,      // Reserved by Intel
  EM_INTEL206 = 206,      // Reserved by Intel
  EM_INTEL207 = 207,      // Reserved by Intel
  EM_INTEL208 = 208,      // Reserved by Intel
  EM_INTEL209 = 209,      // Reserved by Intel
  EM_KM32 = 210,          // KM211 KM32 32-bit processor
  EM_KMX32 = 211,         // KM211 KMX32 32-bit processor
  EM_KMX16 = 212,         // KM211 KMX16 16-bit processor
  EM_KMX8 = 213,          // KM211 KMX8 8-bit processor
  EM_KVARC = 214,         // KM211 KVARC processor
  EM_CDP = 215,           // Paneve CDP architecture family
  EM_COGE = 216,          // Cognitive Smart Memory Processor
  EM_COOL = 217,          // iCelero CoolEngine
  EM_NORC = 218,          // Nanoradio Optimized RISC
  EM_CSR_KALIMBA = 219,   // CSR Kalimba architecture family
  EM_AMDGPU = 224,        // AMD GPU architecture
  EM_RISCV = 243,         // RISC-V
  EM_LANAI = 244,         // Lanai 32-bit processor
  EM_BPF = 247,           // Linux kernel bpf virtual machine
};

// Object file classes.
enum {
  ELFCLASSNONE = 0,
  ELFCLASS32 = 1, // 32-bit object file
  ELFCLASS64 = 2  // 64-bit object file
};

// Object file byte orderings.
enum {
  ELFDATANONE = 0, // Invalid data encoding.
  ELFDATA2LSB = 1, // Little-endian object file
  ELFDATA2MSB = 2  // Big-endian object file
};

// OS ABI identification.
enum {
  ELFOSABI_NONE = 0,           // UNIX System V ABI
  ELFOSABI_HPUX = 1,           // HP-UX operating system
  ELFOSABI_NETBSD = 2,         // NetBSD
  ELFOSABI_GNU = 3,            // GNU/Linux
  ELFOSABI_LINUX = 3,          // Historical alias for ELFOSABI_GNU.
  ELFOSABI_HURD = 4,           // GNU/Hurd
  ELFOSABI_SOLARIS = 6,        // Solaris
  ELFOSABI_AIX = 7,            // AIX
  ELFOSABI_IRIX = 8,           // IRIX
  ELFOSABI_FREEBSD = 9,        // FreeBSD
  ELFOSABI_TRU64 = 10,         // TRU64 UNIX
  ELFOSABI_MODESTO = 11,       // Novell Modesto
  ELFOSABI_OPENBSD = 12,       // OpenBSD
  ELFOSABI_OPENVMS = 13,       // OpenVMS
  ELFOSABI_NSK = 14,           // Hewlett-Packard Non-Stop Kernel
  ELFOSABI_AROS = 15,          // AROS
  ELFOSABI_FENIXOS = 16,       // FenixOS
  ELFOSABI_CLOUDABI = 17,      // Nuxi CloudABI
  ELFOSABI_FIRST_ARCH = 64,    // First architecture-specific OS ABI
  ELFOSABI_AMDGPU_HSA = 64,    // AMD HSA runtime
  ELFOSABI_AMDGPU_PAL = 65,    // AMD PAL runtime
  ELFOSABI_AMDGPU_MESA3D = 66, // AMD GCN GPUs (GFX6+) for MESA runtime
  ELFOSABI_ARM = 97,           // ARM
  ELFOSABI_C6000_ELFABI = 64,  // Bare-metal TMS320C6000
  ELFOSABI_C6000_LINUX = 65,   // Linux TMS320C6000
  ELFOSABI_STANDALONE = 255,   // Standalone (embedded) application
  ELFOSABI_LAST_ARCH = 255     // Last Architecture-specific OS ABI
};

#define ELF_RELOC(name, value) name = value,

// X86_64 relocations.
enum {
#include "ELFRelocs/x86_64.def"
};

// i386 relocations.
enum {
#include "ELFRelocs/i386.def"
};

// ELF Relocation types for PPC32
enum {
#include "ELFRelocs/PowerPC.def"
};

// Specific e_flags for PPC64
enum {
  // e_flags bits specifying ABI:
  // 1 for original ABI using function descriptors,
  // 2 for revised ABI without function descriptors,
  // 0 for unspecified or not using any features affected by the differences.
  EF_PPC64_ABI = 3
};

// Special values for the st_other field in the symbol table entry for PPC64.
enum {
  STO_PPC64_LOCAL_BIT = 5,
  STO_PPC64_LOCAL_MASK = (7 << STO_PPC64_LOCAL_BIT)
};
static inline int64_t decodePPC64LocalEntryOffset(unsigned Other) {
  unsigned Val = (Other & STO_PPC64_LOCAL_MASK) >> STO_PPC64_LOCAL_BIT;
  return ((1 << Val) >> 2) << 2;
}
static inline unsigned encodePPC64LocalEntryOffset(int64_t Offset) {
  unsigned Val =
      (Offset >= 4 * 4 ? (Offset >= 8 * 4 ? (Offset >= 16 * 4 ? 6 : 5) : 4)
                       : (Offset >= 2 * 4 ? 3 : (Offset >= 1 * 4 ? 2 : 0)));
  return Val << STO_PPC64_LOCAL_BIT;
}

// ELF Relocation types for PPC64
enum {
#include "ELFRelocs/PowerPC64.def"
};

// ELF Relocation types for AArch64
enum {
#include "ELFRelocs/AArch64.def"
};

// ARM Specific e_flags
enum : unsigned {
  EF_ARM_SOFT_FLOAT = 0x00000200U,     // Legacy pre EABI_VER5
  EF_ARM_ABI_FLOAT_SOFT = 0x00000200U, // EABI_VER5
  EF_ARM_VFP_FLOAT = 0x00000400U,      // Legacy pre EABI_VER5
  EF_ARM_ABI_FLOAT_HARD = 0x00000400U, // EABI_VER5
  EF_ARM_EABI_UNKNOWN = 0x00000000U,
  EF_ARM_EABI_VER1 = 0x01000000U,
  EF_ARM_EABI_VER2 = 0x02000000U,
  EF_ARM_EABI_VER3 = 0x03000000U,
  EF_ARM_EABI_VER4 = 0x04000000U,
  EF_ARM_EABI_VER5 = 0x05000000U,
  EF_ARM_EABIMASK = 0xFF000000U
};

// ELF Relocation types for ARM
enum {
#include "ELFRelocs/ARM.def"
};

// ARC Specific e_flags
enum : unsigned {
  EF_ARC_MACH_MSK = 0x000000ff,
  EF_ARC_OSABI_MSK = 0x00000f00,
  E_ARC_MACH_ARC600 = 0x00000002,
  E_ARC_MACH_ARC601 = 0x00000004,
  E_ARC_MACH_ARC700 = 0x00000003,
  EF_ARC_CPU_ARCV2EM = 0x00000005,
  EF_ARC_CPU_ARCV2HS = 0x00000006,
  E_ARC_OSABI_ORIG = 0x00000000,
  E_ARC_OSABI_V2 = 0x00000200,
  E_ARC_OSABI_V3 = 0x00000300,
  E_ARC_OSABI_V4 = 0x00000400,
  EF_ARC_PIC = 0x00000100
};

// ELF Relocation types for ARC
enum {
#include "ELFRelocs/ARC.def"
};

// AVR specific e_flags
enum : unsigned {
  EF_AVR_ARCH_AVR1 = 1,
  EF_AVR_ARCH_AVR2 = 2,
  EF_AVR_ARCH_AVR25 = 25,
  EF_AVR_ARCH_AVR3 = 3,
  EF_AVR_ARCH_AVR31 = 31,
  EF_AVR_ARCH_AVR35 = 35,
  EF_AVR_ARCH_AVR4 = 4,
  EF_AVR_ARCH_AVR5 = 5,
  EF_AVR_ARCH_AVR51 = 51,
  EF_AVR_ARCH_AVR6 = 6,
  EF_AVR_ARCH_AVRTINY = 100,
  EF_AVR_ARCH_XMEGA1 = 101,
  EF_AVR_ARCH_XMEGA2 = 102,
  EF_AVR_ARCH_XMEGA3 = 103,
  EF_AVR_ARCH_XMEGA4 = 104,
  EF_AVR_ARCH_XMEGA5 = 105,
  EF_AVR_ARCH_XMEGA6 = 106,
  EF_AVR_ARCH_XMEGA7 = 107
};

// ELF Relocation types for AVR
enum {
#include "ELFRelocs/AVR.def"
};

// Mips Specific e_flags
enum : unsigned {
  EF_MIPS_NOREORDER = 0x00000001, // Don't reorder instructions
  EF_MIPS_PIC = 0x00000002,       // Position independent code
  EF_MIPS_CPIC = 0x00000004,      // Call object with Position independent code
  EF_MIPS_ABI2 = 0x00000020,      // File uses N32 ABI
  EF_MIPS_32BITMODE = 0x00000100, // Code compiled for a 64-bit machine
                                  // in 32-bit mode
  EF_MIPS_FP64 = 0x00000200,      // Code compiled for a 32-bit machine
                                  // but uses 64-bit FP registers
  EF_MIPS_NAN2008 = 0x00000400,   // Uses IEE 754-2008 NaN encoding

  // ABI flags
  EF_MIPS_ABI_O32 = 0x00001000, // This file follows the first MIPS 32 bit ABI
  EF_MIPS_ABI_O64 = 0x00002000, // O32 ABI extended for 64-bit architecture.
  EF_MIPS_ABI_EABI32 = 0x00003000, // EABI in 32 bit mode.
  EF_MIPS_ABI_EABI64 = 0x00004000, // EABI in 64 bit mode.
  EF_MIPS_ABI = 0x0000f000,        // Mask for selecting EF_MIPS_ABI_ variant.

  // MIPS machine variant
  EF_MIPS_MACH_NONE = 0x00000000,    // A standard MIPS implementation.
  EF_MIPS_MACH_3900 = 0x00810000,    // Toshiba R3900
  EF_MIPS_MACH_4010 = 0x00820000,    // LSI R4010
  EF_MIPS_MACH_4100 = 0x00830000,    // NEC VR4100
  EF_MIPS_MACH_4650 = 0x00850000,    // MIPS R4650
  EF_MIPS_MACH_4120 = 0x00870000,    // NEC VR4120
  EF_MIPS_MACH_4111 = 0x00880000,    // NEC VR4111/VR4181
  EF_MIPS_MACH_SB1 = 0x008a0000,     // Broadcom SB-1
  EF_MIPS_MACH_OCTEON = 0x008b0000,  // Cavium Networks Octeon
  EF_MIPS_MACH_XLR = 0x008c0000,     // RMI Xlr
  EF_MIPS_MACH_OCTEON2 = 0x008d0000, // Cavium Networks Octeon2
  EF_MIPS_MACH_OCTEON3 = 0x008e0000, // Cavium Networks Octeon3
  EF_MIPS_MACH_5400 = 0x00910000,    // NEC VR5400
  EF_MIPS_MACH_5900 = 0x00920000,    // MIPS R5900
  EF_MIPS_MACH_5500 = 0x00980000,    // NEC VR5500
  EF_MIPS_MACH_9000 = 0x00990000,    // Unknown
  EF_MIPS_MACH_LS2E = 0x00a00000,    // ST Microelectronics Loongson 2E
  EF_MIPS_MACH_LS2F = 0x00a10000,    // ST Microelectronics Loongson 2F
  EF_MIPS_MACH_LS3A = 0x00a20000,    // Loongson 3A
  EF_MIPS_MACH = 0x00ff0000,         // EF_MIPS_MACH_xxx selection mask

  // ARCH_ASE
  EF_MIPS_MICROMIPS = 0x02000000,     // microMIPS
  EF_MIPS_ARCH_ASE_M16 = 0x04000000,  // Has Mips-16 ISA extensions
  EF_MIPS_ARCH_ASE_MDMX = 0x08000000, // Has MDMX multimedia extensions
  EF_MIPS_ARCH_ASE = 0x0f000000,      // Mask for EF_MIPS_ARCH_ASE_xxx flags

  // ARCH
  EF_MIPS_ARCH_1 = 0x00000000,    // MIPS1 instruction set
  EF_MIPS_ARCH_2 = 0x10000000,    // MIPS2 instruction set
  EF_MIPS_ARCH_3 = 0x20000000,    // MIPS3 instruction set
  EF_MIPS_ARCH_4 = 0x30000000,    // MIPS4 instruction set
  EF_MIPS_ARCH_5 = 0x40000000,    // MIPS5 instruction set
  EF_MIPS_ARCH_32 = 0x50000000,   // MIPS32 instruction set per linux not elf.h
  EF_MIPS_ARCH_64 = 0x60000000,   // MIPS64 instruction set per linux not elf.h
  EF_MIPS_ARCH_32R2 = 0x70000000, // mips32r2, mips32r3, mips32r5
  EF_MIPS_ARCH_64R2 = 0x80000000, // mips64r2, mips64r3, mips64r5
  EF_MIPS_ARCH_32R6 = 0x90000000, // mips32r6
  EF_MIPS_ARCH_64R6 = 0xa0000000, // mips64r6
  EF_MIPS_ARCH = 0xf0000000       // Mask for applying EF_MIPS_ARCH_ variant
};

// ELF Relocation types for Mips
enum {
#include "ELFRelocs/Mips.def"
};

// Special values for the st_other field in the symbol table entry for MIPS.
enum {
  STO_MIPS_OPTIONAL = 0x04,  // Symbol whose definition is optional
  STO_MIPS_PLT = 0x08,       // PLT entry related dynamic table record
  STO_MIPS_PIC = 0x20,       // PIC func in an object mixes PIC/non-PIC
  STO_MIPS_MICROMIPS = 0x80, // MIPS Specific ISA for MicroMips
  STO_MIPS_MIPS16 = 0xf0     // MIPS Specific ISA for Mips16
};

// .MIPS.options section descriptor kinds
enum {
  ODK_NULL = 0,       // Undefined
  ODK_REGINFO = 1,    // Register usage information
  ODK_EXCEPTIONS = 2, // Exception processing options
  ODK_PAD = 3,        // Section padding options
  ODK_HWPATCH = 4,    // Hardware patches applied
  ODK_FILL = 5,       // Linker fill value
  ODK_TAGS = 6,       // Space for tool identification
  ODK_HWAND = 7,      // Hardware AND patches applied
  ODK_HWOR = 8,       // Hardware OR patches applied
  ODK_GP_GROUP = 9,   // GP group to use for text/data sections
  ODK_IDENT = 10,     // ID information
  ODK_PAGESIZE = 11   // Page size information
};

// Hexagon-specific e_flags
enum {
  // Object processor version flags, bits[11:0]
  EF_HEXAGON_MACH_V2 = 0x00000001,  // Hexagon V2
  EF_HEXAGON_MACH_V3 = 0x00000002,  // Hexagon V3
  EF_HEXAGON_MACH_V4 = 0x00000003,  // Hexagon V4
  EF_HEXAGON_MACH_V5 = 0x00000004,  // Hexagon V5
  EF_HEXAGON_MACH_V55 = 0x00000005, // Hexagon V55
  EF_HEXAGON_MACH_V60 = 0x00000060, // Hexagon V60
  EF_HEXAGON_MACH_V62 = 0x00000062, // Hexagon V62
  EF_HEXAGON_MACH_V65 = 0x00000065, // Hexagon V65
  EF_HEXAGON_MACH_V66 = 0x00000066, // Hexagon V66

  // Highest ISA version flags
  EF_HEXAGON_ISA_MACH = 0x00000000, // Same as specified in bits[11:0]
                                    // of e_flags
  EF_HEXAGON_ISA_V2 = 0x00000010,   // Hexagon V2 ISA
  EF_HEXAGON_ISA_V3 = 0x00000020,   // Hexagon V3 ISA
  EF_HEXAGON_ISA_V4 = 0x00000030,   // Hexagon V4 ISA
  EF_HEXAGON_ISA_V5 = 0x00000040,   // Hexagon V5 ISA
  EF_HEXAGON_ISA_V55 = 0x00000050,  // Hexagon V55 ISA
  EF_HEXAGON_ISA_V60 = 0x00000060,  // Hexagon V60 ISA
  EF_HEXAGON_ISA_V62 = 0x00000062,  // Hexagon V62 ISA
  EF_HEXAGON_ISA_V65 = 0x00000065,  // Hexagon V65 ISA
  EF_HEXAGON_ISA_V66 = 0x00000066,  // Hexagon V66 ISA
};

// Hexagon-specific section indexes for common small data
enum {
  SHN_HEXAGON_SCOMMON = 0xff00,   // Other access sizes
  SHN_HEXAGON_SCOMMON_1 = 0xff01, // Byte-sized access
  SHN_HEXAGON_SCOMMON_2 = 0xff02, // Half-word-sized access
  SHN_HEXAGON_SCOMMON_4 = 0xff03, // Word-sized access
  SHN_HEXAGON_SCOMMON_8 = 0xff04  // Double-word-size access
};

// ELF Relocation types for Hexagon
enum {
#include "ELFRelocs/Hexagon.def"
};

// ELF Relocation type for Lanai.
enum {
#include "ELFRelocs/Lanai.def"
};

// RISCV Specific e_flags
enum : unsigned {
  EF_RISCV_RVC = 0x0001,
  EF_RISCV_FLOAT_ABI = 0x0006,
  EF_RISCV_FLOAT_ABI_SOFT = 0x0000,
  EF_RISCV_FLOAT_ABI_SINGLE = 0x0002,
  EF_RISCV_FLOAT_ABI_DOUBLE = 0x0004,
  EF_RISCV_FLOAT_ABI_QUAD = 0x0006,
  EF_RISCV_RVE = 0x0008
};

// ELF Relocation types for RISC-V
enum {
#include "ELFRelocs/RISCV.def"
};

// ELF Relocation types for S390/zSeries
enum {
#include "ELFRelocs/SystemZ.def"
};

// ELF Relocation type for Sparc.
enum {
#include "ELFRelocs/Sparc.def"
};

// AMDGPU specific e_flags.
enum : unsigned {
  // Processor selection mask for EF_AMDGPU_MACH_* values.
  EF_AMDGPU_MACH = 0x0ff,

  // Not specified processor.
  EF_AMDGPU_MACH_NONE = 0x000,

  // R600-based processors.

  // Radeon HD 2000/3000 Series (R600).
  EF_AMDGPU_MACH_R600_R600 = 0x001,
  EF_AMDGPU_MACH_R600_R630 = 0x002,
  EF_AMDGPU_MACH_R600_RS880 = 0x003,
  EF_AMDGPU_MACH_R600_RV670 = 0x004,
  // Radeon HD 4000 Series (R700).
  EF_AMDGPU_MACH_R600_RV710 = 0x005,
  EF_AMDGPU_MACH_R600_RV730 = 0x006,
  EF_AMDGPU_MACH_R600_RV770 = 0x007,
  // Radeon HD 5000 Series (Evergreen).
  EF_AMDGPU_MACH_R600_CEDAR = 0x008,
  EF_AMDGPU_MACH_R600_CYPRESS = 0x009,
  EF_AMDGPU_MACH_R600_JUNIPER = 0x00a,
  EF_AMDGPU_MACH_R600_REDWOOD = 0x00b,
  EF_AMDGPU_MACH_R600_SUMO = 0x00c,
  // Radeon HD 6000 Series (Northern Islands).
  EF_AMDGPU_MACH_R600_BARTS = 0x00d,
  EF_AMDGPU_MACH_R600_CAICOS = 0x00e,
  EF_AMDGPU_MACH_R600_CAYMAN = 0x00f,
  EF_AMDGPU_MACH_R600_TURKS = 0x010,

  // Reserved for R600-based processors.
  EF_AMDGPU_MACH_R600_RESERVED_FIRST = 0x011,
  EF_AMDGPU_MACH_R600_RESERVED_LAST = 0x01f,

  // First/last R600-based processors.
  EF_AMDGPU_MACH_R600_FIRST = EF_AMDGPU_MACH_R600_R600,
  EF_AMDGPU_MACH_R600_LAST = EF_AMDGPU_MACH_R600_TURKS,

  // AMDGCN-based processors.

  // AMDGCN GFX6.
  EF_AMDGPU_MACH_AMDGCN_GFX600 = 0x020,
  EF_AMDGPU_MACH_AMDGCN_GFX601 = 0x021,
  // AMDGCN GFX7.
  EF_AMDGPU_MACH_AMDGCN_GFX700 = 0x022,
  EF_AMDGPU_MACH_AMDGCN_GFX701 = 0x023,
  EF_AMDGPU_MACH_AMDGCN_GFX702 = 0x024,
  EF_AMDGPU_MACH_AMDGCN_GFX703 = 0x025,
  EF_AMDGPU_MACH_AMDGCN_GFX704 = 0x026,
  // AMDGCN GFX8.
  EF_AMDGPU_MACH_AMDGCN_GFX801 = 0x028,
  EF_AMDGPU_MACH_AMDGCN_GFX802 = 0x029,
  EF_AMDGPU_MACH_AMDGCN_GFX803 = 0x02a,
  EF_AMDGPU_MACH_AMDGCN_GFX810 = 0x02b,
  // AMDGCN GFX9.
  EF_AMDGPU_MACH_AMDGCN_GFX900 = 0x02c,
  EF_AMDGPU_MACH_AMDGCN_GFX902 = 0x02d,
  EF_AMDGPU_MACH_AMDGCN_GFX904 = 0x02e,
  EF_AMDGPU_MACH_AMDGCN_GFX906 = 0x02f,
  EF_AMDGPU_MACH_AMDGCN_GFX908 = 0x030,
  EF_AMDGPU_MACH_AMDGCN_GFX909 = 0x031,
  // AMDGCN GFX10.
  EF_AMDGPU_MACH_AMDGCN_GFX1010 = 0x033,
  EF_AMDGPU_MACH_AMDGCN_GFX1011 = 0x034,
  EF_AMDGPU_MACH_AMDGCN_GFX1012 = 0x035,

  // Reserved for AMDGCN-based processors.
  EF_AMDGPU_MACH_AMDGCN_RESERVED0 = 0x027,
  EF_AMDGPU_MACH_AMDGCN_RESERVED1 = 0x032,

  // First/last AMDGCN-based processors.
  EF_AMDGPU_MACH_AMDGCN_FIRST = EF_AMDGPU_MACH_AMDGCN_GFX600,
  EF_AMDGPU_MACH_AMDGCN_LAST = EF_AMDGPU_MACH_AMDGCN_GFX1012,

  // Indicates if the "xnack" target feature is enabled for all code contained
  // in the object.
  EF_AMDGPU_XNACK = 0x100,
  // Indicates if the "sram-ecc" target feature is enabled for all code
  // contained in the object.
  EF_AMDGPU_SRAM_ECC = 0x200,
};

// ELF Relocation types for AMDGPU
enum {
#include "ELFRelocs/AMDGPU.def"
};

// ELF Relocation types for BPF
enum {
#include "ELFRelocs/BPF.def"
};

// MSP430 specific e_flags
enum : unsigned {
  EF_MSP430_MACH_MSP430x11 = 11,
  EF_MSP430_MACH_MSP430x11x1 = 110,
  EF_MSP430_MACH_MSP430x12 = 12,
  EF_MSP430_MACH_MSP430x13 = 13,
  EF_MSP430_MACH_MSP430x14 = 14,
  EF_MSP430_MACH_MSP430x15 = 15,
  EF_MSP430_MACH_MSP430x16 = 16,
  EF_MSP430_MACH_MSP430x20 = 20,
  EF_MSP430_MACH_MSP430x22 = 22,
  EF_MSP430_MACH_MSP430x23 = 23,
  EF_MSP430_MACH_MSP430x24 = 24,
  EF_MSP430_MACH_MSP430x26 = 26,
  EF_MSP430_MACH_MSP430x31 = 31,
  EF_MSP430_MACH_MSP430x32 = 32,
  EF_MSP430_MACH_MSP430x33 = 33,
  EF_MSP430_MACH_MSP430x41 = 41,
  EF_MSP430_MACH_MSP430x42 = 42,
  EF_MSP430_MACH_MSP430x43 = 43,
  EF_MSP430_MACH_MSP430x44 = 44,
  EF_MSP430_MACH_MSP430X = 45,
  EF_MSP430_MACH_MSP430x46 = 46,
  EF_MSP430_MACH_MSP430x47 = 47,
  EF_MSP430_MACH_MSP430x54 = 5