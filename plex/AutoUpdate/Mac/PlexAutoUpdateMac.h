//
//  PlexAutoUpdateMac.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-22.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATEMAC_H
#define PLEXAUTOUPDATEMAC_H

#include "AutoUpdate/PlexAutoUpdate.h"

class CPlexAutoUpdateInstallerMac : public CAutoUpdateInstallerBase
{
  public:
    CPlexAutoUpdateInstallerMac(CAutoUpdateFunctionsBase* functions) : CAutoUpdateInstallerBase(functions) {};
  virtual bool InstallUpdate(const std::string &file, std::string& unpackpath);
};

#endif // PLEXAUTOUPDATEMAC_H
