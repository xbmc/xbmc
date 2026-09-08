/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/Exception.h"

#include <string>

namespace PythonBindings
{
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
  PythonToCppException(const std::string& exceptionType,
                       const std::string& exceptionValue,
                       const std::string& exceptionTraceback);

  static bool ParsePythonException(std::string& exceptionType,
                                   std::string& exceptionValue,
                                   std::string& exceptionTraceback);

protected:
  void SetMessage(const std::string& exceptionType,
                  const std::string& exceptionValue,
                  const std::string& exceptionTraceback);
};
} // namespace PythonBindings
