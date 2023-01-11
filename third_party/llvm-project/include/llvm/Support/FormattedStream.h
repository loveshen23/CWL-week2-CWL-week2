//===-- llvm/Support/FormattedStream.h - Formatted streams ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains raw_ostream implementations for streams to do
// things like pretty-print comments.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_FORMATTEDSTREAM_H
#define LLVM_SUPPORT_FORMATTEDSTREAM_H

#include "llvm/Support/raw_ostream.h"
#include <utility>

namespace llvm {

/// formatted_raw_ostream - A raw_ostream that wraps another one and keeps track
/// of line and column position, allowing padding out to specific column
/// boundaries and querying the number of lines written to the stream.
///
class formatted_raw_ostream : public raw_ostream {
  /// TheStream - The real stream we output to. We set it to be
  /// unbu