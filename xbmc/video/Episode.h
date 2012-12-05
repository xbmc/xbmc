#pragma once
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
    EPISODE(int Season = -1, int Episode = -1, int Subepisode = 0, bool Folder = false)
    {
      iSeason     = Season;
      iEpisode    = Episode;
      iSubepisode = Subepisode;
      isFolder    = Folder;
    }
    bool operator==(const struct EPISODE& rhs)
    {
      return (iSeason     == rhs.iSeason  &&
              iEpisode    == rhs.iEpisode &&
              iSubepisode == rhs.iSubepisode);
    }
  };

  typedef std::vector<EPISODE> EPISODELIST;
}

