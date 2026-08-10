"""Port of the 12 xbmc/interfaces/python/typemaps/*.{in,out}tm mini-templates.

Each is a function (bindings dict -> str) producing the EXACT byte stream the
Groovy SimpleTemplateEngine produced for that typemap (leading/trailing
whitespace included), since the goldens are raw (un-clang-formatted) output.

Recursive typemaps (vector/Tuple/Alternative/smart_ptr) call back into
helper.getIn/OutConversion and thread the shared `sequence` so the generated
temporary names (entryN/pyentryN) match Groovy exactly.
"""

from __future__ import annotations

import helper as H
import swig_type_parser as stp


# --- out: std::string -------------------------------------------------------
def string_out(b):
    api = b["api"]
    enc = "strict" if b["method"].attr("feature_python_strictUnicode") else "surrogateescape"
    return '%s = PyUnicode_DecodeUTF8(%s.c_str(),%s.size(),"%s");\n' % (b["result"], api, api, enc)


# --- out: XbmcCommons::Buffer ----------------------------------------------
def buffer_out(b):
    accessor = "->" if stp.SwigType_ispointer(b["type"]) else "."
    api = b["api"]
    return "%s = PyByteArray_FromStringAndSize((char*)%s%scurPosition(),%s%sremaining());" % (
        b["result"], api, accessor, api, accessor)


# --- in: XbmcCommons::Buffer ------------------------------------------------
_BUFFER_IN = r'''
    if (PyUnicode_Check(${slarg}))
    {
      Py_ssize_t pysize;
      const char* str = PyUnicode_AsUTF8AndSize(${slarg}, &pysize);
      size_t size = static_cast<size_t>(pysize);
      ${api}.allocate(size);
      ${api}.put(str, size);
      ${api}.flip(); // prepare the buffer for reading from
    }
    else if (PyBytes_Check(${slarg}))
    {
      Py_ssize_t pysize = PyBytes_GET_SIZE(${slarg});
      const char* str = PyBytes_AS_STRING(${slarg});
      size_t size = static_cast<size_t>(pysize);
      ${api}.allocate(size);
      ${api}.put(str, size);
      ${api}.flip(); // prepare the buffer for reading from
    }
    else if (PyByteArray_Check(${slarg}))
    {
      size_t size = PyByteArray_Size(${slarg});
      ${api}.allocate(size);
      ${api}.put(PyByteArray_AsString(${slarg}),size);
      ${api}.flip(); // prepare the buffer for reading from
    }
    else
    {
      throw XBMCAddon::WrongTypeException("argument \"%s\" for \"%s\" must be a string, bytes or a bytearray", "${api}", "${mname}");
    }'''


def buffer_in(b):
    return (_BUFFER_IN
            .replace("${slarg}", b["slarg"])
            .replace("${api}", b["api"])
            .replace("${mname}", b["method"].attr("name")))


# --- in: std::map<(K,V)> ----------------------------------------------------
def map_in(b):
    targs = stp.SwigType_templateparmlist(b["ltype"])
    keytype, valtype = targs[0], targs[1]
    keyltype = stp.SwigType_str(stp.SwigType_ltype(keytype))
    valltype = stp.SwigType_str(stp.SwigType_ltype(valtype))
    method = b["method"]
    keyconv = H.getInConversion(keytype, "key", "pykey", method)
    valconv = H.getInConversion(valtype, "value", "pyvalue", method)
    slarg, api = b["slarg"], b["api"]
    return (
        "\n"
        "    {\n"
        "      PyObject *pykey, *pyvalue;\n"
        "      Py_ssize_t pos = 0;\n"
        "      while(PyDict_Next(" + slarg + ", &pos, &pykey, &pyvalue))\n"
        "      {\n"
        "        " + keyltype + " key;\n"
        "        " + valltype + " value;\n"
        "        " + keyconv + "\n"
        "        " + valconv + "\n"
        "        " + api + ".emplace(std::move(key), std::move(value));\n"
        "      }\n"
        "    }")


