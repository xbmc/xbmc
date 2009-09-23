##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

#rimtodo:
# - fix compaction (it assumed newlines parsed independently)
# - active tags

#try:
#  exec('import sre as re')  # due to stack limitations of sre
#  # exec to be backwards compatible with Python 1.5
#except:
#  import re
import re  # otherwise apache 2.0 pcre library conflicts
           # we just can't win! either stack limits (sre), or 
           # library conflicts (pre)! :)

from cStringIO import StringIO
import sys, string, token, tokenize, os
import spyceTag, spyceException, spyceUtil


__doc__ = '''Compile Spyce files into Python code.'''

##################################################
# Special method names
#

SPYCE_CLASS = 'spyceImpl'
SPYCE_INIT_FUNC = 'spyceStart'
SPYCE_DESTROY_FUNC = 'spyceFinish'
SPYCE_PROCESS_FUNC = 'spyceProcess'
SPYCE_GLOBAL_CODE = '__SPYCE_GLOBAL_CODE_CONSTANT'
SPYCE_WRAPPER = '_spyceWrapper'
DEFAULT_CODEPOINT = [SPYCE_PROCESS_FUNC]

##################################################
# Dos-to-Unix linebreaks
#

# split a buffer into lines (regardless of terminators)
def splitLines(buf):
  lines=[]
  f=StringIO(buf)
  l=f.readline()
  while len(l):
    while l and l[-1] in ['\r', '\n']:
      l=l[:-1]
    lines.append(l)
    l=f.readline()
  return lines

# encode document with LF
def CRLF2LF(s):
  return string.join(splitLines(s), '\n')+'\n'

# encode document with CRLF
def LF2CRLF(s):
  return string.join(splitLines(s), '\r\n')+'\r\n'


##################################################
# Tokens
#

T_ESC      = -2
T_EOF      = -1
T_TEXT     = 0
T_EVAL     = 1
T_STMT     = 2
T_CHUNK    = 3
T_CHUNKG   = 4
T_DIRECT   = 5
T_LAMBDA   = 6
T_END      = 7
T_CMNT     = 8
T_END_CMNT = 9

TOKENS = (
  # in the order that they should be tested
  # (i.e. usually longest first)
  (T_ESC,      r'\\\[\[', r'\\<%', r'\\\]\]', r'\\%>'),  # escapes
  (T_CHUNK,    r'\[\[\\', r'<%\\'),                      # open chunk
  (T_CHUNKG,   r'\[\[\\\\', r'<%\\\\'),                  # open global chunk
  (T_EVAL,     r'\[\[=', r'<%='),                        # open eval
  (T_DIRECT,   r'\[\[\.', r'<%\.', r'<%@'),              # open directive
  (T_LAMBDA,   r'\[\[spy', r'<%spy'),                    # open lambda
  (T_CMNT,     r'\[\[--', r'<%--'),                      # open comment
  (T_END_CMNT, r'--\]\]', r'--%>'),                      # close comment
  (T_STMT,     r'\[\[', r'<%'),                          # open statement
  (T_END,      r'\]\]', r'%>'),                          # close
)

def genTokensRE(tokens):
  regexp = []
  typelookup = [None,]
  for group in tokens:
    type, matchstrings = group[0], group[1:]
    for s in matchstrings:
      regexp.append('(%s)' % s)
      typelookup.append(type)
  regexp = string.join(regexp, '|')
  return re.compile(regexp, re.M), typelookup

RE_TOKENS = None
TOKEN_TYPES = None
if not RE_TOKENS:
  RE_TOKENS, TOKEN_TYPES = genTokensRE(TOKENS)

def spyceTokenize(buf):
  # scan using regexp
  tokens = []
  buflen = len(buf)
  pos = 0
  brow = bcol = erow = ecol = 0
  while pos<buflen:
    m = RE_TOKENS.search(buf, pos)
    try:
      mstart, mend = m.start(), m.end()
      other, token = buf[pos:mstart], buf[mstart:mend]
      if other:
        tokens.append((T_TEXT, other, pos, mstart))
      try:
        type = TOKEN_TYPES[m.lastindex]
      except AttributeError, e: 
        # Python 1.5 does not support lastindex
        lastindex = 1
        for x in m.groups():
          if x: break
          lastindex = lastindex + 1
        type = TOKEN_TYPES[lastindex]
      if type==T_ESC:
        token = token[1:]
        type = T_TEXT
      tokens.append((type, token, mstart, mend))
      pos = mend
    except AttributeError, e:
      # handle text before EOF...
      other = buf[pos:]
      if other:
        tokens.append((T_TEXT, other, pos, buflen))
      pos = buflen
  # compute row, col
  brow, bcol = 1, 0
  tokens2 = []
  for type, text, begin, end in tokens:
    lines = string.split(text[:-1], '\n')
    numlines = len(lines)
    erow = brow + numlines - 1
    ecol = bcol
    if numlines>1: ecol = 0
    ecol = ecol + len(lines[-1])
    tokens2.append((type, text, (brow, bcol), (erow, ecol)))
    if text[-1]=='\n':
      brow = erow + 1
      bcol = 0
    else:
      brow = erow
      bcol = ecol + 1
  return tokens2


