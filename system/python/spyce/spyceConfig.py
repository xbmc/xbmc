##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import sys, os, string, ConfigParser
import spyceException

##################################################
# Constants
#

SPYCE_CONCURRENCY_SINGLE = 0
SPYCE_CONCURRENCY_FORK = 1
SPYCE_CONCURRENCY_THREAD = 2

SPYCE_CONFIG_LOCATIONS = [
  '/etc',
]
SPYCE_CONFIG_FILENAME = 'spyce.conf'
SPYCE_HOME = None

##################################################
# determine HOME (installation) directory
#

if SPYCE_HOME == None:
  try:
    SPYCE_HOME = os.path.abspath(os.path.dirname(sys.modules['spyceConfig'].__file__))
  except:
    for p in sys.path:
      path = os.path.join(p, 'spyceConfig.py')
      if os.path.exists(path):
        SPYCE_HOME = os.path.abspath(p)
        break
  if not SPYCE_HOME:
    raise 'unable to determine Spyce home directory'

##################################################
# Server configuration object
#

class spyceConfig:
  def __init__(self,
    file=None,
    overide_path=None,
    overide_import=None,
    overide_error=None,
    overide_pageerror=None,
    overide_globals=None,
    overide_debug=None,
    default_concurrency=None,  # no concurrency
    overide_concurrency=None,
    overide_cache=None,
    default_www_root='.',  # serve from current directory
    overide_www_root=None,
    default_www_port=80,  # default HTTP
    overide_www_port=None,
    default_www_handler={
      'spy': 'spyce',
      None: 'dump',
    },
    default_www_mime = [os.path.join(SPYCE_HOME,'spyce.mime')],
  ):
    self.file = file
    self._procesed = 0
    # config parameters
    self.spyce_config = None # spyce configuration dictionary
    self.spyce_path = None  # spyce module search path
    self.spyce_import = None # python modules loaded at startup
    self.spyce_error = None # spyce engine-level error handler
    self.spyce_pageerror = None # spyce page-level error handler
    self.spyce_globals = {} # spyce engine globals dictionary
    self.spyce_debug = None # spyce engine debug flag
    self.spyce_concurrency = None # concurrency model
    self.spyce_www_root = None # root directory for spyce web server
    self.spyce_www_port = None # port used by spyce web server
    self.spyce_www_mime = None # files with Apache-like mime type listings
    self.spyce_www_handler = None # assign handlers by extension
    # store overides and defaults
    self.overide_path = overide_path
    self.overide_import = overide_import
    self.overide_error = overide_error
    self.overide_pageerror = overide_pageerror
    self.overide_globals = overide_globals
    self.overide_debug = overide_debug
    self.overide_concurrency = overide_concurrency
    self.default_concurrency = default_concurrency
    self.overide_cache = overide_cache
    self.default_www_root = default_www_root
    self.overide_www_root = overide_www_root
    self.default_www_port = default_www_port
    self.overide_www_port = overide_www_port
    self.default_www_handler = default_www_handler
    self.default_www_mime = default_www_mime
  def process(self):
    # process (order matters here!)
    self.processConfigFile()
    self.processSpycePath()
    self.processSpyceDebug()
    self.processSpyceGlobals()
    self.processSpyceImport()
    self.processSpyceError()
    self.processSpyceConcurrency()
    self.processSpyceCache()
    self.processSpyceWWW()
  # accessors
  def getSpyceHome(self):
    return SPYCE_HOME
  def getSpycePath(self):
    return self.spyce_path
  def getSpyceImport(self):
    return self.spyce_import
  def getSpyceError(self):
    return self.spyce_error
  def getSpycePageError(self):
    return self.spyce_pageerror
  def getSpyceGlobals(self):
    return self.spyce_globals
  def getSpyceDebug(self):
    return self.spyce_debug
  def getSpyceConcurrency(self):
    return self.spyce_concurrency
  def getSpyceCache(self):
    return self.spyce_cache
  def getSpyceWWWRoot(self):
    return self.spyce_www_root
  def getSpyceWWWPort(self):
    return self.spyce_www_port
  def getSpyceWWWHandler(self):
    return self.spyce_www_handler
  def getSpyceWWWMime(self):
    return self.spyce_www_mime
  def __repr__(self):
    return \
