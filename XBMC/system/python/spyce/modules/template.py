##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import spyceException, spyceCache
import os

__doc__ = """Template module provides templating functionality: the ability to separate
form from function, or HTML page design from programming code. This module
currently provides support for the Cheetah template engine.
"""

class template(spyceModule):
  def cheetah(self, filename, lookup=None):
    "Hook into the Cheetah template engine."
    # check whether cheetah installed
    from Cheetah.Compiler import Compiler
    # define template cache
    if not self._api.getModule('pool').has_key('cheetahCache'):
      self._api.getModule('pool')['cheetahCache'] = spyceCache.semanticCache(spyceCache.memoryCache(), cheetahValid, cheetahGenerate)
    cheetahCache = self._api.getModule('pool')['cheetahCache']
    # absolute filename, relative to script filename
    filename = os.path.abspath(os.path.join(
        os.path.dirname(self._api.getFilename()), filename))
    # set lookup variables
    if lookup == None:
      import inspect
      lookup = [inspect.currentframe().f_back.f_locals, inspect.currentframe().f_back.f_globals]
    elif type(lookup)!=type([]):
      lookup = [lookup]
    # compile (or get cached) and run template
    return cheetahCache[filename](searchList=lookup)

##################################################
# Cheetah semantic cache helper functions
#

def cheetahValid(filename, validity):
  try:
    return os.path.getmtime(filename) == validity
  except: return 0

def cheetahGenerate(filename):
  # check permissions
  if not os.path.exists(filename):
    raise spyceException.spyceNotFound()
  if not os.access(filename, os.R_OK):
    raise spyceException.spyceForbidden()
  # read the template
  f = None
  try:
    f = open(filename, 'r')
    buf = f.read()
  finally:
    if f: f.close()
  # compile template, get timestamp
  mtime = os.path.getmtime(filename)
  from Cheetah.Compiler import Compiler
  code = Compiler(source=buf).__str__()
  dict = {}
  exec code in dict
  return mtime, dict['GenTemplate']

