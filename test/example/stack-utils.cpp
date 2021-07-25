#include <cassert>
#include <iostream>

#include "ir/stack-utils.h"
#include "literal.h"
#include "mixed_arena.h"
#include "wasm-builder.h"
#include "wasm-type.h"
#include "wasm.h"

using namespace wasm;

Module module;
Builder builder(module);

void test_remove_nops() {
  std::cout << ";; Test removeNops\n";
  auto* block = builder.makeBlock(
    {
      builder.makeNop(),
      builder.makeConst(Literal(int32_t(0))),
      builder.makeNop(),
      builder.makeConst(Literal(int64_t(0))),
      builder.makeNop(),
      builder.makeNop(),
    },
    {Type::i32, Type::i64});
  std::cout << *block << '\n';
  StackUtils::removeNops(block);
  std::cout << *block << '\n';
}

void test_stack_signatures() {
  std::cout << ";; Test stack signatures\n";
  // Typed block
  auto* block = builder.makeBlock({builder.makeUnreachable()}, Type::f32);
  assert(StackSignature(block) ==
         (StackSignature{Type::none, Type::f32, StackSignature::Fixed}));
  // Unreachable block
  auto* unreachable =
    builder.makeBlock({builder.makeUnreachable()}, Type::unreachable);
  assert(StackSignature(unreachable) ==
         (StackSignature{Type::none, Type::none, StackSignature::Polymorphic}));
  {
    // Typed loop
    auto* loop = builder.makeLoop("loop", unreachable, Type::f32);
    assert(StackSignature(loop) ==
           (StackSignature{Type::none, Type::f32, StackSignature::Fixed}));
  }
  {
    // Unreachable loop
    auto* loop = builder.makeLoop("loop", unreachable, Type::unreachable);
    assert(
      StackSignature(loop) ==
      (StackSignature{Type::none, Type::none, StackSignature::Polymorphic}));
  }
  {
    // If (no else)
    auto* if_ = builder.makeIf(
      builder.makePop(Type::i32), unreachable, nullptr, Type::none);
    assert(StackSignature(if_) ==
           (StackSignature{Type::i32, Type::none, StackSignature::Fixed}));
  }
  {
    // If (with else)
    auto* if_ =
      builder.makeIf(builder.makePop(Type::i32), block, block, Type::f32);
    assert(StackSignature(if_) ==
           (StackSignature{Type::i32, Type::f32, StackSignature::Fixed}));
  }
  {
    // Call
    auto* call =
      builder.makeCall("foo",
                       {builder.makePop(Type::i32), builder.makePop(Type::f32)},
                       {Type::i64, Type::f64});
    assert(StackSignature(call) == (StackSignature{{Type::i32, Type::f32},
                                                   {Type::i64, Type::f64},
                                                   StackSignature::Fixed}));
  }
  {
    // Return Call
    auto* call =
      builder.makeCall("bar",
                       {builder.makePop(Type::i32), builder.makePop(Type::f32)},
                       Type::unreachable,
                       StackSignature::Polymorphic);
    assert(StackSignature(call) ==
           (StackSignature{
             {Type::i32, Type::f32}, Type::none, StackSignature::Polymorphic}));
  }
  {
    // Return
    auto* ret = builder.makeReturn(builder.makePop(Type::i32));
    assert(
      StackSignature(ret) ==
      (StackSignature{Type::i32, Type::none, StackSignature::Polymorphic}));
  }
  {
    // Multivalue return
    auto* ret = builder.makeReturn(builder.makePop({Type::i32, Type::f32}));
    assert(StackSignature(ret) ==
           (StackSignature{
             {Type::i32, Type::f32}, Type::none, StackSignature::Polymorphic}));
  }
}

