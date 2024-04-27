/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowMusicPlaylist.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListM3U.h"
#include "profiles/ProfileManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/LabelFormatter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/GUIViewState.h"

using namespace KODI;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY 3
#define CONTROL_BTNSORTASC 4
#define CONTROL_LABELFILES 12

#define CONTROL_BTNSHUFFLE 20
#define CONTROL_BTNSAVE 21
#define CONTROL_BTNCLEAR 22

#define CONTROL_BTNPLAY 23
#define CONTROL_BTNNEXT 24
#define CONTROL_BTNPREVIOUS 25
#define CONTROL_BTNREPEAT 26

CGUIWindowMusicPlayList::CGUIWindowMusicPlayList(void)
  : CGUIWindowMusicBase(WINDOW_MUSIC_PLAYLIST, "MyPlaylist.xml")
{
  m_musicInfoLoader.SetObserver(this);
  m_movingFrom = -1;
}

CGUIWindowMusicPlayList::~CGUIWindowMusicPlayList(void) = default;

bool CGUIWindowMusicPlayList::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
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

      if (m_vecItemsUpdating)
      {
        CLog::Log(LOGWARNING, "CGUIWindowMusicPlayList::OnMessage - updating in progress");
        return true;
      }
      CUpdateGuard ug(m_vecItemsUpdating);

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
      m_musicInfoLoader.UseCacheOnHD("special://temp/archive_cache/MusicPlaylist.fi");

      m_vecItems->SetPath("playlistmusic://");

      // updatebuttons is called in here
      if (!CGUIWindowMusicBase::OnMessage(message))
        return false;

      if (m_vecItems->Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->IsPlayingAudio() &&
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
      {
        int iSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
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
          CServiceBroker::GetPlaylistPlayer().SetShuffle(
              PLAYLIST::TYPE_MUSIC,
              !(CServiceBroker::GetPlaylistPlayer().IsShuffled(PLAYLIST::TYPE_MUSIC)));
          CMediaSettings::GetInstance().SetMusicPlaylistShuffled(
              CServiceBroker::GetPlaylistPlayer().IsShuffled(PLAYLIST::TYPE_MUSIC));
          CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
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
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);
        CServiceBroker::GetPlaylistPlayer().Reset();
        CServiceBroker::GetPlaylistPlayer().Play(m_viewControl.GetSelectedItem(), "");
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);
        CServiceBroker::GetPlaylistPlayer().PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);
        CServiceBroker::GetPlaylistPlayer().PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        // increment repeat state
        PLAYLIST::RepeatState state =
            CServiceBroker::GetPlaylistPlayer().GetRepeat(PLAYLIST::TYPE_MUSIC);
        if (state == PLAYLIST::RepeatState::NONE)
          CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_MUSIC,
                                                        PLAYLIST::RepeatState::ALL);
        else if (state == PLAYLIST::RepeatState::ALL)
          CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_MUSIC,
                                                        PLAYLIST::RepeatState::ONE);
        else
          CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_MUSIC,
                                                        PLAYLIST::RepeatState::NONE);

        // save settings
        CMediaSettings::GetInstance().SetMusicPlaylistRepeat(
            CServiceBroker::GetPlaylistPlayer().GetRepeat(PLAYLIST::TYPE_MUSIC) ==
            PLAYLIST::RepeatState::ALL);
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

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

bool CGUIWindowMusicPlayList::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_PARENT_DIR)
  {
    // Playlist has no parent dirs
    return true;
  }
  if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
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
  CancelUpdateItems();

  if (actionID == ACTION_NAV_BACK)
    return CGUIWindow::OnBack(actionID); // base class goes up a folder, but none to go up
  return CGUIWindowMusicBase::OnBack(actionID);
}

