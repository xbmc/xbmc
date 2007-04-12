##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = '''Spyce modules functionality.'''

##################################################
# Spyce module
#

class spyceModule:
  "All Spyce module should subclass this."
  def __init__(self, wrapper):
    self._api = wrapper
  def start(self):
    pass
  def finish(self, theError=None):
    pass
  def init(self, *args, **kwargs):
    pass
  def __repr__(self):
    return 'no information, '+str(self.__class__)

class spyceModulePlus(spyceModule):
  def __init__(self, wrapper):
    spyceModule.__init__(self, wrapper)
    self.wrapper = self._api  # deprecated
    self.modules = moduleFinder(wrapper)
    self.globals = wrapper.getGlobals()

class moduleFinder:
  def __init__(self, wrapper):
    self._wrapper = wrapper
  def __getattr__(self, name):
    return self._wrapper.getModule(name)

##################################################
# Spyce module API
#

spyceModuleAPI = [ 'getFilename', 'getCode', 
  'getCodeRefs', 'getModRefs', 
  'getServerObject', 'getServerGlobals', 'getServerID',
  'getModules', 'getModule', 'setModule', 'getGlobals', 
  'registerModuleCallback', 'unregisterModuleCallback',
  'getRequest', 'getResponse', 'setResponse',
  'registerResponseCallback', 'unregisterResponseCallback', 
  'spyceString', 'spyceFile', 'spyceModule', 'spyceTaglib',
  'setStdout', 'getStdout',
]

