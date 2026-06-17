"""
Python port of tools/codegenerator/SwigTypeParser.groovy.

These functions are a transliteration of SWIG's own type-mangling routines
(Source/Swig/stype.c), the same way the Groovy original was. The point is
*behavioural identity* with the Groovy generator, not idiomatic Python, so the
control flow -- including the parts that look redundant or buggy -- is kept as
close to the original as the languages allow. Do not "clean this up"; the only
oracle that matters is byte-identical (post-clang-format) output against the
current generator.

Mapping of Groovy/Java string ops used below:
    t.substring(a, b)   -> t[a:b]
    t.substring(a)      -> t[a:]
    t.indexOf(x)        -> t.find(x)        (-1 when absent, same as Java)
    t.charAt(c)         -> t[c]
    t.startsWith(x)     -> t.startswith(x)
    replaceAll(lit)     -> str.replace      (the two uses here are literal)

The module keeps the original PascalCase function names so the rewritten
template/helper layers can call them as `SwigTypeParser.SwigType_str(...)`,
matching the Groovy call sites verbatim.
"""

from __future__ import annotations

from typing import Iterable, List, Optional


# ---------------------------------------------------------------------------
# typedef table
# ---------------------------------------------------------------------------
# Maps (namespace + type) -> basetype, exactly as the Groovy static map did.
_type_table: dict[str, str] = {}


def reset_type_table() -> None:
    """Clear the global typedef table (the Groovy version had no reset; this
    exists so the generator can be driven once per module in a single process
    without cross-contamination between modules)."""
    _type_table.clear()


def append_type_table(entries: Iterable) -> None:
    """Groovy: appendTypeTable(Node typetab) ->
        typetab.each { typeTable[it.@namespace + it.@type] = it.@basetype }

    `entries` is any iterable of typetab <entry> elements. Each entry must
    expose its attributes; we accept either a mapping-like object (``e['namespace']``)
    or an object with an ``attr(name)`` method (the Node wrapper used elsewhere
    in the port). This keeps this file importable/testable on its own."""
    for e in entries:
        ns = _attr(e, "namespace")
        ty = _attr(e, "type")
        base = _attr(e, "basetype")
        _type_table[(ns or "") + (ty or "")] = base


# kept under the original name for call-site parity
appendTypeTable = append_type_table


def _attr(e, name: str):
    if hasattr(e, "attr"):
        return e.attr(name)
    try:
        return e[name]
    except (TypeError, KeyError):
        return getattr(e, name, None)


# ---------------------------------------------------------------------------
# public conversions
# ---------------------------------------------------------------------------
def convertTypeToLTypeForParam(ty: str) -> str:
    ty = ty.strip()
    # an r.* parameter is assumed to be passed by value on the stack
    if ty.startswith("r."):
        return SwigType_ltype(ty[2:])
    return SwigType_ltype(ty)


def getRootType(ty: str) -> str:
    li = ty.rfind(".")
    return ty[li + 1:] if li >= 0 else ty


def SwigType_str(ty: str, id: Optional[str] = None) -> str:
    """Create a C string representation of a datatype."""
    result = id if id else ""
    elements = SwigType_split(ty)
    if elements is None:
        elements = []
    nelements = len(elements)
    element = elements[0] if nelements > 0 else None

    for i in range(nelements):
        if i < (nelements - 1):
            nextelement = elements[i + 1]
            forwardelement = nextelement
            if nextelement.startswith("q("):
                if i < (nelements - 2):
                    forwardelement = elements[i + 2]
        else:
            nextelement = None
            forwardelement = None

        if element.startswith("q("):
            q = SwigType_parm(element)
            result = q + " " + result
        elif SwigType_ispointer(element):
            result = "*" + result
            if forwardelement and (SwigType_isfunction(forwardelement) or SwigType_isarray(forwardelement)):
                result = "(" + result + ")"
        elif SwigType_ismemberpointer(element):
            q = SwigType_parm(element)
            result = q + "::*" + result
            if forwardelement and (SwigType_isfunction(forwardelement) or SwigType_isarray(forwardelement)):
                result = "(" + result + ")"
        elif SwigType_isreference(element):
            result = "&" + result
            if forwardelement and (SwigType_isfunction(forwardelement) or SwigType_isarray(forwardelement)):
                result = "(" + result + ")"
        elif SwigType_isarray(element):
            result += "[" + SwigType_parm(element) + "]"
        elif SwigType_isfunction(element):
            result += "("
            parms = SwigType_parmlist(element)
            didOne = False
            for cur in parms:
                p = SwigType_str(cur)
                result += ("," if didOne else "") + p
                didOne = True
            result += ")"
        else:
            if element.startswith("v(...)"):
                result = result + "..."
            else:
                bs = SwigType_namestr(element)
                result = bs + " " + result

        element = nextelement

    # convert template parameters: '<(' -> '<', ')>' -> '>'
    return result.replace("<(", "<").replace(")>", ">")


