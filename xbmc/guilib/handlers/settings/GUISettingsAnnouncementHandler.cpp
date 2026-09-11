/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISettingsAnnouncementHandler.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"

CGUISettingsAnnouncementHandler::CGUISettingsAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this, ANNOUNCEMENT::Settings);
}

CGUISettingsAnnouncementHandler::~CGUISettingsAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CGUISettingsAnnouncementHandler::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                               const std::string& sender,
                                               const std::string& message,
                                               const CVariant& data)
{
  if (message == "OnLevelChanged")
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_SETTING_LEVEL_CHANGED);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}
