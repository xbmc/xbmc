#!/usr/bin/env python

__version__ = '1.3.13'
__release__ = '1'

DEBUG_ERROR = 0

##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to LICENCE for legalese
#
# Name:        spyce
# Author:      Rimon Barr <rimon-AT-acm.org>
# Start date:  8 April 2002
# Purpose:     Python Server Pages
# WWW:         http://spyce.sourceforge.net/
# CVS:         $Id$
##################################################

# note: doc string used in documentation: doc/index.spy
__doc__ = '''SPYCE is a server-side language that supports simple and
efficient Python-based dynamic HTML generation, otherwise called <i>Python
Server Pages</i> (PSP). Those who are familiar with JSP, PHP, or ASP and like
Python, should have a look at Spyce. Its modular design makes it very flexible
and extensible. It can also be used as a command-line utility for static text
pre-processing or as a web-server proxy.'''

import sys, os, copy, string, imp
import spyceConfig, spyceCompile, spyceException
import spyceModule, spyceTag
import spyceLock, spyceCache, spyceUtil

##################################################
# Spyce engine globals
#

# spyceServer object - one per engine instance
SPYCE_SERVER = None
def getServer(
    config_file=None, 
    overide_www_port=None,
    overide_www_root=None,
    force=0):
  global SPYCE_SERVER
  if force or not SPYCE_SERVER:
    SPYCE_SERVER = spyceServer(
      config_file=config_file,
      overide_www_root=overide_www_root,
      overide_www_port=overide_www_port,
    )
  return SPYCE_SERVER

SPYCE_GLOBALS = None
def getServerGlobals():
  global SPYCE_GLOBALS
  return SPYCE_GLOBALS

SPYCE_LOADER = 'spyceLoader'
SPYCE_ENTRY = 'SPYCE_ENTRY'
DEFAULT_MODULES = ('request', 'response', 'stdout', 'error')

##################################################
# Spyce core objects
#

class spyceServerObject:
  "serverObject placeholder"
  pass

