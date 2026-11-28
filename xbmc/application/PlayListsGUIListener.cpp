/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListsGUIListener.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "playlists/PlayListEntryRules.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "threads/IRunnable.h"
#include "utils/Variant.h"

#include <utility>

using namespace KODI::MESSAGING;

namespace
{
constexpr int STRING_PLAYLIST = 559;
constexpr int STRING_NO_NEXT_ITEM = 34201;
constexpr int STRING_NO_PREVIOUS_ITEM = 34202;
constexpr int STRING_PLAYBACK_FAILED = 16026;
constexpr int STRING_ENTRIES_FAILED = 16027;

class CExpandAsListed : public IRunnable
{
public:
  CExpandAsListed(const std::shared_ptr<CFileItem>& item, CFileItemList& entries)
    : m_item(item),
      m_entries(entries)
  {
  }

  void Run() override
  {
    KODI::PLAYLIST::CEntriesAsListed asListed;
    CApplicationPlayLists::ExpandToEntries(m_item, asListed, nullptr, m_entries);
  }

private:
  const std::shared_ptr<CFileItem> m_item;
  CFileItemList& m_entries;
};

void SendToWindows(int message)
{
  if (CGUIComponent* gui = CServiceBroker::GetGUI(); gui)
  {
    CGUIMessage msg(message, 0, 0);
    gui->GetWindowManager().SendThreadMessage(msg);
  }
}
} // namespace

namespace KODI::APPLICATION
{
bool ExpandAsListed(const std::shared_ptr<CFileItem>& item, CFileItemList& entries)
{
  CExpandAsListed expand(item, entries);
  return CGUIDialogBusy::Wait(&expand, 100, true);
}

void NotifyIfNothingThere(CApplicationPlayLists::Step step, Direction direction)
{
  if (step != CApplicationPlayLists::Step::NothingThere)
    return;
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Info, strings.Get(STRING_PLAYLIST),
      strings.Get(direction == Direction::Forward ? STRING_NO_NEXT_ITEM : STRING_NO_PREVIOUS_ITEM));
}
} // namespace KODI::APPLICATION

CPlayListsGUIListener::CPlayListsGUIListener(std::shared_ptr<CApplicationPlayLists> playLists)
  : m_playLists(std::move(playLists))
{
  m_playLists->SetGUIListener(this);
}

CPlayListsGUIListener::~CPlayListsGUIListener()
{
  m_playLists->SetGUIListener(nullptr);
}

void CPlayListsGUIListener::OnPlayListsChanged()
{
  SendToWindows(GUI_MSG_PLAYLIST_CHANGED);
}

void CPlayListsGUIListener::OnStopped()
{
  SendToWindows(GUI_MSG_PLAYLISTPLAYER_STOPPED);
}

void CPlayListsGUIListener::OnEntriesFailed()
{
  HELPERS::ShowOKDialogText(CVariant{STRING_PLAYBACK_FAILED}, CVariant{STRING_ENTRIES_FAILED});
}
