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

#include "interfaces/python/swig.h"
#include <string>

namespace PythonBindings
{
  void PyXBMCInitializeTypeObject(PyTypeObject* type_object)
  {
    static PyTypeObject py_type_object_header = { PyObject_HEAD_INIT(NULL) 0};
    int size = (long*)&(py_type_object_header.tp_name) - (long*)&py_type_object_header;
    memset(type_object, 0, sizeof(PyTypeObject));
    memcpy(type_object, &py_type_object_header, size);
  }

  int PyXBMCGetUnicodeString(std::string& buf, PyObject* pObject, bool coerceToString,
                             const char* argumentName, const char* methodname)
  {
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
        return 1;
      }
    }
    if (PyString_Check(pObject))
    {
      buf = PyString_AsString(pObject);
      return 1;
    }

    // if we got here then we need to coerce the value to a string
    if (coerceToString)
    {
      PyObject* pyStrCast = PyObject_Str(pObject);
      if (pyStrCast)
      {
        int ret = PyXBMCGetUnicodeString(buf,pyStrCast,false,argumentName,methodname);
        Py_DECREF(pyStrCast);
        return ret;
      }
    }

    // Object is not a unicode or a normal string.
    buf = "";
    PyErr_Format(PyExc_TypeError, "argument \"%s\" for method \"%s\" must be unicode or str", argumentName, methodname);
    return 0;
  }

  // need to compare the typestring
  bool isParameterRightType(const char* passedType, const char* expectedType, const char* methodNamespacePrefix)
  {
    if (strcmp(expectedType,passedType) == 0)
      return true;

    // well now things are a bit more complicated. We need to see if the passed type
    // is a subset of the overall type
    std::string pt(passedType);
    bool isPointer = (pt[0] == 'p' && pt[1] == '.');
    std::string baseType(pt,(isPointer ? 2 : 0));
    std::string ns(methodNamespacePrefix);

    // cut off trailing '::'
    if (ns.size() > 2 && ns[ns.size() - 1] == ':' && ns[ns.size() - 2] == ':')
      ns = ns.substr(0,ns.size()-2);
    
    bool done = false;
    while(! done)
    {
      done = true;
      std::string check(isPointer ? "p." : "");
      check += ns;
      check += "::";
      check += baseType;

      if (strcmp(expectedType,check.c_str()) == 0)
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

    return false;
  }

  PythonToCppException::PythonToCppException() : XbmcCommons::UncheckedException("")
  {
    setClassname("PythonToCppException");

    PyObject* exc_type;
    PyObject* exc_value;
    PyObject* exc_traceback;
    PyObject* pystring = NULL;

    CStdString msg;

    PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
    if (exc_type == 0 && exc_value == 0 && exc_traceback == 0)
    {
      msg = "Strange: No Python exception occured";
    }
    else
    {
      msg = "-->Python callback/script returned the following error<--\n";
      msg += " - NOTE: IGNORING THIS CAN LEAD TO MEMORY LEAKS!\n";
      if (exc_type != NULL && (pystring = PyObject_Str(exc_type)) != NULL && (PyString_Check(pystring)))
      {
          PyObject *tracebackModule;

          msg.AppendFormat("Error Type: %s\n", PyString_AsString(pystring));
          if (PyObject_Str(exc_value))
            msg.AppendFormat("Error Contents: %s\n", PyString_AsString(PyObject_Str(exc_value)));

          tracebackModule = PyImport_ImportModule((char*)"traceback");
          if (tracebackModule != NULL)
          {
            PyObject *tbList, *emptyString, *strRetval;

            tbList = PyObject_CallMethod(tracebackModule, (char*)"format_exception", (char*)"OOO", exc_type, exc_value == NULL ? Py_None : exc_value, exc_traceback == NULL ? Py_None : exc_traceback);
            emptyString = PyString_FromString("");
            strRetval = PyObject_CallMethod(emptyString, (char*)"join", (char*)"O", tbList);
            
            msg.Format("%s%s", msg.c_str(),PyString_AsString(strRetval));

            Py_DECREF(tbList);
            Py_DECREF(emptyString);
            Py_DECREF(strRetval);
            Py_DECREF(tracebackModule);
          }
          msg += "-->End of Python script error report<--\n";
      }
      else
      {
        pystring = NULL;
        msg += "<unknown exception type>";
      }
    }

    SetMessage("%s",msg.c_str());
  }
}

