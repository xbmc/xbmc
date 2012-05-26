#! /usr/bin/env python

import os
import sys
import getopt
import subprocess


configs = ['Debug', 'Release']
solutions = ['../../../Build/Targets/x86-microsoft-win32-vs2008/Platinum.sln']

try:
    opts, args = getopt.getopt(sys.argv[1:], "b:rc")
except getopt.GetoptError, (msg, opt):
    print 'No build_config, defaulting to build all'
    
for opt, arg in opts:
    if opt == '-b':
        config = arg
        
        
def CallVsMake(sln, cfg):
    cmd = 'python VsMake.py -s %s -b %s' % (sln, cfg)
    print cmd
    retVal = subprocess.call(cmd.split())
    if retVal != 0:
        sys.exit(retVal)  
      
for sln in solutions:
    if 'config' not in locals() and 'config' not in globals():
        print '************ Building all configurations **************'
        for cfg in configs:
            CallVsMake(sln, cfg)
    else:
        print '************ Building configuration=' + config + ' ****************'
        CallVsMake(sln, config)
		    		