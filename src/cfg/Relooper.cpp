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
        // prepare for more nesting
        Outer = NextOuter;
      } else {
        CurrName = SwitchLeave; // just go out straight from the table
        if (!Details->SwitchValues) {
          // this is the default, and it has no content. So make the default be
          // the leave
          for (auto& Value : Table) {
            if (Value == SwitchDefault) {
              Value = SwitchLeave;
            }
          }
          SwitchDefault = SwitchLeave;
        }
      }
      if (Details->SwitchValues) {
        for (auto Value : *Details->SwitchValues) {
          while (Table.size() <= Value) {
            Table.push_back(SwitchDefault);
          }
          Table[Value] = CurrName;
        }
      }
    }
    // finish up the whole pattern
    Outer->name = SwitchLeave;
    Inner->list.push_back(
      Builder.makeSwitch(Table, SwitchDefault, SwitchCondition));
    Root = Outer;
  }

  if (Root) {
    Ret->list.push_back(Root);
  }
  Ret->finalize();

  return Ret;
}

// SimpleShape

wasm::Expression* SimpleShape::Render(RelooperBuilder& Builder, bool InLoop) {
  auto* Ret = Inner->Render(Builder, InLoop);
  Ret = HandleFollowupMultiples(Ret, this, Builder, InLoop);
  if (Next) {
    Ret = Builder.makeSequence(Ret, Next->Render(Builder, InLoop));
  }
  return Ret;
}

// MultipleShape

wasm::Expression* MultipleShape::Render(RelooperBuilder& Builder, bool InLoop) {
  // TODO: consider switch
  // emit an if-else chain
  wasm::If* FirstIf = nullptr;
  wasm::If* CurrIf = nullptr;
  std::vector<wasm::If*> finalizeStack;
  for (auto& [Id, Body] : InnerMap) {
    auto* Now =
      Builder.makeIf(Builder.makeCheckLabel(Id), Body->Render(Builder, InLoop));
    finalizeStack.push_back(Now);
    if (!CurrIf) {
      FirstIf = CurrIf = Now;
    } else {
      CurrIf->ifFalse = Now;
      CurrIf->finalize();
      CurrIf = Now;
    }
  }
  while (finalizeStack.size() > 0) {
    wasm::If* curr = finalizeStack.back();
    finalizeStack.pop_back();
    curr->finalize();
  }
  wasm::Expression* Ret = Builder.makeBlock(FirstIf);
  Ret = HandleFollowupMultiples(Ret, this, Builder, InLoop);
  if (Next) {
    Ret = Builder.makeSequence(Ret, Next->Render(Builder, InLoop));
  }
  return Ret;
}

// LoopShape

wasm::Expression* LoopShape::Render(RelooperBuilder& Builder, bool InLoop) {
  wasm::Expression* Ret = Builder.makeLoop(Builder.getShapeContinueName(Id),
                                           Inner->Render(Builder, true));
  Ret = HandleFollowupMultiples(Ret, this, Builder, InLoop);
  if (Next) {
    Ret = Builder.makeSequence(Ret, Next->Render(Builder, InLoop));
  }
  return Ret;
}

// Relooper

Relooper::Relooper(wasm::Module* ModuleInit)
  : Module(ModuleInit), Root(nullptr), MinSize(false), BlockIdCounter(1),
    ShapeIdCounter(0) { // block ID 0 is reserved for clearings
}

Block* Relooper::AddBlock(wasm::Expression* CodeInit,
                          wasm::Expression* SwitchConditionInit) {

  auto block = std::make_unique<Block>(this, CodeInit, SwitchConditionInit);
  block->Id = BlockIdCounter++;
  auto* blockPtr = block.get();
  Blocks.push_back(std::move(block));
  return blockPtr;
}

Branch* Relooper::AddBranch(wasm::Expression* ConditionInit,
                            wasm::Expression* CodeInit) {
  auto branch = std::make_unique<Branch>(ConditionInit, CodeInit);
  auto* branchPtr = branch.get();
  Branches.push_back(std::move(branch));
  return branchPtr;
}
Branch* Relooper::AddBranch(std::vector<wasm::Index>&& ValuesInit,
                            wasm::Expression* CodeInit) {
  auto branch = std::make_unique<Branch>(std::move(ValuesInit), CodeInit);
  auto* branchPtr = branch.get();
  Branches.push_back(std::move(branch));
  return branchPtr;
}

