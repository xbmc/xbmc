##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import string, sys, os
import spyce, spyceException, spyceUtil

__doc__ = '''Apache mod_python-based Spyce entry point.'''

##################################################
# Request / response handlers
#

import _apache   # fails when not running under apache
from mod_python import apache

class NoFlush:
  "Elide flush calls"
  def __init__(self, apacheRequest):
    self.write = apacheRequest.write
  def flush(self):
    pass

# make sure that both sets of classes have identical functionality
class spyceModpyRequest(spyce.spyceRequest):
  "Apache Spyce request object. (see spyce.spyceRequest)"
  def __init__(self, apacheRequest):
    spyce.spyceRequest.__init__(self)
    self._in = apacheRequest
  def env(self, name=None):
    return spyceUtil.extractValue(self._in.subprocess_env, name)
  def getHeader(self, type=None):
    if type:
      if self._in.headers_in.has_key(type):
        return self._in.headers_in[type]
    else: return self._in.headers_in
  def getServerID(self):
    return os.getpid()

class spyceModpyResponse(spyce.spyceResponse):
  "Apache Spyce response object. (see spyce.spyceResponse)"
  def __init__(self, apacheRequest):
    spyce.spyceResponse.__init__(self)
    self.origout = apacheRequest
    self.out = spyceUtil.BufferedOutput(NoFlush(apacheRequest))
    self.isCTset = 0
    self.headersSent = 0
    self.returncode = self.origout.status = self.RETURN_OK
    # functions (for performance)
    self.write = self.out.write
    self.writeErr = sys.stderr.write
  def close(self):
    self.flush()
    #self.out.close()
  def clear(self):
    self.out.clear()
  def sendHeaders(self):
    if self.headersSent: 
      return
    if not self.isCTset:
      self.setContentType("text/html")
    self.origout.send_http_header()
    self.headersSent = 1
  def clearHeaders(self):
    if self.headersSent:
      raise 'headers already sent'
    for header in self.origout.headers_out.keys():
      del self.origout.headers_out[header]
  def setContentType(self, content_type):
    if self.headersSent:
      raise 'headers already sent'
    self.origout.content_type = content_type
    self.isCTset = 1
  def setReturnCode(self, code):
    if self.headersSent:
      raise 'headers already sent'
    self.returncode = self.origout.status = code
  def addHeader(self, type, data, replace=0):
    if self.headersSent:
      raise 'headers already sent'
    if replace:
      self.origout.headers_out[type] = data
    else:
      self.origout.headers_out.add(type, data)
  def flush(self, stopFlag=0):
    if stopFlag: return
    self.sendHeaders()
    self.out.flush()
  def unbuffer(self):
    self.flush()
    self.out.unbuffer()

##################################################
# Apache config
#

def getApacheConfig(apachereq, param, default=None):
  "Return Apache mod_python configuration parameter."
  try:
    return apachereq.get_options()[param]
  except:
    return default

##################################################
# Apache entry point
#

CONFIG_FILE = None

def spyceMain(apacheRequest):
  "Apache entry point."
  os.environ[spyce.SPYCE_ENTRY] = 'modpy'
  apacheRequest.add_common_vars()
  request = spyceModpyRequest(apacheRequest)
  response = spyceModpyResponse(apacheRequest)
  filename = apacheRequest.filename
  global CONFIG_FILE
  if CONFIG_FILE==None:
    CONFIG_FILE = getApacheConfig(apacheRequest, 'SPYCE_CONFIG', None)
  try:
    result = spyce.spyceFileHandler(request, response, filename, config_file=CONFIG_FILE )
  except (spyceException.spyceForbidden, spyceException.spyceNotFound), e:
    response.clear()
    response.setContentType('text/plain')
    response.write(str(e)+'\n')
  except:
    response.clear()
    response.setContentType('text/plain')
    response.write(spyceUtil.exceptionString()+'\n')
  try:
    response.flush()
  except: pass
  return apache.OK

