/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/ImageSubscriptionKey.h"

#include <map>
#include <memory>
#include <string>

namespace KODI
{
namespace SMART_HOME
{
class CSmartHomeGuiBridge;

class CSmartHomeGuiManager
{
public:
  CSmartHomeGuiManager() = default;
  ~CSmartHomeGuiManager();

  // Get bridge between GUI and renderer
  CSmartHomeGuiBridge& GetGuiBridge(const ImageSubscriptionKey& subscription);

private:
  // Smart home parameters
  // Subscription identity -> GUI bridge
  std::map<ImageSubscriptionKey, std::unique_ptr<CSmartHomeGuiBridge>> m_guiBridges;
};

} // namespace SMART_HOME
} // namespace KODI
