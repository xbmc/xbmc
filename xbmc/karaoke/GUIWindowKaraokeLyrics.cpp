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

#include "stdafx.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIVisualisationControl.h"

#include "GUIDialogKaraokeSongSelector.h"
#include "GUIWindowKaraokeLyrics.h"
#include "karaokelyrics.h"

#define CONTROL_KARAVIS          1


CGUIWindowKaraokeLyrics::CGUIWindowKaraokeLyrics(void)
  : CGUIWindow(WINDOW_KARAOKELYRICS, "MusicKaraokeLyrics.xml")
{

  m_bgMode = BACKGROUND_NONE;
  m_pVisControl = 0;
  m_Lyrics = 0;
}


CGUIWindowKaraokeLyrics::~ CGUIWindowKaraokeLyrics(void )
{
}


bool CGUIWindowKaraokeLyrics::OnAction(const CAction &action)
{
  CSingleLock lock (m_CritSection);

  if ( !m_Lyrics || !g_application.IsPlayingAudio() )
    return false;

  CGUIDialogKaraokeSongSelectorSmall * songSelector = (CGUIDialogKaraokeSongSelectorSmall *)
                                      m_gWindowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

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
      if ( songSelector && !songSelector->IsActive() )
	  {
		  CLog::Log( LOGDEBUG, ">>> Popup dialog" );
        songSelector->DoModal( action.wID - REMOTE_0 );
	  }
	  CLog::Log( LOGDEBUG, ">>> Action %d", action.wID );
      break;

    case ACTION_SUBTITLE_DELAY_MIN:
      m_Lyrics->lyricsDelayDecrease();
      return true;

    case ACTION_SUBTITLE_DELAY_PLUS:
      m_Lyrics->lyricsDelayIncrease();
      return true;
  }

  // Send it to the visualisation if we have one
  if ( m_pVisControl && m_bgMode == BACKGROUND_VISUALISATION && m_pVisControl->OnAction(action) )
      return true;

  return CGUIWindow::OnAction(action);
}


bool CGUIWindowKaraokeLyrics::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      if ( !CGUIWindow::OnMessage(message) )
        return false;

      if ( m_Lyrics )
        m_Lyrics->InitGraphics();

      // Set up current visualisation mode
      if ( m_Lyrics->HasBackground() || g_guiSettings.GetString("mymusic.visualisation").Equals("None") )
        m_bgMode = BACKGROUND_NONE;
      else
        m_bgMode = BACKGROUND_VISUALISATION;

      m_pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_KARAVIS);

      if ( m_pVisControl )
      {
        bool enabled = ( m_bgMode == BACKGROUND_VISUALISATION );
        m_pVisControl->SetVisible( enabled );
        m_pVisControl->SetEnabled( enabled );
      }
      else
        CLog::Log( LOGERROR, "Cannot find visualization control" );

      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_CritSection);

      // Close the song selector dialog if shown
      CGUIDialogKaraokeSongSelectorSmall * songSelector = (CGUIDialogKaraokeSongSelectorSmall *)
                                      m_gWindowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

      if ( songSelector && songSelector->IsActive() )
        songSelector->Close();
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
	  if ( m_pVisControl && m_bgMode == BACKGROUND_VISUALISATION )
        return m_pVisControl->OnMessage(message);
    break;

  case GUI_MSG_GET_VISUALISATION:
	  if ( m_pVisControl && m_bgMode == BACKGROUND_VISUALISATION )
        message.SetLPVOID( m_pVisControl->GetVisualisation() );
    break;

  case GUI_MSG_VISUALISATION_ACTION:
	  if ( m_pVisControl && m_bgMode == BACKGROUND_VISUALISATION )
        return m_pVisControl->OnMessage(message);
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
    m_Lyrics->Render();
}


void CGUIWindowKaraokeLyrics::setLyrics(CKaraokeLyrics * lyrics)
{
  CSingleLock lock (m_CritSection);
  m_Lyrics = lyrics;
}
