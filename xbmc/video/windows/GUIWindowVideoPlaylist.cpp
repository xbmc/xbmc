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

#include "GUIWindowVideoPlaylist.h"
#include "playlists/PlayListFactory.h"
#include "Util.h"
#include "playlists/PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "PartyModeManager.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "GUIUserMessages.h"
#include "filesystem/FavouritesDirectory.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
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

CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist()
: CGUIWindowVideoBase(WINDOW_VIDEO_PLAYLIST, "MyVideoPlaylist.xml")
{
  m_movingFrom = -1;
}

CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist()
{
}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
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
      m_movingFrom = -1;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_vecItems->SetPath("playlistvideo://");

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      if (m_vecItems->Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      if (g_application.m_pPlayer->IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      {
        int iSong = g_playlistPlayer.GetCurrentSong();
        if (iSong >= 0 && iSong <= (int)m_vecItems->Size())
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
          g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, !(g_playlistPlayer.IsShuffled(PLAYLIST_VIDEO)));
          CMediaSettings::Get().SetVideoPlaylistShuffled(g_playlistPlayer.IsShuffled(PLAYLIST_VIDEO));
          CSettings::Get().Save();
          UpdateButtons();
          Refresh();
        }
      }
      else if (iControl == CONTROL_BTNSAVE)
      {
        SavePlayList();
      }
      else if (iControl == CONTROL_BTNCLEAR)
      {
        ClearPlayList();
      }
      else if (iControl == CONTROL_BTNPLAY)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Play(m_viewControl.GetSelectedItem());
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        // increment repeat state
        PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(PLAYLIST_VIDEO);
        if (state == PLAYLIST::REPEAT_NONE)
          g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, PLAYLIST::REPEAT_ALL);
        else if (state == PLAYLIST::REPEAT_ALL)
          g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, PLAYLIST::REPEAT_ONE);
        else
          g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, PLAYLIST::REPEAT_NONE);

        // save settings
        CMediaSettings::Get().SetVideoPlaylistRepeat(g_playlistPlayer.GetRepeat(PLAYLIST_VIDEO) == PLAYLIST::REPEAT_ALL);
        CSettings::Get().Save();

        UpdateButtons();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
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
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoPlaylist::OnAction(const CAction &action)
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
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoPlaylist::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
    return CGUIWindow::OnBack(actionID); // base class goes up a folder, but none to go up
  return CGUIWindowVideoBase::OnBack(actionID);
}

bool CGUIWindowVideoPlaylist::MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate /* = true */)
{
  int iSelected = iItem;
  int iNew = iSelected;
  if (iAction == ACTION_MOVE_ITEM_UP)
    iNew--;
  else
    iNew++;

  // is the currently playing item affected?
  bool bFixCurrentSong = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) && (g_application.m_pPlayer->IsPlayingVideo()) &&
    ((g_playlistPlayer.GetCurrentSong() == iSelected) || (g_playlistPlayer.GetCurrentSong() == iNew)))
    bFixCurrentSong = true;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
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


void CGUIWindowVideoPlaylist::ClearPlayList()
{
  ClearFileItems();
  g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
  {
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowVideoPlaylist::UpdateButtons()
{
  // Update playlist buttons
  if (m_vecItems->Size() )
  {
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNPLAY);
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);

    if (g_application.m_pPlayer->IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
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
    CONTROL_DISABLE(CONTROL_BTNCLEAR);
    CONTROL_DISABLE(CONTROL_BTNSAVE);
    CONTROL_DISABLE(CONTROL_BTNSHUFFLE);
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
  }

  CGUIMediaWindow::UpdateButtons();

  // update buttons
  CONTROL_DESELECT(CONTROL_BTNSHUFFLE);
  if (g_playlistPlayer.IsShuffled(PLAYLIST_VIDEO))
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);

  // update repeat button
  int iRepeat = 595 + g_playlistPlayer.GetRepeat(PLAYLIST_VIDEO);
  SET_CONTROL_LABEL(CONTROL_BTNREPEAT, g_localizeStrings.Get(iRepeat));

  MarkPlaying();
}

bool CGUIWindowVideoPlaylist::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Play(iItem);
  else
  {
    CFileItemPtr pItem = m_vecItems->Get(iItem);
    std::string strPath = pItem->GetPath();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    // need to update Playlist FileItem's startOffset and resumePoint based on GUIWindowVideoPlaylist FileItem
    if (pItem->m_lStartOffset == STARTOFFSET_RESUME)
    {
      CFileItemPtr pPlaylistItem = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO)[iItem];
      pPlaylistItem->m_lStartOffset = pItem->m_lStartOffset;
      if (pPlaylistItem->HasVideoInfoTag() && pItem->HasVideoInfoTag())
        pPlaylistItem->GetVideoInfoTag()->m_resumePoint = pItem->GetVideoInfoTag()->m_resumePoint;
    }
    // now play item
    g_playlistPlayer.Play( iItem );
  }
  return true;
}

