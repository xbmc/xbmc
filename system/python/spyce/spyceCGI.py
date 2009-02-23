#!/usr/bin/env python

##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import os, sys
import spyceCmd, spyce
import fcgi

__doc__ = '''(F)CGI-based Spyce entry point.'''

def findScriptFile(path):
  origpath = path
  while path and not path=='/':
    if os.path.isfile(path):
      return path
    path = os.path.dirname(path)
  return origpath

def doSpyce( (stdin, stdout, stderr, environ) ):
  path = None
  if len(sys.argv)<=1 or not os.path.isfile(sys.argv[1]):
    try: path = findScriptFile(environ['PATH_TRANSLATED'])
    except: pass
  result = spyceCmd.spyceMain(cgimode=1, cgiscript=path,
    stdout=stdout, stdin=stdin, stderr=stderr, environ=environ)
  return result

def main():
  cgi = fcgi.FCGI()
  more = cgi.accept()
  if cgi.socket: os.environ[spyce.SPYCE_ENTRY] = 'fcgi'
  else: os.environ[spyce.SPYCE_ENTRY] = 'cgi'
  while more:
    doSpyce(more)
    more = cgi.accept()

if __name__=='__main__':
  if sys.platform == "win32":
    import os, msvcrt
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
  main()

