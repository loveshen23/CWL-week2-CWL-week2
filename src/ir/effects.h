/*
 * Copyright 2017 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef wasm_ir_effects_h
#define wasm_ir_effects_h

#include "ir/intrinsics.h"
#include "pass.h"
#include "wasm-traversal.h"

namespace wasm {

// Analyze various possible effects.

class EffectAnalyzer {
public:
  EffectAnalyzer(const PassOptions& passOptions, Module& module)
    : ignoreImplicitTraps(passOptions.ignoreImplicitTraps),
      trapsNeverHappen(passOptions.trapsNeverHappen),
      funcEffectsMap(passOptions.funcEffectsMap), module(module),
      features(module.features) {}

  EffectAnalyzer(const PassOptions& passOptions,
                 Module& module,
                 Expression* ast)
    : EffectAnalyzer(passOptions, module) {
    walk(ast);
  }

  EffectAnalyzer(const PassOptions& passOptions, Module& module, Function* func)
    : EffectAnalyzer(passOptions, module) {
    walk(func);
  }

  bool ignoreImplicitTraps;
  bool trapsNeverHappen;
  std::shared_ptr<FuncEffectsMap> funcEffectsMap;
  Module& module;
  FeatureSet features;

  // Walk an expression and all its children.
  void walk(Expression* ast) {
    pre();
    InternalAnalyzer(*this).walk(ast);
    post();
  }

  // Visit an expression, without any children.
  void visit(Expression* ast) {
    pre();
    InternalAnalyzer(*this).visit(ast);
    post();
  }

  // Walk an entire function body. This will ignore effects that are not
  // noticeable from the perspective of the caller, that is, effects that are
  // only noticeable during the call, but "vanish" when the call stack is
  // unwound.
  void walk(Function* func) {
    walk(func->body);

    // We can ignore branching out of the function body - this can only be
    // a return, and that is only noticeable in the function, not outside.
    branchesOut = false;

    // When the function exits, changes to locals cannot be noticed any more.
    localsWritten.clear();
    localsRead.clear();
  }

  // Core effect tracking

  // Definitely branches out of this expression, or does a return, etc.
  // breakTargets tracks individual targets, which we may eventually see are
  // internal, while this is set when we see something that will definitely
  // not be internal, or is otherwise special like an infinite loop (which
  // does not technically branch "out", but it does break the normal assumption
  // of control flow proceeding normally).
  bool branchesOut = false;
  bool calls = false;
  std::set<Index> localsRead;
  std::set<Index> localsWritten;
  std::set<Name> mutableGlobalsRead;
  std::set<Name> globalsWritten;
  bool readsMemory = false;
  bool writesMemory = false;
  bool readsTable = false;
  bool writesTable = false;
  // TODO: More specific type-based alias analysis, and not just at the
  //       struct/array level.
  bool readsMutableStruct = false;
  bool writesStruct = false;
  bool readsArray = false;
  bool writesArray = false;
  // A trap, either from an unreachable instruction, or from an implicit trap
  // that we do not ignore (see below).
  //
  // Note that we ignore trap differences, so it is ok to reorder traps with
  // each other, but it is not ok to remove them or reorder them with other
  // effects in a noticeable way.
  //
  // Note also that we ignore *optional* runtime-specific traps: we only
  // consider as trapping something that will trap in *all* VMs, and *all* the
  // time. For example, a single allocation might trap in a VM in a particular
  // execution, if it happens to run out of memory just there, but that is not
  // enough for us to mark it as having a trap effect. (Note that not marking
  // each allocation as possibly trapping has the nice benefit of making it
  // possible to eliminate an allocation whose result is not captured.) OTOH, we
  // *do* mark a potentially infinite number of allocations as trapping, as all
  // VMs would trap eventually, and the same for potentially infinite recursion,
  // etc.
  //   * We assume that VMs will timeout eventually, so any loop that we cannot
  //     prove terminates is considered to trap. (Some VMs might not have
  //     such timeouts, but even they will error before the heat death of the
  //     universe, which is a kind of trap.)
  bool trap = false;
  // A trap from an instruction like a load or div/rem, which may trap on corner
  // cases. If we do not ignore implicit traps then these are counted as a trap.
  bool implicitTrap = false;
  // An atomic load/store/RMW/Cmpxchg or an operator that has a defined ordering
  // wrt atomics (e.g. memory.grow)
  bool isAtomic = false;
  bool throws_ = false;
  // The nested depth of try-catch_all. If an instruction that may throw is
  // inside an inner try-catch_all, we don't mark it as 'throws_', because it
  // will be caught by an inner catch_all. We only count 'try's with a
  // 'catch_all' because instructions within a 'try' without a 'catch_all' can
  // still throw outside of the try.
  size_t tryDepth = 0;
  // The nested depth of catch. This is necessary to track danglng pops.
  size_t catchDepth = 0;
  // If this expression contains 'pop's that are not enclosed in 'catch' body.
  // For example, (drop (pop i32)) should set this to true.
  bool danglingPop = false;

  // Helper functions to check for various effect types

  bool accessesLocal() const {
    return localsRead.size() + localsWritten.size() > 0;
  }
  bool accessesMutableGlobal() const {
    return globalsWritten.size() + mutableGlobalsRead.size() > 0;
  }
  bool accessesMemory() const { return calls || readsMemory || writesMemory; }
  bool accessesTable() const { return calls || readsTable || writesTable; }
  bool accessesMutableStruct() const {
    return calls || readsMutableStruct || writesStruct;
  }
  bool accessesArray() const { return calls || readsArray || writesArray; }
  bool throws() const { return throws_ || !delegateTargets.empty(); }
  // Check whether this may transfer control flow to somewhere outside of this
  // expression (aside from just flowing out normally). That includes a break
  // or a throw (if the throw is not known to be caught inside this expression;
  // note that if the throw is not caught in this expression then it might be
  // caught in this function but outside of this expression, or it might not be
  // caught in the function at all, which would mean control flow cannot be
  // transferred inside the function, but this expression does not know that).
  bool transfersControlFlow() const {
    return branchesOut || throws() || hasExternalBreakTargets();
  }

  // Changes something in globally-stored state.
  bool writesGlobalState() const {
    return globalsWritten.size() || writesMemory || writesTable ||
           writesStruct || writesArray || isAtomic || calls;
  }
  bool readsMutableGlobalState() const {
    return mutableGlobalsRead.size() || readsMemory || readsTable ||
           readsMutableStruct || readsArray || isAtomic || calls;
  }

  bool hasNonTrapSideEffects() const {
    return localsWritten.size() > 0 || danglingPop || writesGlobalState() ||
           throws() || transfersControlFlow();
  }

  bool hasSideEffects() const { return trap || hasNonTrapSideEffects(); }

  // Check if there are side effects, and they are of a kind that cannot be
  // removed by optimization passes.
  //
  // The difference between this and hasSideEffects is subtle, and only related
  // to trapsNeverHappen - if trapsNeverHappen then any trap we see is removable
  // by optimizations. In general, you should call hasSideEffects, and only call
  // this method if you are certain that it is a place that would not perform an
  // unsafe transformation with a trap. Specifically, if a pass calls this
  // and gets the result that there are no unremovable side effects, then it
  // must either
  //
  //  1. Remove any side effects present, if any, so they no longer exist.
  //  2. Keep the code exactly where it is.
  //
  // If instead of 1&2 a pass kept the side effect and also reordered the code
  // with other things, then that could be bad, as the side effect might have
  // been behind a condition that avoids it occurring.
  //
  // TODO: Go through the optimizer and use this in all places that do not move
  //       code around.
  bool hasUnremovableSideEffects() const {
    return hasNonTrapSideEffects() || (trap && !trapsNeverHappen);
  }

  bool hasAnything() const {
    return hasSideEffects() || accessesLocal() || readsMutableGlobalState();
  }

  // check if we break to anything external from ourselves
  bool hasExternalBreakTargets() const { return !breakTargets.empty(); }

  // checks if these effects would invalidate another set (e.g., if we write, we
  // invalidate someone that reads, they can't be moved past us)
  bool invalidates(const EffectAnalyzer& other) {
    if ((transfersControlFlow() && other.hasSideEffects()) ||
        (other.transfersControlFlow() && hasSideEffects()) ||
        ((writesMemory || calls) && other.accessesMemory()) ||
        ((other.writesMemory || other.calls) && accessesMemory()) ||
        ((writesTable || calls) && other.accessesTable()) ||
        ((other.writesTable || other.calls) && accessesTable()) ||
        ((writesStruct || calls) && other.accessesMutableStruct()) ||
        ((other.writesStruct || other.calls) && accessesMutableStruct()) ||
        ((writesArray || calls) && other.accessesArray()) ||
        ((other.writesArray || other.calls) && accessesArray()) ||
        (danglingPop || other.danglingPop)) {
      return true;
    }
    // All atomics are sequentially consistent for now, and ordered wrt other
    // memory references.
    if ((isAtomic && other.accessesMemory()) ||
        (other.isAtomic && accessesMemory())) {
      return true;
    }
    for (auto local : localsWritten) {
      if (other.localsRead.count(local) || other.localsWritten.count(local)) {
        return true;
      }
    }
    for (auto local : localsRead) {
      if (other.localsWritten.count(local)) {
        return true;
      }
    }
    if ((other.calls && accessesMutableGlobal()) ||
        (calls && other.accessesMutableGlobal())) {
      return true;
    }
    for (auto global : globalsWritten) {
      if (other.mutableGlobalsRead.count(global) ||
          other.globalsWritten.count(global)) {
        return true;
      }
    }
    for (auto global : mutableGlobalsRead) {
      if (other.globalsWritten.count(global)) {
        return true;
      }
    }
    // We are ok to reorder implicit traps, but not conditionalize them.
    if ((trap && other.transfersControlFlow()) ||
        (other.trap && transfersControlFlow())) {
      return true;
    }
    // Note that the above includes disallowing the reordering of a trap with an
    // exception (as an exception can transfer control flow inside the current
    // function, so transfersControlFlow would be true) - while we allow the
    // reordering of traps with each other, we do not reorder exceptions with
    // anything.
    assert(!((trap && other.throws()) || (throws() && other.trap)));
    // We can't reorder an implicit trap in a way that could alter what global
    // state is modified.
    if ((trap && other.writesGlobalState()) ||
        (other.trap && writesGlobalState())) {
      return true;
    }
    return false;
  }

  void mergeIn(const EffectAnalyzer& other) {
    branchesOut = branchesOut || other.branchesOut;
    calls = calls || other.calls;
    readsMemory = readsMemory || other.readsMemory;
    writesMemory = writesMemory || other.writesMemory;
    readsTable = readsTable || other.readsTable;
    writesTable = writesTable || other.writesTable;
    readsMutableStruct = readsMutableStruct || other.readsMutableStruct;
    writesStruct = writesStruct || other.writesStruct;
    readsArray = readsArray || other.readsArray;
    writesArray = writesArray || other.writesArray;
    trap = trap || other.trap;
    implicitTrap = implicitTrap || other.implicitTrap;
    trapsNeverHappen = trapsNeverHappen || other.trapsNeverHappen;
    isAtomic = isAtomic || other.isAtomic;
    throws_ = throws_ || other.throws_;
    danglingPop = danglingPop || other.danglingPop;
    for (auto i : other.localsRead) {
      localsRead.insert(i);
    }
    for (auto i : other.localsWritten) {
      localsWritten.insert(i);
    }
    for (auto i : other.mutableGlobalsRead) {
      mutableGlobalsRead.insert(i);
    }
    for (auto i : other.globalsWritten) {
      globalsWritten.insert(i);
    }
    for (auto i : other.breakTargets) {
      breakTargets.insert(i);
    }
    for (auto i : other.delegateTargets) {
      delegateTargets.insert(i);
    }
  }

  // the checks above happen after the node's children were processed, in the
  // order of execution we must also check for control flow that happens before
  // the children, i.e., loops
  bool checkPre(Expression* curr) {
    if (curr->is<Loop>()) {
      branchesOut = true;
      return true;
    }
    return false;
  }

  bool checkPost(Expression* curr) {
    visit(curr);
    if (curr->is<Loop>()) {
      branchesOut = true;
    }
    return hasAnything();
  }

  std::set<Name> breakTargets;
  std::set<Name> delegateTargets;

private:
  struct InternalAnalyzer
    : public PostWalker<InternalAnalyzer, OverriddenVisitor<InternalAnalyzer>> {

    EffectAnalyzer& parent;

    InternalAnalyzer(EffectAnalyzer& parent) : parent(parent) {}

    static void scan(InternalAnalyzer* self, Expression** currp) {
      Expression* curr = *currp;
      // We need to decrement try depth before catch starts, so handle it
      // separately
      if (curr->is<Try>()) {
        self->pushTask(doVisitTry, currp);
        self->pushTask(doEndCatch, currp);
        auto& catchBodies = curr->cast<Try>()->catchBodies;
        for (int i = int(catchBodies.size()) - 1; i >= 0; i--) {
          self->pushTask(scan, &catchBodies[i]);
        }
        self->pushTask(doStartCatch, currp);
        self->pushTask(scan, &curr->cast<Try>()->body);
        self->pushTask(doStartTry, currp);
        return;
      }
      PostWalker<InternalAnalyzer, OverriddenVisitor<InternalAnalyzer>>::scan(
        self, currp);
    }

    static void doStartTry(InternalAnalyzer* self, Expression** currp) {
      Try* curr = (*currp)->cast<Try>();
      // We only count 'try's with a 'catch_all' because instructions within a
      // 'try' without a 'catch_all' can still throw outside of the try.
      if (curr->hasCatchAll()) {
        self->parent.tryDepth++;
      }
    }

    static void doStartCatch(InternalAnalyzer* self, Expression** currp) {
      Try* curr = (*currp)->cast<Try>();
      // This is conservative. When an inner try-delegate targets the current
      // expression, even 