class spyceServer:
  "One per server, stored in SPYCE_SERVER (above) at processing of first request."
  def __init__(self, 
      config_file=None,
      overide_www_root=None,
      overide_www_port=None,
    ):
    global SPYCE_GLOBALS
    # server object
    self.serverobject = spyceServerObject()
    # http headers
    try: self.entry = os.environ[SPYCE_ENTRY]
    except: self.entry = 'UNKNOWN'
    self.spyceHeader = 'Spyce/%s_%s Python/%s' % (self.entry, str(__version__), sys.version[:3])
    # configuration dictionary
    self.config = spyceConfig.spyceConfig(
      file=config_file,
      overide_www_root=overide_www_root,
      overide_www_port=overide_www_port,
    )
    # server globals/constants
    self.globals = self.config.getSpyceGlobals()
    SPYCE_GLOBALS = self.globals # hack
    # now finish processing config file; this way imported modules have
    # access to the globals
    self.config.process ()
    # spyce module search path
    self.path = self.config.getSpycePath()
    # concurrency mode
    self.concurrency = self.config.getSpyceConcurrency()
    # imports
    self.imports = self.config.getSpyceImport()
    # debug mode
    self.debug = self.config.getSpyceDebug()
    # spyce cache
    type, info = self.config.getSpyceCache()
    if type in ('file',):
      type = spyceCache.fileCache(info)
    elif type in ('mem', 'memory'):
      type = spyceCache.memoryCache(info)
    else: type = spyceCache.memoryCache()
    if self.debug: type = None
    self.spyce_cache = spyceCache.semanticCache(type, spyceCacheValid, spyceCacheGenerate)
    # spyce module cache
    self.module_cache = {}
    if self.debug:
      self.module_cache = None
    # page error handler
    pageerror = self.config.getSpycePageError()
    if pageerror[0]=='string':
      pageerror = pageerror[0], self.loadModule(pageerror[2], pageerror[1]+'.py')
    self.pageerror = pageerror
    # engine error handler
    error = self.config.getSpyceError()
    self.error = self.loadModule(error[1], error[0]+'.py')
    # spyce thread-safe stdout object
    if self.concurrency == spyceConfig.SPYCE_CONCURRENCY_THREAD:
      self.stdout = spyceUtil.ThreadedWriter(sys.stdout)
      self.lock = spyceLock.threadLock()
      sys.stdout = self.stdout
    else:
      self.stdout = None
      self.lock = spyceLock.dummyLock()
    # set sys.stdout
  def loadModule(self, name, file=None, rel_file=None):
    "Find and load a module, with caching"
    if not file: file=name+'.py'
    key = name, file, rel_file
    if self.module_cache!=None:
      try: return self.module_cache[key]
      except: pass # cache miss
    def loadModuleHelper(file=file, rel_file=rel_file, path=self.path):
      if rel_file: path = path + [os.path.dirname(rel_file)]
      for p in path:
        f=None
        try:
          p = os.path.join(p, file)
          if os.path.exists(p) and os.access(p, os.R_OK):
            f = open(p, 'r')
            return imp.load_source(SPYCE_LOADER, p, f)
        finally:
          if f: f.close()
      raise 'unable to find module "%s" in path: %s' % (file, path)
    # load and cache module
    dict = {'loadModuleHelper': loadModuleHelper}
    exec 'foo = loadModuleHelper()' in dict
    mod = eval('dict["foo"].%s' % name)
    if self.module_cache!=None:
      self.module_cache[key] = mod
    return mod
  def fileHandler(self, request, response, filename, sig='', args=None, kwargs=None):
    return self.commonHandler(request, response, ('file', (filename, sig)), args, kwargs)
  def stringHandler(self, request, response, code, sig='', args=None, kwargs=None):
    return self.commonHandler(request, response, ('string', (code, sig)), args, kwargs)
  def commonHandler(self, request, response, spyceInfo, args=None, kwargs=None):
    "Handle a request. This method is NOT thread safe."
    try:
      thespyce = theError = None
      try:
        spycecode = self.spyce_cache[spyceInfo]
        thespyce = spycecode.newWrapper()
        response.addHeader('X-Spyce', self.spyceHeader, 1)
        try:
          thespyce.spyceInit(request, response)
          if args==None: args=[]
          if kwargs==None: kwargs={}
          apply(thespyce.spyceProcess, args, kwargs)
        except spyceException.spyceRuntimeException, theError: pass
      finally:
        if DEBUG_ERROR and theError:
          sys.stderr.write(`theError`+'\n')
        if thespyce:
          thespyce.spyceDestroy(theError)
          spycecode.returnWrapper(thespyce)
    except spyceException.spyceDone: pass
    except spyceException.spyceRedirect, e:
      return spyceFileHandler(request, response, e.filename)
    except KeyboardInterrupt: raise
    except (spyceException.spyceNotFound, spyceException.spyceForbidden, 
        spyceException.spyceSyntaxError, spyceException.pythonSyntaxError, 
        SyntaxError), e:
      return self.error(self, request, response, e)
    except SystemExit: pass
    except:
      errorString = spyceUtil.exceptionString()
      try:
        import cgi
        response.clear()
        response.write('<html><pre>\n')
        response.write('Unexpected exception: (please report!)\n')
        response.write(cgi.escape(errorString))
        response.write('\n</pre></html>\n')
        response.returncode = response.RETURN_OK
      except:
        sys.stderr.write(errorString+'\n')
    return response.returncode

class spyceRequest:
  """Underlying Spyce request object. All implementations (CGI, Apache...)
  should subclass and implement the methods marked 'not implemented'."""
  def __init__(self):
    self._in = None
  def read(self, limit=None):
    if limit:
      return self._in.read(limit)
    else:
      return self._in.read()
  def readline(self, limit=None):
    if limit:
      return self._in.readline(limit)
    else:
      return self._in.readline()
  def env(self, name=None):
    raise 'not implemented'
  def getHeader(self, type=None):
    raise 'not implemented'
  def getServerID(self):
    raise 'not implemented'

