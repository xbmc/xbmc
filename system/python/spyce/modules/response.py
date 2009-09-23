##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import string, time

__doc__ = '''Response module provides user control over the browser
response.'''

class response(spyceModule):
  def start(self):
    self.clearFilters()
    self._unbuffer = 0
    self._ioerror = 0
    self._api.registerResponseCallback(self.syncResponse)
    self.syncResponse()
  def syncResponse(self):
    self._response = self._api.getResponse()
  def finish(self, theError=None):
    self._api.unregisterResponseCallback(self.syncResponse)
    if not theError:
      self._filter.flush(1)
  def clearFilters(self):
    self._filter = FilterUnify(self)
    self._filterList = [(99, self._filter)]
  def addFilter(self, level, filter):
    'Inject filter functions into output stream at given level of precedence'
    filterExists = None
    for i in range(len(self._filterList)):
      l, _ = self._filterList[i]
      if l==level:
        _, filterExists = self._filterList[i]
        del self._filterList[i]
        break
    if filter:
      self._filterList.append((level, filter))
      self._filterList.sort()
    for i in range(len(self._filterList)-1):
      l1, f1 = self._filterList[i]
      l2, f2 = self._filterList[i+1]
      f1.setNext(f2)
    _, self._filter = self._filterList[0]
    return filterExists

  # user functions
  def write(self, s):
    "Write out a dynamic (code) string."
    try:
      self._filter.write(s)
      if self._unbuffer: self.flush()
    except IOError:
      self._ioerror = 1
  def writeln(self, s):
    "Writeln out a dynamic (code) string."
    self.write(s+'\n')
  def writeStatic(self, s):
    "Write out a static string."
    try:
      self._filter.writeStatic(s)
      if self._unbuffer: self.flush()
    except IOError:
      self._ioerror = 1
  def writeExpr(self, s, **kwargs):
    "Write out an expression result."
    try:
      apply(self._filter.writeExpr, (s,), kwargs)
      if self._unbuffer: self.flush()
    except IOError:
      self._ioerror = 1
  def clear(self):
    "Clear the output buffer. (must not be unbuffered)"
    self._filter.clear()
  def flush(self, stopFlag=0):
    "Flush resident buffer."
    try:
      self._filter.flush(stopFlag)
    except IOError:
      self._ioerror = 1
  def setContentType(self, ct):
    "Set document content type. (must not be unbuffered)"
    self._response.setContentType(ct)
  def setReturnCode(self, code):
    "Set HTTP return (status) code"
    self._response.setReturnCode(int(code))
  def isCancelled(self):
    return self._ioerror
  def addHeader(self, type, data, replace=0):
    "Add an HTTP header. (must not be unbuffered)"
    if string.find(type, ':') != -1:
      raise 'HTTP header type should not contain ":" (colon).'
    self._response.addHeader(type, data, replace)
  def clearHeaders(self):
    "Clear all HTTP headers (must not be unbuffered)"
    self._response.clearHeaders()
  def unbuffer(self):
    "Turn off output stream buffering; flush immediately to browser."
    self._unbuffer = 1
    self.flush()
  def timestamp(self, thetime=None):
    "Timestamp response with a HTTP Date header"
    self.addHeader('Date', _genTimestampString(thetime), 1)
  def expires(self, thetime=None):
    "Add HTTP expiration headers"
    self.addHeader('Expires', _genTimestampString(thetime), 1)
  def expiresRel(self, secs=0):
    "Set response expiration (relative to now) with a HTTP Expires header"
    self.expires(int(time.time())+secs)
  def lastModified(self, thetime=-1):
    "Set last modification time"
    if thetime==-1:
      filename = self._api.getFilename()
      if not filename or not os.path.exists(filename):
        raise 'request filename not found; can not determine last modification time'
      thetime = os.stat(filename)[9] # file ctime
    self.addHeader('Last-Modified', _genTimestampString(thetime), 1)
    # ensure last modified before timestamp, at least when we're generating it
    if thetime==None: self.timestamp()
  def uncacheable(self):
    "Ensure that compliant clients and proxies don't cache this response"
    self.addHeader('Cache-Control', 'no-store, no-cache, must-revalidate')
    self.addHeader('Pragma', 'no-cache')
  def __repr__(self):
    s = []
    s.append('filters: %s' % len(self._filterList))
    s.append('unbuffered: %s' % self._unbuffer)
    return string.join(s, ', ')

class Filter:
  def setNext(self, filter):
    self.next = filter
  def write(self, s):
    s = self.dynamicImpl(s)
    self.next.write(s)
  def writeStatic(self, s):
    s = self.staticImpl(s)
    self.next.writeStatic(s)
  def writeExpr(self, s, **kwargs):
    s = apply(self.exprImpl, (s,), kwargs)
    apply(self.next.writeExpr, (s,), kwargs)
  def flush(self, stopFlag=0):
    self.flushImpl()
    self.next.flush(stopFlag)
  def clear(self):
    self.clearImpl()
    self.next.clear()
  def dynamicImpl(self, s, *args, **kwargs):
    raise 'not implemented'
  def staticImpl(self, s, *args, **kwargs):
    raise 'not implemented'
  def exprImpl(self, s, *args, **kwargs):
    raise 'not implemented'
  def flushImpl(self):
    raise 'not implemented'
  def clearImpl(self):
    raise 'not implemented'

class FilterUnify(Filter):
  def __init__(self, mod):
    self.mod = mod
    self.mod._api.registerResponseCallback(self.syncResponse)
    self.syncResponse()
  def syncResponse(self):
    response = self.mod._api.getResponse()
    self.write = response.write
    self.writeStatic = response.write
    self.flush = response.flush
    self.clear = response.clear
  def writeExpr(self, s, **kwargs):
    self.write(str(s))
  def setNext(self, filter):
    pass # we are always at the end

def _genTimestampString(thetime=None):
  "Generate timestamp string"
  if thetime==None:
    thetime = int(time.time())
  if type(thetime)==type(0):
    thetime = time.strftime('%a, %d %b %Y %H:%M:%S %Z', time.localtime(thetime))
  if type(thetime)==type(''):
    return thetime
  raise 'thetime value should be None or string or integer (seconds)'
