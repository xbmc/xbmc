/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// python.h should always be included first before any other includes
#include "PythonToCppException.h"

#include "utils/StringUtils.h"

#include <string>

#include <Python.h>

namespace PythonBindings
{
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

PythonToCppException::PythonToCppException(const std::string& exceptionType,
                                           const std::string& exceptionValue,
                                           const std::string& exceptionTraceback)
  : XbmcCommons::UncheckedException(" ")
{
  setClassname("PythonToCppException");

  SetMessage(exceptionType, exceptionValue, exceptionTraceback);
}

bool PythonToCppException::ParsePythonException(std::string& exceptionType,
                                                std::string& exceptionValue,
                                                std::string& exceptionTraceback)
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
  if (exc_traceback != NULL)
  {
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

    PyObject* tracebackModule = PyImport_ImportModule("traceback");
    if (tracebackModule != NULL)
    {
      char method[] = "format_exception";
      char format[] = "OOO";
      PyObject* tbList = PyObject_CallMethod(tracebackModule, method, format, exc_type,
                                             exc_value == NULL ? Py_None : exc_value,
                                             exc_traceback == NULL ? Py_None : exc_traceback);

      if (tbList)
      {
        PyObject* emptyString = PyUnicode_FromString("");
        char method[] = "join";
        char format[] = "O";
        PyObject* strRetval = PyObject_CallMethod(emptyString, method, format, tbList);
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

void PythonToCppException::SetMessage(const std::string& exceptionType,
                                      const std::string& exceptionValue,
                                      const std::string& exceptionTraceback)
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
} // namespace PythonBindings