SimpleShape* Relooper::AddSimpleShape() {
  auto shape = std::make_unique<SimpleShape>();
  shape->Id = ShapeIdCounter++;
  auto* shapePtr = shape.get();
  Shapes.push_back(std::move(shape));
  return shapePtr;
}

MultipleShape* Relooper::AddMultipleShape() {
  auto shape = std::make_unique<MultipleShape>();
  shape->Id = ShapeIdCounter++;
  auto* shapePtr = shape.get();
  Shapes.push_back(std::move(shape));
  return shapePtr;
}

LoopShape* Relooper::AddLoopShape() {
  auto shape = std::make_unique<LoopShape>();
  shape->Id = ShapeIdCounter++;
  auto* shapePtr = shape.get();
  Shapes.push_back(std::move(shape));
  return shapePtr;
}

namespace {

using BlockList = std::list<Block*>;

struct RelooperRecursor {
  Relooper* Parent;
  RelooperRecursor(Relooper* ParentInit) : Parent(ParentInit) {}
};

struct Liveness : public RelooperRecursor {
  Liveness(Relooper* Parent) : RelooperRecursor(Parent) {}
  BlockSet Live;

  void FindLive(Block* Root) {
    BlockList ToInvestigate;
    ToInvestigate.push_back(Root);
    while (ToInvestigate.size() > 0) {
      Block* Curr = ToInvestigate.front();
      ToInvestigate.pop_front();
      if (contains(Live, Curr)) {
        continue;
      }
      Live.insert(Curr);
      for (auto& iter : Curr->BranchesOut) {
        ToInvestigate.push_back(iter.first);
      }
    }
  }
};

using BranchBlock = std::pair<Branch*, Block*>;

struct Optimizer : public RelooperRecursor {
  Block* Entry;

  Optimizer(Relooper* Parent, Block* EntryInit)
    : RelooperRecursor(Parent), Entry(EntryInit) {
    // TODO: there are likely some rare but possible O(N^2) cases with this
    // looping
    bool More = true;
#if RELOOPER_OPTIMIZER_DEBUG
    std::cout << "pre-optimize\n";
    for (auto* Block : Parent->Blocks) {
      DebugDump(Block, "pre-block");
    }
#endif

    // First, run one-time preparatory passes.
    CanonicalizeCode();

    // Loop over passes that allow further reduction.
    while (More) {
      More = false;
      More = SkipEmptyBlocks() || More;
      More = MergeEquivalentBranches() || More;
      More = UnSwitch() || More;
      More = MergeConsecutiveBlocks() || More;
      // TODO: Merge identical blocks. This would avoid taking into account
      // their position / how they are reached, which means that the merging may
      // add overhead, so we do it carefully:
      //  * Merging a large-enough block is good for size, and we do it
      //    in we are in MinSize mode, which means we can tolerate slightly
      //    slower throughput.
      // TODO: Fuse a non-empty block with a single successor.
    }

    // Finally, run one-time final passes.
    // TODO

#if RELOOPER_OPTIMIZER_DEBUG
    std::cout << "post-optimize\n";
    for (auto* Block : Parent->Blocks) {
      DebugDump(Block, "post-block");
    }
#endif
  }

  // We will be performing code comparisons, so do some basic canonicalization
  // to avoid things being unequal for silly reasons.
  void CanonicalizeCode() {
    for (auto& Block : Parent->Blocks) {
      Block->Code = Canonicalize(Block->Code);
      for (auto& [_, Branch] : Block->BranchesOut) {
        if (Branch->Code) {
          Branch->Code = Canonicalize(Branch->Code);
        }
      }
    }
  }

