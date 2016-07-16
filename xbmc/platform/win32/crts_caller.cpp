/*
 *      Copyright (C) 2015 Team Kodi
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
 * \file win32\crts_caller.h
 * \brief Implements crts_caller class for calling same function for all loaded CRTs.
 * \author Karlson2k
 */

#include "crts_caller.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cassert>

namespace win32_utils
{

static const wchar_t* const s_listOfCrts[] =
{
  { L"msvcrt.dll" },   // Visual Studio 6.0 / MinGW[-w64]
  { L"msvcr70.dll" },  // Visual Studio 2002
  { L"msvcr71.dll" },  // Visual Studio 2003
  { L"msvcr80.dll" },  // Visual Studio 2005
  { L"msvcr90.dll" },  // Visual Studio 2008
#ifdef _DEBUG
  { L"msvcr90d.dll" }, // Visual Studio 2008 (debug)
#endif
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
};

std::vector<std::wstring> crts_caller::getCrtNames()
{
  return std::vector<std::wstring>(s_listOfCrts, s_listOfCrts + (sizeof(s_listOfCrts) / sizeof(s_listOfCrts[0])));
}


crts_caller::crts_caller(const char* func_name)
{
  assert(func_name);
  assert(func_name[0]);
  if (func_name == NULL)
    return;

  for (const wchar_t* const crtName : s_listOfCrts)
  {
    HMODULE hCrt = NULL;
    if (!GetModuleHandleExW(0, crtName, &hCrt) || hCrt == NULL) // Flag 0 ensures that CRL will not be unloaded while we are using it here
      continue; // Module not loaded

    void* func_ptr = GetProcAddress(hCrt, func_name);
    if (func_ptr != NULL)
    {
      m_crts.push_back(hCrt);
      m_funcPointers.push_back(func_ptr);
    }
    else
      FreeLibrary(hCrt); // this CRT will not be used
  }
}

crts_caller::~crts_caller()
{
  for (void* hCrt : m_crts)
    FreeLibrary((HMODULE)hCrt);
}

}
