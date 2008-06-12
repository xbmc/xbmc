#!/usr/bin/python

# meta-test-case / example for comfychair.  Should demonstrate
# different kinds of failure.

import comfychair

class NormalTest(comfychair.TestCase):
    def runtest(self):
        pass

class RootTest(comfychair.TestCase):
    def setup(self):
        self.require_root()
            
    def runTest(self):
        pass

class GoodExecTest(comfychair.TestCase):
    def runtest(self):
        stdout = self.runcmd("ls -l")

class BadExecTest(comfychair.TestCase):
    def setup(self):
        exit, stdout = self.runcmd_unchecked("spottyfoot --slobber",
                                             skip_on_noexec = 1)


tests = [NormalTest, RootTest, GoodExecTest, BadExecTest]

if __name__ == '__main__':
    comfychair.main(tests)
    