# --- in: XBMCAddon::Dictionary<(V)> -----------------------------------------
def dict_in(b):
    targs = stp.SwigType_templateparmlist(b["ltype"])
    valtype = targs[0]
    valltype = stp.SwigType_str(stp.SwigType_ltype(valtype))
    method = b["method"]
    valconv = H.getInConversion(valtype, "value", "pyvalue", method)
    slarg, api = b["slarg"], b["api"]
    mname = method.attr("name")
    return (
        "\n"
        "    {\n"
        "      PyObject *pykey, *pyvalue;\n"
        "      Py_ssize_t pos = 0;\n"
        "      while(PyDict_Next(" + slarg + ", &pos, &pykey, &pyvalue))\n"
        "      {\n"
        "        std::string key;\n"
        '        PyXBMCGetUnicodeString(key,pykey,false,"' + api + '","' + mname + '");\n'
        "        " + valltype + " value;\n"
        "        " + valconv + "\n"
        "        " + api + ".emplace(std::move(key), std::move(value));\n"
        "      }\n"
        "    }")


# --- in: std::vector<(T)> ---------------------------------------------------
def vector_in(b):
    ltype, typ = b["ltype"], b["type"]
    slarg, api, method, sequence = b["slarg"], b["api"], b["method"], b["sequence"]
    vectype = stp.SwigType_templateparmlist(ltype)[0]
    ispointer = stp.SwigType_ispointer(typ)
    accessor = "->" if ispointer else "."
    seq = str(sequence.increment())
    entryltype = stp.SwigType_str(stp.SwigType_ltype(vectype))
    nested = H.getInConversion(vectype, "entry" + seq, "pyentry" + seq, method,
                               {"type": vectype, "ltype": stp.SwigType_ltype(vectype), "sequence": sequence})
    if stp.SwigType_ispointer(vectype) or vectype in ("bool", "double", "int"):
        pushval = "entry" + seq
    else:
        pushval = "std::move(entry" + seq + ")"
    line21 = "      "
    if ispointer:
        line21 += api + " = new std::vector<" + stp.SwigType_str(vectype) + ">();"
    return (
        "\n"
        "    if (" + slarg + ")\n"
        "    {\n"
        "      bool isTuple = PyObject_TypeCheck(" + slarg + ",&PyTuple_Type);\n"
        "      if (!isTuple && !PyObject_TypeCheck(" + slarg + ",&PyList_Type))\n"
        '        throw WrongTypeException("The parameter \\"' + api + '\\" must be either a Tuple or a List.");\n'
        "\n"
        + line21 + "\n"
        "      PyObject *pyentry" + seq + " = NULL;\n"
        "      Py_ssize_t vecSize = (isTuple ? PyTuple_Size(" + slarg + ") : PyList_Size(" + slarg + "));\n"
        "      " + api + accessor + "reserve(vecSize);\n"
        "      for(Py_ssize_t i = 0; i < vecSize; i++)\n"
        "      {\n"
        "        pyentry" + seq + " = (isTuple ? PyTuple_GetItem(" + slarg + ", i) : PyList_GetItem(" + slarg + ", i));\n"
        "        " + entryltype + " entry" + seq + ";\n"
        "        " + nested + "\n"
        "        " + api + accessor + "push_back(" + pushval + ");\n"
        "      }\n"
        "    }\n")


# --- out: std::vector<(T)> --------------------------------------------------
def vector_out(b):
    typ = b["type"]
    api, result, method, sequence = b["api"], b["result"], b["method"], b["sequence"]
    vectype = stp.SwigType_templateparmlist(typ)[0]
    ispointer = stp.SwigType_ispointer(typ)
    accessor = "->" if ispointer else "."
    seq = str(sequence.increment())
    nested = H.getOutConversion(vectype, "result", method,
                                {"result": "pyentry" + seq, "api": "(*iter)", "sequence": sequence})
    out = ""
    if ispointer:
        out += "\n    if (" + api + " != NULL)\n    {\n"
    out += (
        "\n      " + result + " = PyList_New(0);\n"
        "\n"
        "      for (std::vector<" + stp.SwigType_str(vectype) + ">::iterator iter = " + api + accessor + "begin(); iter != " + api + accessor + "end(); ++iter)\n"
        "      {\n"
        "        PyObject* pyentry" + seq + ";\n"
        "        " + nested + "\n"
        "        PyList_Append(" + result + ", pyentry" + seq + ");\n"
        "        Py_DECREF(pyentry" + seq + ");\n"
        "      }\n")
    if ispointer:
        out += "\n    }\n"
    out += "\n"
    return out


