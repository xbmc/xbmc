/*
 *      Copyright (C) 2018 Team XBMC
 *      http://kodi.tv
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
 * \file platfrom\win10\Environment.cpp
 * \brief Implements CEnvironment WinRT specified class functions.
 */

#include "platform/Environment.h"
#include "platform/win32/CharsetConverter.h"

// --------------------- Internal Function ---------------------

/**
 * \fn int CEnvironment::win_setenv(const std::wstring &name, const std::wstring &value = L"",
 *     updateAction action = autoDetect)
 * \brief Internal function used to manipulate environment variables on WinRT.
 *
 * This function make all dirty work with setting, deleting and modifying environment variables.
 *
 * \param name   The environment variable name.
 * \param value  (optional) the new value of environment variable.
 * \param action (optional) the action.
 * \return Zero on success and -1 otherwise
 */
int CEnvironment::win_setenv(const std::string &name, const std::string &value /* = "" */, enum updateAction action /* = autoDetect */)
{
  std::wstring Wname = KODI::PLATFORM::WINDOWS::ToW(name);
  if (Wname.empty() || name.find('=') != std::wstring::npos)
    return -1;
  if ((action == addOnly || action == addOrUpdateOnly) && value.empty())
    return -1;
  if (action == addOnly && !getenv(name).empty())
    return 0;

  std::wstring Wvalue = KODI::PLATFORM::WINDOWS::ToW(value);
  int retValue = 0;

  // Update process Environment used for current process and for future new child processes
  if (action == deleteVariable || value.empty())
    retValue += SetEnvironmentVariableW(Wname.c_str(), nullptr) ? 0 : 4; // 4 if failed
  else
    retValue += SetEnvironmentVariableW(Wname.c_str(), Wvalue.c_str()) ? 0 : 4; // 4 if failed

  return retValue;
}

std::string CEnvironment::win_getenv(const std::string &name)
{
  if (name.empty())
    return "";

  uint32_t varSize = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
  if (varSize == 0)
    return ""; // Not found

  std::string result(varSize, 0);
  GetEnvironmentVariableA(name.c_str(), const_cast<char*>(result.c_str()), varSize);

  return result;
}