'''path: %(path)s
import: %(import)s
error: %(error)s
globals: %(globals)s
debug: %(debug)s
concurrency: %(concurrency)s
cache: %(cache)s
www root: %(wwwroot)s
www port: %(wwwport)s
www mime: %(wwwmime)s
www handler: %(wwwhandler)s
''' % {
  'path':        self.spyce_path,
  'import':      self.spyce_import,
  'error':       self.spyce_error,
  'pageerror':   self.spyce_pageerror,
  'globals':     self.spyce_globals,
  'debug':       self.spyce_debug,
  'concurrency': self.spyce_concurrency,
  'cache':       self.spyce_cache,
  'wwwroot':     self.spyce_www_root,
  'wwwport':     self.spyce_www_port,
  'wwwmime':     self.spyce_www_mime,
  'wwwhandler':  self.spyce_www_handler,
}

  # configuration processing
  def processConfigFile(self):
    # config file
    self.spyce_config = {}
    if not self.file:
      self.file = self.findConfigFile()
    if self.file:
      if not os.path.exists(self.file):
        raise spyceException.spyceNotFound(self.file)
      if not os.access(self.file, os.R_OK):
        raise spyceException.spyceForbidden(self.file)
      self.spyce_config = self.parseConfigFile()
  def processSpycePath(self, mod_path=None):
    def processSpycePathString(str, self=self):
      dpath = filter(None, map(string.strip, string.split(str, os.pathsep)))
      for i in range(len(dpath)):
        dpath[i] = os.path.abspath(dpath[i])
      self.spyce_path = self.spyce_path + dpath
      sys.path = sys.path + dpath
    def filterPath(path):
      path2 = []
      for p in path:
        if p not in path2:
          path2.append(p)
      return path2
    self.spyce_path = [
      os.path.join(SPYCE_HOME, 'modules'),
      os.path.join(SPYCE_HOME, 'tags'),
    ]
    if mod_path:
      processSpycePathString(mod_path)
    if self.spyce_config.has_key('path') and not self.overide_path:
      processSpycePathString(self.spyce_config['path'])
    if self.overide_path:
      processSpycePathString(overide_path)
    else:
      if (self.spyce_config and not self.spyce_config.has_key('path')) and os.environ.has_key('SPYCE_PATH'):
        processSpycePathString(os.environ['SPYCE_PATH'])
    self.spyce_path = filterPath(self.spyce_path)
    sys.path = filterPath(sys.path)
  def processSpyceImport(self):
    self.spyce_import = []
    if self.spyce_config.has_key('import'):
      new_import = filter(None, map(string.strip, string.split(self.spyce_config['import'], ',')))
      self.spyce_import = self.spyce_import + new_import
    if self.overide_import:
      new_import = filter(None, map(string.strip, string.split(self.overide_import, ',')))
      self.spyce_import = self.spyce_import + new_import
    for i in self.spyce_import:
      exec('import %s'%i)
  def processSpyceError(self):
    # server-level
    self.spyce_error = 'error:serverHandler'
    if self.spyce_config.has_key('error'):
      self.spyce_error = self.spyce_config['error']
    if self.overide_error:
      self.spyce_error = self.overide_error
    self.spyce_error = string.split(self.spyce_error, ':')
    if len(self.spyce_error)==1:
      raise 'invalid error handler specification (file:function)'
    # page-level
    self.spyce_pageerror = 'string:error:defaultErrorTemplate'
    if self.spyce_config.has_key('pageerror'):
      self.spyce_pageerror = self.spyce_config['pageerror']
    if self.overide_pageerror:
      self.spyce_pageerror = self.overide_pageerror
    self.spyce_pageerror = string.split(self.spyce_pageerror, ':')
    if (len(self.spyce_pageerror)<2 or self.spyce_pageerror[0] not in ('string', 'file')):
      raise 'invalid pageerror handler specification ("string":module:variable, or ("file":file)'
  def processSpyceGlobals(self):
    self.spyce_globals.clear ()
    if self.spyce_config.has_key('globals'):
      for k in self.spyce_config['globals'].keys():
        self.spyce_globals[k] = self.spyce_config['globals'][k]
    if self.overide_globals:
      for k in self.overide_globals.keys():
        self.spyce_globals[k] = self.overide_globals[k]
    for k in self.spyce_globals.keys():
      self.spyce_globals[k] = eval(self.spyce_globals[k])
  def processSpyceDebug(self):
    self.spyce_debug = 0
    if self.spyce_config.has_key('debug'):
      self.spyce_debug = string.strip(string.lower(self.spyce_config['debug'])) not in ('0', 'false', 'no')
    if self.overide_debug:
      self.spyce_debug = 1
  def processSpyceConcurrency(self):
    self.spyce_concurrency = SPYCE_CONCURRENCY_SINGLE
    if self.default_concurrency!=None:
      self.spyce_concurrency = self.default_concurrency
    if self.spyce_config.has_key('concurrency'):
      self.spyce_concurrency = string.lower(self.spyce_config['concurrency'])
      if self.spyce_concurrency in ('thread', 'threading'):
        self.spyce_concurrency = SPYCE_CONCURRENCY_THREAD
      elif self.spyce_concurrency in ('fork', 'forking'):
        self.spyce_concurrency = SPYCE_CONCURRENCY_FORK
      else: 
        self.spyce_concurrency = SPYCE_CONCURRENCY_SINGLE
    if self.overide_concurrency!=None:
      self.spyce_concurrency = self.overide_concurrency
  def processSpyceCache(self):
    cache = 'memory'
    if self.spyce_config.has_key('cache'):
      cache = self.spyce_config['cache']
    cache = string.split(cache, ':')
    self.spyce_cache = string.strip(string.lower(cache[0])), string.join(cache[1:], ':')
  def processSpyceWWW(self):
    # root
    self.spyce_www_root = self.default_www_root
    if self.spyce_config.has_key('www_root'):
      self.spyce_www_root = self.spyce_config['www_root']
    if self.overide_www_root!=None:
      self.spyce_www_root = self.overide_www_root
    # port
    self.spyce_www_port = self.default_www_port
    if self.spyce_config.has_key('www_port'):
      self.spyce_www_port = int(self.spyce_config['www_port'])
    if self.overide_www_port!=None:
      self.spyce_www_port = int(self.overide_www_port)
    # mime
    self.spyce_www_mime = self.default_www_mime
    if self.spyce_config.has_key('www_mime'):
      mime = self.spyce_config['www_mime']
      mime = map(string.strip, string.split(mime, ','))
      self.spyce_www_mime = self.spyce_www_mime + mime
    # handler
    self.spyce_www_handler = self.default_www_handler
    if self.spyce_config.has_key('www_handler'):
      handler = self.spyce_config['www_handler']
      for k in handler.keys():
        self.spyce_www_handler[k] = handler[k]

  # helpers
  def findConfigFile(self):
    locations = [SPYCE_HOME] + SPYCE_CONFIG_LOCATIONS
    for l in locations:
      p = os.path.join(l, SPYCE_CONFIG_FILENAME)
      if os.path.exists(p):
        return p
  def parseConfigFile(self):
    # initial defaults
    path = None
    load = None
    error = None
    pageerror = None
    globals = None
    debug = None
    concurrency = None
    cache = None
    www_root = None
    www_port = None
    www_mime = None
    www_handler = {}
    cfg = ConfigParser.ConfigParser()
    # parse
    cfg.read(self.file)
    if cfg.has_section('spyce'):
      if 'path' in cfg.options('spyce'):
        path = cfg.get('spyce', 'path')
      if 'import' in cfg.options('spyce'):
        load = cfg.get('spyce', 'import')
      if 'error' in cfg.options('spyce'):
        error = cfg.get('spyce', 'error')
      if 'pageerror' in cfg.options('spyce'):
        pageerror = cfg.get('spyce', 'pageerror')
      if 'debug' in cfg.options('spyce'):
        debug = cfg.get('spyce', 'debug')
      if 'concurrency' in cfg.options('spyce'):
        concurrency = cfg.get('spyce', 'concurrency')
      if 'cache' in cfg.options('spyce'):
        cache = cfg.get('spyce', 'cache')
    if cfg.has_section('globals'):
      globals = {}
      for o in cfg.options('globals'):
        if o=='__name__': continue
        globals[o] = cfg.get('globals', o)
    if cfg.has_section('www'):
      for o in cfg.options('www'):
        if o=='__name__': continue
        if o=='root':
          www_root = cfg.get('www', o)
          continue
        if o=='port':
          www_port = cfg.get('www', o)
          continue
        if o=='mime': 
          www_mime = cfg.get('www', o)
          continue
        if o[:len('ext_')]=='ext_':
          ext = o[len('ext_'):]
          if not ext: ext = None
          www_handler[ext] = cfg.get('www', o)
    # results
    config = {}
    if path!=None: config['path'] = path
    if load!=None: config['import'] = load
    if error!=None: config['error'] = error
    if pageerror!=None: config['pageerror'] = pageerror
    if globals!=None: config['globals'] = globals
    if debug!=None: config['debug'] = debug
    if concurrency!=None: config['concurrency'] = concurrency
    if cache!=None: config['cache'] = cache
    if www_root!=None: config['www_root'] = www_root
    if www_port!=None: config['www_port'] = www_port
    if www_mime!=None: config['www_mime'] = www_mime
    if www_handler!={}: config['www_handler'] = www_handler
    return config
