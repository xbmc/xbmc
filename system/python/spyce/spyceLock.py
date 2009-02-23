##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import os

__doc__ = 'Spyce locking-related functions'

##################################################
# Generic lock
#

class genericLock:
  def lock(self, block=1):
    "return true, if lock acquired"
    raise 'not implemented'
  def unlock(self):
    raise 'not implemented'
  def locked(self):
    raise 'not implemented'

##################################################
# Dummy lock
#

class dummyLock(genericLock):
  def lock(self, block=1):
    return 1
  def unlock(self):
    pass
  def locked(self):
    return 0

##################################################
# Thread locking
#

class threadLock(genericLock):
  def __init__(self):
    import thread
    self._thelock = thread.allocate_lock()
  def lock(self, block=1):
    if block: return self._thelock.acquire()
    else: return self._thelock.acquire(0)
  def unlock(self):
    return self._thelock.release()
  def locked(self):
    return self._thelock.locked()

##################################################
# File locking
#

# Adapted from portalocker.py, written by Jonathan Feinberg <jdf@pobox.com>
# Used in rimap (http://rimap.sourceforge.net) before Spyce
# Methods:
#   file_lock(file, flags)
#   file_unlock(file)
# Constants: LOCK_EX, LOCK_SH, LOCK_NB
# -- RB

try:
  if os.name=='nt':
    import win32con, win32file, pywintypes
    LOCK_EX = win32con.LOCKFILE_EXCLUSIVE_LOCK
    LOCK_SH = 0 # the default
    LOCK_NB = win32con.LOCKFILE_FAIL_IMMEDIATELY
    # is there any reason not to reuse the following structure?
    __overlapped = pywintypes.OVERLAPPED()
    def file_lock(file, flags):
      hfile = win32file._get_osfhandle(file.fileno())
      win32file.LockFileEx(hfile, flags, 0, 0xffff0000L, __overlapped)
    def file_unlock(file):
      hfile = win32file._get_osfhandle(file.fileno())
      win32file.UnlockFileEx(hfile, 0, 0xffff0000L, __overlapped)
  elif os.name == 'posix':
    import fcntl
    LOCK_EX = fcntl.LOCK_EX
    LOCK_SH = fcntl.LOCK_SH
    LOCK_NB = fcntl.LOCK_NB
    def file_lock(file, flags):
      fcntl.flock(file.fileno(), flags)
    def file_unlock(file):
      fcntl.flock(file.fileno(), fcntl.LOCK_UN)
  else:
    raise 'locking not supported on this platform'
except:
  LOCK_EX = 0
  LOCK_SH = 0
  LOCK_NB = 0
  # bring on the race conditions! :)
  def file_lock(file, flags): pass
  def file_unlock(file): pass

class fileLock(genericLock):
  f=name=None
  def __init__(self, name):
    self.name=name+'.lock'
    self._locked = 0
  def lock(self, block=1):
    self.f=open(self.name, 'w')
    if block: file_lock(self.f, LOCK_EX)
    else: file_lock(self.f, LOCK_EX or LOCK_NB)
    self._locked = 1
  def unlock(self):
    try:
      if not self.f: return
      file_unlock(self.f)
      self.f.close()
      os.unlink(self.name)
      self.f=None
      self._locked = 0
    except: pass
  def locked(self):
    return self._locked

