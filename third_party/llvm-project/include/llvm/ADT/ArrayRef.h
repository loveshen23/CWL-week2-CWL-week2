//===- ArrayRef.h - Array Reference Wrapper ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_ARRAYREF_H
#define LLVM_ADT_ARRAYREF_H

#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace llvm {

  /// ArrayRef - Represent a constant reference to an array (0 or more elements
  /// consecutively in memory), i.e. a start pointer and a length.  It allows
  /// various APIs to take consecutive elements easily and conveniently.
  ///
  /// This class does not own the underlying data, it is expected to be used in
  /// situations where the data resides in some other buffer, whose lifetime
  /// extends past that of the ArrayRef. For this reason, it is not in general
  /// safe to store an ArrayRef.
  ///
  /// This is intended to be trivially copyable, so it should be passed by
  /// value.
  template<typename T>
  class LLVM_NODISCARD ArrayRef {
  public:
    using iterator = const T *;
    using const_iterator = const T *;
    using size_type = size_t;
    using reverse_iterator = std::reverse_iterator<iterator>;

  private:
    /// The start of the array, in an external buffer.
    const T *Data = nullptr;

    /// The number of elements.
    size_type Length = 0;

  public:
    /// @name Constructors
    /// @{

    /// Construct an empty ArrayRef.
    /*implicit*/ ArrayRef() = default;

    /// Construct an empty ArrayRef from None.
    /*implicit*/ ArrayRef(NoneType) {}

    /// Construct an ArrayRef from a single element.
    /*implicit*/ ArrayRef(const T &OneElt)
      : Data(&OneElt), Length(1) {}

    /// Construct an ArrayRef from a pointer and length.
    /*implicit*/ ArrayRef(const T *data, size_t length)
      : Data(data), Length(length) {}

    /// Construct an ArrayRef from a range.
    ArrayRef(const T *begin, const T *end)
      : Data(begin), Length(end - begin) {}

    /// Construct an ArrayRef from a SmallVector. This is templated in order to
    /// avoid instantiating SmallVectorTemplateCommon<T> whenever we
    /// copy-construct an ArrayRef.
    template<typename U>
    /*implicit*/ ArrayRef(const SmallVectorTemplateCommon<T, U> &Vec)
      : Data(Vec.data()), Length(Vec.size()) {
    }

    /// Construct an ArrayRef from a std::vector.
    template<typename A>
    /*implicit*/ ArrayRef(const std::vector<T, A> &Vec)
      : Data(Vec.data()), Length(Vec.size()) {}

    /// Construct an ArrayRef from a std::array
    template <size_t N>
    /*implicit*/ constexpr ArrayRef(const std::array<T, N> &Arr)
        : Data(Arr.data()), Length(N) {}

    /// Construct an ArrayRef from a C array.
    template <size_t N>
    /*implicit*/ constexpr ArrayRef(const T (&Arr)[N]) : Data(Arr), Length(N) {}

    /// Construct an ArrayRef from a std::initializer_list.
    /*implicit*/ ArrayRef(const std::initializer_list<T> &Vec)
    : Data(Vec.begin() == Vec.end() ? (T*)nullptr : Vec.begin()),
      Length(Vec.size()) {}

    /// Construct an ArrayRef<const T*> from ArrayRef<T*>. This uses SFINAE to
    /// ensure that only ArrayRefs of pointers can be converted.
    template <typename U>
    ArrayRef(
        const ArrayRef<U *> &A,
        typename std::enable_if<
           std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : Data(A.data()), Length(A.size()) {}

    /// Construct an ArrayRef<const T*> from a SmallVector<T*>. This is
    /// templated in order to avoid instantiating SmallVectorTemplateCommon<T>
    /// whenever we copy-construct an ArrayRef.
    template<typename U, typename DummyT>
    /*implicit*/ ArrayRef(
      const SmallVectorTemplateCommon<U *, DummyT> &Vec,
      typename std::enable_if<
          std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : Data(Vec.data()), Length(Vec.size()) {
    }

    /// Construct an ArrayRef<const T*> from std::vector<T*>. This uses SFINAE
    /// to ensure that only vectors of pointers can be converted.
    template<typename U, typename A>
    ArrayRef(const std::vector<U *, A> &Vec,
             typename std::enable_if<
                 std::is_convertible<U *const *, T const *>::value>::type* = 0)
      : Data(Vec.data()), Length(Vec.size()) {}

    /// @}
    /// @name Simple Operations
    /// @{

    iterator begin() const { return Data; }
    iterator end() const { return Data + Length; }

    reverse_iterator rbegin() const { return reverse_iterator(end()); }
    reverse_iterator rend() const { return reverse_iterator(begin()); }

    /// empty - Check if the array is empty.
    bool empty() const { return Length == 0; }

    const T *data() const { return Data; }

    /// size - Get the array size.
    size_t size() const { 