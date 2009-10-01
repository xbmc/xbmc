##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import md5, binascii, os, string
try: from cPickle import dumps, loads
except: from pickle import dumps, loads

__doc__ = '''Caching related functionality.'''

# rimtodo: specify some sort of cache size limit

##################################################
# Generic cache
#

class cache:
  "Generic cache"
  def __getitem__(self, key):
    raise 'not implemented'
  def __setitem__(self, key, value):
    raise 'not implemented'
  def __delitem__(self, key):
    raise 'not implemented'
  def keys(self):
    raise 'not implemented'
  def has_key(self, key):
    raise 'not implemented'

##################################################
# Storage caches
#

class memoryCache(cache):
  "In-memory cache"
  def __init__(self, infoStr=None):
    self.cache = {}
    self.info = infoStr
  def __getitem__(self, key):
    return self.cache[key]
  def __setitem__(self, key, value):
    self.cache[key]=value
  def __delitem__(self, key):
    del self.cache[key]
  def keys(self):
    return self.cache.keys()
  def has_key(self, key):
    return self.cache.has_key(key)

class fileCache(cache):
  "File-based cache"
  def __init__(self, infoStr):
    self._cachedir = string.strip(infoStr)
  def __getitem__(self, key):
    filename = os.path.join(self._cachedir, self._encodeKey(key))
    f = None
    try:
      try:
        f = open(filename, 'r')
        return loads(f.read())
      finally:
        if f: f.close()
    except IOError: pass
    except EOFError: pass
    raise KeyError()
  def __setitem__(self, key, value):
    try:
      if self[key]==value: return
    except KeyError: pass
    filename = os.path.join(self._cachedir, self._encodeKey(key))
    f = None
    try:
      f = open(filename, 'w')
      f.write(dumps(value,1))
    finally:
      if f: f.close()
  def __delitem__(self, key):
    filename = os.path.join(self._cachedir, self._encodeKey(key))
    if os.path.exists(filename):
      os.remove(filename)
  def keys(keys):
    raise 'not implemented'
  def has_key(self, key):
    try:
      self[key]
      return 1
    except KeyError:
      return 0
  def _encodeKey(self, key):
    return 'spyceCache-'+binascii.hexlify(md5.new(str(key)).digest())
    

##################################################
# Policy caches
#

#rimtodo:

##################################################
# Semantic cache
#

class semanticCache(cache):
  """Cache that knows how to validate and generate its own data. Note, that the
  cache stores elements as (validity, data) tuples. The valid is a function
  invoked as valid(key,validity), returning a boolean; and generate is a
  function invoked as generate(key) returning (validity, data). The get()
  method returns only the data."""
  def __init__(self, cache, valid, generate):
    self.valid = valid
    self.generate = generate
    self.cache = cache
  def get(self, key):
    "Get (or generate) a cache element."
    if self.cache:
      if not self.cache.has_key(key) or not self.valid(key, self.cache[key][0]):
        self.cache[key] = self.generate(key)
      return self.cache[key][1]
    else:
      return self.generate(key)[1]
  def purge(self, key):
    "Remove a cache element, if it exists."
    if self.cache.has_key(key):
      del self.cache[key]
  # standard dictionary methods
  def __getitem__(self, key):
    return self.get(key)
  def __delitem__(self, key):
    return self.purge(key)
  def has_key(self, key):
    if self.cache:
      return self.cache.has_key()
    else:
      return 0
  def keys(self):
    if self.cache:
      return self.cache.keys()
    else:
      return []
  def values(self):
    if self.cache:
      return map(lambda x: x[1], self.cache.values())
    else:
      return []
  def clear(self):
    if self.cache:
      self.cache.clear()

