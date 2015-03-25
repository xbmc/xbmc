/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "LanguageHook.h"
#include "swig.h"
#include "utils/StringUtils.h"
#include "interfaces/legacy/AddonString.h"

#include <string>

namespace PythonBindings
{
  TypeInfo::TypeInfo(const std::type_info& ti) : swigType(NULL), parentType(NULL), typeIndex(ti)
  {
    static PyTypeObject py_type_object_header = { PyObject_HEAD_INIT(NULL) 0};
    static int size = (long*)&(py_type_object_header.tp_name) - (long*)&py_type_object_header;
    memcpy(&(this->pythonType), &py_type_object_header, size);
  }

  class PyObjectDecrementor
  {
    PyObject* obj;
  public:
    inline PyObjectDecrementor(PyObject* pyobj) : obj(pyobj) {}
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

    // TODO: UTF-8: Does python use UTF-16?
    //              Do we need to convert from the string charset to UTF-8
    //              for non-unicode data?
    if (PyUnicode_Check(pObject))
    {
      // Python unicode objects are UCS2 or UCS4 depending on compilation
      // options, wchar_t is 16-bit or 32-bit depending on platform.
      // Avoid the complexity by just letting python convert the string.
      PyObject *utf8_pyString = PyUnicode_AsUTF8String(pObject);

      if (utf8_pyString)
      {
        buf = PyString_AsString(utf8_pyString);
        Py_DECREF(utf8_pyString);
        return;
      }
    }
    if (PyString_Check(pObject))
    {
      buf = PyString_AsString(pObject);
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
      ns = ns.substr(0,ns.size()-2);

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
      UncheckedException::SetMessage("Strange: No Python exception occured");
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

    exceptionType.clear();
    exceptionValue.clear();
    exceptionTraceback.clear();

    if (exc_type != NULL && (pystring = PyObject_Str(exc_type)) != NULL && PyString_Check(pystring))
    {
      char *str = PyString_AsString(pystring);
      if (str != NULL)
        exceptionType = str;

      pystring = PyObject_Str(exc_value);
      if (pystring != NULL)
      {
        str = PyString_AsString(pystring);
        exceptionValue = str;
      }

      PyObject *tracebackModule = PyImport_ImportModule("traceback");
      if (tracebackModule != NULL)
      {
        PyObject *tbList = PyObject_CallMethod(tracebackModule, "format_exception", "OOO", exc_type, exc_value == NULL ? Py_None : exc_value, exc_traceback == NULL ? Py_None : exc_traceback);
        PyObject *emptyString = PyString_FromString("");
        PyObject *strRetval = PyObject_CallMethod(emptyString, "join", "O", tbList);

        str = PyString_AsString(strRetval);
        if (str != NULL)
          exceptionTraceback = str;

        Py_DECREF(tbList);
        Py_DECREF(emptyString);
        Py_DECREF(strRetval);
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
      msg += StringUtils::Format("Error Type: %s\n", exceptionType.c_str());

      if (!exceptionValue.empty())
        msg += StringUtils::Format("Error Contents: %s\n", exceptionValue.c_str());

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
    return ((PyHolder*)pythonObj)->pSelf;
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
    PyTypeObject* typeObj = pytype == NULL ? (PyTypeObject*)(&(typeInfo->pythonType)) : pytype;

    PyHolder* self = (PyHolder*)typeObj->tp_alloc(typeObj,0);
    if (!self) return NULL;
    self->magicNumber = XBMC_PYTHON_TYPE_MAGIC_NUMBER;
    self->typeInfo = typeInfo;
    self->pSelf = api;
    if (incrementRefCount)
      Py_INCREF((PyObject*)self);
    return (PyObject*)self;
  }

  std::map<XbmcCommons::type_index, const TypeInfo*> typeInfoLookup;

  void registerAddonClassTypeInformation(const TypeInfo* classInfo)
  {
    typeInfoLookup[classInfo->typeIndex] = classInfo;
  }

  const TypeInfo* getTypeInfoForInstance(XBMCAddon::AddonClass* obj)
  {
    XbmcCommons::type_index ti(typeid(*obj));
    return typeInfoLookup[ti];
  }

}

