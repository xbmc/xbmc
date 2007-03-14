/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowMusicPlayList.h"
#include "util.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIDialogContextMenu.h"
#include "PartyModeManager.h"

using namespace PLAYLIST;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12

#define CONTROL_BTNSHUFFLE    20
#define CONTROL_BTNSAVE      21
#define CONTROL_BTNCLEAR     22

#define CONTROL_BTNPLAY      23
#define CONTROL_BTNNEXT      24
#define CONTROL_BTNPREVIOUS    25
#define CONTROL_BTNREPEAT     26

CGUIWindowMusicPlayList::CGUIWindowMusicPlayList(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_PLAYLIST, "MyMusicPlaylist.xml")
{
  m_musicInfoLoader.SetObserver(this);
  m_musicInfoLoader.SetPriority(THREAD_PRIORITY_LOWEST);
  iPos = -1;
}

CGUIWindowMusicPlayList::~CGUIWindowMusicPlayList(void)
{
}

bool CGUIWindowMusicPlayList::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYLISTPLAYER_REPEAT:
    {
      UpdateButtons();
    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_RANDOM:
  case GUI_MSG_PLAYLIST_CHANGED:
    {
      // global playlist changed outside playlist window
      UpdateButtons();
      Update(m_vecItems.m_strPath);

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems.Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_musicInfoLoader.IsLoading())
        m_musicInfoLoader.StopThread();

      iPos = -1;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      // Setup item cache for tagloader
      m_musicInfoLoader.UseCacheOnHD("Z:\\MusicPlaylist.fi");

      m_vecItems.m_strPath="playlistmusic://";

      // updatebuttons is called in here
      if (!CGUIWindowMusicBase::OnMessage(message))
        return false;

      if (m_vecItems.Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        int iSong = g_playlistPlayer.GetCurrentSong();
        if (iSong >= 0 && iSong <= m_vecItems.Size())
          m_viewControl.SetSelectedItem(iSong);
      }

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSHUFFLE)
      {
        if (!g_partyModeManager.IsEnabled())
        {
          g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, !(g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC)));
          g_stSettings.m_bMyMusicPlaylistShuffle = g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC);
          g_settings.Save();
          UpdateButtons();
          Update(m_vecItems.m_strPath);
        }
      }
      else if (iControl == CONTROL_BTNSAVE)
      {
        if (m_musicInfoLoader.IsLoading()) // needed since we destroy m_vecitems to save memory
          m_musicInfoLoader.StopThread();

        SavePlayList();
      }
      else if (iControl == CONTROL_BTNCLEAR)
      {
        if (m_musicInfoLoader.IsLoading())
          m_musicInfoLoader.StopThread();

        ClearPlayList();
      }
      else if (iControl == CONTROL_BTNPLAY)
      {
        m_guiState->SetPlaylistDirectory("playlistmusic://");
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Play(m_viewControl.GetSelectedItem());
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        // increment repeat state
        PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC);
        if (state == PLAYLIST::REPEAT_NONE)
          g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_ALL);
        else if (state == PLAYLIST::REPEAT_ALL)
          g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_ONE);
        else
          g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);

        // save settings
        g_stSettings.m_bMyMusicPlaylistRepeat = g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC) == PLAYLIST::REPEAT_ALL;
        g_settings.Save();

        UpdateButtons();
      }
      else if (m_viewControl.HasControl(iControl))
      {
        int iAction = message.GetParam1();
        int iItem = m_viewControl.GetSelectedItem();
        if (iAction == ACTION_DELETE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          RemovePlayListItem(iItem);
          MarkPlaying();
        }
      }
    }
    break;

  }
  return CGUIWindowMusicBase::OnMessage(message);
}

