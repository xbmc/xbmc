/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  CSmartHomeGuiBridge& GetGuiBridge(const std::string& pubSubTopic);

private:
  // Smart home parameters
  std::map<std::string, std::unique_ptr<CSmartHomeGuiBridge>> m_guiBridges; // Topic -> bridge
};

} // namespace SMART_HOME
} // namespace KODI