void CGUIWindowVideoPlaylist::RemovePlayListItem(int iItem)
{
  // The current playing song can't be removed
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO && g_application.m_pPlayer->IsPlayingVideo()
      && g_playlistPlayer.GetCurrentSong() == iItem)
    return ;

  g_playlistPlayer.Remove(PLAYLIST_VIDEO, iItem);

  Refresh();

  if (m_vecItems->Size() <= 0)
  {
    SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
  }
  else
  {
    m_viewControl.SetSelectedItem(iItem - 1);
  }

  g_partyModeManager.OnSongChange();
}

/// \brief Save current playlist to playlist folder
void CGUIWindowVideoPlaylist::SavePlayList()
{
  std::string strNewFileName;
  if (CGUIKeyboardFactory::ShowAndGetInput(strNewFileName, g_localizeStrings.Get(16012), false))
  {
    // need 2 rename it
    std::string strFolder = URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "video");
    strNewFileName = CUtil::MakeLegalFileName(strNewFileName);
    strNewFileName += ".m3u";
    std::string strPath = URIUtils::AddFileToFolder(strFolder, strNewFileName);

    CPlayListM3U playlist;
    playlist.Add(*m_vecItems);

    CLog::Log(LOGDEBUG, "Saving video playlist: [%s]", strPath.c_str());
    playlist.Save(strPath);
  }
}

void CGUIWindowVideoPlaylist::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  int itemPlaying = g_playlistPlayer.GetCurrentSong();
  if (m_movingFrom >= 0)
  {
    if (itemNumber != m_movingFrom && (!g_partyModeManager.IsEnabled() || itemNumber > itemPlaying))
      buttons.Add(CONTEXT_BUTTON_MOVE_HERE, 13252);         // move item here
    buttons.Add(CONTEXT_BUTTON_CANCEL_MOVE, 13253);

  }
  else
  {
    if (itemNumber > -1)
    {
      CFileItemPtr item = m_vecItems->Get(itemNumber);
      // check what players we have, if we have multiple display play with option
      VECPLAYERCORES vecCores;
      if (item->IsVideoDb())
      {
        CFileItem item2(item->GetVideoInfoTag()->m_strFileNameAndPath, false);
        CPlayerCoreFactory::Get().GetPlayers(item2, vecCores);
      }
      else
        CPlayerCoreFactory::Get().GetPlayers(*item, vecCores);
      if (vecCores.size() > 1)
        buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213); // Play With...

      if (XFILE::CFavouritesDirectory::IsFavourite(item.get(), GetID()))
        buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14077);     // Remove Favourite
      else
        buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14076);     // Add To Favourites;
    }
    if (itemNumber > (g_partyModeManager.IsEnabled() ? 1 : 0))
      buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_UP, 13332);
    if (itemNumber + 1 < m_vecItems->Size())
      buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_DOWN, 13333);
    if (!g_partyModeManager.IsEnabled() || itemNumber != itemPlaying)
      buttons.Add(CONTEXT_BUTTON_MOVE_ITEM, 13251);

    if (itemNumber != itemPlaying)
      buttons.Add(CONTEXT_BUTTON_DELETE, 15015);
  }
  if (g_partyModeManager.IsEnabled())
  {
    buttons.Add(CONTEXT_BUTTON_EDIT_PARTYMODE, 21439);
    buttons.Add(CONTEXT_BUTTON_CANCEL_PARTYMODE, 588);      // cancel party mode
  }
}

bool CGUIWindowVideoPlaylist::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
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
      if (item->IsVideoDb())
      {
        CFileItem item2(*item->GetVideoInfoTag());
        CPlayerCoreFactory::Get().GetPlayers(item2, vecCores);
      }
      else
        CPlayerCoreFactory::Get().GetPlayers(*item, vecCores);
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::Get().SelectPlayerDialog(vecCores);
      if (g_application.m_eForcedNextPlayer != EPC_NONE)
        OnClick(itemNumber);
      return true;
    }

  case CONTEXT_BUTTON_MOVE_ITEM:
    m_movingFrom = itemNumber;
    return true;

  case CONTEXT_BUTTON_MOVE_HERE:
    if (m_movingFrom >= 0)
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
      XFILE::CFavouritesDirectory::AddOrRemove(item.get(), GetID());
      return true;
    }
  case CONTEXT_BUTTON_CANCEL_PARTYMODE:
    g_partyModeManager.Disable();
    return true;
  case CONTEXT_BUTTON_EDIT_PARTYMODE:
  {
    std::string playlist = "special://profile/PartyMode-Video.xsp";
    if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist))
    {
      // apply new rules
      g_partyModeManager.Disable();
      g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO);
    }
    return true;
  }
  default:
    break;
  }

  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoPlaylist::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return;
  MoveCurrentPlayListItem(iItem, iAction);
}

void CGUIWindowVideoPlaylist::MoveItem(int iStart, int iDest)
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
}

void CGUIWindowVideoPlaylist::MarkPlaying()
{
  /*  // clear markings
  for (int i = 0; i < m_vecItems->Size(); i++)
    m_vecItems->Get(i)->Select(false);

  // mark the currently playing item
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) && (g_application.m_pPlayer->IsPlayingVideo()))
  {
    int iSong = g_playlistPlayer.GetCurrentSong();
    if (iSong >= 0 && iSong <= m_vecItems->Size())
      m_vecItems->Get(iSong)->Select(true);
  }*/
}

