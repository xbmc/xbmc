/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HotKeyController.h"

#include "ServiceBroker.h"
#include "interfaces/AnnouncementManager.h"

#include "platform/darwin/osx/MediaKeys.h"

CHotKeyController::CHotKeyController()
{
  m_mediaKeytap = [CMediaKeyTap new];
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
}

CHotKeyController::~CHotKeyController()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CHotKeyController::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                 const std::string& sender,
                                 const std::string& message,
                                 const CVariant& data)
{
  if (sender != ANNOUNCEMENT::CAnnouncementManager::ANNOUNCEMENT_SENDER)
    return;

  switch (flag)
  {
    case ANNOUNCEMENT::GUI:
    {
      if (message == "WindowFocused")
      {
        m_appHasFocus = true;
        [m_mediaKeytap enableMediaKeyTap];
      }
      else if (message == "WindowUnfocused")
      {
        m_appHasFocus = false;
        if (!m_appIsPlaying)
        {
          [m_mediaKeytap disableMediaKeyTap];
        }
      }
      break;
    }
    case ANNOUNCEMENT::Player:
    {
      if (message == "OnPlay" || message == "OnResume")
      {
        m_appIsPlaying = true;
        [m_mediaKeytap enableMediaKeyTap];
      }
      else if (message == "OnStop")
      {
        m_appIsPlaying = false;
        if (!m_appHasFocus)
        {
          [m_mediaKeytap disableMediaKeyTap];
        }
      }
      break;
    }
    default:
      break;
  }
}
