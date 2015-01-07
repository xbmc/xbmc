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

// C++ Implementation: karaokelyrics

#include <math.h>

#include "utils/MathUtils.h"
#include "Application.h"
#include "music/MusicDatabase.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include "karaokelyrics.h"

CKaraokeLyrics::CKaraokeLyrics()
{
  m_avOrigDelay = 0;
  m_avDelay = 0;
  m_idSong = 0;
}


CKaraokeLyrics::~CKaraokeLyrics()
{
}

void CKaraokeLyrics::Shutdown()
{
  // Update the song-specific delay in the database
  if ( m_idSong && m_avOrigDelay != m_avDelay && g_advancedSettings.m_karaokeKeepDelay )
  {
    // If the song is in karaoke db, get the delay
    CMusicDatabase musicdatabase;
    if ( musicdatabase.Open() )
    {
      int delayval = MathUtils::round_int( m_avDelay * 10.0 );
      musicdatabase.SetKaraokeSongDelay( m_idSong, delayval );
      CLog::Log( LOGDEBUG, "Karaoke timing correction: set new delay %d for song %ld", delayval, m_idSong );
    }

    musicdatabase.Close();
  }

  m_idSong = 0;
}

bool CKaraokeLyrics::InitGraphics()
{
  return true;
}

void CKaraokeLyrics::initData( const std::string & songPath )
{
  m_songPath = songPath;

  // Reset AV delay
  m_avOrigDelay = m_avDelay = 0;

  // Get song ID if available
  m_idSong = 0;
  CMusicDatabase musicdatabase;

  // Get song-specific delay from the database
  if ( g_advancedSettings.m_karaokeKeepDelay && musicdatabase.Open() )
  {
    CSong song;
    if ( musicdatabase.GetSongByFileName( songPath, song) )
    {
      m_idSong = song.idSong;
      if ( song.iKaraokeDelay != 0 )
      {
        m_avOrigDelay = m_avDelay = (double) song.iKaraokeDelay / 10.0;
        CLog::Log( LOGDEBUG, "Karaoke timing correction: restored lyrics delay from database to %g", m_avDelay );
      }
    }

    musicdatabase.Close();
  }
}

void CKaraokeLyrics::lyricsDelayIncrease()
{
  m_avDelay += 0.05; // 50ms
}

void CKaraokeLyrics::lyricsDelayDecrease()
{
  m_avDelay -= 0.05; // 50ms
}

double CKaraokeLyrics::getSongTime() const
{
  // m_avDelay may be negative
  double songtime = g_application.GetTime() + m_avDelay;
  return songtime >= 0 ? songtime : 0.0;
}

std::string CKaraokeLyrics::getSongFile() const
{
  return m_songPath;
}
