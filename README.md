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
 * [`Grain`](https://github.com/gr