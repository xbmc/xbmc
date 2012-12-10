//
//  PlexAutoUpdateFunctionsXBMC.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-09.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATEFUNCTIONSXBMC_H
#define PLEXAUTOUPDATEFUNCTIONSXBMC_H

#include "plex/AutoUpdate/PlexAutoUpdate.h"
#include "utils/XBMCTinyXML.h"

class CAutoUpdateFunctionsXBMC : public CAutoUpdateFunctionsBase
{
   public:

    CAutoUpdateFunctionsXBMC(CPlexAutoUpdate* updater) : CAutoUpdateFunctionsBase(updater) {};
    virtual bool FetchUrlData(const std::string &url, std::string& data);
    virtual bool ParseXMLData(const std::string &xmlData, CAutoUpdateInfoList &infoList);
    virtual void LogDebug(const std::string& msg);
    virtual void LogInfo(const std::string &msg);

    virtual void NotifyNewVersion();

  private:
    bool ParseItemElement(TiXmlElement *el, CAutoUpdateInfo& info);
};

#endif // PLEXAUTOUPDATEFUNCTIONSXBMC_H
