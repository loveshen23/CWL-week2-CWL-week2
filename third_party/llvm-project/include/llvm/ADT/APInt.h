//===-- llvm/ADT/APInt.h - For Arbitrary Precision Integer -----*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements a class to represent arbitrary precision
/// integral constant values and operations on them.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_APINT_H
#define LLVM_ADT_APINT_H

#include "llvm/Support/Compiler.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <climits>
#include <cstring>
#include <string>

namespace llvm {
class FoldingSetNodeID;
class StringRef;
class hash_code;
class raw_ostream;

template <typename T> class SmallVectorImpl;
template <typename T> class ArrayRef;
template <typename T> class Optional;

class APInt;

inline APInt operator-(APInt);

//===----------------------------------------------------------------------===//
//                              APInt Class
//===----------------------------------------------------------------------===//

/// Class for arbitrary precision integers.
///
/// APInt is a functional replacement for common case unsigned integer type like
/// "unsigned", "unsigned long" or "uint64_t", but also allows non-byte-width
/// integer sizes and large integer value types such as 3-bits, 15-bits, or more
/// than 64-bits of precision. APInt provides a variety of arithmetic operators
/// and methods to manipulate integer values of any bit-width. It supports both
/// the typical integer arithmetic and comparison operations as well as bitwise
/// manipulation.
///
/// The class has several invariants worth noting:
///   * All bit, byte, and word positions are zero-based.
///   * Once the bit width is set, it doesn't change except by the Truncate,
///     SignExtend, or ZeroExtend operations.
///   * All binary operators must be on APInt instances of the same bit width.
///     Attempting to use these operators on instances with different bit
///     widths will yield an assertion.
///   * The value is stored canonically as an unsigned value. For operations
///     where it makes a difference, there are both signed and unsigned variants
///     of the operation. For example, sdiv and udiv. However, because the bit
///     widths must be the same, operations such as Mul and Add produce the same
///     results regardless of whether the values are interpreted as signed or
///     not.
///   * In general, the class tries to follow the style of computation that LLVM
///     uses in its IR. This simplifies its use for LLVM.
///
class LLVM_NODISCARD APInt {
public:
  typedef uint64_t WordType;

  /// This enum is used to hold the constants we needed for APInt.
  enum : unsigned {
    /// Byte size of a word.
    APINT_WORD_SIZE = sizeof(WordType),
    /// Bits in a word.
    APINT_BITS_PER_WORD = APINT_WORD_SIZE * CHAR_BIT
  };

  enum class Rounding {
    DOWN,
    TOWARD_ZERO,
    UP,
  };

  static const WordType WORDTYPE_MAX = ~WordType(0);

private:
  /// This union is used to store the integer value. When the
  /// integer bit-width <= 64, it uses VAL, otherwise it uses pVal.
  union {
    uint64_t VAL;   ///< Used to store the <= 64 bits integer value.
    uint64_t *pVal; ///< Used to store the >64 bits integer value.
  } U;

  unsigned BitWidth; ///< The number of bits in this APInt.

  friend struct DenseMapAPIntKeyInfo;

  friend class APSInt;

  /// Fast internal constructor
  ///
  /// This constructor is used only internally for speed of construction of
  /// temporaries. It is unsafe for general use so it is not public.
  APInt(uint64_t *val, unsigned bits) : BitWidth(bits) {
    U.pVal = val;
  }

  /// Determine if this APInt just has one word to store value.
  ///
  /// \returns true if the number of bits <= 64, false otherwise.
  bool isSingleWord() const { return BitWidth <= APINT_BITS_PER_WORD; }

  /// Determine which word a bit is in.
  ///
  /// \returns the word position for the specified bit position.
  static unsigned whichWord(unsigned bitPosition) {
    return bitPosition / APINT_BITS_PER_WORD;
  }

  /// Determine which bit in a word a bit is in.
  ///
  /// \returns the bit position in a word for the specified bit position
  /// in the APInt.
  static unsigned whichBit(unsigned bitPosition) {
    return bitPosition % APINT_BITS_PER_WORD;
  }

