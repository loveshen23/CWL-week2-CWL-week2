//===- llvm/Support/Error.h - Recoverable error handling --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines an API used to report recoverable errors.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_ERROR_H
#define LLVM_SUPPORT_ERROR_H

#include "llvm-c/Error.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Config/abi-breaking.h"
#include "llvm/Support/AlignOf.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <new>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace llvm {

class ErrorSuccess;

/// Base class for error info classes. Do not extend this directly: Extend
/// the ErrorInfo template subclass instead.
class ErrorInfoBase {
public:
  virtual ~ErrorInfoBase() = default;

  /// Print an error message to an output stream.
  virtual void log(raw_ostream &OS) const = 0;

  /// Return the error message as a string.
  virtual std::string message() const {
    std::string Msg;
    raw_string_ostream OS(Msg);
    log(OS);
    return OS.str();
  }

  /// Convert this error to a std::error_code.
  ///
  /// This is a temporary crutch to enable interaction with code still
  /// using std::error_code. It will be removed in the future.
  virtual std::error_code convertToErrorCode() const = 0;

  // Returns the class ID for this type.
  static const void *classID() { return &ID; }

  // Returns the class ID for the dynamic type of this ErrorInfoBase instance.
  virtual const void *dynamicClassID() const = 0;

  // Check whether this instance is a subclass of the class identified by
  // ClassID.
  virtual bool isA(const void *const ClassID) const {
    return ClassID == classID();
  }

  // Check whether this instance is a subclass of ErrorInfoT.
  template <typename ErrorInfoT> bool isA() const {
    return isA(ErrorInfoT::classID());
  }

private:
  virtual void anchor();

  static char ID;
};

/// Lightweight error class with error context and mandatory checking.
///
/// Instances of this class wrap a ErrorInfoBase pointer. Failure states
/// are represented by setting the pointer to a ErrorInfoBase subclass
/// instance containing information describing the failure. Success is
/// represented by a null pointer value.
///
/// Instances of Error also contains a 'Checked' flag, which must be set
/// before the destructor is called, otherwise the destructor will trigger a
/// runtime error. This enforces at runtime the requirement that all Error
/// instances be checked or returned to the caller.
///
/// There are two ways to set the checked flag, depending on what state the
/// Error instance is in. For Error instances indicating success, it
/// is sufficient to invoke the boolean conversion operator. E.g.:
///
///   @code{.cpp}
///   Error foo(<...>);
///
///   if (auto E = foo(<...>))
///     return E; // <- Return E if it is in the error state.
///   // We have verified that E was in the success state. It can now be safely
///   // destroyed.
///   @endcode
///
/// A success value *can not* be dropped. For example, just calling 'foo(<...>)'
/// without testing the return value will raise a runtime error, even if foo
/// returns success.
///
/// For Error instances representing failure, you must use either the
/// handleErrors or handleAllErrors function with a typed handler. E.g.:
///
///   @code{.cpp}
///   class MyErrorInfo : public ErrorInfo<MyErrorInfo> {
///     // Custom error info.
///   };
///
///   Error foo(<...>) { return make_error<MyErrorInfo>(...); }
///
///   auto E = foo(<...>); // <- foo returns failure with MyErrorInfo.
///   auto NewE =
///     handleErrors(E,
///       [](const MyErrorInfo &M) {
///         // Deal with the error.
///       },
///       [](std::unique_ptr<OtherError> M) -> Error {
///         if (canHandle(*M)) {
///           // handle error.
///           return Error::success();
///         }
///         // Couldn't handle this error instance. Pass it up the stack.
///         return Error(std::move(M));
///       );
///   // Note - we must check or return NewE in case any of the handlers
///   // returned a new error.
///   @endcode
///
/// The handleAllErrors function is identical to handleErrors, except
/// that it has a void return type, and requires all errors to be handled and
/// no new errors be returned. It prevents errors (assuming they can all be
/// handled) from having to be bubbled all the way to the top-level.
///
/// *All* Error instances must be checked before destruction, even if
/// they're moved-assigned or constructed from Success values that have already
/// been checked. This enforces checking through all levels of the call stack.
class LLVM_NODISCARD Error {
  // Both ErrorList and FileError need to be able to yank ErrorInfoBase
  // pointers out of this class to add to the error list.
  friend class ErrorList;
  friend class FileError;

