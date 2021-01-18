
// export friendly API methods
function preserveStack(func) {
  try {
    var stack = stackSave();
    return func();
  } finally {
    stackRestore(stack);
  }
}

function strToStack(str) {
  return str ? allocateUTF8OnStack(str) : 0;
}

function i32sToStack(i32s) {
  const ret = stackAlloc(i32s.length << 2);
  HEAP32.set(i32s, ret >>> 2);
  return ret;
}

function i8sToStack(i8s) {
  const ret = stackAlloc(i8s.length);
  HEAP8.set(i8s, ret);
  return ret;
}

function initializeConstants() {

  // Types
  [ ['none', 'None'],
    ['i32', 'Int32'],
    ['i64', 'Int64'],
    ['f32', 'Float32'],
    ['f64', 'Float64'],
    ['v128', 'Vec128'],
    ['funcref', 'Funcref'],
    ['externref', 'Externref'],
    ['anyref', 'Anyref'],
    ['eqref', 'Eqref'],
    ['i31ref', 'I31ref'],
    ['structref', 'Structref'],
    ['stringref', 'Stringref'],
    ['stringview_wtf8', 'StringviewWTF8'],
    ['stringview_wtf16', 'StringviewWTF16'],
    ['stringview_iter', 'StringviewIter'],
    ['unreachable', 'Unreachable'],
    ['auto', 'Auto']
  ].forEach(entry => {
    Module[entry[0]] = Module['_BinaryenType' + entry[1]]();
  });

  // Expression ids
  Module['ExpressionIds'] = {};
  [ 'Invalid',
    'Block',
    'If',
    'Loop',
    'Break',
    'Switch',
    'Call',
    'CallIndirect',
    'LocalGet',
    'LocalSet',
    'GlobalGet',
    'GlobalSet',
    'Load',
    'Store',
    'Const',
    'Unary',
    'Binary',
    'Select',
    'Drop',
    'Return',
    'MemorySize',
    'MemoryGrow',
    'Nop',
    'Unreachable',
    'AtomicCmpxchg',
    'AtomicRMW',
    'AtomicWait',
    'AtomicNotify',
    'AtomicFence',
    'SIMDExtract',
    'SIMDReplace',
    'SIMDShuffle',
    'SIMDTernary',
    'SIMDShift',
    'SIMDLoad',
    'SIMDLoadStoreLane',
    'MemoryInit',
    'DataDrop',
    'MemoryCopy',
    'MemoryFill',
    'RefNull',
    'RefIsNull',
    'RefFunc',
    'RefEq',
    'TableGet',
    'TableSet',
    'TableSize',
    'TableGrow',
    'Try',
    'Throw',
    'Rethrow',
    'TupleMake',
    'TupleExtract',
    'Pop',
    'I31New',
    'I31Get',
    'CallRef',
    'RefTest',
    'RefCast',
    'BrOn',
    'StructNew',
    'StructGet',
    'StructSet',
    'ArrayNew',
    'ArrayNewFixed',
    'ArrayGet',
    'ArraySet',
    'ArrayLen',
    'ArrayCopy',
    'RefAs',
    'StringNew',
    'StringConst',
    'StringMeasure',
    'StringEncode',
    'StringConcat',
    'StringEq',
    'StringAs',
    'StringWTF8Advance',
    'StringWTF16Get',
    'StringIterNext',
    'StringIterMove',
    'StringSliceWTF',
    'StringSliceIter'
  ].forEach(name => {
    Module['ExpressionIds'][name] = Module[name + 'Id'] = Module['_Binaryen' + name + 'Id']();
  });

  // External kinds
  Module['ExternalKinds'] = {};
  [ 'Function',
    'Table',
    'Memory',
    'Global',
    'Tag'
  ].forEach(name => {
    Module['ExternalKinds'][name] = Module['External' + name] = Module['_BinaryenExternal' + name]();
  });

  // Features
  Module['Features'] = {};
  [ 'MVP',
    'Atomics',
    'BulkMemory',
    'MutableGlobals',
    'NontrappingFPToInt',
    'SignExt',
    'SIMD128',
    'ExceptionHandling',
    'TailCall',
    'ReferenceTypes',
    'Multivalue',
    'GC',
    'Memory64',
    'RelaxedSIMD',
    'ExtendedConst',
    'Strings',
    'MultiMemories',
    'All'
  ].forEach(name => {
    Module['Features'][name] = Module['_BinaryenFeature' + name]();
  });

  // Operations
  Module['Operations'] = {};
  [ 'ClzInt32',
    'CtzInt32',
    'PopcntInt32',
    'NegFloat32',
    'AbsFloat32',
    'CeilFloat32',
    'FloorFloat32',
    'TruncFloat32',
    'NearestFloat32',
    'SqrtFloat32',
    'EqZInt32',
    'ClzInt64',
    'CtzInt64',
    'PopcntInt64',
    'NegFloat64',
    'AbsFloat64',
    'CeilFloat64',
    'FloorFloat64',
    'TruncFloat64',
    'NearestFloat64',
    'SqrtFloat64',
    'EqZInt64',
    'ExtendSInt32',
    'ExtendUInt32',
    'WrapInt64',
    'TruncSFloat32ToInt32',
    'TruncSFloat32ToInt64',
    'TruncUFloat32ToInt32',
    'TruncUFloat32ToInt64',
    'TruncSFloat64ToInt32',
    'TruncSFloat64ToInt64',
    'TruncUFloat64ToInt32',
    'TruncUFloat64ToInt64',
    'TruncSatSFloat32ToInt32',
    'TruncSatSFloat32ToInt64',
    'TruncSatUFloat32ToInt32',
    'TruncSatUFloat32ToInt64',
    'TruncSatSFloat64ToInt32',
    'TruncSatSFloat64ToInt64',
    'TruncSatUFloat64ToInt32',
    'TruncSatUFloat64ToInt64',
    'ReinterpretFloat32',
    'ReinterpretFloat64',
    'ConvertSInt32ToFloat32',
    'ConvertSInt32ToFloat64',
    'ConvertUInt32ToFloat32',
    'ConvertUInt32ToFloat64',
    'ConvertSInt64ToFloat32',
    'ConvertSInt64ToFloat64',
    'ConvertUInt64ToFloat32',
    'ConvertUInt64ToFloat64',
    'PromoteFloat32',
    'DemoteFloat64',
    'ReinterpretInt32',
    'ReinterpretInt64',
    'ExtendS8Int32',
    'ExtendS16Int32',
    'ExtendS8Int64',
    'ExtendS16Int64',
    'ExtendS32Int64',
    'AddInt32',
    'SubInt32',
    'MulInt32',
    'DivSInt32',
    'DivUInt32',
    'RemSInt32',
    'RemUInt32',
    'AndInt32',
    'OrInt32',
    'XorInt32',
    'ShlInt32',
    'ShrUInt32',
    'ShrSInt32',
    'RotLInt32',
    'RotRInt32',
    'EqInt32',
    'NeInt32',
    'LtSInt32',
    'LtUInt32',
    'LeSInt32',
    'LeUInt32',
    'GtSInt32',
    'GtUInt32',
    'GeSInt32',
    'GeUInt32',
    'AddInt64',
    'SubInt64',
    'MulInt64',
    'DivSInt64',
    'DivUInt64',
    'RemSInt64',
    'RemUInt64',
    'AndInt64',
    'OrInt64',
    'XorInt64',
    'ShlInt64',
    'ShrUInt64',
    'ShrSInt64',
    'RotLInt64',
    'RotRInt64',
    'EqInt64',
    'NeInt64',
    'LtSInt64',
    'LtUInt64',
    'LeSInt64',
    'LeUInt64',
    'GtSInt64',
    'GtUInt64',
    'GeSInt64',
    'GeUInt64',
    'AddFloat32',
    'SubFloat32',
    'MulFloat32',
    'DivFloat32',
    'CopySignFloat32',
    'MinFloat32',
    'MaxFloat32',
    'EqFloat32',
    'NeFloat32',
    'LtFloat32',
    'LeFloat32',
    'GtFloat32',
    'GeFloat32',
    'AddFloat64',
    'SubFloat64',
    'MulFloat64',
    'DivFloat64',
    'CopySignFloat64',
    'MinFloat64',
    'MaxFloat64',
    'EqFloat64',
    'NeFloat64',
    'LtFloat64',
    'LeFloat64',
    'GtFloat64',
    'GeFloat64',
    'AtomicRMWAdd',
    'AtomicRMWSub',
    'AtomicRMWAnd',
    'AtomicRMWOr',
    'AtomicRMWXor',
    'AtomicRMWXchg',
    'SplatVecI8x16',
    'ExtractLaneSVecI8x16',
    'ExtractLaneUVecI8x16',
    'ReplaceLaneVecI8x16',
    'SplatVecI16x8',
    'ExtractLaneSVecI16x8',
    'ExtractLaneUVecI16x8',
    'ReplaceLaneVecI16x8',
    'SplatVecI32x4',
    'ExtractLaneVecI32x4',
    'ReplaceLaneVecI32x4',
    'SplatVecI64x2',
    'ExtractLaneVecI64x2',
    'ReplaceLaneVecI64x2',
    'SplatVecF32x4',
    'ExtractLaneVecF32x4',
    'ReplaceLaneVecF32x4',
    'SplatVecF64x2',
    'ExtractLaneVecF64x2',
    'ReplaceLaneVecF64x2',
    'EqVecI8x16',
    'NeVecI8x16',
    'LtSVecI8x16',
    'LtUVecI8x16',
    'GtSVecI8x16',
    'GtUVecI8x16',
    'LeSVecI8x16',
    'LeUVecI8x16',
    'GeSVecI8x16',
    'GeUVecI8x16',
    'EqVecI16x8',
    'NeVecI16x8',
    'LtSVecI16x8',
    'LtUVecI16x8',
    'GtSVecI16x8',
    'GtUVecI16x8',
    'LeSVecI16x8',
    'LeUVecI16x8',
    'GeSVecI16x8',
    'GeUVecI16x8',
    'EqVecI32x4',
    'NeVecI32x4',
    'LtSVecI32x4',
    'LtUVecI32x4',
    'GtSVecI32x4',
    'GtUVecI32x4',
    'LeSVecI32x4',
    'LeUVecI32x4',
    'GeSVecI32x4',
    'GeUVecI32x4',
    'EqVecI64x2',
    'NeVecI64x2',
    'LtSVecI64x2',
    'GtSVecI64x2',
    'LeSVecI64x2',
    'GeSVecI64x2',
    'EqVecF32x4',
    'NeVecF32x4',
    'LtVecF32x4',
    'GtVecF32x4',
    'LeVecF32x4',
    'GeVecF32x4',
    'EqVecF64x2',
    'NeVecF64x2',
    'LtVecF64x2',
    'GtVecF64x2',
    'LeVecF64x2',
    'GeVecF64x2',
    'NotVec128',
    'AndVec128',
    'OrVec128',
    'XorVec128',
    'AndNotVec128',
    'BitselectVec128',
    'RelaxedFmaVecF32x4',
    'RelaxedFmsVecF32x4',
    'RelaxedFmaVecF64x2',
    'RelaxedFmsVecF64x2',
    'LaneselectI8x16',
    'LaneselectI16x8',
    'LaneselectI32x4',
    'LaneselectI64x2',
    'DotI8x16I7x16AddSToVecI32x4',
    'AnyTrueVec128',
    'PopcntVecI8x16',
    'AbsVecI8x16',
    'NegVecI8x16',
    'AllTrueVecI8x16',
    'BitmaskVecI8x16',
    'ShlVecI8x16',
    'ShrSVecI8x16',
    'ShrUVecI8x16',
    'AddVecI8x16',
    'AddSatSVecI8x16',
    'AddSatUVecI8x16',
    'SubVecI8x16',
    'SubSatSVecI8x16',
    'SubSatUVecI8x16',
    'MinSVecI8x16',
    'MinUVecI8x16',
    'MaxSVecI8x16',
    'MaxUVecI8x16',
    'AvgrUVecI8x16',
    'AbsVecI16x8',
    'NegVecI16x8',
    'AllTrueVecI16x8',
    'BitmaskVecI16x8',
    'ShlVecI16x8',
    'ShrSVecI16x8',
    'ShrUVecI16x8',
    'AddVecI16x8',
    'AddSatSVecI16x8',
    'AddSatUVecI16x8',
    'SubVecI16x8',
    'SubSatSVecI16x8',
    'SubSatUVecI16x8',
    'MulVecI16x8',
    'MinSVecI16x8',
    'MinUVecI16x8',
    'MaxSVecI16x8',
    'MaxUVecI16x8',
    'AvgrUVecI16x8',
    'Q15MulrSatSVecI16x8',
    'ExtMulLowSVecI16x8',
    'ExtMulHighSVecI16x8',
    'ExtMulLowUVecI16x8',
    'ExtMulHighUVecI16x8',
    'DotSVecI16x8ToVecI32x4',
    'ExtMulLowSVecI32x4',
    'ExtMulHighSVecI32x4',
    'ExtMulLowUVecI32x4',
    'ExtMulHighUVecI32x4',
    'AbsVecI32x4',
    'NegVecI32x4',
    'AllTrueVecI32x4',
    'BitmaskVecI32x4',
    'ShlVecI32x4',
    'ShrSVecI32x4',
    'ShrUVecI32x4',
    'AddVecI32x4',
    'SubVecI32x4',
    'MulVecI32x4',
    'MinSVecI32x4',
    'MinUVecI32x4',
    'MaxSVecI32x4',
    'MaxUVecI32x4',
    'AbsVecI64x2',
    'NegVecI64x2',
    'AllTrueVecI64x2',
    'BitmaskVecI64x2',
    'ShlVecI64x2',
    'ShrSVecI64x2',
    'ShrUVecI64x2',
    'AddVecI64x2',
    'SubVecI64x2',
    'MulVecI64x2',
    'ExtMulLowSVecI64x2',
    'ExtMulHighSVecI64x2',
    'ExtMulLowUVecI64x2',
    'ExtMulHighUVecI64x2',
    'AbsVecF32x4',
    'NegVecF32x4',
    'SqrtVecF32x4',
    'AddVecF32x4',
    'SubVecF32x4',
    'MulVecF32x4',
    'DivVecF32x4',
    'MinVecF32x4',
    'MaxVecF32x4',
    'PMinVecF32x4',
    'PMaxVecF32x4',
    'CeilVecF32x4',
    'FloorVecF32x4',
    'TruncVecF32x4',
    'NearestVecF32x4',
    'AbsVecF64x2',
    'NegVecF64x2',
    'SqrtVecF64x2',
    'AddVecF64x2',
    'SubVecF64x2',
    'MulVecF64x2',
    'DivVecF64x2',
    'MinVecF64x2',
    'MaxVecF64x2',
    'PMinVecF64x2',
    'PMaxVecF64x2',
    'CeilVecF64x2',
    'FloorVecF64x2',
    'TruncVecF64x2',
    'NearestVecF64x2',
    'ExtAddPairwiseSVecI8x16ToI16x8',
    'ExtAddPairwiseUVecI8x16ToI16x8',
    'ExtAddPairwiseSVecI16x8ToI32x4',
    'ExtAddPairwiseUVecI16x8ToI32x4',
    'TruncSatSVecF32x4ToVecI32x4',
    'TruncSatUVecF32x4ToVecI32x4',
    'ConvertSVecI32x4ToVecF32x4',
    'ConvertUVecI32x4ToVecF32x4',
    'Load8SplatVec128',
    'Load16SplatVec128',
    'Load32SplatVec128',
    'Load64SplatVec128',
    'Load8x8SVec128',
    'Load8x8UVec128',
    'Load16x4SVec128',
    'Load16x4UVec128',
    'Load32x2SVec128',
    'Load32x2UVec128',
    'Load32ZeroVec128',
    'Load64ZeroVec128',
    'Load8LaneVec128',
    'Load16LaneVec128',
    'Load32LaneVec128',
    'Load64LaneVec128',
    'Store8LaneVec128',
    'Store16LaneVec128',
    'Store32LaneVec128',
    'Store64LaneVec128',
    'NarrowSVecI16x8ToVecI8x16',
    'NarrowUVecI16x8ToVecI8x16',
    'NarrowSVecI32x4ToVecI16x8',
    'NarrowUVecI32x4ToVecI16x8',
    'ExtendLowSVecI8x16ToVecI16x8',
    'ExtendHighSVecI8x16ToVecI16x8',
    'ExtendLowUVecI8x16ToVecI16x8',
    'ExtendHighUVecI8x16ToVecI16x8',
    'ExtendLowSVecI16x8ToVecI32x4',
    'ExtendHighSVecI16x8ToVecI32x4',
    'ExtendLowUVecI16x8ToVecI32x4',
    'ExtendHighUVecI16x8ToVecI32x4',
    'ExtendLowSVecI32x4ToVecI64x2',
    'ExtendHighSVecI32x4ToVecI64x2',
    'ExtendLowUVecI32x4ToVecI64x2',
    'ExtendHighUVecI32x4ToVecI64x2',
    'ConvertLowSVecI32x4ToVecF64x2',
    'ConvertLowUVecI32x4ToVecF64x2',
    'TruncSatZeroSVecF64x2ToVecI32x4',
    'TruncSatZeroUVecF64x2ToVecI32x4',
    'DemoteZeroVecF64x2ToVecF32x4',
    'PromoteLowVecF32x4ToVecF64x2',
    'RelaxedTruncSVecF32x4ToVecI32x4',
    'RelaxedTruncUVecF32x4ToVecI32x4',
    'RelaxedTruncZeroSVecF64x2ToVecI32x4',
    'RelaxedTruncZeroUVecF64x2ToVecI32x4',
    'SwizzleVecI8x16',
    'RelaxedSwizzleVecI8x16',
    'RelaxedMinVecF32x4',
    'RelaxedMaxVecF32x4',
    'RelaxedMinVecF64x2',
    'RelaxedMaxVecF64x2',
    'RelaxedQ15MulrSVecI16x8',
    'DotI8x16I7x16SToVecI16x8',
    'RefAsNonNull',
    'RefAsExternInternalize',
    'RefAsExternExternalize',
    'BrOnNull',
    'BrOnNonNull',
    'BrOnCast',
    'BrOnCastFail',
    'StringNewUTF8',
    'StringNewWTF8',
    'StringNewReplace',
    'StringNewWTF16',
    'StringNewUTF8Array',
    'StringNewWTF8Array',
    'StringNewReplaceArray',
    'StringNewWTF16Array',
    'StringNewFromCodePoint',
    'StringMeasureUTF8',
    'StringMeasureWTF8',
    'StringMeasureWTF16',
    'StringMeasureIsUSV',
    'StringMeasureWTF16View',
    'StringEncodeUTF8',
    'StringEncodeWTF8',
    'StringEncodeWTF16',
    'StringEncodeUTF8Array',
    'StringEncodeWTF8Array',
    'StringEncodeWTF16Array',
    'StringAsWTF8',
    'StringAsWTF16',
    'StringAsIter',
    'StringIterMoveAdvance',
    'StringIterMoveRewind',
    'StringSliceWTF8',
    'StringSliceWTF16',
    'StringEqEqual',
    'StringEqCompare'
  ].forEach(name => {
    Module['Operations'][name] = Module[name] = Module['_Binaryen' + name]();
  });

  // Expression side effects
  Module['SideEffects'] = {};
  [ 'None',
    'Branches',
    'Calls',
    'ReadsLocal',
    'WritesLocal',
    'ReadsGlobal',
    'WritesGlobal',
    'ReadsMemory',
    'WritesMemory',
    'ReadsTable',
    'WritesTable',
    'ImplicitTrap',
    'IsAtomic',
    'Throws',
    'DanglingPop',
    'TrapsNeverHappen',
    'Any'
  ].forEach(name => {
    Module['SideEffects'][name] = Module['_BinaryenSideEffect' + name]();
  });

  // ExpressionRunner flags
  Module['ExpressionRunner']['Flags'] = {
    'Default': Module['_ExpressionRunnerFlagsDefault'](),
    'PreserveSideeffects': Module['_ExpressionRunnerFlagsPreserveSideeffects'](),
    'TraverseCalls': Module['_ExpressionRunnerFlagsTraverseCalls']()
  };
}

