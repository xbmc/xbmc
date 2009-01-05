##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import re, time, string, random
import spyceLock
try:
  import cPickle
  pickle = cPickle
except:
  import pickle

__doc__ = '''Session module provides support for session management - the
storage of variables on the server between requests under some short
identifier. 

A user must call setHandler() to determine how the sessions are stored, before
using the other session methods. The get(), set() and delete() methods provide
access to the session information. 

The autoSession() method will turn on the automatic session management
(loading and saving the session). When automatic session management is turned
on the session information, identifier, parameter name and browser method are
stored in the variables called auto, autoID, autoName and autoMethod,
respectively.'''

class session(spyceModule):
  def start(self):
    "Initialise the session module variables."
    self._serverobject = self._api.getServerObject()
    if 'session' not in dir(self._serverobject):
      self._serverobject.session = sessionHandlerRegistry()
    self._handler = None
    self._clearAutoSession()
  def finish(self, theError=None):
    "Save the session, if automatic session management is turned on."
    if self.autoID:
      self.set(self.auto, self.autoExpire, self.autoID)
      if self.autoMethod=='cookie':
        self._api.getModule('cookie').set(self.autoName, self.autoID)
    sessionCleanup(self._serverobject.session)
  def init(self, handler=None, *args, **kwargs):
    if handler: 
      session = apply(self.setHandler, (handler,)+args)
      if kwargs.has_key('auto') and kwargs['auto']:
        auto = kwargs['auto']
        if type(auto) != type(()): 
          auto = (auto,)
        apply(session.autoSession, auto)
  def setHandler(self, file_name, *params):
    "Select a session handler."
    file_name = string.split(file_name, ':')
    if len(file_name)==1: file, name = None, file_name[0]
    else: file, name = file_name[:2]
    if file: handler = self._api.loadModule(name, file, self._api.getFilename())
    else: handler = eval(name)
    self._handler = apply(handler, (self,)+params)
    self._serverobject.session.add(self._handler)
    return self
  def get(self, id):  # method deletes session, if stale
    "Retrieve session information."
    if not self._handler: raise 'call setHandler to initialise'
    return self._handler.get(id)
  def delete(self, id=None):
    "Delete session information."
    if not self._handler: raise 'call setHandler to initialise'
    if not id:
      id = self.autoID
      self._clearAutoSession()
    return self._handler.delete(id)
  def set(self, state, expire, id=None):
    "Set session information."
    if not self._handler: raise 'call setHandler to initialise'
    return self._handler.set(state, expire, id)
  def clear(self):
    "Clear all session information in current handler."
    if not self._handler: raise 'call setHandler to initialise'
    return self._handler.clear()
  def autoSession(self, expire, method='cookie', name='spyceSession'):
    "Turn on automatic session management."
    if not self._handler: raise 'call setHandler to initialise'
    method = string.lower(method)
    if method=='cookie': self.autoID = self._api.getModule('cookie').get(name)
    elif method=='post': self.autoID = self._api.getModule('request').post1(name)
    elif method=='get': self.autoID = self._api.getModule('request').get1(name)
    else: raise 'runtime error: invalid autosession method'
    self.autoMethod = method
    self.autoName = name
    self.autoExpire = expire
    self.auto = None
    if self.autoID:
      self.auto = self.get(self.autoID)
      if not self.auto: self.autoID = None
    if not self.autoID:  # generate a sessionid
      self.autoID = self.set(None, self.autoExpire)
  def _clearAutoSession(self):
    self.auto = None
    self.autoID = None
    self.autoMethod = None
    self.autoName = None
    self.autoExpire = None
    
##################################################
# Cleanup
#

# expire sessions every n requests in expectation
SESSION_EXPIRE_CHECK = 50

class sessionHandlerRegistry:
  "Registry of all used session handlers."
  def __init__(self):
    self.handlers = {}
  def add(self, handler):
    self.handlers[handler.getHandlerID()] = handler
  def list(self):
    return self.handlers.values()
  def remove(self, handler):
    del self.handlers[handler.getHandlerID()]

def sessionCleanup(registry):
  """Iterates through all session handlers and sessions to perform session
  cleanup"""
  if random.randrange(SESSION_EXPIRE_CHECK): return
  for handler in registry.list():
    try:
      sessions = handler.keys()
      for s in sessions:
        handler.get(s)   # will delete stale sessions
    except:
      registry.remove(handler)


##################################################
# Session handlers
#

class sessionHandler:
  '''All session handlers should subclass this, and implement the methods
  marked: 'not implemented'.'''
  def __init__(self, sessionModule):
    self.childnum = sessionModule._api.getServerID()
  def getHandlerID(self):
    raise 'not implemented'
  def get(self, id):  # method should delete, if session is stale
    raise 'not implemented'
  def delete(self, id):
    raise 'not implemented'
  def clear(self):
    raise 'not implemented'
  def set(self, state, expire, id=None):
    raise 'not implemented'
  def keys(self):
    raise 'not implemented'
  def __getitem__(self, key):
    return self.get(key)
  def __delitem__(self, key):
    return self.delete(key)

##################################################
# File-based session handler
#

