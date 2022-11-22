//===- ELFTypes.h - Endian specific types for ELF ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECT_ELFTYPES_H
#define LLVM_OBJECT_ELFTYPES_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/Error.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace llvm {
namespace object {

using support::endianness;

template <class ELFT> struct Elf_Ehdr_Impl;
template <class ELFT> struct Elf_Shdr_Impl;
template <class ELFT> struct Elf_Sym_Impl;
template <class ELFT> struct Elf_Dyn_Impl;
template <class ELFT> struct Elf_Phdr_Impl;
template <class ELFT, bool isRela> struct Elf_Rel_Impl;
template <class ELFT> struct Elf_Verdef_Impl;
template <class ELFT> struct Elf_Verdaux_Impl;
template <class ELFT> struct Elf_Verneed_Impl;
template <class ELFT> struct Elf_Vernaux_Impl;
template <class ELFT> struct Elf_Versym_Impl;
template <class ELFT> struct Elf_Hash_Impl;
template <class ELFT> struct Elf_GnuHash_Impl;
template <class ELFT> struct Elf_Chdr_Impl;
template <class ELFT> struct Elf_Nhdr_Impl;
template <class ELFT> class Elf_Note_Impl;
template <class ELFT> class Elf_Note_Iterator_Impl;
template <class ELFT> struct Elf_CGProfile_Impl;

template <endianness E, bool Is64> struct ELFType {
private:
  template <typename Ty>
  using packed = support::detail::packed_endian_specific_integral<Ty, E, 1>;

public:
  static const endianness TargetEndianness = E;
  static const bool Is64Bits = Is64;

  using uint = typename std::conditional<Is64, uint64_t, uint32_t>::type;
  using Ehdr = Elf_Ehdr_Impl<ELFType<E, Is64>>;
  using Shdr = Elf_Shdr_Impl<ELFType<E, Is64>>;
  using Sym = Elf_Sym_Impl<ELFType<E, Is64>>;
  using Dyn = Elf_Dyn_Impl<ELFType<E, Is64>>;
  using Phdr = Elf_Phdr_Impl<ELFType<E, Is64>>;
  using Rel = Elf_Rel_Impl<ELFType<E, Is64>, false>;
  using Rela = Elf_Rel_Impl<ELFType<E, Is64>, true>;
  using Relr = packed<uint>;
  using Verdef = Elf_Verdef_Impl<ELFType<E, Is64>>;
  using Verdaux = Elf_Verdaux_Impl<ELFType<E, Is64>>;
  using Verneed = Elf_Verneed_Impl<ELFType<E, Is64>>;
  using Vernaux = Elf_Vernaux_Impl<ELFType<E, Is64>>;
  using Versym = Elf_Versym_Impl<ELFType<E, Is64>>;
  using Hash = Elf_Hash_Impl<ELFType<E, Is64>>;
  using GnuHash = Elf_GnuHash_Impl<ELFType<E, Is64>>;
  using Chdr = Elf_Chdr_Impl<ELFType<E, Is64>>;
  using Nhdr = Elf_Nhdr_Impl<ELFType<E, Is64>>;
  using Note = Elf_Note_Impl<ELFType<E, Is64>>;
  using NoteIterator = Elf_Note_Iterator_Impl<ELFType<E, Is64>>;
  using CGProfile = Elf_CGProfile_Impl<ELFType<E, Is64>>;
  using DynRange = ArrayRef<Dyn>;
  using ShdrRange = ArrayRef<Shdr>;
  using SymRange = ArrayRef<Sym>;
  using RelRange = ArrayRef<Rel>;
  using RelaRange = ArrayRef<Rela>;
  using RelrRange = ArrayRef<Relr>;
  using PhdrRange = ArrayRef<Phdr>;

  using Half = packed<uint16_t>;
  using Word = packed<uint32_t>;
  using Sword = packed<int32_t>;
  using Xword = packed<uint64_t>;
  using Sxword = packed<int64_t>;
  using Addr = packed<uint>;
  using Off = packed<uint>;
};

using ELF32LE = ELFType<support::little, false>;
using ELF32BE = ELFType<support::big, false>;
using ELF64LE = ELFType<support::little, true>;
using ELF64BE = ELFType<support::big, true>;

// Use an alignment of 2 for the typedefs since that is the worst case for
// ELF files in archives.

// I really don't like doing this, but the alternative is copypasta.
#define LLVM_ELF_IMPORT_TYPES_ELFT(ELFT)                                       \
  using Elf_Addr = typename ELFT::Addr;                                        \
  using Elf_Off = typename ELFT::Off;                                          \
  using Elf_Half = typename ELFT::Half;                                        \
  using Elf_Word = typename ELFT::Word;                                        \
  using Elf_Sword = typename ELFT::Sword;                                      \
  using Elf_Xword = typename ELFT::Xword;                                      \
  using Elf_Sxword = typename ELFT::Sxword;

#define LLVM_ELF_COMMA ,
#define LLVM_ELF_IMPORT_TYPES(E, W)                                            \
  LLVM_ELF_IMPORT_TYPES_ELFT(ELFType<E LLVM_ELF_COMMA W>)

// Section header.
template <class ELFT> struct Elf_Shdr_Base;

template <endianness TargetEndianness>
struct Elf_Shdr_Base<ELFType<TargetEndianness, false>> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, false)
  Elf_Word sh_name;      // Section name (index into string table)
  Elf_Word sh_type;      // Section type (SHT_*)
  Elf_Word sh_flags;     // Section flags (SHF_*)
  Elf_Addr sh_addr;      // Address where section is to be loaded
  Elf_Off sh_offset;     // File offset of section data, in bytes
  Elf_Word sh_size;      // Size of section, in bytes
  Elf_Word sh_link;      // Section type-specific header table index link
  Elf_Word sh_info;      // Section type-specific extra information
  Elf_Word sh_addralign; // Section address alignment
  Elf_Word sh_entsize;   // Size of records contained within the section
};

