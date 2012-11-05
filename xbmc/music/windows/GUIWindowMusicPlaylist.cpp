/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowMusicPlaylist.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "Util.h"
#include "playlists/PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "PartyModeManager.h"
#include "music/LastFmManager.h"
#include "utils/LabelFormatter.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "GUIUserMessages.h"
#include "Favourites.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace PLAYLIST;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LABELFILES        12

#define CONTROL_BTNSHUFFLE        20
#define CONTROL_BTNSAVE           21
#define CONTROL_BTNCLEAR          22

#define CONTROL_BTNPLAY           23
#define CONTROL_BTNNEXT           24
#define CONTROL_BTNPREVIOUS       25
#define CONTROL_BTNREPEAT         26

CGUIWindowMusicPlayList::CGUIWindowMusicPlayList(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_PLAYLIST, "MyMusicPlaylist.xml")
{
  m_musicInfoLoader.SetObserver(this);
  m_movingFrom = -1;
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
      Refresh(true);

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems->Size() <= 0)
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

      m_movingFrom = -1;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      // Setup item cache for tagloader
      m_musicInfoLoader.UseCacheOnHD("special://temp/MusicPlaylist.fi");

      m_vecItems->SetPath("playlistmusic://");

      // updatebuttons is called in here
      if (!CGUIWindowMusicBase::OnMessage(message))
        return false;

      if (m_vecItems->Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        int iSong = g_playlistPlayer.GetCurrentSong();
        if (iSong >= 0 && iSong <= m_vecItems->Size())
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
          g_settings.m_bMyMusicPlaylistShuffle = g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC);
          g_settings.Save();
          UpdateButtons();
          Refresh();
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
        g_settings.m_bMyMusicPlaylistRepeat = g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC) == PLAYLIST::REPEAT_ALL;
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
  if (action.GetID() == ACTION_PARENT_DIR)
  {
    // Playlist has no parent dirs
    return true;
  }
  if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  if ((action.GetID() == ACTION_MOVE_ITEM_UP) || (action.GetID() == ACTION_MOVE_ITEM_DOWN))
  {
    int iItem = -1;
    int iFocusedControl = GetFocusedControlID();
    if (m_viewControl.HasControl(iFocusedControl))
      iItem = m_viewControl.GetSelectedItem();
    OnMove(iItem, action.GetID());
    return true;
  }
  return CGUIWindowMusicBase::OnAction(action);
}

bool CGUIWindowMusicPlayList::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
    return CGUIWindow::OnBack(actionID); // base class goes up a folder, but none to go up
  return CGUIWindowMusicBase::OnBack(actionID);
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
      Refresh();
    return true;
  }

  return false;
}

