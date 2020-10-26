Changelog
=========

This document describes changes between tagged Binaryen versions.

To browse or download snapshots of old tagged versions, visit
https://github.com/WebAssembly/binaryen/releases.

Not all changes are documented here. In particular, new features, user-oriented
fixes, options, command-line parameters, usage changes, deprecations,
significant internal modifications and optimizations etc. generally deserve a
mention. To examine the full set of changes between versions, visit the link to
full changeset diff at the end of each section.

Current Trunk
-------------

v112
----

- Add AbstractTypeRefining pass (#5461)
- Add a mechanism to skip a pass by name (#5448)
- Add TypeMerging pass (#5321)
- Add TypeSSA pass  (#5299)
- Optimization sequences like `-O3 -Os` now do the expected thing and run `-O3`
  followed by `-Os`. Previously the last of them set the defaults that were used
  by all executions, so `-O3 -Os` was equivalent to `-Os -Os`. (There is no
  change to the default optimization level that other passes can see. For
  example, `--precompute-propagate -O2 -O1` will run `--precompute-propagate`
  at opt level `1`, as the global default is set to `2` and then overridden to
  `1`. The only change is that the passes run by `-O2` will actually run `-O2`
  now, while before they'd use the global default which made them do `-O1`.)
- Add `--closed-world` flag. This enables more optimizations in GC mode as it
  lets us assume that we can change types inside the module.
- The isorecursive WasmGC type system (i.e. --hybrid) is now the default to
  match the spec and the old default equirecursive (i.e. --structural) system
  has been removed.
- `ref.is_func`, `ref.is_data`, and `ref.is_i31` have been removed from the C
  and JS APIs and `RefIs` has been replaced with `RefIsNull`.
- Types `Data` and `Dataref` have been replaced with types `Struct` and
  `Structref` in the C and JS APIs.
* `BinaryenStringNew` now takes an additional last argument, `try_`, indicating
  whether the instruction is one of `string.new_utf8_try` respectively
  `string.new_utf8_array_try`.
* `BinaryenStringEq` now takes an additional second argument, `op`, that is
  either `BinaryenStringEqEqual()` if the instruction is `string.eq` or
  `BinaryenStringEqCompare()` if the instruction is `string.compare`.

v111
----

- Add extra `memory64` argument for `BinaryenSetMemory` and new
  `BinaryenMemoryIs64` C-API method to determine 64-bit memory. (#4963)
- `TypeBuilderSetSubType` now takes a supertype as the second argument.
- `call_ref` now takes a mandatory signature type immediate.
- If `THROW_ON_FATAL` is defined at compile-time, then fatal errors will throw a
  `std::runtime_error` instead of terminating the process. This may be used by
  embedders of Binaryen to recover from errors.
- Implemented bottom heap types: `none`, `nofunc`, and `noextern`. RefNull
  expressions and null `Literal`s must now have type `nullref`, `nullfuncref`,
  or `nullexternref`.
- The C-API's `BinaryenTypeI31ref` and `BinaryenTypeDataref` now return nullable
  types.
- The `sign-extension` and `mutable-globals` features are now both enabled by
  default in all tools. This is in order to match llvm's defaults (See
  https://reviews.llvm.org/D125728).
- Add a pass to lower sign-extension operations to MVP.

v110
----

- Add support for non-nullable locals in wasm GC. (#4959)
- Add support for multiple memories. (#4811)
- Add support for the wasm Strings proposal. (see PRs with [Strings] in name)
- Add a new flag to Directize, `--pass-arg=directize-initial-contents-immutable`
  which indicates the initial table contents are immutable. That is the case for
  LLVM, for example, and it allows us to optimize more indirect calls to direct
  ones. (#4942)
- Change constant values of some reference types in the C and JS APIs. This is
  only observable if you hardcode specific values instead of calling the
  relevant methods (like `BinaryenTypeDataref()`). (#4755)
- `BinaryenModulePrintStackIR`, `BinaryenModuleWriteStackIR` and
  `BinaryenModuleAllocateAndWriteStackIR` now have an extra boolean
  argument `optimize`. (#4832)
- Remove support for the `let` instruction that has been removed from the typed
  function references spec.
- HeapType::ext has been restored but is no longer a subtype of HeapType::any to
  match the latest updates in the GC spec. (#4898)
- `i31ref` and `dataref` are now nullable to match the latest GC spec. (#4843)
- Add support for `extern.externalize` and `extern.internalize`. (#4975)

v109
----

- Add Global Struct Inference pass (#4659) (#4714)
- Restore and fix SpillPointers pass (#4570)
- Update relaxed SIMD instructions to latest spec

v108
----

- Add CMake flag BUILD_TOOLS to control building tools (#4655)
- Add CMake flag JS_OF_OCAML for js_of_ocaml (#4637)
- Remove externref (#4633)

v107
----

- Update the wasm GC type section binary format (#4625, #4631)
- Lift the restriction in liveness-traversal.h on max 65535 locals (#4567)
- Switch to nominal fuzzing by default (#4610)
- Refactor Feature::All to match FeatureSet.setAll() (#4557)
- New Signature Pruning pass (#4545)
- Add support for extended-const proposal (#4529)
- Add BUILD_TESTS CMake option to make gtest dependency optional.
- Updated tests to use filecheck 0.0.22 (#4537). Updating is required to
  successfully run the lit tests. This can be done with
  `pip3 install -r requirements-dev.txt`.

v106
----

- [wasm2js] Support exports of Globals (#4523)
- MergeSimilarFunctions optimization pass (#4414)
- Various wasm-ctor-eval improvements, including support for GC.

v105
----

- This release contains binaries for ARM64 MacOS devices (#4397)
- Otherwise, mostly bug fixes and incremental optimization improvements.

v104
----

- Bugfixes only, release created due to incorrect github release artifacts in
  v103 release (#4398).

v103
----

- The EffectAnalyzer now takes advantage of immutability of globals. To achieve
  that it must have access to the module. That is already the case in the C++
  API, but the JS API allowed one to optionally not add a module when calling
  `getSideEffects()`. It is now mandatory to pass in the module.
- JS and Wasm builds now emit ECMAScript modules. New usage is:
  ```js
  import Binaryen from "path/to/binaryen.js";
  const binaryen = await Binaryen();
  ...
  ```
- CallIndirect changed from storing a Signature to storing a HeapType

v102
----

- Add `BinaryenUpdateMaps` to the C API.

- Adds a TrapsNeverHappen mode (#4059). This has many of the benefits of
  IgnoreImplicitTraps, but can be used safely in more cases. IgnoreImplicitTraps
  is now deprecated.

- Adds type argument for BinaryenAddTable method (#4107). For the binaryen.js api
  this parameter is optional and by default is set to funcref type.

- Replace `BinaryenExpressionGetSideEffects`'s features parameter with a module
  parameter.

- OptimizeInstructions now lifts identical code in `select`/`if` arms (#3828). This may cause direct `BinaryenTupleExtract(BinaryenTupleMake(...))` to [use multivalue types](https://github.com/grain-lang/grain/pull/1158).

v101
----

- `BinaryenSetFunctionTable` and `module.setFunctionTable` have been removed
  in favor of `BinaryenAddTable` and `module.addTable` respectively.
- `BinaryenIsFunctionTableImported` is removed.
- A new type `BinaryenElementSegmentRef` has been added to the C API with
  new apis in both C & JS:
  - `BinaryenAddActiveElementSegment`
  - `BinaryenAddPassiveElementSegment`
  - `BinaryenRemoveElementSegment`
  - `BinaryenGetElementSegment`
  - `BinaryenGetElementSegmentByIndex`
  - `BinaryenElementSegmentGetName`
  - `BinaryenElementSegmentSetName`
  - `BinaryenElementSegmentGetTable`
  - `BinaryenElementSegmentSetTable`
  - `BinayenElementSegmentIsPassive`
  - `module.addActiveElementSegment`
  - `module.addPassiveElementSegment`
  - `module.removeElementSegment`
  - `module.getElementSegment`
  - `module.getElementSegmentByIndex`
  - `module.getTableSegments`
  - `module.getNumElementSegments`
  - `binaryen.getElementSegmentInfo`
- `BinaryenAddTable` and `module.addTable` no longer take offset and function
    names.
- `BinaryenGetNumFunctionTableSegments` is replaced with
  `BinaryenGetNumElementSegments`.
- `BinaryenGetFunctionTableSegmentOffset` is replaced with
  `BinaryenElementSegmentGetOffset`.
- `BinaryenGetFunctionTableSegmentLength` is replaced with
  `BinaryenElementSegmentGetLength`.
- `BinaryenGetFunctionTableSegmentData` is replaced with
  `BinaryenElementSegmentGetData`.
- Boolean values in the C API now should use `bool` instead of `int`.
- Experimental SIMD instructions have been removed and the names and opcodes of
  the standard instructions have been updated to match the final spec.

v100
----

- `wasm-dis` now supports options to enable or disable Wasm features.
- Reference types support has been improved by allowing multiple tables in a
  module.
- `call_indirect` and `return_call_indirect` now take an additional table name
  parameter. This is necessary for reference types support.
- New getter/setter methods have been introduced for `call_indirect` table name:
  - `BinaryenCallIndirectGetTable`
  - `BinaryenCallIndirectSetTable`
  - JS API `CallIndirect.table`
- New APIs have been added to add and manipulate multiple tables in a module:
  - `BinaryenAddTable`
  - `BinaryenRemoveTable`
  - `BinaryenGetNumTables`
  - `BinaryenGetTable`
  - `BinaryenGetTableByIndex`
  - `BinaryenTableGetName`
  - `BinaryenTableGetInitial`
  - `BinaryenTableHasMax`
  - `BinaryenTableGetMax`
  - `BinaryenTableImportGetModule`
  - `BinaryenTableImportGetBase`
  - `module.addTable`
  - `module.removeTable`
  - `module.getTable`
  - `module.getTableByIndex`
  - `module.getNumTables`
  - `binaryen.getTableInfo`

v99
---

- Fix optimization behavior on assuming memory is zero-filled. We made that
  assumption before, but it is incorrect in general, which caused problems.
  The fixed behavior is to not assume it, but require the user to pass it in as
  a flag, `--zero-filled-memory`. Large binaries with lots of empty bytes in the
  data section may regress without that flag. Toolchains like Emscripten can
  pass the flag automatically for users if they know it is right to assume,
  which can avoid any regressions. (#3306)
- `RefFunc` C and JS API constructors (`BinaryenRefFunc` and `ref.func`
  respectively) now take an extra `type` parameter, similar to `RefNull`. This
  is necessary for typed function references support.
- JS API functions for atomic notify/wait instructions are renamed.
  - `module.atomic.notify` -> `module.memory.atomic.notify`
  - `module.i32.atomic.wait` -> `module.memory.atomic.wait32`
  - `module.i64.atomic.wait` -> `module.memory.atomic.wait64`
- Remove old/broken SpollPointers pass.  This pass: Spills values that might be
  pointers to the C stack. This allows Boehm-style GC to see them properly.
  This can be revived if needed from git history (#3261).
- Make `NUM_PARAMS` in `FuncCastEmulation` a runtime configuration option named
  `max-func-params`. This defaults to the original value of 16.
- `BinaryenGetFunction`, `BinaryenGetGlobal` and `BinaryenGetEvent` now return
  `NULL` instead of aborting when the respective element does not yet exist.

v98
---

- Add `--fast-math` mode. (#3155)
- Initial implementation of "Memory64" proposal (#3130)
- Lots of changes in support of GC proposal

v9