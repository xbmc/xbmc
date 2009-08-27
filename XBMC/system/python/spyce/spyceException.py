##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = '''Various Spyce-related exceptions'''

import sys, string
import spyceCompile, spyceUtil

##################################################
# Syntax errors
#

class pythonSyntaxError:
  "Generate string out of current pythonSyntaxError exception"
  def __repr__(self):
    return self.str
  def __init__(self, spycewrap):
    self.str = ''
    type, error, _ = sys.exc_info()
    if type is type(SyntaxError):
      raise 'instantiate pythonSyntaxError only when SyntaxError raised: %s' % `type`
    if spycewrap.getCodeRefs().has_key(error.lineno):
      begin, end, text, filename = spycewrap.getCodeRefs()[error.lineno]
      if begin[0]==end[0]:
        linestr = str(begin[0])
      else:
        linestr = '%d-%d' % (begin[0], end[0])
      self.str = 'Python syntax error at %s:%s - %s\n  %s\n' % (filename, linestr, error.msg, text)
    else:
      self.str = spyceUtil.exceptionString()

class spyceSyntaxError:
  "Generate string out of current spyceSyntaxError exception"
  def __init__(self, msg, info=None):
    self.msg = msg
    self.info = info
  def __repr__(self):
    s = 'Spyce syntax error'
    if self.info:
      (begin, _), (end, _), text, filename = self.info
      if begin==end:
        linestr = str(begin)
      else:
        linestr = '%d-%d' % (begin, end)
      s = s + ' at %s:%s - %s\n  %s\n' % (filename, linestr, self.msg, text)
    else:
      s = s + ': '+self.msg
    return s

##################################################
# Runtime errors
#

class spyceRuntimeException:
  "Generate string out of current SpyceException exception."
  # useful fields: str, type, value, traceback, msg
  def __repr__(self):
    return self.str
  def __init__(self, spycewrap=None):
    import traceback, string
    e1, e2, tb = sys.exc_info()
    tb = traceback.extract_tb(tb)
    self.str = ''
    self.type, self.value, self.traceback = e1, e2, tb
    if e1 == spyceRuntimeException:
      self.msg = str(e2)
    else:
      self.msg = string.join(traceback.format_exception_only(e1, e2))
    for i in range(len(tb)):
      filename, lineno, funcname, text = tb[i]
      if filename == '<string>' and spycewrap and spycewrap.getCodeRefs().has_key(lineno):
        if funcname == spyceCompile.SPYCE_PROCESS_FUNC:
          funcname = '(main)'
        begin, end, text, filename = spycewrap.getCodeRefs()[lineno]
        if begin[0]==end[0]:
          lineno = str(begin[0])
        else:
          lineno = '%d-%d' % (begin[0], end[0])
      lineno=str(lineno)
      tb[i] = filename, lineno, funcname, text
    for i in range(len(tb)):
      self.str = self.str + '  %s:%s, in %s: \n    %s\n' % tb[i]
    self.str = self.str + self.msg

class spyceNotFound:
  "Exception class to signal that Spyce file does not exist."
  def __init__(self, file):
    self.file = file
  def __repr__(self):
    return 'spyceNotFound exception: could not find "%s"' % self.file

class spyceForbidden:
  "Exception class to signal that Spyce file has access problems."
  def __init__(self, file):
    self.file = file
  def __repr__(self):
    return 'spyceForbidden exception: could not read "%s"' % self.file

##################################################
# Special control-flow exceptions
#

class spyceRedirect:
  "Exception class to signal an internal redirect."
  def __init__(self, filename):
    self.filename = filename

class spyceDone:
  "Exception class to immediately jump to the end of the spyceProcess method"
  pass