def spyceTokenize4Parse(buf):
  # add eof and reverse (so that you can pop() tokens)
  tokens = spyceTokenize(buf)
  try:
    _, _, _, end = tokens[-1]
  except:
    end = 0;
  tokens.append((T_EOF, '<EOF>', end, end))
  tokens.reverse()
  return tokens

def processMagic(buf):
  if buf[:2]=='#!':
    buf = string.join(string.split(buf, '\n')[1:], '\n')
  return buf

##################################################
# Directives / Active Tags / Multi-line quotes
#

DIRECTIVE_NAME = re.compile('[a-zA-Z][-a-zA-Z0-9_:]*')
DIRECTIVE_ATTR = re.compile(
    r'\s*([a-zA-Z_][-.:a-zA-Z_0-9]*)(\s*=\s*'
    r'(\'[^\']*\'|"[^"]*"|[-a-zA-Z0-9./:;+*%?!&$\(\)_#=~]*))?')
def parseDirective(text):
  "Parse a Spyce directive into name and an attribute list."
  attrs = {}
  match = DIRECTIVE_NAME.match(text)
  if not match: return None, {}
  name = string.lower(text[:match.end()])
  text = string.strip(text[match.end()+1:])
  while text:
    match = DIRECTIVE_ATTR.match(text)
    if not match: break
    attrname, rest, attrvalue = match.group(1, 2, 3)
    if not rest: attrvalue = None
    elif attrvalue[:1] == "'" == attrvalue[-1:] or \
        attrvalue[:1] == '"' == attrvalue[-1:]:
      attrvalue = attrvalue[1:-1]
    attrs[string.lower(attrname)] = attrvalue
    text = text[match.end()+1:]
  return name, attrs

RE_LIB_TAG = re.compile(r'''<          # beginning of tag
  (?P<end>/?)                          # ending tag
  (?P<lib>[a-zA-Z][-.a-zA-Z0-9_]*):    # lib name
  (?P<name>[a-zA-Z][-.a-zA-Z0-9_]*)    # tag name
  (?P<attrs>(?:\s+                     # attributes
    (?:[a-zA-Z_][-.:a-zA-Z0-9_]*       # attribute name
      (?:\s*=\s*                       # value indicator
        (?:'[^']*'                     # LITA-enclosed value
          |"[^"]*"                     # LIT-enclosed value
          |[^'">\s]+                   # bare value
        )
      )?
    )
  )*)
  \s*                                  # trailing whitespace
  (?P<single>/?)                       # single / unpaired tag
  >''', re.VERBOSE)                    # end of tag
def calcEndPos(begin, str):
  if not str: raise 'empty string'
  beginrow, begincol = begin
  eol = 0
  if str[-1]=='\n': 
    str = str[:-1]+' '
    eol = 1
  lines = string.split(str, '\n')
  endrow = beginrow + len(lines)-1
  if endrow!=beginrow:
    begincol = 0
  endcol = begincol + len(lines[-1]) - 1
  beginrow, begincol = endrow, endcol + 1
  if eol:
    begincol = 0
    beginrow = beginrow + 1
  return (endrow, endcol), (beginrow, begincol)

RE_MULTI_LINE_QUOTE_BEGIN = re.compile(r'r?'+"(''')|"+'(""")')
def removeMultiLineQuotes(s):
  def findMultiLineQuote(s):
    quotelist = []
    def eatToken(type, string, begin, end, _, quotelist=quotelist):
      if type == token.STRING and RE_MULTI_LINE_QUOTE_BEGIN.match(string):
        quotelist.append((string, begin,end))
    tokenize.tokenize(StringIO(s).readline, eatToken)
    return quotelist
  def replaceRegionWithLine(s, begin, end, s2):
    (beginrow, begincol), (endrow, endcol) = begin, end
    beginrow, endrow = beginrow-1, endrow-1
    s = string.split(s, '\n')
    s1, s3 = s[:beginrow], s[endrow+1:]
    s2 = s[beginrow][:begincol] + s2 + s[endrow][endcol:]
    return string.join(s1 + [s2] + s3, '\n')
  match = findMultiLineQuote(s)
  offsets = {}
  for _, (obr, _), (oer, _) in match:
    offsets[obr] = oer - obr
  while match:
    s2, begin, end = match[0]
    s = replaceRegionWithLine(s, begin, end, `eval(s2)`)
    match = findMultiLineQuote(s)
  return s, offsets

##################################################
# Pre-Python AST
#

# ast node types
AST_PY      = 0
AST_PYEVAL  = 1
AST_TEXT    = 2
AST_COMPACT = 3

# compacting modes
COMPACT_OFF   = 0
COMPACT_LINE  = 1
COMPACT_SPACE = 2
COMPACT_FULL  = 3

