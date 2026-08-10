"""Port of xbmc/interfaces/python/PythonSwig.cpp.template.

Reproduces the raw (un-clang-formatted) byte stream the Groovy
SimpleTemplateEngine produced. Whitespace is significant; literal spans are
copied verbatim from the template and computed values spliced in at the same
points. The inline typemap registry (the Helper.setup call at the top of the
template) is built here and handed to helper.setup.
"""

from __future__ import annotations

import re
import sys

import helper as H
import python_tools as PT
import method_type as MT
import swig_type_parser as stp
import typemaps as TM

SEP = "  //" + "=" * 73


# Groovy declares known api types in a java.util.HashSet and iterates it, so the
# extern declaration order is Java hash-bucket order, not insertion order.
def _java_string_hashcode(s):
    h = 0
    for ch in s:
        h = (h * 31 + ord(ch)) & 0xFFFFFFFF
    return h


def _java_hashset_order(items):
    cap = 16
    while len(items) > int(cap * 0.75):
        cap <<= 1
    buckets = [[] for _ in range(cap)]
    for it in items:
        h = _java_string_hashcode(it)
        h ^= (h >> 16)
        buckets[h & (cap - 1)].append(it)
    out = []
    for b in buckets:
        out.extend(b)
    return out


# --- inline typemap registry (was the Helper.setup(...) call) ---------------
def DEFAULT_OUT(b):
    return "%s = makePythonInstance(%s,true);" % (b["result"], b["api"])


def DEFAULT_IN(b):
    return '%s = (%s)retrieveApiInstance(%s,"%s","%s","%s");' % (
        b["api"], stp.SwigType_str(b["ltype"]), b["slarg"], b["ltype"],
        H.findNamespace(b["method"]), H.callingName(b["method"]))


OUT_EXACT = {
    "void": lambda b: "Py_INCREF(Py_None);\n    %s = Py_None;" % b["result"],
    "long": lambda b: "%s = PyLong_FromLong(%s);" % (b["result"], b["api"]),
    "unsigned long": lambda b: "%s = PyLong_FromLong(%s);" % (b["result"], b["api"]),
    "bool": lambda b: "%s = %s ? Py_True : Py_False; Py_INCREF(%s);" % (b["result"], b["api"], b["result"]),
    "long long": lambda b: '%s = Py_BuildValue("L", %s);' % (b["result"], b["api"]),
    "int": lambda b: '%s = Py_BuildValue("i", %s);' % (b["result"], b["api"]),
    "unsigned int": lambda b: '%s = Py_BuildValue("I", %s);' % (b["result"], b["api"]),
    "double": lambda b: "%s = PyFloat_FromDouble(%s);" % (b["result"], b["api"]),
    "float": lambda b: '%s = Py_BuildValue("f", static_cast<double>(%s));' % (b["result"], b["api"]),
    "std::string": TM.string_out,
    "p.q(const).char": lambda b: "%s = PyUnicode_FromString(%s);" % (b["result"], b["api"]),
}

OUT_PATTERNS = [
    (re.compile(r"(p.){0,1}XbmcCommons::Buffer"), TM.buffer_out),
    (re.compile(r"std::shared_ptr<\(.*\)>"), TM.smart_ptr_out),
    (re.compile(r"std::unique_ptr<\(.*\)>"), TM.smart_ptr_out),
    (re.compile(r"(p.){0,1}std::vector<\(.*\)>"), TM.vector_out),
    (re.compile(r"(p.){0,1}Tuple<\(.*\)>"), TM.tuple_out),
    (re.compile(r"(p.){0,1}Alternative<\(.*\)>"), TM.alternative_out),
]

IN_EXACT = {
    "std::string": lambda b: 'if (%s) PyXBMCGetUnicodeString(%s,%s,false,"%s","%s");' % (
        b["slarg"], b["api"], b["slarg"], b["api"], b["method"].attr("name")),
    "bool": lambda b: "%s = (PyLong_AsLong(%s) == 0L ? false : true);" % (b["api"], b["slarg"]),
    "long": lambda b: "%s = PyLong_AsLong(%s);" % (b["api"], b["slarg"]),
    "unsigned long": lambda b: "%s = PyLong_AsUnsignedLong(%s);" % (b["api"], b["slarg"]),
    "long long": lambda b: "%s = PyLong_AsLongLong(%s);" % (b["api"], b["slarg"]),
    "unsigned long long": lambda b: "%s = PyLong_AsUnsignedLongLong(%s);" % (b["api"], b["slarg"]),
    "int": lambda b: "%s = (int)PyLong_AsLong(%s);" % (b["api"], b["slarg"]),
    "double": lambda b: "%s = PyFloat_AsDouble(%s);" % (b["api"], b["slarg"]),
    "float": lambda b: "%s = (float)PyFloat_AsDouble(%s);" % (b["api"], b["slarg"]),
    "XBMCAddon::StringOrInt": lambda b: 'if (%s) PyXBMCGetUnicodeString(%s,%s,PyLong_Check(%s) || PyFloat_Check(%s),"%s","%s");' % (
        b["slarg"], b["api"], b["slarg"], b["slarg"], b["slarg"], b["api"], b["method"].attr("name")),
}

