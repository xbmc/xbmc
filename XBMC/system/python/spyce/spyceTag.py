##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = '''Spyce tags functionality.'''

import string
import spyceException, spyceModule

##################################################
# Spyce tag library
#

class spyceTagLibrary:
  "All Spyce tag libraries should subclass this."
  def __init__(self, prefix):
    self._prefix = prefix
    self._taghash = {}
    for tag in self.tags:
      self._taghash[tag.name] = tag
  def getTag(self, name, attrs, paired, parent=None):
    return self.getTagClass(name)(self._prefix, attrs, paired, parent)
  def getTagClass(self, name):
    return self._taghash[name]

  # functions to override
  tags = []
  def start(self):
    pass
  def finish(self):
    pass

##################################################
# Spyce tag
#

class spyceTag:
  "All Spyce tags should subclass this."
  def __init__(self, prefix, attrs, paired, parent=None):
    "Initialize a tag; prefix = current library prefix"
    self._prefix = prefix
    self._attrs = attrs
    self._pair = paired
    self._parent = parent
    self._out = None
    self._context = None
    self._buffered = 0
  # setup tag environment (context and output stream)
  def setOut(self, out):
    "Set output stream"
    self._out = out
  def setContext(self, context):
    "Set tag evaluation context"
    self._context = context
  def setBuffered(self, buffered):
    "Set whether tag is running on a buffer wrt. enclosing scope"
    self._buffered = buffered
  # accessors
  def getPrefix(self):
    "Return tag prefix"
    return self._prefix
  def getAttributes(self):
    "Get tag attributes."
    return self._attrs
  def getPaired(self):
    "Return whether this is a paired or singleton tag."
    return self._pair
  def getParent(self, name=None):
    "Get parent tag"
    parent = self._parent
    if name!=None:
      while parent!=None:
        if parent._prefix==self._prefix and parent.name==name: break;
        parent = parent._parent
    return parent
  def getOut(self):
    "Return output stream"
    return self._out
  def getContext(self):
    return self._context
  def getBuffered(self):
    "Get whether tag is running on a buffer wrt. enclosing scope"
    return self._buffered
  # functions and fields to override
  "The name of this tag!"
  name = None
  "Whether this tag wants to buffer its body processing"
  buffer = 0
  "Whether this tag want to conditionally perform body processing"
  conditional = 0
  "Whether this tag wants to possibly loop body processing"
  loops = 0
  "Whether this tag wants to handle exceptions"
  catches = 0
  "Whether end() must (even on exception) get called if begin() completes"
  mustend = 0
  def syntax(self):
    "Check tag syntax"
    pass
  def begin(self, **kwargs):
    "Process start tag; return true to process body (if conditional==1)"
    return 1
  def body(self, contents):
    "Process tag body; return true to repeat (if loops==1)"
    if contents:
      self.getOut().write(contents)
    return 0
  def end(self):
    "Process end tag"
    pass
  def catch(self, ex):
    "Process any exception thrown by tag (if catches==1)"
    raise