  // handleErrors needs to be able to set the Checked flag.
  template <typename... HandlerTs>
  friend Error handleErrors(Error E, HandlerTs &&... Handlers);

  // Expected<T> needs to be able to steal the payload when constructed from an
  // error.
  template <typename T> friend class Expected;

  // wrap needs to be able to steal the payload.
  friend LLVMErrorRef wrap(Error);

protected:
  /// Create a success value. Prefer using 'Error::success()' for readability
  Error() {
    setPtr(nullptr);
    setChecked(false);
  }

public:
  /// Create a success value.
  static ErrorSuccess success();

  // Errors are not copy-constructable.
  Error(const Error &Other) = delete;

  /// Move-construct an error value. The newly constructed error is considered
  /// unchecked, even if the source error had been checked. The original error
  /// becomes a checked Success value, regardless of its original state.
  Error(Error &&Other) {
    setChecked(true);
    *this = std::move(Other);
  }

  /// Create an error value. Prefer using the 'make_error' function, but
  /// this constructor can be useful when "re-throwing" errors from handlers.
  Error(std::unique_ptr<ErrorInfoBase> Payload) {
    setPtr(Payload.release());
    setChecked(false);
  }

  // Errors are not copy-assignable.
  Error &operator=(const Error &Other) = delete;

  /// Move-assign an error value. The current error must represent success, you
  /// you cannot overwrite an unhandled error. The current error is then
  /// considered unchecked. The source error becomes a checked success value,
  /// regardless of its original state.
  Error &operator=(Error &&Other) {
    // Don't allow overwriting of unchecked values.
    assertIsChecked();
    setPtr(Other.getPtr());

    // This Error is unchecked, even if the source error was checked.
    setChecked(false);

    // Null out Other's payload and set its checked bit.
    Other.setPtr(nullptr);
    Other.setChecked(true);

    return *this;
  }

  /// Destroy a Error. Fails with a call to abort() if the error is
  /// unchecked.
  ~Error() {
    assertIsChecked();
    delete getPtr();
  }

  /// Bool conversion. Returns true if this Error is in a failure state,
  /// and false if it is in an accept state. If the error is in a Success state
  /// it will be considered checked.
  explicit operator bool() {
    setChecked(getPtr() == nullptr);
    return getPtr() != nullptr;
  }

  /// Check whether one error is a subclass of another.
  template <typename ErrT> bool isA() const {
    return getPtr() && getPtr()->isA(ErrT::classID());
  }

  /// Returns the dynamic class id of this error, or null if this is a success
  /// value.
  const void* dynamicClassID() const {
    if (!getPtr())
      return nullptr;
    return getPtr()->dynamicClassID();
  }

private:
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
  // assertIsChecked() happens very frequently, but under normal circumstances
  // is supposed to be a no-op.  So we want it to be inlined, but having a bunch
  // of debug prints can cause the function to be too large for inlining.  So
  // it's important that we define this function out of line so that it can't be
  // inlined.
  LLVM_ATTRIBUTE_NORETURN
  void fatalUncheckedError() const;
#endif

  void assertIsChecked() {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    if (LLVM_UNLIKELY(!getChecked() || getPtr()))
      fatalUncheckedError();
#endif
  }

  ErrorInfoBase *getPtr() const {
    return reinterpret_cast<ErrorInfoBase*>(
             reinterpret_cast<uintptr_t>(Payload) &
             ~static_cast<uintptr_t>(0x1));
  }

  void setPtr(ErrorInfoBase *EI) {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    Payload = reinterpret_cast<ErrorInfoBase*>(
                (reinterpret_cast<uintptr_t>(EI) &
                 ~static_cast<uintptr_t>(0x1)) |
                (reinterpret_cast<uintptr_t>(Payload) & 0x1));
#else
    Payload = EI;
#endif
  }

  bool getChecked() const {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    return (reinterpret_cast<uintptr_t>(Payload) & 0x1) == 0;
#else
    return true;
#endif
  }

  void setChecked(bool V) {
    Payload = reinterpret_cast<ErrorInfoBase*>(
                (reinterpret_cast<uintptr_t>(Payload) &
                  ~static_cast<uintptr_t>(0x1)) |
                  (V ? 0 : 1));
  }