  // If a branch goes to an empty block which has one target,
  // and there is no phi or switch to worry us, just skip through.
  bool SkipEmptyBlocks() {
    bool Worked = false;
    for (auto& CurrBlock : Parent->Blocks) {
      // Generate a new set of branches out TODO optimize
      BlockBranchMap NewBranchesOut;
      for (auto& iter : CurrBlock->BranchesOut) {
        auto* Next = iter.first;
        auto* NextBranch = iter.second;
        auto* First = Next;
        auto* Replacement = First;
#if RELOOPER_OPTIMIZER_DEBUG
        std::cout << " maybeskip from " << Block->Id << " to next=" << Next->Id
                  << '\n';
#endif
        std::unordered_set<decltype(Replacement)> Seen;
        while (1) {
          if (IsEmpty(Next) && Next->BranchesOut.size() == 1) {
            auto iter = Next->BranchesOut.begin();
            Block* NextNext = iter->first;
            Branch* NextNextBranch = iter->second;
            assert(!NextNextBranch->Condition && !NextNextBranch->SwitchValues);
            if (!NextNextBranch->Code) { // TODO: handle extra code too
              // We can skip through!
              Next = Replacement = NextNext;
              // If we've already seen this, stop - it's an infinite loop of
              // empty blocks we can skip through.
              if (!Seen.emplace(Replacement).second) {
                // Stop here. Note that if we started from X and ended up with X
                // once more, then Replacement == First and so lower down we
                // will not report that we did any work, avoiding an infinite
                // loop due to always thinking there is more work to do.
                break;
              } else {
                // Otherwise, keep going.
                continue;
              }
            }
          }
          break;
        }
        if (Replacement != First) {
#if RELOOPER_OPTIMIZER_DEBUG
          std::cout << "  skip to replacement! " << CurrBlock->Id << " -> "
                    << First->Id << " -> " << Replacement->Id << '\n';
#endif
          Worked = true;
        }
        // Add a branch to the target (which may be the unchanged original) in
        // the set of new branches. If it's a replacement, it may collide, and
        // we need to merge.
        if (NewBranchesOut.count(Replacement)) {
#if RELOOPER_OPTIMIZER_DEBUG
          std::cout << "  merge\n";
#endif
          MergeBranchInto(NextBranch, NewBranchesOut[Replacement]);
        } else {
          NewBranchesOut[Replacement] = NextBranch;
        }
      }
      CurrBlock->BranchesOut.swap(NewBranchesOut);
    }
    return Worked;
  }

  // Our IR has one Branch from each block to one of its targets, so there
  // is nothing to reduce there, but different targets may in fact be
  // equivalent in their *contents*.
  bool MergeEquivalentBranches() {
    bool Worked = false;
    for (auto& ParentBlock : Parent->Blocks) {
#if RELOOPER_OPTIMIZER_DEBUG
      std::cout << "at parent " << ParentBlock->Id << '\n';
#endif
      if (ParentBlock->BranchesOut.size() >= 2) {
        std::unordered_map<size_t, std::vector<BranchBlock>> HashedBranchesOut;
        std::vector<Block*> BlocksToErase;
        for (auto& [CurrBlock, CurrBranch] : ParentBlock->BranchesOut) {
#if RELOOPER_OPTIMIZER_DEBUG
          std::cout << "  consider child " << CurrBlock->Id << '\n';
#endif
          if (CurrBranch->Code) {
            // We can't merge code; ignore
            continue;
          }
          auto HashValue = Hash(CurrBlock);
          auto& HashedSiblings = HashedBranchesOut[HashValue];
          // Check if we are equivalent to any of them - if so, merge us.
          bool Merged = false;
          for (auto& [SiblingBranch, SiblingBlock] : HashedSiblings) {
            if (HaveEquivalentContents(CurrBlock, SiblingBlock)) {
#if RELOOPER_OPTIMIZER_DEBUG
              std::cout << "    equiv! to " << SiblingBlock->Id << '\n';
#endif
              MergeBranchInto(CurrBranch, SiblingBranch);
              BlocksToErase.push_back(CurrBlock);
              Merged = true;
              Worked = true;
            }
#if RELOOPER_OPTIMIZER_DEBUG
            else {
              std::cout << "    same hash, but not equiv to "
                        << SiblingBlock->Id << '\n';
            }
#endif
          }
          if (!Merged) {
            HashedSiblings.emplace_back(CurrBranch, CurrBlock);
          }
        }
        for (auto* Curr : BlocksToErase) {
          ParentBlock->BranchesOut.erase(Curr);
        }
      }
    }
    return Worked;
  }

