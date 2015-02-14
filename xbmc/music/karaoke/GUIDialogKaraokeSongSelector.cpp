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

#include "GUIDialogKaraokeSongSelector.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#define CONTROL_LABEL_SONGNUMBER    401
#define CONTROL_LABEL_SONGNAME      402

static const unsigned int MAX_SONG_ID = 100000;


CGUIDialogKaraokeSongSelector::CGUIDialogKaraokeSongSelector( int id, const char *xmlFile )
  : CGUIDialog( id, xmlFile  )
{
  m_selectedNumber = 0;
  m_songSelected = false;
  m_updateData = false;
}

CGUIDialogKaraokeSongSelector::~CGUIDialogKaraokeSongSelector(void)
{
}

void CGUIDialogKaraokeSongSelector::OnButtonNumeric( unsigned int code, bool reset_autotimer )
{
  
  // Add the number
  m_selectedNumber = m_selectedNumber * 10 + code;
  CLog::Log( LOGDEBUG, "CGUIDialogKaraokeSongSelector::OnButtonNumeric %d / %d" , code, m_selectedNumber);

  // If overflow (a typical way to delete the old number is add zeros), handle it
  if ( m_selectedNumber >= MAX_SONG_ID )
    m_selectedNumber %= MAX_SONG_ID;

  // Reset activity timer
  if ( reset_autotimer )
    SetAutoClose( m_autoCloseTimeout );

  m_updateData = true;
}

void CGUIDialogKaraokeSongSelector::OnButtonSelect()
{
  // We only handle "select" if a song is selected
  if ( m_songSelected )
  {
    std::string path = m_karaokeSong.strFileName;
    CFileItemPtr pItem( new CFileItem( path, false) );
    m_songSelected = false;

    if ( m_startPlaying )
    {
      g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
      g_playlistPlayer.SetRepeat( PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE );
      g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, false );
      g_playlistPlayer.Add( PLAYLIST_MUSIC, pItem );
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
      g_playlistPlayer.Play();

      CLog::Log(LOGDEBUG, "Karaoke song selector: playing song %s [%d]", path.c_str(), m_selectedNumber);
    }
    else
    {
      g_playlistPlayer.Add( PLAYLIST_MUSIC, pItem );
      CLog::Log(LOGDEBUG, "Karaoke song selector: adding song %s [%d]", path.c_str(), m_selectedNumber);
    }

    Close();
  }
}

int CGUIDialogKaraokeSongSelector::GetKeyNumber( int actionid )
{
  switch( actionid )
  {
    case REMOTE_0:
      return 0;

    case REMOTE_1:
      return 1;

    case REMOTE_2:
    case ACTION_JUMP_SMS2:
      return 2;

    case REMOTE_3:
    case ACTION_JUMP_SMS3:
      return 3;

    case REMOTE_4:
    case ACTION_JUMP_SMS4:
      return 4;

    case REMOTE_5:
    case ACTION_JUMP_SMS5:
      return 5;

    case REMOTE_6:
    case ACTION_JUMP_SMS6:
      return 6;

    case REMOTE_7:
    case ACTION_JUMP_SMS7:
      return 7;

    case REMOTE_8:
    case ACTION_JUMP_SMS8:
      return 8;

    case REMOTE_9:
    case ACTION_JUMP_SMS9:
      return 9;
  }
  
  return -1;
}

bool CGUIDialogKaraokeSongSelector::OnAction(const CAction & action)
{
  CLog::Log( LOGDEBUG, "CGUIDialogKaraokeSongSelector::OnAction %d" , action.GetID());
  
  if ( GetKeyNumber( action.GetID() ) != -1 )
  {
    OnButtonNumeric( GetKeyNumber( action.GetID() ) );
    return true;
  }
  
  switch(action.GetID())
  {
    case ACTION_SELECT_ITEM:
      OnButtonSelect();
      break;

    case ACTION_DELETE_ITEM:
    case ACTION_BACKSPACE:
      OnBackspace();
      break;
  }

  return CGUIDialog::OnAction( action );
}

void CGUIDialogKaraokeSongSelector::UpdateData()
{
  if ( m_updateData )
  {
    // Update on-screen labels
    std::string message = StringUtils::Format("%06d", m_selectedNumber);
    message = g_localizeStrings.Get(179) + ": " + message; // Translated "Song"

    SET_CONTROL_LABEL(CONTROL_LABEL_SONGNUMBER, message);

    // Now try to find this song in the database
    m_songSelected = m_musicdatabase.GetSongByKaraokeNumber( m_selectedNumber, m_karaokeSong );

    if ( m_songSelected )
      message = m_karaokeSong.strTitle;
    else
      message = "* " + g_localizeStrings.Get(13205) + " *"; // Unknown

    SET_CONTROL_LABEL(CONTROL_LABEL_SONGNAME, message);
  }

  m_updateData = false;
}

void CGUIDialogKaraokeSongSelector::FrameMove()
{
  if ( m_updateData )
    UpdateData();

  CGUIDialog::FrameMove();
}


void CGUIDialogKaraokeSongSelector::OnBackspace()
{
  // Clear the number
  m_selectedNumber /= 10;

  // Reset activity timer
  SetAutoClose( m_autoCloseTimeout );
  m_updateData = true;
}


CGUIDialogKaraokeSongSelectorSmall::CGUIDialogKaraokeSongSelectorSmall()
  : CGUIDialogKaraokeSongSelector( WINDOW_DIALOG_KARAOKE_SONGSELECT, "DialogKaraokeSongSelector.xml" )
{
  m_autoCloseTimeout = 30000;  // 30 sec
  m_startPlaying = false;
}


CGUIDialogKaraokeSongSelectorLarge::CGUIDialogKaraokeSongSelectorLarge()
  : CGUIDialogKaraokeSongSelector( WINDOW_DIALOG_KARAOKE_SELECTOR, "DialogKaraokeSongSelectorLarge.xml" )
{
  m_autoCloseTimeout = 180000; // 180 sec
  m_startPlaying = true;
}

void CGUIDialogKaraokeSongSelector::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Check if there are any karaoke songs in the database
  if ( !m_musicdatabase.Open() )
  {
    Close();
    return;
  }

  if ( m_musicdatabase.GetKaraokeSongsCount() == 0 )
  {
    Close();
    return;
  }

  SetAutoClose( m_autoCloseTimeout );
}


void CGUIDialogKaraokeSongSelector::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  m_musicdatabase.Close();
}


void CGUIDialogKaraokeSongSelectorSmall::DoModal(unsigned int startcode, int iWindowID, const std::string & param)
{
  m_songSelected = false;
  m_selectedNumber = 0;

  OnButtonNumeric( startcode, false );
  CGUIDialog::DoModal( iWindowID, param );
}


void CGUIDialogKaraokeSongSelectorLarge::DoModal(int iWindowID, const std::string & param)
{
  m_songSelected = false;
  m_selectedNumber = 0;

  OnButtonNumeric( 0, false );
  CGUIDialog::DoModal( iWindowID, param );
}