class ppyAST:
  "Generate a pre-Python AST"
  def __init__(self):
    "Initialise parser data structures, AST, token handlers, ..."
    # set up ast
    self._code = { 
      'elements': {}, 
      'leafs': [], 
    }
    self._codepoint = self._code
    self._codepointname = []
    self._mods = []
    self._taglibs = {}
  def getCode(self):
    return self._code
  def getModules(self):
    return self._mods
  def getTaglibs(self):
    return self._taglibs
  # routines to navigate AST
  def selectCodepoint(self, codepointname):
    code = self._code
    for point in codepointname:
      code = code['elements'][point]
    self._codepoint = code
    self._codepointname = codepointname
  def getCodepoint(self):
    return self._codepointname
  def descendCodepoint(self, codepointSuffix):
    self._codepointname.append(codepointSuffix)
    self.selectCodepoint(self._codepointname)
  def ascendCodepoint(self):
    suffix = self._codepointname.pop()
    self.selectCodepoint(self._codepointname)
    return suffix
  # routines that modify the ast
  def appendCodepoint(self, codepointSuffix, firstline, ref=None):
    self._codepoint['elements'][codepointSuffix] = {
      'elements': {},
      'leafs': [],
    }
    self.descendCodepoint(codepointSuffix)
    self.addCode(string.strip(firstline), ref) # note: firstline is not indented
  def addCode(self, code, ref=None):
    self._codepoint['leafs'].append((AST_PY, code, ref))
  def addGlobalCode(self, code, ref=None):
    codepoint = self.getCodepoint()
    self.selectCodepoint([SPYCE_GLOBAL_CODE])
    self.addCode(code+'\n', ref)
    self.selectCodepoint(codepoint)
    pass
  def addEval(self, eval, ref=None):
    self._codepoint['leafs'].append((AST_PYEVAL, eval, ref))
  def addCodeIndented(self, code, ref=None, globalcode=0):
    code, replacelist = removeMultiLineQuotes(code)
    # funky hack: put in NULLs to preserve indentation
    #   NULLs don't appear in code, and the BraceConverter will
    #   turn them back into spaces. If we leave them as spaces,
    #   BraceConverter is just going to ignore them and pay attention
    #   only to the braces. (not the best compile-time performance!)
    code = string.split(code, '\n')
    code = map(lambda l: (len(l)-len(string.lstrip(l)), l), code)
    code = map(lambda (indent, l): chr(0)*indent + l, code)
    code.append('')
    # split code lines
    (brow, bcol), (erow, ecol), text, file = ref
    row = brow
    for l in code:
      cbcol = 0
      cecol = len(l)
      if row==brow: cbcol = bcol
      if row==erow: cecol = ecol
      try: row2 = row + replacelist[row-brow+1]
      except: row2 = row
      ref = (row, cbcol), (row2, cecol), l, file
      if globalcode: self.addGlobalCode(l, ref)
      else: self.addCode(l, ref)
      row = row2 + 1
  def addText(self, text, ref=None):
    self._codepoint['leafs'].append((AST_TEXT, text, ref))
  def addCompact(self, compact, ref):
    self._codepoint['leafs'].append((AST_COMPACT, compact, ref))
  def addModule(self, modname, modfrom, modas):
    self._mods.append((modname, modfrom, modas))
  def addTaglib(self, libname, libfrom=None, libas=None):
    if not libas: libas=libname
    self._taglibs[libas] = libname, libfrom


##################################################
# Parse
#

