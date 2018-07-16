/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
 * \file utils\Environment.h
 * \brief Declares CEnvironment class for platform-independent environment variables manipulations.
 *
 */
#include <string>

/**
 * @class   CEnvironment
 *
 * @brief   Platform-independent environment variables manipulations.
 *
 * 	Provide analog for POSIX functions:
 * 	+ setenv
 * 	+ unsetenv
 * 	+ putenv
 * 	+ getenv
 *
 * 	You can generally use the functions as you would normally in POSIX-style.
 *  The differences below are just to make things more convenient through use of std::string (2,3),
 *  and to also allow the Win32-style of unsetting variables (4,5) if wanted.
 * 	1. CEnvironment::setenv  parameter 'overwrite' is optional, set by default to 1 (allow overwrite).
 * 	2. CEnvironment::putenv  uses copy of provided string (rather than string itself) to change environment,
 *                           so you can free parameter variable right after call of function.
 * 	3. CEnvironment::getenv  returns a copy of environment variable value instead of pointer to value.
 * 	4. CEnvironment::setenv  can be used to unset variables. Just pass empty string for 'value' parameter.
 * 	5. CEnvironment::putenv  can be used to unset variables. Set parameter to 'var=' (Windows style) or
 *                           just 'var' (POSIX style), and 'var' will be unset.
 *
 * 	All 'std::string' types are supposed to be in UTF-8 encoding.
 * 	All functions work on all platforms. Special care is taken on Windows platform where Environment is changed for process itself,
 * 	for process runtime library and for all runtime libraries (MSVCRT) loaded by third-party modules.
 * 	Functions internally make all necessary UTF-8 <-> wide conversions.*
 */

class CEnvironment
{
public:
  /**
   * \fn static int CEnvironment::setenv(const std::string &name, const std::string &value,
   *     int overwrite = 1);
   * \brief Sets or unsets environment variable.
   * \param name      The environment variable name to add/modify/delete.
   * \param value     The environment variable new value. If set to empty string, variable will be
   * 					deleted from the environment.
   * \param overwrite (optional) If set to non-zero, existing variable will be overwritten. If set to zero and
   * 					variable is already present, then variable will be unchanged and function returns success.
   * \return Zero on success, non-zero on error.
   */
  static int setenv(const std::string &name, const std::string &value, int overwrite = 1);
  /**
   * \fn static int CEnvironment::unsetenv(const std::string &name);
   * \brief Deletes environment variable.
   * \param name The environment variable name to delete.
   * \return Zero on success, non-zero on error.
   */
  static int unsetenv(const std::string &name);

  /**
   * \fn static int CEnvironment::putenv(const std::string &envstring);
   * \brief Adds/modifies/deletes environment variable.
   * \param envstring The variable-value string in form 'var=value'. If set to 'var=' or 'var', then variable
   * 					will be deleted from the environment.
   * \return Zero on success, non-zero on error.
   */
  static int putenv(const std::string &envstring);
  /**
   * \fn static std::string CEnvironment::getenv(const std::string &name);
   * \brief Gets value of environment variable in UTF-8 encoding.
   * \param name The name of environment variable.
   * \return Copy of of environment variable value or empty string if variable in not present in environment.
   * \sa xbmc_getenvUtf8, xbmc_getenvW
   */
  static std::string getenv(const std::string &name);
private:
#ifdef TARGET_WINDOWS
  enum updateAction:int
  {
    addOrUpdateOnly = -2,
    deleteVariable = -1,
    addOnly =  0,
    autoDetect = 1
  };
  static int win_setenv(const std::string &name, const std::string &value = "", updateAction action = autoDetect);
  static std::string win_getenv(const std::string &name);
#endif // TARGET_WINDOWS
};