  // Merge consecutive blocks, that is, A -> B where no other branches go to B.
  // In that case we are guaranteed to not increase code size.
  bool MergeConsecutiveBlocks() {
    bool Worked = false;
    // First, count predecessors.
    std::map<Block*, size_t> NumPredecessors;
    for (auto& CurrBlock : Parent->Blocks) {
      for (auto& iter : CurrBlock->BranchesOut) {
        auto* NextBlock = iter.first;
        NumPredecessors[NextBlock]++;
      }
    }
    NumPredecessors[Entry]++;
    for (auto& CurrBlock : Parent->Blocks) {
      if (CurrBlock->BranchesOut.size() == 1) {
        auto iter = CurrBlock->BranchesOut.begin();
        auto* NextBlock = iter->first;
        auto* NextBranch = iter->second;
        assert(NumPredecessors[NextBlock] > 0);
        if (NextBlock != CurrBlock.get() && NumPredecessors[NextBlock] == 1) {
          // Good to merge!
          wasm::Builder Builder(*Parent->Module);
          // Merge in code on the branch as well, if any.
          if (NextBranch->Code) {
            CurrBlock->Code =
              Builder.makeSequence(CurrBlock->Code, NextBranch->Code);
          }
          CurrBlock->Code =
            Builder.makeSequence(CurrBlock->Code, NextBlock->Code);
          // Use the next block's branching behavior
          CurrBlock->BranchesOut.swap(NextBlock->BranchesOut);
          NextBlock->BranchesOut.clear();
          CurrBlock->SwitchCondition = NextBlock->SwitchCondition;
          // The next block now has no predecessors.
          NumPredecessors[NextBlock] = 0;
          Worked = true;
        }
      }
    }
    return Worked;
  }

  // Removes unneeded switches - if only one branch is left, the default, then
  // no switch is needed.
  bool UnSwitch() {
    bool Worked = false;
    for (auto& ParentBlock : Parent->Blocks) {
#if RELOOPER_OPTIMIZER_DEBUG
      std::cout << "un-switching at " << ParentBlock->Id << ' '
                << !!ParentBlock->SwitchCondition << ' '
                << ParentBlock->BranchesOut.size() << '\n';
#endif
      if (ParentBlock->SwitchCondition) {
        if (ParentBlock->BranchesOut.size() <= 1) {
#if RELOOPER_OPTIMIZER_DEBUG
          std::cout << "  un-switching!: " << ParentBlock->Id << '\n';
#endif
          ParentBlock->SwitchCondition = nullptr;
          if (!ParentBlock->BranchesOut.empty()) {
            assert(!ParentBlock->BranchesOut.begin()->second->SwitchValues);
          }
          Worked = true;
        }
      } else {
        // If the block has no switch, the branches must not as well.
#ifndef NDEBUG
        for (auto& [_, CurrBranch] : ParentBlock->BranchesOut) {
          assert(!CurrBranch->SwitchValues);
        }
#endif
      }
    }
    return Worked;
  }

private:
  wasm::Expression* Canonicalize(wasm::Expression* Curr) {
    wasm::Builder Builder(*Parent->Module);
    // Our preferred form is a block with no name and a flat list
    // with Nops removed, and extra Unreachables removed as well.
    // If the block would contain one item, return just the item.
    wasm::Block* Outer = Curr->dynCast<wasm::Block>();
    if (!Outer) {
      Outer = Builder.makeBlock(Curr);
    } else if (Outer->name.is()) {
      // Perhaps the name can be removed.
      if (!wasm::BranchUtils::BranchSeeker::has(Outer, Outer->name)) {
        Outer->name = wasm::Name();
      } else {
        Outer = Builder.makeBlock(Curr);
      }
    }
    Flatten(Outer);
    if (Outer->list.size() == 1) {
      return Outer->list[0];
    } else {
      return Outer;
    }
  }

  void Flatten(wasm::Block* Outer) {
    wasm::ExpressionList NewList(Parent->Module->allocator);
    bool SeenUnreachableType = false;
    auto Add = [&](wasm::Expression* Curr) {
      if (Curr->is<wasm::Nop>()) {
        // Do nothing with it.
        return;
      } else if (Curr->is<wasm::Unreachable>()) {
        // If we already saw an unreachable-typed item, emit no
        // Unreachable nodes after it.
        if (SeenUnreachableType) {
          return;
        }
      }
      NewList.push_back(Curr);
      if (Curr->type == wasm::Type::unreachable) {
        SeenUnreachableType = true;
      }
    };
    std::function<void(wasm::Block*)> FlattenIntoNewList =
      [&](wasm::Block* Curr) {
        assert(!Curr->name.is());
        for (auto* Item : Curr->list) {
          if (auto* Block = Item->dynCast<wasm::Block>()) {
            if (Block->name.is()) {
              // Leave it whole, it's not a trivial block.
              Add(Block);
            } else {
              FlattenIntoNewList(Block);
            }
          } else {
            // A random item.
            Add(Item);
          }
        }
        // All the items have been moved out.
        Curr->list.clear();
      };
    FlattenIntoNewList(Outer);
    assert(Outer->list.empty());
    Outer->list.swap(NewList);
  }

