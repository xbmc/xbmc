# Rewrites SWIG-generated wrapper code to survive concurrent sub-interpreters
# (swig/swig#3535): as generated, every interpreter re-creates the builtin
# types and repoints the process-wide clientdata, so two live
# sub-interpreters corrupt each other. Each of the three rewrites below
# creates its object once per process and reuses it.

cmake_policy(SET CMP0007 NEW)

# Patched pattern verified against these exact releases only
set(_verified 4.5.0)
list(FIND _verified "${SWIG_VERSION}" _idx)
if(_idx EQUAL -1)
  message(FATAL_ERROR "fix-swig-3535.cmake: SWIG ${SWIG_VERSION} is unverified. "
                      "Re-verify the pattern, then add the version to _verified.")
endif()

file(READ "${FILE}" _content)

string(FIND "${_content}" "_clientdata.pytype) {" _already)
if(NOT _already EQUAL -1)
  return()
endif()

# rewrite #1: create each builtin type once per process and reuse it forever,
# as the Groovy bindings' static types did; re-creating per interpreter leaves
# clientdata.pytype pointing into whichever interpreter imported last
#
# BEFORE:
#   builtin_pytype = SwigPyBuiltin__XBMCAddon__xbmcvfs__File_type_create(metatype, builtin_bases, d);
#   if(!builtin_pytype) {
#     return -1;
#   }
#   SwigPyBuiltin__XBMCAddon__xbmcvfs__File_clientdata.pytype = builtin_pytype;
#
# AFTER:
#   if (SwigPyBuiltin__XBMCAddon__xbmcvfs__File_clientdata.pytype) {
#     SWIG_Py_DECREF(d);
#     builtin_pytype = SwigPyBuiltin__XBMCAddon__xbmcvfs__File_clientdata.pytype;
#   } else {
#   builtin_pytype = SwigPyBuiltin__XBMCAddon__xbmcvfs__File_type_create(metatype, builtin_bases, d);
#   if(!builtin_pytype) {
#     return -1;
#   }
#   SwigPyBuiltin__XBMCAddon__xbmcvfs__File_clientdata.pytype = builtin_pytype;
#   SWIG_Py_INCREF((PyObject *)builtin_pytype);
#   }
string(REGEX MATCHALL "SWIG_Py_INCREF\\(\\(PyObject \\*\\)builtin_pytype\\)" _increfs "${_content}")
list(LENGTH _increfs _nclasses)
string(REGEX REPLACE
"builtin_pytype = SwigPyBuiltin__([A-Za-z0-9_]+)_type_create\\(metatype, builtin_bases, d\\);\n  if\\(!builtin_pytype\\) {\n    return -1;\n  }\n  SwigPyBuiltin__([A-Za-z0-9_]+)_clientdata\\.pytype = builtin_pytype;"
"if (SwigPyBuiltin__\\1_clientdata.pytype) {
    SWIG_Py_DECREF(d);
    builtin_pytype = SwigPyBuiltin__\\1_clientdata.pytype;
  } else {
  builtin_pytype = SwigPyBuiltin__\\1_type_create(metatype, builtin_bases, d);
  if(!builtin_pytype) {
    return -1;
  }
  SwigPyBuiltin__\\2_clientdata.pytype = builtin_pytype;
  SWIG_Py_INCREF((PyObject *)builtin_pytype);
  }"
_content "${_content}")
string(REGEX MATCHALL "SWIG_Py_INCREF\\(\\(PyObject \\*\\)builtin_pytype\\)" _increfs "${_content}")
list(LENGTH _increfs _nafter)
math(EXPR _expected "${_nclasses} * 2")
if(NOT _nafter EQUAL _expected)
  message(FATAL_ERROR "fix-swig-3535.cmake: type_create rewrite touched ${_nafter} of ${_expected} expected sites in ${FILE}")
endif()

