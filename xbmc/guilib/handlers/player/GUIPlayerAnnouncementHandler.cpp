/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPlayerAnnouncementHandler.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <variant>

CGUIPlayerAnnouncementHandler::CGUIPlayerAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this, ANNOUNCEMENT::Player);
}

CGUIPlayerAnnouncementHandler::~CGUIPlayerAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CGUIPlayerAnnouncementHandler::OnPlayerEvent(const ANNOUNCEMENT::PlayerEvent& event)
{
  namespace PLAYER = ANNOUNCEMENT::EVENT::PLAYER;
  auto& localizeStrings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  if (const auto* commercial = std::get_if<PLAYER::Commercial>(&event))
  {
    const std::shared_ptr<CAdvancedSettings> advancedSettings =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    if (advancedSettings && advancedSettings->m_EdlDisplayCommbreakNotifications)
    {
      CGUIDialogKaiToast::QueueNotification(localizeStrings.Get(25011), commercial->time);
    }
  }
  else if (std::holds_alternative<PLAYER::SourceSlow>(event))
  {
    CGUIDialogKaiToast::QueueNotification(localizeStrings.Get(21454), localizeStrings.Get(21455));
  }
  else if (const auto* skip = std::get_if<PLAYER::ToggleSkipCommercials>(&event))
  {
    CGUIDialogKaiToast::QueueNotification(localizeStrings.Get(25011),
                                          localizeStrings.Get(skip->skip ? 25013 : 25012));
  }
  else if (std::holds_alternative<PLAYER::ProcessInfo>(event))
  {
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() !=
        WINDOW_DIALOG_PLAYER_PROCESS_INFO)
    {
      // post: a synchronous activation blocks this thread until the dialog closes
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTIVATE_WINDOW,
                                                 WINDOW_DIALOG_PLAYER_PROCESS_INFO, 0);
    }
  }
  // A failure the playlists report carries a reason and is shown by whoever refused it
  else if (const auto* failed = std::get_if<PLAYER::PlaybackFailed>(&event);
           failed && !failed->reason)
  {
    CGUIDialogKaiToast::QueueNotification(localizeStrings.Get(16026), localizeStrings.Get(16029));
  }
#if defined(HAVE_LIBBLURAY)
  else if (std::holds_alternative<PLAYER::BlurayMenuError>(event))
  {
    CGUIDialogKaiToast::QueueNotification(localizeStrings.Get(25008), localizeStrings.Get(25009));
  }
  else if (std::holds_alternative<PLAYER::BlurayEncryptedError>(event))
  {
    CGUIDialogKaiToast::QueueNotification(localizeStrings.Get(16026), localizeStrings.Get(29805));
  }
#endif
  else if (std::holds_alternative<PLAYER::Menu>(event))
  {
    CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}
