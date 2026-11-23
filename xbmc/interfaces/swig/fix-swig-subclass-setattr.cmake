# -builtin makes SwigPyObjectType the metatype of a class defined in Python too, such as an
# add-on's xbmc.Player subclass, and its setattro refuses that class's own attributes. Only the
# wrapped types are immutable (fix-swig-3535.cmake), so a mutable type goes to type's setattro.

file(READ "${FILE}" _content)

set(_fixed_marker "PyType_Type.tp_setattro(typeobject, name, value)")
string(FIND "${_content}" "${_fixed_marker}" _already)
if(NOT _already EQUAL -1)
  return()
endif()

set(_orig "  assert(PyType_Check(typeobject));
  type = (PyTypeObject *)typeobject;
  attribute = _PyType_Lookup(type, name);
")
set(_fixed "  assert(PyType_Check(typeobject));
  type = (PyTypeObject *)typeobject;
#ifdef Py_TPFLAGS_IMMUTABLETYPE
  if (!PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE))
    return ${_fixed_marker};
#endif
  attribute = _PyType_Lookup(type, name);
")

string(FIND "${_content}" "${_orig}" _found)
if(_found EQUAL -1)
  message(FATAL_ERROR "fix-swig-subclass-setattr.cmake: SwigPyObjectType_setattro not found in ${FILE}")
endif()
string(REPLACE "${_orig}" "${_fixed}" _content "${_content}")
file(WRITE "${FILE}" "${_content}")
