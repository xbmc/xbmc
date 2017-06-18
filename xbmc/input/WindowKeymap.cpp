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

#include "WindowKeymap.h"

using namespace KODI;

CWindowKeymap::CWindowKeymap(const std::string &controllerId) :
  m_controllerId(controllerId)
{
}

void CWindowKeymap::MapAction(int windowId, const std::string &keyName, JOYSTICK::KeymapAction action)
{
  m_windowKeymap[windowId][keyName].insert(std::move(action));
}

const JOYSTICK::KeymapActions &CWindowKeymap::GetActions(int windowId, const std::string& keyName) const
{
  auto it = m_windowKeymap.find(windowId);
  if (it != m_windowKeymap.end())
  {
    auto& keymap = it->second;
    auto it2 = keymap.find(keyName);
    if (it2 != keymap.end())
      return it2->second;
  }

  static const JOYSTICK::KeymapActions empty;
  return empty;
}