  std::unique_ptr<ErrorInfoBase> takePayload() {
    std::unique_ptr<ErrorInfoBase> Tmp(getPtr());
    setPtr(nullptr);
    setChecked(true);
    return Tmp;
  }

  friend raw_ostream &operator<<(raw_ostream &OS, const Error &E) {
    if (auto P = E.getPtr())
      P->log(OS);
    else
      OS << "success";
    return OS;
  }

  ErrorInfoBase *Payload = nullptr;
};

/// Subclass of Error for the sole purpose of identifying the success path in
/// the type system. This allows to catch invalid conversion to Expected<T> at
/// compile time.
class ErrorSuccess final : public Error {};

inline ErrorSuccess Error::success() { return ErrorSuccess(); }

/// Make a Error instance representing failure using the given error info
/// type.
template <typename ErrT, typename... ArgTs> Error make_error(ArgTs &&... Args) {
  return Error(std::make_unique<ErrT>(std::forward<ArgTs>(Args)...));
}

/// Base class for user error types. Users should declare their error types
/// like:
///
/// class MyError : public ErrorInfo<MyError> {
///   ....
/// };
///
/// This class provides an implementation of the ErrorInfoBase::kind
/// method, which is used by the Error RTTI system.
template <typename ThisErrT, typename ParentErrT = ErrorInfoBase>
class ErrorInfo : public ParentErrT {
public:
  using ParentErrT::ParentErrT; // inherit constructors

  static const void *classID() { return &ThisErrT::ID; }

  const void *dynamicClassID() const override { return &ThisErrT::ID; }

  bool isA(const void *const ClassID) const override {
    return ClassID == classID() || ParentErrT::isA(ClassID);
  }
};

/// Special ErrorInfo subclass representing a list of ErrorInfos.
/// Instances of this class are constructed by joinError.
class ErrorList final : public ErrorInfo<ErrorList> {
  // handleErrors needs to be able to iterate the payload list of an
  // ErrorList.
  template <typename... HandlerTs>
  friend Error handleErrors(Error E, HandlerTs &&... Handlers);

  // joinErrors is implemented in terms of join.
  friend Error joinErrors(Error, Error);

public:
  void log(raw_ostream &OS) const override {
    OS << "Multiple errors:\n";
    for (auto &ErrPayload : Payloads) {
      ErrPayload->log(OS);
      OS << "\n";
    }
  }

  std::error_code convertToErrorCode() const override;

  // Used by ErrorInfo::classID.
  static char ID;

private:
  ErrorList(std::unique_ptr<ErrorInfoBase> Payload1,
            std::unique_ptr<ErrorInfoBase> Payload2) {
    assert(!Payload1->isA<ErrorList>() && !Payload2->isA<ErrorList>() &&
           "ErrorList constructor payloads should be singleton errors");
    Payloads.push_back(std::move(Payload1));
    Payloads.push_back(std::move(Payload2));
  }

  static Error join(Error E1, Error E2) {
    if (!E1)
      return E2;
    if (!E2)
      return E1;
    if (E1.isA<ErrorList>()) {
      auto &E1List = static_cast<ErrorList &>(*E1.getPtr());
      if (E2.isA<ErrorList>()) {
        auto E2Payload = E2.takePayload();
        auto &E2List = static_cast<ErrorList &>(*E2Payload);
        for (auto &Payload : E2List.Payloads)
          E1List.Payloads.push_back(std::move(Payload));
      } else
        E1List.Payloads.push_back(E2.takePayload());

      return E1;
    }
    if (E2.isA<ErrorList>()) {
      auto &E2List = static_cast<ErrorList &>(*E2.getPtr());
      E2List.Payloads.insert(E2List.Payloads.begin(), E1.takePayload());
      return E2;
    }
    return Error(std::unique_ptr<ErrorList>(
        new ErrorList(E1.takePayload(), E2.takePayload())));
  }

  std::vector<std::unique_ptr<ErrorInfoBase>> Payloads;
};

/// Concatenate errors. The resulting Error is unchecked, and contains the
/// ErrorInfo(s), if any, contained in E1, followed by the
/// ErrorInfo(s), if any, contained in E2.
inline Error joinErrors(Error E1, Error E2) {
  return ErrorList::join(std::move(E1), std::move(E2));
}

