"""Port of the template-facing half of tools/codegenerator/Helper.groovy.

(transformSwigXml lives in transform.py.) This holds the in/out type-conversion
dispatch (getOutConversion / getInConversion), the Sequence counter that names
recursive-typemap temporaries, the typemap registry (set via setup), and the
Node-walking helpers the emitter and typemaps call.

The typemap registry stores Python callables (bindings dict -> str) rather than
Groovy template strings: exact-string keys in dicts, regex keys in ordered lists
scanned with re.fullmatch (Groovy Matcher.matches() is whole-string).
"""

from __future__ import annotations

import html
import sys

import swig_type_parser as SwigTypeParser
from node import Node, parents


# --- registry state (populated by setup) -----------------------------------
_classes = []
_out_exact = {}
_out_patterns = []      # list of (compiled_regex, callable)
_default_out = None
_in_exact = {}
_in_patterns = []
_default_in = None


def setup(classes, out_exact, out_patterns, default_out, in_exact, in_patterns, default_in):
    global _classes, _out_exact, _out_patterns, _default_out, _in_exact, _in_patterns, _default_in
    _classes = classes if classes else []
    _out_exact = out_exact
    _out_patterns = out_patterns
    _default_out = default_out
    _in_exact = in_exact
    _in_patterns = in_patterns
    _default_in = default_in


# --- Sequence (finding D): names recursive-typemap temporaries --------------
class _Sequence:
    def __init__(self):
        self.cur = 0

    def increment(self):
        self.cur += 1
        return self.cur


_current = None   # the active Sequence, or None


def _first_match(patterns, s):
    for rx, val in patterns:
        if rx.fullmatch(s):
            return val
    return None


# --- out conversion ---------------------------------------------------------
def getOutConversion(apiType, apiName, method, overrideBindings=None, recurse=True):
    global _current
    convertTemplate = _out_exact.get(apiType)
    className = None

    if convertTemplate is None and apiType.startswith("p."):
        classNode = findClassNodeByName(parents(method)[0], SwigTypeParser.getRootType(apiType), method)
        if classNode:
            className = findFullClassName(classNode)
            convertTemplate = _default_out

    if convertTemplate is None:
        convertTemplate = _first_match(_out_patterns, apiType)

    if not convertTemplate:
        knownApiType = isKnownApiType(apiType, method)
        if knownApiType:
            convertTemplate = _default_out
            className = knownApiType

    if not convertTemplate:
        apiTypeResolved = SwigTypeParser.SwigType_resolve_all_typedefs(apiType)
        if apiTypeResolved != apiType:
            return getOutConversion(apiTypeResolved, apiName, method, overrideBindings, recurse)
        elif recurse:
            return getOutConversion(SwigTypeParser.SwigType_ltype(apiType), apiName, method, overrideBindings, False)
        elif not isKnownApiType(apiType, method):
            raise RuntimeError("WARNING: Cannot convert the return value of swig type %s for the call %s" %
                               (apiType, str(findFullClassName(method)) + "::" + callingName(method)))

    seqSetHere = _current is None
    if seqSetHere:
        _current = _Sequence()
    seq = _current

    bindings = {"result": apiName, "api": "apiResult", "type": apiType,
                "method": method, "helper": sys.modules[__name__],
                "swigTypeParser": SwigTypeParser, "sequence": seq}
    if className:
        bindings["classname"] = className
    if overrideBindings:
        bindings.update(overrideBindings)

    ret = convertTemplate(bindings)
    if seqSetHere:
        _current = None
    return ret


# --- in conversion ----------------------------------------------------------
def getInConversion(apiType, apiName, slName, method, overrideBindings=None):
    # Only this call form is used by the template/typemaps; paramName == apiName.
    global _current
    paramName = apiName
    convertTemplate = _in_exact.get(apiType)

    apiLType = SwigTypeParser.convertTypeToLTypeForParam(apiType)
    if convertTemplate is None:
        convertTemplate = _in_exact.get(apiLType)

    if convertTemplate is None and apiType.startswith("p."):
        thisNamespace = findNamespace(method)
        target = apiLType[2:]
        clazz = None
        for it in _classes:
            if findFullClassName(it) == target or \
               (it.attr("sym_name") == target and thisNamespace == findNamespace(it)):
                clazz = it
                break
        if clazz is not None:
            convertTemplate = _default_in

    if convertTemplate is None:
        convertTemplate = _first_match(_in_patterns, apiType)

    if convertTemplate is None:
        convertTemplate = _first_match(_in_patterns, apiLType)

    if not convertTemplate:
        apiTypeResolved = SwigTypeParser.SwigType_resolve_all_typedefs(apiType)
        if apiTypeResolved != apiType:
            return getInConversion(apiTypeResolved, apiName, slName, method, overrideBindings)
        if not isKnownApiType(apiType, method) and not isKnownApiType(apiLType, method):
            print("WARNING: Unknown parameter type: %s (or %s) for the call %s" %
                  (apiType, apiLType, str(findFullClassName(method)) + "::" + callingName(method)),
                  file=sys.stderr)
        convertTemplate = _default_in

    if convertTemplate:
        seqSetHere = _current is None
        if seqSetHere:
            _current = _Sequence()
        seq = _current

        bindings = {"type": apiType, "ltype": apiLType,
                    "slarg": slName, "api": apiName, "param": paramName,
                    "method": method, "helper": sys.modules[__name__],
                    "swigTypeParser": SwigTypeParser, "sequence": seq}
        if overrideBindings:
            bindings.update(overrideBindings)

        ret = convertTemplate(bindings)
        if seqSetHere:
            _current = None
        return ret

    return ""