class spyceTagPlus(spyceTag):
  "An easier spyceTag class to work with..."
  # tag context helpers
  def contextSet(self, name, (exists, value)): 
    "Set a variable in the context"
    prev = self.contextGet(name) 
    if exists: self._context[name] = value 
    else: del self._context[name] 
    return prev 
  def contextGet(self, name): 
    "Get a variable from the context" 
    try: return 1, self._context[name] 
    except KeyError: return 0, None 
  def contextEval(self, expr): 
    "Evaluate an expression within the context" 
    if expr and expr[0]=='=': 
      expr = eval(expr[1:], self._context) 
    return expr
  def contextEvalAttrs(self, attrs):
    "Evaluate attribute dictionary within context" 
    attrs2 = {} 
    for name in attrs.keys(): 
      attrs2[name] = self.contextEval(attrs[name]) 
    return attrs2
  def contextGetModule(self, name): 
    "Return a Spyce module reference" 
    try: return self._context[name]
    except KeyError: 
      return self._context['taglib']._api.getModule(name)

  # tag syntax checking helpers
  def syntaxExist(self, *must):
    "Ensure that certain attributes exist"
    for attr in must:
      if not self._attrs.has_key(attr):
        raise spyceTagSyntaxException('missing compulsory "%s" attribute' % attr)
  def syntaxExistOr(self, *mustgroups):
    "Ensure that one of a group of attributes must exist"
    errors = []
    success = 0
    for must in mustgroups:
      try: 
        if must==type(''): must = (must,)
        self.apply(self.syntaxExist, must)
        success = success + 1
      except spyceTagSyntaxException, e:
        errors.append(str(e))
    if not success:
      raise spyceTagSyntaxException(string.join(errors, ' OR '))
  def syntaxExistOrEx(self, *mustgroups):
    success = apply(self.syntaxExistOr, mustgroups)
    if success > 1:
      raise spyceTagSyntaxException('only one set of the following groups of tags are allowed: %s', string.join(map(repr, mustgroups), ', '))
  def syntaxNonEmpty(self, *names):
    for name in names:
      try: value = self._attrs[name]
      except KeyError: return
      if not value:
        raise spyceTagSyntaxException('attribute "%s" should not be empty', name)
  def syntaxValidSet(self, name, validSet):
    try: value = self._attrs[name]
    except KeyError: return
    if value not in validSet:
      raise spyceTagSyntaxException('attribute "%s" should be one of: %s'% (name, string.join(validSet, ', ')))
  def syntaxPairOnly(self):
    "Ensure that this tag is paired i.e. open/close"
    if not self._pair:
      raise spyceTagSyntaxException('singleton tag not allowed')
  def syntaxSingleOnly(self):
    "Ensure that this tag is single i.e. <foo/>"
    if self._pair:
      raise spyceTagSyntaxException('paired tag not allowed')


##################################################
# Spyce tag syntax checking
#

class spyceTagChecker:
  def __init__(self, server):
    self._server = server
    self._taglibs = {}
    self._stack = []
  def loadLib(self, libname, libfrom, libas, rel_file, info=None):
    if not libas: libas = libname
    try: 
      self._taglibs[(libname, libfrom)] = \
        self._server.loadModule(libname, libfrom, rel_file)(libas)
    except (SyntaxError, TypeError):
      raise
    except:
      raise spyceException.spyceSyntaxError(
        'unable to load module: %s'%libname, info)
  def getTag(self, (libname,libfrom), name, attrs, pair, info):
    lib = self._taglibs[(libname, libfrom)]
    try:
      return lib.getTag(name, attrs, pair, None)
    except:
      raise spyceException.spyceSyntaxError(
        'unknown tag "%s:%s"'%(libname, name), info)
  def getTagClass(self, (libname, libfrom), name, info):
    lib = self._taglibs[(libname, libfrom)]
    try:
      return lib.getTagClass(name)
    except:
      raise spyceException.spyceSyntaxError(
        'unknown tag "%s:%s"'%(libname, name), info)
  def startTag(self, (libname,libfrom), name, attrs, pair, info):
    tag = self.getTag((libname, libfrom), name, attrs, pair, info)
    try:
      error = tag.syntax()
    except spyceTagSyntaxException, e:
      raise spyceException.spyceSyntaxError(str(e), info)
    if error:
      raise spyceException.spyceSyntaxError(error, info)
    if pair:
      self._stack.append( (libname, libfrom, name, info) )
  def endTag(self, (libname,libfrom), name, info):
    try:
      libname1, libfrom1, name1, info1 = self._stack.pop()
    except IndexError:
      raise spyceException.spyceSyntaxError(
        'unmatched close tag', info)
    if (libname1,libfrom1,name1) != (libname,libfrom,name): 
      raise spyceException.spyceSyntaxError(
        'unmatched close tag, expected <%s:%s>' % (libname1,name1), info)
  def finish(self):
    if self._stack:
      libname, libfrom, name, info = self._stack.pop()
      raise spyceException.spyceSyntaxError(
        'unmatched open tag', info)

##################################################
# Spyce tag syntax exception
#

class spyceTagSyntaxException:
  def __init__(self, str):
    self._str = str
  def __repr__(self):
    return self._str

