/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowKeymap.h"

#include "input/WindowTranslator.h"

using namespace KODI;
using namespace KEYMAP;

CWindowKeymap::CWindowKeymap(const std::string& controllerId) : m_controllerId(controllerId)
{
}

void CWindowKeymap::MapAction(int windowId, const std::string& keyName, KeymapAction action)
{
  auto& actionGroup = m_windowKeymap[windowId][keyName];

  actionGroup.windowId = windowId;
  auto it = actionGroup.actions.begin();
  while (it != actionGroup.actions.end())
  {
    if (it->holdTimeMs == action.holdTimeMs && it->hotkeys == action.hotkeys)
      it = actionGroup.actions.erase(it);
    else
      it++;
  }
  actionGroup.actions.insert(std::move(action));
}

const KeymapActionGroup& CWindowKeymap::GetActions(int windowId, const std::string& keyName) const
{
  // handle virtual windows
  windowId = CWindowTranslator::GetVirtualWindow(windowId);

  auto it = m_windowKeymap.find(windowId);
  if (it != m_windowKeymap.end())
  {
    auto& keymap = it->second;
    auto it2 = keymap.find(keyName);
    if (it2 != keymap.end())
      return it2->second;
  }

  static const KeymapActionGroup empty{};
  return empty;
}
