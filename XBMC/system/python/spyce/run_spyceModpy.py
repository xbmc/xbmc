#!/usr/bin/env python

##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = '''Version checking spyceModpy.py wrapper.'''

try:
  import _apache
  from mod_python import apache
except: pass

def spyceMain(apacheRequest):
  return spyceMainVersion(apacheRequest)

def spyceMainVersion(apacheRequest):
  "Version checking Apache entry point."
  import verchk
  if not verchk.checkversion(verchk.REQUIRED):
    import sys
    apacheRequest.content_type = 'text/plain'
    apacheRequest.send_http_header()
    apacheRequest.write('Spyce can not run on this version of Python.\n')
    apacheRequest.write('Python version '+sys.version[:3]+' detected.\n')
    apacheRequest.write('Python version '+verchk.REQUIRED+' or greater required.\n')
    try:
      return apache.OK
    except: pass
  else:
    global spyceMain
    import spyceModpy
    spyceMain = spyceModpy.spyceMain
    return spyceModpy.spyceMain(apacheRequest)

if __name__ == '__main__':
  print "********** ERROR: **********"
  print "This program can not be run from the command-line."
  print "Use run_spyceCmd.py, or run via Apache."
  print "For configuring Apache, have a look at 'spyceApache.conf'."
  print
  print "Also, please read the documentation at:"
  print "  http://spyce.sourceforge.net"
  print "for other options."