  /// Get a single bit mask.
  ///
  /// \returns a uint64_t with only bit at "whichBit(bitPosition)" set
  /// This method generates and returns a uint64_t (word) mask for a single
  /// bit at a specific bit position. This is used to mask the bit in the
  /// corresponding word.
  static uint64_t maskBit(unsigned bitPosition) {
    return 1ULL << whichBit(bitPosition);
  }

  /// Clear unused high order bits
  ///
  /// This method is used internally to clear the top "N" bits in the high order
  /// word that are not used by the APInt. This is needed after the most
  /// significant word is assigned a value to ensure that those bits are
  /// zero'd out.
  APInt &clearUnusedBits() {
    // Compute how many bits are used in the final word
    unsigned WordBits = ((BitWidth-1) % APINT_BITS_PER_WORD) + 1;

    // Mask out the high bits.
    uint64_t mask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - WordBits);
    if (isSingleWord())
      U.VAL &= mask;
    else
      U.pVal[getNumWords() - 1] &= mask;
    return *this;
  }

  /// Get the word corresponding to a bit position
  /// \returns the corresponding word for the specified bit position.
  uint64_t getWord(unsigned bitPosition) const {
    return isSingleWord() ? U.VAL : U.pVal[whichWord(bitPosition)];
  }

  /// Utility method to change the bit width of this APInt to new bit width,
  /// allocating and/or deallocating as necessary. There is no guarantee on the
  /// value of any bits upon return. Caller should populate the bits after.
  void reallocate(unsigned NewBitWidth);

  /// Convert a char array into an APInt
  ///
  /// \param radix 2, 8, 10, 16, or 36
  /// Converts a string into a number.  The string must be non-empty
  /// and well-formed as a number of the given base. The bit-width
  /// must be sufficient to hold the result.
  ///
  /// This is used by the constructors that take string arguments.
  ///
  /// StringRef::getAsInteger is superficially similar but (1) does
  /// not assume that the string is well-formed and (2) grows the
  /// result to hold the input.
  void fromString(unsigned numBits, StringRef str, uint8_t radix);

  /// An internal division function for dividing APInts.
  ///
  /// This is used by the toString method to divide by the radix. It simply
  /// provides a more convenient form of divide for internal use since KnuthDiv
  /// has specific constraints on its inputs. If those constraints are not met
  /// then it provides a simpler form of divide.
  static void divide(const WordType *LHS, unsigned lhsWords,
                     const WordType *RHS, unsigned rhsWords, WordType *Quotient,
                     WordType *Remainder);

  /// out-of-line slow case for inline constructor
  void initSlowCase(uint64_t val, bool isSigned);

  /// shared code between two array constructors
  void initFromArray(ArrayRef<uint64_t> array);

  /// out-of-line slow case for inline copy constructor
  void initSlowCase(const APInt &that);

  /// out-of-line slow case for shl
  void shlSlowCase(unsigned ShiftAmt);

  /// out-of-line slow case for lshr.
  void lshrSlowCase(unsigned ShiftAmt);

  /// out-of-line slow case for ashr.
  void ashrSlowCase(unsigned ShiftAmt);

  /// out-of-line slow case for operator=
  void AssignSlowCase(const APInt &RHS);

  /// out-of-line slow case for operator==
  bool EqualSlowCase(const APInt &RHS) const LLVM_READONLY;

  /// out-of-line slow case for countLeadingZeros
  unsigned countLeadingZerosSlowCase() const LLVM_READONLY;

  /// out-of-line slow case for countLeadingOnes.
  unsigned countLeadingOnesSlowCase() const LLVM_READONLY;

  /// out-of-line slow case for countTrailingZeros.
  unsigned countTrailingZerosSlowCase() const LLVM_READONLY;

  /// out-of-line slow case for countTrailingOnes
  unsigned countTrailingOnesSlowCase() const LLVM_READONLY;

  /// out-of-line slow case for countPopulation
  unsigned countPopulationSlowCase() const LLVM_READONLY;

  /// out-of-line slow case for intersects.
  bool intersectsSlowCase(const APInt &RHS) const LLVM_READONLY;

  /// out-of-line slow case for isSubsetOf.
  bool isSubsetOfSlowCase(const APInt &RHS) const LLVM_READONLY;

  /// out-of-line slow case for setBits.
  void setBitsSlowCase(unsigned loBit, unsigned hiBit);

