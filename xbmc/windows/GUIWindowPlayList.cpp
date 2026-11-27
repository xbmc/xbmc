/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPlayList.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "PartyMode.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationPlayLists.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "filesystem/PlaylistDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/windows/GUIWindowMusicBase.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListM3U.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ItemProperties.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "view/GUIViewState.h"

using namespace KODI;

namespace
{
constexpr int CONTROL_BTNVIEWASICONS = 2;

constexpr int CONTROL_BTNSHUFFLE = 20;
constexpr int CONTROL_BTNSAVE = 21;
constexpr int CONTROL_BTNCLEAR = 22;

constexpr int CONTROL_BTNPLAY = 23;
constexpr int CONTROL_BTNNEXT = 24;
constexpr int CONTROL_BTNPREVIOUS = 25;
constexpr int CONTROL_BTNREPEAT = 26;

} // namespace

int GetPlayListWindowId(PLAYLIST::Type type)
{
  return type == PLAYLIST::Audio ? WINDOW_MUSIC_PLAYLIST : WINDOW_VIDEO_PLAYLIST;
}

void ShowPlayListWindow(PLAYLIST::Type type)
{
  CGUIWindowManager& windowManager = CServiceBroker::GetGUI()->GetWindowManager();
  if (const int window = GetPlayListWindowId(type); windowManager.GetActiveWindow() != window)
    windowManager.ActivateWindow(window);
}

template<typename Base>
CGUIWindowPlayList<Base>::CGUIWindowPlayList(PLAYLIST::Type type)
  : Base(GetPlayListWindowId(type), "MyPlaylist.xml"),
    m_type(type),
    m_playLists(CServiceBroker::GetPlayLists())
{
}

template<typename Base>
CGUIWindowPlayList<Base>::~CGUIWindowPlayList() = default;

template<typename Base>
bool CGUIWindowPlayList<Base>::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_PLAYLIST_CHANGED:
    {
      UpdateButtons();

      if (this->m_vecItemsUpdating)
      {
        CLog::LogF(LOGWARNING, "updating in progress");
        return true;
      }
      typename Base::CUpdateGuard guard(this->m_vecItemsUpdating);

      this->Refresh(true);

      if (this->m_viewControl.HasControl(this->m_iLastControl) && this->m_vecItems->IsEmpty())
      {
        this->m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(this->m_iLastControl, 0);
      }
      break;
    }

    // what can be skipped to moves with the entry playing
    case GUI_MSG_PLAYBACK_STARTED:
    case GUI_MSG_PLAYLISTPLAYER_STOPPED:
      UpdateButtons();
      break;

    case GUI_MSG_WINDOW_DEINIT:
      StopLoadingItems();
      m_movingFrom = -1;
      break;

    case GUI_MSG_WINDOW_INIT:
    {
      this->m_vecItems->SetPath(XFILE::CPlaylistDirectory::PathOf(m_type));

      if (!Base::OnMessage(message))
        return false;

      if (this->m_vecItems->IsEmpty())
      {
        this->m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(this->m_iLastControl, 0);
      }

      if (const int row = RowOf(m_playLists->GetPlayingPosition(m_type)); row >= 0)
        this->m_viewControl.SetSelectedItem(row);
      return true;
    }

    case GUI_MSG_CLICKED:
    {
      const int control = message.GetSenderId();
      if (control == CONTROL_BTNSHUFFLE)
      {
        ToggleShuffle();
      }
      else if (control == CONTROL_BTNSAVE)
      {
        StopLoadingItems();
        SavePlayList();
      }
      else if (control == CONTROL_BTNCLEAR)
      {
        StopLoadingItems();
        ClearPlayList();
      }
      else if (control == CONTROL_BTNPLAY)
      {
        m_playLists->PlayFrom(m_type, ListPosition(this->m_viewControl.GetSelectedItem()));
        UpdateButtons();
      }
      else if (control == CONTROL_BTNNEXT)
      {
        m_playLists->PlayNext(m_type);
      }
      else if (control == CONTROL_BTNPREVIOUS)
      {
        m_playLists->PlayPrevious(m_type);
      }
      else if (control == CONTROL_BTNREPEAT)
      {
        CycleRepeat();
      }
      else if (this->m_viewControl.HasControl(control))
      {
        const int action = message.GetParam1();
        if (action == ACTION_DELETE_ITEM || action == ACTION_MOUSE_MIDDLE_CLICK)
          RemovePlayListItem(this->m_viewControl.GetSelectedItem());
      }
      break;
    }
  }
  return Base::OnMessage(message);
}

