#!/usr/bin/env python3
#
# Copyright 2015 WebAssembly Community Group participants
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

import glob
import os
import subprocess
import sys
import unittest
from collections import OrderedDict

from scripts.test import binaryenjs
from scripts.test import lld
from scripts.test import shared
from scripts.test import support
from scripts.test import wasm2js
from scripts.test import wasm_opt


def get_changelog_version():
    with open(os.path.join(shared.options.binaryen_root, 'CHANGELOG.md')) as f:
        lines = f.readlines()
    lines = [l for l in lines if len(l.split()) == 1]
    lines = [l for l in lines if l.startswith('v')]
    version = lines[0][1:]
    print("Parsed CHANGELOG.md version: %s" % version)
    return int(version)


def run_version_tests():
    print('[ checking --version ... ]\n')

    not_executable_suffix = ['.DS_Store', '.txt', '.js', '.ilk', '.pdb', '.dll', '.wasm', '.manifest']
    not_executable_prefix = ['binaryen-lit', 'binaryen-unittests']
    bin_files = [os.path.join(shared.options.binaryen_bin, f) for f in os.listdir(shared.options.binaryen_bin)]
    executables = [f for f in bin_files if os.path.isfile(f) and
                   not any(f.endswith(s) for s in not_executable_suffix) and
                   not any(os.path.basename(f).startswith(s) for s in not_executable_prefix)]
    executables = sorted(executables)
    assert len(executables)

    changelog_version = get_changelog_version()
    for e in executables:
        print('.. %s --version' % e)
        out, err = subprocess.Popen([e, '--version'],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE).communicate()
        out = out.decode('utf-8')
        err = err.decode('utf-8')
        assert len(err) == 0, 'Expected no stderr, got:\n%s' % err
        assert os.path.basename(e).replace('.exe', '') in out, 'Expected version to contain program name, got:\n%s' % out
        assert len(out.strip().splitlines()) == 1, 'Expected only version info, got:\n%s' % out
        parts = out.split()
        assert parts[1] == 'version'
        version = int(parts[2])
        assert version == changelog_version


def run_wasm_dis_tests():
    print('\n[ checking wasm-dis on provided binaries... ]\n')

    for t in shared.get_tests(shared.options.binaryen_test, ['.wasm']):
        print('..', os.path.basename(t))
        cmd = shared.WASM_DIS + [t]
        if os.path.isfile(t + '.map'):
            cmd += ['--source-map', t + '.map']

        actual = support.run_command(cmd)
        shared.fail_if_not_identical_to_file(actual, t + '.fromBinary')

        # also verify there are no validation errors
        def check():
            cmd = shared.WASM_OPT + [t, '-all']
            support.run_command(cmd)

        shared.with_pass_debug(check)


def run_crash_tests():
    print("\n[ checking we don't crash on tricky inputs... ]\n")

    for t in shared.get_tests(shared.get_test_dir('crash'), ['.wast', '.wasm']):
        print('..', os.path.basename(t))
        cmd = shared.WASM_OPT + [t]
        # expect a parse error to be reported
        support.run_command(cmd, expected_err='parse exception:', err_contains=True, expected_status=1)


def run_dylink_tests():
    print("\n[ we emit dylink sections properly... ]\n")

    dylink_tests = glob.glob(os.path.join(shared.options.binaryen_test, 'dylib*.wasm'))
    for t in sorted(dylink_tests):
        print('..', os.path.basename(t))
        cmd = shared.WASM_OPT + [t, '-o', 'a.wasm']
        support.run_command(cmd)
        with open('a.wasm', 'rb') as output:
            index = output.read().find(b'dylink')
            print('  ', index)
            assert index == 11, 'dylink section must be first, right after the magic number etc.'


def run_ctor_eval_tests():
    print('\n[ checking wasm-ctor-eval... ]\n')

    for t in shared.get_tests(shared.get_test_dir('ctor-eval'), ['.wast', '.wasm']):
        print('..', os.path.basename(t))
        ctors = open(t + '.ctors').read().strip()
        cmd = shared.WASM_CTOR_EVAL + [t, '-all', '-o', 'a.wat', '-S', '--ctors', ctors]
        if 'ignore-external-input' in t:
            cmd += ['--ignore-external-input']
        if 'results' in t:
            cmd += ['--kept-exports', 'test1,test3']
        support.run_command(cmd)
        actual = open('a.wat').read()
        out = t + '.out'
        shared.fail_if_not_identical_to_file(actual, out)


def run_wasm_metadce_tests():
    print('\n[ checking wasm-metadce ]\n')

    for t in shared.get_tests(shared.get_test_dir('metadce'), ['.wast', '.wasm']):
        print('..', os.path.basename(t))
        graph = t + '.graph.txt'
        cmd = shared.WASM_METADCE + [t, '--graph-file=' + graph, '-o', 'a.wat', '-S', '-all']
        stdout = support.run_command(cmd)
        expected = t + '.dced'
        with open('a.wat') as seen:
            shared.fail_if_not_identical_to_file(seen.read(), expected)
        shared.fail_if_not_identical_to_file(stdout, expected + '.stdout')


def run_wasm_reduce_tests():
    if not shared.has_shell_timeout():
        print('\n[ skipping wasm-reduce testcases]\n')
        return

    print('\n[ checking wasm-reduce testcases]\n')

    # fixed testcases
    for t in shared.get_tests(shared.get_test_dir('reduce'), ['.wast']):
        print('..', os.path.basename(t))
        # convert to wasm
        support.run_command(shared.WASM_AS + [t, '-o', 'a.wasm', '-all'])
        support.run_command(shared.WASM_REDUCE + ['a.wasm', '--command=