class spyceParse:
  def __init__(self, server, buf, filename, sig):
    try:
      # initialization
      self._tagChecker = spyceTag.spyceTagChecker(server)
      self._load_spylambda = 0
      self._load_taglib = 0
      self._curdir, self._curfile = os.getcwd(), '<string>'
      if filename:
        self._curdir, self._curfile = os.path.split(filename)
      if not self._curdir:
        self._curdir = os.getcwd()
      # prime ast
      self._ast = ppyAST()
      self._ast.selectCodepoint([])
      self._ast.appendCodepoint(SPYCE_GLOBAL_CODE, '')
      self._ast.addGlobalCode('''
try:
  exec('from __future__ import nested_scopes')
except: pass
from spyceException import spyceDone, spyceRedirect, spyceRuntimeException
  ''')
      # define spyceProcess
      self._ast.selectCodepoint([])
      self._ast.appendCodepoint(SPYCE_PROCESS_FUNC, 'def '+SPYCE_PROCESS_FUNC+'('+sig+')')
      # spyceProcess pre
      self._ast.selectCodepoint(DEFAULT_CODEPOINT)
      self._ast.addCode('try:{', None)
      self._ast.addCode('pass', None)
      # spyceProcess body
      self._tokens = spyceTokenize4Parse(processMagic(buf))
      self._tokenType = None
      self.popToken()
      self.processSpyce()
      # spyceProcess post
      self._ast.addCode('} except spyceDone: pass', None)
      self._ast.addCode('except spyceRedirect: raise', None)
      self._ast.addCode('except KeyboardInterrupt: raise', None)
      self._ast.addCode('except:{ raise spyceRuntimeException(%s) }'%SPYCE_WRAPPER, None)
      # post processing
      if self._load_taglib: self._ast.addModule('taglib', None, None)
      if self._load_spylambda: self._ast.addModule('spylambda', None, None)
      self._tagChecker.finish()
    except spyceException.spyceSyntaxError, e:
      raise
      if e.info:
        begin, end, text, _ = e.info
        e.info = begin, end, text, self._curfile
      raise e
  def info(self):
    return self._ast.getCode(), self._ast.getModules()
  def popToken(self):
    if self._tokenType!=T_EOF:
      self._tokenType, self._tokenText, self._tokenBegin, self._tokenEnd = self._tokens.pop()

  def processSpyce(self):
    while self._tokenType!=T_EOF:
      [
        self.processText,        # T_TEXT
        self.processEval,        # T_EVAL
        self.processStmt,        # T_STMT
        self.processChunk,       # T_CHUNK
        self.processGlobalChunk, # T_CHUNKG
        self.processDirective,   # T_DIRECT
        self.processUnexpected,  # T_LAMBDA
        self.processUnexpected,  # T_END
        self.processComment,     # T_CMNT
        self.processUnexpected,  # T_END_CMNT
      ][self._tokenType]()
      self.popToken()
  def processComment(self):
    # collect comment
    self.popToken()
    while self._tokenType not in [T_END_CMNT, T_EOF]:
      self.popToken()
    if self._tokenType==T_EOF:
      self.processUnexpected()
  def processText(self):
    "Process HTML (possibly with some active tags)"
    html, begin, end = self._tokenText, self._tokenBegin, self._tokenEnd
    m = RE_LIB_TAG.search(html)
    while m:
      plain = html[:m.start()]
      if plain:
        plain_end, tag_begin = calcEndPos(begin, plain)
        self._ast.addText(plain, (begin, plain_end, '<html string>', self._curfile))
      else: tag_begin = begin
      tag = m.group(0)
      tag_end, begin = calcEndPos(tag_begin, tag)
      self.processActiveTag(tag, 
        not not m.group('end'), m.group('lib'), m.group('name'), 
        m.group('attrs'), not m.group('single'),
        tag_begin, tag_end)
      html = html[m.end():]
      m = RE_LIB_TAG.search(html)
    self._ast.addText(html, (begin, end, '<html string>', self._curfile))
  def processActiveTag(self, tag, tagend, taglib, tagname, tagattrs, tagpair, begin, end):
    "Process HTML tags"
    # make sure prefix belongs to loaded taglibrary
    if not self._ast._taglibs.has_key(taglib):
      self._ast.addText(tag, (begin, end, '<html string>', self._curfile))
      return
    # parse process tag attributes
    _, tagattrs = parseDirective('x '+tagattrs)
    # get tag class
    tagclass = self._tagChecker.getTagClass(self._ast._taglibs[taglib], 
      tagname, (begin, end, tag, self._curfile))
    # syntax check
    if not tagend: # start tag
      self._tagChecker.startTag(self._ast._taglibs[taglib], 
        tagname, tagattrs, tagpair, (begin, end, tag, self._curfile))
    else: # end tag
      self._tagChecker.endTag(self._ast._taglibs[taglib], 
        tagname, (begin, end, tag, self._curfile))
    # add python code for active tag
    if not tagend or not tagpair: # open or singleton tag
      self._ast.addCode('try: {  taglib.tagPush(%s, %s, %s, %s)' % (
          repr(taglib), repr(tagname), repr(tagattrs), repr(tagpair)), 
        (begin, end, tag, self._curfile))
      if tagclass.catches:
        self._ast.addCode('try: {', (begin, end, tag, self._curfile))
      if tagclass.conditional:
        self._ast.addCode('if taglib.tagBegin(): {', (begin, end, tag, self._curfile))
      else:
        self._ast.addCode('taglib.tagBegin()', (begin, end, tag, self._curfile))
      if tagclass.mustend:
        self._ast.addCode('try: {', (begin, end, tag, self._curfile))
      if tagclass.loops:
        self._ast.addCode('while 1: {', (begin, end, tag, self._curfile))
    if tagend or not tagpair: # close or singleton tag
        if tagclass.loops:
          self._ast.addCode('if not taglib.tagBody(): break }', (begin, end, tag, self._curfile))
        else:
          self._ast.addCode('taglib.tagBody()', (begin, end, tag, self._curfile))
        if tagclass.mustend:
          self._ast.addCode('} finally: taglib.tagEnd()', (begin, end, tag, self._curfile))
        else:
          self._ast.addCode('taglib.tagEnd()', (begin, end, tag, self._curfile))
        if tagclass.conditional:
          self._ast.addCode('}', (begin, end, tag, self._curfile))
        if tagclass.catches:
          self._ast.addCode('} except: taglib.tagCatch()', (begin, end, tag, self._curfile))
        self._ast.addCode('} finally: taglib.tagPop()', (begin, end, tag, self._curfile))
  def processEval(self):
    # collect expression
    begin = self._tokenBegin
    self.popToken()
    expr = ''
    while self._tokenType not in [T_END, T_EOF]:
      if self._tokenType==T_TEXT:
        expr = expr + self._tokenText
      elif self._tokenType==T_LAMBDA:
        expr = expr + self.processLambda()
      else: self.processUnexpected()
      self.popToken()
    expr = string.strip(expr)
    if not expr: self.processUnexpected()
    # add expression to ast
    self._ast.addEval(expr, (begin, self._tokenEnd, '='+expr, self._curfile))
  def processStmt(self):
    # collect statement
    self.popToken()
    beginrow, begincol = self._tokenBegin
    stmt = ''
    while self._tokenType not in [T_END, T_EOF]:
      if self._tokenType==T_TEXT:
        stmt = stmt + self._tokenText
      elif self._tokenType==T_LAMBDA:
        stmt = stmt + self.processLambda()
      else: self.processUnexpected()
      endrow, endcol = self._tokenEnd
      self.popToken()
    if not string.strip(stmt): self.processUnexpected()
    # add statement to ast, row-by-row
    currow = beginrow
    lines = string.split(stmt, '\n')
    for l in lines:
      if currow==beginrow: curcolbegin = begincol
      else: curcolbegin = 0
      if currow==endrow: curcolend = endcol
      else: curcolend = len(l)
      l = string.strip(l)
      if l:
        self._ast.addCode(l, ((currow, curcolbegin), (currow, curcolend), l, self._curfile))
      currow = currow + 1
  def processChunk(self, globalChunk=0):
    # collect chunk
    self.popToken()
    begin = self._tokenBegin
    chunk = ''
    while self._tokenType not in [T_END, T_EOF]:
      if self._tokenType==T_TEXT:
        chunk = chunk + self._tokenText
      elif self._tokenType==T_LAMBDA:
        chunk = chunk + self.processLambda()
      else: self.processUnexpected()
      end = self._tokenEnd
      self.popToken()
    chunk = string.split(chunk, '\n')
    # eliminate initial blank lines
    brow, bcol = begin
    while chunk and not string.strip(chunk[0]):
      chunk = chunk[1:]
      brow = brow + 1
      bcol = 0
    begin = brow, bcol
    if not chunk: self.processUnexpected()
    # outdent chunk based on first line
    # note: modifies multi-line strings having more spaces than first line outdent
    #    by removing outdent number of spaces at the beginning of each line.
    #    -- difficult to deal with efficiently (without parsing python) so just 
    #    don't do this!
    outdent = len(chunk[0]) - len(string.lstrip(chunk[0]))
    for i in range(len(chunk)):
      if string.strip(chunk[i][:outdent]):
        chunk[i] = ' '*outdent + chunk[i]
    chunk = map(lambda l, outdent=outdent: l[outdent:], chunk)
    chunk = string.join(chunk, '\n')
    # add chunk block at ast
    if chunk:
      try:
        self._ast.addCodeIndented(chunk, (begin, end, chunk, self._curfile), globalChunk)
      except tokenize.TokenError, e:
        msg, _ = e
        raise spyceException.spyceSyntaxError(msg, (begin, end, chunk, self._curfile) )
  def processGlobalChunk(self):
    self.processChunk(1)
  def processDirective(self):
    # collect directive
    begin = self._tokenBegin
    self.popToken()
    directive = ''
    while self._tokenType not in [T_END, T_EOF]:
      if self._tokenType==T_TEXT:
        directive = directive + self._tokenText
      else: self.processUnexpected()
      end = self._tokenEnd
      self.popToken()
    directive = string.strip(directive)
    if not directive: self.processUnexpected()
    # process directives
    name, attrs = parseDirective(directive)
    if name=='compact':
      compact_mode = COMPACT_FULL
      if attrs.has_key('mode'):
        mode = string.lower(attrs['mode'])
        if mode=='off':
          compact_mode = COMPACT_OFF
        elif mode=='line':
          compact_mode = COMPACT_LINE
        elif mode=='space':
          compact_mode = COMPACT_SPACE
        elif mode=='full':
          compact_mode = COMPACT_FULL
        else:
          raise spyceException.spyceSyntaxError('invalid compacting mode "%s" specified'%mode, (begin, end, directive, self._curfile))
      self._ast.addCompact(compact_mode, (begin, end, '<spyce compact directive>', self._curfile))
    elif name in ('module', 'import'):
      if not attrs.has_key('name') and not attrs.has_key('names'):
        raise spyceException.spyceSyntaxError('name or names attribute required', (begin, end, directive, self._curfile) )
      if attrs.has_key('names'):
        mod_names = filter(None, map(string.strip, string.split(attrs['names'],',')))
        for mod_name in mod_names:
          self._ast.addModule(mod_name, None, None)
          self._ast.addCode('%s.init()'%mod_name, (begin, end, directive, self._curfile))
      else:
        mod_name = attrs['name']
        mod_from = spyceUtil.extractValue(attrs, 'from')
        mod_as = spyceUtil.extractValue(attrs, 'as')
        mod_args = spyceUtil.extractValue(attrs, 'args', '')
        if mod_as: theName=mod_as
        else: theName=mod_name
        self._ast.addModule(mod_name, mod_from, mod_as)
        self._ast.addCode('%s.init(%s)'%(theName,mod_args), (begin, end, directive, self._curfile))
    elif name in ('taglib',):
      if not attrs.has_key('name') and not attrs.has_key('names'):
        raise spyceException.spyceSyntaxError('name or names attribute required', (begin, end, directive, self._curfile) )
      fullfile = os.path.join(self._curdir, self._curfile)
      if attrs.has_key('names'):
        taglib_names = filter(None, map(string.strip, string.split(attrs['names'],',')))
        for taglib_name in taglib_names:
          self._tagChecker.loadLib(taglib_name, None, None, fullfile, (begin, end, directive, self._curfile))
          self._ast.addTaglib(taglib_name)
          self._load_taglib = 1
          self._ast.addCode('taglib.load(%s)'%repr(taglib_name), (begin, end, directive, self._curfile))
      else:
        taglib_name = attrs['name']
        taglib_from = spyceUtil.extractValue(attrs, 'from')
        taglib_as = spyceUtil.extractValue(attrs, 'as')
        self._tagChecker.loadLib(taglib_name, taglib_from, taglib_as, fullfile, (begin, end, directive, self._curfile))
        self._ast.addTaglib(taglib_name, taglib_from, taglib_as)
        self._load_taglib = 1
        self._ast.addCode('taglib.load(%s, %s, %s)'%(repr(taglib_name), repr(taglib_from), repr(taglib_as)), (begin, end, directive, self._curfile))
    elif name=='include':
      if not attrs.has_key('file'):
        raise spyceException.spyceSyntaxError('file attribute missing', (begin, end, directive, self._curfile) )
      filename = os.path.join(self._curdir, attrs['file'])
      f = None
      try:
        try:
          f = open(filename)
          buf = f.read()
        finally:
          if f: f.close()
      except KeyboardInterrupt: raise
      except:
        raise spyceException.spyceSyntaxError('unable to open included file: %s'%filename, (begin, end, directive, self._curfile) )
      prev = (self._curdir, self._curfile, self._tokens,
        self._tokenType, self._tokenText, self._tokenBegin, self._tokenEnd)
      self._curdir, self._curfile = os.path.dirname(filename), filename
      self._tokens = spyceTokenize4Parse(processMagic(buf))
      self.popToken()
      self.processSpyce()
      (self._curdir, self._curfile, self._tokens,
        self._tokenType, self._tokenText, self._tokenBegin, self._tokenEnd) = prev
    else:
      raise spyceException.spyceSyntaxError('invalid spyce directive', (begin, end, directive, self._curfile) )
  def processLambda(self):
    # collect lambda
    self.popToken()
    begin = self._tokenBegin
    lamb = ''
    depth = 1
    while self._tokenType!=T_EOF:
      if self._tokenType in [T_END,]:
        depth = depth - 1
        if not depth: break
        lamb = lamb + self._tokenText
      elif self._tokenType in [T_EVAL, T_STMT, T_CHUNK, T_CHUNKG, T_DIRECT, T_LAMBDA]:
        depth = depth + 1
        lamb = lamb + self._tokenText
      elif self._tokenType==T_CMNT:
        self.processComment()
      else:
        lamb = lamb + self._tokenText
      end = self._tokenEnd
      self.popToken()
    # process lambda
    lamb = string.split(lamb, ':')
    try:
      params = lamb[0]
      memoize = 0
      if params and params[0]=='!':
        params = params[1:]
        memoize = 1
      lamb = string.join(lamb[1:],':')
    except:
      raise spyceException.spyceSyntaxError('invalid spyce lambda', (begin, end, lamb, self._curfile))
    self._load_spylambda = 1
    lamb = 'spylambda.define(%s,%s,%d)' % (`string.strip(params)`, `lamb`, memoize)
    return lamb
  def processUnexpected(self):
    raise spyceException.spyceSyntaxError('unexpected token: "%s"'%self._tokenText, 
      (self._tokenBegin, self._tokenEnd, self._tokenText, self._curfile))

