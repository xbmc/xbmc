##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
from spyceUtil import NoCloseOut
from cStringIO import StringIO

__doc__ = '''Sets the thread-safe server stdout to the response object for
convenience of using print statements, and supports output redirection.'''

class stdout(spyceModule):
  def start(self):
    # thread-safe stdout swap
    self.stdout = self._api.getStdout()
    self._api.setStdout(myResponseWrapper(self))
    # output redirection stack
    self.outputStack = []
    # memoize storage
    try: self.memoizeCache = self._api.getServerObject().memoized
    except AttributeError:
      self.memoizeCache = self._api.getServerObject().memoized = {}
  def finish(self, theError=None):
    # close all redirects
    while self.outputStack:
      self.pop()
    # thread-safe stdout swap back
    self._api.setStdout(self.stdout)
  def push(self, file=None):
    'Redirect stdout to buffer'
    old_response = self._api.getResponse()
    old_response_mod = self._api.getModule('response')
    new_response = spyceCaptureResponse(old_response)
    self._api.setResponse(new_response)
    new_response_mod = self._api.spyceModule('response', 'response.py')(self._api)
    self._api.setModule('response', new_response_mod)
    new_response_mod.start()
    self.outputStack.append( (file, old_response, old_response_mod) )
  def pop(self):
    'Return buffer value, and possible write to file'
    self._api.getModule('response').finish()
    buffer = self._api.getResponse().getCapturedOutput()
    file, old_response, old_response_mod = self.outputStack.pop()
    self._api.setModule('response', old_response_mod)
    self._api.setResponse(old_response)
    if file:
      file = os.path.join(os.path.dirname(self._api.getFilename()), file)
      out = None
      try:
        out = open(file, 'w')
        out.write(buffer)
      finally:
        if out: out.close()
    return buffer
  def capture(self, _spyceReserved, *args, **kwargs):
    'Capture the output side-effects of a function'
    f = _spyceReserved # placeholder not to collide with kwargs
    self.push()
    r = apply(f, args, kwargs)
    s = self.pop()
    return r, s
  def __repr__(self):
    return 'depth: %s' % len(self.outputStack)

class myResponseWrapper:
  def __init__(self, mod):
    self._mod = mod
    mod._api.registerModuleCallback(self.syncResponse)
    self.syncResponse()
  def syncResponse(self):
    response = self._mod._api.getModule('response')
    # functions (for performance)
    self.write = response.write
    self.writeln = response.writeln
    self.flush = response.flush
  def close(self):
    raise 'method not allowed'

class spyceCaptureResponse:
  "Capture output, and let everything else through."
  def __init__(self, old_response):
    self._old_response = old_response
    self._buf = StringIO()
  def write(self, s):
    self._buf.write(s)
  def close(self):
    raise 'cannot close output while capturing output'
  def clear(self):
    self._buf = StringIO()
  def sendHeaders(self):
    raise 'cannot sendHeaders while capturing output!'
  def flush(self, stopFlag=0):
    pass
  def unbuffer(self):
    raise 'cannot unbuffer output while capturing output!'
  def __getattr__(self, name):
    return eval('self._old_response.%s'%name)
  def getCapturedOutput(self):
    return self._buf.getvalue()