// 'Module' interface
Module['Module'] = function(module) {
  assert(!module); // guard against incorrect old API usage
  wrapModule(Module['_BinaryenModuleCreate'](), this);
};

// Receives a C pointer to a C Module and a JS object, and creates
// the JS wrappings on the object to access the C data.
// This is meant for internal use only, and is necessary as we
// want to access Module from JS that were perhaps not created
// from JS.
function wrapModule(module, self = {}) {
  assert(module); // guard against incorrect old API usage

  self['ptr'] = module;

  // The size of a single literal in memory as used in Const creation,
  // which is a little different: we don't want users to need to make
  // their own Literals, as the C API handles them by value, which means
  // we would leak them. Instead, Const creation is fused together with
  // an intermediate stack allocation of this size to pass the value.
  const sizeOfLiteral = _BinaryenSizeofLiteral();

  // 'Expression' creation
  self['block'] = function(name, children, type) {
    return preserveStack(() =>
      Module['_BinaryenBlock'](module, name ? strToStack(name) : 0,
                               i32sToStack(children), children.length,
                               typeof type !== 'undefined' ? type : Module['none'])
    );
  };
  self['if'] = function(condition, ifTrue, ifFalse) {
    return Module['_BinaryenIf'](module, condition, ifTrue, ifFalse);
  };
  self['loop'] = function(label, body) {
    return preserveStack(() => Module['_BinaryenLoop'](module, strToStack(label), body));
  };
  self['break'] = self['br'] = function(label, condition, value) {
    return preserveStack(() => Module['_BinaryenBreak'](module, strToStack(label), condition, value));
  };
  self['br_if'] = function(label, condition, value) {
    return self['br'](label, condition, value);
  };
  self['switch'] = function(names, defaultName, condition, value) {
    return preserveStack(() =>
      Module['_BinaryenSwitch'](module, i32sToStack(names.map(strToStack)), names.length, strToStack(defaultName), condition, value)
    );
  };
  self['call'] = function(name, operands, type) {
    return preserveStack(() => Module['_BinaryenCall'](module, strToStack(name), i32sToStack(operands), operands.length, type));
  };
  // 'callIndirect', 'returnCall', 'returnCallIndirect' are deprecated and may
  // be removed in a future release. Please use the the snake_case names
  // instead.
  self['callIndirect'] = self['call_indirect'] = function(table, target, operands, params, results) {
    return preserveStack(() =>
      Module['_BinaryenCallIndirect'](module, strToStack(table), target, i32sToStack(operands), operands.length, params, results)
    );
  };
  self['returnCall'] = self['return_call'] = function(name, operands, type) {
    return preserveStack(() =>
      Module['_BinaryenReturnCall'](module, strToStack(name), i32sToStack(operands), operands.length, type)
    );
  };
  self['returnCallIndirect'] = self['return_call_indirect'] = function(table, target, operands, params, results) {
    return preserveStack(() =>
      Module['_BinaryenReturnCallIndirect'](module, strToStack(table), target, i32sToStack(operands), operands.length, params, results)
    );
  };

  self['local'] = {
    'get'(index, type) {
      return Module['_BinaryenLocalGet'](module, index, type);
    },
    'set'(index, value) {
      return Module['_BinaryenLocalSet'](module, index, value);
    },
    'tee'(index, value, type) {
      if (typeof type === 'undefined') {
        throw new Error("local.tee's type should be defined");
      }
      return Module['_BinaryenLocalTee'](module, index, value, type);
    }
  }

  self['global'] = {
    'get'(name, type) {
      return Module['_BinaryenGlobalGet'](module, strToStack(name), type);
    },
    'set'(name, value) {
      return Module['_BinaryenGlobalSet'](module, strToStack(name), value);
    }
  }

  self['table'] = {
    'get'(name, index, type) {
      return Module['_BinaryenTableGet'](module, strToStack(name), index, type);
    },
    'set'(name, index, value) {
      return Module['_BinaryenTableSet'](module, strToStack(name), index, value);
    },
    'size'(name) {
      return Module['_BinaryenTableSize'](module, strToStack(name));
    },
    'grow'(name, value, delta) {
      return Module['_BinaryenTableGrow'](module, strToStack(name), value, delta);
    }
  }

  self['memory'] = {
    // memory64 defaults to undefined/false.
    'size'(name, memory64) {
      return Module['_BinaryenMemorySize'](module, strToStack(name), memory64);
    },
    'grow'(value, name, memory64) {
      return Module['_BinaryenMemoryGrow'](module, value, strToStack(name), memory64);
    },
    'init'(segment, dest, offset, size, name) {
      return Module['_BinaryenMemoryInit'](module, segment, dest, offset, size, strToStack(name));
    },
    'copy'(dest, source, size, destMemory, sourceMemory) {
      return Module['_BinaryenMemoryCopy'](module, dest, source, size, strToStack(destMemory), strToStack(sourceMemory));
    },
    'fill'(dest, value, size, name) {
      return Module['_BinaryenMemoryFill'](module, dest, value, size, strToStack(name));
    },
    'atomic': {
      'notify'(ptr, notifyCount, name) {
        return Module['_BinaryenAtomicNotify'](module, ptr, notifyCount, strToStack(name));
      },
      'wait32'(ptr, expected, timeout, name) {
        return Module['_BinaryenAtomicWait'](module, ptr, expected, timeout, Module['i32'], strToStack(name));
      },
      'wait64'(ptr, expected, timeout, name) {
        return Module['_BinaryenAtomicWait'](module, ptr, expected, timeout, Module['i64'], strToStack(name));
      }
    }
  }

  self['data'] = {
    'drop'(segment) {
      return Module['_BinaryenDataDrop'](module, segment);
    }
  }

  self['i32'] = {
    'load'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 4, true, offset, align, Module['i32'], ptr, strToStack(name));
    },
    'load8_s'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 1, true, offset, align, Module['i32'], ptr, strToStack(name));
    },
    'load8_u'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 1, false, offset, align, Module['i32'], ptr, strToStack(name));
    },
    'load16_s'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 2, true, offset, align, Module['i32'], ptr, strToStack(name));
    },
    'load16_u'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 2, false, offset, align, Module['i32'], ptr, strToStack(name));
    },
    'store'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 4, offset, align, ptr, value, Module['i32'], strToStack(name));
    },
    'store8'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 1, offset, align, ptr, value, Module['i32'], strToStack(name));
    },
    'store16'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 2, offset, align, ptr, value, Module['i32'], strToStack(name));
    },
    'const'(x) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralInt32'](tempLiteral, x);
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'clz'(value) {
      return Module['_BinaryenUnary'](module, Module['ClzInt32'], value);
    },
    'ctz'(value) {
      return Module['_BinaryenUnary'](module, Module['CtzInt32'], value);
    },
    'popcnt'(value) {
      return Module['_BinaryenUnary'](module, Module['PopcntInt32'], value);
    },
    'eqz'(value) {
      return Module['_BinaryenUnary'](module, Module['EqZInt32'], value);
    },
    'trunc_s': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSFloat32ToInt32'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSFloat64ToInt32'], value);
      },
    },
    'trunc_u': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncUFloat32ToInt32'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncUFloat64ToInt32'], value);
      },
    },
    'trunc_s_sat': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatSFloat32ToInt32'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatSFloat64ToInt32'], value);
      },
    },
    'trunc_u_sat': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatUFloat32ToInt32'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatUFloat64ToInt32'], value);
      },
    },
    'reinterpret'(value) {
      return Module['_BinaryenUnary'](module, Module['ReinterpretFloat32'], value);
    },
    'extend8_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendS8Int32'], value);
    },
    'extend16_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendS16Int32'], value);
    },
    'wrap'(value) {
      return Module['_BinaryenUnary'](module, Module['WrapInt64'], value);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddInt32'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubInt32'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulInt32'], left, right);
    },
    'div_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivSInt32'], left, right);
    },
    'div_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivUInt32'], left, right);
    },
    'rem_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RemSInt32'], left, right);
    },
    'rem_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RemUInt32'], left, right);
    },
    'and'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AndInt32'], left, right);
    },
    'or'(left, right) {
      return Module['_BinaryenBinary'](module, Module['OrInt32'], left, right);
    },
    'xor'(left, right) {
      return Module['_BinaryenBinary'](module, Module['XorInt32'], left, right);
    },
    'shl'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ShlInt32'], left, right);
    },
    'shr_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ShrUInt32'], left, right);
    },
    'shr_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ShrSInt32'], left, right);
    },
    'rotl'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RotLInt32'], left, right);
    },
    'rotr'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RotRInt32'], left, right);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqInt32'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeInt32'], left, right);
    },
    'lt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtSInt32'], left, right);
    },
    'lt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtUInt32'], left, right);
    },
    'le_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeSInt32'], left, right);
    },
    'le_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeUInt32'], left, right);
    },
    'gt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtSInt32'], left, right);
    },
    'gt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtUInt32'], left, right);
    },
    'ge_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeSInt32'], left, right);
    },
    'ge_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeUInt32'], left, right);
    },
    'atomic': {
      'load'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 4, offset, Module['i32'], ptr, strToStack(name));
      },
      'load8_u'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 1, offset, Module['i32'], ptr, strToStack(name));
      },
      'load16_u'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 2, offset, Module['i32'], ptr, strToStack(name));
      },
      'store'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 4, offset, ptr, value, Module['i32'], strToStack(name));
      },
      'store8'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 1, offset, ptr, value, Module['i32'], strToStack(name));
      },
      'store16'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 2, offset, ptr, value, Module['i32'], strToStack(name));
      },
      'rmw': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 4, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 4, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 4, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 4, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 4, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 4, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 4, offset, ptr, expected, replacement, Module['i32'], strToStack(name))
        },
      },
      'rmw8_u': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 1, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 1, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 1, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 1, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 1, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 1, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 1, offset, ptr, expected, replacement, Module['i32'], strToStack(name))
        },
      },
      'rmw16_u': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 2, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 2, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 2, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 2, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 2, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 2, offset, ptr, value, Module['i32'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 2, offset, ptr, expected, replacement, Module['i32'], strToStack(name))
        },
      },
    },
    'pop'() {
      return Module['_BinaryenPop'](module, Module['i32']);
    }
  };

  self['i64'] = {
    'load'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 8, true, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'load8_s'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 1, true, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'load8_u'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 1, false, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'load16_s'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 2, true, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'load16_u'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 2, false, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'load32_s'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 4, true, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'load32_u'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 4, false, offset, align, Module['i64'], ptr, strToStack(name));
    },
    'store'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 8, offset, align, ptr, value, Module['i64'], strToStack(name));
    },
    'store8'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 1, offset, align, ptr, value, Module['i64'], strToStack(name));
    },
    'store16'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 2, offset, align, ptr, value, Module['i64'], strToStack(name));
    },
    'store32'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 4, offset, align, ptr, value, Module['i64'], strToStack(name));
    },
    'const'(x, y) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralInt64'](tempLiteral, x, y);
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'clz'(value) {
      return Module['_BinaryenUnary'](module, Module['ClzInt64'], value);
    },
    'ctz'(value) {
      return Module['_BinaryenUnary'](module, Module['CtzInt64'], value);
    },
    'popcnt'(value) {
      return Module['_BinaryenUnary'](module, Module['PopcntInt64'], value);
    },
    'eqz'(value) {
      return Module['_BinaryenUnary'](module, Module['EqZInt64'], value);
    },
    'trunc_s': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSFloat32ToInt64'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSFloat64ToInt64'], value);
      },
    },
    'trunc_u': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncUFloat32ToInt64'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncUFloat64ToInt64'], value);
      },
    },
    'trunc_s_sat': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatSFloat32ToInt64'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatSFloat64ToInt64'], value);
      },
    },
    'trunc_u_sat': {
      'f32'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatUFloat32ToInt64'], value);
      },
      'f64'(value) {
        return Module['_BinaryenUnary'](module, Module['TruncSatUFloat64ToInt64'], value);
      },
    },
    'reinterpret'(value) {
      return Module['_BinaryenUnary'](module, Module['ReinterpretFloat64'], value);
    },
    'extend8_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendS8Int64'], value);
    },
    'extend16_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendS16Int64'], value);
    },
    'extend32_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendS32Int64'], value);
    },
    'extend_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendSInt32'], value);
    },
    'extend_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendUInt32'], value);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddInt64'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubInt64'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulInt64'], left, right);
    },
    'div_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivSInt64'], left, right);
    },
    'div_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivUInt64'], left, right);
    },
    'rem_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RemSInt64'], left, right);
    },
    'rem_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RemUInt64'], left, right);
    },
    'and'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AndInt64'], left, right);
    },
    'or'(left, right) {
      return Module['_BinaryenBinary'](module, Module['OrInt64'], left, right);
    },
    'xor'(left, right) {
      return Module['_BinaryenBinary'](module, Module['XorInt64'], left, right);
    },
    'shl'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ShlInt64'], left, right);
    },
    'shr_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ShrUInt64'], left, right);
    },
    'shr_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ShrSInt64'], left, right);
    },
    'rotl'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RotLInt64'], left, right);
    },
    'rotr'(left, right) {
      return Module['_BinaryenBinary'](module, Module['RotRInt64'], left, right);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqInt64'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeInt64'], left, right);
    },
    'lt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtSInt64'], left, right);
    },
    'lt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtUInt64'], left, right);
    },
    'le_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeSInt64'], left, right);
    },
    'le_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeUInt64'], left, right);
    },
    'gt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtSInt64'], left, right);
    },
    'gt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtUInt64'], left, right);
    },
    'ge_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeSInt64'], left, right);
    },
    'ge_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeUInt64'], left, right);
    },
    'atomic': {
      'load'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 8, offset, Module['i64'], ptr, strToStack(name));
      },
      'load8_u'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 1, offset, Module['i64'], ptr, strToStack(name));
      },
      'load16_u'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 2, offset, Module['i64'], ptr, strToStack(name));
      },
      'load32_u'(offset, ptr, name) {
        return Module['_BinaryenAtomicLoad'](module, 4, offset, Module['i64'], ptr, strToStack(name));
      },
      'store'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 8, offset, ptr, value, Module['i64'], strToStack(name));
      },
      'store8'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 1, offset, ptr, value, Module['i64'], strToStack(name));
      },
      'store16'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 2, offset, ptr, value, Module['i64'], strToStack(name));
      },
      'store32'(offset, ptr, value, name) {
        return Module['_BinaryenAtomicStore'](module, 4, offset, ptr, value, Module['i64'], strToStack(name));
      },
      'rmw': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 8, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 8, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 8, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 8, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 8, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 8, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 8, offset, ptr, expected, replacement, Module['i64'], strToStack(name))
        },
      },
      'rmw8_u': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 1, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 1, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 1, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 1, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 1, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 1, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 1, offset, ptr, expected, replacement, Module['i64'], strToStack(name))
        },
      },
      'rmw16_u': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 2, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 2, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 2, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 2, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 2, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 2, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 2, offset, ptr, expected, replacement, Module['i64'], strToStack(name))
        },
      },
      'rmw32_u': {
        'add'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAdd'], 4, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'sub'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWSub'], 4, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'and'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWAnd'], 4, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'or'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWOr'], 4, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xor'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXor'], 4, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'xchg'(offset, ptr, value, name) {
          return Module['_BinaryenAtomicRMW'](module, Module['AtomicRMWXchg'], 4, offset, ptr, value, Module['i64'], strToStack(name));
        },
        'cmpxchg'(offset, ptr, expected, replacement, name) {
          return Module['_BinaryenAtomicCmpxchg'](module, 4, offset, ptr, expected, replacement, Module['i64'], strToStack(name))
        },
      },
    },
    'pop'() {
      return Module['_BinaryenPop'](module, Module['i64']);
    }
  };

  self['f32'] = {
    'load'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 4, true, offset, align, Module['f32'], ptr, strToStack(name));
    },
    'store'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 4, offset, align, ptr, value, Module['f32'], strToStack(name));
    },
    'const'(x) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralFloat32'](tempLiteral, x);
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'const_bits'(x) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralFloat32Bits'](tempLiteral, x);
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegFloat32'], value);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsFloat32'], value);
    },
    'ceil'(value) {
      return Module['_BinaryenUnary'](module, Module['CeilFloat32'], value);
    },
    'floor'(value) {
      return Module['_BinaryenUnary'](module, Module['FloorFloat32'], value);
    },
    'trunc'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncFloat32'], value);
    },
    'nearest'(value) {
      return Module['_BinaryenUnary'](module, Module['NearestFloat32'], value);
    },
    'sqrt'(value) {
      return Module['_BinaryenUnary'](module, Module['SqrtFloat32'], value);
    },
    'reinterpret'(value) {
      return Module['_BinaryenUnary'](module, Module['ReinterpretInt32'], value);
    },
    'convert_s': {
      'i32'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertSInt32ToFloat32'], value);
      },
      'i64'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertSInt64ToFloat32'], value);
      },
    },
    'convert_u': {
      'i32'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertUInt32ToFloat32'], value);
      },
      'i64'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertUInt64ToFloat32'], value);
      },
    },
    'demote'(value) {
      return Module['_BinaryenUnary'](module, Module['DemoteFloat64'], value);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddFloat32'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubFloat32'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulFloat32'], left, right);
    },
    'div'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivFloat32'], left, right);
    },
    'copysign'(left, right) {
      return Module['_BinaryenBinary'](module, Module['CopySignFloat32'], left, right);
    },
    'min'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinFloat32'], left, right);
    },
    'max'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxFloat32'], left, right);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqFloat32'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeFloat32'], left, right);
    },
    'lt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtFloat32'], left, right);
    },
    'le'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeFloat32'], left, right);
    },
    'gt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtFloat32'], left, right);
    },
    'ge'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeFloat32'], left, right);
    },
    'pop'() {
      return Module['_BinaryenPop'](module, Module['f32']);
    }
  };

  self['f64'] = {
    'load'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 8, true, offset, align, Module['f64'], ptr, strToStack(name));
    },
    'store'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 8, offset, align, ptr, value, Module['f64'], strToStack(name));
    },
    'const'(x) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralFloat64'](tempLiteral, x);
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'const_bits'(x, y) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralFloat64Bits'](tempLiteral, x, y);
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegFloat64'], value);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsFloat64'], value);
    },
    'ceil'(value) {
      return Module['_BinaryenUnary'](module, Module['CeilFloat64'], value);
    },
    'floor'(value) {
      return Module['_BinaryenUnary'](module, Module['FloorFloat64'], value);
    },
    'trunc'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncFloat64'], value);
    },
    'nearest'(value) {
      return Module['_BinaryenUnary'](module, Module['NearestFloat64'], value);
    },
    'sqrt'(value) {
      return Module['_BinaryenUnary'](module, Module['SqrtFloat64'], value);
    },
    'reinterpret'(value) {
      return Module['_BinaryenUnary'](module, Module['ReinterpretInt64'], value);
    },
    'convert_s': {
      'i32'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertSInt32ToFloat64'], value);
      },
      'i64'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertSInt64ToFloat64'], value);
      },
    },
    'convert_u': {
      'i32'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertUInt32ToFloat64'], value);
      },
      'i64'(value) {
        return Module['_BinaryenUnary'](module, Module['ConvertUInt64ToFloat64'], value);
      },
    },
    'promote'(value) {
      return Module['_BinaryenUnary'](module, Module['PromoteFloat32'], value);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddFloat64'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubFloat64'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulFloat64'], left, right);
    },
    'div'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivFloat64'], left, right);
    },
    'copysign'(left, right) {
      return Module['_BinaryenBinary'](module, Module['CopySignFloat64'], left, right);
    },
    'min'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinFloat64'], left, right);
    },
    'max'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxFloat64'], left, right);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqFloat64'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeFloat64'], left, right);
    },
    'lt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtFloat64'], left, right);
    },
    'le'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeFloat64'], left, right);
    },
    'gt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtFloat64'], left, right);
    },
    'ge'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeFloat64'], left, right);
    },
    'pop'() {
      return Module['_BinaryenPop'](module, Module['f64']);
    }
  };

  self['v128'] = {
    'load'(offset, align, ptr, name) {
      return Module['_BinaryenLoad'](module, 16, false, offset, align, Module['v128'], ptr, strToStack(name));
    },
    'load8_splat'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load8SplatVec128'], offset, align, ptr, strToStack(name));
    },
    'load16_splat'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load16SplatVec128'], offset, align, ptr, strToStack(name));
    },
    'load32_splat'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load32SplatVec128'], offset, align, ptr, strToStack(name));
    },
    'load64_splat'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load64SplatVec128'], offset, align, ptr, strToStack(name));
    },
    'load8x8_s'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load8x8SVec128'], offset, align, ptr, strToStack(name));
    },
    'load8x8_u'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load8x8UVec128'], offset, align, ptr, strToStack(name));
    },
    'load16x4_s'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load16x4SVec128'], offset, align, ptr, strToStack(name));
    },
    'load16x4_u'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load16x4UVec128'], offset, align, ptr, strToStack(name));
    },
    'load32x2_s'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load32x2SVec128'], offset, align, ptr, strToStack(name));
    },
    'load32x2_u'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load32x2UVec128'], offset, align, ptr, strToStack(name));
    },
    'load32_zero'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load32ZeroVec128'], offset, align, ptr, strToStack(name));
    },
    'load64_zero'(offset, align, ptr, name) {
      return Module['_BinaryenSIMDLoad'](module, Module['Load64ZeroVec128'], offset, align, ptr, strToStack(name));
    },
    'load8_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Load8LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'load16_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Load16LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'load32_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Load32LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'load64_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Load64LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'store8_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Store8LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'store16_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Store16LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'store32_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Store32LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'store64_lane'(offset, align, index, ptr, vec, name) {
      return Module['_BinaryenSIMDLoadStoreLane'](module, Module['Store64LaneVec128'], offset, align, index, ptr, vec, strToStack(name));
    },
    'store'(offset, align, ptr, value, name) {
      return Module['_BinaryenStore'](module, 16, offset, align, ptr, value, Module['v128'], strToStack(name));
    },
    'const'(i8s) {
      return preserveStack(() => {
        const tempLiteral = stackAlloc(sizeOfLiteral);
        Module['_BinaryenLiteralVec128'](tempLiteral, i8sToStack(i8s));
        return Module['_BinaryenConst'](module, tempLiteral);
      });
    },
    'not'(value) {
      return Module['_BinaryenUnary'](module, Module['NotVec128'], value);
    },
    'any_true'(value) {
      return Module['_BinaryenUnary'](module, Module['AnyTrueVec128'], value);
    },
    'and'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AndVec128'], left, right);
    },
    'or'(left, right) {
      return Module['_BinaryenBinary'](module, Module['OrVec128'], left, right);
    },
    'xor'(left, right) {
      return Module['_BinaryenBinary'](module, Module['XorVec128'], left, right);
    },
    'andnot'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AndNotVec128'], left, right);
    },
    'bitselect'(left, right, cond) {
      return Module['_BinaryenSIMDTernary'](module, Module['BitselectVec128'], left, right, cond);
    },
    'pop'() {
      return Module['_BinaryenPop'](module, Module['v128']);
    }
  };

  self['i8x16'] = {
    'shuffle'(left, right, mask) {
      return preserveStack(() => Module['_BinaryenSIMDShuffle'](module, left, right, i8sToStack(mask)));
    },
    'swizzle'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SwizzleVecI8x16'], left, right);
    },
    'splat'(value) {
      return Module['_BinaryenUnary'](module, Module['SplatVecI8x16'], value);
    },
    'extract_lane_s'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneSVecI8x16'], vec, index);
    },
    'extract_lane_u'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneUVecI8x16'], vec, index);
    },
    'replace_lane'(vec, index, value) {
      return Module['_BinaryenSIMDReplace'](module, Module['ReplaceLaneVecI8x16'], vec, index, value);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqVecI8x16'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeVecI8x16'], left, right);
    },
    'lt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtSVecI8x16'], left, right);
    },
    'lt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtUVecI8x16'], left, right);
    },
    'gt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtSVecI8x16'], left, right);
    },
    'gt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtUVecI8x16'], left, right);
    },
    'le_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeSVecI8x16'], left, right);
    },
    'le_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeUVecI8x16'], left, right);
    },
    'ge_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeSVecI8x16'], left, right);
    },
    'ge_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeUVecI8x16'], left, right);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsVecI8x16'], value);
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegVecI8x16'], value);
    },
    'all_true'(value) {
      return Module['_BinaryenUnary'](module, Module['AllTrueVecI8x16'], value);
    },
    'bitmask'(value) {
      return Module['_BinaryenUnary'](module, Module['BitmaskVecI8x16'], value);
    },
    'popcnt'(value) {
      return Module['_BinaryenUnary'](module, Module['PopcntVecI8x16'], value);
    },
    'shl'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShlVecI8x16'], vec, shift);
    },
    'shr_s'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrSVecI8x16'], vec, shift);
    },
    'shr_u'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrUVecI8x16'], vec, shift);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddVecI8x16'], left, right);
    },
    'add_saturate_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddSatSVecI8x16'], left, right);
    },
    'add_saturate_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddSatUVecI8x16'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubVecI8x16'], left, right);
    },
    'sub_saturate_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubSatSVecI8x16'], left, right);
    },
    'sub_saturate_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubSatUVecI8x16'], left, right);
    },
    'min_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinSVecI8x16'], left, right);
    },
    'min_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinUVecI8x16'], left, right);
    },
    'max_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxSVecI8x16'], left, right);
    },
    'max_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxUVecI8x16'], left, right);
    },
    'avgr_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AvgrUVecI8x16'], left, right);
    },
    'narrow_i16x8_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NarrowSVecI16x8ToVecI8x16'], left, right);
    },
    'narrow_i16x8_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NarrowUVecI16x8ToVecI8x16'], left, right);
    },
  };

  self['i16x8'] = {
    'splat'(value) {
      return Module['_BinaryenUnary'](module, Module['SplatVecI16x8'], value);
    },
    'extract_lane_s'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneSVecI16x8'], vec, index);
    },
    'extract_lane_u'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneUVecI16x8'], vec, index);
    },
    'replace_lane'(vec, index, value) {
      return Module['_BinaryenSIMDReplace'](module, Module['ReplaceLaneVecI16x8'], vec, index, value);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqVecI16x8'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeVecI16x8'], left, right);
    },
    'lt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtSVecI16x8'], left, right);
    },
    'lt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtUVecI16x8'], left, right);
    },
    'gt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtSVecI16x8'], left, right);
    },
    'gt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtUVecI16x8'], left, right);
    },
    'le_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeSVecI16x8'], left, right);
    },
    'le_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeUVecI16x8'], left, right);
    },
    'ge_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeSVecI16x8'], left, right);
    },
    'ge_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeUVecI16x8'], left, right);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsVecI16x8'], value);
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegVecI16x8'], value);
    },
    'all_true'(value) {
      return Module['_BinaryenUnary'](module, Module['AllTrueVecI16x8'], value);
    },
    'bitmask'(value) {
      return Module['_BinaryenUnary'](module, Module['BitmaskVecI16x8'], value);
    },
    'shl'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShlVecI16x8'], vec, shift);
    },
    'shr_s'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrSVecI16x8'], vec, shift);
    },
    'shr_u'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrUVecI16x8'], vec, shift);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddVecI16x8'], left, right);
    },
    'add_saturate_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddSatSVecI16x8'], left, right);
    },
    'add_saturate_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddSatUVecI16x8'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubVecI16x8'], left, right);
    },
    'sub_saturate_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubSatSVecI16x8'], left, right);
    },
    'sub_saturate_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubSatUVecI16x8'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulVecI16x8'], left, right);
    },
    'min_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinSVecI16x8'], left, right);
    },
    'min_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinUVecI16x8'], left, right);
    },
    'max_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxSVecI16x8'], left, right);
    },
    'max_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxUVecI16x8'], left, right);
    },
    'avgr_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AvgrUVecI16x8'], left, right);
    },
    'q15mulr_sat_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['Q15MulrSatSVecI16x8'], left, right);
    },
    'extmul_low_i8x16_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulLowSVecI16x8'], left, right);
    },
    'extmul_high_i8x16_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulHighSVecI16x8'], left, right);
    },
    'extmul_low_i8x16_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulLowUVecI16x8'], left, right);
    },
    'extmul_high_i8x16_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulHighUVecI16x8'], left, right);
    },
    'extadd_pairwise_i8x16_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtAddPairwiseSVecI8x16ToI16x8'], value);
    },
    'extadd_pairwise_i8x16_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtAddPairwiseUVecI8x16ToI16x8'], value);
    },
    'narrow_i32x4_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NarrowSVecI32x4ToVecI16x8'], left, right);
    },
    'narrow_i32x4_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NarrowUVecI32x4ToVecI16x8'], left, right);
    },
    'extend_low_i8x16_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendLowSVecI8x16ToVecI16x8'], value);
    },
    'extend_high_i8x16_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendHighSVecI8x16ToVecI16x8'], value);
    },
    'extend_low_i8x16_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendLowUVecI8x16ToVecI16x8'], value);
    },
    'extend_high_i8x16_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendHighUVecI8x16ToVecI16x8'], value);
    },
  };

  self['i32x4'] = {
    'splat'(value) {
      return Module['_BinaryenUnary'](module, Module['SplatVecI32x4'], value);
    },
    'extract_lane'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneVecI32x4'], vec, index);
    },
    'replace_lane'(vec, index, value) {
      return Module['_BinaryenSIMDReplace'](module, Module['ReplaceLaneVecI32x4'], vec, index, value);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqVecI32x4'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeVecI32x4'], left, right);
    },
    'lt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtSVecI32x4'], left, right);
    },
    'lt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtUVecI32x4'], left, right);
    },
    'gt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtSVecI32x4'], left, right);
    },
    'gt_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtUVecI32x4'], left, right);
    },
    'le_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeSVecI32x4'], left, right);
    },
    'le_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeUVecI32x4'], left, right);
    },
    'ge_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeSVecI32x4'], left, right);
    },
    'ge_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeUVecI32x4'], left, right);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsVecI32x4'], value);
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegVecI32x4'], value);
    },
    'all_true'(value) {
      return Module['_BinaryenUnary'](module, Module['AllTrueVecI32x4'], value);
    },
    'bitmask'(value) {
      return Module['_BinaryenUnary'](module, Module['BitmaskVecI32x4'], value);
    },
    'shl'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShlVecI32x4'], vec, shift);
    },
    'shr_s'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrSVecI32x4'], vec, shift);
    },
    'shr_u'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrUVecI32x4'], vec, shift);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddVecI32x4'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubVecI32x4'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulVecI32x4'], left, right);
    },
    'min_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinSVecI32x4'], left, right);
    },
    'min_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinUVecI32x4'], left, right);
    },
    'max_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxSVecI32x4'], left, right);
    },
    'max_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxUVecI32x4'], left, right);
    },
    'dot_i16x8_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DotSVecI16x8ToVecI32x4'], left, right);
    },
    'extmul_low_i16x8_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulLowSVecI32x4'], left, right);
    },
    'extmul_high_i16x8_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulHighSVecI32x4'], left, right);
    },
    'extmul_low_i16x8_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulLowUVecI32x4'], left, right);
    },
    'extmul_high_i16x8_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulHighUVecI32x4'], left, right);
    },
    'extadd_pairwise_i16x8_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtAddPairwiseSVecI16x8ToI32x4'], value);
    },
    'extadd_pairwise_i16x8_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtAddPairwiseUVecI16x8ToI32x4'], value);
    },
    'trunc_sat_f32x4_s'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncSatSVecF32x4ToVecI32x4'], value);
    },
    'trunc_sat_f32x4_u'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncSatUVecF32x4ToVecI32x4'], value);
    },
    'extend_low_i16x8_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendLowSVecI16x8ToVecI32x4'], value);
    },
    'extend_high_i16x8_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendHighSVecI16x8ToVecI32x4'], value);
    },
    'extend_low_i16x8_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendLowUVecI16x8ToVecI32x4'], value);
    },
    'extend_high_i16x8_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendHighUVecI16x8ToVecI32x4'], value);
    },
    'trunc_sat_f64x2_s_zero'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncSatZeroSVecF64x2ToVecI32x4'], value);
    },
    'trunc_sat_f64x2_u_zero'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncSatZeroUVecF64x2ToVecI32x4'], value);
    },
  };

  self['i64x2'] = {
    'splat'(value) {
      return Module['_BinaryenUnary'](module, Module['SplatVecI64x2'], value);
    },
    'extract_lane'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneVecI64x2'], vec, index);
    },
    'replace_lane'(vec, index, value) {
      return Module['_BinaryenSIMDReplace'](module, Module['ReplaceLaneVecI64x2'], vec, index, value);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqVecI64x2'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeVecI64x2'], left, right);
    },
    'lt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtSVecI64x2'], left, right);
    },
    'gt_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtSVecI64x2'], left, right);
    },
    'le_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeSVecI64x2'], left, right);
    },
    'ge_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeSVecI64x2'], left, right);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsVecI64x2'], value);
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegVecI64x2'], value);
    },
    'all_true'(value) {
      return Module['_BinaryenUnary'](module, Module['AllTrueVecI64x2'], value);
    },
    'bitmask'(value) {
      return Module['_BinaryenUnary'](module, Module['BitmaskVecI64x2'], value);
    },
    'shl'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShlVecI64x2'], vec, shift);
    },
    'shr_s'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrSVecI64x2'], vec, shift);
    },
    'shr_u'(vec, shift) {
      return Module['_BinaryenSIMDShift'](module, Module['ShrUVecI64x2'], vec, shift);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddVecI64x2'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubVecI64x2'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulVecI64x2'], left, right);
    },
    'extmul_low_i32x4_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulLowSVecI64x2'], left, right);
    },
    'extmul_high_i32x4_s'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulHighSVecI64x2'], left, right);
    },
    'extmul_low_i32x4_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulLowUVecI64x2'], left, right);
    },
    'extmul_high_i32x4_u'(left, right) {
      return Module['_BinaryenBinary'](module, Module['ExtMulHighUVecI64x2'], left, right);
    },
    'extend_low_i32x4_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendLowSVecI32x4ToVecI64x2'], value);
    },
    'extend_high_i32x4_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendHighSVecI32x4ToVecI64x2'], value);
    },
    'extend_low_i32x4_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendLowUVecI32x4ToVecI64x2'], value);
    },
    'extend_high_i32x4_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ExtendHighUVecI32x4ToVecI64x2'], value);
    },
  };

  self['f32x4'] = {
    'splat'(value) {
      return Module['_BinaryenUnary'](module, Module['SplatVecF32x4'], value);
    },
    'extract_lane'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneVecF32x4'], vec, index);
    },
    'replace_lane'(vec, index, value) {
      return Module['_BinaryenSIMDReplace'](module, Module['ReplaceLaneVecF32x4'], vec, index, value);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqVecF32x4'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeVecF32x4'], left, right);
    },
    'lt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtVecF32x4'], left, right);
    },
    'gt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtVecF32x4'], left, right);
    },
    'le'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeVecF32x4'], left, right);
    },
    'ge'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeVecF32x4'], left, right);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsVecF32x4'], value);
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegVecF32x4'], value);
    },
    'sqrt'(value) {
      return Module['_BinaryenUnary'](module, Module['SqrtVecF32x4'], value);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddVecF32x4'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubVecF32x4'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulVecF32x4'], left, right);
    },
    'div'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivVecF32x4'], left, right);
    },
    'min'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinVecF32x4'], left, right);
    },
    'max'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxVecF32x4'], left, right);
    },
    'pmin'(left, right) {
      return Module['_BinaryenBinary'](module, Module['PMinVecF32x4'], left, right);
    },
    'pmax'(left, right) {
      return Module['_BinaryenBinary'](module, Module['PMaxVecF32x4'], left, right);
    },
    'ceil'(value) {
      return Module['_BinaryenUnary'](module, Module['CeilVecF32x4'], value);
    },
    'floor'(value) {
      return Module['_BinaryenUnary'](module, Module['FloorVecF32x4'], value);
    },
    'trunc'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncVecF32x4'], value);
    },
    'nearest'(value) {
      return Module['_BinaryenUnary'](module, Module['NearestVecF32x4'], value);
    },
    'convert_i32x4_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ConvertSVecI32x4ToVecF32x4'], value);
    },
    'convert_i32x4_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ConvertUVecI32x4ToVecF32x4'], value);
    },
    'demote_f64x2_zero'(value) {
      return Module['_BinaryenUnary'](module, Module['DemoteZeroVecF64x2ToVecF32x4'], value);
    },
  };

  self['f64x2'] = {
    'splat'(value) {
      return Module['_BinaryenUnary'](module, Module['SplatVecF64x2'], value);
    },
    'extract_lane'(vec, index) {
      return Module['_BinaryenSIMDExtract'](module, Module['ExtractLaneVecF64x2'], vec, index);
    },
    'replace_lane'(vec, index, value) {
      return Module['_BinaryenSIMDReplace'](module, Module['ReplaceLaneVecF64x2'], vec, index, value);
    },
    'eq'(left, right) {
      return Module['_BinaryenBinary'](module, Module['EqVecF64x2'], left, right);
    },
    'ne'(left, right) {
      return Module['_BinaryenBinary'](module, Module['NeVecF64x2'], left, right);
    },
    'lt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LtVecF64x2'], left, right);
    },
    'gt'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GtVecF64x2'], left, right);
    },
    'le'(left, right) {
      return Module['_BinaryenBinary'](module, Module['LeVecF64x2'], left, right);
    },
    'ge'(left, right) {
      return Module['_BinaryenBinary'](module, Module['GeVecF64x2'], left, right);
    },
    'abs'(value) {
      return Module['_BinaryenUnary'](module, Module['AbsVecF64x2'], value);
    },
    'neg'(value) {
      return Module['_BinaryenUnary'](module, Module['NegVecF64x2'], value);
    },
    'sqrt'(value) {
      return Module['_BinaryenUnary'](module, Module['SqrtVecF64x2'], value);
    },
    'add'(left, right) {
      return Module['_BinaryenBinary'](module, Module['AddVecF64x2'], left, right);
    },
    'sub'(left, right) {
      return Module['_BinaryenBinary'](module, Module['SubVecF64x2'], left, right);
    },
    'mul'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MulVecF64x2'], left, right);
    },
    'div'(left, right) {
      return Module['_BinaryenBinary'](module, Module['DivVecF64x2'], left, right);
    },
    'min'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MinVecF64x2'], left, right);
    },
    'max'(left, right) {
      return Module['_BinaryenBinary'](module, Module['MaxVecF64x2'], left, right);
    },
    'pmin'(left, right) {
      return Module['_BinaryenBinary'](module, Module['PMinVecF64x2'], left, right);
    },
    'pmax'(left, right) {
      return Module['_BinaryenBinary'](module, Module['PMaxVecF64x2'], left, right);
    },
    'ceil'(value) {
      return Module['_BinaryenUnary'](module, Module['CeilVecF64x2'], value);
    },
    'floor'(value) {
      return Module['_BinaryenUnary'](module, Module['FloorVecF64x2'], value);
    },
    'trunc'(value) {
      return Module['_BinaryenUnary'](module, Module['TruncVecF64x2'], value);
    },
    'nearest'(value) {
      return Module['_BinaryenUnary'](module, Module['NearestVecF64x2'], value);
    },
    'convert_low_i32x4_s'(value) {
      return Module['_BinaryenUnary'](module, Module['ConvertLowSVecI32x4ToVecF64x2'], value);
    },
    'convert_low_i32x4_u'(value) {
      return Module['_BinaryenUnary'](module, Module['ConvertLowUVecI32x4ToVecF64x2'], value);
    },
    'promote_low_f32x4'(value) {
      return Module['_BinaryenUnary'](module, Module['PromoteLowVecF32x4ToVecF64x2'], value);
    },
  };

  self['funcref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['funcref']);
    }
  };

  self['externref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['externref']);
    }
  };

  self['anyref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['anyref']);
    }
  };

  self['eqref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['eqref']);
    }
  };

  self['i31ref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['i31ref']);
    }
  };

  self['structref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['structref']);
    }
  };

  self['stringref'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['stringref']);
    }
  };

  self['stringview_wtf8'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['stringview_wtf8']);
    }
  };

  self['stringview_wtf16'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['stringview_wtf16']);
    }
  };

  self['stringview_iter'] = {
    'pop'() {
      return Module['_BinaryenPop'](module, Module['stringview_iter']);
    }
  };

  self['ref'] = {
    'null'(type) {
      return Module['_BinaryenRefNull'](module, type);
    },
    'is_null'(value) {
      return Module['_BinaryenRefIsNull'](module, value);
    },
    'as_non_null'(value) {
      return Module['_BinaryenRefAs'](module, Module['RefAsNonNull'], value);
    },
    'func'(func, type) {
      return preserveStack(() => Module['_BinaryenRefFunc'](module, strToStack(func), type));
    },
    'eq'(left, right) {
      return Module['_BinaryenRefEq'](module, left, right);
    }
  };

  self['select'] = function(condition, ifTrue, ifFalse, type) {
    return Module['_BinaryenSelect'](module, condition, ifTrue, ifFalse, typeof type !== 'undefined' ? type : Module['auto']);
  };
  self['drop'] = function(value) {
    return Module['_BinaryenDrop'](module, value);
  };
  self['return'] = function(value) {
    return Module['_BinaryenReturn'](module, value);
  };
  self['nop'] = function() {
    return Module['_BinaryenNop'](module);
  };
  self['unreachable'] = function() {
    return Module['_BinaryenUnreachable'](module);
  };

  self['atomic'] = {
    'fence'() {
      return Module['_BinaryenAtomicFence'](module);
    }
  };

  self['try'] = function(name, body, catchTags, catchBodies, delegateTarget) {
    return preserveStack(() =>
      Module['_BinaryenTry'](module, name ? strToStack(name) : 0, body, i32sToStack(catchTags.map(strToStack)), catchTags.length, i32sToStack(catchBodies), catchBodies.length, delegateTarget ? strToStack(delegateTarget) : 0));
  };
  self['throw'] = function(tag, operands) {
    return preserveStack(() => Module['_BinaryenThrow'](module, strToStack(tag), i32sToStack(operands), operands.length));
  };
  self['rethrow'] = function(target) {
    return Module['_BinaryenRethrow'](module, strToStack(target));
  };

  self['tuple'] = {
    'make'(elements) {
      return preserveStack(() => Module['_BinaryenTupleMake'](module, i32sToStack(elements), elements.length));
    },
    'extract'(tuple, index) {
      return Module['_BinaryenTupleExtract'](module, tuple, index);
    }
  };

  self['i31'] = {
    'new'(value) {
      return Module['_BinaryenI31New'](module, value);
    },
    'get_s'(i31) {
      return Module['_BinaryenI31Get'](module, i31, 1);
    },
    'get_u'(i31) {
      return Module['_BinaryenI31Get'](module, i31, 0);
    }
  };

  // TODO: extern.internalize
  // TODO: extern.externalize
  // TODO: ref.test
  // TODO: ref.cast
  // TODO: br_on_*
  // TODO: struct.*
  // TODO: array.*
  // TODO: string.*
  // TODO: stringview_wtf8.*
  // TODO: stringview_wtf16.*
  // TODO: stringview_iter.*

  // 'Module' operations
  self['addFunction'] = function(name, params, results, varTypes, body) {
    return preserveStack(() =>
      Module['_BinaryenAddFunction'](module, strToStack(name), params, results, i32sToStack(varTypes), varTypes.length, body)
    );
  };
  self['getFunction'] = function(name) {
    return preserveStack(() => Module['_BinaryenGetFunction'](module, strToStack(name)));
  };
  self['removeFunction'] = function(name) {
    return preserveStack(() => Module['_BinaryenRemoveFunction'](module, strToStack(name)));
  };
  self['addGlobal'] = function(name, type, mutable, init) {
    return preserveStack(() => Module['_BinaryenAddGlobal'](module, strToStack(name), type, mutable, init));
  }
  self['getGlobal'] = function(name) {
    return preserveStack(() => Module['_BinaryenGetGlobal'](module, strToStack(name)));
  };
  self['addTable'] = function(table, initial, maximum, type = Module['_BinaryenTypeFuncref']()) {
    return preserveStack(() => Module['_BinaryenAddTable'](module, strToStack(table), initial, maximum, type));
  }
  self['getTable'] = function(name) {
    return preserveStack(() => Module['_BinaryenGetTable'](module, strToStack(name)));
  };
  self['addActiveElementSegment'] = function(table, name, funcNames, offset = self['i32']['const'](0)) {
    return preserveStack(() => Module['_BinaryenAddActiveElementSegment'](
      module,
      strToStack(table),
      strToStack(name),
      i32sToStack(funcNames.map(strToStack)),
      funcNames.length,
      offset
    ));
  };
  self['addPassiveElementSegment'] = function(name, funcNames) {
    return preserveStack(() => Module['_BinaryenAddPassiveElementSegment'](
      module,
      strToStack(name),
      i32sToStack(funcNames.map(strToStack)),
      funcNames.length
    ));
  };
  self['getElementSegment'] = function(name) {
    return preserveStack(() => Module['_BinaryenGetElementSegment'](module, strToStack(name)));
  };
  self['getTableSegments'] = function(table) {
    var numElementSegments = Module['_BinaryenGetNumElementSegments'](module);
    var tableName = UTF8ToString(Module['_BinaryenTableGetName'](table));
    var ret = [];
    for (var i = 0; i < numElementSegments; i++) {
      var segment = Module['_BinaryenGetElementSegmentByIndex'](module, i);
      var elemTableName = UTF8ToString(Module['_BinaryenElementSegmentGetTable'](segment));
      if (tableName === elemTableName) {
        ret.push(segment);
      }
    }
    return ret;
  }
  self['removeGlobal'] = function(name) {
    return preserveStack(() => Module['_BinaryenRemoveGlobal'](module, strToStack(name)));
  }
  self['removeTable'] = function(name) {
    return preserveStack(() => Module['_BinaryenRemoveTable'](module, strToStack(name)));
  };
  self['removeElementSegment'] = function(name) {
    return preserveStack(() => Module['_BinaryenRemoveElementSegment'](module, strToStack(name)));
  };
  self['addTag'] = function(name, params, results) {
    return preserveStack(() => Module['_BinaryenAddTag'](module, strToStack(name), params, results));
  };
  self['getTag'] = function(name) {
    return preserveStack(() => Module['_BinaryenGetTag'](module, strToStack(name)));
  };
  self['removeTag'] = function(name) {
    return preserveStack(() => Module['_BinaryenRemoveTag'](module, strToStack(name)));
  };
  self['addFunctionImport'] = function(internalName, externalModuleName, externalBaseName, params, results) {
    return preserveStack(() =>
      Module['_BinaryenAddFunctionImport'](module, strToStack(internalName), strToStack(externalModuleName), strToStack(externalBaseName), params, results)
    );
  };
  self['addTableImport'] = function(internalName, externalModuleName, externalBaseName) {
    return preserveStack(() =>
      Module['_BinaryenAddTableImport'](module, strToStack(internalName), strToStack(externalModuleName), strToStack(externalBaseName))
    );
  };
  self['addMemoryImport'] = function(internalName, externalModuleName, externalBaseName, shared) {
    return preserveStack(() =>
      Module['_BinaryenAddMemoryImport'](module, strToStack(internalName), strToStack(externalModuleName), strToStack(externalBaseName), shared)
    );
  };
  self['addGlobalImport'] = function(internalName, externalModuleName, externalBaseName, globalType, mutable) {
    return preserveStack(() =>
      Module['_BinaryenAddGlobalImport'](module, strToStack(internalName), strToStack(externalModuleName), strToStack(externalBaseName), globalType, mutable)
    );
  };
  self['addTagImport'] = function(internalName, externalModuleName, externalBaseName, params, results) {
    return preserveStack(() =>
      Module['_BinaryenAddTagImport'](module, strToStack(internalName), strToStack(externalModuleName), strToStack(externalBaseName), params, results)
    );
  };
  self['addExport'] = // deprecated
  self['addFunctionExport'] = function(internalName, externalName) {
    return preserveStack(() => Module['_BinaryenAddFunctionExport'](module, strToStack(internalName), strToStack(externalName)));
  };
  self['addTableExport'] = function(internalName, externalName) {
    return preserveStack(() => Module['_BinaryenAddTableExport'](module, strToStack(internalName), strToStack(externalName)));
  };
  self['addMemoryExport'] = function(internalName, externalName) {
    return preserveStack(() => Module['_BinaryenAddMemoryExport'](module, strToStack(internalName), strToStack(externalName)));
  };
  self['addGlobalExport'] = function(internalName, externalName) {
    return preserveStack(() => Module['_BinaryenAddGlobalExport'](module, strToStack(internalName), strToStack(externalName)));
  };
  self['addTagExport'] = function(internalName, externalName) {
    return preserveStack(() => Module['_BinaryenAddTagExport'](module, strToStack(internalName), strToStack(externalName)));
  };
  self['removeExport'] = function(externalName) {
    return preserveStack(() => Module['_BinaryenRemoveExport'](module, strToStack(externalName)));
  };
  self['setMemory'] = function(initial, maximum, exportName, segments = [], shared = false, memory64 = false, internalName = null) {
    // segments are assumed to be { passive: bool, offset: expression ref, data: array of 8-bit data }
    return preserveStack(() => {
      const segmentsLen = segments.length;
      const segmentData = new Array(segmentsLen);
      const segmentDataLen = new Array(segmentsLen);
      const segmentPassive = new Array(segmentsLen);
      const segmentOffset = new Array(segmentsLen);
      for (let i = 0; i < segmentsLen; i++) {
        const { data, offset, passive } = segments[i];
        segmentData[i] = stackAlloc(data.length);
        HEAP8.set(data, segmentData[i]);
        segmentDataLen[i] = data.length;
        segmentPassive[i] = passive;
        segmentOffset[i] = offset;
      }
      return Module['_BinaryenSetMemory'](
        module, initial, maximum, strToStack(exportName),
        i32sToStack(segmentData),
        i8sToStack(segmentPassive),
        i32sToStack(segmentOffset),
        i32sToStack(segmentDataLen),
        segmentsLen,
        shared,
        memory64,
        strToStack(internalName)
      );
    });
  };
  self['hasMemory'] = function() {
    return Boolean(Module['_BinaryenHasMemory'](module));
  };
  self['getMemoryInfo'] = function(name) {
    var memoryInfo = {
      'module': UTF8ToString(Module['_BinaryenMemoryImportGetModule'](module, strToStack(name))),
      'base': UTF8ToString(Module['_BinaryenMemoryImportGetBase'](module, strToStack(name))),
      'initial': Module['_BinaryenMemoryGetInitial'](module, strToStack(name)),
      'shared': Boolean(Module['_BinaryenMemoryIsShared'](module, strToStack(name))),
      'is64': Boolean(Module['_BinaryenMemoryIs64'](module, strToStack(name))),
    };
    if (Module['_BinaryenMemoryHasMax'](module, strToStack(name))) {
      memoryInfo['max'] = Module['_BinaryenMemoryGetMax'](module, strToStack(name));
    }
    return memoryInfo;
  };
  self['getNumMemorySegments'] = function() {
    return Module['_BinaryenGetNumMemorySegments'](module);
  };
  self['getMemorySegmentInfoByIndex'] = function(id) {
    const passive = Boolean(Module['_BinaryenGetMemorySegmentPassive'](module, id));
    let offset = null;
    if (!passive) {
      offset = Module['_BinaryenGetMemorySegmentByteOffset'](module, id);
    }
    return {
      'offset': offset,
      'data': (function(){
        const size = Module['_BinaryenGetMemorySegmentByteLength'](module, id);
        const ptr = _malloc(size);
        Module['_BinaryenCopyMemorySegmentData'](module, id, ptr);
        const res = new Uint8Array(size);
        res.set(HEAP8.subarray(ptr, ptr + size));
        _free(ptr);
        return res.buffer;
      })(),
      'passive': passive
    };
  };
  self['setStart'] = function(start) {
    return Module['_BinaryenSetStart'](module, start);
  };
  self['getFeatures'] = function() {
    return Module['_BinaryenModuleGetFeatures'](module);
  };
  self['setFeatures'] = function(features) {
    Module['_BinaryenModuleSetFeatures'](module, features);
  };
  self['addCustomSection'] = function(name, contents) {
    return preserveStack(() =>
      Module['_BinaryenAddCustomSection'](module, strToStack(name), i8sToStack(contents), contents.length)
    );
  };
  self['getExport'] = function(externalName) {
    return preserveStack(() => Module['_BinaryenGetExport'](module, strToStack(externalName)));
  };
  self['getNumExports'] = function() {
    return Module['_BinaryenGetNumExports'](module);
  };
  self['getExportByIndex'] = function(index) {
    return Module['_BinaryenGetExportByIndex'](module, index);
  };
  self['getNumFunctions'] = function() {
    return Module['_BinaryenGetNumFunctions'](module);
  };
  self['getFunctionByIndex'] = function(index) {
    return Module['_BinaryenGetFunctionByIndex'](module, index);
  };
  self['getNumGlobals'] = function() {
    return Module['_BinaryenGetNumGlobals'](module);
  };
  self['getNumTables'] = function() {
    return Module['_BinaryenGetNumTables'](module);