bool CGUIWindowMusicPlayList::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    // Playlist has no parent dirs
    return true;
  }
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    m_gWindowManager.ChangeActiveWindow(WINDOW_MUSIC);
    return true;
  }
  if ((action.wID == ACTION_MOVE_ITEM_UP) || (action.wID == ACTION_MOVE_ITEM_DOWN))
  {
    int iItem = -1;
    int iFocusedControl = GetFocusedControlID();
    if (m_viewControl.HasControl(iFocusedControl))
      iItem = m_viewControl.GetSelectedItem();
    OnMove(iItem, action.wID);
    return true;
  }
  return CGUIWindowMusicBase::OnAction(action);
}

bool CGUIWindowMusicPlayList::MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate /* = true */)
{
  int iSelected = iItem;
  int iNew = iSelected;
  if (iAction == ACTION_MOVE_ITEM_UP)
    iNew--;
  else
    iNew++;

  // is the currently playing item affected?
  bool bFixCurrentSong = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC) && (g_application.IsPlayingAudio()) &&
    ((g_playlistPlayer.GetCurrentSong() == iSelected) || (g_playlistPlayer.GetCurrentSong() == iNew)))
    bFixCurrentSong = true;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  if (playlist.Swap(iSelected, iNew))
  {
    // Correct the current playing song in playlistplayer
    if (bFixCurrentSong)
    {
      int iCurrentSong = g_playlistPlayer.GetCurrentSong();
      if (iSelected == iCurrentSong)
        iCurrentSong = iNew;
      else if (iNew == iCurrentSong)
        iCurrentSong = iSelected;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
    }

    if (bUpdate)
      Update(m_vecItems.m_strPath);
    return true;
  }

  return false;
}

void CGUIWindowMusicPlayList::SavePlayList()
{
  CStdString strNewFileName;
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFileName, g_localizeStrings.Get(16012), false))
  {
    // need 2 rename it
    CStdString strFolder, strPath;
    strFolder = CUtil::MusicPlaylistsLocation();
    CUtil::RemoveIllegalChars( strNewFileName );
    strNewFileName += ".m3u";
    CUtil::AddFileToFolder(strFolder, strNewFileName, strPath);

    // get selected item
    int iItem = m_viewControl.GetSelectedItem();
    CStdString strSelectedItem = "";
    if (iItem >= 0 && iItem < m_vecItems.Size())
    {
      CFileItem* pItem = m_vecItems[iItem];
      if (!pItem->IsParentFolder())
      {
        GetDirectoryHistoryString(pItem, strSelectedItem);
      }
    }

    CStdString strOldDirectory = m_vecItems.m_strPath;
    m_history.SetSelectedItem(strSelectedItem, strOldDirectory);

    CPlayListM3U playlist;
    for (int i = 0; i < (int)m_vecItems.Size(); ++i)
    {
      CFileItem* pItem = m_vecItems[i];
      CPlayList::CPlayListItem newItem;
      newItem.SetFileName(pItem->m_strPath);
      newItem.SetDescription(pItem->GetLabel());
      if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->GetDuration())
        newItem.SetDuration(pItem->GetMusicInfoTag()->GetDuration());
      else
        newItem.SetDuration(0);

      //  Musicdatabase items should contain the real path instead of a musicdb url
      //  otherwise the user can't save and reuse the playlist when the musicdb gets deleted
      if (pItem->IsMusicDb())
        newItem.m_strPath=pItem->GetMusicInfoTag()->GetURL();

      playlist.Add(newItem);
      m_vecItems.Remove(i--);
    }
    CLog::Log(LOGDEBUG, "Saving music playlist: [%s]", strPath.c_str());
    playlist.Save(strPath);
    Update(m_vecItems.m_strPath); // need to update
  }
}

