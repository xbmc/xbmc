#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
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

#if (defined USE_EXTERNAL_PYTHON) && (!defined HAVE_LIBPYTHON2_4)
  /* Upstream Python rename Py_InitModule4 for 64-bit systems for Python
   versions higher than 2.4 */
  #if SIZEOF_SIZE_T != SIZEOF_INT
  #define Py_InitModule4 Py_InitModule4_64
  #endif
#endif
/* Allow our modules to be compatible with Python2.4.
   http://www.python.org/dev/peps/pep-0353/ */
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

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

