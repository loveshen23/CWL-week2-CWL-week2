/*
 * Copyright 2016 WebAssembly Community Group participants
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

#include "Relooper.h"

#include <stdlib.h>
#include <string.h>

#include <list>
#include <stack>
#include <string>

#include "ir/branch-utils.h"
#include "ir/utils.h"
#include "parsing.h"

namespace CFG {

template<class T, class U>
static bool contains(const T& container, const U& contained) {
  return !!container.count(contained);
}

#ifdef RELOOPER_DEBUG
static void PrintDebug(const char* Format, ...);
#define DebugDump(x, ...) Debugging::Dump(x, __VA_ARGS__)
#else
#define PrintDebug(x, ...)
#define DebugDump(x, ...)
#endif

// Rendering utilities

static wasm::Expression* HandleFollowupMultiples(wasm::Expression* Ret,
                                                 Shape* Parent,
                                                 RelooperBuilder& Builder,
                                                 bool InLoop) {
  if (!Parent->Next) {
    return Ret;
  }

  auto* Curr = Ret->dynCast<wasm::Block>();
  if (!Curr || Curr->name.is()) {
    Curr = Builder.makeBlock(Ret);
  }
  // for each multiple after us, we create a block target for breaks to reach
  while (Parent->Next) {
    auto* Multiple = Shape::IsMultiple(Parent->Next);
    if (!Multiple) {
      break;
    }
    for (auto& [Id, Body] : Multiple->InnerMap) {
      Curr->name = Builder.getBlockBreakName(Id);
      Curr->finalize(); // it may now be reachable, via a break
      auto* Outer = Builder.makeBlock(Curr);
      Outer->list.push_back(Body->Render(Builder, InLoop));
      Outer->finalize(); // TODO: not really necessary
      Curr = Outer;
    }
    Parent->Next = Parent->Next->Next;
  }
  // after the multiples is a simple or a loop, in both cases we must hit an
  // entry block, and so this is the last one we need to take into account now
  // (this is why we require that loops hit an entry).
  if (Parent->Next) {
    auto* Simple = Shape::IsSimple(Parent->Next);
    if (Simple) {
      // breaking on the next block's id takes us out, where we
      // will reach its rendering
      Curr->name = Builder.getBlockBreakName(Simple->Inner->Id);
    } else {
      // add one break target per entry for the loop
      auto* Loop = Shape::IsLoop(Parent->Next);
      assert(Loop);
      assert(Loop->Entries.size() > 0);
      if (Loop->Entries.size() == 1) {
        Curr->name = Builder.getBlockBreakName((*Loop->Entries.begin())->Id);
      } else {
        for (auto* Entry : Loop->Entries) {
          Curr->name = Builder.getBlockBreakName(Entry->Id);
          Curr->finalize();
          auto* Outer = Builder.makeBlock(Curr);
          Outer->finalize(); // TODO: not really necessary
          Curr = Outer;
        }
      }
    }
  }
  Curr->finalize();
  return Curr;
}

// Branch

Branch::Branch(wasm::Expression* ConditionInit, wasm::Expression* CodeInit)
  : Condition(ConditionInit), Code(CodeInit) {}

Branch::Branch(std::vector<wasm::Index>&& ValuesInit,
               wasm::Expression* CodeInit)
  : Condition(nullptr), Code(CodeInit) {
  if (ValuesInit.size() > 0) {
    SwitchValues = wasm::make_unique<std::vector<wasm::Index>>(ValuesInit);
  }
  // otherwise, it is the default
}

wasm::Expression*
Branch::Render(RelooperBuilder& Builder, Block* Target, bool SetLabel) {
  auto* Ret = Builder.makeBlock();
  if (Code) {
    Ret->list.push_back(Code);
  }
  if (SetLabel) {
    Ret->list.push_back(Builder.makeSetLabel(Target->Id));
  }
  if (Type == Break) {
    Ret->list.push_back(Builder.makeBlockBreak(Target->Id));
  } else if (Type == Continue) {
    assert(Ancestor);
    Ret->list.push_back(Builder.makeShapeContinue(Ancestor->Id));
  }
  Ret->finalize();
  return Ret;
}

// Block

Block::Block(Relooper* relooper,
             wasm::Expression* CodeInit,
             wasm::Expression* SwitchConditionInit)
  : relooper(relooper), Code(CodeInit), SwitchCondition(SwitchConditionInit),
    IsCheckedMultipleEntry(false) {}

void Block::AddBranchTo(Block* Target,
                        wasm::Expression* Condition,
                        wasm::Expression* Code) {
  // cannot add more than one branch to the same target
  assert(!contains(BranchesOut, Target));
  BranchesOut[Target] = relooper->AddBranch(Condition, Code);
}

void Block::AddSwitchBranchTo(Block* Target,
                              std::vector<wasm::Index>&& Values,
                              wasm::Expression* Code) {
  // cannot add more than one branch to the same target
  assert(!contains(BranchesOut, Target));
  BranchesOut[Target] = relooper->AddBranch(std::move(Values), Code);
}

wasm::Expression* Block::Render(RelooperBuilder& Builder, bool InLoop) {
  auto* Ret = Builder.makeBlock();
  if (IsCheckedMultipleEntry && InLoop) {
    Ret->list.push_back(Builder.makeSetLabel(0));
  }
  if (Code) {
    Ret->list.push_back(Code);
  }

  if (!ProcessedBranchesOut.size()) {
    Ret->finalize();
    return Ret;
  }

  // in some cases it is clear we can avoid setting label, see later
  bool SetLabel = true;

  // A setting of the label variable (label = x) is necessary if it can
  // cause an impact. The main case is where we set label to x, then elsewhere
  // we check if label is equal to that value, i.e., that label is an entry
  // in a multiple block. We also need to reset the label when we enter
  // that block, so that each setting is a one-time action: consider
  //
  //    while (1) {
  //      if (check) label = 1;
  //      if (label == 1) { label = 0 }
  //    }
  //
  // (Note that this case is impossible due to fusing, but that is not
  // material here.) So setting to 0 is important just to clear the 1 for
  // future iterations.
  // TODO: When inside a loop, if necessary clear the label variable
  //       once on the top, and never do settings that are in effect clears

  // Fusing: If the next is a Multiple, we can fuse it with this block. Note
  // that we must be the Inner of a Simple, so fusing means joining a Simple
  // to a Multiple. What happens there is that all options in the Multiple
  // *must* appear in the Simple (the Simple is the only one reaching the
  // Multiple), so we can remove the Multiple and add its independent groups
  // into the Simple's branches.
  MultipleShape* Fused = Shape::IsMultiple(Parent->Next);
  if (Fused) {
    PrintDebug("Fusing Multiple to Simple\n", 0);
    Parent->Next = Parent->Next->Next;
    // When the Multiple has the same number of groups as we have branches,
    // they will all be fused, so it is safe to not set the label at all.
    // If a switch, then we can have multiple branches to the same target
    // (in different table indexes), and so this check is not sufficient
    // TODO: optimize
    if (SetLabel && Fused->InnerMap.size() == ProcessedBranchesOut.size() &&
        !SwitchCondition) {
      SetLabel = false;
    }
  }

  // The block we branch to without checking the condition, if none of the other
  // conditions held.
  Block* DefaultTarget = nullptr;

  // Find the default target, the one without a condition
  for (auto& [Target, Details] : ProcessedBranchesOut) {
    if ((!SwitchCondition && !Details->Condition) ||
        (SwitchCondition && !Details->SwitchValues)) {
      assert(!DefaultTarget &&
             "block has branches without a default (nullptr for the "
             "condition)"); // Must be exactly one default // nullptr
      DefaultTarget = Target;
    }
  }
  // Since each Block* must* branch somewhere, this must be set
  assert(DefaultTarget);

  // root of the main part, that we are about to emit
  wasm::Expression* Root = nullptr;

  if (!SwitchCondition) {
    // We'll emit a chain of if-elses
    wasm::If* CurrIf = nullptr;

    // we build an if, then add a child, then add a child to that, etc., so we
    // must finalize them in reverse order
    std::vector<wasm::If*> finalizeStack;

    wasm::Expression* RemainingConditions = nullptr;

    for (auto iter = ProcessedBranchesOut.begin();; iter++) {
      Block* Target;
      Branch* Details;
      if (iter != ProcessedBranchesOut.end()) {
        Target = iter->first;
        if (Target == DefaultTarget) {
          continue; // done at the end
        }
        Details = iter->second;
        // must have a condition if this is not the default target
        assert(Details->Condition);
      } else {
        Target = DefaultTarget;
        Details = ProcessedBranchesOut[DefaultTarget];
      }
      bool SetCurrLabel = SetLabel && Target->IsCheckedMultipleEntry;
      bool HasFusedContent = Fused && contains(Fused->InnerMap, Target->Id);
      if (HasFusedContent) {
        assert(Details->Type == Branch::Break);
        Details->Type = Branch::Direct;
      }
      wasm::Expression* CurrContent = nullptr;
      bool IsDefault = iter == ProcessedBranchesOut.end();
      if (SetCurrLabel || Details->Type != Branch::Direct || HasFusedContent ||
          Details->Code) {
        CurrContent = Details->Render(Builder, Target, SetCurrLabel);
        if (HasFusedContent) {
          CurrContent = Builder.blockify(
            CurrContent,
            Fused->InnerMap.find(Target->Id)->second->Render(Builder, InLoop));
        }
      }
      // If there is nothing to show in this branch, omit the condition
      if (CurrContent) {
        if (IsDefault) {
          wasm::Expression* Now;
          if (RemainingConditions) {
            Now = Builder.makeIf(RemainingConditions, CurrContent);
            finalizeStack.push_back(Now->cast<wasm::If>());
          } else {
            Now = CurrContent;
          }
          if (!CurrIf) {
            assert(!Root);
            Root = Now;
          } else {
            CurrIf->ifFalse = Now;
            CurrIf->finalize();
          }
        } else {
          auto* Now = Builder.makeIf(Details->Condition, CurrContent);
          finalizeStack.push_back(Now);
          if (!CurrIf) {
            assert(!Root);
            Root = CurrIf = Now;
          } else {
            CurrIf->ifFalse = Now;
            CurrIf->finalize();
            CurrIf = Now;
          }
        }
      } else {
        auto* Now = Builder.makeUnary(wasm::EqZInt32, Details->Condition);
        if (RemainingConditions) {
          RemainingConditions =
            Builder.makeBinary(wasm::AndInt32, RemainingConditions, Now);
        } else {
          RemainingConditions = Now;
        }
      }
      if (IsDefault) {
        break;
      }
    }

    // finalize the if-chains
    while (finalizeStack.size() > 0) {
      wasm::If* curr = finalizeStack.back();
      finalizeStack.pop_back();
      curr->finalize();
    }

  } else {
    // Emit a switch
    auto Base = std::string("switch$") + std::to_string(Id);
    auto SwitchDefault = wasm::Name(Base + "$default");
    auto SwitchLeave = wasm::Name(Base + "$leave");
    std::map<Block*, wasm::Name> BlockNameMap;
    auto* Outer = Builder.makeBlock();
    auto* Inner = Outer;
    std::vector<wasm::Name> Table;
    for (auto& [Target, Details] : ProcessedBranchesOut) {
      wasm::Name CurrName;
      if (Details->SwitchValues) {
        CurrName = wasm::Name(Base + "$case$" + std::to_string(Target->Id));
      } else {
        CurrName = SwitchDefault;
      }
      // generate the content for this block
      bool SetCurrLabel = SetLabel && Target->IsCheckedMultipleEntry;
      bool HasFusedContent = Fused && contains(Fused->InnerMap, Target->Id);
      if (HasFusedContent) {
        assert(Details->Type == Branch::Break);
        Details->Type = Branch::Direct;
      }
      wasm::Expression* CurrContent = nullptr;
      if (SetCurrLabel || Details->Type != Branch::Direct || HasFusedContent ||
          Details->Code) {
        CurrContent = Details->Render(Builder, Target, SetCurrLabel);
        if (HasFusedContent) {
          CurrContent = Builder.blockify(
            CurrContent,
            Fused->InnerMap.find(Target->Id)->second->Render(Builder, InLoop));
        }
      }
      // generate a block to branch to, if we have content
      if (CurrContent) {
        auto* NextOuter = Builder.makeBlock();
        NextOuter->list.push_back(Outer);
        // breaking on Outer leads to the content in NextOuter
        Outer->name = CurrName;
        NextOuter->list.push_back(CurrContent);
        // if this is not a dead end, also need to break to the outside this is
        // both an optimization, and avoids incorrectness as adding a break in
        // unreachable code can make a place look reachable that isn't
        if (CurrContent->type != wasm::Type::unreachable) {
          NextOuter->list.push_back(Builder.makeBreak(SwitchLeave));
        }
     