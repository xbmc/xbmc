"""Port of xbmc/interfaces/python/PythonTools.groovy.

Python-specific helpers used by the emitter: PyArg format-char mapping, the
PyArg_ParseTupleAndKeywords format string, class-name-as-variable mangling, the
generated CPython method names, docstrings, and base-class resolution.

Behavioural identity with the Groovy original is the goal; keep the asserts.
"""

from __future__ import annotations

import re

import helper as H
import method_type as MT
import swig_type_parser as SwigTypeParser


# api spec ltype -> python parse format char. default (lookup miss) is 'O'.
ltypeToFormatChar = {
    "p.char": "s", "bool": "b",
    "int": "i", "unsigned int": "I",
    "long": "l", "unsigned long": "k",
    "double": "d", "float": "f",
    "long long": "L",
}


def parameterCanBeUsedDirectly(param):
    return ltypeToFormatChar.get(SwigTypeParser.convertTypeToLTypeForParam(param.attr("type"))) is not None


def makeFormatStringFromParameters(method):
    if not method:
        return ""
    params = method.kids("parm")
    fmt = ""
    previousDefaulted = False
    for param in params:
        defaultValue = param.attr("value")
        paramtype = SwigTypeParser.convertTypeToLTypeForParam(param.attr("type"))
        curFormat = ltypeToFormatChar.get(paramtype)
        if curFormat is None:  # then we will assume it's an object
            curFormat = "O"
        if defaultValue is not None and not previousDefaulted:
            fmt += "|"
            previousDefaulted = True
        fmt += curFormat
    return fmt


def getClassNameAsVariable(clazz):
    return H.findFullClassName(clazz).replace("::", "_")


def getPyMethodName(method, methodType):
    full = H.findFullClassName(method)
    clazz = full.replace("::", "_") if full is not None else None

    assert (clazz is not None or methodType == MT.method), \
        "Cannot use a non-class function as a constructor or destructor " + str(method)
    assert (method.name() != "class" or methodType in (MT.constructor, MT.destructor))
    assert (method.name() != "constructor" or methodType == MT.constructor), \
        "Cannot use a constructor node and not identify the type as a constructor" + str(method)
    assert (method.name() != "destructor" or methodType == MT.destructor), \
        "Cannot use a destructor node and not identify the type as a destructor" + str(method)

    if clazz is None:
        return method.attr("sym_name")

    if methodType == MT.constructor:
        return clazz + "_New"

    if methodType == MT.destructor:
        return clazz + "_Dealloc"

    name = method.attr("name")
    if name is not None and name.startswith("operator "):
        if name[9:] == "[]":
            return clazz + "_operatorIndex_"
        if name[9:] == "()":
            return clazz + "_callable_"

    return clazz + "_" + method.attr("sym_name")


def makeDocString(docnode):
    if docnode is None or docnode.name() != "doc":
        raise RuntimeError("Invalid doc Node passed to PythonTools.makeDocString (" + str(docnode) + ")")

    newline = "\n"
    lines = docnode.attr("value").split(newline)
    ret = ""
    n = len(lines)
    for index, val in enumerate(lines):
        val = re.sub(r"\\n", "", val)          # remove extraneous \n's
        val = val.replace("\\", "\\\\")        # escape backslash
        val = re.sub(r"\"", "\\\"", val)       # escape quotes
        ret += ('"' + val + '\\n"' + (newline if index != n - 1 else ""))
    return ret


def findValidBaseClass(clazz, module, warn=False):
    baselists = clazz.kids("baselist")
    assert len(baselists) < 2, \
        str(clazz) + " has multiple baselists - need to write code to separate out the public one."
    knownbases = []
    if baselists:
        for b in baselists[0].kids("base"):
            baseclassnode = H.findClassNodeByName(module, b.attr("name"), clazz)
            if baseclassnode:
                knownbases.append(baseclassnode)
            elif warn and not H.isKnownBaseType(b.attr("name"), clazz):
                print("WARNING: the base class %s for %s is unrecognized within %s." %
                      (b.attr("name"), H.findFullClassName(clazz), module.attr("name")))
    assert len(knownbases) < 2, \
        ("The class %s has too many known base classes. Multiple inheritance isn't supported "
         "in the code generator. Please \"#ifdef SWIG\" out all but one." % H.findFullClassName(clazz))
    return knownbases[0] if knownbases else None
