//===- StringMap.h - String Hash table map interface ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the StringMap class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_STRINGMAP_H
#define LLVM_ADT_STRINGMAP_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "llvm/Support/ErrorHandling.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <utility>

namespace llvm {

template<typename ValueTy> class StringMapConstIterator;
template<typename ValueTy> class StringMapIterator;
template<typename ValueTy> class StringMapKeyIterator;

/// StringMapEntryBase - Shared base class of StringMapEntry instances.
class StringMapEntryBase {
  size_t StrLen;

public:
  explicit StringMapEntryBase(size_t Len) : StrLen(Len) {}

  size_t getKeyLength() const { return StrLen; }
};

/// StringMapImpl - This is the base class of StringMap that is shared among
/// all of its instantiations.
class StringMapImpl {
protected:
  // Array of NumBuckets pointers to entries, null pointers are holes.
  // TheTable[NumBuckets] contains a sentinel value for easy iteration. Followed
  // by an array of the actual hash values as unsigned integers.
  StringMapEntryBase **TheTable = nullptr;
  unsigned NumBuckets = 0;
  unsigned NumItems = 0;
  unsigned NumTombstones = 0;
  unsigned ItemSize;

protected:
  explicit StringMapImpl(unsigned itemSize)
      : ItemSize(itemSize) {}
  StringMapImpl(StringMapImpl &&RHS)
      : TheTable(RHS.TheTable), NumBuckets(RHS.NumBuckets),
        NumItems(RHS.NumItems), NumTombstones(RHS.NumTombstones),
        ItemSize(RHS.ItemSize) {
    RHS.TheTable = nullptr;
    RHS.NumBuckets = 0;
    RHS.NumItems = 0;
    RHS.NumTombstones = 0;
  }

  StringMapImpl(unsigned InitSize, unsigned ItemSize);
  unsigned RehashTable(unsigned BucketNo = 0);

  /// LookupBucketFor - Look up the bucket that the specified string should end
  /// up in.  If it already exists as a key in the map, the Item pointer for the
  /// specified bucket will be non-null.  Otherwise, it will be null.  In either
  /// case, the FullHashValue field of the bucket will be set to the hash value
  /// of the string.
  unsigned LookupBucketFor(StringRef Key);

  /// FindKey - Look up the bucket that contains the specified key. If it exists
  /// in the map, return the bucket number of the key.  Otherwise return -1.
  /// This does not modify the map.
  int FindKey(StringRef Key) const;

  /// RemoveKey - Remove the specified StringMapEntry from the table, but do not
  /// delete it.  This aborts if the value isn't in the table.
  void RemoveKey(StringMapEntryBase *V);

  /// RemoveKey - Remove the StringMapEntry for the specified key from the
  /// table, returning it.  If the key is not in the table, this returns null.
  StringMapEntryBase *RemoveKey(StringRef Key);

  /// Allocate the table with the specified number of buckets and otherwise
  /// setup the map as empty.
  void init(unsigned Size);

public:
  static StringMapEntryBase *getTombstoneVal() {
    uintptr_t Val = static_cast<uintptr_t>(-1);
    Val <<= PointerLikeTypeTraits<StringMapEntryBase *>::NumLowBitsAvailable;
    return reinterpret_cast<StringMapEntryBase *>(Val);
  }

  unsigned getNumBuckets() const { return NumBuckets; }
  unsigned getNumItems() const { return NumItems; }

  bool empty() const { return NumItems == 0; }
  unsigned size() const { return NumItems; }

  void swap(StringMapImpl &Other) {
    std::swap(TheTable, Other.TheTable);
    std::swap(NumBuckets, Other.NumBuckets);
    std::swap(NumItems, Other.NumItems);
    std::swap(NumTombstones, Other.NumTombstones);
  }
};

/// StringMapEntryStorage - Holds the value in a StringMapEntry.
///
/// Factored out into a separate base class to make it easier to specialize.
/// This is primarily intended to support StringSet, which doesn't need a value
/// stored at all.
template<typename ValueTy>
class StringMapEntryStorage : public StringMapEntryBase {
public:
  ValueTy second;

  explicit StringMapEntryStorage(size_t strLen)
    : StringMapEntryBase(strLen), second() {}
  template <typename... InitTy>
  StringMapEntryStorage(size_t strLen, InitTy &&... InitVals)
      : StringMapEntryBase(strLen), second(std::forward<InitTy>(InitVals)...) {}
  StringMapEntryStorage(StringMapEntryStorage &E) = delete;

  const ValueTy &getValue() const { return second; }
  ValueTy &getValue() { return second; }

  void setValue(const ValueTy &V) { second = V; }
};

template<>
class StringMapEntryStorage<NoneType> : public StringMapEntryBase {
public:
  explicit StringMapEntryStorage(size_t strLen, NoneType none = None)
    : StringMapEntryBase(strLen) {}
  StringMapEntryStorage(StringMapEntryStorage &E) = delete;

  NoneType getValue() const { return None; }
};

/// StringMapEntry - This is used to represent one value that is inserted into
/// a StringMap.  It contains the Value itself and the key: the string length
/// and data.
template<typename ValueTy>
class StringMapEntry final : public StringMapEntryStorage<ValueTy> {
public:
  using StringMapEntryStorage<ValueTy>::StringMapEntryStorage;

  StringRef getKey() const {
    return StringRef(getKeyData(), this->getKeyLength());
  }

  /// getKeyData - Return the start of the string data that is the key for this
  /// value.  The string data is always stored immediately after the
  /// StringMapEntry object.
  const char *getKeyData() const {return reinterpret_cast<const char*>(this+1);}

  StringRef first() const {
    return StringRef(getKeyData(), this->getKeyLength());
  }

  /// Create a StringMapEntry for the specified key construct the value using
  /// \p InitiVals.
  template <typename AllocatorTy, typename... InitTy>
  static StringMapEntry *Create(StringRef Key, AllocatorTy &Allocator,
                                InitTy &&... InitVals) {
    size_t KeyLength = Key.size();

    // Allocate a new item with space for the string at the end and a null
    // terminator.
    size_t AllocSize = sizeof(StringMapEntry) + KeyLength + 1;
    size_t Alignment = alignof(StringMapEntry);

    StringMapEntry *NewItem =
      static_cast<StringMapEntry*>(Allocator.Allocate(AllocSize,Alignment));
    assert(NewItem && "Unhandled out-of-memory");

    // Construct the value.
    new (New