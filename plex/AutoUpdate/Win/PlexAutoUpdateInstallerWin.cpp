//
//  PlexAutoUpdateInstallerWin.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-22.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexAutoUpdateInstallerWin.h"

#include <Windows.h>
#include <Shellapi.h>
#include <ShlObj.h>

bool
CPlexAutoUpdateInstallerWin::InstallUpdate(const std::string &file, std::string& unpackPath)
{
  ITEMIDLIST *pidl = ILCreateFromPath(file.c_str());
  if(pidl) {
    SHOpenFolderAndSelectItems(pidl,0,0,0);
    ILFree(pidl);
  }

  m_functions->TerminateApplication();
  return true;
}
