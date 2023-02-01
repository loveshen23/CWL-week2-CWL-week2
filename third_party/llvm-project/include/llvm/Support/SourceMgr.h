//===- SourceMgr.h - Manager for Source Buffers & Diagnostics ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the SMDiagnostic and SourceMgr classes.  This
// provides a simple substrate for diagnostics, #include handling, and other low
// level things for simple parsers.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_SOURCEMGR_H
#define LLVM_SUPPORT_SOURCEMGR_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace llvm {

class raw_ostream;
class SMDiagnostic;
class SMFixIt;

/// This owns the files read by a parser, handles include stacks,
/// and handles diagnostic wrangling.
class SourceMgr {
public:
  enum DiagKind {
    DK_Error,
    DK_Warning,
    DK_Remark,
    DK_Note,
  };

  /// Clients that want to handle their own diagnostics in a custom way can
  /// register a function pointer+context as a diagnostic handler.
  /// It gets called each time PrintMessage is invoked.
  using DiagHandlerTy = void (*)(const SMDiagnostic &, void *Context);

private:
  struct SrcBuffer {
    /// The memory buffer for the file.
    std::unique_ptr<MemoryBuffer> Buffer;

    /// Helper type for OffsetCache below: since we're storing many offsets
    /// into relatively small files (often smaller than 2^8 or 2^16 bytes),
    /// we select the offset vector element type dynamically based on the
    /// size of Buffer.
    using VariableSizeOffsets = PointerUnion4<std::vector<uint8_t> *,
                                              std::vector<uint16_t> *,
                                              std::vector<uint32_t> *,
                                              std::vector<uint64_t> *>;

    /// Vector of offsets into Buffer at which there are line-endings
    /// (lazily populated). Once populated, the '\n' that marks the end of
    /// line number N from [1..] is at Buffer[OffsetCache[N-1]]. Since
    /// these offsets are in sorted (ascending) order, they can be
    /// binary-searched for the first one after any given offset (eg. an
    /// offset corresponding to a particular SMLoc).
    mutable VariableSizeOffsets OffsetCache;

    /// Populate \c OffsetCache and look up a given \p Ptr in it, assuming
    /// it points somewhere into \c Buffer. The static type parameter \p T
    /// must be an unsigned integer type from uint{8,16,32,64}_t large
    /// enough to store offsets inside \c Buffer.
    template<typename T>
    unsigned getLineNumber(const char *Ptr) const;

    /// This is the location of the parent include, or null if at the top level.
    SMLoc IncludeLoc;

    SrcBuffer() = default;
    SrcBuffer(SrcBuffer &&);
    SrcBuffer(const SrcBuffer &) = delete;
    SrcBuffer &operator=(const SrcBuffer &) = delete;
    ~SrcBuffer();
  };

  /// This is all of the buffers that we are reading from.
  std::vector<SrcBuffer> Buffers;

  // This is the list of directories we should search for include files in.
  std::vector<std::string> IncludeDirectories;

  DiagHandlerTy DiagHandler = nullptr;
  void *DiagContext = nullptr;

  bool isValidBufferID(unsigned i) const { return i && i <= Buffers.size(); }

public:
  SourceMgr() = default;
  SourceMgr(const SourceMgr &) = delete;
  SourceMgr &operator=(const SourceMgr &) = delete;
  SourceMgr(SourceMgr &&) = default;
  SourceMgr &operator=(SourceMgr &&) = default;
  ~SourceMgr() = default;

  void setIncludeDirs(const std::vector<std::string> &Dirs) {
    IncludeDirectories = Dirs;
  }

  /// Specify a diagnostic handler to be invoked every time PrintMessage is
  /// called. \p Ctx is passed into the handler when it is invoked.
  void setDiagHandler(DiagHandlerTy DH, void *Ctx = nullptr) {
    DiagHandler = DH;
    DiagContext = Ctx;
  }

  DiagHandlerTy getDiagHandler() const { return DiagHandler; }
  void *getDiagContext() const { return DiagContext; }

  const SrcBuffer &getBufferInfo(unsigned i) const {
    assert(isValidBufferID(i));
    return Buffers[i - 1];
  }

  const MemoryBuffer *getMemoryBuffer(unsigned i) const {
    assert(isValidBufferID(i));
    return Buffers[i - 1].Buffer.get();
  }

  unsigned getNumBuffers() const {
    return Buffers.size();
  }

  unsigned getMainFileID() const {
    assert(getNumBuffers());
    return 1;
  }

  SMLoc getParentIncludeLoc(unsigned i) const {
    assert(isValidBufferID(i));
    return Buffers[i - 1].IncludeLoc;
  }

  /// Add a new source buffer to this source manager. This takes ownership of
  /// the memory buffer.
  unsigned AddNewSourceBuffer(std::unique_ptr<MemoryBuffer> F,
                              SMLoc IncludeLoc) {
    SrcBuffer NB;
    NB.Buffer = std::move(F);
    NB.IncludeLoc = IncludeLoc;
    Buffers.push_back(std::move(NB));
    return Buffers.size();
  }

  /// Search for a file with the specified name in the current directory or in
  /// one of the IncludeDirs.
  ///
  /// If no file is found, this returns 0, otherwise it returns the buffer ID
  /// of the stacked file. The full path to the included file can be found in
  /// \p IncludedFile.
  unsigned AddIncludeFile(const std::string &Filename, SMLoc IncludeLoc,
                          std::string &IncludedFile);

  /// Return the ID of the buffer containing the specified location.
  ///
  /// 0 is returned if the buffer is not found.
  unsigned FindBufferContainingLoc(SMLoc Loc) const;

  /// Find the line number for the specified location in the specified file.
  /// This is not a fast method.
  unsigned FindLineNumber(SMLoc Loc, unsigned BufferID = 0) const {
    return getLineAndColumn(Loc, BufferID).first;
  }

  /// Find the line and column number for the specified location in the
  /// specified file. This is not a fast method.
  std::pair<unsigned, unsigned> getLineAndColumn(SMLoc Loc,
                                                 unsigned BufferID = 0) const;

  /// Emit a message about the specified location with the specified string.
  ///
  /// \param ShowColors Display colored messages if output is a terminal and
  /// the default error handler is used.
  void PrintMessage(raw_ostream &OS, SMLoc Loc, DiagKind Kind,
                    const Twine &Msg,
                    ArrayRef<SMRange> Ranges = None,
                    ArrayRef<SMFixIt> FixIts = None,
                    bool ShowColors = true) const;

  /// Emits a diagnostic to llvm::errs().
  void PrintMessage(SMLoc Loc, DiagKind Kind, const Twine &Msg,
                    ArrayRef<SMRange> Ranges = None,
                    ArrayRef<SMFixIt> FixIts = None,
                    bool ShowColors = true) const;

  /// Emits a manually-constructed diagnostic to the given output stream.
  ///
  /// \param ShowColors Display colored messages if output is a terminal and
  /// the default error handler is used.
  void PrintMessage(raw_ostream &OS, const SMDiagnostic &Diagnostic,
                    bool ShowColors = true) const;

  /// Return an SMDiagnostic at the specified location with th