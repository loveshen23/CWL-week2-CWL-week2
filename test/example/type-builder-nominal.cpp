#include <cassert>
#include <iostream>

#include "wasm-builder.h"
#include "wasm-type-printing.h"
#include "wasm-type.h"

using namespace wasm;

// Construct Signature, Struct, and Array heap types using undefined types.
void test_builder() {
  std::cout << ";; Test TypeBuilder\n";

  // (type $sig (func (param (ref $struct)) (result (ref $array) i32)))
  // (type $struct (struct (field (ref null $array))))
  // (type $array (array (mut anyref)))

  TypeBuilder builder;
  assert(builder.size() == 0);
  builder.grow(3);
  assert(builder.size() == 3);

  Type refSig = builder.getTempRefType(builder[0], NonNullable);
  Type refStruct = builder.getTempRefType(builder[1], NonNullable);
  Type refArray = builder.getTempRefType(builder[2], NonNullable);
  Type refNullArray = builder.getTempRefType(builder[2], Nullable);
  Type refNullAny(HeapType::any, Nullable);

  Signature sig(refStruct, builder.getTempTupleType({refArray, Type::i32}));
  Struct struct_({Field(refNullArray, Immutable)});
  Array array(Field(refNullAny, Mutable));

  {
    IndexedTypeNameGenerator print(builder);
    std::cout << "Before setting heap types:\n";
    std::cout << "$sig => " << print(builder[0]) << "\n";
    std::cout << "$struct => " << print(builder[1]) << "\n";
    std::cout << "$array => " << print(builder[2]) << "\n";
    std::cout << "(ref $sig) => " << print(refSig) << "\n";
    std::cout << "(ref $struct) => " << print(refStruct) << "\n";
    std::cout << "(ref $array) => " << print(refArray) << "\n";
    std::cout << "(ref null $array) => " << print(refNullArray) << "\n";
  }

  builder[0] = sig;
  builder[1] = struct_;
  builder[2] = array;

  {
    IndexedTypeNameGenerator print(builder);
    std::cout << "After setting heap types:\n";
    std::cout << "$sig => " << print(builder[0]) << "\n";
    std::cout << "$struct => " << print(builder[1]) << "\n";
    std::cout << "$array => " << print(builder[2]) << "\n";
    std::cout << "(ref $sig) => " << print(refSig) << "\n";
    std::cout << "(ref $struct) => " << print(refStruct) << "\n";
    std::cout << "(ref $array) => " << print(refArray) << "\n";
    std::cout << "(ref null $array) => " << print(refNullArray) << "\n";
  }

  std::vector<HeapType> built = *builder.build();

  Type newRefSig = Type(built[0], NonNullable);
  Type newRefStruct = Type(built[1], NonNullable);
  Type newRefArray = Type(built[2], NonNullable);
  Type newRefNullArray = Type(built[2], Nullable);

  {
    IndexedTypeNameGenerator print(built);
    std::cout << "After building types:\n";
    std::cout << "$sig => " << print(built[0]) << "\n";
    std::cout << "$struct => " << print(built[1]) << "\n";
    std::cout << "$array => " << print(built[2]) << "\n";
    std::cout << "(ref $sig) => " << print(newRefSig) << "\n";
    std::cout << "(ref $struct) => " << print(newRefStruct) << "\n";
    std::cout << "(ref $array) => " << print(newRefArray) << "\n";
    std::cout << "(ref null $array) => " << print(newRefNullArray) << "\n";
  }
}

// Check that the builder works when there are duplicate definitions
void test_canonicalization() {
  std::cout << ";; Test canonicalization\n";

  // (type $struct (struct (field (ref null $sig) (ref null $sig))))
  // (type $sig (func))
  HeapType sig = Signature(Type::none, Type::none);
  HeapType struct_ = Struct({Field(Type(sig, Nullable), Immutable),
                             Field(Type(sig, Nullable), Immutable)});

  TypeBuilder builder(4);

  Type tempSigRef1 = builder.getTempRefType(builder[2], Nullable);
  Type tempSigRef2 = builder.getTempRefType(builder[3], Nullable);

  assert(tempSigRef1 != tempSigRef2);
  assert(tempSigRef1 != Type(sig, Nullable));
  assert(tempSigRef2 != Type(sig, Nullable));

  builder[0] =
    Struct({Field(tempSigRef1, Immutable), Field(tempSigRef1, Immutable)});
  builder[1] =
    Struct({Field(tempSigRef2, Immutable), Field(tempSigRef2, Immutable)});
  builder[2] = Signature(Type::none, Type::none);
  builder[3] = Signature(Type::none, Type::none);

  std::vector<HeapType> built = *builder.build();

  assert(built[0] != struct_);
  assert(built[1] != struct_);
  assert(built[0] != built[1]);
  assert(built[2] != sig);
  assert(built[3] != sig);
  assert(built[2] != built[3]);
}

// Check that defined basic HeapTypes are handled correctly.
void test_basic() {
  std::cout << ";; Test basic\n";

  Type canonAnyref = Type(HeapType::any, Nullable);
  Type canonI31ref = Type(HeapType::i31, NonNullable);

  TypeBuilder builder(6);

  Type anyref = builder.getTempRefType(builder[4], Nullable);
  Type i31ref = builder.getTempRefType(builder[5], NonNullable);

  builder[0] = Signature(canonAnyref, canonI31ref);
  builder[1] = Signature(anyref, canonI31ref);
  builder[2] = Signature(canonAnyref, i31ref);
  builder[3] = Signature(anyref, i31ref);
  builder[4] = HeapType::any;
  builder[5] = HeapType::i31;

  std::vector<HeapType> built = *builder.build();

  assert(built[0].getSignature() == Signature(canonAnyref, canonI31ref));
  assert(built[1].getSignature() == built[0].getSignature());
  assert(built[2].getSignature() == built[1].getSignature());
  assert(built[3].getSignature() == built[2].getSignature());
  assert(built[4] == HeapType::any);
  assert(built[5] == HeapType::i31);
}

// Check that signatures created with TypeBuilders are properly recorded as
// canonical.
void test_signatures(bool warm) {
  std::cout << ";; Test canonical signatures\n";

  Type canonAnyref = Type(HeapType::any, Nullable);
  Type canonI31ref = Type(HeapType::i31, NonNullable);

  TypeBuilder builder(2);
  Type tempRef = builder.getTempRefType(builder[0], Nullable);
  builder[0] = Signature(canonI31ref, canonAnyref);
  builder[1] = Signature(tempRef, tempRef);
  std::vector<HeapType> built = *builder.build();

  HeapType small = Signature(canonI31ref, canonAnyref);
  HeapType big = Signature(Type(Signature(canonI31ref, canon