template<typename Base>
bool CGUIWindowPlayList<Base>::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PARENT_DIR:
      return true;

    case ACTION_SHOW_PLAYLIST:
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
      return true;

    case ACTION_MOVE_ITEM_UP:
    case ACTION_MOVE_ITEM_DOWN:
    {
      const int item = this->m_viewControl.HasControl(this->GetFocusedControlID())
                           ? this->m_viewControl.GetSelectedItem()
                           : -1;
      OnMove(item, action.GetID());
      return true;
    }

    case ACTION_PLAYER_PLAY:
      if (this->m_viewControl.HasControl(this->GetFocusedControlID()))
        return OnPlayMedia(this->m_viewControl.GetSelectedItem());
      break;

    default:
      break;
  }
  return Base::OnAction(action);
}

template<typename Base>
bool CGUIWindowPlayList<Base>::OnBack(int actionID)
{
  // the media window's back goes up a folder, and a playlist has none
  if (actionID == ACTION_NAV_BACK)
    return CGUIWindow::OnBack(actionID);
  return Base::OnBack(actionID);
}

template<typename Base>
bool CGUIWindowPlayList<Base>::OnPlayMedia(int iItem, const std::string& player)
{
  if (iItem < 0 || iItem >= this->m_vecItems->Size())
    return false;

  PlayEntry(iItem, player);
  return true;
}

template<typename Base>
void CGUIWindowPlayList<Base>::PlayEntry(int iItem, const std::string& player)
{
  m_playLists->PlayFrom(m_type, ListPosition(iItem), {.player = player});
}

template<typename Base>
void CGUIWindowPlayList<Base>::UpdateButtons()
{
  Base::UpdateButtons();

  if (!this->m_vecItems->IsEmpty())
  {
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);
    CONTROL_ENABLE(CONTROL_BTNPLAY);

    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTNNEXT, m_playLists->HasNext(m_type));
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTNPREVIOUS, m_playLists->HasPrevious(m_type));
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNSHUFFLE);
    CONTROL_DISABLE(CONTROL_BTNSAVE);
    CONTROL_DISABLE(CONTROL_BTNCLEAR);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
  }

  CONTROL_DESELECT(CONTROL_BTNSHUFFLE);
  if (m_playLists->IsShuffled(m_type))
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);

  SET_CONTROL_LABEL(
      CONTROL_BTNREPEAT,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
          CApplicationPlayLists::RepeatLabel(m_playLists->GetRepeat(m_type),
                                             CApplicationPlayLists::RepeatWording::WithSetting)));
}

template<typename Base>
void CGUIWindowPlayList<Base>::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  if (itemNumber >= 0 && itemNumber < this->m_vecItems->Size())
  {
    const int itemPlaying = m_playLists->GetPlayingPosition(m_type);

    if (m_movingFrom >= 0)
    {
      if (itemNumber != m_movingFrom)
        buttons.Add(CONTEXT_BUTTON_MOVE_HERE, 13252);
      buttons.Add(CONTEXT_BUTTON_CANCEL_MOVE, 13253);
    }
    else
    {
      // rows in play order are not list order, so a shuffled list is not moved by its rows
      if (!m_playLists->IsShuffled(m_type))
      {
        if (itemNumber > 0)
          buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_UP, 13332);
        if (itemNumber + 1 < this->m_vecItems->Size())
          buttons.Add(CONTEXT_BUTTON_MOVE_ITEM_DOWN, 13333);
        buttons.Add(CONTEXT_BUTTON_MOVE_ITEM, 13251);
      }
      if (ListPosition(itemNumber) != itemPlaying)
        buttons.Add(CONTEXT_BUTTON_DELETE, 1210); // Remove
    }
  }

  if (PARTYMODE::IsRunning(m_type))
  {
    buttons.Add(CONTEXT_BUTTON_EDIT_PARTYMODE, 21439);
    buttons.Add(CONTEXT_BUTTON_CANCEL_PARTYMODE, 588);
  }
}

