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

#include "JoystickMapper.h"
#include "ActionIDs.h"
#include "ActionTranslator.h"
#include "input/joysticks/JoystickIDs.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/WindowKeymap.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <sstream>
#include <utility>

using namespace KODI;

#define JOYSTICK_XML_NODE_PROFILE    "profile"
#define JOYSTICK_XML_ATTR_DIRECTION  "direction"
#define JOYSTICK_XML_ATTR_HOLDTIME   "holdtime"
#define JOYSTICK_XML_ATTR_HOTKEY     "hotkey"

CJoystickMapper::~CJoystickMapper()
{
}

void CJoystickMapper::MapActions(int windowID, const TiXmlNode *pDevice)
{
  std::string controllerId;
  DeserializeJoystickNode(pDevice, controllerId);

  const TiXmlElement *pButton = pDevice->FirstChildElement();
  while (pButton != nullptr)
  {
    std::string feature;
    JOYSTICK::ANALOG_STICK_DIRECTION dir;
    unsigned int holdtimeMs;
    std::set<std::string> hotkeys;
    std::string actionString;
    if (DeserializeButton(pButton, feature, dir, holdtimeMs, hotkeys, actionString))
    {
      // Update Controller IDs
      if (std::find(m_controllerIds.begin(), m_controllerIds.end(), controllerId) == m_controllerIds.end())
        m_controllerIds.emplace_back(controllerId);

      // Find/create keymap
      auto &keymap = m_joystickKeymaps[controllerId];
      if (!keymap)
        keymap.reset(new CWindowKeymap(controllerId));

      // Update keymap
      unsigned int actionId = ACTION_NONE;
      if (CActionTranslator::TranslateString(actionString, actionId))
      {
        JOYSTICK::KeymapAction action = {
          actionId,
          std::move(actionString),
          holdtimeMs,
          std::move(hotkeys),
        };
        keymap->MapAction(windowID, JOYSTICK::CJoystickUtils::MakeKeyName(feature, dir), std::move(action));
      }
    }
    pButton = pButton->NextSiblingElement();
  }
}

void CJoystickMapper::Clear()
{
  m_joystickKeymaps.clear();
  m_controllerIds.clear();
}

std::vector<std::shared_ptr<const IWindowKeymap>> CJoystickMapper::GetJoystickKeymaps() const
{
  std::vector<std::shared_ptr<const IWindowKeymap>> keymaps;

  for (const auto &controllerId : m_controllerIds)
  {
    auto it = m_joystickKeymaps.find(controllerId);
    if (it != m_joystickKeymaps.end())
      keymaps.emplace_back(it->second);
  }

  return keymaps;
}

void CJoystickMapper::DeserializeJoystickNode(const TiXmlNode* pDevice, std::string &controllerId)
{
  controllerId = DEFAULT_CONTROLLER_ID;

  const TiXmlElement* deviceElem = pDevice->ToElement();
  if (deviceElem != nullptr)
    deviceElem->QueryValueAttribute(JOYSTICK_XML_NODE_PROFILE, &controllerId);
}

bool CJoystickMapper::DeserializeButton(const TiXmlElement *pButton, std::string &feature, JOYSTICK::ANALOG_STICK_DIRECTION &dir, unsigned int& holdtimeMs, std::set<std::string>& hotkeys, std::string &actionStr)
{
  const char *szButton = pButton->Value();
  if (szButton != nullptr)
  {
    const char *szAction = nullptr;

    const TiXmlNode *actionNode = pButton->FirstChild();
    if (actionNode != nullptr)
      szAction = actionNode->Value();

    if (szAction != nullptr)
    {
      feature = szButton;
      StringUtils::ToLower(feature);
      actionStr = szAction;
    }
  }

  if (!feature.empty() && !actionStr.empty())
  {
    // Handle direction
    dir = JOYSTICK::ANALOG_STICK_DIRECTION::UNKNOWN;
    const char *szDirection = pButton->Attribute(JOYSTICK_XML_ATTR_DIRECTION);
    if (szDirection != nullptr)
      dir = JOYSTICK::CJoystickTranslator::TranslateDirection(szDirection);

    // Process holdtime parameter
    holdtimeMs = 0;
    std::string strHoldTime;
    if (pButton->QueryValueAttribute(JOYSTICK_XML_ATTR_HOLDTIME, &strHoldTime) == TIXML_SUCCESS)
    {
      std::istringstream ss(std::move(strHoldTime));
      ss >> holdtimeMs;
    }

    // Process hotkeys
    hotkeys.clear();
    std::string strHotkeys;
    if (pButton->QueryValueAttribute(JOYSTICK_XML_ATTR_HOTKEY, &strHotkeys) == TIXML_SUCCESS)
    {
      std::vector<std::string> vecHotkeys = StringUtils::Split(strHotkeys, ",");
      for (auto& hotkey : vecHotkeys)
        hotkeys.insert(std::move(hotkey));
    }

    return true;
  }

  return false;
}
