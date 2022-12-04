//===-- llvm/Support/Compiler.h - Compiler abstraction support --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines several macros, based on the current compiler.  This allows
// use of compiler-specific features in a way that remains portable. This header
// can be included from either C or C++.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_COMPILER_H
#define LLVM_SUPPORT_COMPILER_H

#include "llvm/Config/llvm-config.h"

#ifdef __cplusplus
#include <new>
#endif
#include <stddef.h>

#if defined(_MSC_VER)
#include <sal.h>
#endif

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#ifndef __has_extension
# define __has_extension(x) 0
#endif

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

// Only use __has_cpp_attribute in C++ mode. GCC defines __has_cpp_attribute in
// C mode, but the :: in __has_cpp_attribute(scoped::attribute) is invalid.
#ifndef LLVM_HAS_CPP_ATTRIBUTE
#if defined(__cplusplus) && defined(__has_cpp_attribute)
# define LLVM_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
# define LLVM_HAS_CPP_ATTRIBUTE(x) 0
#endif
#endif

/// \macro LLVM_GNUC_PREREQ
/// Extend the default __GNUC_PREREQ even if glibc's features.h isn't
/// available.
#ifndef LLVM_GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#  define LLVM_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >= \
     ((maj) << 20) + ((min) << 10) + (patch))
# elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define LLVM_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
# else
#  define LLVM_GNUC_PREREQ(maj, min, patch) 0
# endif
#endif

/// \macro LLVM_MSC_PREREQ
/// Is the compiler MSVC of at least the specified version?
/// The common \param version values to check for are:
/// * 1910: VS2017, version 15.1 & 15.2
/// * 1911: VS2017, version 15.3 & 15.4
/// * 1912: VS2017, version 15.5
/// * 1913: VS2017, version 15.6
/// * 1914: VS2017, version 15.7
/// * 1915: VS2017, version 15.8
/// * 1916: VS2017, version 15.9
/// * 1920: VS2019, version 16.0
/// * 1921: VS2019, version 16.1
#ifdef _MSC_VER
#define LLVM_MSC_PREREQ(version) (_MSC_VER >= (version))

// We require at least MSVC 2017.
#if !LLVM_MSC_PREREQ(1910)
#error LLVM requires at least MSVC 2017.
#endif

#else
#define LLVM_MSC_PREREQ(version) 0
#endif

/// Does the compiler support ref-qualifiers for *this?
///
/// Sadly, this is separate from just rvalue reference support because GCC
/// and MSVC implemented this later than everything else.
#if __has_feature(cxx_rvalue_references) || LLVM_GNUC_PREREQ(4, 8, 1)
#define LLVM_HAS_RVALUE_REFERENCE_THIS 1
#else
#define LLVM_HAS_RVALUE_REFERENCE_THIS 0
#endif

/// Expands to '&' if ref-qualifiers for *this are supported.
///
/// This can be used to provide lvalue/rvalue overrides of member functions.
/// The rvalue override should be guarded by LLVM_HAS_RVALUE_REFERENCE_THIS
#if LLVM_HAS_RVALUE_REFERENCE_THIS
#define LLVM_LVALUE_FUNCTION &
#else
#define LLVM_LVALUE_FUNCTION
#endif

/// LLVM_LIBRARY_VISIBILITY - If a class marked with this attribute is linked
/// into a shared library, then the class should be private to the library and
/// not accessible from outside it.  Can also be used to mark variables and
/// functions, making them private to any shared library they are linked into.
/// On PE/COFF targets, library visibility is the default, so this isn't needed.
#if (__has_attribute(visibility) || LLVM_GNUC_PREREQ(4, 0, 0)) &&              \
    !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(_WIN32)
#define LLVM_LIBRARY_VISIBILITY __attribute__ ((visibility("hidden")))
#else
#define LLVM_LIBRARY_VISIBILITY
#endif

#if defined(__GNUC__)
#define LLVM_PREFETCH(addr, rw, locality) __builtin_prefetch(addr, rw, locality)
#else
#define LLVM_PREFETCH(addr, rw, locality)
#endif

#if __has_attribute(used) || LLVM_GNUC_PREREQ(3, 1, 0)
#define LLVM_ATTRIBUTE_USED __attribute__((__used__))
#else
#define LLVM_ATTRIBUTE_USED
#endif

/// LLVM_NODISCARD - Warn if a type or return value is discarded.

// Use the 'nodiscard' attribute in C++17 or newer mode.
#if __cplusplus > 201402L && LLVM_HAS_CPP_ATTRIBUTE(nodiscard)
#define LLVM_NODISCARD [[nodiscard]]
#elif LLVM_HAS_CPP_ATTRIBUTE(clang::warn_unused_result)
#define LLVM_NODISCARD [[clang::warn_unused_result]]
// Clang in C++14 mode claims that it has the 'nodiscard' attribute, but also
// warns in the pedantic mode that 'nodiscard' is a C++17 extension (PR33518).
// Use the 'nodiscard' attribute in C++14 mode only with GCC.
// TODO: remove this workaround when PR33518 is resolved.
#elif defined(__GNUC__) && LLVM_HAS_CPP_ATTRIBUTE(nodiscard)
#define LLVM_NODISCARD [[nodiscard]]
#else
#define LLVM_NODISCARD
#endif

