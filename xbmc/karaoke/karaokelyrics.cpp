//
// C++ Implementation: karaokelyrics
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <math.h>

#include "MathUtils.h"
#include "Application.h"
#include "MusicDatabase.h"
#include "AdvancedSettings.h"
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

void CKaraokeLyrics::initData( const CStdString & songPath )
{
  m_songPath = songPath;

  // Reset AV delay
  m_avOrigDelay = m_avDelay = 0;

  // Get song ID if available
  m_idSong = 0;
  CSong song;
  CMusicDatabase musicdatabase;

  // Get song-specific delay from the database
  if ( g_advancedSettings.m_karaokeKeepDelay && musicdatabase.Open() )
  {
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

CStdString CKaraokeLyrics::getSongFile() const
{
  return m_songPath;
}