template <endianness TargetEndianness>
struct Elf_Shdr_Base<ELFType<TargetEndianness, true>> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, true)
  Elf_Word sh_name;       // Section name (index into string table)
  Elf_Word sh_type;       // Section type (SHT_*)
  Elf_Xword sh_flags;     // Section flags (SHF_*)
  Elf_Addr sh_addr;       // Address where section is to be loaded
  Elf_Off sh_offset;      // File offset of section data, in bytes
  Elf_Xword sh_size;      // Size of section, in bytes
  Elf_Word sh_link;       // Section type-specific header table index link
  Elf_Word sh_info;       // Section type-specific extra information
  Elf_Xword sh_addralign; // Section address alignment
  Elf_Xword sh_entsize;   // Size of records contained within the section
};

template <class ELFT>
struct Elf_Shdr_Impl : Elf_Shdr_Base<ELFT> {
  using Elf_Shdr_Base<ELFT>::sh_entsize;
  using Elf_Shdr_Base<ELFT>::sh_size;

  /// Get the number of entities this section contains if it has any.
  unsigned getEntityCount() const {
    if (sh_entsize == 0)
      return 0;
    return sh_size / sh_entsize;
  }
};

template <class ELFT> struct Elf_Sym_Base;

template <endianness TargetEndianness>
struct Elf_Sym_Base<ELFType<TargetEndianness, false>> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, false)
  Elf_Word st_name;       // Symbol name (index into string table)
  Elf_Addr st_value;      // Value or address associated with the symbol
  Elf_Word st_size;       // Size of the symbol
  unsigned char st_info;  // Symbol's type and binding attributes
  unsigned char st_other; // Must be zero; reserved
  Elf_Half st_shndx;      // Which section (header table index) it's defined in
};

template <endianness TargetEndianness>
struct Elf_Sym_Base<ELFType<TargetEndianness, true>> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, true)
  Elf_Word st_name;       // Symbol name (index into string table)
  unsigned char st_info;  // Symbol's type and binding attributes
  unsigned char st_other; // Must be zero; reserved
  Elf_Half st_shndx;      // Which section (header table index) it's defined in
  Elf_Addr st_value;      // Value or address associated with the symbol
  Elf_Xword st_size;      // Size of the symbol
};

template <class ELFT>
struct Elf_Sym_Impl : Elf_Sym_Base<ELFT> {
  using Elf_Sym_Base<ELFT>::st_info;
  using Elf_Sym_Base<ELFT>::st_shndx;
  using Elf_Sym_Base<ELFT>::st_other;
  using Elf_Sym_Base<ELFT>::st_value;

  // These accessors and mutators correspond to the ELF32_ST_BIND,
  // ELF32_ST_TYPE, and ELF32_ST_INFO macros defined in the ELF specification:
  unsigned char getBinding() const { return st_info >> 4; }
  unsigned char getType() const { return st_info & 0x0f; }
  uint64_t getValue() const { return st_value; }
  void setBinding(unsigned char b) { setBindingAndType(b, getType()); }
  void setType(unsigned char t) { setBindingAndType(getBinding(), t); }

