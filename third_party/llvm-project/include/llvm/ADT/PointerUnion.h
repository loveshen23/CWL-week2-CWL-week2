//===- llvm/ADT/PointerUnion.h - Discriminated Union of 2 Ptrs --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the PointerUnion class, which is a discriminated union of
// pointer types.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_POINTERUNION_H
#define LLVM_ADT_POINTERUNION_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace llvm {

template <typename T> struct PointerUnionTypeSelectorReturn {
  using Return = T;
};

/// Get a type based on whether two types are the same or not.
///
/// For:
///
/// \code
///   using Ret = typename PointerUnionTypeSelector<T1, T2, EQ, NE>::Return;
/// \endcode
///
/// Ret will be EQ type if T1 is same as T2 or NE type otherwise.
template <typename T1, typename T2, typename RET_EQ, typename RET_NE>
struct PointerUnionTypeSelector {
  using Return = typename PointerUnionTypeSelectorReturn<RET_NE>::Return;
};

template <typename T, typename RET_EQ, typename RET_NE>
struct PointerUnionTypeSelector<T, T, RET_EQ, RET_NE> {
  using Return = typename PointerUnionTypeSelectorReturn<RET_EQ>::Return;
};

template <typename T1, typename T2, typename RET_EQ, typename RET_NE>
struct PointerUnionTypeSelectorReturn<
    PointerUnionTypeSelector<T1, T2, RET_EQ, RET_NE>> {
  using Return =
      typename PointerUnionTypeSelector<T1, T2, RET_EQ, RET_NE>::Return;
};

namespace pointer_union_detail {
  /// Determine the number of bits required to store integers with values < n.
  /// This is ceil(log2(n)).
  constexpr int bitsRequired(unsigned n) {
    return n > 1 ? 1 + bitsRequired((n + 1) / 2) : 0;
  }

  template <typename... Ts> constexpr int lowBitsAvailable() {
    return std::min<int>({PointerLikeTypeTraits<Ts>::NumLowBitsAvailable...});
  }

  /// Find the index of a type in a list of types. TypeIndex<T, Us...>::Index
  /// is the index of T in Us, or sizeof...(Us) if T does not appear in the
  /// list.
  template <typename T, typename ...Us> struct TypeIndex;
  template <typename T, typename ...Us> struct TypeIndex<T, T, Us...> {
    static constexpr int Index = 0;
  };
  template <typename T, typename U, typename... Us>
  struct TypeIndex<T, U, Us...> {
    static constexpr int Index = 1 + TypeIndex<T, Us...>::Index;
  };
  template <typename T> struct TypeIndex<T> {
    static constexpr int Index = 0;
  };

  /// Find the first type in a list of types.
  template <typename T, typename...> struct GetFirstType {
    using type = T;
  };

  /// Provide PointerLikeTypeTraits for void* that 