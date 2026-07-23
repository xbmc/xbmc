"""
Port of Helper.transformSwigXml (+ transform / transformSwigAttributeList /
flatten / functionNodesByOverloads) from tools/codegenerator/Helper.groovy.

Input: an xml.etree Element (the raw `swig -xml` output root, <top>).
Output: a codegen.node.Node tree rooted at <module>, normalized the same way
the Groovy generator normalizes it:

  1. attributelist <attribute> entries become real attributes of their parent
  2. ignored attributes are dropped; element addr becomes the `id` attribute
  3. the auto-included swig.swg tree and typescope*/typetab* items are dropped
  4. cdecl nodes are renamed to their `kind` (function/variable/...)
  5. the outermost include/module is unwrapped into a fresh <module> node
  6. include / parmlist / typescope wrappers are flattened away
  7. function/constructor nodes carrying `defaultargs` are removed
  8. all typetabs are collapsed into one <typetab> of <entry> nodes
  9. non-public functions (except constructors) and variables are removed

The doxygen docstring pass is intentionally omitted: the CMake build invokes
the generator with no doxygen dir, so doc nodes are never produced on the build
path. Add it back only if a build is found that passes one.
"""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from typing import Callable, List, Optional, Tuple

from node import Node, parents

_SWIG_SWG_RE = re.compile(r"swig\.swg$")

IGNORE_ATTRIBUTES = {
    "classes", "symtab", "sym_symtab", "sym_overname", "options",
    "sym_nextSibling", "csym_nextSibling", "sym_previousSibling",
}


# --- the input-side node filter (operates on raw ET elements) --------------
def _el_attr(el: ET.Element, name: str) -> Optional[str]:
    """value of <attribute name=.. value=..> directly under el's attributelist(s)"""
    for al in el:
        if al.tag == "attributelist":
            for a in al:
                if a.tag == "attribute" and a.get("name") == name:
                    return a.get("value")
    return None


def _keep_node(el: ET.Element) -> bool:
    # drop the auto-included swig.swg include subtree and typescope/typetab items
    if el.tag == "include":
        nm = _el_attr(el, "name")
        if nm is not None and _SWIG_SWG_RE.search(nm):
            return False
    if el.tag in ("typescopesitem", "typetabsitem"):
        return False
    return True


def _keep_attr(key: str) -> bool:
    return key not in IGNORE_ATTRIBUTES


# --- core transform (raw ET element -> model Node) -------------------------
def _transform_attribute_list(attribute_list: ET.Element) -> Tuple[dict, List[ET.Element]]:
    attrs: dict = {}
    nodes: List[ET.Element] = []
    for it in attribute_list:
        if it.tag == "attribute":
            attrs[it.get("name")] = it.get("value")
        else:
            nodes.append(it)
    return attrs, nodes


def _transform(el: ET.Element,
               node_filter: Callable[[ET.Element], bool],
               attr_filter: Callable[[str], bool]) -> Node:
    attributes: dict = {}
    child_nodes: List[Node] = []

    for it in el:
        if node_filter is None or node_filter(it):
            if it.tag == "attributelist":
                raw_attrs, inner = _transform_attribute_list(it)
                attributes.update({k: v for k, v in raw_attrs.items() if attr_filter(k)})
                for c in inner:
                    if node_filter is not None and node_filter(c):
                        child_nodes.append(_transform(c, node_filter, attr_filter))
            else:
                child_nodes.append(_transform(it, node_filter, attr_filter))

    # transfer element addr -> id (raw swig nodes carry id/addr as XML attrs)
    addr = el.get("addr")
    if addr:
        for k, v in el.attrib.items():
            if k != "addr" and k != "id":
                attributes[k] = v
        attributes["id"] = addr

    # cdecl gets renamed to its 'kind'
    if el.tag == "cdecl" and "kind" in attributes:
        kind = attributes["kind"]
        ret = Node(None, kind, {k: v for k, v in attributes.items() if k != "kind"})
    else:
        ret = Node(None, el.tag, attributes)

    for c in child_nodes:
        ret.append(c)
    return ret