  bool IsEmpty(Block* Curr) {
    if (Curr->SwitchCondition) {
      // This is non-trivial, so treat it as a non-empty block.
      return false;
    }
    return IsEmpty(Curr->Code);
  }

  bool IsEmpty(wasm::Expression* Code) {
    if (Code->is<wasm::Nop>()) {
      return true; // a nop
    }
    if (auto* WasmBlock = Code->dynCast<wasm::Block>()) {
      for (auto* Item : WasmBlock->list) {
        if (!IsEmpty(Item)) {
          return false;
        }
      }
      return true; // block with no non-empty contents
    }
    return false;
  }

  // Checks functional equivalence, namely: the Code and SwitchCondition.
  // We also check the branches out, *non-recursively*: that is, we check
  // that they are literally identical, not that they can be computed to
  // be equivalent.
  bool HaveEquivalentContents(Block* A, Block* B) {
    if (!IsPossibleCodeEquivalent(A->SwitchCondition, B->SwitchCondition)) {
      return false;
    }
    if (!IsCodeEquivalent(A->Code, B->Code)) {
      return false;
    }
    if (A->BranchesOut.size() != B->BranchesOut.size()) {
      return false;
    }
    for (auto& [ABlock, ABranch] : A->BranchesOut) {
      if (B->BranchesOut.count(ABlock) == 0) {
        return false;
      }
      auto* BBranch = B->BranchesOut[ABlock];
      if (!IsPossibleCodeEquivalent(ABranch->Condition, BBranch->Condition)) {
        return false;
      }
      if (!IsPossibleUniquePtrEquivalent(ABranch->SwitchValues,
                                         BBranch->SwitchValues)) {
        return false;
      }
      if (!IsPossibleCodeEquivalent(ABranch->Code, BBranch->Code)) {
        return false;
      }
    }
    return true;
  }

  // Checks if values referred to by pointers are identical, allowing the code
  // to also be nullptr
  template<typename T>
  static bool IsPossibleUniquePtrEquivalent(std::unique_ptr<T>& A,
                                            std::unique_ptr<T>& B) {
    if (A == B) {
      return true;
    }
    if (!A || !B) {
      return false;
    }
    return *A == *B;
  }

  // Checks if code is equivalent, allowing the code to also be nullptr
  static bool IsPossibleCodeEquivalent(wasm::Expression* A,
                                       wasm::Expression* B) {
    if (A == B) {
      return true;
    }
    if (!A || !B) {
      return false;
    }
    return IsCodeEquivalent(A, B);
  }

  static bool IsCodeEquivalent(wasm::Expression* A, wasm::Expression* B) {
    return wasm::ExpressionAnalyzer::equal(A, B);
  }

  // Merges one branch into another. Valid under the assumption that the
  // blocks they reach are identical, and so one branch is enough for both
  // with a unified condition.
  // Only one is allowed to have code, as the code may have side effects,
  // and we don't have a way to order or resolve those, unless the code
  // is equivalent.
  void MergeBranchInto(Branch* Curr, Branch* Into) {
    assert(Curr != Into);
    if (Curr->SwitchValues) {
      if (!Into->SwitchValues) {
        assert(!Into->Condition);
        // Merging into the already-default, nothing to do.
      } else {
        Into->SwitchValues->insert(Into->SwitchValues->end(),
                                   Curr->SwitchValues->begin(),
                                   Curr->SwitchValues->end());
      }
    } else {
      if (!Curr->Condition) {
        // This is now the new default. Whether Into has a condition
        // or switch values, remove them all to make us the default.
        Into->Condition = nullptr;
        Into->SwitchValues.reset();
      } else if (!Into->Condition) {
        // Nothing to do, already the default.
      } else {
        assert(!Into->SwitchValues);
        // Merge them, checking both.
        Into->Condition =
          wasm::Builder(*Parent->Module)
            .makeBinary(wasm::OrInt32, Into->Condition, Curr->Condition);
      }
    }
    if (!Curr->Code) {
      // No code to merge in.
    } else if (!Into->Code) {
      // Just use the code being merged in.
      Into->Code = Curr->Code;
    } else {
      assert(IsCodeEquivalent(Into->Code, Curr->Code));
      // Keep the code already there, either is fine.
    }
  }

