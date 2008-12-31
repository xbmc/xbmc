##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import spyceException
import os

__doc__ = '''Error module provides error-handling functionality.'''

class error(spyceModule):
  def start(self):
    self._error = None
    pageerrorType, pageerrorData = self._api.getPageError()
    self.handler = lambda self, pageerrorType=pageerrorType, pageerrorData=pageerrorData: spyceHandler(self, pageerrorData, pageerrorType)
  def finish(self, theError=None):
    self._error = theError
    self._fromFile = self._api.getFilename()
    if theError:
      self.handler(self)
  def setHandler(self, fn):
    if not type(fn)==type(spyceHandler):
      raise 'parameter should be a function'
    self.handler = fn
  def setStringHandler(self, s):
    if not type(s)==type(''):
      raise 'parameter should be a string of spyce code'
    self.handler = lambda self, s=s: spyceHandler(self, s, 'string')
  def setFileHandler(self, f):
    if not type(f)==type(''):
      raise 'parameter should be a filename'
    self.handler = lambda self, f=f: spyceHandler(self, f)
  def getHandler(self):
    return self.handler
  def isError(self):
    return not not self._error
  def getMessage(self):
    if self._error: return self._error.msg
  def getType(self):
    if self._error: return self._error.type
  def getFile(self):
    if self._error: return self._fromFile
  def getTraceback(self):
    if self._error: return self._error.traceback
  def getString(self):
    if self._error: return self._error.str
  def __repr__(self):
    if not self._error: return 'None'
    return 'type: %s, string: %s, msg: %s, file: %s' % (
      self.getType(), self.getString(), self.getMessage(), self.getFile())

def spyceHandler(errorModule, spyceCode, type='file'):
  try:
    responseModule = errorModule._api.getModule('response')
    responseModule.clear()
    responseModule.clearHeaders()
    responseModule.clearFilters()
    responseModule.setContentType('text/html')
    responseModule.setReturnCode(errorModule._api.getResponse().RETURN_OK)
  except: pass
  try:
    s, file = None, None
    if type=='file':
      file = os.path.join(os.path.dirname(errorModule._api.getFilename()), spyceCode)
      code = errorModule._api.spyceFile(file)
    elif type=='string':
      file = '<string>'
      code = errorModule._api.spyceString(spyceCode)
    else:
      raise 'unrecognized handler type'
    try:
      s = code.newWrapper()
      modules = errorModule._api.getModules()
      for name in modules.keys():
        s.setModule(name, modules[name])  # include modules as well!
      s.spyceInit(errorModule._api.getRequest(), errorModule._api.getResponse())
      errmod = s._startModule('error', None, None, 1)
      errmod._error = errorModule._error
      errmod._fromFile = errorModule._fromFile
      s.spyceProcess()
    finally:
      if s: 
        s.spyceDestroy()
        code.returnWrapper(s)
  except spyceException.spyceRuntimeException, e:
    errorModule._error = e
    errorModule._fromFile = file
    if (type, spyceCode) == ('string', defaultErrorTemplate):
      raise  # avoid infinite loop
    else:
      spyceHandler(errorModule, defaultErrorTemplate, 'string')

defaultErrorTemplate = '''
[[.module name=transform]]
[[transform.expr('html_encode')]]
<html>
<title>Spyce exception: [[=error.getMessage()]]</title>
<body>
<table border=0>
  <tr><td colspan=2><h1>Spyce exception</h1></td></tr>
  <tr><td valign=top><b>File:</b></td><td>[[=error.getFile()]]</tr>
  <tr><td valign=top><b>Message:</b></td>
    <td><pre>[[=error.getMessage()]]</pre></tr>
  <tr><td valign=top><b>Stack:</b></td><td>
    [[ for frame in error.getTraceback(): { ]]
      [[=frame[0] ]]:[[=frame[1] ]], in [[=frame[2] ]]:<br>
      <table border=0><tr><td width=10></td><td>
        <pre>[[=frame[3] ]]</pre>
      </td></tr></table>
    [[ } ]]
    </td></tr>
</table>
</body></html>
'''

def serverHandler(theServer, theRequest, theResponse, theError):
  try:
    theResponse.clear()
    theResponse.clearHeaders()
    theResponse.setContentType('text/html')
    theResponse.setReturnCode(theResponse.RETURN_OK)
  except: pass
  s = None
  try:
    spycecode = theServer.spyce_cache[('string', serverErrorTemplate)]
    s = spycecode.newWrapper()
    s.spyceInit(theRequest, theResponse)
    s.getModule('error')._error = theError
    s.spyceProcess()
  finally:
    if s: 
      s.spyceDestroy()
      spycecode.returnWrapper(s)


serverErrorTemplate = '''
[[.module name=transform]]
[[import string, spyceException
  if isinstance(error._error, spyceException.spyceNotFound): { ]]
  <html><body>
  [[=error._error.file]] not found
  [[response.setReturnCode(response._api.getResponse().RETURN_NOT_FOUND)]]
  </body></html>
[[ } elif isinstance(error._error, spyceException.spyceForbidden): { ]]
  <html><body>
  [[=error._error.file]] forbidden
  [[response.setReturnCode(response._api.getResponse().RETURN_FORBIDDEN)]]
  </body></html>
[[ } elif isinstance(error._error, spyceException.spyceSyntaxError): { ]]
  <html><body><pre>
  [[=transform.html_encode(`error._error`)]]
  </pre></body></html>
[[ } elif isinstance(error._error, spyceException.pythonSyntaxError): { ]]
  <html><body><pre>
  [[=transform.html_encode(`error._error`)]]
  </pre></body></html>
[[ } elif isinstance(error._error, SyntaxError): { ]]
  <html><body><pre>
  Syntax error at [[=error._error.filename]]:[[=error._error.lineno]] - 
    [[=transform.html_encode(error._error.text)]]    [[
      if not error._error.offset==None: {
        print ' '*error._error.offset+'^'
      }
    ]]
  </pre></body></html>
[[ } else: { raise error._error } ]]
'''
