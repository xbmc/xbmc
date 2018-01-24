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
 * \file platform\win32\Environment.cpp
 * \brief Implements CEnvironment win32 specified class functions.
 */

#include "platform/Environment.h"
#include "platform/win32/CharsetConverter.h"
#include <stdlib.h>
#include <Windows.h>

// --------------------- Internal Function ---------------------

typedef int (_cdecl * wputenvPtr) (const wchar_t *envstring);

/**
 * \fn int CEnvironment::win32_setenv(const std::wstring &name, const std::wstring &value = L"",
 *     updateAction action = autoDetect)
 * \brief Internal function used to manipulate environment variables on win32.
 *
 * This function make all dirty work with setting, deleting and modifying environment variables.
 *
 * \param name   The environment variable name.
 * \param value  (optional) the new value of environment variable.
 * \param action (optional) the action.
 * \return Zero on success, 2 if at least one external runtime update failed, 4 if process
 * 		   environment update failed, 8 if our runtime environment update failed or, in case of
 * 		   several errors, sum of all errors values; non-zero in case of other errors.
 */
int CEnvironment::win_setenv(const std::string &name, const std::string &value /* = "" */, enum updateAction action /* = autoDetect */)
{
  std::wstring Wname = KODI::PLATFORM::WINDOWS::ToW(name);
  if (Wname.empty() || name.find('=') != std::wstring::npos)
    return -1;
  if ( (action == addOnly || action == addOrUpdateOnly) && value.empty() )
    return -1;
  if (action == addOnly && !getenv(name).empty() )
    return 0;

  std::wstring Wvalue = KODI::PLATFORM::WINDOWS::ToW(value);
  int retValue = 0;

  std::wstring EnvString;
  if (action == deleteVariable)
    EnvString = Wname + L"=";
  else
    EnvString = Wname + L"=" + Wvalue;

  static const wchar_t *modulesList[] =
  {
  /*{ L"msvcrt20.dll" }, // Visual C++ 2.0 / 2.1 / 2.2
    { L"msvcrt40.dll" }, // Visual C++ 4.0 / 4.1 */ // too old and no UNICODE support - ignoring
    { L"msvcrt.dll" },   // Visual Studio 6.0 / MinGW[-w64]
    { L"msvcr70.dll" },  // Visual Studio 2002
    { L"msvcr71.dll" },  // Visual Studio 2003
    { L"msvcr80.dll" },  // Visual Studio 2005
    { L"msvcr90.dll" },  // Visual Studio 2008
    { L"msvcr100.dll" }, // Visual Studio 2010
#ifdef _DEBUG
    { L"msvcr100d.dll" },// Visual Studio 2010 (debug)
#endif
    { L"msvcr110.dll" }, // Visual Studio 2012
#ifdef _DEBUG
    { L"msvcr110d.dll" },// Visual Studio 2012 (debug)
#endif
    { L"msvcr120.dll" }, // Visual Studio 2013
#ifdef _DEBUG
    { L"msvcr120d.dll" },// Visual Studio 2013 (debug)
#endif
    { L"vcruntime140.dll" },
    { L"ucrtbase.dll" },
#ifdef _DEBUG
    { L"vcruntime140d.dll" },
    { L"ucrtbased.dll" },
#endif
    { nullptr }             // Terminating nullptr for list
  };

  // Check all modules each function run, because modules can be loaded/unloaded at runtime
  for (int i = 0; modulesList[i]; i++)
  {
    HMODULE hModule;
    if (!GetModuleHandleExW(0, modulesList[i], &hModule) || hModule == nullptr) // Flag 0 ensures that module will be kept loaded until it'll be freed
      continue; // Module not loaded

    wputenvPtr wputenvFunc = (wputenvPtr) GetProcAddress(hModule, "_wputenv");
    if (wputenvFunc != nullptr && wputenvFunc(EnvString.c_str()) != 0)
      retValue |= 2; // At lest one external runtime library Environment update failed
    FreeLibrary(hModule);
  }

  // Update process Environment used for current process and for future new child processes
  if (action == deleteVariable || value.empty())
    retValue += SetEnvironmentVariableW(Wname.c_str(), nullptr) ? 0 : 4; // 4 if failed
  else
    retValue += SetEnvironmentVariableW(Wname.c_str(), Wvalue.c_str()) ? 0 : 4; // 4 if failed

  // Finally update our runtime Environment
  retValue += (::_wputenv(EnvString.c_str()) == 0) ? 0 : 8; // 8 if failed
  return retValue;
}

std::string CEnvironment::win_getenv(const std::string &name)
{
  std::wstring Wname = KODI::PLATFORM::WINDOWS::ToW(name);
  if (Wname.empty())
    return "";

  wchar_t * wStr = ::_wgetenv(Wname.c_str());
  if (wStr != nullptr)
    return KODI::PLATFORM::WINDOWS::FromW(wStr);

  // Not found in Environment of runtime library
  // Try Environment of process as fallback
  unsigned int varSize = GetEnvironmentVariableW(Wname.c_str(), nullptr, 0);
  if (varSize == 0)
    return ""; // Not found
  wchar_t * valBuf = new wchar_t[varSize];
  if (GetEnvironmentVariableW(Wname.c_str(), valBuf, varSize) != varSize-1)
  {
    delete[] valBuf;
    return "";
  }
  std::wstring Wvalue (valBuf);
  delete[] valBuf;

  return KODI::PLATFORM::WINDOWS::FromW(Wvalue);
}
