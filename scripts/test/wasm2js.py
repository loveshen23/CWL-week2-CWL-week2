# Copyright 2016 WebAssembly Community Group participants
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

from . import shared
from . import support

tests = shared.get_tests(shared.options.binaryen_test)
# memory64 is not supported in wasm2js yet (but may be with BigInt eventually).
tests = [t for t in tests if '64.wast' not in t]
spec_tests = shared.options.spec_tests
spec_tests = [t for t in spec_tests if '.fail' not in t]
spec_tests = [t for t in spec_tests if '64.wast' not in t]
wasm2js_tests = shared.get_tests(shared.get_test_dir('wasm2js'), ['.wast'])
assert_tests = ['wasm2js.wast.asserts']
# These tests exercise functionality not supported by wasm2js
wasm2js_blacklist = ['empty_imported_table.wast']


def check_for_stale_files():
    if shared.options.test_name_filter:
        return

    # TODO(sbc): Generalize and apply other test suites
    all_tests = []
    for t in tests + spec_tests + wasm2js_tests:
        all_tests.append(os.path.basename(os.path.splitext(t)[0]))

    all_files = os.listdir(shared.get_test_dir('wasm2js'))
    for f in all_files:
     