class spyceResponse:
  """Underlying Spyce response object. All implementations (CGI, Apache...)
  should subclass and implement the methods marked 'not implemented', and
  also properly define the RETURN codes."""
  RETURN_CONTINUE = 100
  RETURN_SWITCHING_PROTOCOLS = 101
  RETURN_OK = 200
  RETURN_CREATED = 201
  RETURN_ACCEPTED = 202
  RETURN_NON_AUTHORITATIVE_INFORMATION = 203
  RETURN_NO_CONTENT = 204
  RETURN_RESET_CONTENT = 205
  RETURN_PARTIAL_CONTENT = 206
  RETURN_MULTIPLE_CHOICES = 300
  RETURN_MOVED_PERMANENTLY = 301
  RETURN_MOVED_TEMPORARILY = 302
  RETURN_SEE_OTHER = 303
  RETURN_NOT_MODIFIED = 304
  RETURN_USE_PROXY = 305
  RETURN_TEMPORARY_REDIRECT = 307
  RETURN_BAD_REQUEST = 400
  RETURN_UNAUTHORIZED = 401
  RETURN_PAYMENT_REQUIRED = 402
  RETURN_FORBIDDEN = 403
  RETURN_NOT_FOUND = 404
  RETURN_METHOD_NOT_ALLOWED = 405
  RETURN_NOT_ACCEPTABLE = 406
  RETURN_PROXY_AUTHENTICATION_REQUIRED = 407
  RETURN_REQUEST_TIMEOUT = 408
  RETURN_CONFLICT = 409
  RETURN_GONE = 410
  RETURN_LENGTH_REQUIRED = 411
  RETURN_PRECONDITION_FAILED = 412
  RETURN_REQUEST_ENTITY_TOO_LARGE = 413
  RETURN_REQUEST_URI_TOO_LONG = 414
  RETURN_UNSUPPORTED_MEDIA_TYPE = 415
  RETURN_REQUEST_RANGE_NOT_SATISFIABLE = 416
  RETURN_EXPECTATION_FAILED = 417
  RETURN_INTERNAL_SERVER_ERROR = 500
  RETURN_NOT_IMPLEMENTED = 501
  RETURN_BAD_GATEWAY = 502
  RETURN_SERVICE_UNAVAILABLE = 503
  RETURN_GATEWAY_TIMEOUT = 504
  RETURN_HTTP_VERSION_NOT_SUPPORTED = 505
  RETURN_CODE = {
    RETURN_CONTINUE: 'CONTINUE',
    RETURN_SWITCHING_PROTOCOLS: 'SWITCHING PROTOCOLS',
    RETURN_OK: 'OK',
    RETURN_CREATED: 'CREATED',
    RETURN_ACCEPTED: 'ACCEPTED',
    RETURN_NON_AUTHORITATIVE_INFORMATION: 'NON AUTHORITATIVE INFORMATION',
    RETURN_NO_CONTENT: 'NO CONTENT',
    RETURN_RESET_CONTENT: 'RESET CONTENT',
    RETURN_PARTIAL_CONTENT: 'PARTIAL CONTENT',
    RETURN_MULTIPLE_CHOICES: 'MULTIPLE CHOICES',
    RETURN_MOVED_PERMANENTLY: 'MOVED PERMANENTLY',
    RETURN_MOVED_TEMPORARILY: 'MOVED TEMPORARILY',
    RETURN_SEE_OTHER: 'SEE OTHER',
    RETURN_NOT_MODIFIED: 'NOT MODIFIED',
    RETURN_USE_PROXY: 'USE PROXY',
    RETURN_TEMPORARY_REDIRECT: 'TEMPORARY REDIRECT',
    RETURN_BAD_REQUEST: 'BAD REQUEST',
    RETURN_UNAUTHORIZED: 'UNAUTHORIZED',
    RETURN_PAYMENT_REQUIRED: 'PAYMENT REQUIRED',
    RETURN_FORBIDDEN: 'FORBIDDEN',
    RETURN_NOT_FOUND: 'NOT FOUND',
    RETURN_METHOD_NOT_ALLOWED: 'METHOD NOT ALLOWED',
    RETURN_NOT_ACCEPTABLE: 'NOT ACCEPTABLE',
    RETURN_PROXY_AUTHENTICATION_REQUIRED: 'PROXY AUTHENTICATION REQUIRED',
    RETURN_REQUEST_TIMEOUT: 'REQUEST TIMEOUT',
    RETURN_CONFLICT: 'CONFLICT',
    RETURN_GONE: 'GONE',
    RETURN_LENGTH_REQUIRED: 'LENGTH REQUIRED',
    RETURN_PRECONDITION_FAILED: 'PRECONDITION FAILED',
    RETURN_REQUEST_ENTITY_TOO_LARGE: 'REQUEST ENTITY TOO LARGE',
    RETURN_REQUEST_URI_TOO_LONG: 'REQUEST URI TOO LONG',
    RETURN_UNSUPPORTED_MEDIA_TYPE: 'UNSUPPORTED MEDIA TYPE',
    RETURN_REQUEST_RANGE_NOT_SATISFIABLE: 'REQUEST RANGE NOT SATISFIABLE',
    RETURN_EXPECTATION_FAILED: 'EXPECTATION FAILED',
    RETURN_INTERNAL_SERVER_ERROR: 'INTERNAL SERVER ERROR',
    RETURN_NOT_IMPLEMENTED: 'NOT IMPLEMENTED',
    RETURN_BAD_GATEWAY: 'BAD GATEWAY',
    RETURN_SERVICE_UNAVAILABLE: 'SERVICE UNAVAILABLE',
    RETURN_GATEWAY_TIMEOUT: 'GATEWAY TIMEOUT',
    RETURN_HTTP_VERSION_NOT_SUPPORTED: 'HTTP VERSION NOT SUPPORTED',
  }
  def __init__(self):
    pass
  def write(self, s):
    raise 'not implemented'
  def writeErr(self, s):
    raise 'not implemented'
  def close(self):
    raise 'not implemented'
  def clear(self):
    raise 'not implemented'
  def sendHeaders(self):
    raise 'not implemented'
  def clearHeaders(self):
    raise 'not implemented'
  def setContentType(self, content_type):
    raise 'not implemented'
  def setReturnCode(self, code):
    raise 'not implemented'
  def addHeader(self, type, data, replace=0):
    raise 'not implemented'
  def flush(self):
    raise 'not implemented'
  def unbuffer(self):
    raise 'not implemented'

