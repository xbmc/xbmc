#!/usr/bin/env python

##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = '''Version checking spyceCGI.py wrapper.'''

script = 'spyceCGI.py'

import sys, os, verchk

if __name__ == '__main__':
  spycePath = os.path.abspath(os.path.dirname(sys.modules['verchk'].__file__))
  sys.argv[0] = os.path.join(spycePath, script)
  sys.argv.insert(0, os.path.join(spycePath, 'verchk.py'))
  execfile(sys.argv[0])
