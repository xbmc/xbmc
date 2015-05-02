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
 * \file utils\Environment.cpp
 * \brief Implements CEnvironment class functions.
 *  
 *  Some ideas were inspired by PostgreSQL's pgwin32_putenv function. 
 *  Refined, updated, enhanced and modified for XBMC by Karlson2k.
 */

#include "Environment.h"
#include <stdlib.h>
#ifdef TARGET_WINDOWS
#include <Windows.h>
#endif

// --------------------- Helper Functions ---------------------

#ifdef TARGET_WINDOWS

std::wstring CEnvironment::win32ConvertUtf8ToW(const std::string &text, bool *resultSuccessful /* = NULL*/)  
{
  if (text.empty())
  {
    if (resultSuccessful != NULL)
      *resultSuccessful = true;
    return L"";
  }
  if (resultSuccessful != NULL)
    *resultSuccessful = false;

  int bufSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), -1, NULL, 0);
  if (bufSize == 0)
    return L"";
  wchar_t *converted = new wchar_t[bufSize];
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), -1, converted, bufSize) != bufSize)
  {
    delete[] converted;
    return L"";
  }

  std::wstring Wret (converted);
  delete[] converted;

  if (resultSuccessful != NULL)
    *resultSuccessful = true;
  return Wret;
}

std::string CEnvironment::win32ConvertWToUtf8(const std::wstring &text, bool *resultSuccessful /*= NULL*/)  
{
  if (text.empty())
  {
    if (resultSuccessful != NULL)
      *resultSuccessful = true;
    return "";
  }
  if (resultSuccessful != NULL)
    *resultSuccessful = false;

  int bufSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.c_str(), -1, NULL, 0, NULL, NULL);
  if (bufSize == 0)
    return "";
  char * converted = new char[bufSize];
  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.c_str(), -1, converted, bufSize, NULL, NULL) != bufSize)
  {
    delete[] converted;
    return "";
  }

  std::string ret(converted);
  delete[] converted;
  
  if (resultSuccessful != NULL)
    *resultSuccessful = true;
  return ret;
}

// --------------------- Internal Function ---------------------

typedef int (_cdecl * wputenvPtr) (const wchar_t *envstring);

/**
 * \fn int CEnvironment::win32_setenv(const std::wstring &name, const std::wstring &value = L"",
 *     updateAction action = autoDetect)
 * \brief Internal function used to manipulate with environment variables on win32.
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
int CEnvironment::win32_setenv(const std::string &name, const std::string &value /* = "" */, enum updateAction action /* = autoDetect */)
{
  std::wstring Wname (win32ConvertUtf8ToW(name));
  if (Wname.empty() || name.find('=') != std::wstring::npos)
    return -1;
  if ( (action == addOnly || action == addOrUpdateOnly) && value.empty() )
    return -1;
  if (action == addOnly && !(getenv(name).empty()) )
    return 0;

  bool convIsOK;
  std::wstring Wvalue (win32ConvertUtf8ToW(value,&convIsOK));
  if (!convIsOK)
    return -1;

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
    { NULL }             // Terminating NULL for list
  };
  
  // Check all modules each function run, because modules can be loaded/unloaded at runtime
  for (int i = 0; modulesList[i]; i++)
  {
    HMODULE hModule;
    if (!GetModuleHandleExW(0, modulesList[i], &hModule) || hModule == NULL) // Flag 0 ensures that module will be kept loaded until it'll be freed
      continue; // Module not loaded

    wputenvPtr wputenvFunc = (wputenvPtr) GetProcAddress(hModule, "_wputenv");
    if (wputenvFunc != NULL && wputenvFunc(EnvString.c_str()) != 0)
      retValue |= 2; // At lest one external runtime library Environment update failed
    FreeLibrary(hModule);
  }

  // Update process Environment used for current process and for future new child processes
  if (action == deleteVariable || value.empty())
    retValue += SetEnvironmentVariableW(Wname.c_str(), NULL) ? 0 : 4; // 4 if failed
  else
    retValue += SetEnvironmentVariableW(Wname.c_str(), Wvalue.c_str()) ? 0 : 4; // 4 if failed
  
  // Finally update our runtime Environment
  retValue += (::_wputenv(EnvString.c_str()) == 0) ? 0 : 8; // 8 if failed
  
  return retValue;
}
#endif

// --------------------- Main Functions ---------------------

int CEnvironment::setenv(const std::string &name, const std::string &value, int overwrite /*= 1*/)
{
#ifdef TARGET_WINDOWS
  return (win32_setenv(name, value, overwrite ? autoDetect : addOnly)==0) ? 0 : -1;
#else
  if (value.empty() && overwrite != 0)
    return ::unsetenv(name.c_str());
  return ::setenv(name.c_str(), value.c_str(), overwrite);
#endif
}

std::string CEnvironment::getenv(const std::string &name)
{
#ifdef TARGET_WINDOWS
  std::wstring Wname (win32ConvertUtf8ToW(name));
  if (Wname.empty())
    return "";

  wchar_t * wStr = ::_wgetenv(Wname.c_str());
  if (wStr != NULL)
    return win32ConvertWToUtf8(wStr);

  // Not found in Environment of runtime library 
  // Try Environment of process as fallback
  unsigned int varSize = GetEnvironmentVariableW(Wname.c_str(), NULL, 0);
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

  return win32ConvertWToUtf8(Wvalue);
#else
  char * str = ::getenv(name.c_str());
  if (str == NULL)
    return "";
  return str;
#endif
}

int CEnvironment::unsetenv(const std::string &name)
{
#ifdef TARGET_WINDOWS
  return (win32_setenv(name, "", deleteVariable)) == 0 ? 0 : -1;
#else
  return ::unsetenv(name.c_str());
#endif
}

int CEnvironment::putenv(const std::string &envstring)
{
  if (envstring.empty())
    return 0;
  size_t pos = envstring.find('=');
  if (pos == 0) // '=' is the first character
    return -1;
  if (pos == std::string::npos)
    return unsetenv(envstring);
  if (pos == envstring.length()-1) // '=' is in last position
  {
    std::string name(envstring);
    name.erase(name.length()-1, 1);
    return unsetenv(name);
  }
  std::string name(envstring, 0, pos), value(envstring, pos+1);

  return setenv(name, value);
}

