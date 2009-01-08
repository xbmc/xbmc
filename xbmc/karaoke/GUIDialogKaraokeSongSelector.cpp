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
#include "GUIDialogKaraokeSongSelector.h"
#include "Application.h"
#include "PlayList.h"
#include "MusicDatabase.h"

#define CONTROL_LABEL_SONGNUMBER    401
#define CONTROL_LABEL_SONGNAME      402

static const unsigned int INACTIVITY_TIME = 5000;  // 5 secs
static const unsigned int MAX_SONG_ID = 100000;


CGUIDialogKaraokeSongSelector::CGUIDialogKaraokeSongSelector( DWORD dwID, const char *xmlFile )
  : CGUIDialog( dwID, xmlFile  )
{
  m_selectedNumber = 0;
  m_songSelected = false;
  m_updateData = false;
}

CGUIDialogKaraokeSongSelector::~CGUIDialogKaraokeSongSelector(void)
{
}

void CGUIDialogKaraokeSongSelector::OnButtonNumeric( unsigned int code )
{
  // Add the number
  m_selectedNumber = m_selectedNumber * 10 + code;

  // If overflow (a typical way to delete the old number is add zeros), handle it
  if ( m_selectedNumber >= MAX_SONG_ID )
    m_selectedNumber %= MAX_SONG_ID;

  // Reset activity timer
  SetAutoClose( INACTIVITY_TIME );
  m_updateData = true;
}

void CGUIDialogKaraokeSongSelector::OnButtonSelect()
{
  // We only handle "select" if a song is selected
  if ( m_songSelected )
  {
    CStdString path = m_karaokeSong.strFileName;
    CFileItemPtr pItem( new CFileItem( path, false) );
    m_songSelected = false;

    if ( m_startPlaying )
    {
      g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
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

void CGUIDialogKaraokeSongSelector::init(unsigned int startcode)
{
  m_songSelected = false;
  m_selectedNumber = 0;

  OnButtonNumeric( startcode );
}

bool CGUIDialogKaraokeSongSelector::OnAction(const CAction & action)
{
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
      OnButtonNumeric( action.wID - REMOTE_0 );
      return true;

    case ACTION_SELECT_ITEM:
      OnButtonSelect();
      break;
  }

  return CGUIDialog::OnAction( action );
}

void CGUIDialogKaraokeSongSelector::UpdateData()
{
  if ( m_updateData )
  {
    // Update on-screen labels
    CStdString message;
    message.Format( "%06d", m_selectedNumber );
    message = g_localizeStrings.Get(179) + ": " + message; // Translated "Song"

    SET_CONTROL_LABEL(CONTROL_LABEL_SONGNUMBER, message);

    // Now try to find this song in the database
    CMusicDatabase musicdatabase;
    if ( musicdatabase.Open() )
    {
      m_songSelected = musicdatabase.GetSongByKaraokeNumber( m_selectedNumber, m_karaokeSong );
      musicdatabase.Close();

      if ( m_songSelected )
        message = m_karaokeSong.strTitle;
      else
        message = "* " + g_localizeStrings.Get(13205) + " *"; // Unknown
    }
    else
    {
      m_songSelected = false;
      message = "* " + g_localizeStrings.Get(315) + " *"; // Database error
    }

    SET_CONTROL_LABEL(CONTROL_LABEL_SONGNAME, message);
  }

  m_updateData = false;
}

void CGUIDialogKaraokeSongSelector::Render()
{
  if ( m_updateData )
    UpdateData();

  CGUIDialog::Render();
}

CGUIDialogKaraokeSongSelectorSmall::CGUIDialogKaraokeSongSelectorSmall()
  : CGUIDialogKaraokeSongSelector( WINDOW_DIALOG_KARAOKE_SONGSELECT, "DialogKaraokeSongSelector.xml" )
{
  m_autoCloseTimeout = 5000;  // 5 sec
  m_startPlaying = false;
}

CGUIDialogKaraokeSongSelectorLarge::CGUIDialogKaraokeSongSelectorLarge()
  : CGUIDialogKaraokeSongSelector( WINDOW_DIALOG_KARAOKE_SELECTOR, "DialogKaraokeSongSelectorLarge.xml" )
{
  m_autoCloseTimeout = 60000; // 60 sec
  m_startPlaying = true;
}