void test_signature_composition() {
  std::cout << ";; Test stack signature composition\n";
  // No unreachables
  {
    StackSignature a{Type::none, {Type::f32, Type::i32}, StackSignature::Fixed};
    StackSignature b{{Type::f32, Type::i32}, Type::none, StackSignature::Fixed};
    assert(a.composes(b));
    assert(a + b ==
           (StackSignature{Type::none, Type::none, StackSignature::Fixed}));
  }
  {
    StackSignature a{Type::none, Type::i32, StackSignature::Fixed};
    StackSignature b{{Type::f32, Type::i32}, Type::none, StackSignature::Fixed};
    assert(a.composes(b));
    assert(a + b ==
           StackSignature(Type::f32, Type::none, StackSignature::Fixed));
  }
  {
    StackSignature a{Type::none, {Type::f32, Type::i32}, StackSignature::Fixed};
    StackSignature b{Type::i32, Type::none, StackSignature::Fixed};
    assert(a.composes(b));
    assert(a + b ==
           (StackSignature{Type::none, Type::f32, StackSignature::Fixed}));
  }
  {
    StackSignature a{Type::none, Type::f32, StackSignature::Fixed};
    StackSignature b{{Type::f32, Type::i32}, Type::none, StackSignature::Fixed};
    assert(!a.composes(b));
  }
  {
    StackSignature a{Type::none, {Type::f32, Type::i32}, StackSignature::Fixed};
    StackSignature b{Type::f32, Type::none, StackSignature::Fixed};
    assert(!a.composes(b));
  }
  // First unreachable
  {
    StackSignature a{
      Type::none, {Type::f32, Type::i32}, StackSignature::Polymorphic};
    StackSignature b{{Type::f32, Type::i32}, Type::none, StackSignature::Fixed};
    assert(a.composes(b));
    assert(a + b == (StackSignature{
                      Type::none, Type::none, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::i32, StackSignature::Polymorphic};
    StackSignature b{{Type::f32, Type::i32}, Type::none, StackSignature::Fixed};
    assert(a.composes(b));
    assert(a + b ==
           StackSignature(Type::none, Type::none, StackSignature::Polymorphic));
  }
  {
    StackSignature a{
      Type::none, {Type::f32, Type::i32}, StackSignature::Polymorphic};
    StackSignature b{Type::i32, Type::none, StackSignature::Fixed};
    assert(a.composes(b));
    assert(a + b == (StackSignature{
                      Type::none, Type::f32, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::f32, StackSignature::Polymorphic};
    StackSignature b{{Type::f32, Type::i32}, Type::none, StackSignature::Fixed};
    assert(!a.composes(b));
  }
  {
    StackSignature a{
      Type::none, {Type::f32, Type::i32}, StackSignature::Polymorphic};
    StackSignature b{Type::f32, Type::none, StackSignature::Fixed};
    assert(!a.composes(b));
  }
  // Second unreachable
  {
    StackSignature a{Type::none, {Type::f32, Type::i32}, StackSignature::Fixed};
    StackSignature b{
      {Type::f32, Type::i32}, Type::none, StackSignature::Polymorphic};
    assert(a.composes(b));
    assert(a + b == (StackSignature{
                      Type::none, Type::none, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::i32, StackSignature::Fixed};
    StackSignature b{
      {Type::f32, Type::i32}, Type::none, StackSignature::Polymorphic};
    assert(a.composes(b));
    assert(a + b ==
           StackSignature(Type::f32, Type::none, StackSignature::Polymorphic));
  }
  {
    StackSignature a{Type::none, {Type::f32, Type::i32}, StackSignature::Fixed};
    StackSignature b{Type::i32, Type::none, StackSignature::Polymorphic};
    assert(a.composes(b));
    assert(a + b == (StackSignature{
                      Type::none, Type::none, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::f32, StackSignature::Fixed};
    StackSignature b{
      {Type::f32, Type::i32}, Type::none, StackSignature::Polymorphic};
    assert(!a.composes(b));
  }
  {
    StackSignature a{Type::none, {Type::f32, Type::i32}, StackSignature::Fixed};
    StackSignature b{Type::f32, Type::none, StackSignature::Polymorphic};
    assert(!a.composes(b));
  }
  // Both unreachable
  {
    StackSignature a{
      Type::none, {Type::f32, Type::i32}, StackSignature::Polymorphic};
    StackSignature b{
      {Type::f32, Type::i32}, Type::none, StackSignature::Polymorphic};
    assert(a.composes(b));
    assert(a + b == (StackSignature{
                      Type::none, Type::none, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::i32, StackSignature::Polymorphic};
    StackSignature b{
      {Type::f32, Type::i32}, Type::none, StackSignature::Polymorphic};
    assert(a.composes(b));
    assert(a + b ==
           StackSignature(Type::none, Type::none, StackSignature::Polymorphic));
  }
  {
    StackSignature a{
      Type::none, {Type::f32, Type::i32}, StackSignature::Polymorphic};
    StackSignature b{Type::i32, Type::none, StackSignature::Polymorphic};
    assert(a.composes(b));
    assert(a + b == (StackSignature{
                      Type::none, Type::none, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::f32, StackSignature::Polymorphic};
    StackSignature b{
      {Type::f32, Type::i32}, Type::none, StackSignature::Polymorphic};
    assert(!a.composes(b));
  }
  {
    StackSignature a{
      Type::none, {Type::f32, Type::i32}, StackSignature::Polymorphic};
    StackSignature b{Type::f32, Type::none, StackSignature::Polymorphic};
    assert(!a.composes(b));
  }
}

void test_signature_subtype() {
  Type eqref = Type(HeapType::eq, Nullable);
  Type anyref = Type(HeapType::any, Nullable);

  std::cout << ";; Test stack signature subtyping\n";
  // Differences in unreachability only
  {
    StackSignature a(Type::none, Type::none, StackSignature::Polymorphic);
    StackSignature b(Type::none, Type::none, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
    assert(!StackSignature::isSubType(b, a));
  }
  // Covariance of results
  {
    StackSignature a(Type::none, eqref, StackSignature::Fixed);
    StackSignature b(Type::none, anyref, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
    assert(!StackSignature::isSubType(b, a));
  }
  // Contravariance of params
  {
    StackSignature a(anyref, Type::none, StackSignature::Fixed);
    StackSignature b(eqref, Type::none, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
    assert(!StackSignature::isSubType(b, a));
  }
  // First not unreachable
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b(Type::i32, Type::f32, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b(
      {Type::i64, Type::i32}, {Type::i64, Type::f32}, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b({Type::i64, Type::i32},
                     {Type::i64, Type::i64, Type::f32},
                     StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b(
      {Type::i64, Type::i32}, {Type::f64, Type::f32}, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b(Type::none, Type::f32, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b(Type::i32, Type::none, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Fixed);
    StackSignature b(Type::f32, Type::i32, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  // First unreachable
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b(Type::i32, Type::f32, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b(
      {Type::i64, Type::i32}, {Type::i64, Type::f32}, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b({Type::i64, Type::i32},
                     {Type::i64, Type::i64, Type::f32},
                     StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b(
      {Type::i64, Type::i32}, {Type::f64, Type::f32}, StackSignature::Fixed);
    assert(StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b(Type::none, Type::f32, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b(Type::i32, Type::none, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
  {
    StackSignature a(Type::i32, Type::f32, StackSignature::Polymorphic);
    StackSignature b(Type::f32, Type::i32, StackSignature::Fixed);
    assert(!StackSignature::isSubType(a, b));
  }
}

void test_signature_lub() {
  Type eqref = Type(HeapType::eq, Nullable);
  Type anyref = Type(HeapType::any, Nullable);

  std::cout << ";; Test stack signature lub\n";
  {
    StackSignature a{Type::none, Type::none, StackSignature::Fixed};
    StackSignature b{Type::none, Type::none, StackSignature::Fixed};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{Type::none, Type::none, StackSignature::Fixed}));
  }
  {
    StackSignature a{Type::none, Type::none, StackSignature::Polymorphic};
    StackSignature b{Type::none, Type::none, StackSignature::Fixed};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{Type::none, Type::none, StackSignature::Fixed}));
  }
  {
    StackSignature a{Type::none, Type::none, StackSignature::Fixed};
    StackSignature b{Type::none, Type::none, StackSignature::Polymorphic};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{Type::none, Type::none, StackSignature::Fixed}));
  }
  {
    StackSignature a{Type::i32, Type::none, StackSignature::Polymorphic};
    StackSignature b{Type::none, Type::i32, StackSignature::Polymorphic};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{Type::i32, Type::i32, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, Type::i32, StackSignature::Polymorphic};
    StackSignature b{Type::i32, Type::none, StackSignature::Polymorphic};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{Type::i32, Type::i32, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{Type::none, anyref, StackSignature::Polymorphic};
    StackSignature b{Type::none, eqref, StackSignature::Polymorphic};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{Type::none, anyref, StackSignature::Polymorphic}));
  }
  {
    StackSignature a{anyref, Type::none, StackSignature::Polymorphic};
    StackSignature b{eqref, Type::none, StackSignature::Polymorphic};
    // assert(StackSignature::haveLeastUpperBound(a, b));
    // assert(StackSignature::getLeastUpperBound(a, b) ==
    //        (StackSignature{eqref, Type::none,
    //        StackSignature::Polymorphic}));
  }
  {
    StackSignature a{{Type::i32, eqref}, eqref, StackSignature::Polymorphic};
    StackSignature b{eqref, {Type::f32, anyref}, StackSignature::Polymorphic};
    assert(StackSignature::haveLeastUpperBound(a, b));
    assert(StackSignature::getLeastUpperBound(a, b) ==
           (StackSignature{{Type::i32, eqref},
                           {Type::f32, anyref},
                           StackSignature::Polymorphic}));
  }
  // No LUB
  {
    StackSignature a(Type::none, Type::i32, StackSignature::Fixed);
    StackSignature b(Type::none, Type::f32, StackSignature::Fixed);
    assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a(Type::none, Type::i32, StackSignature::Polymorphic);
    StackSignature b(Type::none, Type::f32, StackSignature::Polymorphic);
    assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a(Type::i32, Type::none, StackSignature::Fixed);
    StackSignature b(Type::f32, Type::none, StackSignature::Fixed);
    // assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a(Type::i32, Type::none, StackSignature::Polymorphic);
    StackSignature b(Type::f32, Type::none, StackSignature::Polymorphic);
    // assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a(Type::none, Type::none, StackSignature::Fixed);
    StackSignature b(Type::none, Type::i32, StackSignature::Polymorphic);
    assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a(Type::none, Type::none, StackSignature::Fixed);
    StackSignature b(Type::i32, Type::none, StackSignature::Polymorphic);
    assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a{Type::none, Type::i32, StackSignature::Fixed};
    StackSignature b{Type::i32, Type::none, StackSignature::Fixed};
    assert(!StackSignature::haveLeastUpperBound(a, b));
  }
  {
    StackSignature a{Ty