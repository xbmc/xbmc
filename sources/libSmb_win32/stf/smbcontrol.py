#!/usr/bin/python
#
# Test for smbcontrol command line argument handling.
#

import comfychair

class NoArgs(comfychair.TestCase):
    """Test no arguments produces usage message."""
    def runtest(self):
        out = self.runcmd("smbcontrol", expectedResult = 1)
        self.assert_re_match("Usage: smbcontrol", out[1])
        
class OneArg(comfychair.TestCase):
    """Test single argument produces usage message."""
    def runtest(self):
        out = self.runcmd("smbcontrol foo", expectedResult = 1)
        self.assert_re_match("Usage: smbcontrol", out[1])
        
class SmbdDest(comfychair.TestCase):
    """Test the broadcast destination 'smbd'."""
    def runtest(self):
        out = self.runcmd("smbcontrol smbd noop")

class NmbdDest(comfychair.TestCase):
    """Test the destination 'nmbd'."""
    def runtest(self):
        # We need a way to start/stop/whatever nmbd
        raise comfychair.NotRunError, "not implemented"

class PidDest(comfychair.TestCase):
    """Test a pid number destination'."""
    def runtest(self):
        out = self.runcmd("smbcontrol 1234 noop")

class SelfDest(comfychair.TestCase):
    """Test the destination 'self'."""
    def runtest(self):
        out = self.runcmd("smbcontrol self noop")

class WinbinddDest(comfychair.TestCase):
    """Test the destination 'winbindd'."""
    def runtest(self):
        # We need a way to start/stop/whatever winbindd
        raise comfychair.NotRunError, "not implemented"

class BadDest(comfychair.TestCase):
    """Test a bad destination."""
    def runtest(self):
        out = self.runcmd("smbcontrol foo noop", expectedResult = 1)

class BadCmd(comfychair.TestCase):
    """Test a bad command."""
    def runtest(self):
        out = self.runcmd("smbcontrol self spottyfoot", expectedResult = 1)
        self.assert_re_match("smbcontrol: unknown command", out[1]);

class NoArgCmdTest(comfychair.TestCase):
    """A test class that tests a command with no argument."""
    def runtest(self):
        self.require_root()
        out = self.runcmd("smbcontrol self %s" % self.cmd)
        out = self.runcmd("smbcontrol self %s spottyfoot" % self.cmd,
                          expectedResult = 1)

class ForceElection(NoArgCmdTest):
    """Test a force-election message."""
    def setup(self):
        self.cmd = "force-election"

class SamSync(NoArgCmdTest):
    """Test a samsync message."""
    def setup(self):
        self.cmd = "samsync"

class SamRepl(NoArgCmdTest):
    """Test a samrepl message."""
    def setup(self):
        self.cmd = "samrepl"

class DmallocChanged(NoArgCmdTest):
    """Test a dmalloc-changed message."""
    def setup(self):
        self.cmd = "dmalloc-log-changed"

class DmallocMark(NoArgCmdTest):
    """Test a dmalloc-mark message."""
    def setup(self):
        self.cmd = "dmalloc-mark"

class Shutdown(NoArgCmdTest):
    """Test a shutdown message."""
    def setup(self):
        self.cmd = "shutdown"

class Ping(NoArgCmdTest):
    """Test a ping message."""
    def setup(self):
        self.cmd = "ping"

class Debuglevel(NoArgCmdTest):
    """Test a debuglevel message."""
    def setup(self):
        self.cmd = "debuglevel"

class OneArgCmdTest(comfychair.TestCase):
    """A test class that tests a command with one argument."""
    def runtest(self):
        self.require_root()
        out = self.runcmd("smbcontrol self %s spottyfoot" % self.cmd)
        out = self.runcmd("smbcontrol self %s" % self.cmd, expectedResult = 1)

class DrvUpgrade(OneArgCmdTest):
    """Test driver upgrade message."""
    def setup(self):
        self.cmd = "drvupgrade"
        
class CloseShare(OneArgCmdTest):
    """Test close share message."""
    def setup(self):
        self.cmd = "close-share"
        