void CGUIWindowMusicPlayList::SavePlayList()
{
  CStdString strNewFileName;
  if (CGUIKeyboardFactory::ShowAndGetInput(strNewFileName, g_localizeStrings.Get(16012), false))
  {
    // need 2 rename it
    CStdString strFolder, strPath;
    URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strFolder);
    strNewFileName= CUtil::MakeLegalFileName( strNewFileName );
    strNewFileName += ".m3u";
    URIUtils::AddFileToFolder(strFolder, strNewFileName, strPath);

    // get selected item
    int iItem = m_viewControl.GetSelectedItem();
    CStdString strSelectedItem = "";
    if (iItem >= 0 && iItem < m_vecItems->Size())
    {
      CFileItemPtr pItem = m_vecItems->Get(iItem);
      if (!pItem->IsParentFolder())
      {
        GetDirectoryHistoryString(pItem.get(), strSelectedItem);
      }
    }

    CStdString strOldDirectory = m_vecItems->GetPath();
    m_history.SetSelectedItem(strSelectedItem, strOldDirectory);

    CPlayListM3U playlist;
    for (int i = 0; i < (int)m_vecItems->Size(); ++i)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);

      //  Musicdatabase items should contain the real path instead of a musicdb url
      //  otherwise the user can't save and reuse the playlist when the musicdb gets deleted
      if (pItem->IsMusicDb())
        pItem->SetPath(pItem->GetMusicInfoTag()->GetURL());

      playlist.Add(pItem);
    }
    CLog::Log(LOGDEBUG, "Saving music playlist: [%s]", strPath.c_str());
    playlist.Save(strPath);
    Refresh(); // need to update
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
  Refresh();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowMusicPlayList::RemovePlayListItem(int iItem)
{
  if (iItem < 0 || iItem > m_vecItems->Size()) return;

  // The current playing song can't be removed
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC && g_application.IsPlayingAudio()
      && g_playlistPlayer.GetCurrentSong() == iItem)
    return ;

  g_playlistPlayer.Remove(PLAYLIST_MUSIC, iItem);

  Refresh();

  if (m_vecItems->Size() <= 0)
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
  if (m_vecItems->Size() && !g_partyModeManager.IsEnabled() && !CLastFmManager::GetInstance()->IsRadioEnabled())
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
  CStdString items;
  items.Format("%i %s", m_vecItems->GetObjectCount(), g_localizeStrings.Get(127).c_str());
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
        m_guiState->SetPlaylistDirectory(m_vecItems->GetPath());

      g_playlistPlayer.SetCurrentPlaylist( iPlaylist );
      g_playlistPlayer.Play( iItem );
    }
    else
    {
      // Reset Playlistplayer, playback started now does
      // not use the playlistplayer.
      CFileItemPtr pItem=m_vecItems->Get(iItem);
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
    CLabelFormatter formatter(strTrackLeft, strTrackRight);
    formatter.FormatLabels(pItem);
  } // if (pItem->m_musicInfoTag.Loaded())
  else
  {
    // Our tag may have a duration even if its not loaded
    if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->GetDuration())
    {
      int nDuration = pItem->GetMusicInfoTag()->GetDuration();
      if (nDuration > 0)
        pItem->SetLabel2(StringUtils::SecondsToTimeString(nDuration));
    }
    else if (pItem->GetLabel() == "") // pls labels come in preformatted
    {
      // FIXME: get the position of the item in the playlist
      //        currently it is hacked into m_iprogramCount

      // No music info and it's not CDDA so we'll just show the filename
      CStdString str;
      str = CUtil::GetTitleFromPath(pItem->GetPath());
      str.Format("%02.2i. %s ", pItem->m_iprogramCount, str);
      pItem->SetLabel(str);
    }
  }
}

bool CGUIWindowMusicPlayList::Update(const CStdString& strDirectory, bool updateFilterPath /* = true */)
{
  if (m_musicInfoLoader.IsLoading())
    m_musicInfoLoader.StopThread();

  if (!CGUIWindowMusicBase::Update(strDirectory, updateFilterPath))
    return false;

  if (m_vecItems->GetContent().IsEmpty())
    m_vecItems->SetContent("songs");

  m_musicInfoLoader.Load(*m_vecItems);
  return true;
}

void CGUIWindowMusicPlayList::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  // is this playlist playing?
  int itemPlaying = g_playlistPlayer.GetCurrentSong();

  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
  {
    CFileItemPtr item;
    item = m_vecItems->Get(itemNumber);

    if (m_movingFrom >= 0)
    {
      // we can move the item to any position not where we are, and any position not above currently
      // playing item in party mode
      if (itemNumber != m_movingFrom && (!g_partyModeManager.IsEnabled() || itemNumber > itemPlaying))
        buttons.Add(CONTEXT_BUTTON_MOVE_HERE, 13252);         // move item here
      buttons.Add(CONTEXT_BUTTON_CANCEL_MOVE, 13253);
    }
    else
    { // aren't in a move
      // check what players we have, if we have multiple display play with option
      VECPLAYERCORES vecCores;
      CPlayerCoreFactory::GetPlayers(*item, vecCores);
      if (vecCores.size() > 1)
        buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213); // Play With...

      if (!item->IsLastFM())
        buttons.Add(CONTEXT_BUTTON_SONG_INFO, 658); // Song Info
      if (CFavourites::IsFavourite(item.get(), GetID()))
        buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14077);     // Remove Favourite
      else
        buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14076);     // Add To Favourites;
      if (itemNumber > (g_partyModeManager.IsEnabled() ? 1 : 0))
        buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_UP, 13332);
      if (itemNumber + 1 < m_vecItems->Size())
        buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_DOWN, 13333);
      if (!g_partyModeManager.IsEnabled() || itemNumber != itemPlaying)
        buttons.Add(CONTEXT_BUTTON_MOVE_ITEM, 13251);
      if (itemNumber != itemPlaying)
        buttons.Add(CONTEXT_BUTTON_DELETE, 1210); // Remove
    }
  }

  if (g_partyModeManager.IsEnabled())
  {
    buttons.Add(CONTEXT_BUTTON_EDIT_PARTYMODE, 21439);
    buttons.Add(CONTEXT_BUTTON_CANCEL_PARTYMODE, 588);      // cancel party mode
  }
}

