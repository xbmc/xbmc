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
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

CGUIPlayerAnnouncementHandler::CGUIPlayerAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this, ANNOUNCEMENT::Player);
}

CGUIPlayerAnnouncementHandler::~CGUIPlayerAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CGUIPlayerAnnouncementHandler::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                             const std::string& sender,
                                             const std::string& message,
                                             const CVariant& data)
{
  if (message == "OnCommercial")
  {
    const std::shared_ptr<CAdvancedSettings> advancedSettings =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    if (advancedSettings && advancedSettings->m_EdlDisplayCommbreakNotifications)
    {
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011), data.asString());
    }
  }
  else if (message == "SourceSlow")
  {
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(21454),
                                          g_localizeStrings.Get(21455));
  }
  else if (message == "OnToggleSkipCommercials")
  {
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011),
                                          g_localizeStrings.Get(data.asBoolean() ? 25013 : 25012));
  }
  else if (message == "OnProcessInfo")
  {
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() !=
        WINDOW_DIALOG_PLAYER_PROCESS_INFO)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(
          WINDOW_DIALOG_PLAYER_PROCESS_INFO);
    }
  }
  else if (message == "OnPlaybackFailed")
  {
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(16026),
                                          g_localizeStrings.Get(16029));
  }
#if defined(HAVE_LIBBLURAY)
  else if (message == "OnBlurayMenuError")
  {
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25008),
                                          g_localizeStrings.Get(25009));
  }
  else if (message == "OnBlurayEncryptedError")
  {
    CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(16026),
                                          g_localizeStrings.Get(29805));
  }
#endif
  else if (message == "OnMenu")
  {
    CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}
