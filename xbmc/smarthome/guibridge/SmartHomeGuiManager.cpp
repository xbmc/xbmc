/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeGuiManager.h"

#include "SmartHomeGuiBridge.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeGuiManager::~CSmartHomeGuiManager() = default;

CSmartHomeGuiBridge& CSmartHomeGuiManager::GetGuiBridge(const std::string& pubSubTopic)
{
  if (m_guiBridges.find(pubSubTopic) == m_guiBridges.end())
    m_guiBridges[pubSubTopic] = std::make_unique<CSmartHomeGuiBridge>();

  return *m_guiBridges[pubSubTopic];
}
