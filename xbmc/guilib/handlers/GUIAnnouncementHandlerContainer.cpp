/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAnnouncementHandlerContainer.h"

#include "player/GUIPlayerAnnouncementHandler.h"
#include "sources/GUISourcesAnnouncementHandler.h"

CGUIAnnouncementHandlerContainer::CGUIAnnouncementHandlerContainer()
{
  m_announcementHandlers.emplace_back(std::make_unique<CGUISourcesAnnouncementHandler>());
  m_announcementHandlers.emplace_back(std::make_unique<CGUIPlayerAnnouncementHandler>());
}
