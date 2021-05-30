/*
 *  Copyright (C) 2021-2026 Team Kodi
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

CSmartHomeGuiBridge& CSmartHomeGuiManager::GetGuiBridge(const ImageSubscriptionKey& subscription)
{
  if (m_guiBridges.find(subscription) == m_guiBridges.end())
    m_guiBridges[subscription] = std::make_unique<CSmartHomeGuiBridge>();

  return *m_guiBridges[subscription];
}