# --- node helpers -----------------------------------------------------------
def hasDefinedConstructor(clazz):
    return len(clazz.kids("constructor")) > 0


def hasDoc(methodOrClass):
    docs = methodOrClass.kids("doc")
    return len(docs) > 0 and docs[0].attr("value") is not None


def hasHiddenConstructor(clazz):
    if not hasDefinedConstructor(clazz):
        return False
    ctor = clazz.kids("constructor")[0]
    return ctor.attr("access") is not None and ctor.attr("access") != "public"


def findClassNodeByName(module, classname, referenceNode=None):
    for it in module.depth_first():
        if it.name() != "class":
            continue
        if findFullClassName(it).strip() == classname.strip():
            return it
        if referenceNode is not None and (findNamespace(referenceNode) + classname) == findFullClassName(it):
            return it
        if it.attr("name") == classname:
            return it
    return None


def findClassNode(node):
    if node.name() == "class":
        return node
    return None if node.parent() is None else findClassNode(node.parent())


def findFullClassName(node, separator="::", filename=False):
    ret = None
    rents = parents(node, lambda it: it.name() == "class")
    if node.name() == "class":
        rents.append(node)
    for it in rents:
        if ret is None:
            ret = it.attr("sym_name")
        else:
            ret += separator + it.attr("sym_name")
    return (findNamespace(node, separator, True, filename) + ret) if ret else None


def findNamespace(node, separator="::", endingSeparator=True, filename=False):
    ret = None
    for it in parents(node, lambda n: n.name() == "namespace"):
        data = it.attr("name")
        if filename:
            data = data.replace("_", "__")
        if ret is None:
            ret = data
        else:
            ret += separator + data
    return "" if ret is None else (ret + (separator if endingSeparator else ""))


def functionNodesByOverloads(module):
    ret = {}
    for it in module.depth_first():
        if it.name() in ("function", "constructor", "destructor"):
            key = it.attr("sym_overloaded") if it.attr("sym_overloaded") is not None else it.attr("id")
            ret.setdefault(key, []).append(it)
    return ret


def getPropertyReturnSwigType(method):
    prefix = "p." if (method.attr("decl") is not None and method.attr("decl") == "p.") else ""
    return prefix + method.attr("type") if method.attr("type") is not None else "void"


def getReturnSwigType(method):
    decl = method.attr("decl")
    prefix = "p." if (decl is not None and decl.endswith(".p.")) else ""
    return prefix + method.attr("type") if method.attr("type") is not None else "void"


def callingName(method):
    clazz = findFullClassName(method)
    if clazz is None:
        return method.attr("name")
    if method.name() == "constructor":
        return "new " + findNamespace(method) + method.attr("sym_name")
    if method.name() == "destructor":
        return "delete"
    return method.attr("name")


def getInsertNodes(module, section):
    return [it for it in module.kids("insert")
            if section == it.attr("section") or (section == "header" and it.attr("section") is None)]


def unescape(insertSection):
    if isinstance(insertSection, Node):
        return unescape(insertSection.attr("code"))
    # Groovy: commons-text StringEscapeUtils.unescapeHtml4, applied after the XML
    # parser already decoded standard XML entities (ElementTree does the same).
    return html.unescape(insertSection)


def isDirector(method):
    clazz = findClassNode(method)
    if not clazz or not clazz.attr("feature_director"):
        return False
    if method.name() == "destructor":
        return False
    if method.name() == "constructor":
        return False
    return bool(method.attr("storage")) and method.attr("storage") == "virtual"


def isKnownBaseType(typ, searchFrom):
    return hasFeatureSetting(typ, searchFrom, "feature_knownbasetypes",
                             lambda attr: any(x.strip() == typ for x in attr.split(",")))


def isKnownApiType(typ, searchFrom):
    rootType = SwigTypeParser.getRootType(typ)
    state = {"namespace": findNamespace(searchFrom, "::", False), "lastMatch": None}

    def test(attr):
        for entry in attr.split(","):
            if entry.strip() == rootType:
                state["lastMatch"] = rootType
                return True
            # assume 'type' is defined within namespace; walk up appending the type.
            while state["namespace"] != "":
                if (state["namespace"] + "::" + rootType) == entry.strip():
                    state["lastMatch"] = entry.strip()
                    return True
                chop = state["namespace"].rfind("::")
                state["namespace"] = state["namespace"][:chop] if chop > 0 else ""
        return False

    hasFeatureSetting(typ, searchFrom, "feature_knownapitypes", test)
    return state["lastMatch"]


def hasFeatureSetting(typ, searchFrom, feature, test):
    if not searchFrom:
        return None
    attr = searchFrom.attribute(feature)
    if attr and test(attr):
        return str(attr)
    return hasFeatureSetting(typ, searchFrom.parent(), feature, test)