##################################################
# Peep-hole optimizer
#

class spyceOptimize:
  def __init__(self, ast):
    self.compaction(ast)
    self.sideBySideWrites(ast)
    #self.splitCodeLines(ast)
  def splitCodeLines(self, ast):
    elements, leafs = ast['elements'], ast['leafs']
    for el in elements.keys():
      self.splitCodeLines(elements[el])
    if leafs:
      i = 0
      while i<len(leafs):
        row = 1
        type, text, ref = leafs[i]
        if type == AST_PY and ref:
          (brow, bcol), (erow, ecol), code, file = ref
          lines = string.split(code, '\n')
          if code==text and len(lines)>1:
            del leafs[i]
            row = brow
            for l in lines:
              cbcol = 0
              cecol = len(l)
              if row==brow: cbcol = bcol
              if row==erow: becol = ecol
              leafs.insert(i+(brow-row), (AST_PY, l, ((row, cbcol), (row, cecol), l, file)))
              row = row + 1
        i = i + row

  def sideBySideWrites(self, ast):
    elements, leafs = ast['elements'], ast['leafs']
    for el in elements.keys():
      self.sideBySideWrites(elements[el])
    if leafs:
      i = 0
      while i+1<len(leafs):
        type1, text1, ref1 = leafs[i]
        type2, text2, ref2 = leafs[i+1]
        file1 = None
        file2 = None
        if ref1:
          _, _, _, file1 = ref1
        if ref2:
          _, _, _, file2 = ref2
        if type1==AST_TEXT and type2==AST_TEXT and file1==file2:
          text = text1 + text2
          begin, _, orig, _ = ref1
          _, end, _, _ = ref2
          leafs[i] = AST_TEXT, text, (begin, end, orig, file1)
          del leafs[i+1]
          i = i - 1
        i = i+1
  def compaction(self, ast):
    elements, leafs = ast['elements'], ast['leafs']
    compact = COMPACT_LINE
    for el in elements.keys():
      self.compaction(elements[el])
    if leafs:
      i = 0
      while i<len(leafs):
        type, text, ref = leafs[i]
        if type==AST_COMPACT:
          compact = text
        elif type==AST_TEXT:
          # line compaction
          if compact==COMPACT_LINE or compact==COMPACT_FULL:
            # remove any trailing whitespace
            text = string.split(text, '\n')
            for j in range(len(text)-1):
              text[j] = string.rstrip(text[j])
            text = string.join(text, '\n')
            # gobble the end of the line
            ((row, _), _, _, file) = ref
            rowtext = string.split(text, '\n')
            if rowtext: rowtext = string.strip(rowtext[0])
            crow = row ; cfile = file
            j = i
            while j and not rowtext:
              j = j - 1
              type2, text2, ref2 = leafs[j]
              if ref2: (_, (crow, _), _, cfile) = ref2
              if crow != row or file != cfile: break
              if type2 == AST_TEXT:
                text2 = string.split(text2, '\n')
                if text2: text2 = text2[-1]
                rowtext = rowtext + string.strip(text2)
              elif type2 == AST_PYEVAL:
                rowtext = 'x'
            if not rowtext:
              text = string.split(text, '\n')
              if text and not string.strip(text[0]):
                text = text[1:]
              text = string.join(text, '\n')
            # gobble beginning of the line
            (_, (row, _), _, file) = ref
            rowtext = string.split(text, '\n')
            if rowtext: rowtext = string.strip(rowtext[-1])
            crow = row ; cfile = file
            j = i + 1
            while j<len(leafs) and not rowtext:
              type2, text2, ref2 = leafs[j]
              if ref2: ((crow, _), _, _, cfile) = ref2
              if crow != row or file != cfile: break
              if type2 == AST_TEXT:
                text2 = string.split(text2, '\n')
                if text2: text2 = text2[0]
                rowtext = rowtext + string.strip(text2)
              elif type2 == AST_PYEVAL:
                rowtext = 'x'
              j = j + 1
            if not rowtext:
              text = string.split(text, '\n')
              if text: text[-1] = string.strip(text[-1])
              text = string.join(text, '\n')
          # space compaction
          if compact==COMPACT_SPACE or compact==COMPACT_FULL:
            text = spyceUtil.spaceCompact(text)
          # update text, if any
          if text: leafs[i] = type, text, ref
          else: 
            del leafs[i]
            i = i -1
        elif type in [AST_PY, AST_PYEVAL]:
          pass
        else:
          raise 'error: unknown AST node type'
        i = i + 1