template<typename Base>
bool CGUIWindowPlayList<Base>::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
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
      PARTYMODE::Stop();
      return true;

    case CONTEXT_BUTTON_EDIT_PARTYMODE:
    {
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(PARTYMODE::RulesPath(m_type)))
        PARTYMODE::Start(m_type);
      return true;
    }

    default:
      break;
  }
  return Base::OnContextButton(itemNumber, button);
}

template<typename Base>
int CGUIWindowPlayList<Base>::ListPosition(int iItem) const
{
  if (iItem < 0 || iItem >= this->m_vecItems->Size())
    return -1;
  return static_cast<int>(
      this->m_vecItems->Get(iItem)->GetProperty(ITEM::PROPERTY::PLAYLIST_POSITION).asInteger(-1));
}

template<typename Base>
int CGUIWindowPlayList<Base>::RowOf(int position) const
{
  if (position < 0)
    return -1;
  for (int row = 0; row < this->m_vecItems->Size(); ++row)
  {
    if (ListPosition(row) == position)
      return row;
  }
  return -1;
}

template<typename Base>
void CGUIWindowPlayList<Base>::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= this->m_vecItems->Size() || m_playLists->IsShuffled(m_type))
    return;

  const bool restart = StopLoadingItems();
  MoveCurrentPlayListItem(iItem, iAction);
  if (restart)
    StartLoadingItems();
}

template<typename Base>
void CGUIWindowPlayList<Base>::MoveItem(int iStart, int iDest)
{
  if (iStart < 0 || iStart >= this->m_vecItems->Size() || iDest < 0 ||
      iDest >= this->m_vecItems->Size() || m_playLists->IsShuffled(m_type))
  {
    return;
  }

  const bool restart = StopLoadingItems();
  m_playLists->Move(m_type, iStart, iDest);
  this->Refresh();

  if (restart)
    StartLoadingItems();
}

template<typename Base>
void CGUIWindowPlayList<Base>::MoveCurrentPlayListItem(int iItem, int iAction)
{
  const int destination = iAction == ACTION_MOVE_ITEM_UP ? iItem - 1 : iItem + 1;
  if (m_playLists->Swap(m_type, iItem, destination))
    this->Refresh();
}

template<typename Base>
void CGUIWindowPlayList<Base>::RemovePlayListItem(int iItem)
{
  if (iItem < 0 || iItem >= this->m_vecItems->Size())
    return;

  if (!m_playLists->Remove(m_type, ListPosition(iItem)))
    return;
  this->Refresh();

  if (this->m_vecItems->IsEmpty())
    SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
  else
    this->m_viewControl.SetSelectedItem(iItem);
}

template<typename Base>
void CGUIWindowPlayList<Base>::ClearPlayList()
{
  this->ClearFileItems();
  m_playLists->Clear(m_type);
  this->Refresh();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

template<typename Base>
void CGUIWindowPlayList<Base>::SavePlayList()
{
  std::string name;
  if (!CGUIKeyboardFactory::ShowAndGetInput(
          name, CVariant{CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(16012)},
          false))
  {
    return;
  }

  const std::string path =
      URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                                    CSettings::SETTING_SYSTEM_PLAYLISTSPATH),
                                m_type == PLAYLIST::Audio ? "music" : "video",
                                CUtil::MakeLegalFileName(std::move(name)) + ".m3u8");

  PLAYLIST::CPlayListM3U playlist;
  playlist.Add(*this->m_vecItems);
  CLog::LogF(LOGDEBUG, "saving [{}]", path);
  playlist.Save(path);
}

template<typename Base>
void CGUIWindowPlayList<Base>::ToggleShuffle()
{
  m_playLists->ToggleShuffle(m_type, CApplicationPlayLists::Persist::Yes);
  UpdateButtons();
  this->Refresh();
}

template<typename Base>
void CGUIWindowPlayList<Base>::CycleRepeat()
{
  m_playLists->CycleRepeat(m_type, CApplicationPlayLists::Persist::Yes);
  UpdateButtons();
}

template class CGUIWindowPlayList<CGUIWindowMusicBase>;
template class CGUIWindowPlayList<CGUIWindowVideoBase>;