class session_dir(sessionHandler):
  def __init__(self, sessionModule, dir):
    sessionHandler.__init__(self, sessionModule)
    if not os.path.exists(dir):
      raise "session directory '%s' does not exist" % dir
    self.dir = dir
    self.prefix = 'spy'
    self.BINARY_MODE = 1
  def getHandlerID(self):
    return 'session_dir', self.childnum, self.dir
  def get(self, id):
    if not id: return None
    filename = os.path.join(self.dir, self.prefix+id)
    f=None
    sessionInfo = None
    try:
      f=open(filename, 'r')
      sessionInfo = pickle.load(f)
      f.close()
    except:
      try:
        if f: f.close()
        os.unlink(filename)
      except: pass
    if sessionInfo:
      if time.time() > sessionInfo['expire']:
        self.delete(id)
        return None
      else: return sessionInfo['state']
    else: return None
  def delete(self, id):
    try:
      filename = os.path.join(self.dir, self.prefix+id)
      os.remove(filename)
    except: pass
  def clear(self):
    for id in self.keys():
      self.delete(id)
  def set(self, state, expire, id=None):
    f=None
    try:
      if id:
        filename = os.path.join(self.dir, self.prefix+id)
        f=open(filename, 'w')
      else:
        filename, f, id = openUniqueFile(self.dir, self.prefix, ('%d_' % self.childnum))
      sessionInfo = {}
      sessionInfo['expire'] = int(time.time())+expire
      sessionInfo['state'] = state
      pickle.dump(sessionInfo, f, self.BINARY_MODE)
      f.close()
    except:
      try:
        if f: f.close()
      except: pass
      raise
    return id
  def keys(self):
    sessions = os.listdir(self.dir)
    sessions = filter(lambda s, p=self.prefix: s[:len(p)]==p, sessions)
    sessions = map(lambda s, self=self: s[len(self.prefix):], sessions)
    return sessions

# requires unique (dir, prefix)
def openUniqueFile(dir, prefix, unique, mode='w', max=1000000):
  filelock = spyceLock.fileLock(os.path.join(dir, prefix))
  filelock.lock(1)
  try:
    id = "%06d"%random.randrange(max)
    filename = os.path.join(dir, prefix+unique+id)
    while os.path.exists(filename):
      id = str(random.randrange(max))
      filename = os.path.join(dir, prefix+unique+id)
    f = None
    f = open(filename, mode)
    return filename, f, unique+id
  finally:
    filelock.unlock()

##################################################
# Hash file session handlers
#

class sessionHandlerDBM(sessionHandler):
  def __init__(self, sessionModule, filename):
    sessionHandler.__init__(self, sessionModule)
    self.filename = filename
    self.dbm = None
    self.BINARY_MODE = 1
    self.dbm_type = None  # redefine in subclass
  def getHandlerID(self):
    return 'session_'+self.dbm_type, self.childnum, self.filename
  def _open(self):
    raise 'need to implement'
  def _close(self):
    if self.dbm:
      self.dbm.close()
      self.dbm = None
  def get(self, id):
    if not id: return None
    self._open()
    try:
      expire, state = None, None
      if self.dbm.has_key(id):
        expire, state = pickle.loads(self.dbm[id])
      if expire!=None and time.time() > expire:
        self.delete(id)
        state = None
      return state
    finally:
      self._close()
  def delete(self, id):
    self._open()
    try:
      if self.dbm.has_key(id):
        del self.dbm[id]
    finally:
      self._close()
  def clear(self):
    if os.path.exists(self.filename):
      os.unlink(self.filename)
  def set(self, state, expire, id=None):
    self._open()
    try:
      if not id:
        id = generateKey(self.dbm, self.childnum)
      value = pickle.dumps( (int(time.time())+expire, state), self.BINARY_MODE)
      self.dbm[id] = value
      return id
    finally:
      self._close()
  def keys(self):
    self._open()
    try:
      return self.dbm.keys()
    finally:
      self._close()

def opendb(dbm_session_handler, module, filename, flags):
  mod = __import__(module)
  if not dbm_session_handler.dbm:
    dbm_session_handler.dbm = mod.open(filename, flags)
  
class session_gdbm(sessionHandlerDBM):
  def __init__(self, sessionModule, filename):
    sessionHandlerDBM.__init__(self, sessionModule, filename)
    self.dbm_type = 'gdbm'
  def _open(self):
    opendb(self, self.dbm_type, self.filename, 'cu')

class session_bsddb(sessionHandlerDBM):
  def __init__(self, sessionModule, filename):
    sessionHandlerDBM.__init__(self, sessionModule, filename)
    self.dbm_type = 'bsddb'
  def _open(self):
    opendb(self, 'dbhash', self.filename, 'c')

def generateKey(hash, prefix, max = 1000000):
  prefix = str(prefix)+'_'
  key = random.randrange(max)
  while hash.has_key(prefix+str(key)):
    key = random.randrange(max)
  key = prefix+str(key)
  hash[key] = pickle.dumps(None, 1)
  return key


##################################################
# User callback session handlers
#

class session_user(sessionHandler):
  '''User-callback session handler'''
  def __init__(self, sessionModule, getf, setf, delf, idsf, info=None):
    self.serverID = sessionModule._api.getServerID()
    self.info = info
    self.getf = getf
    self.setf = setf
    self.delf = delf
    self.idsf = idsf
  def getHandlerID(self):
    return 'session_user', self.serverID, self.info
  def get(self, id):  # method should delete, if session is stale
    return self.getf(self.info, id)
  def set(self, state, expire, id):
    return self.setf(self.info, state, expire, self.serverID, id)
  def delete(self, id):
    return self.delf(self.info, id)
  def keys(self):
    return self.idsf(self.info)
  def clear(self):
    for id in self.keys():
      self.delete(id)

##################################################
# database-based session handlers
#

# rimtodo: database-based session handler

