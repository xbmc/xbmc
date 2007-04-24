##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
from cStringIO import StringIO
import gzip, string, spyceUtil

__doc__ = '''Compress module provides dynamic compression.'''

OUTPUT_POSITION = 95

class compress(spyceModule):
  def start(self):
    # install compress filter into response module
    self._filter = FilterCompress(self)
    self._api.getModule('response').addFilter(OUTPUT_POSITION, self._filter)
  def finish(self, theError=None):
    if not theError:
      self._filter.close()
  def init(self, gzip=None, spaces=None):
    if gzip: self.gzip()
    if spaces: self.spaces()
  def spaces(self, on=1):
    self._filter.setSpaceMode(on)
  def gzip(self, level=None):
    self._filter.setGZIP(level)

class FilterCompress(Filter):
  def __init__(self, module):
    self._module = module
    self._buf = StringIO()
    self._flushed = 0
    self._space = 0
    self._gzip = None
    self._gzipfile = None
  def writeStatic(self, s):
    self.write(s)
  def writeExpr(self, s, **kwargs):
    self.write(str(s))
  def setSpaceMode(self, on):
    self._space = on
  def setGZIP(self, level):
    if self._flushed:
      raise 'output already flushed'
    encodings = self._module._api.getModule('request').getHeader('Accept-Encoding')
    if not encodings or string.find(encodings, 'gzip')<0:
      return  # ensure the browser can cope
    if level==0:
      self._gzip = None
      self._gzipfile = None
    else:
      self._gzipfile = StringIO()
      if level:
        self._gzip = gzip.GzipFile(mode='w', fileobj=self._gzipfile, compresslevel=level)
      else:
        self._gzip = gzip.GzipFile(mode='w', fileobj=self._gzipfile)
  def write(self, s, *args, **kwargs):
    self._buf.write(s)
  def flushImpl(self, final=0):
    self._flushed = 1
    s = self._buf.getvalue()
    self._buf = StringIO()
    if self._space:
      s = spyceUtil.spaceCompact(s)
    if self._gzip:
      self._gzip.write(s)
      self._gzip.flush()
      if final:
        self._module._api.getModule('response').addHeader('Content-Encoding', 'gzip')
        self._gzip.close()
        self._gzip = None
      s = self._gzipfile.getvalue()
      self._gzipfile.truncate(0)
    self.next.write(s)
  def clearImpl(self):
    self._buf = StringIO()
  def close(self):
    self.flushImpl(1)