  /// out-of-line slow case for flipAllBits.
  void flipAllBitsSlowCase();

  /// out-of-line slow case for operator&=.
  void AndAssignSlowCase(const APInt& RHS);

  /// out-of-line slow case for operator|=.
  void OrAssignSlowCase(const APInt& RHS);

  /// out-of-line slow case for operator^=.
  void XorAssignSlowCase(const APInt& RHS);

  /// Unsigned comparison. Returns -1, 0, or 1 if this APInt is less than, equal
  /// to, or greater than RHS.
  int compare(const APInt &RHS) const LLVM_READONLY;

  /// Signed comparison. Returns -1, 0, or 1 if this APInt is less than, equal
  /// to, or greater than RHS.
  int compareSigned(const APInt &RHS) const LLVM_READONLY;

public:
  /// \name Constructors
  /// @{

  /// Create a new APInt of numBits width, initialized as val.
  ///
  /// If isSigned is true then val is treated as if it were a signed value
  /// (i.e. as an int64_t) and the appropriate sign extension to the bit width
  /// will be done. Otherwise, no sign extension occurs (high order bits beyond
  /// the range of val are zero filled).
  ///
  /// \param numBits the bit width of the constructed APInt
  /// \param val the initial value of the APInt
  /// \param isSigned how to treat signedness of val
  APInt(unsigned numBits, uint64_t val, bool isSigned = false)
      : BitWidth(numBits) {
    assert(BitWidth && "bitwidth too small");
    if (isSingleWord()) {
      U.VAL = val;
      clearUnusedBits();
    } else {
      initSlowCase(val, isSigned);
    }
  }

  /// Construct an APInt of numBits width, initialized as bigVal[].
  ///
  /// Note that bigVal.size() can be smaller or larger than the corresponding
  /// bit width but any extraneous bits will be dropped.
  ///
  /// \param numBits the bit width of the constructed APInt
  /// \param bigVal a sequence of words to form the initial value of the APInt
  APInt(unsigned numBits, ArrayRef<uint64_t> bigVal);

  /// Equivalent to APInt(numBits, ArrayRef<uint64_t>(bigVal, numWords)), but
  /// deprecated because this constructor is prone to ambiguity with the
  /// APInt(unsigned, uint64_t, bool) constructor.
  ///
  /// If this overload is ever deleted, care should be taken to prevent calls
  /// from being incorrectly captured by the APInt(unsigned, uint64_t, bool)
  /// constructor.
  APInt(unsigned numBits, unsigned numWords, const uint64_t bigVal[]);

  /// Construct an APInt from a string representation.
  ///
  /// This constructor interprets the string \p str in the given radix. The
  /// interpretation stops when the first character that is not suitable for the
  /// radix is encountered, or the end of the string. Acceptable radix values
  /// are 2, 8, 10, 16, and 36. It is an error for the value implied by the
  /// string to require more bits than numBits.
  ///
  /// \param numBits the bit width of the constructed APInt
  /// \param str the string to be interpreted
  /// \param radix the radix to use for the conversion
  APInt(unsigned numBits, StringRef str, uint8_t radix);

  /// Simply makes *this a copy of that.
  /// Copy Constructor.
  APInt(const APInt &that) : BitWidth(that.BitWidth) {
    if (isSingleWord())
      U.VAL = that.U.VAL;
    else
      initSlowCase(that);
  }

  /// Move Constructor.
  APInt(APInt &&that) : BitWidth(that.BitWidth) {
    memcpy(&U, &that.U, sizeof(U));
    that.BitWidth = 0;
  }

  /// Destructor.
  ~APInt() {
    if (needsCleanup())
      delete[] U.pVal;
  }

  /// Default constructor that creates an uninteresting APInt
  /// representing a 1-bit zero value.
  ///
  /// This is useful for object deserialization (pair this with the static
  ///  method Read).
  explicit APInt() : BitWidth(1) { U.VAL = 0; }

  /// Returns whether this instance allocated memory.
  bool needsCleanup() const { return !isSingleWord(); }

  /// Used to insert APInt objects, or objects that contain APInt objects, into
  ///  FoldingSets.
  void Profile(FoldingSetNodeID &id) const;

  /// @}
  /// \name Value Tests
  /// @{

