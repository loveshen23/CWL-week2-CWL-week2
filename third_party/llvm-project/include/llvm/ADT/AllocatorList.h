//===- llvm/ADT/AllocatorList.h - Custom allocator list ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_ALLOCATORLIST_H
#define LLVM_ADT_ALLOCATORLIST_H

#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/simple_ilist.h"
#include "llvm/Support/Allocator.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace llvm {

/// A linked-list with a custom, local allocator.
///
/// Expose a std::list-like interface that owns and uses a custom LLVM-style
/// allocator (e.g., BumpPtrAllocator), leveraging \a simple_ilist for the
/// implementation details.
///
/// Because this list owns the allocator, 