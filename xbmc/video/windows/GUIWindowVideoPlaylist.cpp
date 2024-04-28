/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowVideoPlaylist.h"

#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "playlists/PlayListM3U.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/guilib/VideoPlayActionProcessor.h"

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

using namespace KODI;

CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist()
  : CGUIWindowVideoBase(WINDOW_VIDEO_PLAYLIST, "MyPlaylist.xml")
{
  m_movingFrom = -1;
}

CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist() = default;

void CGUIWindowVideoPlaylist::OnPrepareFileItems(CFileItemList& items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);

  if (items.IsEmpty())
    return;

  if (!VIDEO::IsVideoDb(items) && !items.IsVirtualDirectoryRoot())
  { // load info from the database
    std::string label;
    if (items.GetLabel().empty() &&
        m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::GetInstance().GetSources("video"),
                           &label))
      items.SetLabel(label);
    if (!items.IsSourcesPath() && !items.IsLibraryFolder())
      LoadVideoInfo(items, m_database);
  }
}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
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

      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->IsPlayingVideo() &&
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_VIDEO)
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
              PLAYLIST::TYPE_VIDEO,
              !(CServiceBroker::GetPlaylistPlayer().IsShuffled(PLAYLIST::TYPE_VIDEO)));
          CMediaSettings::GetInstance().SetVideoPlaylistShuffled(
              CServiceBroker::GetPlaylistPlayer().IsShuffled(PLAYLIST::TYPE_VIDEO));
          CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
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
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
        CServiceBroker::GetPlaylistPlayer().Reset();
        CServiceBroker::GetPlaylistPlayer().Play(m_viewControl.GetSelectedItem(), "");
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
        CServiceBroker::GetPlaylistPlayer().PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
        CServiceBroker::GetPlaylistPlayer().PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        // increment repeat state
        PLAYLIST::RepeatState state =
            CServiceBroker::GetPlaylistPlayer().GetRepeat(PLAYLIST::TYPE_VIDEO);
        if (state == PLAYLIST::RepeatState::NONE)
          CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_VIDEO,
                                                        PLAYLIST::RepeatState::ALL);
        else if (state == PLAYLIST::RepeatState::ALL)
          CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_VIDEO,
                                                        PLAYLIST::RepeatState::ONE);
        else
          CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_VIDEO,
                                                        PLAYLIST::RepeatState::NONE);

        // save settings
        CMediaSettings::GetInstance().SetVideoPlaylistRepeat(
            CServiceBroker::GetPlaylistPlayer().GetRepeat(PLAYLIST::TYPE_VIDEO) ==
            PLAYLIST::RepeatState::ALL);
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

        UpdateButtons();
      }
      else if (m_viewControl.HasControl(iControl)) // list/thumb control
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

bool CGUIWindowVideoPlaylist::OnAction(const CAction& action)
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
  if (action.GetID() == ACTION_PLAYER_PLAY)
  {
    if (m_viewControl.HasControl(GetFocusedControlID()))
      return OnPlayMedia(m_viewControl.GetSelectedItem());
  }

  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoPlaylist::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
    return CGUIWindow::OnBack(actionID); // base class goes up a folder, but none to go up
  return CGUIWindowVideoBase::OnBack(actionID);
}

bool CGUIWindowVideoPlaylist::OnSelect(int iItem)
{
  // We ignore default select action and always play the selected item.
  return OnPlayMedia(iItem);
}

bool CGUIWindowVideoPlaylist::MoveCurrentPlayListItem(int iItem,
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
  if ((CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_VIDEO) &&
      appPlayer->IsPlayingVideo() &&
      ((CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == iSelected) ||
       (CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == iNew)))
    bFixCurrentSong = true;

  PLAYLIST::CPlayList& playlist =
      CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_VIDEO);
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

void CGUIWindowVideoPlaylist::ClearPlayList()
{
  ClearFileItems();
  CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_VIDEO);
  if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_VIDEO)
  {
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_NONE);
  }
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowVideoPlaylist::UpdateButtons()
{
  // Update playlist buttons
  if (m_vecItems->Size())
  {
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNPLAY);
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);

    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlayingVideo() &&
        CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_VIDEO)
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
  if (CServiceBroker::GetPlaylistPlayer().IsShuffled(PLAYLIST::TYPE_VIDEO))
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);

  // update repeat button
  PLAYLIST::RepeatState repState =
      CServiceBroker::GetPlaylistPlayer().GetRepeat(PLAYLIST::TYPE_VIDEO);
  int iLocalizedString;
  if (repState == PLAYLIST::RepeatState::NONE)
    iLocalizedString = 595; // Repeat: Off
  else if (repState == PLAYLIST::RepeatState::ONE)
    iLocalizedString = 596; // Repeat: One
  else
    iLocalizedString = 597; // Repeat: All

  SET_CONTROL_LABEL(CONTROL_BTNREPEAT, g_localizeStrings.Get(iLocalizedString));

  MarkPlaying();
}

