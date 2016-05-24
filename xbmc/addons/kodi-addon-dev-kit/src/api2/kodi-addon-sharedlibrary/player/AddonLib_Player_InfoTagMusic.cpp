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
#include KITINCLUDE(ADDON_API_LEVEL, player/InfoTagMusic.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, player/Player.hpp)

API_NAMESPACE

namespace KodiAPI
{

namespace Player
{

  CInfoTagMusic::CInfoTagMusic(CPlayer* player)
   : m_playCount(0)
  {
    if (!player || !player->m_ControlHandle)
    {
      return;
    }

    AddonInfoTagMusic infoTag;
    if (g_interProcess.m_Callbacks->AddonInfoTagMusic.GetFromPlayer(g_interProcess.m_Handle, player, &infoTag))
    {
      TransferInfoTag(infoTag);
      g_interProcess.m_Callbacks->AddonInfoTagMusic.Release(g_interProcess.m_Handle, &infoTag);
    }
  }

  CInfoTagMusic::~CInfoTagMusic()
  {
  }

  const std::string& CInfoTagMusic::GetURL() const
  {
    return m_url;
  }

  const std::string& CInfoTagMusic::GetTitle() const
  {
    return m_title;
  }

  const std::string& CInfoTagMusic::GetArtist() const
  {
    return m_artist;
  }

  const std::string& CInfoTagMusic::GetAlbum() const
  {
    return m_album;
  }

  const std::string& CInfoTagMusic::GetAlbumArtist() const
  {
    return m_albumArtist;
  }

  const std::string& CInfoTagMusic::GetGenre() const
  {
    return m_genre;
  }

  int CInfoTagMusic::GetDuration() const
  {
    return m_duration;
  }

  int CInfoTagMusic::GetTrack() const
  {
    return m_tracks;
  }

  int CInfoTagMusic::GetDisc() const
  {
    return m_disc;
  }

  const std::string& CInfoTagMusic::GetReleaseDate() const
  {
    return m_releaseDate;
  }

  int CInfoTagMusic::GetListeners() const
  {
    return m_listener;
  }

  int CInfoTagMusic::GetPlayCount() const
  {
    return m_playCount;
  }

  const std::string& CInfoTagMusic::GetLastPlayed() const
  {
    return m_lastPlayed;
  }

  const std::string& CInfoTagMusic::GetComment() const
  {
    return m_comment;
  }

  const std::string& CInfoTagMusic::GetLyrics() const
  {
    return m_lyrics;
  }

  void CInfoTagMusic::TransferInfoTag(AddonInfoTagMusic& infoTag)
  {
    m_url = infoTag.m_url;
    m_title = infoTag.m_title;
    m_artist = infoTag.m_artist;
    m_album = infoTag.m_album;
    m_albumArtist = infoTag.m_albumArtist;
    m_genre = infoTag.m_genre;
    m_duration = infoTag.m_duration;
    m_tracks = infoTag.m_tracks;
    m_disc = infoTag.m_disc;
    m_releaseDate = infoTag.m_releaseDate;
    m_listener = infoTag.m_listener;
    m_playCount = infoTag.m_playCount;
    m_lastPlayed = infoTag.m_lastPlayed;
    m_comment = infoTag.m_comment;
    m_lyrics = infoTag.m_lyrics;
  }


} /* namespace Player */
} /* namespace KodiAPI */

END_NAMESPACE()
