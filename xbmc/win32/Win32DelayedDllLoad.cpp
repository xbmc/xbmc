/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <DelayImp.h>
#include "Application.h"
#include "utils/StringUtils.h"

static const std::string dlls[] = {
  "ssh.dll",
  "sqlite3.dll",
  "dnssd.dll",
  "libxslt.dll",
  "avcodec-56.dll",
  "avfilter-5.dll",
  "avformat-56.dll",
  "avutil-54.dll",
  "postproc-53.dll",
  "swresample-1.dll",
  "swscale-3.dll"
};

FARPROC WINAPI delayHookNotifyFunc (unsigned dliNotify, PDelayLoadInfo pdli)
{
  switch (dliNotify)
  {
    case dliNotePreLoadLibrary:
      for (size_t i = 0; i < ARRAYSIZE(dlls); ++i)
      {
        if (stricmp(pdli->szDll, dlls[i].c_str()) == 0)
        {
          HMODULE hMod = LoadLibraryEx(pdli->szDll, 0, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
          return (FARPROC)hMod;
        }
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
      std::string strError = StringUtils::Format("Uh oh, can't load %s, exiting.", pdli->szDll);
      MessageBox(NULL, strError.c_str(), "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
      exit(1);
      break;
  }
  return NULL;
}

// assign hook functions
PfnDliHook __pfnDliNotifyHook2 = delayHookNotifyFunc;
PfnDliHook __pfnDliFailureHook2 = delayHookFailureFunc;