##################################################
# Output classes
#

class LineWriter:
  "Output class that counts lines written."
  def __init__(self, f, initialLine = 1):
    self.f = f
    self.lineno = initialLine
  def write(self, s):
    self.f.write(s)
    self.lineno = self.lineno + len(string.split(s,'\n'))-1
  def writeln(self, s):
    self.f.write(s+'\n')
  def close(self):
    self.f.close()

class IndentingWriter:
  "Output class that helps with indentation of code."
  # Note: this writer is line-oriented.
  def __init__(self, f, indentSize=2):
    self._f = f
    self._indentSize = indentSize
    self._indent = 0
    self._indentString = ' '*(self._indent*self._indentSize)
    self._currentLine = ''
  def close(self):
    if self._indent > 0:
      raise 'unmatched open brace'
    self._f.close()
  def indent(self):
    self._indent = self._indent + 1
    self._indentString = ' '*(self._indent*self._indentSize)
  def outdent(self):
    self._indent = self._indent - 1
    if self._indent<0: 
      raise 'unmatched close brace'
    self._indentString = ' '*(self._indent*self._indentSize)
  def dumpLine(self, s):
    self._f.write(self._indentString+s+'\n')
  def write(self, s):
    self._currentLine = self._currentLine + s
    lines = string.split(self._currentLine, '\n')
    for l in lines[:-1]:
      self.dumpLine(l)
    self._currentLine=lines[-1]
  def writeln(self, s=''):
    self.write(s+'\n')
  # remaining methods are defined in terms of writeln(), indent(), outdent()
  def pln(self, s=''):
    self.writeln(s)
  def pIln(self, s=''):
    self.indent(); self.pln(s)
  def plnI(self, s=''):
    self.pln(s); self.indent()
  def pOln(self, s=''):
    self.outdent(); self.pln(s)
  def plnO(self, s=''):
    self.pln(s); self.outdent()
  def pOlnI(self, s=''):
    self.outdent(); self.pln(s); self.indent()
  def pIlnO(self, s=''):
    self.indent(); self.pln(s); self.outdent()