IN_PATTERNS = [
    (re.compile(r"(p.){0,1}std::vector<\(.*\)>"), TM.vector_in),
    (re.compile(r"(p.){0,1}Tuple(3){0,1}<\(.*\)>"), TM.tuple_in),
    (re.compile(r"(p.){0,1}Alternative<\(.*\)>"), TM.alternative_in),
    (re.compile(r"(r.){0,1}XbmcCommons::Buffer"), TM.buffer_in),
    (re.compile(r"(p.){0,1}std::map<\(.*\)>"), TM.map_in),
    (re.compile(r"(r.){0,1}XBMCAddon::Dictionary<\(.*\)>"), TM.dict_in),
    (re.compile(r"p.void"), lambda b: "%s = (void*)%s;" % (b["api"], b["slarg"])),
]


class Gen:
    def __init__(self, module):
        self.module = module
        self.mod = module.attr("name")
        self.o = []
        self.classes = [n for n in module.depth_first() if n.name() == "class"]
        self.methods = [n for n in module.depth_first()
                        if n.name() in ("function", "constructor", "destructor")]
        self.initTypeCalls = []
        self.classNameAsVariables = []

    def e(self, s):
        self.o.append(s)

    # --- doMethod -----------------------------------------------------------
    def doMethod(self, method, methodType):
        e = self.e
        mod = self.mod
        isOperator = method.attr("name").startswith("operator ")
        doAsMappingIndex = False
        doAsCallable = False
        if isOperator:
            tail = method.attr("name")[9:]
            if tail == "[]":
                doAsMappingIndex = True
            elif tail == "()":
                doAsCallable = True
            else:
                return

        constructor = methodType == MT.constructor
        if constructor and method.attr("access") is not None and method.attr("access") != "public":
            return
        destructor = methodType == MT.destructor
        params = method.kids("parm")
        numParams = len(params)
        clazz = H.findFullClassName(method)
        returns = ("p." + clazz) if constructor else ("void" if destructor else H.getReturnSwigType(method))
        classnode = H.findClassNode(method)
        classNameAsVariable = None if clazz is None else PT.getClassNameAsVariable(classnode)
        useKeywordParsing = not ((classnode is not None and classnode.attr("feature_python_nokwds") == "true")
                                 or method.attr("feature_python_nokwds") == "true")

        if not constructor and not destructor:
            if H.hasDoc(method):
                e("\n  PyDoc_STRVAR(%s__doc__,\n               %s);\n" %
                  (PT.getPyMethodName(method, methodType), PT.makeDocString(method.kids("doc")[0])))

        # signature
        rettype = "void" if destructor else "PyObject*"
        selfty = "PyObject" if clazz is None else ("PyTypeObject" if constructor else "PyHolder")
        selfname = "pytype" if constructor else "self"
        sig = "\n  static %s %s_%s (%s* %s " % (
            rettype, mod, PT.getPyMethodName(method, methodType), selfty, selfname)
        if doAsMappingIndex:
            sig += ", PyObject* py%s" % params[0].attr("name")
        elif not destructor:
            sig += " , PyObject *args, PyObject *kwds "
        sig += " )"
        e(sig)
        e("\n  {\n    XBMC_TRACE;\n")

        if numParams > 0:
            if useKeywordParsing and not doAsMappingIndex:
                e("\n    static const char *keywords[] = {")
                for it in params:
                    e('\n          "%s",' % it.attr("name"))
                e("\n          NULL};\n")
            for it in params:
                value = it.attr("value")
                if value is not None:
                    valpart = " = " + value
                elif stp.SwigType_ispointer(it.attr("type")):
                    valpart = " = nullptr"
                else:
                    valpart = ""
                e("\n    %s %s %s;" % (
                    stp.SwigType_str(stp.convertTypeToLTypeForParam(it.attr("type"))), it.attr("name"), valpart))
                if not PT.parameterCanBeUsedDirectly(it) and not doAsMappingIndex:
                    e("\n    PyObject* py%s = NULL;" % it.attr("name"))
            if not doAsMappingIndex:
                e("\n    if (!%s(\n       args,\n       " %
                  ("PyArg_ParseTupleAndKeywords" if useKeywordParsing else "PyArg_ParseTuple"))
                if useKeywordParsing:
                    e("kwds,")
                e('\n       "%s",\n       ' % PT.makeFormatStringFromParameters(method))
                if useKeywordParsing:
                    e("const_cast<char**>(keywords),")
                for i, param in enumerate(params):
                    e("\n         &%s%s%s" % (
                        "" if PT.parameterCanBeUsedDirectly(param) else "py",
                        param.attr("name"),
                        "," if i < len(params) - 1 else ""))
                e("\n       ))\n    {\n      return NULL;\n    }\n\n")

        if returns != "void":
            e("    %s apiResult;" % stp.SwigType_str(returns))
        e("\n    try\n    {\n")

        # input conversions
        for it in params:
            if (not PT.parameterCanBeUsedDirectly(it)) or doAsMappingIndex:
                e("      %s \n" % H.getInConversion(it.attr("type"), it.attr("name"), "py" + it.attr("name"), method))
        e("\n")

        isDirectorCall = H.isDirector(method)
        if isDirectorCall:
            e("      // This is a director call coming from python so it explicitly calls the base class method.\n")

        if not destructor:
            if constructor or not clazz:
                e("      XBMCAddon::SetLanguageHookGuard slhg(XBMCAddon::Python::PythonLanguageHook::GetIfExists(PyThreadState_Get()->interp).get());\n")
            e("      ")
            if returns != "void":
                e("apiResult = ")
            if clazz and not constructor:
                e('((%s*)retrieveApiInstance((PyObject*)self,&Ty%s_Type,"%s","%s"))-> ' %
                  (clazz, classNameAsVariable, H.callingName(method), clazz))
            if constructor and classnode.attr("feature_director"):
                e("(&(Ty%s_Type.pythonType) != pytype) ? new %s_Director(" % (classNameAsVariable, classNameAsVariable))
                for i, param in enumerate(params):
                    e(" %s%s " % (param.attr("name"), "," if i < len(params) - 1 else ""))
                e(") : ")
            if isDirectorCall:
                e("%s::" % clazz)
            e("%s( " % H.callingName(method))
            for i, param in enumerate(params):
                e(" %s%s " % (param.attr("name"), "," if i < len(params) - 1 else ""))
            e(" );\n")
            if constructor:
                e("      prepareForReturn(apiResult);")
        else:
            e('\n      %s* theObj = (%s*)retrieveApiInstance((PyObject*)self,&Ty%s_Type,"~%s","%s");\n      cleanForDealloc(theObj);\n' %
              (clazz, clazz, classNameAsVariable, H.callingName(method), clazz))

        # catch blocks
        e("\n    }\n    catch (const XBMCAddon::WrongTypeException& e)\n    {\n"
          '      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n'
          "      PyErr_SetString(PyExc_TypeError, e.GetExMessage()); ")
        if not destructor:
            e("\n      return NULL; ")
        e("\n    }\n    catch (const XbmcCommons::Exception& e)\n    {\n"
          '      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n'
          "      PyErr_SetString(PyExc_RuntimeError, e.GetExMessage()); ")
        if not destructor:
            e("\n      return NULL; ")
        e('\n    }\n    catch (...)\n    {\n'
          '      CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call \\"%s\\"");\n'
          '      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call \\"%s\\""); ' %
          (H.callingName(method), H.callingName(method)))
        if not destructor:
            e("\n      return NULL; ")
        e("\n    }\n")

        # return section
        if not destructor:
            e("\n    PyObject* result = Py_None;\n\n    // transform the result\n")
            if constructor:
                e("    result = makePythonInstance(apiResult,pytype,false);")
            else:
                e("    %s" % H.getOutConversion(returns, "result", method))
            if constructor and method.attr("feature_director"):
                e("\n    if (&(Ty%s_Type.pythonType) != pytype)\n      ((%s_Director*)apiResult)->setPyObjectForDirector(result);" %
                  (classNameAsVariable, classNameAsVariable))
            e("\n\n    return result; ")
        else:
            e("\n    (((PyObject*)(self))->ob_type)->tp_free((PyObject*)self);\n    ")
        e("\n  } ")

    # --- doClassTypeInfo ----------------------------------------------------
    def doClassTypeInfo(self, clazz, classNameAsVariables=None):
        cnav = PT.getClassNameAsVariable(clazz)
        full = H.findFullClassName(clazz)
        if classNameAsVariables is not None:
            classNameAsVariables.append(cnav)
        self.e("\n%s\n  // These variables will hold the Python Type information for %s\n  TypeInfo Ty%s_Type(typeid(%s));\n%s\n" %
               (SEP, full, cnav, full, SEP))

    # --- doExternClassTypeInfo ----------------------------------------------
    def doExternClassTypeInfo(self, knownType):
        cnav = knownType.replace("::", "_")
        self.e("\n%s\n  // These variables define the type %s from another module\n  extern TypeInfo Ty%s_Type;\n%s\n" %
               (SEP, knownType, cnav, SEP))

    # --- getAllVirtualMethods -----------------------------------------------
    def getAllVirtualMethods(self, clazz):
        ret = [c for c in clazz.children()
               if c.name() == "function" and c.attr("storage") == "virtual"]
        baselists = clazz.kids("baselist")
        if baselists:
            for b in baselists[0].kids("base"):
                bcn = H.findClassNodeByName(self.module, b.attr("name"), clazz)
                if bcn and bcn.attr("feature_director"):
                    ret.extend(self.getAllVirtualMethods(bcn))
        return ret

    # --- doClassMethodInfo --------------------------------------------------
    def doClassMethodInfo(self, clazz, initTypeCalls):
        e = self.e
        mod = self.mod
        cnav = PT.getClassNameAsVariable(clazz)
        full = H.findFullClassName(clazz)
        initTypeCall = "initPy%s_Type" % cnav
        if initTypeCalls is not None:
            initTypeCalls.append(initTypeCall)

        doComparator = False
        doAsMapping = False
        hasEquivalenceOp = hasLtOp = hasGtOp = False
        indexOp = callableOp = sizeNode = None

        normalMethods = [it for it in clazz.kids("function") if not it.attr("name").startswith("operator ")]
        operators = [it for it in clazz.kids("function") if it.attr("name").startswith("operator ")]
        properties = [it for it in clazz.kids("variable")
                      if it.attr("access") is not None and it.attr("access") == "public"]
        properties_set = [it for it in properties
                          if it.attr("feature_immutable") is None or it.attr("feature_immutable") == 0]

        for it in operators:
            tail = it.attr("name")[9:]
            if tail.startswith("=="):
                hasEquivalenceOp = True
            elif tail == "<":
                hasLtOp = True
            elif tail == ">":
                hasGtOp = True
            elif tail == "[]":
                indexOp = it
            elif tail == "()":
                callableOp = it
            else:
                sys.stderr.write('Warning: class %s has an operator "%s" that is being ignored.\n' %
                                 (full, it.attr("name")))

        if hasGtOp or hasLtOp or hasEquivalenceOp:
            if not (hasLtOp and hasGtOp and hasEquivalenceOp):
                sys.stderr.write("Warning: class %s has an inconsistent operator set.\n" % full)
            else:
                doComparator = True

        if indexOp:
            sizeNode = next((f for f in clazz.kids("function") if f.attr("name") == "size"), None)
            if sizeNode:
                doAsMapping = True
            else:
                sys.stderr.write("Warning: class %s has an inconsistent operator set (need size + operator[]).\n" % full)

        if doAsMapping:
            e('\n  static Py_ssize_t %s_%s_size_(PyObject* self)\n  {\n'
              '    return (Py_ssize_t)((%s*)retrieveApiInstance(self,&Ty%s_Type,"%s","%s"))-> size();\n  }\n\n'
              '%s\n  // tp_as_mapping struct for %s\n%s\n  PyMappingMethods %s_%s_as_mapping = {\n'
              '    %s_%s_size_,    /* inquiry mp_length;                  __len__ */\n'
              '    (PyCFunction)%s_%s,   /* binaryfunc mp_subscript             __getitem__ */\n'
              '    0,                  /* objargproc mp_ass_subscript;     __setitem__ */\n  };\n' %
              (mod, cnav, full, cnav, H.callingName(indexOp), full,
               SEP, full, SEP, mod, cnav, mod, cnav, mod, PT.getPyMethodName(indexOp, MT.method)))

        if clazz.attr("feature_python_rcmp"):
            e("\n  static PyObject* %s_%s_rcmp(PyObject* obj1, PyObject *obj2, int method)\n  %s\n" %
              (mod, cnav, H.unescape(clazz.attr("feature_python_rcmp"))))

        e("\n%s\n  // This section contains the initialization for the\n"
          "  // Python extension for the Api class %s\n%s\n  // All of the methods on this class\n"
          "  static PyMethodDef %s_methods[] = { " % (SEP, full, SEP, cnav))
        for it in normalMethods:
            e('\n    {"%s", (PyCFunction)%s_%s, METH_VARARGS|METH_KEYWORDS, %s }, ' %
              (it.attr("sym_name"), mod, PT.getPyMethodName(it, MT.method),
               (PT.getPyMethodName(it, MT.method) + "__doc__") if H.hasDoc(it) else "NULL"))
        for key in [k for k in clazz.attributes.keys() if k.startswith("feature_python_method_")]:
            methodName = key[len("feature_python_method_"):]
            e('\n    {"%s", (PyCFunction)%s_%s_%s, METH_VARARGS|METH_KEYWORDS, NULL},\n' %
              (methodName, mod, PT.getClassNameAsVariable(clazz), methodName))
        e("\n    {NULL, NULL, 0, NULL}\n  };\n\n")

        if len(properties) > 0:
            clazzName = H.findFullClassName(properties[0])
            e("  static PyObject* %s_getMember(PyHolder *self, void *name)\n  {\n    if (self == NULL)\n      return NULL;\n" % cnav)
            e('\n    try\n    {\n      %s* theObj = (%s*)retrieveApiInstance((PyObject*)self, &Ty%s_Type, "%s_getMember()", "%s");\n\n      PyObject* result = NULL;\n   ' %
              (clazzName, clazzName, cnav, cnav, clazzName))
            for it in properties:
                returns = H.getPropertyReturnSwigType(it)
                e(' if (strcmp((char*)name, "%s") == 0)\n      {\n        %s apiResult = theObj->%s;\n        %s\n      }\n      else' %
                  (it.attr("sym_name"), stp.SwigType_lstr(returns), it.attr("sym_name"),
                   H.getOutConversion(returns, "result", it)))
            e('\n      {\n        Py_INCREF(Py_None);\n        return Py_None;\n      }\n\n      return result;\n    }\n'
              '    catch (const XBMCAddon::WrongTypeException& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_TypeError, e.GetExMessage());\n      return NULL;\n    }\n'
              '    catch (const XbmcCommons::Exception& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_RuntimeError, e.GetExMessage());\n      return NULL;\n    }\n'
              '    catch (...)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call \\"%s_getMember()\\"");\n      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call \\"%s_getMember()\\"");\n      return NULL;\n    }\n\n    return NULL;\n  }\n\n' %
              (cnav, cnav))
            if len(properties_set) > 0:
                e("  int %s_setMember(PyHolder *self, PyObject *value, void *name)\n  {\n    if (self == NULL)\n      return -1;\n\n    %s* theObj = NULL;\n    try\n    {\n      theObj = (%s*)retrieveApiInstance((PyObject*)self, &Ty%s_Type, \"%s_getMember()\", \"%s\");\n    }\n" %
                  (cnav, clazzName, clazzName, cnav, cnav, clazzName))
                e('    catch (const XBMCAddon::WrongTypeException& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_TypeError, e.GetExMessage());\n      return -1;\n    }\n'
                  '    catch (const XbmcCommons::Exception& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_RuntimeError, e.GetExMessage());\n      return -1;\n    }\n'
                  '    catch (...)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call \\"%s_getMember()\\"");\n      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call \\"%s_getMember()\\"");\n      return -1;\n    }\n\n' %
                  (cnav, cnav))
                for it in properties_set:
                    returns = H.getPropertyReturnSwigType(it)
                    e(' if (strcmp((char*)name, "%s") == 0)\n      {\n        %s tmp;\n        %s\n        if (PyErr_Occurred())\n          throw PythonBindings::PythonToCppException();\n\n        theObj->%s = tmp;\n      }\n      else' %
                      (it.attr("sym_name"), stp.SwigType_lstr(returns),
                       H.getInConversion(returns, "tmp", "value", it), it.attr("sym_name")))
                e("\n        return -1;\n\n    return 0;\n  } ")
            e("\n\n  // All of the methods on this class\n  static PyGetSetDef %s_getsets[] = { " % cnav)
            for it in properties:
                immutable = it.attr("feature_immutable") is None or it.attr("feature_immutable") == 0
                e('\n    {(char*)"%s", (getter)%s_getMember, %s, (char*)%s, (char*)"%s" }, ' %
                  (it.attr("sym_name"), cnav,
                   ("(setter)" + cnav + "_setMember") if immutable else "NULL",
                   PT.makeDocString(it.kids("doc")[0]) if H.hasDoc(it) else "NULL",
                   it.attr("sym_name")))
            e("\n    {NULL}\n  };\n")

        feat_iter = clazz.attr("feature_iterator")
        feat_iterable = clazz.attr("feature_iterable")
        if (feat_iter and feat_iter != "") or (feat_iterable and feat_iterable != ""):
            e("\n  static PyObject* %s_%s_iter(PyObject* self)\n  { " % (mod, cnav))
            if feat_iter:
                e("\n    return self; ")
            else:
                e('\n    PyObject* result = NULL;\n    try\n    {\n      %s* apiResult = ((%s*)retrieveApiInstance(self,&Ty%s_Type,"%s_%s_iternext","%s"))->begin();\n\n      %s\n    }\n'
                  '    catch (const XBMCAddon::WrongTypeException& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_TypeError, e.GetExMessage());\n      return NULL;\n    }\n'
                  '    catch (const XbmcCommons::Exception& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_RuntimeError, e.GetExMessage());\n      return NULL;\n    }\n'
                  '    catch (...)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call \\"%s_%s_iternext\\"");\n      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call \\"%s_%s_iternext\\"");\n      return NULL;\n    }\n\n    return result; ' %
                  (feat_iterable, full, cnav, mod, cnav, full,
                   H.getOutConversion("p." + feat_iterable, "result", clazz),
                   mod, cnav, mod, cnav))
            e("\n  }\n")
            if feat_iter:
                e('\n  static PyObject* %s_%s_iternext(PyObject* self)\n  {\n    PyObject* result = NULL;\n    try\n    {\n      %s* iter = (%s*)retrieveApiInstance(self,&Ty%s_Type,"%s_%s_iternext","%s");\n\n      // check if we have reached the end\n      if (!iter->end())\n      {\n        ++(*iter);\n\n        %s apiResult = **iter;\n        %s\n      }\n    }\n'
                  '    catch (const XBMCAddon::WrongTypeException& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_TypeError, e.GetExMessage());\n      return NULL;\n    }\n'
                  '    catch (const XbmcCommons::Exception& e)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: {}",e.GetExMessage());\n      PyErr_SetString(PyExc_RuntimeError, e.GetExMessage());\n      return NULL;\n    }\n'
                  '    catch (...)\n    {\n      CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call \\"%s_%s_iternext\\"");\n      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call \\"%s_%s_iternext\\"");\n      return NULL;\n    }\n\n    return result;\n  }\n' %
                  (mod, cnav, full, full, cnav, mod, cnav, full, feat_iter,
                   H.getOutConversion(feat_iter, "result", clazz), mod, cnav, mod, cnav))
        e("\n")

        # init method
        e("\n  // This method initializes the above mentioned Python Type structure\n  static void %s()\n  {\n" % initTypeCall)
        if H.hasDoc(clazz):
            e("\n    PyDoc_STRVAR(%s__doc__,\n                 %s\n                );\n" %
              (cnav, PT.makeDocString(clazz.kids("doc")[0])))
        e('\n\n    PyTypeObject& pythonType = Ty%s_Type.pythonType;\n    pythonType.tp_name = "%s.%s";\n    pythonType.tp_basicsize = sizeof(PyHolder);\n    pythonType.tp_dealloc = (destructor)%s_%s_Dealloc; ' %
          (cnav, mod, clazz.attr("sym_name"), mod, cnav))
        if clazz.attr("feature_python_rcmp"):
            e("\n    pythonType.tp_richcompare=(richcmpfunc)%s_%s_rcmp;" % (mod, cnav))
        e("\n\n    pythonType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;\n\n    pythonType.tp_doc = %s;\n    pythonType.tp_methods = %s_methods; " %
          ((cnav + "__doc__") if H.hasDoc(clazz) else "NULL", cnav))
        if len(properties) > 0:
            e("\n    pythonType.tp_getset = %s_getsets;\n" % cnav)
        if callableOp:
            e("\n    pythonType.tp_call = (ternaryfunc)%s_%s;\n" % (mod, PT.getPyMethodName(callableOp, MT.method)))
        if doAsMapping:
            e("\n    pythonType.tp_as_mapping = &%s_%s_as_mapping;\n" % (mod, cnav))
        if feat_iter:
            e("\n    pythonType.tp_iter = (getiterfunc)%s_%s_iter;\n    pythonType.tp_iternext = (iternextfunc)%s_%s_iternext;\n" % (mod, cnav, mod, cnav))
        elif feat_iterable and feat_iterable != "":
            e("\n    pythonType.tp_iter = (getiterfunc)%s_%s_iter;\n" % (mod, cnav))
        baseclass = PT.findValidBaseClass(clazz, self.module)
        e("\n\n    pythonType.tp_base = %s;\n    pythonType.tp_new = " %
          (("&(Ty%s_Type.pythonType)" % PT.getClassNameAsVariable(baseclass)) if baseclass else "NULL"))
        e("NULL" if (not H.hasDefinedConstructor(clazz) or H.hasHiddenConstructor(clazz)) else ("%s_%s_New" % (mod, cnav)))
        e(';\n    pythonType.tp_init = dummy_tp_init;\n\n    Ty%s_Type.swigType="p.%s";' % (cnav, full))
        if baseclass:
            e("\n    Ty%s_Type.parentType=&Ty%s_Type;\n" % (cnav, PT.getClassNameAsVariable(baseclass)))
        if not H.hasHiddenConstructor(clazz):
            e("\n    registerAddonClassTypeInformation(&Ty%s_Type);\n" % cnav)
        e("\n  }\n%s\n" % SEP)

    # --- directors ----------------------------------------------------------
    def doDirectors(self):
        e = self.e
        for clazz in [c for c in self.classes if c.attr("feature_director") is not None]:
            cnav = PT.getClassNameAsVariable(clazz)
            ctors = clazz.kids("constructor")
            constructor = ctors[0] if ctors else None
            e("\n%s\n  // This class is the Director for %s.\n  // It provides the \"reverse bridge\" from C++ to Python to support\n  // cross-language polymorphism.\n%s\n  class %s_Director : public Director, public %s\n  {\n    public:\n" %
              (SEP, H.findFullClassName(clazz), SEP, cnav, clazz.attr("name")))
            if constructor:
                e("\n      inline %s_Director(" % cnav)
                params = constructor.kids("parm")
                for i, param in enumerate(params):
                    e("%s %s%s " % (stp.SwigType_str(param.attr("type")), param.attr("name"), "," if i < len(params) - 1 else ""))
                e(") : %s(" % H.findFullClassName(constructor))
                for i, param in enumerate(params):
                    e(" %s%s " % (param.attr("name"), "," if i < len(params) - 1 else ""))
                e(") { } ")
            e("\n")
            for it in self.getAllVirtualMethods(clazz):
                e("\n      virtual %s %s( " % (stp.SwigType_str(H.getReturnSwigType(it)), H.callingName(it)))
                params = it.kids("parm")
                paramFormatStr = "".join("O" for _ in params)
                for i, param in enumerate(params):
                    e(" %s %s%s " % (stp.SwigType_str(param.attr("type")), param.attr("name"), "," if i < len(params) - 1 else ""))
                e(" )\n      { ")
                for param in params:
                    e("\n        PyObject* py%s = NULL;\n        %s" %
                      (param.attr("name"),
                       H.getOutConversion(param.attr("type"), "result", it,
                                          {"result": "py" + param.attr("name"), "api": param.attr("name")})))
                e('\n        XBMCAddon::Python::PyContext pyContext;\n        PyObject_CallMethod(self,"%s","(%s)"' %
                  (H.callingName(it), paramFormatStr))
                for param in params:
                    e(", py%s " % param.attr("name"))
                e(");\n        if (PyErr_Occurred())\n          throw PythonBindings::PythonToCppException();\n      }\n")
            e("\n  };\n")

    # --- top-level driver ---------------------------------------------------
    def run(self):
        e = self.e
        mod = self.mod
        module = self.module

        H.setup(self.classes, OUT_EXACT, OUT_PATTERNS, DEFAULT_OUT, IN_EXACT, IN_PATTERNS, DEFAULT_IN)

        e("\n\n/*\n *  Copyright (C) 2005-2018 Team Kodi\n *  This file is part of Kodi - https://kodi.tv\n *\n *  SPDX-License-Identifier: GPL-2.0-or-later\n *  See LICENSES/README.md for more information.\n */\n\n")
        e('// ************************************************************************\n// This file was generated by xbmc compile process. DO NOT EDIT!!\n//  It was created by running the code generator on the spec file for\n//  the module "%s" on the template file PythonSwig.template.cpp\n// ************************************************************************\n\n' % mod)
        for it in H.getInsertNodes(module, "begin"):
            e(H.unescape(it))
        e('\n\n#include <Python.h>\n#include <string>\n#include "CompileInfo.h"\n#include "interfaces/python/LanguageHook.h"\n#include "interfaces/python/swig.h"\n#include "interfaces/python/PyContext.h"\n\n')
        for it in H.getInsertNodes(module, "header"):
            e(H.unescape(it))
        e("\n\nnamespace PythonBindings\n{\n")

        for clazz in self.classes:
            self.doClassTypeInfo(clazz, self.classNameAsVariables)

        knownApiTypes = []
        seen = set()
        for it in module.depth_first():
            attr = it.attribute("feature_knownapitypes")
            if attr is not None:
                for t in attr.strip().split(","):
                    if t not in seen:
                        seen.add(t)
                        knownApiTypes.append(t)
        for t in _java_hashset_order(knownApiTypes):
            self.doExternClassTypeInfo(t)

        e("\n\n")
        self.doDirectors()

        for it in self.methods:
            if it.name() != "destructor":
                self.doMethod(it, MT.constructor if it.name() == "constructor" else MT.method)
                e("\n")
        for clazz in self.classes:
            self.doMethod(clazz, MT.destructor)

        for node in self.classes:
            for key in [k for k in node.attributes.keys() if k.startswith("feature_python_method_")]:
                methodName = key[len("feature_python_method_"):]
                e("\n  static PyObject* %s_%s_%s(PyObject* self, PyObject *args, PyObject *kwds)\n  %s\n" %
                  (mod, PT.getClassNameAsVariable(node), methodName, H.unescape(node.attribute(key))))

        for clazz in self.classes:
            self.doClassMethodInfo(clazz, self.initTypeCalls)

        e("\n\n  static PyMethodDef %s_methods[] = { " % mod)
        for it in module.depth_first():
            if it.name() == "function" and len(H.parents(it, lambda n: n.name() == "class")) == 0:
                e('\n    {"%s", (PyCFunction)%s_%s, METH_VARARGS|METH_KEYWORDS, %s }, ' %
                  (it.attr("sym_name"), mod, PT.getPyMethodName(it, MT.method),
                   (PT.getPyMethodName(it, MT.method) + "__doc__") if H.hasDoc(it) else "NULL"))
        e("\n    {NULL, NULL, 0, NULL}\n  };\n\n"
          "  // This is the call that will call all of the other initializes\n"
          "  //  for all of the classes in this module\n  static void initTypes()\n  {\n"
          "    static bool typesAlreadyInitialized = false;\n    if (!typesAlreadyInitialized)\n    {\n"
          "      typesAlreadyInitialized = true;\n")
        for it in self.initTypeCalls:
            e("\n      %s();" % it)
        for it in self.classNameAsVariables:
            e("\n      if (PyType_Ready(&(Ty%s_Type.pythonType)) < 0)\n        return;" % it)
        e("\n    }\n  }\n\n  static struct PyModuleDef createModule\n  {\n      PyModuleDef_HEAD_INIT,\n      \"%s\",\n      \"\",\n      -1,\n      %s_methods,\n      nullptr,\n      nullptr,\n      nullptr,\n      nullptr,\n  };\n\n  PyObject *PyInit_Module_%s()\n  {\n    initTypes();\n\n    // init general %s modules\n    PyObject* module;\n\n" %
          (mod, mod, mod, mod))
        for it in self.classNameAsVariables:
            e("\n    Py_INCREF(&(Ty%s_Type.pythonType));" % it)
        e("\n\n    module = PyModule_Create(&createModule);\n    if (module == NULL) return NULL;\n\n")
        for clazz in self.classes:
            e('\n    PyModule_AddObject(module, "%s", (PyObject*)(&(Ty%s_Type.pythonType)));' %
              (clazz.attr("sym_name"), PT.getClassNameAsVariable(clazz)))
        e('\n\n   // constants\n   PyModule_AddStringConstant(module, "__author__", "Team Kodi <http://kodi.tv>");\n   PyModule_AddStringConstant(module, "__date__", CCompileInfo::GetBuildDate().c_str());\n   PyModule_AddStringConstant(module, "__version__", "3.0.2");\n   PyModule_AddStringConstant(module, "__credits__", "Team Kodi");\n   PyModule_AddStringConstant(module, "__platform__", "ALL");\n\n   // need to handle constants\n   // #define constants\n')
        for it in module.depth_first():
            if it.name() == "constant":
                pyCall = ("PyModule_AddIntConstant"
                          if it.attr("type") in ("int", "long", "unsigned int", "unsigned long", "bool")
                          else "PyModule_AddStringConstant")
                e('\n   %s(module,"%s",%s); ' % (pyCall, it.attr("sym_name"), it.attr("value")))
        e("\n  // constexpr constants\n")
        for it in module.depth_first():
            if it.name() == "variable" and it.attr("storage") == "constexpr" and not it.attr("error"):
                pyCall = ("PyModule_AddIntConstant"
                          if it.attr("type") in ("q(const).int", "q(const).long", "q(const).unsigned int",
                                                  "q(const).unsigned long", "q(const).bool")
                          else "PyModule_AddStringConstant")
                e('\n   %s(module,"%s",%s); ' % (pyCall, it.attr("sym_name"), it.attr("value")))
        e("\n  return module;\n  }\n\n} // end PythonBindings namespace for python type definitions\n\n")
        for it in H.getInsertNodes(module, "footer"):
            e(H.unescape(it))
        e("\n")
        return "".join(self.o)


def generate(module):
    return Gen(module).run()
