installScript = '''
#!/usr/bin/python
# Aptitude script for XBMC
#
# LICENSE - See the LICENSE file that came with this module
#
# Copyright 2008 by Tobias Arrskog <topfs2@xbmc.org>

import sys
sys.path.append("/usr/lib/python2.5/site-packages") 
from apt.progress import *
import apt_pkg
import apt

class TerminalInstallProgress(InstallProgress):
    def __init__(self):
        InstallProgress.__init__(self)
        self.line1 = "Installing and configurating the necessary packages"
        self.line2 = " "
        self.line3 = " "
    def startUpdate(self):
        InstallProgress.startUpdate(self)
        print "Start;%s;%s;%s" % (self.line1, self.line2, self.line3)
    def statusChange(self, pkg, percent, status):
        self.line2 = pkg
    def updateInterface(self):
        InstallProgress.updateInterface(self)
        print "Progress;%f;%s;%s;%s" % (self.percent, self.line1, self.line2, self.line3)
    def finishUpdate(self):
        InstallProgress.finishUpdate(self)
        print "Finish"
        

class TerminalFetchProgress(FetchProgress):
    def __init__(self):
        self.line1 = "Fetching the necessary packages"
        self.line2 = " "
        self.line3 = " "
    
    def start(self):
        print "Start;%s;%s;%s" % (self.line1, self.line2, self.line3)
    
    def stop(self):
        print "Finish"
    
    def updateStatus(self, uri, descr, shortDescr, status):
        self.line2 = shortDescr
    def pulse(self):
        FetchProgress.pulse(self)
        print "Progress;%f;%s;%s;%s" % (self.percent, self.line1, self.line2, self.line3)
        return True

    def mediaChange(self, medium, drive):
	print "Please insert medium %s in drive %s" % (medium, drive)
	sys.stdin.readline()
        #return False

class TerminalProgress(OpProgress):
    def __init__(self):
        self.line1 = "Reading the packagecache"
        self.line2 = " "
        self.line3 = " "
        print "Start;%s;%s;%s" % (self.line1, self.line2, self.line3)

    def update(self, percent):
        print "Progress;%f;%s;%s;%s" % (percent, self.line1, self.line2, self.line3)
    def done(self):
        print "Finish"     


apt_pkg.init()

c = apt.Cache(TerminalProgress())
c.upgrade()
changes = c.getChanges()


fprogress = TerminalFetchProgress()
iprogress = TerminalInstallProgress()

res = c.commit(fprogress, iprogress)

print "Complete;%s" % str(res)
'''
