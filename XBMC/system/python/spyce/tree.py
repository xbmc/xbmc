##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import string

class tree:
  def __init__(self, data):
    self.data = data
    self.parent = None
    self.children = []
    self.next = self.prev = None
    self.depth = 0
  def append(self, data):
    node = tree(data)
    self.children.append(node)
    node.parent = self
    node.depth = self.depth+1
    return node
  def delete(self):
    for c in self.children:
      c.delete()
    if self.parent:
      self.parent.children.remove(self)
      self.parent = None
  def __repr__(self):
    return '%s [%s]' % (self.data, string.join(map(str, self.children),', '))
  def postWalk(self, f):
    for c in self.children:
      c.postWalk(f)
    f(self)
  def preWalk(self, f):
    f(self)
    for c in self.children:
      c.preWalk(f)
  def computePreChain(self):
    prev = [None]
    def walker(node, prev=prev):
      node.prev = prev[0]
      if prev[0]: 
        node.prev.next = node
      prev[0] = node
    self.preWalk(walker)
  def __cmp__(self, o):
    try:
      x = not self.data == o.data
      if x: return x
      x = not self.children == o.children
      if x: return x
    except:
      return 1
    return 0

if __name__=='__main__':
  root = tree('1')
  n = root.append('1.1')
  n.append('1.1.1')
  n = root.append('1.2')
  n.append('1.2.1')
  root.computePreChain()
  n = root
  while(n):
    print n.data
    n = n.next
  root.delete()

