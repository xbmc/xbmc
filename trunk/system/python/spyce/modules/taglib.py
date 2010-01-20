##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import string

__doc__ = '''Spyce tags functionality.'''

class taglib(spyceModule):
  def start(self):
    self.context = {}
    self.stack = []
    self.taglibs = {}
    self._api.registerModuleCallback(self.__syncModules)
    self.__syncModules()
  def __syncModules(self):
    modules = self._api.getModules()
    for name in modules.keys():
      self.context[name] = modules[name]
    self.mod_response = modules['response']
    self.mod_stdout = modules['stdout']
  def finish(self, theError):
    self._api.unregisterModuleCallback(self.__syncModules)
    try:
      for taglib in self.taglibs.keys():
        self.unload(taglib)
    finally:
      del self.context
  # load and unload tag libraries
  def load(self, libname, libfrom=None, libas=None):
    thename = libname
    if libas: thename = libas
    if self.taglibs.has_key(thename):
      raise 'tag library with that name already loaded'
    lib = self._api.spyceTaglib(
      libname, libfrom, self._api.getFilename())(libname)
    lib.start()
    self.taglibs[thename] = lib
  def unload(self, libname):
    lib = None
    try: 
      lib = self.taglibs[libname]
      del self.taglibs[libname]
    except KeyError: pass
    if lib: lib.finish()
  # tag processing
  def tagPush(self, libname, tagname, attr, pair):
    try: parent = self.stack[-1]
    except: parent = None
    tag = self.taglibs[libname].getTag(tagname, attr, pair, parent)
    self.stack.append(tag)
  def tagPop(self):
    self.outPopCond()
    if self.stack: self.stack.pop()
  def getTag(self):
    return self.stack[-1]
  def outPush(self):
    tag = self.stack[-1]
    if tag.buffer:
      tag.setBuffered(1)
      return self.mod_stdout.push()
  def outPopCond(self):
    tag = self.stack[-1]
    if tag.getBuffered():
      tag.setBuffered(0)
      return self.mod_stdout.pop()
  def tagBegin(self):
    tag = self.getTag()
    tag.setContext(self.context)
    tag.setOut(self.mod_response)
    result = apply(tag.begin, (), tag._attrs)
    self.outPush()
    return result
  def tagBody(self):
    contents = self.outPopCond()
    tag = self.getTag()
    tag.setContext(self.context)
    tag.setOut(self.mod_response)
    result = tag.body(contents)
    if result: self.outPush()
    return result
  def tagEnd(self):
    self.outPopCond()
    tag = self.getTag()
    tag.setContext(self.context)
    tag.setOut(self.mod_response)
    return tag.end()
  def tagCatch(self):
    self.outPopCond()
    tag = self.getTag()
    tag.setOut(self.mod_response)
    tag.catch(sys.exc_info()[0])
  def __repr__(self):
    return 'prefixes: %s; stack: %s' % (
      string.join(self.taglibs.keys(), ', '),
      string.join(map(lambda x: x.name, self.stack), ', '))

