/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_DARWIN_OSX)
#include "utils/URIUtils.h"
#include "platform/darwin/DarwinUtils.h"
#elif defined(TARGET_POSIX)
#else
#endif

#include "AliasShortcutUtils.h"
#include "utils/log.h"

bool IsAliasShortcut(const std::string& path, bool isdirectory)
{
  bool  rtn = false;

#if defined(TARGET_DARWIN_OSX)
  // Note: regular files that have an .alias extension can be
  //   reported as an alias when clearly, they are not. Trap them out.
  if (!URIUtils::HasExtension(path, ".alias"))//! @todo - check if this is still needed with the new API
  {
    rtn = CDarwinUtils::IsAliasShortcut(path, isdirectory);
  }
#elif defined(TARGET_POSIX)
  // Linux does not use alias or shortcut methods
#elif defined(TARGET_WINDOWS)
/* Needs testing under Windows platform so ignore shortcuts for now
    if (CUtil::GetExtension(path) == ".lnk")
    {
      rtn = true;
    }
*/
#endif
  return(rtn);
}

void TranslateAliasShortcut(std::string& path)
{
#if defined(TARGET_DARWIN_OSX)
  CDarwinUtils::TranslateAliasShortcut(path);
#elif defined(TARGET_POSIX)
  // Linux does not use alias or shortcut methods
#elif defined(TARGET_WINDOWS_STORE)
  // Win10 does not use alias or shortcut methods
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
#elif defined(TARGET_WINDOWS)
/* Needs testing under Windows platform so ignore shortcuts for now
  CComPtr<IShellLink> ipShellLink;

  // Get a pointer to the IShellLink interface
  if (NOERROR == CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&ipShellLink))
    WCHAR wszTemp[MAX_PATH];

    // Get a pointer to the IPersistFile interface
    CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);

    // IPersistFile is using LPCOLESTR so make sure that the string is Unicode
#if !defined _UNICODE
    MultiByteToWideChar(CP_ACP, 0, lpszShortcutPath, -1, wszTemp, MAX_PATH);
#else
    wcsncpy(wszTemp, lpszShortcutPath, MAX_PATH);
#endif

    // Open the shortcut file and initialize it from its contents
    if (NOERROR == ipPersistFile->Load(wszTemp, STGM_READ))
    {
      // Try to find the target of a shortcut even if it has been moved or renamed
      if (NOERROR == ipShellLink->Resolve(NULL, SLR_UPDATE))
      {
        WIN32_FIND_DATA wfd;
        TCHAR real_path[PATH_MAX];
        // Get the path to the shortcut target
        if (NOERROR == ipShellLink->GetPath(real_path, MAX_PATH, &wfd, SLGP_RAWPATH))
        {
          // Get the description of the target
          TCHAR szDesc[MAX_PATH];
          if (NOERROR == ipShellLink->GetDescription(szDesc, MAX_PATH))
          {
            path = real_path;
          }
        }
      }
    }
  }
*/
#endif
}