# --- in: Tuple<(...)> -------------------------------------------------------
def tuple_in(b):
    ltype, typ = b["ltype"], b["type"]
    slarg, api, method, sequence = b["slarg"], b["api"], b["method"], b["sequence"]
    types = stp.SwigType_templateparmlist(ltype)
    ispointer = stp.SwigType_ispointer(typ)
    accessor = "->" if ispointer else "."
    seq = str(sequence.increment())
    tupleAccess = ["first", "second", "third", "fourth"]
    out = (
        "\n"
        "    if(" + slarg + ")\n"
        "    {\n"
        "      bool isTuple = PyObject_TypeCheck(" + slarg + ",&PyTuple_Type);\n"
        "      if (!isTuple && !PyObject_TypeCheck(" + slarg + ",&PyList_Type))\n"
        '        throw WrongTypeException("The parameter \\"' + api + '\\" must be either a Tuple or a List.");\n'
        "      Py_ssize_t vecSize = (isTuple ? PyTuple_Size(" + slarg + ") : PyList_Size(" + slarg + "));\n")
    for entryIndex, curType in enumerate(types):
        ei = str(entryIndex)
        entryltype = stp.SwigType_str(stp.SwigType_ltype(curType))
        nested = H.getInConversion(curType, "entry" + ei + "_" + seq, "pyentry" + ei + "_" + seq, method,
                                   {"sequence": sequence})
        out += (
            "\n"
            "      if (vecSize > " + ei + ")\n"
            "      {\n"
            "        PyObject *pyentry" + ei + "_" + seq + " = NULL;\n"
            "        pyentry" + ei + "_" + seq + " = (isTuple ? PyTuple_GetItem(" + slarg + ", " + ei + ") : PyList_GetItem(" + slarg + ", " + ei + "));\n"
            "        " + entryltype + " entry" + ei + "_" + seq + ";\n"
            "        " + nested + "\n"
            "        " + api + accessor + tupleAccess[entryIndex] + "() = entry" + ei + "_" + seq + ";\n"
            "      }\n")
    out += "\n    }\n"
    return out


# --- out: Tuple<(...)> ------------------------------------------------------
def tuple_out(b):
    typ = b["type"]
    api, result, method, sequence = b["api"], b["result"], b["method"], b["sequence"]
    types = stp.SwigType_templateparmlist(typ)
    ispointer = stp.SwigType_ispointer(typ)
    accessor = "->" if ispointer else "."
    seq = str(sequence.increment())
    tupleAccess = ["first", "second", "third", "fourth"]
    out = (
        "\n"
        "    int vecSize = " + api + accessor + "GetNumValuesSet();\n"
        "    " + result + " = PyTuple_New(vecSize);\n")
    if ispointer:
        out += "\n    if (" + api + " != NULL)\n"
    out += "    {\n      PyObject* pyentry" + seq + "; "
    for entryIndex, curType in enumerate(types):
        ei = str(entryIndex)
        lrt = stp.SwigType_str(stp.SwigType_lrtype(curType))
        nested = H.getOutConversion(curType, "result", method,
                                    {"result": "pyentry" + seq, "api": "entry" + seq, "sequence": sequence})
        out += (
            "\n"
            "\n"
            "      if (vecSize > " + ei + ")\n"
            "      {\n"
            "        " + lrt + " entry" + seq + " = " + api + accessor + tupleAccess[entryIndex] + "();\n"
            "        {\n"
            "          " + nested + "\n"
            "        }\n"
            "        PyTuple_SetItem(" + result + ", " + ei + ", pyentry" + seq + ");\n"
            "      }\n")
    out += "\n    }"
    return out