  void setBindingAndType(unsigned char b, unsigned char t) {
    st_info = (b << 4) + (t & 0x0f);
  }

  /// Access to the STV_xxx flag stored in the first two bits of st_other.
  /// STV_DEFAULT: 0
  /// STV_INTERNAL: 1
  /// STV_HIDDEN: 2
  /// STV_PROTECTED: 3
  unsigned char getVisibility() const { return st_other & 0x3; }
  void setVisibility(unsigned char v) {
    assert(v < 4 && "Invalid value for visibility");
    st_other = (st_other & ~0x3) | v;
  }

  bool isAbsolute() const { return st_shndx == ELF::SHN_ABS; }

  bool isCommon() const {
    return getType() == ELF::STT_COMMON || st_shndx == ELF::SHN_COMMON;
  }

  bool isDefined() const { return !isUndefined(); }

  bool isProcessorSpecific() const {
    return st_shndx >= ELF::SHN_LOPROC && st_shndx <= ELF::SHN_HIPROC;
  }

  bool isOSSpecific() const {
    return st_shndx >= ELF::SHN_LOOS && st_shndx <= ELF::SHN_HIOS;
  }

  bool isReserved() const {
    // ELF::SHN_HIRESERVE is 0xffff so st_shndx <= ELF::SHN_HIRESERVE is always
    // true and some compilers warn about it.
    return st_shndx >= ELF::SHN_LORESERVE;
  }

  bool isUndefined() const { return st_shndx == ELF::SHN_UNDEF; }

  bool isExternal() const {
    return getBinding() != ELF::STB_LOCAL;
  }

  Expected<StringRef> getName(StringRef StrTab) const;
};

template <class ELFT>
Expected<StringRef> Elf_Sym_Impl<ELFT>::getName(StringRef StrTab) const {
  uint32_t Offset = this->st_name;
  if (Offset >= StrTab.size())
    return createStringError(object_error::parse_failed,
                             "st_name (0x%" PRIx32
                             ") is past the end of the string table"
                             " of size 0x%zx",
                             Offset, StrTab.size());
  return StringRef(StrTab.data() + Offset);
}

/// Elf_Versym: This is the structure of entries in the SHT_GNU_versym section
/// (.gnu.version). This structure is identical for ELF32 and ELF64.
template <class ELFT>
struct Elf_Versym_Impl {
  LLVM_ELF_IMPORT_TYPES_ELFT(ELFT)
  Elf_Half vs_index; // Version index with flags (e.g. VERSYM_HIDDEN)
};

/// Elf_Verdef: This is the structure of entries in the SHT_GNU_verdef section
/// (.gnu.version_d). This structure is identical for ELF32 and ELF64.
template <class ELFT>
struct Elf_Verdef_Impl {
  LLVM_ELF_IMPORT_TYPES_ELFT(ELFT)
  using Elf_Verdaux = Elf_Verdaux_Impl<ELFT>;
  Elf_Half vd_version; // Version of this structure (e.g. VER_DEF_CURRENT)
  Elf_Half vd_flags;   // Bitwise flags (VER_DEF_*)
  Elf_Half vd_ndx;     // Version index, used in .gnu.version entries
  Elf_Half vd_cnt;     // Number of Verdaux entries
  Elf_Word vd_hash;    // Hash of name
  Elf_Word vd_aux;     // Offset to the first Verdaux entry (in bytes)
  Elf_Word vd_next;    // Offset to the next Verdef entry (in bytes)

  /// Get the first Verdaux entry for this Verdef.
  const Elf_Verdaux *getAux() const {
    return reinterpret_cast<const Elf_Verdaux *>((const char *)this + vd_aux);
  }
};

/// Elf_Verdaux: This is the structure of auxiliary data in the SHT_GNU_verdef
/// section (.gnu.version_d). This structure is identical for ELF32 and ELF64.
template <class ELFT>
struct Elf_Verdaux_Impl {
  LLVM_ELF_IMPORT_TYPES_ELFT(ELFT)
  Elf_Word vda_name; // Version name (offset in string table)
  Elf_Word vda_next; // Offset to next Verdaux entry (in bytes)
};