def SwigType_typedef_resolve(t: str) -> str:
    td = _type_table.get(t)
    return t if td is None else td


def SwigType_resolve_all_typedefs(s: str) -> str:
    result = ""
    tc = s

    # nuke all leading qualifiers, appending them to the result
    while SwigType_isqualifier(tc):
        popped, tc = SwigType_pop(tc)
        result += popped

    if SwigType_issimple(tc):
        # resolve any typedef definitions
        tt = tc
        td = tt
        while (td := SwigType_typedef_resolve(tt)) != tt:
            if td != tt:
                tt = td
                break
            elif td != tt:  # unreachable, preserved from the original
                tt = td
        tc = td
        return tc

    popped, rest = SwigType_pop(tc)
    result += popped
    result += SwigType_resolve_all_typedefs(rest)
    return result


def SwigType_ltype(s: str) -> str:
    """Create a locally assignable type."""
    result = ""
    tc = s

    # nuke all leading qualifiers
    while SwigType_isqualifier(tc):
        tc = SwigType_pop(tc)[1]

    if SwigType_issimple(tc):
        # resolve any typedef definitions
        tt = tc
        td = tt
        while (td := SwigType_typedef_resolve(tt)) != tt:
            if (td != tt) and (SwigType_isconst(td) or SwigType_isarray(td) or SwigType_isreference(td)):
                tt = td
                break
            elif td != tt:
                tt = td
        tc = td

    elements = SwigType_split(tc)
    nelements = len(elements)

    notypeconv = False
    firstarray = True
    for i in range(nelements):
        element = elements[i]
        # when we see a function, preserve the following types
        if SwigType_isfunction(element):
            notypeconv = True
        if SwigType_isqualifier(element):
            pass  # ignore
        elif SwigType_ispointer(element):
            result += element
            # short circuit: collapse the rest of the list and recurse
            tmps = "".join(elements[i + 1:])
            return result + SwigType_ltype(tmps)
        elif SwigType_ismemberpointer(element):
            result += element
            firstarray = False
        elif SwigType_isreference(element):
            if notypeconv:
                result += element
            else:
                result += "p."
            firstarray = False
        elif SwigType_isarray(element) and firstarray:
            if notypeconv:
                result += element
            else:
                result += "p."
            firstarray = False
        elif SwigType_isenum(element):
            anonymous_enum = (element == "enum ")
            if notypeconv or not anonymous_enum:
                result += element
            else:
                result += "int"
        else:
            result += element

    return result


def SwigType_lrtype(s: str) -> str:
    ltype = SwigType_ltype(s)
    if SwigType_ispointer(s):
        return ltype
    return "r." + ltype


def SwigType_lstr(type_: str) -> str:
    return SwigType_str(convertTypeToLTypeForParam(type_))


# ---------------------------------------------------------------------------
# predicates / small accessors
# ---------------------------------------------------------------------------
def SwigType_ispointer(t: str) -> bool:
    if t.startswith("q("):
        t = t[t.find(".") + 1:]
    return t.startswith("p.")


def SwigType_makepointer(t: str) -> str:
    if t.startswith("q("):
        prefix = t[: t.find(".") + 1]
        remainder = t[t.find(".") + 1:]
    else:
        prefix = ""
        remainder = t
    return prefix + "p." + remainder


def SwigType_isarray(t: str) -> bool:
    return t.startswith("a(")


def SwigType_ismemberpointer(t: str) -> bool:
    return bool(t) and t.startswith("m(")


