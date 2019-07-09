/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