void CGUIWindowMusicPlayList::ClearPlayList()
{
  ClearFileItems();
  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
  {
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }
  Update(m_vecItems.m_strPath);
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowMusicPlayList::RemovePlayListItem(int iItem)
{
  if (iItem < 0 || iItem > m_vecItems.Size()) return;

  // The current playing song can't be removed
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC && g_application.IsPlayingAudio()
      && g_playlistPlayer.GetCurrentSong() == iItem)
    return ;

  g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(iItem);

  // Correct the current playing song in playlistplayer
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC && g_application.IsPlayingAudio())
  {
    int iCurrentSong = g_playlistPlayer.GetCurrentSong();
    if (iItem <= iCurrentSong)
    {
      iCurrentSong--;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
    }
  }

  Update(m_vecItems.m_strPath);

  if (m_vecItems.Size() <= 0)
  {
    SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
  }
  else
  {
    m_viewControl.SetSelectedItem(iItem);
  }

  g_partyModeManager.OnSongChange();
}

void CGUIWindowMusicPlayList::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // Update playlist buttons
  if (m_vecItems.Size() && !g_partyModeManager.IsEnabled())
  {
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);
    CONTROL_ENABLE(CONTROL_BTNPLAY);

    if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
    {
      CONTROL_ENABLE(CONTROL_BTNNEXT);
      CONTROL_ENABLE(CONTROL_BTNPREVIOUS);
    }
    else
    {
      CONTROL_DISABLE(CONTROL_BTNNEXT);
      CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    }

    // disable repeat options if clear on end is enabled
    if (g_guiSettings.GetBool("mymusic.clearplaylistsonend"))
    {
      g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);
      CONTROL_DISABLE(CONTROL_BTNREPEAT);
    }
  }
  else
  {
    // disable buttons if party mode is enabled too
    CONTROL_DISABLE(CONTROL_BTNSHUFFLE);
    CONTROL_DISABLE(CONTROL_BTNSAVE);
    CONTROL_DISABLE(CONTROL_BTNCLEAR);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
  }

  // update buttons
  CONTROL_DESELECT(CONTROL_BTNSHUFFLE);
  if (g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC))
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);

  // update repeat button
  int iRepeat = 595 + g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC);
  SET_CONTROL_LABEL(CONTROL_BTNREPEAT, g_localizeStrings.Get(iRepeat));

  // Update object count label
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  MarkPlaying();
}

bool CGUIWindowMusicPlayList::OnPlayMedia(int iItem)
{
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Play(iItem);
  else
  {
    int iPlaylist=m_guiState->GetPlaylist();
    if (iPlaylist!=PLAYLIST_NONE)
    {
      if (m_guiState.get())
        m_guiState->SetPlaylistDirectory(m_vecItems.m_strPath);

      g_playlistPlayer.SetCurrentPlaylist( iPlaylist );
      g_playlistPlayer.Play( iItem );
    }
    else
    {
      // Reset Playlistplayer, playback started now does
      // not use the playlistplayer.
      CFileItem* pItem=m_vecItems[iItem];
      g_playlistPlayer.Reset();
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
      g_application.PlayFile(*pItem);
    }
  }

  return true;
}