def SwigType_isqualifier(t: str) -> bool:
    return bool(t) and t.startswith("q(")


def SwigType_isreference(t: str) -> bool:
    return t.startswith("r.")


def SwigType_isenum(t: str) -> bool:
    return t.startswith("enum")


def SwigType_istemplate(t: str) -> bool:
    c = t.find("<(")
    return c >= 0 and t.find(")>", c + 2) >= 0


def SwigType_isfunction(t: str) -> bool:
    if t.startswith("q("):
        t = t[t.find(".") + 1:]
    return t.startswith("f(")


def SwigType_isconst(t: str) -> bool:
    if t is None:
        return False
    if t.startswith("q("):
        q = SwigType_parm(t)
        if q is not None and q.find("const") >= 0:
            return True
    # might be const through a typedef
    if SwigType_issimple(t):
        td = SwigType_typedef_resolve(t)
        if td != t:
            return SwigType_isconst(td)
    return False


# ---------------------------------------------------------------------------
# internal splitters / parsers
# ---------------------------------------------------------------------------
def SwigType_parm(t: str) -> Optional[str]:
    start = t.find("(")
    if start < 0:
        return None
    start += 1
    nparens = 0
    c = start
    while c < len(t):
        if t[c] == ")":
            if nparens == 0:
                break
            nparens -= 1
        elif t[c] == "(":
            nparens += 1
        c += 1
    return t[start:c]


def SwigType_templateparmlist(t: str) -> List[str]:
    i = t.find("<")
    return SwigType_parmlist(t[i:])


def SwigType_parmlist(p: str) -> List[str]:
    lst: List[str] = []
    assert p, "Cannot pass null to SwigType_parmlist"
    itemstart = p.find("(")
    dot = p.find(".")
    assert dot == -1 or dot > itemstart, f"{p} is expected to contain sub elements of a type"
    itemstart += 1
    c = itemstart
    while c < len(p):
        if p[c] == ",":
            lst.append(p[itemstart:c])
            itemstart = c + 1
        elif p[c] == "(":
            nparens = 1
            c += 1
            while c < len(p):
                if p[c] == "(":
                    nparens += 1
                if p[c] == ")":
                    nparens -= 1
                    if nparens == 0:
                        break
                c += 1
        elif p[c] == ")":
            break
        if c < len(p):
            c += 1

    if c != itemstart:
        lst.append(p[itemstart:c])
    return lst


def SwigType_namestr(t: str) -> str:
    c = t.find("<(")
    if c < 0 or t.find(")>", c + 2) < 0:
        return t

    r = t[:c]
    if t[c - 1] == "<":
        r += " "
    r += "<"

    p = SwigType_parmlist(t[c + 1:])
    for i in range(len(p)):
        s = SwigType_str(p[i], None)
        # avoid creating a '<:' token (same as '[' in C++): space after '<'
        if i == 0 and len(s) > 0:
            r += " "
        r += s
        if (i + 1) < len(p):
            r += ","
    r += " >"
    suffix = SwigType_templatesuffix(t)
    if len(suffix) > 0:
        r += SwigType_namestr(suffix)
    else:
        r += suffix
    return r


def SwigType_templatesuffix(t: str) -> str:
    c = 0
    while c < len(t):
        if (t[c] == "<") and (t[c + 1] == "("):
            nest = 1
            c += 1
            while c < len(t) and nest != 0:
                if t[c] == "<":
                    nest += 1
                if t[c] == ">":
                    nest -= 1
                c += 1
            return t[c:]
        c += 1
    return ""


def SwigType_split(t: str) -> List[str]:
    lst: List[str] = []
    c = 0
    while c < len(t):
        length = element_size(t[c:])
        item = t[c:c + length]
        lst.append(item)
        c = c + length
        if c < len(t) and t[c] == ".":
            c += 1
    return lst


def element_size(s: str) -> int:
    c = 0
    while c < len(s):
        if s[c] == ".":
            c += 1
            return c
        elif s[c] == "(":
            nparen = 1
            c += 1
            while c < len(s):
                if s[c] == "(":
                    nparen += 1
                if s[c] == ")":
                    nparen -= 1
                    if nparen == 0:
                        break
                c += 1
        if c < len(s):
            c += 1
    return c