bool CGUIWindowMusicPlayList::MoveCurrentPlayListItem(int iItem,
                                                      int iAction,
                                                      bool bUpdate /* = true */)
{
  int iSelected = iItem;
  int iNew = iSelected;
  if (iAction == ACTION_MOVE_ITEM_UP)
    iNew--;
  else
    iNew++;

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  // is the currently playing item affected?
  bool bFixCurrentSong = false;
  if ((CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC) &&
      appPlayer->IsPlayingAudio() &&
      ((CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == iSelected) ||
       (CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == iNew)))
    bFixCurrentSong = true;

  PLAYLIST::CPlayList& playlist =
      CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC);
  if (playlist.Swap(iSelected, iNew))
  {
    // Correct the current playing song in playlistplayer
    if (bFixCurrentSong)
    {
      int iCurrentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
      if (iSelected == iCurrentSong)
        iCurrentSong = iNew;
      else if (iNew == iCurrentSong)
        iCurrentSong = iSelected;
      CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(iCurrentSong);
    }

    if (bUpdate)
      Refresh();
    return true;
  }

  return false;
}

void CGUIWindowMusicPlayList::SavePlayList()
{
  std::string strNewFileName;
  if (CGUIKeyboardFactory::ShowAndGetInput(strNewFileName, CVariant{g_localizeStrings.Get(16012)},
                                           false))
  {
    // need 2 rename it
    strNewFileName = CUtil::MakeLegalFileName(std::move(strNewFileName));
    strNewFileName += ".m3u8";
    std::string strPath =
        URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                                      CSettings::SETTING_SYSTEM_PLAYLISTSPATH),
                                  "music", strNewFileName);

    // get selected item
    int iItem = m_viewControl.GetSelectedItem();
    std::string strSelectedItem = "";
    if (iItem >= 0 && iItem < m_vecItems->Size())
    {
      CFileItemPtr pItem = m_vecItems->Get(iItem);
      if (!pItem->IsParentFolder())
      {
        GetDirectoryHistoryString(pItem.get(), strSelectedItem);
      }
    }

    std::string strOldDirectory = m_vecItems->GetPath();
    m_history.SetSelectedItem(strSelectedItem, strOldDirectory, iItem);

    PLAYLIST::CPlayListM3U playlist;
    for (int i = 0; i < m_vecItems->Size(); ++i)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);

      //  Musicdatabase items should contain the real path instead of a musicdb url
      //  otherwise the user can't save and reuse the playlist when the musicdb gets deleted
      if (MUSIC::IsMusicDb(*pItem))
        pItem->SetPath(pItem->GetMusicInfoTag()->GetURL());

      playlist.Add(pItem);
    }
    CLog::Log(LOGDEBUG, "Saving music playlist: [{}]", strPath);
    playlist.Save(strPath);
    Refresh(); // need to update
  }
}

void CGUIWindowMusicPlayList::ClearPlayList()
{
  ClearFileItems();
  CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_MUSIC);
  if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
  {
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_NONE);
  }
  Refresh();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowMusicPlayList::RemovePlayListItem(int iItem)
{
  if (iItem < 0 || iItem > m_vecItems->Size())
    return;

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  // The current playing song can't be removed
  if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC &&
      appPlayer->IsPlayingAudio() &&
      CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == iItem)
    return;

  CServiceBroker::GetPlaylistPlayer().Remove(PLAYLIST::TYPE_MUSIC, iItem);

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
  if (m_vecItems->Size() && !g_partyModeManager.IsEnabled())
  {
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);
    CONTROL_ENABLE(CONTROL_BTNPLAY);

    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlayingAudio() &&
        CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
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
  if (CServiceBroker::GetPlaylistPlayer().IsShuffled(PLAYLIST::TYPE_MUSIC))
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);

  // update repeat button
  int iLocalizedString;
  PLAYLIST::RepeatState repState =
      CServiceBroker::GetPlaylistPlayer().GetRepeat(PLAYLIST::TYPE_MUSIC);
  if (repState == PLAYLIST::RepeatState::NONE)
    iLocalizedString = 595; // Repeat: Off
  else if (repState == PLAYLIST::RepeatState::ONE)
    iLocalizedString = 596; // Repeat: One
  else
    iLocalizedString = 597; // Repeat: All

  SET_CONTROL_LABEL(CONTROL_BTNREPEAT, g_localizeStrings.Get(iLocalizedString));

  // Update object count label
  std::string items =
      StringUtils::Format("{} {}", m_vecItems->GetObjectCount(), g_localizeStrings.Get(127));
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  MarkPlaying();
}

