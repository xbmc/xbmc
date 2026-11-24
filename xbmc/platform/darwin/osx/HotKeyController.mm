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
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this,
                                                         ANNOUNCEMENT::GUI | ANNOUNCEMENT::Player);
}

CHotKeyController::~CHotKeyController()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CHotKeyController::OnGUIEvent(const ANNOUNCEMENT::GUIEvent& event)
{
  namespace GUI = ANNOUNCEMENT::EVENT::GUI;
  if (std::holds_alternative<GUI::WindowFocused>(event))
  {
    m_appHasFocus = true;
    [m_mediaKeytap enableMediaKeyTap];
  }
  else if (std::holds_alternative<GUI::WindowUnfocused>(event))
  {
    m_appHasFocus = false;
    if (!m_appIsPlaying)
    {
      [m_mediaKeytap disableMediaKeyTap];
    }
  }
}

void CHotKeyController::OnPlayerEvent(const ANNOUNCEMENT::PlayerEvent& event)
{
  namespace PLAYER = ANNOUNCEMENT::EVENT::PLAYER;
  if (std::holds_alternative<PLAYER::Play>(event) || std::holds_alternative<PLAYER::Resume>(event))
  {
    m_appIsPlaying = true;
    [m_mediaKeytap enableMediaKeyTap];
  }
  else if (std::holds_alternative<PLAYER::Stop>(event))
  {
    m_appIsPlaying = false;
    if (!m_appHasFocus)
    {
      [m_mediaKeytap disableMediaKeyTap];
    }
  }
}
