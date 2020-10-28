[![CI](https://github.com/WebAssembly/binaryen/workflows/CI/badge.svg?branch=main&event=push)](https://github.com/WebAssembly/binaryen/actions?query=workflow%3ACI)

# Binaryen

Binaryen is a compiler and toolchain infrastructure library for WebAssembly,
written in C++. It aims to make [compiling to WebAssembly] **easy, fast, and
effective**:

 * **Easy**: Binaryen has a simple [C API] in a single header, and can also be
   [used from JavaScript][JS_API]. It accepts input in [WebAssembly-like
   form][compile_to_wasm] but also accepts a general [control flow graph] for
   compilers that prefer that.

 * **Fast**: Binaryen's internal IR uses compact data structures and is designed
   for completely parallel codegen and optimization, using all available CPU
   cores. Binaryen's IR also compiles down to WebAssembly extremely easily and
   quickly because it is essentially a subset of WebAssembly.

 * **Effective**: Binaryen's optimizer has many passes (see an overview later
   down) that can improve code size and speed. These optimizations aim to make
   Binaryen powerful enough to be used as a [compiler backend][backend] by
   itself.  One specific area of focus is on WebAssembly-specific optimizations
   (that general-purpose compilers might not do), which you can think of as
   wasm [minification], similar to minification for JavaScript, CSS, etc., all
   of which are language-specific.

Compilers using Binaryen include:

 * [`AssemblyScript`](https://github.com/AssemblyScript/assemblyscript) which compiles a variant of TypeScript to WebAssembly
 * [`wasm2js`](https://github.com/WebAssembly/binaryen/blob/main/src/wasm2js.h) which compiles WebAssembly to JS
 * [`Asterius`](https://github.com/tweag/asterius) which compiles Haskell to WebAssembly
 * [`Grain`](https://github.com/grain-lang/grain) which compiles Grain to WebAssembly

Binaryen also provides a set of **toolchain utilities** that can

 * **Parse** and **emit** WebAssembly. In particular this lets you load
   WebAssembly, optimize it using Binaryen, and re-emit it, thus implementing a
   wasm-to-wasm optimizer in a single command.
 * **Interpret** WebAssembly as well as run the WebAssembly spec tests.
 * Integrate with **[Emscripten](http://emscripten.org)** in order to provide a
   complete compiler toolchain from C and C++ to WebAssembly.
 * **Polyfill** WebAssembly by running it in the interpreter compiled to
   JavaScript, if the browser does not yet have native support (useful for
   testing).

Consult the [contributing instructions](Contributing.md) if you're interested in
participating.

## Binaryen IR

Binaryen's internal IR is designed to be

 * **Flexible and fast** for optimization.
 * **As close as possible to WebAssembly** so it is simple and fast to convert
   it to and from WebAssembly.

There are a few differences between Binaryen IR and the WebAssembly language:

 * Tree structure
   * Binaryen IR [is a tree][binaryen_ir], i.e., it has hierarchical structure,
     for convenience of optimization. This differs from the WebAssembly binary
     format which is a stack machine.
   * Consequently Binaryen's text format allows only s-expressions.
     WebAssembly's official text format is primarily a linear instruction list
     (with s-expression extensions). Binaryen can't read the linear style, but
     it can read a wasm text file if it contains only s-expressions.
   * Binaryen uses Stack IR to optimize "stacky" code (that can't be
     represented in structured form).
   * When stacky code must be represented in Binaryen IR, such as with
     multivalue instructions and blocks, it is represented with tuple types that
     do not exist in the WebAssembly language. In addition to multivalue
     instructions, locals and globals can also have tuple types in Binaryen IR
     but not in WebAssembly. Experiments show that better support for
     multivalue could enable useful but small code size savings of 1-3%, so it
     has not been worth changing the core IR structure to support it better.
   * Block input values (currently only supported in `catch` blocks in the
     exception handling feature) are represented as `pop` subexpressions.
 * Types and unreachable code
   * WebAssembly limits block/if/loop types to none and the concrete value types
     (i32, i64, f32, f64). Binaryen IR has an unreachable type, and it allows
     block/if/loop to take it, allowing [local transforms that don't need to
     know the global context][unreachable]. As a result, Binaryen's default
     text output is not necessarily valid wasm text. (To get valid wasm text,
     you can do `--generate-stack-ir --print-stack-ir`, which prints Stack IR,
     this is guaranteed to be valid for wasm parsers.)
   * Binaryen ignores unreachable code when reading WebAssembly binaries. That
     means that if you read a wasm file with unreachable code, that code will be
     discarded as if it were optimized out (often this is what you want anyhow,
     and optimized programs have no unreachable code anyway, but if you write an
     unoptimized file and then read it, it may look different). The reason for
     this behavior is that unreachable code in WebAssembly has corner cases that
     are tricky to handle in Binaryen IR (it can be very unstructured, and
     Binaryen IR is more structured than WebAssembly as noted earlier). Note
     that Binaryen does support unreachable code in .wat text files, since as we
     saw Binaryen only supports s-expressions there, which are structured.
 * Blocks
   * Binaryen IR has only one node that contains a variable-length list of
     operands: the block. WebAssembly on the other hand allows lists in loops,
     if arms, and the top level of a function. Binaryen's IR has a single
     operand for all non-block nodes; this operand may of course be a block.
     The motivation for this property is that many passes need special code
     for iterating on lists, so having a single IR node with a list simplifies
     them.
   * As in wasm, blocks and loops may have names. Branch targets in the IR are
     resolved by name (as opposed to nesting depth). This has 2 consequences:
     * Blocks without names may not be branch targets.
     * Names are required to be unique. (Reading .wat files with duplicate names
       is supported; the names are modified when the IR is constructed).
   * As an optimization, a block that is the child of a loop (or if arm, or
     function toplevel) and which has no branches targeting it will not be
     emitted when generating wasm. Instead its list of operands will be directly
     used in the containing node. Such a block is sometimes called an "implicit
     block".
 * Reference Types
  * The wasm text and binary formats require that a function whose address is
    taken by `ref.func` must be either in the table, or declared via an
    `(elem declare func $..)`. Binaryen will emit that data when necessary, but
    it does not represent it in IR. That is, IR can be worked on without needing
    to think about declaring function references.
  * Binaryen IR allows non-nullable locals in the form that the wasm spec does,
    (which was historically nicknamed "1a"), in which a `local.get` must be
    structurally dominated by a `local.set` in order to validate (that ensures
    we do not read the default value of null). Despite being aligned with the
    wasm spec, there are some minor details that you may notice:
    * A nameless `Block` in Binaryen IR does not interfere with validation.
      Nameless blocks are never emitted into the binary format (we just emit
      their contents), so we ignore them for purposes of non-nullable locals. As
      a result, if you read wasm text emitted by Binaryen then you may see what
      seems to be code that should not validate per the spec (and may not
      validate in wasm text parsers), but that difference will not exist in the
      binary format (binaries emitted by Binaryen will always work everywhere,
      aside for bugs of course).
    * The Binaryen pass runner will automatically fix up validation after each
      pass (finding things that do not validate and fixing them up, usually by
      demoting a local to be nullable). As a result you do not need to worry
      much about this when writing Binaryen passes. For more details see the
      `requiresNonNullableLocalFixups()` hook in `pass.h` and the
      `LocalStructuralDominance` class.

As a result, you might notice that round-trip conversions (wasm => Binaryen IR
=> wasm) change code a little in some corner cases.

 * When optimizing Binaryen uses an additional IR, Stack IR (see
   `src/wasm-stack.h`). Stack IR allows a bunch of optimizations that are
   tailored for the stack machine form of WebAssembly's binary format (but Stack
   IR is less efficient for general optimizations than the main Binaryen IR). If
   you have a wasm file that has been particularly well-optimized, a simple
   round-trip conversion (just read and write, without optimization) may cause
   more noticeable differences, as Binaryen fits it into Binaryen IR's more
   structured format. If you also optimize during the round-trip conversion then
   Stack IR opts will be run and the final wasm will be better optimized.

Notes when working with Binaryen IR:

 * As mentioned above, Binaryen IR has a tree structure. As a result, each
   expression should have exactly one parent - you should not "reuse" a node by
   having it appear more than once in the tree. The motivation for this
   limitation is that when we optimize we modify nodes, so if they appear more
   than once in the tree, a change in one place can appear in another
   incorrectly.
 * For similar reasons, nodes should not appear in more than one functions.

### Intrinsics

Binaryen intrinsic functions look like calls to imports, e.g.,

```wat
(import "binaryen-intrinsics" "foo" (func $foo))
```

Implementing them that way allows them to be read and written by other tools,
and it avoids confusing errors on a binary format error that could happen in
those tools if we had a custom binary format extension.

An intrinsic method may be optimized away by the optimizer. If it is not, it
must be **lowered** before shipping the wasm, as otherwise it will look like a
call to an import that does not exist (and VMs will show an error on not having
a proper value for that import). That final lowering is *not* done
automatically. A user of intrinsics must run the pass for that explicitly,
because the tools do not know when the user intends to finish optimizing, as the
user may have a pipeline of multiple optimization steps, or may be doing local
experimentation, or fuzzing/reducing, etc. Only the user knows when the final
optimization happens before the wasm is "final" and ready to be shipped. Note
that, in general, some additional optimizations may be possible after the final
lowering, and so a useful pattern is to  optimize once normally with intrinsics,
then lower them away, then optimize after that, e.g.:

```
wasm-opt input.wasm -o output.wasm  -O --intrinsic-lowering -O
```

Each intrinsic defines its semantics, which includes what the optimizer is
allowed to do with it and what the final lowering will turn it to. See
[intrinsics.h](https://github.com/WebAssembly/binaryen/blob/main/src/ir/intrinsics.h)
for the detailed definitions. A quick summary appears here:

* `call.without.effects`: Similar to a `call_ref` in that it receives
  parameters, and a reference to a function to call, and calls that function
  with those parameters, except that the optimizer can assume the call has no
  side effects, and may be able to optimize it out (if it does not have a
  result that is used, generally).

## Tools

This repository contains code that builds the following tools in `bin/`:

 * **wasm-opt**: Loads WebAssembly and runs Binaryen IR passes on it.
 * **wasm-as**: Assembles WebAssembly in text format (currently S-Expression
   format) into binary format (going through Binaryen IR).
 * **wasm-dis**: Un-assembles WebAssembly in binary format into text format
   (going through Binaryen IR).
 * **wasm2js**: A WebAssembly-to-JS compiler. This is used by Emscripten to
   generate JavaScript as an alternative to WebAssembly.
 * **wasm-reduce**: A testcase reducer for WebAssembly files. Given a wasm file
   that is interesting for some reason (say, it crashes a specific VM),
   wasm-reduce can find a smaller wasm file that has the same property, which is
   often easier to debug. See the
   [docs](https://github.com/WebAssembly/binaryen/wiki/Fuzzing#reducing)
   for more details.
 * **wasm-shell**: A shell that can load and interpret WebAssembly code. It can
   also run the spec test suite.
 * **wasm-emscripten-finalize**: Takes a wasm binary produced by llvm+lld and
   performs emscripten-specific passes over it.
 * **wasm-ctor-eval**: A tool that can execute functions (or parts of functions)
   at compile time.
 * **binaryen.js**: A standalone JavaScript library that exposes Binaryen methods for [creating and optimizing Wasm modules](https://github.com/WebAssembly/binaryen/blob/main/test/binaryen.js/hello-world.js). For builds, see [binaryen.js on npm](https://www.npmjs.com/package/binaryen) (or download it directly from [github](https://raw.githubusercontent.com/AssemblyScript/binaryen.js/master/index.js), [rawgit](https://cdn.rawgit.com/AssemblyScript/binaryen.js/master/index.js), or [unpkg](https://unpkg.com/binaryen@latest/index.js)). Minimal requirements: Node.js v15.8 or Chrome v75 or Firefox v78.

Usage instructions for each are below.

## Binaryen Optimizations

Binaryen contains
[a lot of optimization passes](https://github.com/WebAssembly/binaryen/tree/main/src/passes)
to make WebAssembly smaller and faster. You can run the Binaryen optimizer 