bool CGUIWindowMusicPlayList::OnPlayMedia(int iItem, const std::string& player)
{
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Play(iItem);
  else
  {
    PLAYLIST::Id playlistId = m_guiState->GetPlaylist();
    if (playlistId != PLAYLIST::TYPE_NONE)
    {
      if (m_guiState)
        m_guiState->SetPlaylistDirectory(m_vecItems->GetPath());

      CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(playlistId);
      CServiceBroker::GetPlaylistPlayer().Play(iItem, player);
    }
    else
    {
      // Reset Playlistplayer, playback started now does
      // not use the playlistplayer.
      CFileItemPtr pItem = m_vecItems->Get(iItem);
      CServiceBroker::GetPlaylistPlayer().Reset();
      CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_NONE);
      g_application.PlayFile(*pItem, player);
    }
  }

  return true;
}

void CGUIWindowMusicPlayList::OnItemLoaded(CFileItem* pItem)
{
  if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded())
  { // set label 1+2 from tags
    const std::shared_ptr<CSettings> settings =
        CServiceBroker::GetSettingsComponent()->GetSettings();
    std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT);
    if (strTrack.empty())
      strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
    CLabelFormatter formatter(strTrack, "%D");
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
      std::string str;
      str = CUtil::GetTitleFromPath(pItem->GetPath());
      str = StringUtils::Format("{:02}. {} ", pItem->m_iprogramCount, str);
      pItem->SetLabel(str);
    }
  }
}

bool CGUIWindowMusicPlayList::Update(const std::string& strDirectory,
                                     bool updateFilterPath /* = true */)
{
  if (m_musicInfoLoader.IsLoading())
    m_musicInfoLoader.StopThread();

  if (!CGUIWindowMusicBase::Update(strDirectory, updateFilterPath))
    return false;

  if (m_vecItems->GetContent().empty())
    m_vecItems->SetContent("songs");

  m_musicInfoLoader.Load(*m_vecItems);
  return true;
}

void CGUIWindowMusicPlayList::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  // is this playlist playing?
  int itemPlaying = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();

  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
  {
    CFileItemPtr item;
    item = m_vecItems->Get(itemNumber);

    if (m_movingFrom >= 0)
    {
      // we can move the item to any position not where we are, and any position not above currently
      // playing item in party mode
      if (itemNumber != m_movingFrom &&
          (!g_partyModeManager.IsEnabled() || itemNumber > itemPlaying))
        buttons.Add(CONTEXT_BUTTON_MOVE_HERE, 13252); // move item here
      buttons.Add(CONTEXT_BUTTON_CANCEL_MOVE, 13253);
    }
    else
    {
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
    buttons.Add(CONTEXT_BUTTON_CANCEL_PARTYMODE, 588); // cancel party mode
  }
}

bool CGUIWindowMusicPlayList::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
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

    case CONTEXT_BUTTON_CANCEL_PARTYMODE:
      g_partyModeManager.Disable();
      return true;

    case CONTEXT_BUTTON_EDIT_PARTYMODE:
    {
      const std::shared_ptr<CProfileManager> profileManager =
          CServiceBroker::GetSettingsComponent()->GetProfileManager();

      std::string playlist = profileManager->GetUserDataItem("PartyMode.xsp");
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
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  bool bRestart = m_musicInfoLoader.IsLoading();
  if (bRestart)
    m_musicInfoLoader.StopThread();

  MoveCurrentPlayListItem(iItem, iAction);

  if (bRestart)
    m_musicInfoLoader.Load(*m_vecItems);
}

void CGUIWindowMusicPlayList::MoveItem(int iStart, int iDest)
{
  if (iStart < 0 || iStart >= m_vecItems->Size())
    return;
  if (iDest < 0 || iDest >= m_vecItems->Size())
    return;

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
  if ((CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == TYPE_MUSIC) && (g_application.GetAppPlayer().IsPlayingAudio()))
  {
    int iSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
    if (iSong >= 0 && iSong <= m_vecItems->Size())
      m_vecItems->Get(iSong)->Select(true);
  }*/
}