void CGUIWindowMusicPlayList::OnItemLoaded(CFileItem* pItem)
{
  if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded())
  { // set label 1+2 from tags
    if (m_guiState.get()) m_hideExtensions = m_guiState->HideExtensions();
    CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.nowplayingtrackformat");
    if (strTrackLeft.IsEmpty())
      strTrackLeft = g_guiSettings.GetString("musicfiles.trackformat");
    CStdString strTrackRight=g_guiSettings.GetString("musicfiles.nowplayingtrackformatright");
    if (strTrackRight.IsEmpty())
      strTrackRight = g_guiSettings.GetString("musicfiles.trackformatright");
    pItem->FormatLabel(strTrackLeft);
    pItem->FormatLabel2(strTrackRight);
  } // if (pItem->m_musicInfoTag.Loaded())
  else
  {
    // Our tag may have a duration even if its not loaded
    if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->GetDuration())
    {
      int nDuration = pItem->GetMusicInfoTag()->GetDuration();
      if (nDuration > 0)
      {
        CStdString str;
        StringUtils::SecondsToTimeString(nDuration, str);
        pItem->SetLabel2(str);
      }
    }
    else if (pItem->GetLabel() == "") // pls labels come in preformatted
    {
      // FIXME: get the position of the item in the playlist
      //        currently it is hacked into m_iprogramCount

      // No music info and it's not CDDA so we'll just show the filename
      CStdString str;
      str = CUtil::GetTitleFromPath(pItem->m_strPath);
      str.Format("%02.2i. %s ", pItem->m_iprogramCount, str);
      pItem->SetLabel(str);
    }
  }

  if (m_guiState.get())
  {
    CPlayList& playlist=g_playlistPlayer.GetPlaylist(m_guiState->GetPlaylist());
    CPlayList::CPlayListItem& item=playlist[pItem->m_iprogramCount];
    if (item.m_strPath==pItem->m_strPath &&
        item.m_lStartOffset==pItem->m_lStartOffset &&
        item.m_lEndOffset==pItem->m_lEndOffset)
    {
      item.SetDescription(pItem->GetLabel());
    }
    else
    { // for some reason the order is wrong - do it the incredibly slow way
      // FIXME: Highly inefficient. :)
      // Since we can't directly use the items
      // of the playlistplayer, we need to set each
      // label of the playlist items or else the label
      // is reset to the filename each time Update()
      // is called and this is annoying. ;)
      for (int i=0; i<playlist.size(); ++i)
      {
        CPlayList::CPlayListItem& item=playlist[i];

        if (item.m_strPath==pItem->m_strPath &&
            item.m_lStartOffset==pItem->m_lStartOffset &&
            item.m_lEndOffset==pItem->m_lEndOffset)
        {
          item.SetDescription(pItem->GetLabel());
          break;
        }
      }
    }
  }

  //  MusicDb items already have thumbs
  if (!pItem->IsMusicDb())
  {
    // Reset thumbs and default icons
    pItem->SetMusicThumb();
    pItem->FillInDefaultIcon();
  }
}

bool CGUIWindowMusicPlayList::Update(const CStdString& strDirectory)
{
  if (m_musicInfoLoader.IsLoading())
    m_musicInfoLoader.StopThread();

  if (!CGUIWindowMusicBase::Update(strDirectory))
    return false;

  m_musicInfoLoader.Load(m_vecItems);



  return true;
}

