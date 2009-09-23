##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import tree

__doc__ = '''Table-of-Contents module helps in creating indexed documents NB:
The TOC module may force two passes of a file, if the first pass TOC emitted
was not accurate. The second pass occurs via a redirect, so all modules are
reinitialized. Unfortunately, this breaks things like include.context...'''

ROOT_NAME = 'root'

class toc(spyceModule):

  def start(self):
    if not self._api.getModule('pool').has_key('toc'):
      self._api.getModule('pool')['toc'] = {}
    try:
      self.oldtree, self.oldtags = self._api.getModule('pool')['toc'][self._api.getFilename()]
    except (KeyError, TypeError):
      self.oldtree = tree.tree( (ROOT_NAME, [], None) )
      self.oldtags = {ROOT_NAME: self.oldtree}
    # tree data: (tag, numbering, data)
    self.tree = tree.tree((ROOT_NAME, [], None))
    self.tags = {ROOT_NAME: self.tree}
    self.node = self.tree
    self.numbering = []
    self.autotag = 0
    self.tocShown = 0
    self.fDOC_PUSH = None
    self.fDOC_POP = None
    self.fDOC_START = None
    self.fDOC_END = None
    self.fTOC_PUSH = None
    self.fTOC_POP = None
    self.fTOC_ENTRY = None
  def finish(self, theError):
    if not theError:
      self.tree.computePreChain()
      regenerate = not (self.oldtree == self.tree)
      file = self._api.getFilename()
      self._api.getModule('pool')['toc'][file] = self.tree, self.tags
      self.oldtree.delete()
      self.oldtree = None
      self.oldtags = None
      if self.tocShown and regenerate:
        self._api.getModule('redirect').internal(file)

  # set callbacks
  def setDOC_PUSH(self, f):
    self.fDOC_PUSH = f
  def setDOC_POP(self, f):
    self.fDOC_POP = f
  def setDOC_START(self, f):
    self.fDOC_START = f
  def setDOC_END(self, f):
    self.fDOC_END = f
  def setTOC_PUSH(self, f):
    self.fTOC_PUSH = f
  def setTOC_POP(self, f):
    self.fTOC_POP = f
  def setTOC_ENTRY(self, f):
    self.fTOC_ENTRY = f

  # sectioning
  def begin(self, data, tag=None, number=1):
    self._emit(self.node, self.fDOC_PUSH)
    self.numbering = _in(self.numbering)
    if number: 
      self.numbering = _inc(self.numbering)
      self.node = self.node.append( (tag, self.numbering, data) )
    else:
      self.node = self.node.append( (tag, None, data) )
    if not tag: tag = self._genTag()
    self.tags[tag] = self.node
    self._emit(self.node, self.fDOC_START)
  def end(self):
    self._emit(self.node, self.fDOC_END)
    self.numbering = _out(self.numbering)
    self.node = self.node.parent
    self._emit(self.node, self.fDOC_POP)
  def next(self, data, tag=None, number=1):
    self._emit(self.node, self.fDOC_END)
    self.node = self.node.parent
    if number:
      self.numbering = _inc(self.numbering)
      self.node = self.node.append( (tag, self.numbering, data) )
    else:
      self.node = self.node.append( (tag, None, data) )
    if not tag: tag = self._genTag()
    self.tags[tag] = self.node
    self._emit(self.node, self.fDOC_START)
  def anchor(self, data, tag=ROOT_NAME):
    self.tree.data = tag, [], data
    self.tags[tag] = self.tree

  # shortcuts
  b=begin
  e=end
  n=next

  # sectioning by depth
  def level(self, depth, data, tag=None):
    curdepth = self.getDepth()
    if curdepth > depth:  # indent
      while curdepth > depth:
        self.end()
        curdepth = self.getDepth()
      self.next(data, tag)
    elif curdepth < depth:  # outdent
      while curdepth < depth - 1:
        self.begin(None)
        curdepth = self.getDepth()
      self.begin(data, tag)
    else: # next
      self.next(data, tag)
  def l1(self, data, tag=None):
    self.level(1, data, tag)
  def l2(self, data, tag=None):
    self.level(2, data, tag)
  def l3(self, data, tag=None):
    self.level(3, data, tag)
  def l4(self, data, tag=None):
    self.level(4, data, tag)
  def l5(self, data, tag=None):
    self.level(5, data, tag)
  def l6(self, data, tag=None):
    self.level(6, data, tag)
  def l7(self, data, tag=None):
    self.level(7, data, tag)
  def l8(self, data, tag=None):
    self.level(8, data, tag)
  def l9(self, data, tag=None):
    self.level(9, data, tag)

  # show toc
  def showTOC(self):
    self.tocShown = 1
    self._tocHelper(self.oldtree)
  def _tocHelper(self, node):
    self._emit(node, self.fTOC_ENTRY)
    if node.children:
      self._emit(node, self.fTOC_PUSH)
      for c in node.children:
        self._tocHelper(c)
      self._emit(node, self.fTOC_POP)

  # current state
  def getTag(self, node=None):
    self.tocShown = 1
    if not node: node = self.node
    tag, numbering, data = node.data
    return tag
  def getNumbering(self, tag=None):
    self.tocShown = 1
    try:
      node = self.node
      if tag: node = self.oldtags[tag]
      tag, numbering, data = node.data
      return numbering
    except KeyError:
      return None
  def getData(self, tag=None):
    self.tocShown = 1
    try:
      node = self.node
      if tag: node = self.oldtags[tag]
      tag, numbering, data = node.data
      return data
    except KeyError:
      return None
  def getDepth(self, tag=None):
    self.tocShown = 1
    try:
      node = self.node
      if tag: node = self.tags[tag]
      return node.depth
    except KeyError:
      return None
  def getNextTag(self, tag=None):
    self.tocShown = 1
    try:
      if not tag: tag = self.getTag()
      tag = self.oldtags[tag].next
      if tag==None: return None
      return self.getTag(tag)
    except KeyError:
      return None
  def getPrevTag(self, tag=None):
    self.tocShown = 1
    try:
      if not tag: tag = self.getTag()
      node = self.oldtags[tag].prev
      if node==None: return None
      return self.getTag(node)
    except KeyError:
      return None
  def getParentTag(self, tag=None):
    self.tocShown = 1
    try:
      if not tag: tag = self.getTag()
      node = self.oldtags[tag].parent
      if node==None: return None
      return self.getTag(node)
    except KeyError:
      return None
  def getChildrenTags(self, tag=None):
    self.tocShown = 1
    try:
      if not tag: tag = self.getTag()
      nodes = self.oldtags[tag].children
      return map(self.getTag, nodes)
    except KeyError:
      return None

  # internal helpers
  def _genTag(self):
    tag = 'auto_'+str(self.autotag)
    self.autotag = self.autotag + 1
    return tag
  def _emit(self, node, f):
    tag, numbering, data = node.data
    if f: s = f(node.depth, tag, numbering, data)

# hierarchical counting
def _inc(numbering, inc=1):
  return numbering[:-1]+[numbering[-1]+inc]
def _in(numbering, start=0):
  return numbering+[start]
def _out(numbering):
  return numbering[:-1]

def defaultOutput(tag, numbering, data):
  return reduce(lambda s, i: '%s%d.' % (s, i), numbering, '') + ' ' + str(data)
