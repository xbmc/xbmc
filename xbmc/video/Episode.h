#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include "FileItem.h"
#include "utils/ScraperUrl.h"
#include "XBDateTime.h"

// single episode information
namespace VIDEO
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
    CFileItemPtr item;
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