namespace
{
class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item,
                            int itemIndex,
                            const std::string& player)
    : CVideoPlayActionProcessorBase(item), m_itemIndex(itemIndex), m_player(player)
  {
  }

protected:
  bool OnResumeSelected() override
  {
    auto& playlistPlayer = CServiceBroker::GetPlaylistPlayer();
    playlistPlayer.SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);

    const auto playlistItem{playlistPlayer.GetPlaylist(PLAYLIST::TYPE_VIDEO)[m_itemIndex]};
    playlistItem->SetStartOffset(STARTOFFSET_RESUME);
    if (playlistItem->HasVideoInfoTag() && m_item->HasVideoInfoTag())
      playlistItem->GetVideoInfoTag()->SetResumePoint(m_item->GetVideoInfoTag()->GetResumePoint());

    playlistPlayer.Play(m_itemIndex, m_player);
    return true;
  }

  bool OnPlaySelected() override
  {
    auto& playlistPlayer = CServiceBroker::GetPlaylistPlayer();
    playlistPlayer.SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
    playlistPlayer.Play(m_itemIndex, m_player);
    return true;
  }

private:
  const int m_itemIndex{-1};
  const std::string m_player;
};
} // namespace

bool CGUIWindowVideoPlaylist::OnPlayMedia(int iItem, const std::string& player)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  if (g_partyModeManager.IsEnabled())
  {
    g_partyModeManager.Play(iItem);
  }
  else
  {
    const auto item{m_vecItems->Get(iItem)};
    // play the current video version, even if multiple versions are available
    item->SetProperty("has_resolved_video_asset", true);
    CVideoPlayActionProcessor proc{item, iItem, player};
    proc.ProcessDefaultAction();
    item->ClearProperty("has_resolved_video_asset");
  }
  return true;
}

void CGUIWindowVideoPlaylist::RemovePlayListItem(int iItem)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  // The current playing song can't be removed
  if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_VIDEO &&
      appPlayer->IsPlayingVideo() &&
      CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == iItem)
    return;

  CServiceBroker::GetPlaylistPlayer().Remove(PLAYLIST::TYPE_VIDEO, iItem);

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
  if (CGUIKeyboardFactory::ShowAndGetInput(strNewFileName, CVariant{g_localizeStrings.Get(16012)},
                                           false))
  {
    // need 2 rename it
    strNewFileName = CUtil::MakeLegalFileName(std::move(strNewFileName));
    strNewFileName += ".m3u8";
    std::string strPath =
        URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                                      CSettings::SETTING_SYSTEM_PLAYLISTSPATH),
                                  "video", strNewFileName);

    PLAYLIST::CPlayListM3U playlist;
    playlist.Add(*m_vecItems);

    CLog::Log(LOGDEBUG, "Saving video playlist: [{}]", strPath);
    playlist.Save(strPath);
  }
}

void CGUIWindowVideoPlaylist::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  int itemPlaying = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
  if (m_movingFrom >= 0)
  {
    if (itemNumber != m_movingFrom && (!g_partyModeManager.IsEnabled() || itemNumber > itemPlaying))
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
      buttons.Add(CONTEXT_BUTTON_DELETE, 15015);
  }
  if (g_partyModeManager.IsEnabled())
  {
    buttons.Add(CONTEXT_BUTTON_EDIT_PARTYMODE, 21439);
    buttons.Add(CONTEXT_BUTTON_CANCEL_PARTYMODE, 588); // cancel party mode
  }
}

bool CGUIWindowVideoPlaylist::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
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
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;
  MoveCurrentPlayListItem(iItem, iAction);
}

void CGUIWindowVideoPlaylist::MoveItem(int iStart, int iDest)
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
  if ((CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == TYPE_VIDEO) && (g_application.GetAppPlayer().IsPlayingVideo()))
  {
    int iSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
    if (iSong >= 0 && iSong <= m_vecItems->Size())
      m_vecItems->Get(iSong)->Select(true);
  }*/
}
