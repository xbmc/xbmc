//
// C++ Implementation: karaokelyricsmanager
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stdafx.h"

#include "Application.h"
#include "GUIWindowManager.h"

#include "karaokelyrics.h"
#include "karaokelyricsfactory.h"
#include "karaokelyricsmanager.h"

#include "GUIDialogKaraokeSongSelector.h"
#include "GUIWindowKaraokeLyrics.h"


CKaraokeLyricsManager::CKaraokeLyricsManager()
{
  m_Lyrics = 0;
  m_karaokeSongPlaying = false;
  m_karaokeSongPlayed = false;
  m_lastPlayedTime = 0;
}

CKaraokeLyricsManager::~ CKaraokeLyricsManager()
{
  if ( m_Lyrics )
  {
    m_Lyrics->Shutdown();
    delete m_Lyrics;
  }
}

bool CKaraokeLyricsManager::Start(const CStdString & strSongPath)
{
  CSingleLock lock (m_CritSection);

  // Init to false
  m_karaokeSongPlayed = false;
  m_lastPlayedTime = 0;

  if ( m_Lyrics )
    Stop();  // shouldn't happen, but...

  // If disabled by configuration, do nothing
  if ( !g_guiSettings.GetBool("karaoke.enabled") )
    return false;

  m_Lyrics = CKaraokeLyricsFactory::CreateLyrics( strSongPath );

  if ( !m_Lyrics )
  {
    CLog::Log( LOGDEBUG, "Karaoke: lyrics for song %s not found", strSongPath.c_str() );
    return false;
  }

  m_Lyrics->initData( strSongPath );

  // Load the lyrics
  if ( !m_Lyrics->Load() )
  {
    CLog::Log( LOGWARNING, "Karaoke: lyrics for song %s found but cannot be loaded", strSongPath.c_str() );
    delete m_Lyrics;
    m_Lyrics = 0;
    return false;
  }

  CLog::Log( LOGDEBUG, "Karaoke: lyrics for song %s loaded successfully", strSongPath.c_str() );

  CGUIWindowKaraokeLyrics *window = (CGUIWindowKaraokeLyrics*) m_gWindowManager.GetWindow(WINDOW_KARAOKELYRICS);

  if ( !window )
  {
    CLog::Log( LOGERROR, "Karaoke window is not found" );
    return false;
  }

  window->setLyrics( m_Lyrics );

  // Activate karaoke window
  m_gWindowManager.ActivateWindow(WINDOW_KARAOKELYRICS);

  m_karaokeSongPlaying = true;
  m_karaokeSongPlayed = true;
  return true;
}

void CKaraokeLyricsManager::Stop()
{
  CSingleLock lock (m_CritSection);

  m_karaokeSongPlaying = false;

  if ( !m_Lyrics )
    return;

  // Clean up and close karaoke window when stopping
  CGUIWindowKaraokeLyrics *window = (CGUIWindowKaraokeLyrics*) m_gWindowManager.GetWindow(WINDOW_KARAOKELYRICS);

  if ( window )
    window->setLyrics( 0 );

   // turn off visualisation window when stopping
  if (m_gWindowManager.GetActiveWindow() == WINDOW_KARAOKELYRICS)
    m_gWindowManager.PreviousWindow();

  m_Lyrics->Shutdown();
  delete m_Lyrics;
  m_Lyrics = 0;
}


void CKaraokeLyricsManager::ProcessSlow()
{
  CSingleLock lock (m_CritSection);

  if ( g_application.IsPlaying() )
  {
    if ( m_karaokeSongPlaying )
      m_lastPlayedTime = timeGetTime();

    return;
  }

  if ( !m_karaokeSongPlayed || !g_guiSettings.GetBool("karaoke.autopopupselector") )
    return;

  // If less than 750ms passed return; we're still processing STOP events
  if ( !m_lastPlayedTime || timeGetTime() - m_lastPlayedTime < 750 )
    return;

  m_karaokeSongPlayed = false; // so it won't popup again

  CGUIDialogKaraokeSongSelectorLarge * selector = 
      (CGUIDialogKaraokeSongSelectorLarge*)m_gWindowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SELECTOR );

  selector->DoModal();
}