void CGUIWindowMusicPlayList::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  // calculate our position
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // mark the item
  m_vecItems[iItem]->Select(true);
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();

  // is this playlist playing?
  int iPlaying = g_playlistPlayer.GetCurrentSong();
  bool bIsPlaying = false;
  bool bItemIsPlaying = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC) && (g_application.IsPlayingAudio()))
  {
    bIsPlaying = true;
    if (iItem == iPlaying) bItemIsPlaying = true;
  }

  // add the buttons
  int btn_Move = 0;   // move item
  int btn_MoveTo = 0; // move item here
  int btn_Cancel = 0; // cancel move
  int btn_MoveUp = 0; // move up
  int btn_MoveDn = 0; // move down
  int btn_Delete = 0; // delete
  int btn_PartyMode = 0; // cancel party mode

  if (iPos < 0)
  {
    btn_Move = pMenu->AddButton(13251);       // move item
    btn_MoveUp = pMenu->AddButton(13332);     // move up
    if (iItem == 0)
      pMenu->EnableButton(btn_MoveUp, false); // disable if top item
    btn_MoveDn = pMenu->AddButton(13333);     // move down
    if (iItem == (m_vecItems.Size()-1))
      pMenu->EnableButton(btn_MoveDn, false); // disable if bottom item
    btn_Delete = pMenu->AddButton(15015);     // delete
    if (bItemIsPlaying)
      pMenu->EnableButton(btn_Delete, false); // disable if current item

    // party mode automatically moves the current song to the top
    if (g_partyModeManager.IsEnabled())
    {
      // cant move the current playing song
      if (bItemIsPlaying)
      {
        pMenu->EnableButton(btn_Move, false);
        pMenu->EnableButton(btn_MoveUp, false);
        pMenu->EnableButton(btn_MoveDn, false);
      }
      if (iItem == (iPlaying-1))
        pMenu->EnableButton(btn_MoveDn, false);
      if (iItem == (iPlaying+1))
        pMenu->EnableButton(btn_MoveUp, false);
    }
  }
  // after selecting "move item" only two choices
  else
  {
    btn_MoveTo = pMenu->AddButton(13252);         // move item here
    if (iItem == iPos)
      pMenu->EnableButton(btn_MoveTo, false);     // disable the button if its the same position or current item
    if (g_partyModeManager.IsEnabled() && iItem <= iPlaying)
      pMenu->EnableButton(btn_MoveTo, false);     // cant move a song above the currently playing
    btn_Cancel = pMenu->AddButton(13253);         // cancel move
  }
  if (g_partyModeManager.IsEnabled())
    btn_PartyMode = pMenu->AddButton(588);      // cancel party mode

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid > 0)
  {
    // move item
    if (btnid == btn_Move)
    {
      iPos = iItem;
    }
    // move item here
    else if (btnid == btn_MoveTo && iPos >= 0)
    {
      MoveItem(iPos, iItem);
      iPos = -1;
    }
    // cancel move
    else if (btnid == btn_Cancel)
    {
      iPos = -1;
    }
    // move up
    else if (btnid == btn_MoveUp)
    {
      OnMove(iItem, ACTION_MOVE_ITEM_UP);
    }
    // move down
    else if (btnid == btn_MoveDn)
    {
      OnMove(iItem, ACTION_MOVE_ITEM_DOWN);
    }
    // delete
    else if (btnid == btn_Delete)
    {
      RemovePlayListItem(iItem);
      return;
    }
    else if (btnid == btn_PartyMode)
    {
      g_partyModeManager.Disable();
    }
  }
  MarkPlaying();
}

void CGUIWindowMusicPlayList::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return;

  bool bRestart = m_musicInfoLoader.IsLoading();
  if (bRestart)
    m_musicInfoLoader.StopThread();

  MoveCurrentPlayListItem(iItem, iAction);

  if (bRestart)
    m_musicInfoLoader.Load(m_vecItems);
}

void CGUIWindowMusicPlayList::MoveItem(int iStart, int iDest)
{
  if (iStart < 0 || iStart >= m_vecItems.Size()) return;
  if (iDest < 0 || iDest >= m_vecItems.Size()) return;

  // default to move up
  int iAction = ACTION_MOVE_ITEM_UP;
  int iDirection = -1;
  // are we moving down?
  if (iStart < iDest)
  {
    iAction = ACTION_MOVE_ITEM_DOWN;
    iDirection = 1;
  }

  bool bRestart = m_musicInfoLoader.IsLoading();
  if (bRestart)
    m_musicInfoLoader.StopThread();

  // keep swapping until you get to the destination or you
  // hit the currently playing song
  int i = iStart;
  while (i != iDest)
  {
    // try to swap adjacent items
    if (MoveCurrentPlayListItem(i, iAction, false))
      i = i + (1 * iDirection);
    // we hit currently playing song, so abort
    else
      break;
  }
  Update(m_vecItems.m_strPath);

  if (bRestart)
    m_musicInfoLoader.Load(m_vecItems);
}

void CGUIWindowMusicPlayList::MarkPlaying()
{
/*  // clear markings
  for (int i = 0; i < m_vecItems.Size(); i++)
    m_vecItems[i]->Select(false);

  // mark the currently playing item
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC) && (g_application.IsPlayingAudio()))
  {
    int iSong = g_playlistPlayer.GetCurrentSong();
    if (iSong >= 0 && iSong <= m_vecItems.Size())
      m_vecItems[iSong]->Select(true);
  }*/
}