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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <DelayImp.h>
#include "DllPaths_win32.h"
#include "filesystem/SpecialProtocol.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"

FARPROC WINAPI delayHookNotifyFunc (unsigned dliNotify, PDelayLoadInfo pdli)
{
  switch (dliNotify)
  {
    case dliNotePreLoadLibrary:
      if (stricmp(pdli->szDll, "libmicrohttpd-5.dll") == 0)
      {
        CStdString strDll = CSpecialProtocol::TranslatePath(DLL_PATH_LIBMICROHTTP);
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "ssh.dll") == 0)
      {
        CStdString strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/ssh.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "sqlite3.dll") == 0)
      {
        CStdString strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/sqlite3.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "libsamplerate-0.dll") == 0)
      {
        CStdString strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/libsamplerate-0.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "dnssd.dll") == 0)
      {
        CStdString strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/dnssd.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      break;
  }
  return NULL;
}

FARPROC WINAPI delayHookFailureFunc (unsigned dliNotify, PDelayLoadInfo pdli)
{
  switch (dliNotify)
  {
    case dliFailLoadLib:
      g_application.Stop(1);
      CStdString strError;
      strError.Format("Uh oh, can't load %s, exiting.", pdli->szDll);
      MessageBox(NULL, strError.c_str(), "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
      exit(1);
      break;
  }
  return NULL;
}

// assign hook functions
PfnDliHook __pfnDliNotifyHook2 = delayHookNotifyFunc;
PfnDliHook __pfnDliFailureHook2 = delayHookFailureFunc;