##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import string

__doc__ = """spylambda module produces functions from spyce strings."""

class spylambda(spyceModule):
  def define(self, sig, code, memoize=0):
    # compile spyce to cache errors early
    spycecode = self._api.spyceString((code, sig))
    if string.strip(sig): sigcomma = sig + ','
    def processSpyce(args, kwargs, self=self, spycecode=spycecode):
      s = None
      try:
        s = spycecode.newWrapper()
        modules = self._api.getModules()
        for name in modules.keys():
          s.setModule(name, modules[name])
        s.spyceInit(self._api.getRequest(), self._api.getResponse())
        result = apply(s.spyceProcess, args, kwargs)
      finally:
        if s:
          s.spyceDestroy()
          spycecode.returnWrapper(s)
      return result
    if memoize:
      def memoizer(f, id, stdout=self._api.getModule('stdout')):
        def memoized(args, kwargs, f=f, id=id, stdout=stdout):
          key = id, `args, kwargs`
          try: r, s = stdout.memoizeCache[key]
          except:
            r, s = stdout.memoizeCache[key] = apply(stdout.capture, (f, args, kwargs))
          print s
          return r
        return memoized
      processSpyce = memoizer(processSpyce, code)
    def makeArgProcessor(f):
      dict = { 'f': f }
      exec '''
def processArg(*args, **kwargs):
  return f(args, kwargs)
''' in dict
      return dict['processArg']
    return makeArgProcessor(processSpyce)
  def __call__(self, sig, code, memoize=0):
    return self.define(sig, code)
  def __repr__(self):
    return ''
