##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import Cookie, time, calendar

__doc__ = """Cookie module gives users full control over browser cookie
functionality. """

class cookie(spyceModule):
  def start(self):
    self._cookie = None
  def get(self, key=None):
    "Get brower cookie(s)"
    if self._cookie == None:
      self._cookie = {}
      cookie = Cookie.SimpleCookie(self._api.getModule('request').env('HTTP_COOKIE'))
      for c in cookie.keys():
        self._cookie[c] = cookie[c].value
    if key:
      if self._cookie.has_key(key):
        return self._cookie[key]
    else: return self._cookie
  def __getitem__(self, key):
    "Get browser cookie(s)"
    return self.get(key)
  def set(self, key, value, expire=None, domain=None, path=None, secure=0):
    "Set browser cookie"
    if value==None:  # delete (set to expire one week ago)
      return self.set(key, '', -60*60*24*7, domain, path, secure)
    text = '%s=%s' % (key, value)
    if expire != None: text = text + ';EXPIRES=%s' % time.strftime(
      '%a, %d-%b-%y %H:%M:%S GMT',
      time.gmtime(time.time()+expire))
    if domain: text = text + ';DOMAIN=%s' % domain
    if path: text = text + ';PATH=%s' % path
    if secure: text = text + ';SECURE'
    self._api.getModule('response').addHeader('Set-Cookie', text)
  def delete(self, key):
    "Delete browser cookie"
    self.set(key, None)
  def __delitem__(self, key):
    "Delete browser cookie"
    return self.delete(self, key)

