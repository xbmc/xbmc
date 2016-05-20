/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, player/InfoTagVideo.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, player/Player.hpp)

API_NAMESPACE

namespace KodiAPI
{

namespace Player
{

  CInfoTagVideo::CInfoTagVideo(CPlayer* player)
  {
    if (!player || !player->m_ControlHandle)
    {
      return;
    }

    AddonInfoTagVideo infoTag;
    if (g_interProcess.m_Callbacks->AddonInfoTagVideo.GetFromPlayer(g_interProcess.m_Handle, player, &infoTag))
    {
      TransferInfoTag(infoTag);
      g_interProcess.m_Callbacks->AddonInfoTagVideo.Release(g_interProcess.m_Handle, &infoTag);
    }
  }

  CInfoTagVideo::~CInfoTagVideo()
  {
  }

  const std::string& CInfoTagVideo::GetDirector() const
  {
    return m_director;
  }

  const std::string& CInfoTagVideo::GetWritingCredits() const
  {
    return m_writingCredits;
  }

  const std::string& CInfoTagVideo::GetGenre() const
  {
    return m_genre;
  }

  const std::string& CInfoTagVideo::GetCountry() const
  {
    return m_country;
  }

  const std::string& CInfoTagVideo::GetTagLine() const
  {
    return m_tagLine;
  }

  const std::string& CInfoTagVideo::GetPlotOutline() const
  {
    return m_plotOutline;
  }

  const std::string& CInfoTagVideo::GetPlot() const
  {
    return m_plot;
  }

  const std::string& CInfoTagVideo::GetTrailer() const
  {
    return m_trailer;
  }

  const std::string& CInfoTagVideo::GetPictureURL() const
  {
    return m_pictureURL;
  }

  const std::string& CInfoTagVideo::GetTitle() const
  {
    return m_title;
  }

  const std::string& CInfoTagVideo::GetVotes() const
  {
    return m_votes;
  }

  const std::string& CInfoTagVideo::GetCast() const
  {
    return m_cast;
  }

  const std::string& CInfoTagVideo::GetFile() const
  {
    return m_file;
  }

  const std::string& CInfoTagVideo::GetPath() const
  {
    return m_path;
  }

  const std::string& CInfoTagVideo::GetIMDBNumber() const
  {
    return m_IMDBNumber;
  }

  const std::string& CInfoTagVideo::GetMPAARating() const
  {
    return m_MPAARating;
  }

  int CInfoTagVideo::GetYear() const
  {
    return m_year;
  }

  double CInfoTagVideo::GetRating() const
  {
    return m_rating;
  }

  int CInfoTagVideo::GetPlayCount() const
  {
    return m_playCount;
  }

  const std::string& CInfoTagVideo::GetLastPlayed() const
  {
    return m_lastPlayed;
  }

  const std::string& CInfoTagVideo::GetOriginalTitle() const
  {
    return m_originalTitle;
  }

  const std::string& CInfoTagVideo::GetPremiered() const
  {
    return m_premiered;
  }

  const std::string& CInfoTagVideo::GetFirstAired() const
  {
    return m_firstAired;
  }

  unsigned int CInfoTagVideo::GetDuration() const
  {
    return m_duration;
  }

  void CInfoTagVideo::TransferInfoTag(AddonInfoTagVideo& infoTag)
  {
    m_director = infoTag.m_director;
    m_writingCredits = infoTag.m_writingCredits;
    m_genre = infoTag.m_genre;
    m_country = infoTag.m_country;
    m_tagLine = infoTag.m_tagLine;
    m_plotOutline = infoTag.m_plotOutline;
    m_plot = infoTag.m_plot;
    m_trailer = infoTag.m_trailer;
    m_pictureURL = infoTag.m_pictureURL;
    m_title = infoTag.m_title;
    m_votes = infoTag.m_votes;
    m_cast = infoTag.m_cast;
    m_file = infoTag.m_file;
    m_path = infoTag.m_path;
    m_IMDBNumber = infoTag.m_IMDBNumber;
    m_MPAARating = infoTag.m_MPAARating;
    m_year = infoTag.m_year;
    m_rating = infoTag.m_rating;
    m_playCount = infoTag.m_playCount;
    m_lastPlayed = infoTag.m_lastPlayed;
    m_originalTitle = infoTag.m_originalTitle;
    m_premiered = infoTag.m_premiered;
    m_firstAired = infoTag.m_firstAired;
    m_duration = infoTag.m_duration;
  }


} /* namespace Player */
} /* namespace KodiAPI */

END_NAMESPACE()