bool CGUIWindowMusicPlayList::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
  case CONTEXT_BUTTON_PLAY_WITH:
    {
      CFileItemPtr item;
      if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
        item = m_vecItems->Get(itemNumber);
      if (!item)
        break;

      VECPLAYERCORES vecCores;  
      CPlayerCoreFactory::GetPlayers(*item, vecCores);
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores);
      if( g_application.m_eForcedNextPlayer != EPC_NONE )
        OnClick(itemNumber);
      return true;
    }
  case CONTEXT_BUTTON_MOVE_ITEM:
    m_movingFrom = itemNumber;
    return true;

  case CONTEXT_BUTTON_MOVE_HERE:
    MoveItem(m_movingFrom, itemNumber);
    m_movingFrom = -1;
    return true;

  case CONTEXT_BUTTON_CANCEL_MOVE:
    m_movingFrom = -1;
    return true;

  case CONTEXT_BUTTON_MOVE_ITEM_UP:
    OnMove(itemNumber, ACTION_MOVE_ITEM_UP);
    return true;

  case CONTEXT_BUTTON_MOVE_ITEM_DOWN:
    OnMove(itemNumber, ACTION_MOVE_ITEM_DOWN);
    return true;

  case CONTEXT_BUTTON_DELETE:
    RemovePlayListItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_ADD_FAVOURITE:
    {
      CFileItemPtr item = m_vecItems->Get(itemNumber);
      CFavourites::AddOrRemove(item.get(), GetID());
      return true;
    }

  case CONTEXT_BUTTON_CANCEL_PARTYMODE:
    g_partyModeManager.Disable();
    return true;

  case CONTEXT_BUTTON_EDIT_PARTYMODE:
  {
    CStdString playlist = g_settings.GetUserDataItem("PartyMode.xsp");
    if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist))
    {
      // apply new rules
      g_partyModeManager.Disable();
      g_partyModeManager.Enable();
    }
    return true;
  }

  default:
    break;
  }
  return CGUIWindowMusicBase::OnContextButton(itemNumber, button);
}


void CGUIWindowMusicPlayList::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return;

  bool bRestart = m_musicInfoLoader.IsLoading();
  if (bRestart)
    m_musicInfoLoader.StopThread();

  MoveCurrentPlayListItem(iItem, iAction);

  if (bRestart)
    m_musicInfoLoader.Load(*m_vecItems);
}

void CGUIWindowMusicPlayList::MoveItem(int iStart, int iDest)
{
  if (iStart < 0 || iStart >= m_vecItems->Size()) return;
  if (iDest < 0 || iDest >= m_vecItems->Size()) return;

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
  Refresh();

  if (bRestart)
    m_musicInfoLoader.Load(*m_vecItems);
}

void CGUIWindowMusicPlayList::MarkPlaying()
{
/*  // clear markings
  for (int i = 0; i < m_vecItems->Size(); i++)
    m_vecItems->Get(i)->Select(false);

  // mark the currently playing item
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC) && (g_application.IsPlayingAudio()))
  {
    int iSong = g_playlistPlayer.GetCurrentSong();
    if (iSong >= 0 && iSong <= m_vecItems->Size())
      m_vecItems->Get(iSong)->Select(true);
  }*/
}

