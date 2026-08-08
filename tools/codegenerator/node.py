"""
Stand-in for groovy.util.Node, used for the *transformed* model tree.

Input SWIG XML is read with xml.etree and only ever walked top-down, so the
input side needs nothing special. transformSwigXml builds a *new* tree with
`new Node(parent, name, attrs)` and mutates it (removals, typetab collapse,
appends); that tree is what the rest of the generator consumes, and it needs
parent links + predicate search that ElementTree can't give. This is that type.

Semantics deliberately match Groovy's Node:
  - bare each / find / findAll iterate DIRECT CHILDREN
  - depthFirst / breadthFirst iterate DESCENDANTS (self first, pre-order / BFS)
  - new Node(parent, name, attrs) auto-appends to parent
  - it.@x reads (None on miss); it.@x = v writes
GPath child-by-tag (clazz.constructor, method.parm) is `kids('tag')`, written
explicitly rather than via __getattr__ to avoid clashing with method names.
"""

from __future__ import annotations

from typing import Callable, List, Optional


class Node:
    __slots__ = ("_name", "attributes", "_children", "_parent")

    def __init__(self, parent: "Optional[Node]" = None,
                 name: Optional[str] = None,
                 attributes: Optional[dict] = None):
        self._name = name
        self.attributes = dict(attributes) if attributes else {}
        self._children: List["Node"] = []
        self._parent: "Optional[Node]" = None
        if parent is not None:
            parent.append(self)

    # identity / attributes ------------------------------------------------
    def name(self) -> Optional[str]:
        return self._name

    def attr(self, key: str, default=None):
        return self.attributes.get(key, default)

    def set_attr(self, key: str, value) -> None:
        self.attributes[key] = value

    def attribute(self, key: str):          # Helper.hasFeatureSetting uses this
        return self.attributes.get(key)

    # structure ------------------------------------------------------------
    def parent(self) -> "Optional[Node]":
        return self._parent

    def children(self) -> List["Node"]:
        return self._children

    def append(self, child: "Node") -> "Node":
        child._parent = self
        self._children.append(child)
        return child

    def remove(self, child: "Node") -> None:
        self._children.remove(child)
        child._parent = None

    # children-scoped iteration -------------------------------------------
    def __iter__(self):
        return iter(self._children)

    def each(self, fn: Callable[["Node"], None]) -> None:
        for c in list(self._children):
            fn(c)

    def find(self, pred: Callable[["Node"], bool]) -> "Optional[Node]":
        for c in self._children:
            if pred(c):
                return c
        return None

    def find_all(self, pred: Callable[["Node"], bool]) -> List["Node"]:
        return [c for c in self._children if pred(c)]

    def kids(self, tag: str) -> List["Node"]:
        return [c for c in self._children if c._name == tag]

    # descendant iteration -------------------------------------------------
    def depth_first(self) -> List["Node"]:
        out = [self]
        for c in self._children:
            out.extend(c.depth_first())
        return out

    def breadth_first(self) -> List["Node"]:
        out: List["Node"] = []
        q: List["Node"] = [self]
        while q:
            n = q.pop(0)
            out.append(n)
            q.extend(n._children)
        return out

    def descendants(self, tag: str) -> List["Node"]:
        return [n for n in self.depth_first() if n._name == tag]

    # debug ----------------------------------------------------------------
    def __repr__(self) -> str:
        return f"Node({self._name!r}, {self.attributes!r})"

    def dump(self, indent: int = 0, drop_attrs=("id",)) -> str:
        """Canonical-ish text dump for eyeballing/diffing the model. `id` is
        dropped by default because it is the SWIG addr and changes every run."""
        attrs = {k: v for k, v in sorted(self.attributes.items()) if k not in drop_attrs}
        line = "  " * indent + f"<{self._name}> {attrs}"
        parts = [line]
        for c in self._children:
            parts.append(c.dump(indent + 1, drop_attrs))
        return "\n".join(parts)


def parents(node: Node,
            filt: Optional[Callable[[Node], bool]] = None) -> List[Node]:
    """Helper.parents: ancestors ordered top-down (root-most ... immediate
    parent), optionally filtered. Recursive to match the Groovy ordering."""
    out: List[Node] = []
    p = node.parent()
    if p is not None:
        out = parents(p, filt)
        if filt is None or filt(p):
            out.append(p)
    return out