class spyceCode:
  '''Takes care of compiling the Spyce file, and generating a wrapper'''
  def __init__(self, code, filename=None, sig='', limit=3):
    # store variables
    self._filename = filename
    self._limit = limit
    # generate code
    self._code, self._coderefs, self._modrefs = \
      spyceCompile.spyceCompile(code, filename, sig, getServer())
    self._wrapperQueue = []
    self._wrappersMade = 0
  # wrappers
  def newWrapper(self):
    "Get a wrapper for this code from queue, or make new one"
    try: return self._wrapperQueue.pop()
    except IndexError: pass
    self._wrappersMade = self._wrappersMade + 1
    return spyceWrapper(self)
  def returnWrapper(self, w):
    "Return wrapper back to queue after use"
    if len(self._wrapperQueue)<self._limit:
      self._wrapperQueue.append(w)
  # serialization
  def __getstate__(self):
    return self._filename, self._code, self._coderefs, self._modrefs, self._limit
  def __setstate__(self, state):
    self._filename, self._code, self._coderefs, self._modrefs, self._limit = state
    self._wrapperQueue = []
    self._wrappersMade = 0
  # accessors
  def getCode(self):
    "Return processed Spyce (i.e. Python) code"
    return self._code
  def getFilename(self):
    "Return source filename, if it exists"
    return self._filename
  def getCodeRefs(self):
    "Return python-to-Spyce code line references"
    return self._coderefs
  def getModRefs(self):
    "Return list of import references in Spyce code"
    return self._modrefs

