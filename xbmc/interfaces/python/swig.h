/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Exception.h"
#include "interfaces/legacy/Window.h"

#include <stdint.h>
#include <string>
#include <typeindex>

#include <Python.h>

namespace PythonBindings
{
  /**
   * This call will convert the python object passed to a string. The object
   * passed must be a python str or unicode object unless coerceToString is
   * true. If coerceToString is true then the type must be castable to a string
   * using the python call str(pObject).
   *
   * This method will handle a 'None' that's passed in. If 'None' is passed then
   * the resulting buf will contain the value of XBMCAddon::emptyString (which
   * is simply a std::string instantiated with the default constructor.
   */
  void PyXBMCGetUnicodeString(std::string& buf, PyObject* pObject, bool coerceToString = false,
                              const char* pos = "unknown",
                              const char* methodname = "unknown");

  struct TypeInfo
  {
    const char* swigType;
    TypeInfo* parentType;
    PyTypeObject pythonType;
    const std::type_index typeIndex;

    explicit TypeInfo(const std::type_info& ti);
  };

  // This will hold the pointer to the api type, whether known or unknown
  struct PyHolder
  {
    PyObject_HEAD
    int32_t magicNumber;
    const TypeInfo* typeInfo;
    XBMCAddon::AddonClass* pSelf;
  };

#define XBMC_PYTHON_TYPE_MAGIC_NUMBER 0x58626D63

  /**
   * This method retrieves the pointer from the PyHolder. The return value should
   * be cast to the appropriate type.
   *
   * Since the calls to this are generated there's no NULL pointer checks
   */
  inline XBMCAddon::AddonClass* retrieveApiInstance(PyObject* pythonObj, const TypeInfo* typeToCheck,
                                   const char* methodNameForErrorString,
                                   const char* typenameForErrorString)
  {
    if (pythonObj == NULL || pythonObj == Py_None)
      return NULL;
    if (reinterpret_cast<PyHolder*>(pythonObj)->magicNumber != XBMC_PYTHON_TYPE_MAGIC_NUMBER || !PyObject_TypeCheck(pythonObj, const_cast<PyTypeObject*>((&(typeToCheck->pythonType)))))
      throw XBMCAddon::WrongTypeException("Incorrect type passed to \"%s\", was expecting a \"%s\".",methodNameForErrorString,typenameForErrorString);
    return reinterpret_cast<PyHolder*>(pythonObj)->pSelf;
  }

  bool isParameterRightType(const char* passedType, const char* expectedType, const char* methodNamespacePrefix, bool tryReverse = true);

  XBMCAddon::AddonClass* doretrieveApiInstance(const PyHolder* pythonObj, const TypeInfo* typeInfo, const char* expectedType,
                              const char* methodNamespacePrefix, const char* methodNameForErrorString);

  /**
   * This method retrieves the pointer from the PyHolder. The return value should
   * be cast to the appropriate type.
   *
   * Since the calls to this are generated there's no NULL pointer checks
   *
   * This method will return NULL if either the pythonObj is NULL or the
   * pythonObj is Py_None.
   */
  inline XBMCAddon::AddonClass* retrieveApiInstance(const PyObject* pythonObj, const char* expectedType, const char* methodNamespacePrefix,
                                   const char* methodNameForErrorString)
  {
    return (pythonObj == NULL || pythonObj == Py_None) ? NULL :
      doretrieveApiInstance(reinterpret_cast<const PyHolder*>(pythonObj),reinterpret_cast<const PyHolder*>(pythonObj)->typeInfo, expectedType, methodNamespacePrefix, methodNameForErrorString);
  }

  /**
   * This method is a helper for the generated API. It's called prior to any API
   * class constructor being returned from the generated code to Python
   */
  void prepareForReturn(XBMCAddon::AddonClass* c);

  /**
   * This method is a helper for the generated API. It's called prior to any API
   * class destructor being dealloc-ed from the generated code from Python
   */
  void cleanForDealloc(XBMCAddon::AddonClass* c);

  /**
   * This method is a helper for the generated API. It's called prior to any API
   * class destructor being dealloc-ed from the generated code from Python
   *
   * There is a Catch-22 in the destruction of a Window. 'dispose' needs to be
   * called on destruction but cannot be called from the destructor.
   * This overrides the default cleanForDealloc to resolve that.
   */
  void cleanForDealloc(XBMCAddon::xbmcgui::Window* c);

  /**
   * This method allows for conversion of the native api Type to the Python type.
   *
   * When this form of the call is used (and pythonType isn't NULL) then the
   * passed type is used in the instance. This is for classes that extend API
   * classes in python. The type passed may not be the same type that's stored
   * in the class metadata of the AddonClass of which 'api' is an instance,
   * it can be a subclass in python.
   *
   * if pythonType is NULL then the type is inferred using the class metadata
   * stored in the AddonClass instance 'api'.
   */
  PyObject* makePythonInstance(XBMCAddon::AddonClass* api, PyTypeObject* pythonType, bool incrementRefCount);

  /**
   * This method allows for conversion of the native api Type to the Python type.
   *
   * When this form of the call is used then the python type constructed will be the
   * type given by the class metadata in the AddonClass instance 'api'.
   *
   * This is just a helper inline to call the other makePythonInstance with NULL as
   * the pythonType.
   */
  inline PyObject* makePythonInstance(XBMCAddon::AddonClass* api, bool incrementRefCount)
  {
    return makePythonInstance(api,NULL,incrementRefCount);
  }

  void registerAddonClassTypeInformation(const TypeInfo* classInfo);
  const TypeInfo* getTypeInfoForInstance(XBMCAddon::AddonClass* obj);

  int dummy_tp_init(PyObject* self, PyObject* args, PyObject* kwds);

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
    PythonToCppException(const std::string &exceptionType, const std::string &exceptionValue, const std::string &exceptionTraceback);

    static bool ParsePythonException(std::string &exceptionType, std::string &exceptionValue, std::string &exceptionTraceback);

  protected:
    void SetMessage(const std::string &exceptionType, const std::string &exceptionValue, const std::string &exceptionTraceback);
  };

  template<class T> struct PythonCompare
  {
    static inline int compare(PyObject* obj1, PyObject* obj2, const char* swigType, const char* methodNamespacePrefix, const char* methodNameForErrorString)
    {
      XBMC_TRACE;
      try
      {
        T* o1 = (T*)retrieveApiInstance(obj1, swigType, methodNamespacePrefix, methodNameForErrorString);
        T* o2 = (T*)retrieveApiInstance(obj2, swigType, methodNamespacePrefix, methodNameForErrorString);

        return ((*o1) < (*o2) ? -1 :
                ((*o1) > (*o2) ? 1 : 0));
      }
      catch (const XBMCAddon::WrongTypeException& e)
      {
        CLog::Log(LOGERROR, "EXCEPTION: {}", e.GetExMessage());
        PyErr_SetString(PyExc_RuntimeError, e.GetExMessage());
      }
      return -1;
    }
  };
}
