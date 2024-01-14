/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Keymap.h"

#include "input/keymaps/interfaces/IKeymapEnvironment.h"

using namespace KODI;
using namespace KEYMAP;

CKeymap::CKeymap(std::shared_ptr<const IWindowKeymap> keymap, const IKeymapEnvironment* environment)
  : m_keymap(std::move(keymap)), m_environment(environment)
{
}

std::string CKeymap::ControllerID() const
{
  return m_keymap->ControllerID();
}

const KeymapActionGroup& CKeymap::GetActions(const std::string& keyName) const
{
  const int windowId = m_environment->GetWindowID();
  const auto& actions = m_keymap->GetActions(windowId, keyName);
  if (!actions.actions.empty())
    return actions;

  const int fallbackWindowId = m_environment->GetFallthrough(windowId);
  if (fallbackWindowId >= 0)
  {
    const auto& fallbackActions = m_keymap->GetActions(fallbackWindowId, keyName);
    if (!fallbackActions.actions.empty())
      return fallbackActions;
  }

  if (m_environment->UseGlobalFallthrough())
  {
    const auto& globalActions = m_keymap->GetActions(-1, keyName);
    if (!globalActions.actions.empty())
      return globalActions;
  }

  static const KeymapActionGroup empty{};
  return empty;
}
