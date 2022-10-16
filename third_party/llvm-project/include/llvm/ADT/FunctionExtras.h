//===- FunctionExtras.h - Function type erasure utilities -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file provides a collection of function (or more generally, callable)
/// type erasure utilities supplementing those provided by the standard library
/// in `<function>`.
///
/// It provides `unique_function`, which works like `std::function` but supports
/// move-only callable objects.
///
/// Future plans:
/// - Add a `function` that provides const, volatile, and ref-qualified support,
///   which doesn't work with `std::function`.
/// - Provide support for specifying multiple signatures to type erase callable
///   objects with an overload set, such as those produced by generic lambdas.
/// - Expand to include a copyable utility that directly replaces std::function
///   but brings the above improvements.
///
/// Note that LLVM's utilities are greatly simplified by not supporting
/// allocators.
///
/// If the standard library ever begins to provide comparable facilities we can
/// consider switching to those.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_FUNCTION_EXTRAS_H
#define LLVM_ADT_FUNCTION_EXTRAS_H

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/type_traits.h"
#include <memory>

namespace llvm {

template <typename FunctionT> class unique_function;

template <typename ReturnT, typename... ParamTs>
class unique_function<ReturnT(ParamTs...)> {
  static constexpr size_t InlineStorageSize = sizeof(void *) * 3;

  // MSVC has a bug and ICEs if we give it a particular dependent value
  // expression as part of the `std::conditional` below. To work around this,
  // we build that into a template struct's constexpr bool.
  template <typename T> struct IsSizeLessThanThresholdT {
    static constexpr bool value = sizeof(T) <= (2 * sizeof(void *));
  };

  // Provide a type function to map parameters that won't observe extra copies
  // or moves and which are small enough to likely pass in register to values
  // and all other types to l-value reference types. We use this to compute the
  // types used in our erased call utility to minimize copies and moves unless
  // doing so would force things unnecessarily into memory.
  //
  // The heuristic used is related to common ABI register passing conventions.
  // It doesn't have to be exact though, and in one way it is more strict
  // because we want to still be able to observe either moves *or* copies.
  template <typename T>
  using AdjustedParamT = typename std::conditional<
      !std::is_reference<T>::value &&
          llvm::is_trivially_copy_constructible<T>::value &&
          llvm::is_trivially_move_constructible<T>::value &&
          IsSizeLessThanThresholdT<T>::value,
      T, T &>::type;

  // The type of the erased function pointer we use as a callback to dispatch to
  // the stored callable when it is trivial to move and destroy.
  using CallPtrT = ReturnT (*)(void *CallableAddr,
                               AdjustedParamT<ParamTs>... Params);
  using MovePtrT = void (*)(void *LHSCallableAddr, void *RHSCallableAddr);
  using DestroyPtrT = void (*)(void *CallableAddr);

  /// A struct to hold a single trivial callback with sufficient alignment for
  /// our bitpacking.
  struct alignas(8) TrivialCallback {
    CallPtrT CallPtr;
  };

  /// A struct we use to aggregate three callbacks when we need full set of
  /// operations.
  struct alignas(8) NonTrivialCallbacks {
    CallPtrT CallPtr;
    MovePtrT MovePtr;
    DestroyPtrT DestroyPtr;
  };

  // Create a pointer union between either a pointer to a static trivial call
  // pointer in a struct or a pointer to a static struct of the call, move, and
  // destroy pointers.
  using CallbackPointerUnionT =
      PointerUnion<TrivialCallback *, NonTrivialCallbacks *>;

  // The main storage buffer. This will either have a pointer to out-of-line
  // storage or an inline buffer storing the callable.
  union StorageUnionT {
    // For out-of-line storage we keep a pointer to the underlying storage and
    // the size. This is enough to deallocate the memory.
    struct OutOfLineStorageT {
      void *StoragePtr;
      size_t Size;
      size_t Alignment;
    } OutOfLineStorage;
    static_assert(
        sizeof(OutOfLineStorageT) <= InlineStorageSize,
        "Should always use all of the out-of-line storage for inline storage!");

