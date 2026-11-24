/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISourcesAnnouncementHandler.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"

#include <variant>

CGUISourcesAnnouncementHandler::CGUISourcesAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this, ANNOUNCEMENT::Sources);
}

CGUISourcesAnnouncementHandler::~CGUISourcesAnnouncementHandler()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CGUISourcesAnnouncementHandler::OnSourcesEvent(const ANNOUNCEMENT::SourcesEvent& event)
{
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
  message.SetStringParam(
      std::visit([](const auto& changed) { return changed.path; },
                 static_cast<const ANNOUNCEMENT::SourcesEvent::variant&>(event)));
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
}
