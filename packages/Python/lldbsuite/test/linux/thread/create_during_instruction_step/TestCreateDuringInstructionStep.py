"""
This tests that we do not lose control of the inferior, while doing an instruction-level step
over a thread creation instruction.
"""

from __future__ import print_function



import os
import lldb
from lldbsuite.test.lldbtest import *
import lldbsuite.test.lldbutil as lldbutil

class CreateDuringInstructionStepTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)

    @skipUnlessPlatform(['linux'])
    @expectedFailureAndroid('llvm.org/pr24737', archs=['arm'])
    def test_step_inst(self):
        self.build(dictionary=self.getBuildFlags())
        exe = os.path.join(os.getcwd(), "a.out")
        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target and target.IsValid(), "Target is valid")

        # This should create a breakpoint in the stepping thread.
        breakpoint = target.BreakpointCreateByName("main")
        self.assertTrue(breakpoint and breakpoint.IsValid(), "Breakpoint is valid")

        # Run the program.
        process = target.LaunchSimple(None, None, self.get_process_working_directory())
        self.assertTrue(process and process.IsValid(), PROCESS_IS_VALID)

        # The stop reason of the thread should be breakpoint.
        self.assertEqual(process.GetState(), lldb.eStateStopped, PROCESS_STOPPED)

        threads = lldbutil.get_threads_stopped_at_breakpoint(process, breakpoint)
        self.assertEqual(len(threads), 1, STOPPED_DUE_TO_BREAKPOINT)

        thread = threads[0]
        self.assertTrue(thread and thread.IsValid(), "Thread is valid")

        # Make sure we see only one threads
        self.assertEqual(process.GetNumThreads(), 1, 'Number of expected threads and actual threads do not match.')

        # Keep stepping until we see the thread creation
        while process.GetNumThreads() < 2:
            thread.StepInstruction(False)
            self.assertEqual(process.GetState(), lldb.eStateStopped, PROCESS_STOPPED)
            self.assertEqual(thread.GetStopReason(), lldb.eStopReasonPlanComplete, "Step operation succeeded")
            if self.TraceOn():
                self.runCmd("disassemble --pc")

        if self.TraceOn():
            self.runCmd("thread list")

        # We have successfully caught thread creation. Now just run to completion
        process.Continue()

        # At this point, the inferior process should have exited.
        self.assertEqual(process.GetState(), lldb.eStateExited, PROCESS_EXITED)
