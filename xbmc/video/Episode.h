/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "utils/ScraperUrl.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;

// single episode information
namespace KODI::VIDEO
{
  struct EPISODE
  {
    bool        isFolder;
    int         iSeason;
    int         iEpisode;
    int         iSubepisode;
    std::string strPath;
    std::string strTitle;
    CDateTime   cDate;
    CScraperUrl cScraperUrl;
    std::shared_ptr<CFileItem> item;
    EPISODE(int Season = -1, int Episode = -1, int Subepisode = 0, bool Folder = false)
    {
      iSeason     = Season;
      iEpisode    = Episode;
      iSubepisode = Subepisode;
      isFolder    = Folder;
    }
    bool operator==(const struct EPISODE& rhs) const
    {
      return (iSeason     == rhs.iSeason  &&
              iEpisode    == rhs.iEpisode &&
              iSubepisode == rhs.iSubepisode);
    }
  };

  typedef std::vector<EPISODE> EPISODELIST;
}

