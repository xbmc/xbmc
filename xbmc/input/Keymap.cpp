/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Keymap.h"
#include "IKeymapEnvironment.h"

using namespace KODI;

CKeymap::CKeymap(std::shared_ptr<const IWindowKeymap> keymap, const IKeymapEnvironment *environment) :
  m_keymap(std::move(keymap)),
  m_environment(environment)
{
}

std::string CKeymap::ControllerID() const
{
  return m_keymap->ControllerID();
}

const JOYSTICK::KeymapActions &CKeymap::GetActions(const std::string& keyName) const
{
  const int windowId = m_environment->GetWindowID();
  const JOYSTICK::KeymapActions& actions = m_keymap->GetActions(windowId, keyName);
  if (!actions.empty())
    return actions;

  const int fallbackWindowId = m_environment->GetFallthrough(windowId);
  if (fallbackWindowId >= 0)
  {
    const JOYSTICK::KeymapActions& fallbackActions = m_keymap->GetActions(fallbackWindowId, keyName);
    if (!fallbackActions.empty())
      return fallbackActions;
  }

  if (m_environment->UseGlobalFallthrough())
  {
    const JOYSTICK::KeymapActions& globalActions = m_keymap->GetActions(-1, keyName);
    if (!globalActions.empty())
      return globalActions;
  }

  static const JOYSTICK::KeymapActions empty;
  return empty;
}