/// Tagged union holding either a T or a Error.
///
/// This class parallels ErrorOr, but replaces error_code with Error. Since
/// Error cannot be copied, this class replaces getError() with
/// takeError(). It also adds an bool errorIsA<ErrT>() method for testing the
/// error class type.
template <class T> class LLVM_NODISCARD Expected {
  template <class T1> friend class ExpectedAsOutParameter;
  template <class OtherT> friend class Expected;

  static const bool isRef = std::is_reference<T>::value;

  using wrap = std::reference_wrapper<typename std::remove_reference<T>::type>;

  using error_type = std::unique_ptr<ErrorInfoBase>;

public:
  using storage_type = typename std::conditional<isRef, wrap, T>::type;
  using value_type = T;

private:
  using reference = typename std::remove_reference<T>::type &;
  using const_reference = const typename std::remove_reference<T>::type &;
  using pointer = typename std::remove_reference<T>::type *;
  using const_pointer = const typename std::remove_reference<T>::type *;

public:
  /// Create an Expected<T> error value from the given Error.
  Expected(Error Err)
      : HasError(true)
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
        // Expected is unchecked upon construction in Debug builds.
        , Unchecked(true)
#endif
  {
    assert(Err && "Cannot create Expected<T> from Error success value.");
    new (getErrorStorage()) error_type(Err.takePayload());
  }

  /// Forbid to convert from Error::success() implicitly, this avoids having
  /// Expected<T> foo() { return Error::success(); } which compiles otherwise
  /// but triggers the assertion above.
  Expected(ErrorSuccess) = delete;

  /// Create an Expected<T> success value from the given OtherT value, which
  /// must be convertible to T.
  template <typename OtherT>
  Expected(OtherT &&Val,
           typename std::enable_if<std::is_convertible<OtherT, T>::value>::type
               * = nullptr)
      : HasError(false)
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
        // Expected is unchecked upon construction in Debug builds.
        , Unchecked(true)
#endif
  {
    new (getStorage()) storage_type(std::forward<OtherT>(Val));
  }

  /// Move construct an Expected<T> value.
  Expected(Expected &&Other) { moveConstruct(std::move(Other)); }

  /// Move construct an Expected<T> value from an Expected<OtherT>, where OtherT
  /// must be convertible to T.
  template <class OtherT>
  Expected(Expected<OtherT> &&Other,
           typename std::enable_if<std::is_convertible<OtherT, T>::value>::type
               * = nullptr) {
    moveConstruct(std::move(Other));
  }

  /// Move construct an Expected<T> value from an Expected<OtherT>, where OtherT
  /// isn't convertible to T.
  template <class OtherT>
  explicit Expected(
      Expected<OtherT> &&Other,
      typename std::enable_if<!std::is_convertible<OtherT, T>::value>::type * =
          nullptr) {
    moveConstruct(std::move(Other));
  }

  /// Move-assign from another Expected<T>.
  Expected &operator=(Expected &&Other) {
    moveAssign(std::move(Other));
    return *this;
  }

  /// Destroy an Expected<T>.
  ~Expected() {
    assertIsChecked();
    if (!HasError)
      getStorage()->~storage_type();
    else
      getErrorStorage()->~error_type();
  }

  /// Return false if there is an error.
  explicit operator bool() {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    Unchecked = HasError;
#endif
    return !HasError;
  }

  /// Returns a reference to the stored T value.
  reference get() {
    assertIsChecked();
    return *getStorage();
  }

  /// Returns a const reference to the stored T value.
  const_reference get() const {
    assertIsChecked();
    return const_cast<Expected<T> *>(this)->get();
  }

  /// Check that this Expected<T> is an error of type ErrT.
  template <typename ErrT> bool errorIsA() const {
    return HasError && (*getErrorStorage())->template isA<ErrT>();
  }

  /// Take ownership of the stored error.
  /// After calling this the Expected<T> is in an indeterminate state that can
  /// only be safely destructed. No further calls (beside the destructor) should
  /// be made on the Expected<T> value.
  Error takeError() {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    Unchecked = false;
#endif
    return HasError ? Error(std::move(*getErrorStorage())) : Error::success();
  }

  /// Returns a pointer to the stored T value.
  pointer operator->() {
    assertIsChecked();
    return toPointer(getStorage());
  }

  /// Returns a const pointer to the stored T value.
  const_pointer operator->() const {
    assertIsChecked();
    return toPointer(getStorage());
  }

  /// Returns a reference to the stored T value.
  reference operator*() {
    assertIsChecked();
    return *getStorage();
  }

  /// Returns a const reference to the stored T value.
  const_reference operator*() const {
    assertIsChecked();
    return *getStorage();
  }

private:
  template <class T1>
  static bool compareThisIfSameType(const T1 &a, const T1 &b) {
    return &a == &b;
  }

  template <class T1, class T2>
  static bool compareThisIfSameType(const T1 &a, const T2 &b) {
    return false;
  }

  template <class OtherT> void moveConstruct(Expected<OtherT> &&Other) {
    HasError = Other.HasError;
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    Unchecked = true;
    Other.Unchecked = false;
#endif

    if (!HasError)
      new (getStorage()) storage_type(std::move(*Other.getStorage()));
    else
      new (getErrorStorage()) error_type(std::move(*Other.getErrorStorage()));
  }

  template <class OtherT> void moveAssign(Expected<OtherT> &&Other) {
    assertIsChecked();

    if (compareThisIfSameType(*this, Other))
      return;

    this->~Expected();
    new (this) Expected(std::move(Other));
  }

  pointer toPointer(pointer Val) { return Val; }

  const_pointer toPointer(const_pointer Val) const { return Val; }

  pointer toPointer(wrap *Val) { return &Val->get(); }

  const_pointer toPointer(const wrap *Val) const { return &Val->get(); }

  storage_type *getStorage() {
    assert(!HasError && "Cannot get value when an error exists!");
    return reinterpret_cast<storage_type *>(TStorage.buffer);
  }

  const storage_type *getStorage() const {
    assert(!HasError && "Cannot get value when an error exists!");
    return reinterpret_cast<const storage_type *>(TStorage.buffer);
  }

  error_type *getErrorStorage() {
    assert(HasError && "Cannot get error when a value exists!");
    return reinterpret_cast<error_type *>(ErrorStorage.buffer);
  }

  const error_type *getErrorStorage() const {
    assert(HasError && "Cannot get error when a value exists!");
    return reinterpret_cast<const error_type *>(ErrorStorage.buffer);
  }

  // Used by ExpectedAsOutParameter to reset the checked flag.
  void setUnchecked() {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    Unchecked = true;
#endif
  }

#if LLVM_ENABLE_ABI_BREAKING_CHECKS
  LLVM_ATTRIBUTE_NORETURN
  LLVM_ATTRIBUTE_NOINLINE
  void fatalUncheckedExpected() const {
    dbgs() << "Expected<T> must be checked before access or destruction.\n";
    if (HasError) {
      dbgs() << "Unchecked Expected<T> contained error:\n";
      (*getErrorStorage())->log(dbgs());
    } else
      dbgs() << "Expected<T> value was in success state. (Note: Expected<T> "
                "values in success mode must still be checked prior to being "
                "destroyed).\n";
    abort();
  }
#endif

  void assertIsChecked() {
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
    if (LLVM_UNLIKELY(Unchecked))
      fatalUncheckedExpected();
#endif
  }

  union {
    AlignedCharArrayUnion<storage_type> TStorage;
    AlignedCharArrayUnion<error_type> ErrorStorage;
  };
  bool HasError : 1;
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
  bool Unchecked : 1;
#endif
};

/// Report a serious error, calling any installed error handler. See
/// ErrorHandling.h.
LLVM_ATTRIBUTE_NORETURN void report_fatal_error(Error Err,
                                                bool gen_crash_diag = true);

/// Report a fatal error if Err is a failure value.
///
/// This function can be used to wrap calls to fallible functions ONLY when it
/// is known that the Error will always be a success value. E.g.
///
///   @code{.cpp}
///   // foo only attempts the fallible operation if DoFallibleOperation is
///   // true. If DoFallibleOperation is false then foo always returns
///   // Error::success().
///   Error foo(bool DoFallibleOperation);
///
///   cantFail(foo(false));
///   @endcode
inline void cantFail(Error Err, const char *Msg = nullptr) {
  if (Err) {
    if (!Msg)
      Msg = "Failure value returned from cantFail wrapped call";
#ifndef NDEBUG
    std::string Str;
    raw_string_ostream OS(Str);