def SwigType_pop(t: str):
    """Pop one type element off the front. Returns (element, remainder)."""
    if t is None:
        return None
    sz = element_size(t)
    return (t[:sz], t[sz:])


def SwigType_issimple(t: str) -> bool:
    c = 0
    if not t:
        return False
    while c < len(t):
        if t[c] == "<":
            nest = 1
            c += 1
            while c < len(t) and nest != 0:
                if t[c] == "<":
                    nest += 1
                if t[c] == ">":
                    nest -= 1
                c += 1
            c -= 1
        if t[c] == ".":
            return False
        c += 1
    return True


# ---------------------------------------------------------------------------
# self-test (mirrors the Groovy main()/testPrint and adds hand-derived asserts)
# ---------------------------------------------------------------------------
def _test_print(ty: str, id: str = "foo") -> str:
    lt = SwigType_ltype(ty)
    line = f"{lt}|{SwigType_str(lt, id)}  = {ty}|{SwigType_str(ty, id)}"
    print(line)
    return line


if __name__ == "__main__":
    reset_type_table()
    append_type_table([
        {
            "namespace": "XBMCAddon::xbmcgui::",
            "type": "ListItemList",
            "basetype": "std::vector<(p.XBMCAddon::xbmcgui::ListItem)>",
        }
    ])

    # --- hand-derived expectations (deterministic from the algorithm; no JVM
    #     needed to verify these). See module-level derivations in the PR notes. ---
    checks = [
        # SwigType_str basic pointer-to-const-char
        (SwigType_str("p.q(const).char", "foo"), "char const *foo"),
        # ltype strips outer const, keeps pointer, drops inner const on simple base
        (SwigType_ltype("q(const).p.q(const).char"), "p.char"),
        (SwigType_str(SwigType_ltype("q(const).p.q(const).char"), "foo"), "char *foo"),
        # makepointer then ltype: 'q(const).p.q(const).char' -> double pointer
        (SwigType_makepointer("q(const).p.q(const).char"), "q(const).p.p.q(const).char"),
        (SwigType_ltype(SwigType_makepointer("q(const).p.q(const).char")), "p.p.char"),
        (SwigType_str(SwigType_ltype(SwigType_makepointer("q(const).p.q(const).char")), "bfoo"),
         "char **bfoo"),
        # reference-to-const-string by value -> drops the reference for a param ltype
        (convertTypeToLTypeForParam("r.q(const).String"), "String"),
        # split / parmlist sanity on a templated vector type
        (SwigType_split("p.q(const).char"), ["p.", "q(const).", "char"]),
        (SwigType_templateparmlist("std::vector<(p.XBMCAddon::xbmcgui::ListItem)>"),
         ["p.XBMCAddon::xbmcgui::ListItem"]),
        # typedef table resolution (ListItemList -> vector<...>)
        (SwigType_typedef_resolve("XBMCAddon::xbmcgui::ListItemList"),
         "std::vector<(p.XBMCAddon::xbmcgui::ListItem)>"),
        # predicates
        (SwigType_ispointer("p.char"), True),
        (SwigType_ispointer("q(const).p.char"), True),
        (SwigType_isconst("q(const).p.char"), True),
        (SwigType_isfunction("f(int,int).int"), True),
        (SwigType_issimple("std::vector<(p.int)>"), True),
        (SwigType_issimple("p.q(const).char"), False),
    ]

    failures = 0
    for i, (got, want) in enumerate(checks):
        ok = got == want
        if not ok:
            failures += 1
        print(f"[{'ok ' if ok else 'FAIL'}] case {i:2d}: got={got!r} want={want!r}")

    # a few descriptive prints, like the Groovy testPrint
    print("-" * 60)
    _test_print(SwigType_makepointer("q(const).p.q(const).char"), "bfoo")
    _test_print("f(r.q(const).String,p.q(const).XBMCAddon::xbmcgui::ListItem,bool)")
    _test_print("std::vector<(p.String)>")

    print("-" * 60)
    print(f"{len(checks) - failures}/{len(checks)} checks passed")
    raise SystemExit(1 if failures else 0)