  /// Determine sign of this APInt.
  ///
  /// This tests the high bit of this APInt to determine if it is set.
  ///
  /// \returns true if this APInt is negative, false otherwise
  bool isNegative() const { return (*this)[BitWidth - 1]; }

  /// Determine if this APInt Value is non-negative (>= 0)
  ///
  /// This tests the high bit of the APInt to determine if it is unset.
  bool isNonNegative() const { return !isNegative(); }

  /// Determine if sign bit of this APInt is set.
  ///
  /// This tests the high bit of this APInt to determine if it is set.
  ///
  /// \returns true if this APInt has its sign bit set, false otherwise.
  bool isSignBitSet() const { return (*this)[BitWidth-1]; }

  /// Determine if sign bit of this APInt is clear.
  ///
  /// This tests the high bit of this APInt to determine if it is clear.
  ///
  /// \returns true if this APInt has its sign bit clear, false otherwise.
  bool isSignBitClear() const { return !isSignBitSet(); }

  /// Determine if this APInt Value is positive.
  ///
  /// This tests if the value of this APInt is positive (> 0). Note
  /// that 0 is not a positive value.
  ///
  /// \returns true if this APInt is positive.
  bool isStrictlyPositive() const { return isNonNegative() && !isNullValue(); }

  /// Determine if all bits are set
  ///
  /// This checks to see if the value has all bits of the APInt are set or not.
  bool isAllOnesValue() const {
    if (isSingleWord())
      return U.VAL == WORDTYPE_MAX >> (APINT_BITS_PER_WORD - BitWidth);
    return countTrailingOnesSlowCase() == BitWidth;
  }

  /// Determine if all bits are clear
  ///
  /// This checks to see if the value has all bits of the APInt are clear or
  /// not.
  bool isNullValue() const { return !*this; }

  /// Determine if this is a value of 1.
  ///
  /// This checks to see if the value of this APInt is one.
  bool isOneValue() const {
    if (isSingleWord())
      return U.VAL == 1;
    return countLeadingZerosSlowCase() == BitWidth - 1;
  }

  /// Determine if this is the largest unsigned value.
  ///
  /// This checks to see if the value of this APInt is the maximum unsigned
  /// value for the APInt's bit width.
  bool isMaxValue() const { return isAllOnesValue(); }

  /// Determine if this is the largest signed value.
  ///
  /// This checks to see if the value of this APInt is the maximum signed
  /// value for the APInt's bit width.
  bool isMaxSignedValue() const {
    if (isSingleWord())
      return U.VAL == ((WordType(1) << (BitWidth - 1)) - 1);
    return !isNegative() && countTrailingOnesSlowCase() == BitWidth - 1;
  }

  /// Determine if this is the smallest unsigned value.
  ///
  /// This checks to see if the value of this APInt is the minimum unsigned
  /// value for the APInt's bit width.
  bool isMinValue() const { return isNullValue(); }

  /// Determine if this is the smallest signed value.
  ///
  /// This checks to see if the value of this APInt is the minimum signed
  /// value for the APInt's bit width.
  bool isMinSignedValue() const {
    if (isSingleWord())
      return U.VAL == (WordType(1) << (BitWidth - 1));
    return isNegative() && countTrailingZerosSlowCase() == BitWidth - 1;
  }

  /// Check if this APInt has an N-bits unsigned integer value.
  bool isIntN(unsigned N) const {
    assert(N && "N == 0 ???");
    return getActiveBits() <= N;
  }

  /// Check if this APInt has an N-bits signed integer value.
  bool isSignedIntN(unsigned N) const {
    assert(N && "N == 0 ???");
    return getMinSignedBits() <= N;
  }

  /// Check if this APInt's value is a power of two greater than zero.
  ///
  /// \returns true if the argument APInt value is a power of two > 0.
  bool isPowerOf2() const {
    if (isSingleWord())
      return isPowerOf2_64(U.VAL);
    return countPopulationSlowCase() == 1;
  }

  /// Check if the APInt's value is returned by getSignMask.
  ///
  /// \returns true if this is the value returned by getSignMask.
  bool isSignMask() const { return isMinSignedValue(); }

  /// Convert APInt to a boolean value.
  ///
  /// This converts the APInt to a boolean value as a test against zero.
  boo