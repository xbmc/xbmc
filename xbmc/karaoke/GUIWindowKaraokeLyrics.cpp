/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "Application.h"
#include "GUIWindowManager.h"
#include "AdvancedSettings.h"

#include "GUIDialogKaraokeSongSelector.h"
#include "GUIDialogKaraokeNextSong.h"
#include "GUIWindowKaraokeLyrics.h"
#include "PlayList.h"
#include "PlayListPlayer.h"
#include "LocalizeStrings.h"
#include "MusicInfoTag.h"
#include "karaokelyrics.h"
#include "karaokewindowbackground.h"
#include "utils/SingleLock.h"
#include "utils/log.h"


CGUIWindowKaraokeLyrics::CGUIWindowKaraokeLyrics(void)
  : CGUIWindow(WINDOW_KARAOKELYRICS, "MusicKaraokeLyrics.xml")
{
  m_Lyrics = 0;
  m_lastCheckTime = 0.0;
  m_Background = new CKaraokeWindowBackground();
}


CGUIWindowKaraokeLyrics::~ CGUIWindowKaraokeLyrics(void )
{
  delete m_Background;
}


bool CGUIWindowKaraokeLyrics::OnAction(const CAction &action)
{
  CSingleLock lock (m_CritSection);

  if ( !m_Lyrics || !g_application.IsPlayingAudio() )
    return false;

  CGUIDialogKaraokeSongSelectorSmall * songSelector = (CGUIDialogKaraokeSongSelectorSmall *)
                                      g_windowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

  switch(action.GetID())
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
      if ( songSelector && !songSelector->IsActive() )
        songSelector->DoModal( action.GetID() - REMOTE_0 );
      break;

    case ACTION_SUBTITLE_DELAY_MIN:
      m_Lyrics->lyricsDelayDecrease();
      return true;

    case ACTION_SUBTITLE_DELAY_PLUS:
      m_Lyrics->lyricsDelayIncrease();
      return true;
  }

  // If our background control could handle the action, let it do it
  if ( m_Background && m_Background->OnAction(action) )
    return true;

  return CGUIWindow::OnAction(action);
}


bool CGUIWindowKaraokeLyrics::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // Must be called here so we get our window ID and controls
      if ( !CGUIWindow::OnMessage(message) )
        return false;

      m_Background->Init( this );
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_CritSection);

      // Close the song selector dialog if shown
      CGUIDialogKaraokeSongSelectorSmall * songSelector = (CGUIDialogKaraokeSongSelectorSmall *)
                                      g_windowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

      if ( songSelector && songSelector->IsActive() )
        songSelector->Close();

      // Close the next song window if shown
      CGUIDialogKaraokeNextSong * nextSong = (CGUIDialogKaraokeNextSong *)
              g_windowManager.GetWindow( WINDOW_DIALOG_KARAOKE_NEXTSONG );

      if ( nextSong && nextSong->IsDialogRunning() )
        nextSong->Close();
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}


void CGUIWindowKaraokeLyrics::Render()
{
  g_application.ResetScreenSaver();
  CGUIWindow::Render();

  CSingleLock lock (m_CritSection);

  if ( m_Lyrics )
  {
    m_Background->Render();
    m_Lyrics->Render();
  }

  // Show or hide the next song window
  showNextSongIfNecessary();
}

void CGUIWindowKaraokeLyrics::newSong(CKaraokeLyrics * lyrics)
{
  CSingleLock lock (m_CritSection);
  m_Lyrics = lyrics;
  m_lastCheckTime = 0.0;

  if ( m_Lyrics )
    m_Lyrics->InitGraphics();

  // Set up current background mode
  if ( m_Lyrics->HasVideo() )
  {
    CStdString path;
    __int64 offset;

    // Start the required video
    m_Lyrics->GetVideoParameters( path, offset );
    m_Background->StartVideo( path, offset );
  }
  else if ( m_Lyrics->HasBackground() && g_advancedSettings.m_karaokeAlwaysEmptyOnCdgs )
  {
    m_Background->StartEmpty();
  }
  else
  {
    m_Background->StartDefault();
  }
}


void CGUIWindowKaraokeLyrics::stopSong()
{
  CSingleLock lock (m_CritSection);
  m_Lyrics = 0;

  m_Background->Stop();
}

void CGUIWindowKaraokeLyrics::pauseSong(bool now_paused)
{
  CSingleLock lock (m_CritSection);
  m_Background->Pause( now_paused );
}

void CGUIWindowKaraokeLyrics::showNextSongIfNecessary()
{
  // Check if enabled

  // Do not waste CPU, check only four times a second
  if ( g_application.GetTime() - m_lastCheckTime < 0.25 )
    return;

  m_lastCheckTime = g_application.GetTime();

  // How much time in advance we should show it?
  double timing = g_advancedSettings.m_karaokeNextSongPopupTime;

  // Zero value disables it
  if ( timing == 0.0 )
    return;
  
  double time_remaining = g_application.GetTotalTime() - g_application.GetTime();

  // Not yet to show?
  if ( time_remaining > timing )
    return;

  // Do we have a window? Missing skin?
  CGUIDialogKaraokeNextSong * nextSongWindow = (CGUIDialogKaraokeNextSong *)
            g_windowManager.GetWindow( WINDOW_DIALOG_KARAOKE_NEXTSONG );

  if ( !nextSongWindow )
  {
    static bool noannoy = false;

    if ( !noannoy )
    {
      CLog::Log( LOGERROR, "CGUIDialogKaraokeNextSong cannot be created; likely because DialogKaraokeNextSong.xml is missing" );
      noannoy = true;
    }

    return;
  }

  // Less than 0.25 second left? Hide it if it is shown.
  if ( time_remaining < 0.25 )
  {
    if ( nextSongWindow->IsDialogRunning() )
      nextSongWindow->Close();

    return;
  }

  // The window need to be pop up now. Find the next song in playlist
  CStdString nextsong;

  int iNext = g_playlistPlayer.GetNextSong();
  PLAYLIST::CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
  
  if ( iNext >= 0 && iNext < playlist.size() )
  {
    CFileItemPtr item = playlist[iNext];
    item->LoadMusicTag();
    const MUSIC_INFO::CMusicInfoTag* mtag = item->GetMusicInfoTag();

    if ( item->IsKaraoke() && mtag )
      nextsong = g_localizeStrings.Get(209) + ": " + mtag->GetTitle(); // Next:
  }
  
  // Now if nextsong is empty, this means there is no next song (or it is not a karaoke).
  // If the window is already shown, hide it (if the playlist changed via web interface)
  if ( nextsong.empty() )
  {
    if ( nextSongWindow->IsDialogRunning() )
      nextSongWindow->Close();

    return;
  }

  // Pop up the window if it does not yet.
  if ( !nextSongWindow->IsDialogRunning() )
    nextSongWindow->Show();
  
  // Update the song name if it is now different
  if ( nextSongWindow->songName() != nextsong )
    nextSongWindow->setSongName( nextsong );
}