class spyceWrapper:
  """Wrapper object runs the entire show, bringing together the code, the
  Spyce environment, the request and response objects and the modules. It is
  NOT thread safe - new instances are generated as necessary by spyceCode!
  This object is generated by a spyceCode object. The common Spyce handler
  code calls the 'processing' functions. Module writers interact with this
  object via the spyceModuleAPI calls. This is arguable the trickiest portion
  of the Spyce so don't touch unless you know what you are doing."""
  def __init__(self, spycecode):
    # store variables
    self._spycecode = spycecode
    # api object
    self._api = self
    # module tracking
    self._modCache = {}
    self._modstarted = []
    self._modules = {}
    # compile python code
    self._codeenv = { spyceCompile.SPYCE_WRAPPER: self._api }
    try: exec self._spycecode.getCode() in self._codeenv
    except SyntaxError: raise spyceException.pythonSyntaxError(self)
    self._freshenv = self._codeenv.keys()
    # spyce hooks
    noop = lambda *args: None
    self.process = self._codeenv[spyceCompile.SPYCE_PROCESS_FUNC]
    try: self.init = self._codeenv[spyceCompile.SPYCE_INIT_FUNC]
    except: self.init = noop
    try: self.destroy = self._codeenv[spyceCompile.SPYCE_DESTROY_FUNC]
    except: self.destroy = noop
    # request, response
    self._response = self._request = None
    self._responseCallback = {}
    self._moduleCallback = {}
  def _startModule(self, name, file=None, as=None, force=0):
    "Initialise module for current request."
    if as==None: as=name
    if force or not self._codeenv.has_key(as):
      modclass = getServer().loadModule(name, file, self._spycecode.getFilename())
      mod = modclass(self._api)
      self.setModule(as, mod, 0)
      if DEBUG_ERROR:
        sys.stderr.write(as+'.start\n')
      mod.start()
      self._modstarted.append((as, mod))
    else: mod = self._codeenv[as]
    return mod
  # spyce processing
  def spyceInit(self, request, response):
    "Initialise a Spyce for processing."
    self._request = request
    self._response = response
    for mod in DEFAULT_MODULES:
      self._startModule(mod)
    self._modstarteddefault = self._modstarted
    self._modstarted = []
    for (modname, modfrom, modas) in self.getModRefs():
      self._startModule(modname, modfrom, modas, 1)
    exec '_spyce_start()' in { '_spyce_start': self.init }
  def spyceProcess(self, *args, **kwargs):
    "Process the current Spyce request."
    path = sys.path
    try:
      if self._spycecode.getFilename():
        path = copy.copy(sys.path)
        sys.path.append(os.path.dirname(self._spycecode.getFilename()))
      dict = { '_spyce_process': self.process,
        '_spyce_args': args, '_spyce_kwargs': kwargs, }
      exec '_spyce_result = apply(_spyce_process, _spyce_args, _spyce_kwargs)' in dict
      return dict['_spyce_result']
    finally:
      sys.path = path
  def spyceDestroy(self, theError=None):
    "Cleanup after the request processing."
    try:
      exec '_spyce_finish()' in { '_spyce_finish': self.destroy }
      # explicit modules
      self._modstarted.reverse()
      for as, mod in self._modstarted:
        try: 
          if DEBUG_ERROR:
            sys.stderr.write(as+'.finish\n')
          mod.finish(theError)
        except spyceException.spyceDone: pass
        except spyceException.spyceRedirect, e:
          return spyceFileHandler(self._request, self._response, e.filename)
        except KeyboardInterrupt: raise
        except SystemExit: pass
        except: theError = spyceException.spyceRuntimeException(self._api)
      finishError = None
      # default modules
      self._modstarteddefault.reverse()
      for as, mod in self._modstarteddefault:
        try: 
          if DEBUG_ERROR:
            sys.stderr.write(as+'.finish\n')
          mod.finish(theError)
        except: finishError = 1
      self._request = None
      self._response = None
      if finishError: raise
    finally:
      self.spyceCleanup()
  def spyceCleanup(self):
    "Sweep the Spyce environment."
    self._modstarteddefault = []
    self._modstarted = []
    self._modules = {}
    if self._freshenv!=None:
      for e in self._codeenv.keys():
        if e not in self._freshenv:
          del self._codeenv[e]
  # API methods
  def getFilename(self):
    "Return filename of current Spyce"
    return self._spycecode.getFilename()
  def getCode(self):
    "Return processed Spyce (i.e. Python) code"
    return self._spycecode.getCode()
  def getCodeRefs(self):
    "Return python-to-Spyce code line references"
    return self._spycecode.getCodeRefs()
  def getModRefs(self):
    "Return list of import references in Spyce code"
    return self._spycecode.getModRefs()
  def getServerObject(self):
    "Return unique (per engine instance) server object"
    return getServer().serverobject
  def getServerGlobals(self):
    "Return server configuration globals"
    return getServer().globals
  def getServerID(self):
    "Return unique server identifier"
    return self._request.getServerID()
  def getPageError(self):
    "Return default page error value"
    return getServer().pageerror
  def getRequest(self):
    "Return internal request object"
    return self._request
  def getResponse(self):
    "Return internal response object"
    return self._response
  def setResponse(self, o):
    "Set internal response object"
    self._response = o
    for f in self._responseCallback.keys(): f()
  def registerResponseCallback(self, f):
    "Register a callback for when internal response changes"
    self._responseCallback[f] = 1
  def unregisterResponseCallback(self, f):
    "Unregister a callback for when internal response changes"
    try: del self._responseCallback[f]
    except KeyError: pass
  def getModules(self):
    "Return references to currently loaded modules"
    return self._modules
  def getModule(self, name):
    """Get module reference. The module is dynamically loaded and initialised
    if it does not exist (ie. if it was not explicitly imported, but requested
    by another module during processing)"""
    return self._startModule(name)
  def setModule(self, name, mod, notify=1):
    "Add existing module (by reference) to Spyce namespace (used for includes)"
    self._codeenv[name] = mod
    self._modules[name] = mod
    if notify:
      for f in self._moduleCallback.keys(): 
        f()
  def delModule(self, name, notify=1):
    "Add existing module (by reference) to Spyce namespace (used for includes)"
    del self._codeenv[name]
    del self._modules[name]
    if notify:
      for f in self._moduleCallback.keys(): 
        f()
  def getGlobals(self):
    "Return the Spyce global namespace dictionary"
    return self._codeenv
  def registerModuleCallback(self, f):
    "Register a callback for modules change"
    self._moduleCallback[f] = 1
  def unregisterModuleCallback(self, f):
    "Unregister a callback for modules change"
    try: del self._moduleCallback[f]
    except KeyError: pass
  def spyceFile(self, file):
    "Return a spyceCode object of a file"
    return getServer().spyce_cache[('file', file)]
  def spyceString(self, code):
    "Return a spyceCode object of a string"
    return getServer().spyce_cache[('string', code)]
  def spyceModule(self, name, file=None, rel_file=None):
    "Return Spyce module class"
    return getServer().loadModule(name, file, rel_file)
  def spyceTaglib(self, name, file=None, rel_file=None):
    "Return Spyce taglib class"
    return getServer().loadModule(name, file, rel_file)
  def setStdout(self, out):
    "Set the stdout stream (thread-safe)"
    serverout = getServer().stdout
    if serverout: serverout.setObject(out)
    else: sys.stdout = out
  def getStdout(self):
    "Get the stdout stream (thread-safe)"
    serverout = getServer().stdout
    if serverout: return serverout.getObject()
    else: return sys.stdout

