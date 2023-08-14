/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "swig.h"

#include "LanguageHook.h"
#include "interfaces/legacy/AddonString.h"
#include "utils/StringUtils.h"

#include <string>

namespace PythonBindings
{
  TypeInfo::TypeInfo(const std::type_info& ti) : swigType(NULL), parentType(NULL), typeIndex(ti)
  {
    static PyTypeObject py_type_object_header = {
      PyVarObject_HEAD_INIT(nullptr, 0) 0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
#if PY_VERSION_HEX > 0x03080000
      0,
      0,
#endif
#if PY_VERSION_HEX < 0x03090000
      0,
#endif
#if PY_VERSION_HEX >= 0x030C00A1
      0,
#endif
    };

    static int size = (long*)&(py_type_object_header.tp_name) - (long*)&py_type_object_header;
    memcpy(&(this->pythonType), &py_type_object_header, size);
  }

  class PyObjectDecrementor
  {
    PyObject* obj;
  public:
    inline explicit PyObjectDecrementor(PyObject* pyobj) : obj(pyobj) {}
    inline ~PyObjectDecrementor() { Py_XDECREF(obj); }

    inline PyObject* get() { return obj; }
  };

  void PyXBMCGetUnicodeString(std::string& buf, PyObject* pObject, bool coerceToString,
                              const char* argumentName, const char* methodname)
  {
    // It's okay for a string to be "None". In this case the buf returned
    // will be the emptyString.
    if (pObject == Py_None)
    {
      buf = XBMCAddon::emptyString;
      return;
    }

    //! @todo UTF-8: Does python use UTF-16?
    //!              Do we need to convert from the string charset to UTF-8
    //!              for non-unicode data?
    if (PyUnicode_Check(pObject))
    {
      // Python unicode objects are UCS2 or UCS4 depending on compilation
      // options, wchar_t is 16-bit or 32-bit depending on platform.
      // Avoid the complexity by just letting python convert the string.

      buf = PyUnicode_AsUTF8(pObject);
      return;
    }

    if (PyBytes_Check(pObject)) // If pobject is of type Bytes
    {
      buf = PyBytes_AsString(pObject);
      return;
    }

    // if we got here then we need to coerce the value to a string
    if (coerceToString)
    {
      PyObjectDecrementor dec(PyObject_Str(pObject));
      PyObject* pyStrCast = dec.get();
      if (pyStrCast)
      {
        PyXBMCGetUnicodeString(buf,pyStrCast,false,argumentName,methodname);
        return;
      }
    }

    // Object is not a unicode or a normal string.
    buf = "";
    throw XBMCAddon::WrongTypeException("argument \"%s\" for method \"%s\" must be unicode or str", argumentName, methodname);
  }

  // need to compare the typestring
  bool isParameterRightType(const char* passedType, const char* expectedType, const char* methodNamespacePrefix, bool tryReverse)
  {
    if (strcmp(expectedType,passedType) == 0)
      return true;

    // well now things are a bit more complicated. We need to see if the passed type
    // is a subset of the overall type
    std::string et(expectedType);
    bool isPointer = (et[0] == 'p' && et[1] == '.');
    std::string baseType(et,(isPointer ? 2 : 0)); // this may contain a namespace

    std::string ns(methodNamespacePrefix);
    // cut off trailing '::'
    if (ns.size() > 2 && ns[ns.size() - 1] == ':' && ns[ns.size() - 2] == ':')
      ns.resize(ns.size() - 2);

    bool done = false;
    while(! done)
    {
      done = true;

      // now we need to see if the expected type can be munged
      //  into the passed type by tacking on the namespace of
      //  of the method.
      std::string check(isPointer ? "p." : "");
      check += ns;
      check += "::";
      check += baseType;

      if (strcmp(check.c_str(),passedType) == 0)
        return true;

      // see if the namespace is nested.
      int posOfScopeOp = ns.find("::");
      if (posOfScopeOp >= 0)
      {
        done = false;
        // cur off the outermost namespace
        ns = ns.substr(posOfScopeOp + 2);
      }
    }

    // so far we applied the namespace to the expected type. Now lets try
    //  the reverse if we haven't already.
    if (tryReverse)
      return isParameterRightType(expectedType, passedType, methodNamespacePrefix, false);

    return false;
  }