# rewrite #2: create the SwigPyObject base type only when this process has
# none to adopt; unpatched output creates one per interpreter and discards
# all but the first
#
# BEFORE:
#   swigpyobject = SwigPyObject_TypeOnce();
#   SwigPyObject_stype = SWIG_MangledTypeQuery("_p_SwigPyObject");
#   assert(SwigPyObject_stype);
#   cd = (SwigPyClientData*) SwigPyObject_stype->clientdata;
#   if (!cd) {
#     SwigPyObject_stype->clientdata = &SwigPyObject_clientdata;
#     SwigPyObject_clientdata.pytype = swigpyobject;
#   } else if (swigpyobject->tp_basicsize != cd->pytype->tp_basicsize) {
#     PyErr_SetString(PyExc_RuntimeError, "Import error: attempted to load two incompatible swig-generated modules.");
#     return -1;
#   }
#
# AFTER:
#   SwigPyObject_stype = SWIG_MangledTypeQuery("_p_SwigPyObject");
#   assert(SwigPyObject_stype);
#   cd = (SwigPyClientData*) SwigPyObject_stype->clientdata;
#   if (!cd) {
#     swigpyobject = SwigPyObject_TypeOnce();
#     SwigPyObject_stype->clientdata = &SwigPyObject_clientdata;
#     SwigPyObject_clientdata.pytype = swigpyobject;
#   } else {
#     swigpyobject = cd->pytype;
#   }
string(FIND "${_content}" "  swigpyobject = SwigPyObject_TypeOnce();" _pos)
if(_pos EQUAL -1)
  message(FATAL_ERROR "fix-swig-3535.cmake: SwigPyObject creation line not found in ${FILE}")
endif()
string(REPLACE "  swigpyobject = SwigPyObject_TypeOnce();
" "" _content "${_content}")
set(_orig_base "  cd = (SwigPyClientData*) SwigPyObject_stype->clientdata;
  if (!cd) {
    SwigPyObject_stype->clientdata = &SwigPyObject_clientdata;
    SwigPyObject_clientdata.pytype = swigpyobject;
  } else if (swigpyobject->tp_basicsize != cd->pytype->tp_basicsize) {
    PyErr_SetString(PyExc_RuntimeError, \"Import error: attempted to load two incompatible swig-generated modules.\");
    return -1;
  }")
set(_fixed_base "  cd = (SwigPyClientData*) SwigPyObject_stype->clientdata;
  if (!cd) {
    swigpyobject = SwigPyObject_TypeOnce();
    SwigPyObject_stype->clientdata = &SwigPyObject_clientdata;
    SwigPyObject_clientdata.pytype = swigpyobject;
  } else {
    swigpyobject = cd->pytype;
  }")
string(FIND "${_content}" "${_orig_base}" _pos)
if(_pos EQUAL -1)
  message(FATAL_ERROR "fix-swig-3535.cmake: SwigPyObject adoption pattern not found in ${FILE}")
endif()
string(REPLACE "${_orig_base}" "${_fixed_base}" _content "${_content}")

# rewrite #3: create "this" and "thisown" descriptors once per process
# instead of once per interpreter
#
# BEFORE:
#   this_descr = PyDescr_NewGetSet(SwigPyObject_Type(), &this_getset_def);
#   (void)this_descr;
#
# AFTER:
#   static PyObject *this_descr_once = NULL;
#   if (!this_descr_once)
#     this_descr_once = PyDescr_NewGetSet(SwigPyObject_Type(), &this_getset_def);
#   this_descr = this_descr_once;
#   (void)this_descr;
foreach(_d this thisown)
  set(_orig_descr "  ${_d}_descr = PyDescr_NewGetSet(SwigPyObject_Type(), &${_d}_getset_def);
  (void)${_d}_descr;")
  set(_fixed_descr "  static PyObject *${_d}_descr_once = NULL;
  if (!${_d}_descr_once)
    ${_d}_descr_once = PyDescr_NewGetSet(SwigPyObject_Type(), &${_d}_getset_def);
  ${_d}_descr = ${_d}_descr_once;
  (void)${_d}_descr;")
  string(FIND "${_content}" "${_orig_descr}" _pos)
  if(_pos EQUAL -1)
    message(FATAL_ERROR "fix-swig-3535.cmake: ${_d}_descr pattern not found in ${FILE}")
  endif()
  string(REPLACE "${_orig_descr}" "${_fixed_descr}" _content "${_content}")
endforeach()

file(WRITE "${FILE}" "${_content}")
