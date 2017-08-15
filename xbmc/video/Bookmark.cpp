/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Bookmark.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"

CBookmark::CBookmark()
{
  Reset();
}

void CBookmark::Reset()
{
  idBookmark = -1;
  episodeNumber = 0;
  seasonNumber = -1;
  timeInSeconds = 0.0f;
  totalTimeInSeconds = 0.0f;
  partNumber = 0;
  mediaType = MediaTypeNone;
  type = STANDARD;
}

bool CBookmark::IsSet() const
{
  return totalTimeInSeconds > 0.0f;
}

bool CBookmark::IsPartWay() const
{
  return totalTimeInSeconds > 0.0f && timeInSeconds > 0.0f;
}

std::string CBookmark::GetBookmarkTitle()
{
  switch (type)
  {
  case CBookmark::RESUME:
  case CBookmark::STANDARD:
    return StringUtils::SecondsToTimeString(static_cast<long>(timeInSeconds), TIME_FORMAT_HH_MM_SS);
  default:
    if (mediaType == MediaTypeEpisode)
    {
      if (seasonNumber == -1 && episodeNumber == 0)
      {
        CVideoDatabase videoDatabase;
        videoDatabase.Open();
        videoDatabase.GetEpisodeByBookmarkID(idBookmark, seasonNumber, episodeNumber);
        videoDatabase.Close();
      }
      return StringUtils::Format(g_localizeStrings.Get(20473).c_str(), seasonNumber, episodeNumber);
    }
    if (mediaType == MediaTypeMovie)
      return g_localizeStrings.Get(20474);
    if (mediaType == MediaTypeMusicVideo)
      return g_localizeStrings.Get(20472);
    break;
  }
  return "";
}