def _flatten(node: Node, elements_to_remove) -> None:
    remove = set(elements_to_remove)
    done = False
    while not done:
        done = True
        for child in node.breadth_first():
            if child.name() in remove:
                parent = child.parent()
                parent.remove(child)
                for gc in list(child.children()):
                    parent.append(gc)
                done = False
                break


def _function_nodes_by_overloads(module: Node) -> dict:
    ret: dict = {}
    for it in module.depth_first():
        if it.name() in ("function", "constructor", "destructor"):
            key = it.attr("sym_overloaded") or it.attr("id")
            ret.setdefault(key, []).append(it)
    return ret


def _find_namespace(node: Node, separator: str = "::",
                    ending_separator: bool = True, filename: bool = False) -> str:
    ret: Optional[str] = None
    for p in parents(node, lambda n: n.name() == "namespace"):
        data = p.attr("name")
        if filename:
            data = data.replace("_", "__")
        ret = data if ret is None else ret + separator + data
    if ret is None:
        return ""
    return ret + (separator if ending_separator else "")


# --- public entry point ----------------------------------------------------
def transform_swig_xml(swigxml: ET.Element) -> Node:
    node = _transform(swigxml, _keep_node, _keep_attr)

    includes = node.kids("include")
    assert len(includes) == 1 and len(includes[0].kids("module")) == 1 \
        and includes[0].kids("module")[0].attr("name") is not None, \
        "Invalid xml: expected a single 'include' child with a single 'module' child"

    module_name = includes[0].kids("module")[0].attr("name")
    ret = Node(None, "module", {"name": module_name})
    for c in list(includes[0].children()):
        if c.name() != "module":
            ret.append(c)

    _flatten(ret, ["include", "parmlist", "typescope"])

    # remove function/constructor nodes with default arguments
    for cur in list(ret.depth_first()):
        if cur.name() in ("function", "constructor") and cur.attr("defaultargs") is not None:
            cur.parent().remove(cur)

    # no remaining overloads may exist (defaulting is the only allowed form)
    for key, value in _function_nodes_by_overloads(ret).items():
        assert len(value) == 1, f"Cannot handle overloaded methods unless simply using defaulting: {value}"

    # collapse all typetabs into a single deduplicated <typetab> of <entry>
    all_typetabs = [n for n in ret.depth_first() if n.name() == "typetab"]
    typenode = Node(ret, "typetab")
    for tt in all_typetabs:
        for key, value in list(tt.attributes.items()):
            if key != "id" and key != value:
                namespace = _find_namespace(tt)
                entry = Node(None, "entry")
                entry.set_attr("namespace", namespace.strip() if namespace is not None else "")
                entry.set_attr("type", key)
                entry.set_attr("basetype", value)
                if typenode.find(lambda e: e.attr("basetype") == entry.attr("basetype")
                                 and e.attr("namespace") == entry.attr("namespace")) is None:
                    typenode.append(entry)
        if tt.parent() is not None:
            tt.parent().remove(tt)

    # remove non-public functions/destructors (keep constructors); doc omitted
    for it in [n for n in ret.depth_first()
               if n.name() in ("function", "destructor", "constructor")]:
        if it.attr("access") is not None and it.attr("access") != "public" \
                and it.name() != "constructor":
            it.parent().remove(it)

    # remove non-public variables
    for it in [n for n in ret.depth_first() if n.name() == "variable"]:
        if it.attr("access") is not None and it.attr("access") != "public":
            it.parent().remove(it)

    return ret


def transform_swig_xml_file(path: str) -> Node:
    return transform_swig_xml(ET.parse(path).getroot())


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("usage: python -m codegen.transform <swig.xml>", file=sys.stderr)
        raise SystemExit(2)
    print(transform_swig_xml_file(sys.argv[1]).dump())