  PythonToCppException::PythonToCppException() : XbmcCommons::UncheckedException(" ")
  {
    setClassname("PythonToCppException");

    std::string msg;
    std::string type, value, traceback;
    if (!ParsePythonException(type, value, traceback))
      UncheckedException::SetMessage("Strange: No Python exception occurred");
    else
      SetMessage(type, value, traceback);
  }

  PythonToCppException::PythonToCppException(const std::string &exceptionType, const std::string &exceptionValue, const std::string &exceptionTraceback) : XbmcCommons::UncheckedException(" ")
  {
    setClassname("PythonToCppException");

    SetMessage(exceptionType, exceptionValue, exceptionTraceback);
  }

  bool PythonToCppException::ParsePythonException(std::string &exceptionType, std::string &exceptionValue, std::string &exceptionTraceback)
  {
    PyObject* exc_type;
    PyObject* exc_value;
    PyObject* exc_traceback;
    PyObject* pystring = NULL;

    PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
    if (exc_type == NULL && exc_value == NULL && exc_traceback == NULL)
      return false;

    // See https://docs.python.org/3/c-api/exceptions.html#c.PyErr_NormalizeException
    PyErr_NormalizeException(&exc_type, &exc_value, &exc_traceback);
    if (exc_traceback != NULL) {
      PyException_SetTraceback(exc_value, exc_traceback);
    }

    exceptionType.clear();
    exceptionValue.clear();
    exceptionTraceback.clear();

    if (exc_type != NULL && (pystring = PyObject_Str(exc_type)) != NULL && PyUnicode_Check(pystring))
    {
      const char* str = PyUnicode_AsUTF8(pystring);
      if (str != NULL)
        exceptionType = str;

      pystring = PyObject_Str(exc_value);
      if (pystring != NULL)
      {
        str = PyUnicode_AsUTF8(pystring);
        exceptionValue = str;
      }

      PyObject *tracebackModule = PyImport_ImportModule("traceback");
      if (tracebackModule != NULL)
      {
        char method[] = "format_exception";
        char format[] = "OOO";
        PyObject *tbList = PyObject_CallMethod(tracebackModule, method, format, exc_type, exc_value == NULL ? Py_None : exc_value, exc_traceback == NULL ? Py_None : exc_traceback);

        if (tbList)
        {
          PyObject* emptyString = PyUnicode_FromString("");
          char method[] = "join";
          char format[] = "O";
          PyObject *strRetval = PyObject_CallMethod(emptyString, method, format, tbList);
          Py_DECREF(emptyString);

          if (strRetval)
          {
            str = PyUnicode_AsUTF8(strRetval);
            if (str != NULL)
              exceptionTraceback = str;
            Py_DECREF(strRetval);
          }
          Py_DECREF(tbList);
        }
        Py_DECREF(tracebackModule);

      }
    }

    Py_XDECREF(exc_type);
    Py_XDECREF(exc_value);
    Py_XDECREF(exc_traceback);
    Py_XDECREF(pystring);

    return true;
  }

  void PythonToCppException::SetMessage(const std::string &exceptionType, const std::string &exceptionValue, const std::string &exceptionTraceback)
  {
    std::string msg = "-->Python callback/script returned the following error<--\n";
    msg += " - NOTE: IGNORING THIS CAN LEAD TO MEMORY LEAKS!\n";

    if (!exceptionType.empty())
    {
      msg += StringUtils::Format("Error Type: {}\n", exceptionType);

      if (!exceptionValue.empty())
        msg += StringUtils::Format("Error Contents: {}\n", exceptionValue);

      if (!exceptionTraceback.empty())
        msg += exceptionTraceback;

      msg += "-->End of Python script error report<--\n";
    }
    else
      msg += "<unknown exception type>";

    UncheckedException::SetMessage("%s", msg.c_str());
  }

  XBMCAddon::AddonClass* doretrieveApiInstance(const PyHolder* pythonObj, const TypeInfo* typeInfo, const char* expectedType,
                              const char* methodNamespacePrefix, const char* methodNameForErrorString)
  {
    if (pythonObj->magicNumber != XBMC_PYTHON_TYPE_MAGIC_NUMBER)
      throw XBMCAddon::WrongTypeException("Non api type passed to \"%s\" in place of the expected type \"%s.\"",
                                          methodNameForErrorString, expectedType);
    if (!isParameterRightType(typeInfo->swigType,expectedType,methodNamespacePrefix))
    {
      // maybe it's a child class
      if (typeInfo->parentType)
        return doretrieveApiInstance(pythonObj, typeInfo->parentType,expectedType,
                                     methodNamespacePrefix, methodNameForErrorString);
      else
        throw XBMCAddon::WrongTypeException("Incorrect type passed to \"%s\", was expecting a \"%s\" but received a \"%s\"",
                                 methodNameForErrorString,expectedType,typeInfo->swigType);
    }
    return const_cast<XBMCAddon::AddonClass*>(pythonObj->pSelf);
  }