// Indicate that a non-static, non-const C++ member function reinitializes
// the entire object to a known state, independent of the previous state of
// the object.
//
// The clang-tidy check bugprone-use-after-move recognizes this attribute as a
// marker that a moved-from object has left the indeterminate state and can be
// reused.
#if LLVM_HAS_CPP_ATTRIBUTE(clang::reinitializes)
#define LLVM_ATTRIBUTE_REINITIALIZES [[clang::reinitializes]]
#else
#define LLVM_ATTRIBUTE_REINITIALIZES
#endif

// Some compilers warn about unused functions. When a function is sometimes
// used or not depending on build settings (e.g. a function only called from
// within "assert"), this attribute can be used to suppress such warnings.
//
// However, it shouldn't be used for unused *variables*, as those have a much
// more portable solution:
//   (void)unused_var_name;
// Prefer cast-to-void wherever it is sufficient.
#if __has_attribute(unused) || LLVM_GNUC_PREREQ(3, 1, 0)
#define LLVM_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define LLVM_ATTRIBUTE_UNUSED
#endif

// FIXME: Provide this for PE/COFF targets.
#if (__has_attribute(weak) || LLVM_GNUC_PREREQ(4, 0, 0)) &&                    \
    (!defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(_WIN32))
#define LLVM_ATTRIBUTE_WEAK __attribute__((__weak__))
#else
#define LLVM_ATTRIBUTE_WEAK
#endif

// Prior to clang 3.2, clang did not accept any spelling of
// __has_attribute(const), so assume it is supported.
#if defined(__clang__) || defined(__GNUC__)
// aka 'CONST' but following LLVM Conventions.
#define LLVM_READNONE __attribute__((__const__))
#else
#define LLVM_READNONE
#endif

#if __has_attribute(pure) || defined(__GNUC__)
// aka 'PURE' but following LLVM Conventions.
#define LLVM_READONLY __attribute__((__pure__))
#else
#define LLVM_READONLY
#endif

#if __has_builtin(__builtin_expect) || LLVM_GNUC_PREREQ(4, 0, 0)
#define LLVM_LIKELY(EXPR) __builtin_expect((bool)(EXPR), true)
#define LLVM_UNLIKELY(EXPR) __builtin_expect((bool)(EXPR), false)
#else
#define LLVM_LIKELY(EXPR) (EXPR)
#define LLVM_UNLIKELY(EXPR) (EXPR)
#endif

/// LLVM_ATTRIBUTE_NOINLINE - On compilers where we have a directive to do so,
/// mark a method "not for inlining".
#if __has_attribute(noinline) || LLVM_GNUC_PREREQ(3, 4, 0)
#define LLVM_ATTRIBUTE_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define LLVM_ATTRIBUTE_NOINLINE __declspec(noinline)
#else
#define LLVM_ATTRIBUTE_NOINLINE
#endif

/// LLVM_ATTRIBUTE_ALWAYS_INLINE - On compilers where we have a directive to do
/// so, mark a method "always inline" because it is performance sensitive. GCC
/// 3.4 supported this but is buggy in various cases and produces unimplemented
/// errors, just use it in GCC 4.0 and later.
#if __has_attribute(always_inline) || LLVM_GNUC_PREREQ(4, 0, 0)
#define LLVM_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define LLVM_ATTRIBUTE_ALWAYS_INLINE __forceinline
#else
#define LLVM_ATTRIBUTE_ALWAYS_INLINE
#endif

#ifdef __GNUC__
#define LLVM_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define LLVM_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define LLVM_ATTRIBUTE_NORETURN
#endif

#if __has_attribute(returns_nonnull) || LLVM_GNUC_PREREQ(4, 9, 0)
#define LLVM_ATTRIBUTE_RETURNS_NONNULL __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#define LLVM_ATTRIBUTE_RETURNS_NONNULL _Ret_notnull_
#else
#define LLVM_ATTRIBUTE_RETURNS_NONNULL
#endif

/// \macro LLVM_ATTRIBUTE_RETURNS_NOALIAS Used to mark a function as returning a
/// pointer that does not alias any other valid pointer.
#ifdef __GNUC__
#define LLVM_ATTRIBUTE_RETURNS_NOALIAS __attribute__((__malloc__))
#elif defined(_MSC_VER)
#define LLVM_ATTRIBUTE_RETURNS_NOALIAS __declspec(restrict)
#else
#define LLVM_ATTRIBUTE_RETURNS_NOALIAS
#endif

/// LLVM_FALLTHROUGH - Mark fallthrough cases in switch statements.
#if __cplusplus > 201402L && LLVM_HAS_CPP_ATTRIBUTE(fallthrough)
#define LLVM_FALLTHROUGH [[fallthrough]]
#elif LLVM_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
#define LLVM_FALLTHROUGH [[gnu::fallthrough]]
#elif __has_attribute(fallthrough)
#define LLVM_FALLTHROUGH __attribu