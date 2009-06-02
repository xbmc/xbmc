##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import sys, re, string
from cStringIO import StringIO

__doc__ = '''Spyce utility functions'''

##################################################
# Current exception string
#

def exceptionString():
  "Generate string out of current exception."
  import traceback, string
  ex=sys.exc_info()
  ex=traceback.format_exception(ex[0], ex[1], ex[2])
  ex=string.join(ex, '')
  return ex

##################################################
# Return hashtable value, or entire hashtable
#

def extractValue(hash, key, default=None):
  """Extract value from dictionary, if it exists. 
  If key is none, return entire dictionary"""
  if key==None: return hash
  if hash.has_key(key): return hash[key]
  return default

##################################################
# Return hashtable value, or entire hashtable
#

RE_SPACE_REDUCE = re.compile('[ \t][ \t]+')
RE_SPACE_NEWLINE_REDUCE = re.compile('\n\s+')
def spaceCompact(text):
  text = string.split(text, '\n')
  text = map(lambda s: RE_SPACE_REDUCE.sub(' ', s), text)
  text = string.join(text, '\n')
  text = RE_SPACE_NEWLINE_REDUCE.sub('\n', text)
  return text

##################################################
# Threading helpers
#

class ThreadedWriter:
  '''Thread-safe writer'''
  def __init__(self, o=None):
    try: import thread,threading
    except: raise 'threading not supported!'
    self.__dict__['_currentThread'] = threading.currentThread
    self.__dict__['_o'] = o
  def setObject(self, o=None):
    self._currentThread().threadOut = o
    self._currentThread().threadWrite = o.write
  def getObject(self):
    try: return self._currentThread().threadOut
    except AttributeError: return self._o
  def clearObject(self):
    try: del self._currentThread().threadOut
    except AttributeError: pass
  def write(self, s):
    try: self._currentThread().threadWrite(s)
    except AttributeError: self._o.write(s)
  def close(self):
    self.getObject().close()
  def flush(self):
    self.getObject().flush()
  def __getattr__(self, name):
    if name=='softspace':  # performance
      return self.getObject().softspace
    return eval('self.getObject().%s'%name)
  def __setattr__(self, name, value):
    if name=='softspace': # performance
      self.getObject().softspace = value
    eval('self.getObject().%s=value'%name)
  def __delattr__(self, name):
    return eval('del self.getObject().%s'%name)

##################################################
# Output
#

class BufferedOutput:
  "Buffered output stream."
  def __init__(self, out):
    self.buf = StringIO()
    self.writeBuf = self.buf.write
    self.out = out
    self.closed = 0
  def write(self, s):
    if self.closed:
      raise 'output stream closed'
    self.writeBuf(s)
  def clear(self):
    if not self.buf:
      raise 'stream is not buffered'
    self.buf = StringIO()
    self.writeBuf = self.buf.write
  def flush(self, stopFlag=0):
    if stopFlag: return
    if self.buf and self.buf.getvalue():
      self.out.write(self.buf.getvalue())
      self.out.flush()
      self.clear()
  def close(self):
    if self.closed:
      raise 'output stream closed'
    self.closed = 1
    self.flush()
    self.out.close()
  def unbuffer(self):
    "Turn this into a pass-through."
    if self.buf:
      self.flush()
      self.buf = None
      self.writeBuf = self.out.write
  def getOut(self):
    "Return underlying output stream."
    return self.out


class NoCloseOut:
  def __init__(self, out):
    self.out = out
    self.write = self.out.write
    self.flush = self.out.flush
  def close(self):
    pass
  def getOut(self):
    return self.out

def panicOutput(response, s):
  import cgi
  # output to browser, if possible
  try: response.clear()
  except: pass
  try:
    response.write('<html><pre>\n')
    response.write('Spyce Panic!<br>\n')
    response.write(cgi.escape(s))
    response.write('</pre></html>\n')
    response.returncode = response.RETURN_OK
    response.flush()
  except: pass
  # output to error log
  sys.stderr.write(s)
  sys.stderr.flush()
  sys.exit(1)
