/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * This code is taken from epg.c in the Video Disk Recorder ('VDR')
 */

#include "epg.h"

cEpg::cEpg()
{
  m_uid             = 0;
  m_title           = NULL;
  m_shortText       = NULL;
  m_description     = NULL;
  m_aux             = NULL;
  m_StartTime       = 0;
  m_EndTime         = 0;
  m_Duration        = 0;
  m_genre           = NULL;
  m_genre_type      = 0;
  m_genre_sub_type  = 0;
  m_parental_rating = -1;
  m_vps             = 0;

}

cEpg::~cEpg()
{
  free(m_aux);
  free(m_genre);
  free(m_title);
  free(m_shortText);
  free(m_description);
}

void cEpg::Reset()
{
  free(m_aux);
  free(m_genre);
  free(m_title);
  free(m_shortText);
  free(m_description);

  m_StartTime       = 0;
  m_EndTime         = 0;
  m_Duration        = 0;
  m_genre_type      = 0;
  m_genre_sub_type  = 0;
  m_parental_rating = -1;
  m_vps             = 0;
  m_uid             = 0;
  m_title           = NULL;
  m_shortText       = NULL;
  m_description     = NULL;
  m_aux             = NULL;
  m_genre           = NULL;
}

bool cEpg::ParseLine(const char *s)
{
  char *t = skipspace(s + 1);
  switch (*s)
  {
    case 'T': SetTitle(t);
              break;
    case 'S': SetShortText(t);
              break;
    case 'D': strreplace(t, '|', '\n');
              SetDescription(t);
              break;
    case 'E':
      {
        unsigned int EventID;
        time_t StartTime;
        int Duration;
        unsigned int TableID = 0;
        unsigned int Version = 0xFF; // actual value is ignored
        int n = sscanf(t, "%u %ld %d %X %X", &EventID, &StartTime, &Duration, &TableID, &Version);
        if (n >= 3 && n <= 5)
        {
          m_StartTime = StartTime;
          m_EndTime   = StartTime+Duration;
          m_Duration  = Duration;
          m_uid       = EventID;
        }
      }
      break;
    case 'G':
      {
        char genre[1024];
        int genreType;
        int genreSubType;
        int n = sscanf(t, "%u %u %[^\n]", &genreType, &genreSubType, genre);
        if (n == 3)
        {
          SetGenre(genre, genreType, genreSubType);
        }
      }
      break;
    case 'X': break;
    case 'C': break;
    case 'c': break;
    case 'V': SetVps(atoi(t));
              break;
    case 'R': SetParentalRating(atoi(t));
              break;
    case 'e': return true;
    default:  XBMC->Log(LOG_ERROR, "cEpg::ParseLine - unexpected tag while reading EPG data: %s", s);
              return true;
  }
  return false;
}

void cEpg::SetTitle(const char *Title)
{
  m_title = strcpyrealloc(m_title, Title);
}

void cEpg::SetShortText(const char *ShortText)
{
  m_shortText = strcpyrealloc(m_shortText, ShortText);
}

void cEpg::SetDescription(const char *Description)
{
  m_description = strcpyrealloc(m_description, Description);
}

void cEpg::SetGenre(const char *Genre, int genreType, int genreSubType)
{
  m_genre_type      = genreType;
  m_genre_sub_type  = genreSubType;
  if (Genre)
    m_genre = strcpyrealloc(m_genre, Genre);
}

void cEpg::SetVps(time_t Vps)
{
  m_vps = Vps;
}

void cEpg::SetParentalRating(int Rating)
{
  m_parental_rating = Rating;
}