##################################################
# Spyce cache
#

def spyceFileCacheValid(key, validity):
  "Determine whether compiled Spyce is valid"
  try: 
    filename, sig = key
  except:
    filename, sig = key, ''
  try:
    if not os.path.exists(filename):
      return 0
    if not os.access(filename, os.R_OK):
      return 0
    return os.path.getmtime(filename) == validity
  except KeyboardInterrupt: raise
  except:
    return 0

def spyceFileCacheGenerate(key):
  "Generate new Spyce wrapper (recompiles)."
  try: 
    filename, sig = key
  except:
    filename, sig = key, ''
  # ensure file exists and we have permissions
  if not os.path.exists(filename):
    raise spyceException.spyceNotFound(filename)
  if not os.access(filename, os.R_OK):
    raise spyceException.spyceForbidden(filename)
  # generate
  mtime = os.path.getmtime(filename)
  f = None
  try:
    f = open(filename)
    code = f.read()
  finally:
    if f: f.close()
  s = spyceCode(code, filename=filename, sig=sig)
  return mtime, s

def spyceStringCacheValid(code, validity):
  return 1

def spyceStringCacheGenerate(key):
  try: 
    code, sig = key
  except:
    code, sig = key, ''
  s = spyceCode(code, sig=sig)
  return None, s

def spyceCacheValid((type, key), validity):
  return { 
    'string': spyceStringCacheValid,
    'file': spyceFileCacheValid,
  }[type](key, validity)

def spyceCacheGenerate((type, key)):
  return {
    'string': spyceStringCacheGenerate,
    'file': spyceFileCacheGenerate,
  }[type](key)


##################################################
# Spyce common entry points
#

def spyceFileHandler(request, response, filename, sig='', args=None, kwargs=None, config_file=None):
  return _spyceCommonHandler(request, response, ('file', (filename, sig)), args, kwargs, config_file)

def spyceStringHandler(request, response, code, sig='', args=None, kwargs=None, config_file=None):
  return _spyceCommonHandler(request, response, ('string', (code, sig)), args, kwargs, config_file)

def _spyceCommonHandler(request, response, spyceInfo, args=None, kwargs=None, config_file=None):
  return getServer(config_file).commonHandler(request, response, spyceInfo, args, kwargs)

if __name__ == '__main__':
  execfile('run_spyceCmd.py')

