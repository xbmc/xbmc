#! /usr/bin/env python
"""

XCode Build Script

$Id: XCodeMake.py 655 2010-09-29 22:40:22Z soothe $

"""

import os
import sys
import getopt
import subprocess


# ------------------------------------------------------------
#       usage
# ------------------------------------------------------------
def usage(errMsg):
    try:
        print 'Error: %s' % (errMsg)
    except NameError:
        pass

    print 'Usage: '
    print '  %s -p <path to project> -b [Release|Debug|etc.] -t [All|Platinum|PlatinumFramework|etc.] -s [macosx|iphoneos]' % (sys.argv[0])
    print ''
    print '    REQUIRED OPTIONS'
    print '\t-p <project>'
    print '\t-b <configuration>'
    print '\t-t <target>'
    print '\t-s <sdk>'
    print ''
    print '    BUILD OPTIONS'
    print '\t-c\tMake clean'


# ------------------------------------------------------------
#       main
# ------------------------------------------------------------
try:
    opts, args = getopt.getopt(sys.argv[1:], "p:b:t:s:c")
except getopt.GetoptError, (msg, opt):
#    print 'Error: invalid argument, %s: %s' % (opt, msg)
    usage('invalid argument, %s: %s' % (opt, msg))
    sys.exit(2)

# Build options
doingBuild = False
rebuildAll = False
makeClean = False

for opt, arg in opts:
    if opt == '-p':
        projectFile = arg
        doingBuild = True
    elif opt == '-b':
        buildName = arg
        doingBuild = True
    elif opt == '-t':
        targetName = arg
    elif opt == '-s':
        sdk = arg
    elif opt == '-c':
        makeClean = True

try:
    buildSwitch = 'build'
    if makeClean: buildSwitch = 'clean'
        
    cmd_list = ['xcodebuild', '-project', '%s' % projectFile, '-target', '%s' % targetName, '-sdk', '%s' % sdk, '-configuration', '%s' % buildName, '%s' % buildSwitch]
    cmd = " ".join(cmd_list)
    print 'Executing:'
    print cmd
    retVal = subprocess.call(cmd_list)
    # only the least sig 8 bits are the real return value
    if retVal != 0:
        print cmd
        print '** BUILD FAILURE **'
        sys.exit(retVal)
except NameError, (name):
    usage('missing argument %s' % (name))
    sys.exit(2)
