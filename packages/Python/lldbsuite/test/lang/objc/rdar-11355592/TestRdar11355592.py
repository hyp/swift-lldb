"""
Test that we do not attempt to make a dynamic type for a 'const char*'
"""

from __future__ import print_function



import os, time
import lldb
from lldbsuite.test.lldbtest import *
import lldbsuite.test.lldbutil as lldbutil

@skipUnlessDarwin
class Rdar10967107TestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # We'll use the test method name as the exe_name.
        self.exe_name = self.testMethodName
        # Find the line number to break inside main().
        self.main_source = "main.m"
        self.line = line_number(self.main_source, '// Set breakpoint here.')

    def test_charstar_dyntype(self):
        """Test that we do not attempt to make a dynamic type for a 'const char*'"""
        d = {'EXE': self.exe_name}
        self.build(dictionary=d)
        self.setTearDownCleanup(dictionary=d)

        exe = os.path.join(os.getcwd(), self.exe_name)
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        lldbutil.run_break_set_by_file_and_line (self, self.main_source, self.line, num_expected_locations=1, loc_exact=True)

        self.runCmd("run", RUN_SUCCEEDED)
        # check that we correctly see the const char*, even with dynamic types on
        self.expect("frame variable my_string", substrs = ['const char *'])
        self.expect("frame variable my_string --dynamic-type run-target", substrs = ['const char *'])
        # check that expr also gets it right
        self.expect("expr my_string", substrs = ['const char *'])
        self.expect("expr -d run -- my_string", substrs = ['const char *'])
        # but check that we get the real Foolie as such
        self.expect("frame variable my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("frame variable my_foolie --dynamic-type run-target", substrs = ['FoolMeOnce *'])
        # check that expr also gets it right
        self.expect("expr my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("expr -d run -- my_foolie", substrs = ['FoolMeOnce *'])
        # now check that assigning a true string does not break anything
        self.runCmd("next")
        # check that we correctly see the const char*, even with dynamic types on
        self.expect("frame variable my_string", substrs = ['const char *'])
        self.expect("frame variable my_string --dynamic-type run-target", substrs = ['const char *'])
        # check that expr also gets it right
        self.expect("expr my_string", substrs = ['const char *'])
        self.expect("expr -d run -- my_string", substrs = ['const char *'])
        # but check that we get the real Foolie as such
        self.expect("frame variable my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("frame variable my_foolie --dynamic-type run-target", substrs = ['FoolMeOnce *'])
        # check that expr also gets it right
        self.expect("expr my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("expr -d run -- my_foolie", substrs = ['FoolMeOnce *'])
