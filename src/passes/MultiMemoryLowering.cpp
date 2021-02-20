/*
 * Copyright 2022 WebAssembly Community Group participants
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

//
// Condensing a module with multiple memories into a module with a single memory
// for browsers that don’t support multiple memories.
//
// This pass also disables multi-memories so that the target features section in
// the emitted module does not report the use of MultiMemories. Disabling the
// multi-memories feature also prevents later passes from adding additional
// memories.
//
// The offset computation in function maybeMakeBoundsCheck is not precise
// according to the spec. In the spec offsets do not overflow as
// twos-complement, but i32.add does. Concretely, a load from address 1000 with
// offset 0xffffffff should actually trap, as the combined number is greater
// than 32 bits. But with an add, 1000 + 0xffffffff = 999 due to overflow, which
// would not trap. In theory we could compute like the spec, by expanding the
// i32s to i64s and adding there (where we won't overflow), but we don't have
// i128s to handle i64 overflow.
//
// The Atomic instructions memory.atomic.wait and memory.atomic.notify, have
// browser engine impl