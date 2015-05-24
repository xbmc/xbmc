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
#include "filesystem/SpecialProtocol.h"
#include "Application.h"
#include "utils/StringUtils.h"

FARPROC WINAPI delayHookNotifyFunc (unsigned dliNotify, PDelayLoadInfo pdli)
{
  switch (dliNotify)
  {
    case dliNotePreLoadLibrary:
      if (stricmp(pdli->szDll, "ssh.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/ssh.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "sqlite3.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/sqlite3.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "dnssd.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/dnssd.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "libxslt.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/libxslt.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "avcodec-56.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/avcodec-56.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "avfilter-5.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/avfilter-5.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "avformat-56.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/avformat-56.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "avutil-54.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/avutil-54.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "postproc-53.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/postproc-53.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "swresample-1.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/swresample-1.dll");
        HMODULE hMod = LoadLibraryEx(strDll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
        return (FARPROC)hMod;
      }
      if (stricmp(pdli->szDll, "swscale-3.dll") == 0)
      {
        std::string strDll = CSpecialProtocol::TranslatePath("special://xbmcbin/system/players/dvdplayer/swscale-3.dll");
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
