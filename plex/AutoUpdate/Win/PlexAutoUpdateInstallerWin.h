//
//  PlexAutoUpdate.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATEINSTALLERWIN_H
#define PLEXAUTOUPDATEINSTALLERWIN_H

#include <string>
#include "AutoUpdate\PlexAutoUpdate.h"

class CPlexAutoUpdateInstallerWin : public CAutoUpdateInstallerBase
{
  public:
    CPlexAutoUpdateInstallerWin(CAutoUpdateFunctionsBase* functions) : CAutoUpdateInstallerBase(functions) {};
	  virtual bool InstallUpdate(const std::string &file, std::string& unpackpath);
};

#endif