##################################################
# Print out Braced Python
#

class emitBracedPython:
  def __init__(self, out, ast):
    out = LineWriter(out)
    self._spyceRefs = {}
    # text compaction
    self.compact = COMPACT_LINE
    self._gobblelineNumber = 1
    self._gobblelineText = ''
    # do the deed!
    self.emitSpyceRec(out, self._spyceRefs, None, ast['elements'], ast['leafs'][1:])
  def getSpyceRefs(self):
    return self._spyceRefs
  def emitSpyceRec(self, out, spyceRefs, header, elements, leafs):
    if header:
      out.write(header+':{\n')
    def processLevel(el, out=out, spyceRefs=spyceRefs, self=self):
      self.emitSpyceRec(out, spyceRefs, el['leafs'][0][1], el['elements'], el['leafs'][1:])
    try:
      processLevel(elements[SPYCE_GLOBAL_CODE])
      del elements[SPYCE_GLOBAL_CODE]
    except KeyError: pass
    for el in elements.keys():
      processLevel(elements[el])
    if leafs:
      for type, text, ref in leafs:
        line1 = out.lineno
        if type==AST_TEXT:
          out.write('response.writeStatic(%s)\n' % `text`)
        elif type==AST_PY:
          out.write(text+'\n')
        elif type==AST_PYEVAL:
          out.write('response.writeExpr(%s)\n' % text)
        elif type==AST_COMPACT:
          self.compact = text
        else:
          raise 'error: unknown AST node type'
        line2 = out.lineno
        if ref:
          for l in range(line1, line2):
            spyceRefs[l] = ref
    if not leafs and not elements:
      out.write('pass\n')
    if header:
      out.write('}\n')