  /**
   * This method is a helper for the generated API. It's called prior to any API
   * class constructor being returned from the generated code to Python
   */
  void prepareForReturn(XBMCAddon::AddonClass* c)
  {
    XBMC_TRACE;
    if(c) {
      c->Acquire();
      PyThreadState* state = PyThreadState_Get();
      XBMCAddon::Python::PythonLanguageHook::GetIfExists(state->interp)->RegisterAddonClassInstance(c);
    }
  }

  static bool handleInterpRegistrationForClean(XBMCAddon::AddonClass* c)
  {
    XBMC_TRACE;
    if(c){
      XBMCAddon::AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> lh =
        XBMCAddon::AddonClass::Ref<XBMCAddon::AddonClass>(c->GetLanguageHook());

      if (lh.isNotNull())
      {
        lh->UnregisterAddonClassInstance(c);
        return true;
      }
      else
      {
        PyThreadState* state = PyThreadState_Get();
        lh = XBMCAddon::Python::PythonLanguageHook::GetIfExists(state->interp);
        if (lh.isNotNull()) lh->UnregisterAddonClassInstance(c);
        return true;
      }
    }
    return false;
  }

  /**
   * This method is a helper for the generated API. It's called prior to any API
   * class destructor being dealloc-ed from the generated code from Python
   */
  void cleanForDealloc(XBMCAddon::AddonClass* c)
  {
    XBMC_TRACE;
    if (handleInterpRegistrationForClean(c))
      c->Release();
  }

  /**
   * This method is a helper for the generated API. It's called prior to any API
   * class destructor being dealloc-ed from the generated code from Python
   *
   * There is a Catch-22 in the destruction of a Window. 'dispose' needs to be
   * called on destruction but cannot be called from the destructor.
   * This overrides the default cleanForDealloc to resolve that.
   */
  void cleanForDealloc(XBMCAddon::xbmcgui::Window* c)
  {
    XBMC_TRACE;
    if (handleInterpRegistrationForClean(c))
    {
      c->dispose();
      c->Release();
    }
  }

  /**
   * This method allows for conversion of the native api Type to the Python type.
   *
   * When this form of the call is used (and pytype isn't NULL) then the
   * passed type is used in the instance. This is for classes that extend API
   * classes in python. The type passed may not be the same type that's stored
   * in the class metadata of the AddonClass of which 'api' is an instance,
   * it can be a subclass in python.
   *
   * if pytype is NULL then the type is inferred using the class metadata
   * stored in the AddonClass instance 'api'.
   */
  PyObject* makePythonInstance(XBMCAddon::AddonClass* api, PyTypeObject* pytype, bool incrementRefCount)
  {
    // null api types result in Py_None
    if (!api)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

    // retrieve the TypeInfo from the api class
    const TypeInfo* typeInfo = getTypeInfoForInstance(api);
    PyTypeObject* typeObj = pytype == NULL ? const_cast<PyTypeObject*>(&(typeInfo->pythonType)) : pytype;

    PyHolder* self = reinterpret_cast<PyHolder*>(typeObj->tp_alloc(typeObj,0));
    if (!self) return NULL;
    self->magicNumber = XBMC_PYTHON_TYPE_MAGIC_NUMBER;
    self->typeInfo = typeInfo;
    self->pSelf = api;
    if (incrementRefCount)
      Py_INCREF((PyObject*)self);
    return (PyObject*)self;
  }

  std::map<std::type_index, const TypeInfo*> typeInfoLookup;

  void registerAddonClassTypeInformation(const TypeInfo* classInfo)
  {
    typeInfoLookup[classInfo->typeIndex] = classInfo;
  }

  const TypeInfo* getTypeInfoForInstance(XBMCAddon::AddonClass* obj)
  {
    std::type_index ti(typeid(*obj));
    return typeInfoLookup[ti];
  }

  int dummy_tp_init(PyObject* self, PyObject* args, PyObject* kwds)
  {
    return 0;
  }
}