/// Elf_Verneed: This is the structure of entries in the SHT_GNU_verneed
/// section (.gnu.version_r). This structure is identical for ELF32 and ELF64.
template <class ELFT>
struct Elf_Verneed_Impl {
  LLVM_ELF_IMPORT_TYPES_ELFT(ELFT)
  Elf_Half vn_version; // Version of this structure (e.g. VER_NEED_CURRENT)
  Elf_Half vn_cnt;     // Number of associated Vernaux entries
  Elf_Word vn_file;    // Library name (string table offset)
  Elf_Word vn_aux;     // Offset to first Vernaux entry (in bytes)
  Elf_Word vn_next;    // Offset to next Verneed entry (in bytes)
};

/// Elf_Vernaux: This is the structure of auxiliary data in SHT_GNU_verneed
/// section (.gnu.version_r). This structure is identical for ELF32 and ELF64.
template <class ELFT>
struct Elf_Vernaux_Impl {
  LLVM_ELF_IMPORT_TYPES_ELFT(ELFT)
  Elf_Word vna_hash;  // Hash of dependency name
  Elf_Half vna_flags; // Bitwise Flags (VER_FLAG_*)
  Elf_Half vna_other; // Version index, used in .gnu.version entries
  Elf_Word vna_name;  // Dependency name
  Elf_Word vna_next;  // Offset to next Vernaux entry (in bytes)
};

/// Elf_Dyn_Base: This structure matches the form of entries in the dynamic
///               table section (.dynamic) look like.
template <class ELFT> struct Elf_Dyn_Base;

template <endianness TargetEndianness>
struct Elf_Dyn_Base<ELFType<TargetEndianness, false>> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, false)
  Elf_Sword d_tag;
  union {
    Elf_Word d_val;
    Elf_Addr d_ptr;
  } d_un;
};

template <endianness TargetEndianness>
struct Elf_Dyn_Base<ELFType<TargetEndianness, true>> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, true)
  Elf_Sxword d_tag;
  union {
    Elf_Xword d_val;
    Elf_Addr d_ptr;
  } d_un;
};

/// Elf_Dyn_Impl: This inherits from Elf_Dyn_Base, adding getters.
template <class ELFT>
struct Elf_Dyn_Impl : Elf_Dyn_Base<ELFT> {
  using Elf_Dyn_Base<ELFT>::d_tag;
  using Elf_Dyn_Base<ELFT>::d_un;
  using intX_t = typename std::conditional<ELFT::Is64Bits,
                                           int64_t, int32_t>::type;
  using uintX_t = typename std::conditional<ELFT::Is64Bits,
                                            uint64_t, uint32_t>::type;
  intX_t getTag() const { return d_tag; }
  uintX_t getVal() const { return d_un.d_val; }
  uintX_t getPtr() const { return d_un.d_ptr; }
};

template <endianness TargetEndianness>
struct Elf_Rel_Impl<ELFType<TargetEndianness, false>, false> {
  LLVM_ELF_IMPORT_TYPES(TargetEndianness, false)
  static const bool IsRela = false;
  Elf_Addr r_offset; // Location (file byte offset, or program virtual addr)
  Elf_Word r_info;   // Symbol table index and type of relocation to apply

  uint32_t getRInfo(bool isMips64EL) const {
    assert(!isMips64EL);
    return r_info;
  }
  void setRInfo(uint32_t R, bool IsMips64EL) {
    assert(!IsMips64EL);
    r_info = R;
  }

  // These accessors and mutators correspond to the ELF32_R_SYM, ELF32_R_TYPE,
  // and ELF32_R_INFO macros defined in the ELF specification:
  uint32_t getSymbol(bool isMips64EL) const {
    return this->getRInfo(isMips64EL) >> 8;
  }
  unsigned char getType(bool isMips64EL) const {
    return (unsigned char)(this->getRInfo(isMips64EL) & 0x0ff);
  }
  void setSymbol(uint32_t s, bool IsMips64EL) {
    setSymbolAndType(s, getType(IsMips64EL), IsMips64EL);
  }
  void setType(unsigned char t, bool IsMips64EL) {
    setSymbolAndType(getSymbol(IsMips64EL), t, IsMips64EL);
  }
  void setSymbolAndType(uint32_t s, unsigned char t, bool IsMips64EL) {
    this->setRInfo((s << 8) + t, IsMips64EL);
  }
};

template <endianness TargetEndianness>
struct Elf_Rel_Impl<ELFType<TargetEndianness, false>, true>
    : public Elf_Rel_Impl<ELFType<TargetEndianness, false>, false> {
  LLVM_ELF_