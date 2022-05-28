import os

from scripts.test import shared
from . import utils


class FeatureValidationTest(utils.BinaryenTestCase):
    def check_feature(self, module, error, flag, const_flags=[]):
        p = shared.run_process(shared.WASM_OPT +
                               ['--mvp-features', '--print', '-o', os.devnull] +
                               const_flags,
                               input=module, check=False, capture_output=True)
        self.assertIn(error, p.stderr)
        self.assertIn('Fatal: error validating input', p.stderr)
        self.assertNotEqual(p.returncode, 0)
        p