class Debug(OneArgCmdTest):
    """Test a debug message."""
    def setup(self):
        self.cmd = "debug"

class PrintNotify(comfychair.TestCase):
    """Test print notification commands."""
    def runtest(self):

        # No subcommand

        out = self.runcmd("smbcontrol self printnotify", expectedResult = 1)
        self.assert_re_match("Must specify subcommand", out[1]);

        # Invalid subcommand name

        out = self.runcmd("smbcontrol self printnotify spottyfoot",
                          expectedResult = 1)
        self.assert_re_match("Invalid subcommand", out[1]);

        # Queue commands

        for cmd in ["queuepause", "queueresume"]:

            out = self.runcmd("smbcontrol self printnotify %s" % cmd,
                              expectedResult = 1)
            self.assert_re_match("Usage:", out[1])
        
            out = self.runcmd("smbcontrol self printnotify %s spottyfoot"
                              % cmd)

        # Job commands

        for cmd in ["jobpause", "jobresume", "jobdelete"]:

            out = self.runcmd("smbcontrol self printnotify %s" % cmd,
                              expectedResult = 1)
            self.assert_re_match("Usage:", out[1])

            out = self.runcmd("smbcontrol self printnotify %s spottyfoot"
                              % cmd, expectedResult = 1)
            self.assert_re_match("Usage:", out[1])

            out = self.runcmd("smbcontrol self printnotify %s spottyfoot 123"
                              % cmd)

        # Printer properties

        out = self.runcmd("smbcontrol self printnotify printer",
                          expectedResult = 1)
        self.assert_re_match("Usage", out[1])

        out = self.runcmd("smbcontrol self printnotify printer spottyfoot",
                          expectedResult = 1)
        self.assert_re_match("Usage", out[1])

        for cmd in ["comment", "port", "driver"]:

            out = self.runcmd("smbcontrol self printnotify printer spottyfoot "
                              "%s" % cmd, expectedResult = 1)
            self.assert_re_match("Usage", out[1])

            out = self.runcmd("smbcontrol self printnotify printer spottyfoot "
                              "%s value" % cmd)

class Profile(comfychair.TestCase):
    """Test setting the profiling level."""
    def runtest(self):
        self.require_root()
        out = self.runcmd("smbcontrol self profile", expectedResult = 1)
        self.assert_re_match("Usage", out[1])
        
        out = self.runcmd("smbcontrol self profile spottyfoot",
                          expectedResult = 1)
        self.assert_re_match("Unknown", out[1])
        
        for cmd in ["off", "count", "on", "flush"]:
            out = self.runcmd("smbcontrol self profile %s" % cmd)

class ProfileLevel(comfychair.TestCase):
    """Test requesting the current profiling level."""
    def runtest(self):
        self.require_root()
        out = self.runcmd("smbcontrol self profilelevel spottyfoot",
                          expectedResult = 1)
        self.assert_re_match("Usage", out[1])
        
        out = self.runcmd("smbcontrol self profilelevel")
        
class TimeoutArg(comfychair.TestCase):
    """Test the --timeout argument."""
    def runtest(self):
        out = self.runcmd("smbcontrol --timeout 5 self noop")
        out = self.runcmd("smbcontrol --timeout spottyfoot self noop",
                          expectedResult = 1)

class ConfigFileArg(comfychair.TestCase):
    """Test the --configfile argument."""
    def runtest(self):
        out = self.runcmd("smbcontrol --configfile /dev/null self noop")

class BogusArg(comfychair.TestCase):
    """Test a bogus command line argument."""
    def runtest(self):
        out = self.runcmd("smbcontrol --bogus self noop", expectedResult = 1)

tests = [NoArgs, OneArg, SmbdDest, NmbdDest, WinbinddDest, PidDest,
         SelfDest, BadDest, BadCmd, Debug, ForceElection, SamSync,
         SamRepl, DmallocMark, DmallocChanged, Shutdown, DrvUpgrade,
         CloseShare, Ping, Debuglevel, PrintNotify, Profile, ProfileLevel,
         TimeoutArg, ConfigFileArg, BogusArg]

# Handle execution of this file as a main program

if __name__ == '__main__':
    comfychair.main(tests)
