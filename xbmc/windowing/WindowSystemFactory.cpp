/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowSystemFactory.h"

#include <algorithm>

using namespace KODI::WINDOWING;

std::list<std::pair<std::string, std::function<std::unique_ptr<CWinSystemBase>()>>>
    CWindowSystemFactory::m_windowSystems;

std::list<std::string> CWindowSystemFactory::GetWindowSystems()
{
  std::list<std::string> available;
  for (const auto& windowSystem : m_windowSystems)
    available.emplace_back(windowSystem.first);

  return available;
}

std::unique_ptr<CWinSystemBase> CWindowSystemFactory::CreateWindowSystem(const std::string& name)
{
  auto windowSystem =
      std::find_if(m_windowSystems.begin(), m_windowSystems.end(),
                   [&name](auto& windowSystem) { return windowSystem.first == name; });
  if (windowSystem != m_windowSystems.end())
    return windowSystem->second();

  return nullptr;
}

void CWindowSystemFactory::RegisterWindowSystem(
    const std::function<std::unique_ptr<CWinSystemBase>()>& createFunction, const std::string& name)
{
  m_windowSystems.emplace_back(name, createFunction);
}
