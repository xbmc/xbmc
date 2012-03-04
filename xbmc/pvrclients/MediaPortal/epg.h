#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __EPG_H
#define __EPG_H

#include <stdlib.h>
#include <string>
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "GenreTable.h"

using namespace std;

class cEpg
{
private:
  unsigned int m_uid;
  string m_title;
  string m_shortText;
  string m_description;
  time_t m_StartTime;
  time_t m_EndTime;
  time_t m_originalAirDate;
  int m_Duration;
  string m_genre;
  int m_genre_type;
  int m_genre_subtype;
  int m_episodeNumber;
  string m_episodePart;
  string m_episodeName;
  int m_seriesNumber;
  int m_starRating;
  int m_parentalRating;
  CGenreTable* m_genretable;

public:
  cEpg();
  virtual ~cEpg();
  void Reset();

  bool ParseLine(string& data);
  int UniqueId(void) const { return m_uid; }
  time_t StartTime(void) const { return m_StartTime; }
  time_t EndTime(void) const { return m_EndTime; }
  time_t Duration(void) const { return m_Duration; }
  time_t OriginalAirDate(void) const { return m_originalAirDate; }
  const char *Title(void) const { return m_title.c_str(); }
  const char *ShortText(void) const { return m_shortText.c_str(); }
  const char *Description(void) const { return m_description.c_str(); }
  const char *Genre(void) const { return m_genre.c_str(); }
  int GenreType(void) const { return m_genre_type; }
  int GenreSubType(void) const { return m_genre_subtype; }
  int SeriesNumber(void) const { return m_seriesNumber; };
  int EpisodeNumber(void) const { return m_episodeNumber; };
  const char* EpisodeName(void) const { return m_episodeName.c_str(); };
  const char* EpisodePart(void) const { return m_episodePart.c_str(); };
  int StarRating(void) const { return m_starRating; };
  int ParentalRating(void) const { return m_parentalRating; };
  void SetGenreTable(CGenreTable* genremap);
};

#endif //__EPG_H
