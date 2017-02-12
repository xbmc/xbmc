/*
 *      Copyright (C) 2016 Team Kodi
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

#include "MouseWindowingButtonMap.h"
#include "input/MouseStat.h"

#include <algorithm>

using namespace KODI;
using namespace MOUSE;

#define CONTROLLER_PROFILE  "game.controller.mouse"

std::vector<std::pair<unsigned int, std::string>> CMouseWindowingButtonMap::m_buttonMap = {
  { XBMC_BUTTON_LEFT,      "left" },
  { XBMC_BUTTON_MIDDLE,    "middle" },
  { XBMC_BUTTON_RIGHT,     "right" },
  { XBMC_BUTTON_WHEELUP,   "wheelup" },
  { XBMC_BUTTON_WHEELDOWN, "wheeldown" },
};

std::string CMouseWindowingButtonMap::m_pointerName = "pointer";

std::string CMouseWindowingButtonMap::ControllerID(void) const
{
  return CONTROLLER_PROFILE;
}

bool CMouseWindowingButtonMap::GetButton(unsigned int buttonIndex, std::string& feature)
{
  auto it = std::find_if(m_buttonMap.begin(), m_buttonMap.end(),
    [buttonIndex](const std::pair<unsigned int, std::string>& entry)
    {
      return entry.first == buttonIndex;
    });

  if (it != m_buttonMap.end())
  {
    feature = it->second;
    return true;
  }

  return false;
}

bool CMouseWindowingButtonMap::GetRelativePointer(std::string& feature)
{
  feature = m_pointerName;
  return true;
}

bool CMouseWindowingButtonMap::GetButtonIndex(const std::string& feature, unsigned int& buttonIndex)
{
  auto it = std::find_if(m_buttonMap.begin(), m_buttonMap.end(),
    [&feature](const std::pair<unsigned int, std::string>& entry)
    {
      return entry.second == feature;
    });

  if (it != m_buttonMap.end())
  {
    buttonIndex = it->first;
    return true;
  }
  
  return false;
}