##################################################
# Print out regular Python
#

class BraceConverter:
  "Convert Python with braces into indented (normal) Python code."
  def __init__(self, out):
    self.out = IndentingWriter(out)
    self.prevname = 0
    self.prevstring = 0
    self.dictlevel = 0
  def emitToken(self, type, string):
    if type==token.NAME:
      if self.prevname: self.out.write(' ')
      if self.prevstring: self.out.write(' ')
      self.out.write(string)
    elif type==token.STRING:
      if self.prevname: self.out.write(' ')
      string  = `eval(string)`  # get rid of multi-line strings
      self.out.write(string)
    elif type==token.NUMBER:
      if self.prevname: self.out.write(' ')
      self.out.write(string)
    elif type==token.OP:
      if string=='{': 
        if self.prevcolon and not self.dictlevel:
          self.out.plnI()
        else:
          self.dictlevel = self.dictlevel + 1
          self.out.write(string)
      elif string=='}':
        if not self.dictlevel:
          self.out.plnO()
        else:
          self.dictlevel = self.dictlevel - 1
          self.out.write(string)
      else:
        self.out.write(string)
    elif type==token.ERRORTOKEN and string==chr(0):
      self.out.write(' ')
    else:
      #print type, token.tok_name[type], `string`
      self.out.write(string)
    self.prevname = type==token.NAME
    self.prevstring = type==token.STRING
    self.prevcolon = type==token.OP and string==':'

def emitPython(out, bracedPythonCode, spyceRefs):
  out = LineWriter(out)
  spyceRefs2 = {}
  braceConv = BraceConverter(out)
  def eatToken(type, string, begin, end, _, out=out, braceConv=braceConv, spyceRefs=spyceRefs, spyceRefs2=spyceRefs2):
    try:
      beginrow, _ = begin
      line1 = out.lineno
      braceConv.emitToken(type, string)
      line2 = out.lineno
      if spyceRefs.has_key(beginrow):
        for l in range(line1, line2):
          spyceRefs2[l] = spyceRefs[beginrow]
    except:
      raise spyceException.spyceSyntaxError(sys.exc_info()[0])
  try:
    tokenize.tokenize(StringIO(bracedPythonCode).readline, eatToken)
  except tokenize.TokenError, e:
    msg, (row, col) = e
    raise spyceException.spyceSyntaxError(msg)
  return spyceRefs2

def calcRowCol(str, pos):
  lines = string.split(str, '\n')
  row = 1
  while pos > len(lines[0]):
    pos = pos - len(lines[0]) - 1
    del lines[0]
    row = row + 1
  return row, pos

RE_BRACES = re.compile('{|}')
def checkBalancedParens(str, refs):
  m = RE_BRACES.search(str)
  stack = []
  try:
    while m:
      if m.group(0)=='{': stack.append(m)
      else: stack.pop()
      m = RE_BRACES.search(str, m.end())
  except IndexError: 
    row, _ = calcRowCol(str, m.start())
    try: info = refs[row]
    except KeyError: info =None
    raise spyceException.spyceSyntaxError("unbalanced open brace '{'", info)
  if stack:
    m = stack[-1]
    row, _ = calcRowCol(str, m.start())
    try: info = refs[row]
    except KeyError: info =None
    raise spyceException.spyceSyntaxError("unbalanced close brace '}'", info)

##############################################
# Compile spyce files
#

def spyceCompile(buf, filename, sig, server):
  # parse 
  ast, libs = spyceParse(server, CRLF2LF(buf), filename, sig).info()
  # optimize the ast
  spyceOptimize(ast)
  # generate braced code
  out = StringIO()
  refs = emitBracedPython(out, ast).getSpyceRefs()
  # then, generate regular python code
  bracedPythonCode = out.getvalue()
  checkBalancedParens(bracedPythonCode, refs)
  out = StringIO()
  refs = emitPython(out, bracedPythonCode, refs)
  return out.getvalue(), refs, libs

def test():
  import spyce
  f = open(sys.argv[1])
  spycecode = f.read()
  f.close()
  tokens = spyceTokenize(processMagic(CRLF2LF(spycecode)))
  for type, text, begin, end in tokens:
    print '%s (%s, %s): %s' % (type, begin, end, `text`)
  pythoncode, refs, libs = spyceCompile(spycecode, sys.argv[1], '', spyce.getServer())
  print pythoncode

if __name__ == '__main__':
  test()