    // For in-line storage, we just provide an aligned character buffer. We
    // provide three pointers worth of storage here.
    typename std::aligned_storage<InlineStorageSize, alignof(void *)>::type
        InlineStorage;
  } StorageUnion;

  // A compressed pointer to either our dispatching callback or our table of
  // dispatching callbacks and the flag for whether the callable itself is
  // stored inline or not.
  PointerIntPair<CallbackPointerUnionT, 1, bool> CallbackAndInlineFlag;

  bool isInlineStorage() const { return CallbackAndInlineFlag.getInt(); }

  bool isTrivialCallback() const {
    return CallbackAndInlineFlag.getPointer().template is<TrivialCallback *>();
  }

  CallPtrT getTrivialCallback() const {
    return CallbackAndInlineFlag.getPointer().template get<TrivialCallback *>()->CallPtr;
  }

  NonTrivialCallbacks *getNonTrivialCallbacks() const {
    return CallbackAndInlineFlag.getPointer()
        .template get<NonTrivialCallbacks *>();
  }

  void *getInlineStorage() { return &StorageUnion.InlineStorage; }

  void *getOutOfLineStorage() {
    return StorageUnion.OutOfLineStorage.StoragePtr;
  }
  size_t getOutOfLineStorageSize() const {
    return StorageUnion.OutOfLineStorage.Size;
  }
  size_t getOutOfLineStorageAlignment() const {
    return StorageUnion.OutOfLineStorage.Alignment;
  }

  void setOutOfLineStorage(void *Ptr, size_t Size, size_t Alignment) {
    StorageUnion.OutOfLineStorage = {Ptr, Size, Alignment};
  }

  template <typename CallableT>
  static ReturnT CallImpl(void *CallableAddr, AdjustedParamT<ParamTs>... Params) {
    return (*reinterpret_cast<CallableT *>(CallableAddr))(
        std::forward<ParamTs>(Params)...);
  }

  template <typename CallableT>
  static void MoveImpl(void *LHSCallableAddr, void *RHSCallableAddr) noexcept {
    new (LHSCallableAddr)
        CallableT(std::move(*reinterpret_cast<CallableT *>(RHSCallableAddr)));
  }

  template <typename CallableT>
  static void DestroyImpl(void *CallableAddr) noexcept {
    reinterpret_cast<CallableT *>(CallableAddr)->~CallableT();
  }

public:
  unique_function() = default;
  unique_function(std::nullptr_t /*null_callable*/) {}

  ~unique_function() {
    if (!CallbackAndInlineFlag.getPointer())
      return;

    // Cache this value so we don't re-check it after type-erased operations.
    bool IsInlineStorage = isInlineStorage();

    if (!isTrivialCallback())
      getNonTrivialCallbacks()->DestroyPtr(
          IsInlineStorage ? getInlineStorage() : getOutOfLineStorage());

    if (!IsInlineStorage)
      deallocate_buffer(getOutOfLineStorage(), getOutOfLineStorageSize(),
                        getOutOfLineStorageAlignment());
  }

  unique_function(unique_function &&RHS) noexcept {
    // Copy the callback and inline flag.
    CallbackAndInlineFlag = RHS.CallbackAndInlineFlag;

    // If the RHS is empty, just copying the above is sufficient.
    if (!RHS)
      return;

    if (!isInlineStorage()) {
      // The out-of-line case is easiest to move.
      StorageUnion.OutOfLineStorage = RHS.StorageUnion.OutOfLineStorage;
    } else if (isTrivialCallback()) {
      // Move is trivial, just memcpy the bytes across.
      memcpy(getInlineStorage(), RHS.getInlineStorage(), InlineStorageSize);
    } else {
      // Non-trivial move, so dispatch to a type-erased implementation.
      getNonTrivialCallbacks()->MovePtr(getInlineStorage(),
                                        RHS.getInlineStorage());
    }

    // Clear the old callback and inline flag to get back to as-if-null.
    RHS.CallbackAndInlineFlag = {};

#ifndef NDEBUG
    // In debug builds, we also scribble across the rest of the storage.
    memset(RHS.getInlineStorag