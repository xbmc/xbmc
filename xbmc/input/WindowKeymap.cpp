/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowKeymap.h"
#include "WindowTranslator.h"

using namespace KODI;

CWindowKeymap::CWindowKeymap(const std::string &controllerId) :
  m_controllerId(controllerId)
{
}

void CWindowKeymap::MapAction(int windowId, const std::string &keyName, JOYSTICK::KeymapAction action)
{
  auto &actionGroup = m_windowKeymap[windowId][keyName];

  actionGroup.windowId = windowId;
  actionGroup.actions.insert(std::move(action));
}

const JOYSTICK::KeymapActionGroup &CWindowKeymap::GetActions(int windowId, const std::string& keyName) const
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

  static const JOYSTICK::KeymapActionGroup empty{};
  return empty;
}
