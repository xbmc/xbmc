##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import cgi, string, urlparse, spyceUtil

__doc__ = """Request module provides access to the browser request
information. """

class request(spyceModule):
  def start(self):
    "Initialise module variables"
    self._get = None
    self._postfields = None
  def uri(self, component=None):
    "Return request URI, or URI component"
    theuri = self._api.getRequest().env()['REQUEST_URI']
    if not component:
      return theuri
    else:
      component = string.lower(component)
      if component == 'scheme': component = 0
      elif component == 'location': component = 1
      elif component == 'path': component = 2
      elif component == 'parameters': component = 3
      elif component == 'query': component = 4
      elif component == 'fragment': component = 5
      else: raise 'unknown uri component'
      return urlparse.urlparse(theuri)[component]
  def uri_scheme(self):
    "Return request URI scheme, ie. http (usually)"
    return urlparse.urlparse(self.uri())[0]
  def uri_location(self):
    "Return request URI scheme, ie. http (usually)"
    return urlparse.urlparse(self.uri())[1]
  def uri_path(self):
    "Return request URI path component"
    return urlparse.urlparse(self.uri())[2]
  def method(self):
    "Return request method: get/post/..."
    return string.upper(self._api.getRequest().env()['REQUEST_METHOD'])
  def query(self):
    "Return request query string"
    return self._api.getRequest().env()['QUERY_STRING']
  def filename(self, relative=None):
    "Return original Spyce filename"
    myfile = self._api.getFilename()
    if relative==None:
      return myfile
    else:
      return os.path.normpath(os.path.join(os.path.dirname(myfile), relative))
  def default(self, value, value2):
    "Return value, or value2 if value==None"
    if value==None: return value2
    return value
  def _getInit(self):
    if self._get==None:
      self._get = cgi.parse_qs(self.query(), 1)
      self._get1 = {}
      self._getL = {}
      self._getL1 = {}
      for key in self._get.keys():
        self._getL[string.lower(key)] = []
      for key in self._get.keys():
        keyL = string.lower(key)
        self._getL[keyL] = self._getL[keyL] + self._get[key]
      for key in self._get.keys():
        self._get1[key] = self._get[key][0]
      for keyL in self._getL.keys():
        self._getL1[keyL] = self._getL[keyL][0]
  def get(self, name=None, default=None, ignoreCase=0):
    "Return GET parameter(s) list(s)"
    self._getInit()
    if ignoreCase:
      if name: name = string.lower(name)
      value = spyceUtil.extractValue(self._getL, name)
    else:
      value = spyceUtil.extractValue(self._get, name)
    return self.default(value, default)
  def get1(self, name=None, default=None, ignoreCase=0):
    "Return single GET parameter(s)"
    self._getInit()
    if ignoreCase:
      if name: name = string.lower(name)
      value = spyceUtil.extractValue(self._getL1, name)
    else:
      value = spyceUtil.extractValue(self._get1, name)
    return self.default(value, default)
  def _postInit(self):
    if self._postfields==None:
      if hasattr(self._api.getRequest(), 'spycepostinfo'):
        # stream was already parsed (possibly this is an internal redirect)
        (self._post, self._post1, self._file, 
          self._postL, self._postL1, self._fileL, 
          self._postfields) = self._api.getRequest().spycepostinfo
        return
      self._post = {}
      self._post1 = {}
      self._file = {}
      self._postL = {}
      self._postL1 = {}
      self._fileL = {}
      self._postfields={}
      try: len = int(str(self.env('CONTENT_LENGTH')))
      except: len=0
      if self.method()=='POST' and len:
        self._postfields = cgi.FieldStorage(fp=self._api.getRequest(), environ=self.env(), keep_blank_values=1)
        for key in self._postfields.keys():
          if type(self._postfields[key]) == type( [] ):
            self._post[key] = map(lambda attr: attr.value, self._postfields[key])
            self._post1[key] = self._post[key][0]
          elif not self._postfields[key].filename:
            self._post[key] = [self._postfields[key].value]
            self._post1[key] = self._post[key][0]
          else:
            self._file[key] = self._fileL[string.lower(key)] = self._postfields[key]
        for key in self._post.keys():
          self._postL[string.lower(key)] = []
        for key in self._post.keys():
          keyL = string.lower(key)
          self._postL[keyL] = self._postL[keyL] + self._post[key]
        for keyL in self._postL.keys():
          self._postL1[keyL] = self._postL[keyL][0]
      # save parsed information in request object to prevent reparsing (on redirection)
      self._api.getRequest().spycepostinfo = (self._post, self._post1, self._file, 
        self._postL, self._postL1, self._fileL, self._postfields)
  def post(self, name=None, default=None, ignoreCase=0):
    "Return POST parameter(s) list(s)"
    self._postInit()
    if ignoreCase:
      if name: name = string.lower(name)
      value = spyceUtil.extractValue(self._postL, name)
    else:
      value = spyceUtil.extractValue(self._post, name)
    return self.default(value, default)
  def post1(self, name=None, default=None, ignoreCase=0):
    "Return single POST parameter(s)"
    self._postInit()
    if ignoreCase:
      if name: name = string.lower(name)
      value = spyceUtil.extractValue(self._postL1, name)
    else:
      value = spyceUtil.extractValue(self._post1, name)
    return self.default(value, default)
  def file(self, name=None, ignoreCase=0):
    "Return POSTed file(s)"
    self._postInit()
    if ignoreCase:
      if name: name = string.lower(name)
      return spyceUtil.extractValue(self._fileL, name)
    else:
      return spyceUtil.extractValue(self._file, name)
  def env(self, name=None, default=None):
    "Return other request (CGI) environment variables"
    return self.default(self._api.getRequest().env(name), default)
  def getHeader(self, type=None):
    "Return browser HTTP header(s)"
    return self._api.getRequest().getHeader(type)
  def __getitem__(self, key):
    if type(key) == type(0):
      return self.getpost().keys()[key]
    else:
      v = self.get1(key)
      if v!=None: return v
      v = self.post1(key)
      if v!=None: return v
      v = self.file(key)
      if v!=None: return v
  def __repr__(self):
    return ''
  def __multidict(self, *args):
    args = list(args)
    args.reverse()
    dict = {}
    for d in args:
      for k in d.keys():
        dict[k] = d[k]
    return dict
  def getpost(self, name=None, default=None, ignoreCase=0):
    "Return get() if not None, otherwise post() if not None, otherwise default"
    if name==None:
      self._getInit()
      self._postInit()
      return self.__multidict(self._get, self._post)
    else:
      value = self.get(name, None, ignoreCase)
      if value==None: value = self.post(name, default, ignoreCase)
      return value
  def getpost1(self, name=None, default=None, ignoreCase=0):
    "Return get1() if not None, otherwise post1() if not None, otherwise default"
    if name==None:
      self._getInit()
      self._postInit()
      return self.__multidict(self._get1, self._post1)
    else:
      value = self.get1(name, None, ignoreCase)
      if value==None: value = self.post1(name, default, ignoreCase)
      return value
  def postget(self, name=None, default=None, ignoreCase=0):
    "Return post() if not None, otherwise get() if not None, otherwise default"
    if name==None:
      self._getInit()
      self._postInit()
      return self.__multidict(self._post, self._get)
    else:
      value = self.post(name, None, ignoreCase)
      if value==None: value = self.get(name, default, ignoreCase)
      return value
  def postget1(self, name=None, default=None, ignoreCase=0):
    "Return post1() if not None, otherwise get1() if not None, otherwise default"
    if name==None:
      self._getInit()
      self._postInit()
      return self.__multidict(self._post1, self._get1)
    else:
      value = self.post1(name, None, ignoreCase)
      if value==None: value = self.get1(name, default, ignoreCase)
      return value

