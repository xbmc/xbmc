#!/usr/bin/env python

##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = '''Version checking script.'''

import sys, os

REQUIRED = '1.5'

def checkversion(required):
  if int(sys.version[0])<int(required[0]) or \
     (sys.version[0]==required[0] and int(sys.version[2])<int(required[2])):
    return 0
  return 1

if __name__ == "__main__":
  if not checkversion(REQUIRED):
    print 'Python version '+REQUIRED+' required.'
    sys.exit(-1)
  if len(sys.argv)<2:
    print 'Python version '+sys.version[:3]+' - OK'
  else:
    #sys.argv[1] = os.path.join(os.path.dirname(sys.argv[0]), sys.argv[1])
    del sys.argv[0]
    if not os.path.exists(sys.argv[0]):
      print 'Script "'+sys.argv[0]+'" not found.'
      sys.exit(-1)
    execfile(sys.argv[0])

