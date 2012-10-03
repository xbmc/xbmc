#pragma once
#include "utils/ScraperUrl.h"

// single episode information
struct EPISODE
{
  int iSeason;
  int iEpisode;
  int iSubepisode;
  CDateTime cDate;
  CScraperUrl cScraperUrl;
  EPISODE(int Season = -1, int Episode = -1, int Subepisode = 0)
  {
    iSeason = Season;
    iEpisode = Episode;
    iSubepisode = Subepisode;
  }
  bool operator==(const struct EPISODE& rhs)
  {
    return (iSeason == rhs.iSeason &&
            iEpisode == rhs.iEpisode &&
            iSubepisode == rhs.iSubepisode);
  }
};

typedef std::vector<EPISODE> EPISODELIST;

