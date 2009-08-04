##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import os, re

__doc__ = """Include module is used to assist the inclusion of abitrary
elements/files into a Spyce file. It can also support the notion of an
'inclusion context'."""

class include(spyceModule):
  def start(self):
    self.context = None
    self.vars = None
    self.fromFile = None
  def spyce(self, file, context=None):
    "Include a Spyce file"
    file = os.path.join(os.path.dirname(self._api.getFilename()), file)
    result = s = code = None
    try:
      code = self._api.spyceFile(file)
      s = code.newWrapper()
      modules = self._api.getModules()
      for name in modules.keys():
        s.setModule(name, modules[name])  # include module as well!
      s.spyceInit(self._api.getRequest(), self._api.getResponse())
      incmod = s._startModule('include', None, None, 1)
      incmod.context = context
      if type(context)==type({}):
        incmod.vars = spyceVars(context)
      incmod.fromFile = self._api.getFilename()
      result = s.spyceProcess()
    finally:
      if s:
        s.spyceDestroy()
        code.returnWrapper(s)
    return result
  def spyceStr(self, file, context=None):
    stdout = self._api.getModule('stdout')
    stdout.push()
    try:
      result = self.spyce(file, context)
    finally:
      output = stdout.pop()
    return output
  def dump(self, file, binary=0):
    "Include a plain text file, verbatim"
    file = os.path.join(os.path.dirname(self._api.getFilename()), file)
    f = None
    try:
      if binary: mode='rb'
      else: mode='r'
      f = open(file, mode)
      buf = f.read()
    finally:
      if f: f.close()
    return buf
  def spycecode(self, file=None, string=None, html=None, code=None, eval=None, directive=None, comment=None):
    "Emit formatted Spyce code"
    if not html: html = ('<font color="#000000"><b>', '</b></font>')
    if not code: code = ('<font color="#0000CC">', '</font>')
    if not eval: eval = ('<font color="#CC0000">', '</font>')
    if not directive: directive = ('<font color="#CC00CC">', '</font>')
    if not comment: comment = ('<font color="#777777">', '</font>')
    import spyceCompile
    from StringIO import StringIO
    if (file and string) or (not file and not string):
      raise 'must specify either file or string, and not both'
    if file:
      f = None
      try:
        file = os.path.join(os.path.dirname(self._api.getFilename()), file)
        f = open(file, 'r')
        string = f.read()
      finally:
        if f: f.close()
    html_encode = self._api.getModule('transform').html_encode
    try:
      tokens = spyceCompile.spyceTokenize(string)
      buf = StringIO()
      markupstack = []
      buf.write(html[0]); markupstack.append(html[1])
      for type, text, _, _ in tokens:
        if type in (spyceCompile.T_STMT, spyceCompile.T_CHUNK, spyceCompile.T_CHUNKG,):
          buf.write(code[0]); markupstack.append(code[1])
        if type in (spyceCompile.T_LAMBDA,):
          buf.write(html[0]); markupstack.append(html[1])
        if type in (spyceCompile.T_EVAL,):
          buf.write(eval[0]); markupstack.append(eval[1])
        if type in (spyceCompile.T_DIRECT,):
          buf.write(directive[0]); markupstack.append(directive[1])
        if type in (spyceCompile.T_CMNT,):
          buf.write(comment[0]); markupstack.append(comment[1])
        buf.write(html_encode(text))
        if type in (spyceCompile.T_END_CMNT, spyceCompile.T_END,):
          buf.write(markupstack.pop())
      while markupstack:
        buf.write(markupstack.pop())
      return buf.getvalue()
    except:
      raise
      raise 'error tokenizing!'

class spyceVars:
  def __init__(self, vars):
    self.__dict__['vars'] = vars
  def __getattr__(self, name):
    try: return self.__dict__['vars'][name]
    except KeyError: raise AttributeError
  def __setattr__(self, name, value):
    self.__dict__['vars'][name] = value

