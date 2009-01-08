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
#include "GUIWindowManager.h"
#include "Application.h"

#include "karaokelyrics.h"
#include "karaokelyricsfactory.h"
#include "karaokelyricsmanager.h"

#include "GUIDialogKaraokeSongSelector.h"


CKaraokeLyricsManager::CKaraokeLyricsManager()
{
  m_Lyrics = 0;
  m_songSelector = 0;
  m_karaokeSongPlaying = false;
  m_karaokeSongPlayed = false;
}

CKaraokeLyricsManager::~ CKaraokeLyricsManager()
{
  if ( m_Lyrics )
  {
    m_Lyrics->Shutdown();
    delete m_Lyrics;
  }
  
  delete m_songSelector;
}

bool CKaraokeLyricsManager::Start(const CStdString & strSongPath)
{
  CSingleLock lock (m_CritSection);

  // Init to false
  m_karaokeSongPlayed = false;

  if ( m_Lyrics )
    Stop();  // shouldn't happen, but...

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
  m_Lyrics->InitGraphics();

  // make sure we have fullscreen viz
  if ( m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION )
    m_gWindowManager.ActivateWindow( WINDOW_VISUALISATION );

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

  m_Lyrics->Shutdown();
  delete m_Lyrics;
  m_Lyrics = 0;
}


void CKaraokeLyricsManager::Render()
{
  CSingleLock lock (m_CritSection);

  if ( m_Lyrics )
    m_Lyrics->Render();
}


bool CKaraokeLyricsManager::isLyricsAvailable() const
{
  CSingleLock lock (m_CritSection);

  return m_Lyrics != 0;
}

bool CKaraokeLyricsManager::OnAction(const CAction & action)
{
  CSingleLock lock (m_CritSection);

  if ( !m_Lyrics || !g_application.IsPlayingAudio() || !m_karaokeSongPlaying )
    return false;

  if ( !isSongSelectorAvailable() )
    return false;

  switch(action.wID)
  {
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
      // Offset from key codes back to button number
      if ( !m_songSelector->IsActive() )
      {
        m_songSelector->init( action.wID - REMOTE_0 );
        m_songSelector->DoModal();
      }
      break;

    case ACTION_SUBTITLE_DELAY_MIN:
      m_Lyrics->lyricsDelayDecrease();
      return true;

    case ACTION_SUBTITLE_DELAY_PLUS:
      m_Lyrics->lyricsDelayIncrease();
      return true;
  }

  return false;
}

bool CKaraokeLyricsManager::isSongSelectorAvailable()
{
  if ( !m_songSelector )
    m_songSelector = (CGUIDialogKaraokeSongSelectorSmall *)m_gWindowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

  return m_songSelector ? true : false;
}

void CKaraokeLyricsManager::OnPlaybackEnded()
{
  CSingleLock lock (m_CritSection);

  if ( m_karaokeSongPlaying )
    Stop();

  if ( !m_karaokeSongPlayed )
    return;

  m_karaokeSongPlayed = false; // so it won't popup again
  CGUIDialogKaraokeSongSelectorLarge * selector = 
      (CGUIDialogKaraokeSongSelectorLarge*)m_gWindowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SELECTOR );

  selector->init( 0 );
  selector->DoModal();
}
