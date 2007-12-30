
#pragma once

#define DATA_OBJECT(data) unsigned long pointer_##data;

#define _Py_NoneStruct (*((PyObject*)pointer__Py_NoneStruct))
#define _Py_NotImplementedStruct (*((PyObject*)pointer__Py_NotImplementedStruct))
#define _Py_TrueStruct (*((PyObject*)pointer__Py_TrueStruct))
#define _Py_ZeroStruct (*((PyObject*)pointer__Py_ZeroStruct))
#define PyString_Type (*((PyTypeObject*)pointer_PyString_Type))
#define PyList_Type (*((PyTypeObject*)pointer_PyList_Type))
#define PyLong_Type (*((PyTypeObject*)pointer_PyLong_Type))
#define PyInt_Type (*((PyTypeObject*)pointer_PyInt_Type))
#define PyUnicode_Type (*((PyTypeObject*)pointer_PyUnicode_Type))
#define PyTuple_Type (*((PyTypeObject*)pointer_PyTuple_Type))
#define PyDict_Type (*((PyTypeObject*)pointer_PyDict_Type))

#define PyExc_SystemError ((PyObject*)(*(long*)pointer_PyExc_SystemError))
#define PyExc_ValueError ((PyObject*)(*(long*)pointer_PyExc_ValueError))
#define PyExc_Exception ((PyObject*)(*(long*)pointer_PyExc_Exception))
#define PyExc_TypeError ((PyObject*)(*(long*)pointer_PyExc_TypeError))
#define PyExc_KeyboardInterrupt ((PyObject*)(*(long*)pointer_PyExc_KeyboardInterrupt))
#define PyExc_RuntimeError ((PyObject*)(*(long*)pointer_PyExc_RuntimeError))
#define PyExc_ReferenceError ((PyObject*)(*(long*)pointer_PyExc_ReferenceError))

#ifdef __cplusplus
extern "C"
{
#endif

class LibraryLoader;

  extern DATA_OBJECT(_Py_NoneStruct)
  extern DATA_OBJECT(_Py_NotImplementedStruct)
  extern DATA_OBJECT(_Py_TrueStruct)
  extern DATA_OBJECT(_Py_ZeroStruct)
  extern DATA_OBJECT(PyString_Type)
  extern DATA_OBJECT(PyList_Type)
  extern DATA_OBJECT(PyLong_Type)
  extern DATA_OBJECT(PyInt_Type)
  extern DATA_OBJECT(PyUnicode_Type)
  extern DATA_OBJECT(PyTuple_Type)
  extern DATA_OBJECT(PyDict_Type)

  extern DATA_OBJECT(PyExc_SystemError)
  extern DATA_OBJECT(PyExc_ValueError)
  extern DATA_OBJECT(PyExc_Exception)
  extern DATA_OBJECT(PyExc_TypeError)
  extern DATA_OBJECT(PyExc_KeyboardInterrupt)
  extern DATA_OBJECT(PyExc_RuntimeError)
  extern DATA_OBJECT(PyExc_ReferenceError)

  bool python_load_dll(LibraryLoader& dll);

#ifdef __cplusplus
}
#endif
