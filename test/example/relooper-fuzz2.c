#include <assert.h>
#include <stdio.h>

#include "binaryen-c.h"

// globals: address 4 is index
// decisions are at address 8+

int main() {
  BinaryenModuleRef module = BinaryenModuleCreate();

  // check()

  // if the end, halt
  BinaryenExpressionRef halter = BinaryenIf(
    module,
    BinaryenBinary(
      module,
      BinaryenGeUInt32(),
      BinaryenLoad(module,
                   4,
                   0,
                   0,
                   0,
                   BinaryenTypeInt32(),
                   BinaryenConst(module, BinaryenLiteralInt32(4)),
                   "0"),
      BinaryenConst(module, BinaryenLiteralInt32(4 * 27)) // jumps of 4 bytes
      ),
    BinaryenUnreachable(module),
    NULL);
  // increment index
  BinaryenExpressionRef incer = BinaryenStore(
    module,
    4,
    0,
    0,
    BinaryenConst(module, BinaryenLiteralInt32(4)),
    BinaryenBinary(module,
                   BinaryenAddInt32(),
                   BinaryenLoad(module,
                                4,
                                0,
                                0,
                                0,
                                BinaryenTypeInt32(),
                                BinaryenConst(module, BinaryenLiteralInt32(4)),
                                "0"),
                   BinaryenConst(module, BinaryenLiteralInt32(4))),
    BinaryenTypeInt32(),
    "0");

  // optionally, print the return value
  BinaryenExpressionRef args[] = {BinaryenBinary(
    module,
    BinaryenSubInt32(),
    BinaryenConst(module, BinaryenLiteralInt32(0)),
    BinaryenLoad(module,
                 4,
                 0,
                 4,
                 0,
                 BinaryenTypeInt32(),
                 BinaryenLoad(module,
                              4,
                              0,
                              0,
                              0,
                              BinaryenTypeInt32(),
                              BinaryenConst(module, BinaryenLiteralInt32(4)),
                              "0"),
                 "0"))};
  BinaryenExpressionRef debugger;
  if (1)
    debugger = BinaryenCall(module, "print", args, 1, BinaryenTypeNone());
  else
    debugger = BinaryenNop(module);

  // return the decision. need to subtract 4 that we just added,
  // and add 8 since that's where we start, so overall offset 4
  BinaryenExpressionRef returner =
    BinaryenLoad(module,
                 4,
                 0,
                 4,
                 0,
                 BinaryenTypeInt32(),
                 BinaryenLoad(module,
                              4,
                              0,
                              0,
                              0,
                              BinaryenTypeInt32(),
                              BinaryenConst(module, BinaryenLiteralInt32(4)),
                              "0"),
                 "0");
  BinaryenExpressionRef checkBodyList[] = {halter, incer, debugger, returner};
  BinaryenExpressionRef checkBody =
    BinaryenBlock(module,
                  NULL,
                  checkBodyList,
                  sizeof(checkBodyList) / sizeof(BinaryenExpressionRef),
                  BinaryenTypeInt32());
  BinaryenAddFunction(module,
                      "check",
                      BinaryenTypeNone(),
                      BinaryenTypeInt32(),
                      NULL,
                      0,
                      checkBody);

  // contents of main() begin here

  RelooperRef relooper = RelooperCreate(module);

  RelooperBlockRef b0;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b0 = RelooperAddBlock(
      relooper, BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()));
  }

  RelooperBlockRef b1;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b1 = RelooperAddBlock(
      relooper, BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()));
  }

  RelooperBlockRef b2;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b2 = RelooperAddBlockWithSwitch(
      relooper,
      BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()),
      BinaryenBinary(module,
                     BinaryenRemUInt32(),
                     BinaryenLocalGet(module, 0, BinaryenTypeInt32()),
                     BinaryenConst(module, BinaryenLiteralInt32(2))));
  }

  RelooperBlockRef b3;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b3 = RelooperAddBlockWithSwitch(
      relooper,
      BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()),
      BinaryenBinary(module,
                     BinaryenRemUInt32(),
                     BinaryenLocalGet(module, 0, BinaryenTypeInt32()),
                     BinaryenConst(module, BinaryenLiteralInt32(1))));
  }

  RelooperBlockRef b4;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(4))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b4 = RelooperAddBlock(
      relooper, BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()));
  }

  RelooperBlockRef b5;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b5 = RelooperAddBlockWithSwitch(
      relooper,
      BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()),
      BinaryenBinary(module,
                     BinaryenRemUInt32(),
                     BinaryenLocalGet(module, 0, BinaryenTypeInt32()),
                     BinaryenConst(module, BinaryenLiteralInt32(1))));
  }

  RelooperBlockRef b6;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(6))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b6 = RelooperAddBlockWithSwitch(
      relooper,
      BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()),
      BinaryenBinary(module,
                     BinaryenRemUInt32(),
                     BinaryenLocalGet(module, 0, BinaryenTypeInt32()),
                     BinaryenConst(module, BinaryenLiteralInt32(3))));
  }

  RelooperBlockRef b7;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b7 = RelooperAddBlock(
      relooper, BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()));
  }

  RelooperBlockRef b8;
  {
    BinaryenExpressionRef args[] = {
      BinaryenConst(module, BinaryenLiteralInt32(0))};
    BinaryenExpressionRef list[] = {
      BinaryenCall(module, "print", args, 1, BinaryenTypeNone()),
      BinaryenLocalSet(
        module,
        0,
        BinaryenCall(module, "check", NULL, 0, BinaryenTypeInt32()))};

    b8 = RelooperAddBlock(
      relooper, BinaryenBlock(module, NULL, list, 2, BinaryenTypeNone()));
  }

  RelooperAddBranch(
    b0,
    b1,
    NULL,
    BinaryenStore(module,
                  4,
                  0,
                  0,
                  BinaryenConst(module, BinaryenLiteralInt32(4)),
                  BinaryenBinary(
                    module,
                    BinaryenAddInt32(),
                    BinaryenLoad(module,
                                 4,
                                 0,
                                 0,
                                 0,
                                 BinaryenTypeInt32(),
                                 BinaryenConst(module, BinaryenLiteralInt32(4)),
                                 "0"),
                    BinaryenConst(module, BinaryenLiteralInt32(4 * 4))),
                  BinaryenTypeInt32(),
                  "0"));

  RelooperAddBranch(
    b1,
    b1,
    NULL,
    BinaryenStore(module,
                  4,
                  0,
                  0,
                  BinaryenConst(module, BinaryenLiteralInt32(4)),
                  BinaryenBinary(
                    module,
                    BinaryenAddInt32(),
                    BinaryenLoad(module,
                                 4,
                                 0,
                                 0,
                                 0,
                                 BinaryenTypeInt32(),
                                 BinaryenConst(module, BinaryenLiteralInt32(4)),
                                 "0"),
                    BinaryenConst(module, BinaryenLiteralInt32(4 * 4))),
                  BinaryenTypeInt32(),
                  "0"));

  {
    BinaryenIndex values[] = {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20,
                              22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42,
                              44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64,
                              66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86};
    RelooperAddBranchForSwitch(
      b2,
      b7,
      values,
      sizeof(values) / sizeof(BinaryenIndex),
      BinaryenStore(
        module,
        4,
        0,
        0,
        BinaryenConst(module, BinaryenLiteralInt32(4)),
        BinaryenBinary(
          module,
          BinaryenAddInt32(),
          BinaryenLoad(module,
                       4,
                       0,
                       0,
                       0,
                       BinaryenTypeInt32(),
                       BinaryenConst(module, BinaryenLiteralInt32(4)),
                       "0"),
          BinaryenConst(module, BinaryenLiteralInt32(4 * 6))),
        BinaryenTypeInt32(),
        "0"));
  }

  RelooperAddBranchForSwitch(
    b2,
    b4,
    NULL,
    0,
    BinaryenStore(module,
                  4,
                  0,
                  0,
                  BinaryenConst(module, BinaryenLiteralInt32(4)),
                  BinaryenBinary(
           