# --- in: Alternative<(A,B)> -------------------------------------------------
def alternative_in(b):
    ltype = b["ltype"]
    api, method, sequence = b["api"], b["method"], b["sequence"]
    ispointer = stp.SwigType_ispointer(ltype)
    accessor = "->" if ispointer else "."
    seq = str(sequence.increment())
    altAccess = ["former", "later"]
    types = stp.SwigType_templateparmlist(ltype)
    lt0 = stp.SwigType_str(stp.SwigType_ltype(types[0]))
    lt1 = stp.SwigType_str(stp.SwigType_ltype(types[1]))
    conv0 = H.getInConversion(types[0], "entry0" + "_" + seq, "pyentry" + "_" + seq, method, {"sequence": sequence})
    conv1 = H.getInConversion(types[1], "entry1" + "_" + seq, "pyentry" + "_" + seq, method, {"sequence": sequence})
    msg0 = stp.SwigType_ltype(types[0])
    msg1 = stp.SwigType_ltype(types[1])
    return (
        "\n"
        "    {\n"
        "      // we need to check the parameter type and see if it matches\n"
        "      PyObject *pyentry_" + seq + " = " + b["slarg"] + ";\n"
        "      try\n"
        "      {\n"
        "        " + lt0 + " entry0_" + seq + ";\n"
        "        " + conv0 + "\n"
        "        " + api + accessor + altAccess[0] + "() = entry0_" + seq + ";\n"
        "      }\n"
        "      catch (const XBMCAddon::WrongTypeException&)\n"
        "      {\n"
        "        try\n"
        "        {\n"
        "          " + lt1 + " entry1_" + seq + ";\n"
        "          " + conv1 + "\n"
        "          " + api + accessor + altAccess[1] + "() = entry1_" + seq + ";\n"
        "        }\n"
        "        catch (const XBMCAddon::WrongTypeException&)\n"
        "        {\n"
        '          throw XBMCAddon::WrongTypeException("Failed to convert to input type to either a "\n'
        '                                              "' + msg0 + ' or a "\n'
        '                                              "' + msg1 + '" );\n'
        "        }\n"
        "      }\n"
        "    }")


# --- out: Alternative<(A,B)> ------------------------------------------------
def alternative_out(b):
    typ = b["type"]
    api, result, method, sequence = b["api"], b["result"], b["method"], b["sequence"]
    types = stp.SwigType_templateparmlist(typ)
    ispointer = stp.SwigType_ispointer(typ)
    accessor = "->" if ispointer else "."
    seq = str(sequence.increment())
    altAccess = ["former", "later"]
    altSwitch = ["first", "second"]
    nullcheck = (api + " != NULL && ") if ispointer else ""
    out = (
        "\n"
        "    WhichAlternative pos = " + api + accessor + "which();\n"
        "\n"
        "    if (" + nullcheck + "pos != XBMCAddon::none)\n"
        "    { ")
    for entryIndex, curType in enumerate(types):
        lrt = stp.SwigType_str(stp.SwigType_lrtype(curType))
        nested = H.getOutConversion(curType, result, method, {"api": "entry" + seq, "sequence": sequence})
        out += (
            "\n"
            "      if (pos == XBMCAddon::" + altSwitch[entryIndex] + ")\n"
            "      {\n"
            "        " + lrt + " entry" + seq + " = " + api + accessor + altAccess[entryIndex] + "();\n"
            "        {\n"
            "          " + nested + "\n"
            "        }\n"
            "      }\n")
    out += (
        "\n    }\n"
        "    else\n"
        "      " + result + " = Py_None;")
    return out


# --- out: shared_ptr / unique_ptr -------------------------------------------
def smart_ptr_out(b):
    typ = b["type"]
    api, method, sequence = b["api"], b["method"], b["sequence"]
    itype = stp.SwigType_templateparmlist(typ)[0]
    pointertype = stp.SwigType_makepointer(itype)
    seq = str(sequence.increment())
    ltype = stp.SwigType_str(stp.SwigType_ltype(pointertype))
    nested = H.getOutConversion(pointertype, "result", method, {"api": "entry" + seq, "sequence": sequence})
    return "\n    " + ltype + " entry" + seq + " = " + api + ".get();\n    " + nested + "\n"
