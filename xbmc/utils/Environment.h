#pragma once
#ifndef XBMC_SETENV_H
#define XBMC_SETENV_H
/*
 *      Copyright (C) 2013 Team XBMC
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
#if defined(TARGET_WINDOWS) || defined(TARGET_WIN10)
private:
  static std::wstring win32ConvertUtf8ToW(const std::string &text, bool *resultSuccessful = NULL);
  static std::string win32ConvertWToUtf8(const std::wstring &text, bool *resultSuccessful = NULL);
  enum updateAction:int {addOrUpdateOnly = -2, deleteVariable = -1, addOnly =  0, autoDetect = 1};
  static int win32_setenv(const std::string &name, const std::string &value = "", updateAction action = autoDetect);
#endif // TARGET_WINDOWS || TARGET_WIN10
};
#endif
