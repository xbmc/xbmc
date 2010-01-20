##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule

__doc__ = """Pool module supports the creation of server-pooled objects. The
single pool is shared among all Spyce execution context residing on a given
server, and remains until the server dies. It is often useful to store
database connections, and other variables that are expensive to compute on a
per-request basis. """

class pool(spyceModule):
  def start(self):
    "Define or retrieve the pool."
    self._serverobject = self._api.getServerObject()
    if 'pool' not in dir(self._serverobject):
      self._serverobject.pool = {}
    self.server = self._api.getServerGlobals()
  def __getitem__(self, key):
    "Get an item from the pool."
    return self._serverobject.pool[key]
  def __setitem__(self, key, value):
    "Set an item in the pool."
    self._serverobject.pool[key] = value
  def __delitem__(self, key):
    "Delete an item in the pool."
    del self._serverobject.pool[key]
  def keys(self):
    "Return the pool hash keys."
    return self._serverobject.pool.keys()
  def values(self):
    "Return the pool hash values."
    return self._serverobject.pool.values()
  def has_key(self, key):
    "Test of existence of key in pool."
    return self._serverobject.pool.has_key(key)
  def clear(self):
    "Purge the pool of all items."
    return self._serverobject.pool.clear()

