#! /usr/bin/env python
"""

Visual Studio 2005 Build Script

$Id: VsMake.py 655 2010-09-29 22:40:22Z soothe $

"""

import os
import sys
import getopt
import subprocess

# Setup some path stuff
try:
    if environ['VISUALSTUDIO_BIN']:
        VSBINDIR = environ['VISUALSTUDIO_BIN']
except NameError:
    # Use default value for visual studio.
    VSBINDIR = 'C:/Program Files/Microsoft Visual Studio 8/Common7/IDE'
    print 'VISUALSTUDIO_BIN not set. Trying default value:'
    print '    ' + VSBINDIR
    print ''


# ------------------------------------------------------------
#       usage
# ------------------------------------------------------------
def usage(errMsg):
    try:
        print 'Error: %s' % (errMsg)
    except NameError:
        pass

    print 'Usage: '
    print '  %s -s <path to solution> -b [Release|Debug|etc.]' % (sys.argv[0])
    print ''
    print '    REQUIRED OPTIONS'
    print '\t-s <solution>'
    print '\t-b <configuration>'
    print ''
    print '    BUILD OPTIONS'
    print '\t-c\tMake clean'
    print '\t-r\tRe-build all'


# ------------------------------------------------------------
#       main
# ------------------------------------------------------------
try:
    opts, args = getopt.getopt(sys.argv[1:], "s:b:rc")
except getopt.GetoptError, (msg, opt):
#    print 'Error: invalid argument, %s: %s' % (opt, msg)
    usage('invalid argument, %s: %s' % (opt, msg))
    sys.exit(2)

# Build options
doingBuild = False
rebuildAll = False
makeClean = False

for opt, arg in opts:
    if opt == '-s':
        solutionFile = arg
        doingBuild = True
    elif opt == '-b':
        buildName = arg
        doingBuild = True
    elif opt == '-r':
        rebuildAll = True
        doingBuild = True
    elif opt == '-c':
        makeClean = True

if rebuildAll and makeClean:
    usage('Error cannot specify -c and -r together')
    sys.exit(2)

try:
    buildSwitch = 'build'
    if rebuildAll: buildSwitch = 'rebuild'
    elif makeClean: buildSwitch = 'clean'
        
    cmd_list = ['%s/devenv.com' % VSBINDIR, '/%s' % buildSwitch, buildName, solutionFile]
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
