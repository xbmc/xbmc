/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include <Python.h>
#include <string>
#include <stdint.h>

#include "utils/StdString.h"
#include "interfaces/legacy/Exception.h"
#include "threads/ThreadLocal.h"

namespace PythonBindings
{
  void PyXBMCInitializeTypeObject(PyTypeObject* type_object);
  int PyXBMCGetUnicodeString(std::string& buf, PyObject* pObject, bool coerceToString = false,
                             const char* pos = "unknown", 
                             const char* methodname = "unknown");

  // This will hold the pointer to the api type, whether known or unknown
  struct PyHolder
  { 
    PyObject_HEAD
    int32_t magicNumber;
    const char* swigType;
    void* pSelf;
  };

  XBMCCOMMONS_STANDARD_EXCEPTION(WrongTypeException);

#define XBMC_PYTHON_TYPE_MAGIC_NUMBER 0x58626D63

  /**
   * This method retrieves the pointer from the PyHolder. The return value should
   * be case to the appropriate type.
   *
   * Since the calls to this are generated there's no NULL pointer checks
   */
  inline void* retrieveApiInstance(PyObject* pythonType, PyTypeObject* typeToCheck, 
                                   const char* methodNameForErrorString, 
                                   const char* typenameForErrorString) throw (WrongTypeException)
  {
    if (pythonType == NULL || ((PyHolder*)pythonType)->magicNumber != XBMC_PYTHON_TYPE_MAGIC_NUMBER)
      return NULL;
    if (!PyObject_TypeCheck(pythonType, typeToCheck))
      throw WrongTypeException("Incorrect type passed to \"%s\", was expecting a \"%s\".",methodNameForErrorString,typenameForErrorString);
    return ((PyHolder*)pythonType)->pSelf;
  }

  bool isParameterRightType(const char* passedType, const char* expectedType, const char* methodNamespacePrefix);

  /**
   * This method retrieves the pointer from the PyHolder. The return value should
   * be case to the appropriate type.
   *
   * Since the calls to this are generated there's no NULL pointer checks
   */
  inline void* retrieveApiInstance(PyObject* pythonType, const char* swigType, const char* methodNamespacePrefix,
                                   const char* methodNameForErrorString) throw (WrongTypeException)
  {
    if (pythonType == NULL || ((PyHolder*)pythonType)->magicNumber != XBMC_PYTHON_TYPE_MAGIC_NUMBER)
      throw WrongTypeException("Non api type passed in place of the expected type \"%s.\"",swigType);
    if (!isParameterRightType(((PyHolder*)pythonType)->swigType,swigType,methodNamespacePrefix))
      throw WrongTypeException("Incorrect type passed to \"%s\", was expecting a \"%s\" but received a \"%s\"",
                               methodNameForErrorString,swigType,((PyHolder*)pythonType)->swigType);
    return ((PyHolder*)pythonType)->pSelf;
  }

  /**
   * This method allows for conversion of the native api Type to the Python type
   *
   * NOTE: swigTypeString must be in the data segment. That is, it should be an explicit string since
   * the const char* is stored in a PyHolder struct and never deleted.
   */
  inline PyObject* makePythonInstance(void* api, PyTypeObject* typeObj, const char* swigTypeString)
  {
    PyHolder* self = (PyHolder*)typeObj->tp_alloc(typeObj,0);
    if (!self) return NULL;
    self->magicNumber = XBMC_PYTHON_TYPE_MAGIC_NUMBER;
    self->swigType = swigTypeString;
    self->pSelf = api;
    return (PyObject*)self;
  }

  class Director
  {
  protected:
    PyObject* self;
  public:
    inline Director() : self(NULL) {}
    inline void setPyObjectForDirector(PyObject* pyargself) { self = pyargself; }
  };

  /**
   * This exception is thrown from Director calls that call into python when the 
   * Python error is 
   */
  class PythonToCppException : public XbmcCommons::UncheckedException
  {
  public:
    /**
     * Assuming a PyErr_Occurred, this will fill the exception message with all
     *  of the appropriate information including the traceback if it can be
     *  obtained. It will also clear the python message.
     */
    PythonToCppException();
  };
}