  // Hashes the direct block contents, but not Relooper internals
  // (like Shapes). Only partially hashes the branches out, no
  // recursion: hashes the branch infos, looks at raw pointers
  // for the blocks.
  size_t Hash(Block* Curr) {
    auto digest = wasm::ExpressionAnalyzer::hash(Curr->Code);
    wasm::rehash(digest, uint8_t(1));
    if (Curr->SwitchCondition) {
      wasm::hash_combine(digest,
                         wasm::ExpressionAnalyzer::hash(Curr->SwitchCondition));
    }
    wasm::rehash(digest, uint8_t(2));
    for (auto& [CurrBlock, CurrBranch] : Curr->BranchesOut) {
      // Hash the Block* as a pointer TODO: full hash?
      wasm::rehash(digest, reinterpret_cast<size_t>(CurrBlock));
      // Hash the Branch info properly
      wasm::hash_combine(digest, Hash(CurrBranch));
    }
    return digest;
  }

  // Hashes the direct block contents, but not Relooper internals
  // (like Shapes).
  size_t Hash(Branch* Curr) {
    auto digest = wasm::hash(0);
    if (Curr->SwitchValues) {
      for (auto i : *Curr->SwitchValues) {
        wasm::rehash(digest, i); // TODO hash i
      }
    } else {
      if (Curr->Condition) {
        wasm::hash_combine(digest,
                           wasm::ExpressionAnalyzer::hash(Curr->Condition));
      }
    }
    wasm::rehash(digest, uint8_t(1));
    if (Curr->Code) {
      wasm::hash_combine(digest, wasm::ExpressionAnalyzer::hash(Curr->Code));
    }
    return digest;
  }
};

} // namespace

void Relooper::Calculate(Block* Entry) {
  // Optimize.
  Optimizer(this, Entry);

  // Find live blocks.
  Liveness Live(this);
  Live.FindLive(Entry);

  // Add incoming branches from live blocks, ignoring dead code
  for (unsigned i = 0; i < Blocks.size(); i++) {
    Block* Curr = Blocks[i].get();
    if (!contains(Live.Live, Curr)) {
      continue;
    }
    for (auto& [CurrBlock, _] : Curr->BranchesOut) {
      CurrBlock->BranchesIn.insert(Curr);
    }
  }

  // Recursively process the graph

  struct Analyzer : public RelooperRecursor {
    Analyzer(Relooper* Parent) : RelooperRecursor(Parent) {}

    // Create a list of entries from a block. If LimitTo is provided, only
    // results in that set will appear
    void GetBlocksOut(Block* Source,
                      BlockSet& Entries,
                      BlockSet* LimitTo = nullptr) {
      for (auto& [CurrBlock, _] : Source->BranchesOut) {
        if (!LimitTo || contains(*LimitTo, CurrBlock)) {
          Entries.insert(CurrBlock);
        }
      }
    }

    // Converts/processes all branchings to a specific target
    void Solipsize(Block* Target,
                   Branch::FlowType Type,
                   Shape* Ancestor,
                   BlockSet& From) {
      PrintDebug("Solipsizing branches into %d\n", Target->Id);
      DebugDump(From, "  relevant to solipsize: ");
      for (auto iter = Target->BranchesIn.begin();
           iter != Target->BranchesIn.end();) {
        Block* Prior = *iter;
        if (!contains(From, Prior)) {
          iter++;
          continue;
        }
        Branch* PriorOut = Prior->BranchesOut[Target];
        PriorOut->Ancestor = Ancestor;
        PriorOut->Type = Type;
        iter++; // carefully increment iter before erasing
        Target->BranchesIn.erase(Prior);
        Target->Processe