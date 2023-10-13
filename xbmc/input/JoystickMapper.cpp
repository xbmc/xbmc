/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JoystickMapper.h"

#include "input/WindowKeymap.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

#include <tinyxml2.h>

using namespace KODI;

#define JOYSTICK_XML_NODE_PROFILE "profile"
#define JOYSTICK_XML_ATTR_DIRECTION "direction"
#define JOYSTICK_XML_ATTR_HOLDTIME "holdtime"
#define JOYSTICK_XML_ATTR_HOTKEY "hotkey"

void CJoystickMapper::MapActions(int windowID, const tinyxml2::XMLNode* pDevice)
{
  std::string controllerId;
  DeserializeJoystickNode(pDevice, controllerId);
  if (controllerId.empty())
    return;

  // Update Controller IDs
  if (std::find(m_controllerIds.begin(), m_controllerIds.end(), controllerId) ==
      m_controllerIds.end())
    m_controllerIds.emplace_back(controllerId);

  // Create/overwrite keymap
  auto& keymap = m_joystickKeymaps[controllerId];
  if (!keymap)
    keymap = std::make_shared<CWindowKeymap>(controllerId);

  const auto* pButton = pDevice->FirstChildElement();
  while (pButton != nullptr)
  {
    std::string feature;
    JOYSTICK::ANALOG_STICK_DIRECTION dir;
    unsigned int holdtimeMs;
    std::set<std::string> hotkeys;
    std::string actionString;
    if (DeserializeButton(pButton, feature, dir, holdtimeMs, hotkeys, actionString))
    {
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
        keymap->MapAction(windowID, JOYSTICK::CJoystickUtils::MakeKeyName(feature, dir),
                          std::move(action));
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

  for (const auto& controllerId : m_controllerIds)
  {
    auto it = m_joystickKeymaps.find(controllerId);
    if (it != m_joystickKeymaps.end())
      keymaps.emplace_back(it->second);
  }

  return keymaps;
}

void CJoystickMapper::DeserializeJoystickNode(const tinyxml2::XMLNode* pDevice,
                                              std::string& controllerId)
{
  const auto* deviceElem = pDevice->ToElement();
  if (deviceElem != nullptr)
  {
    const char* controller;
    if (deviceElem->QueryStringAttribute(JOYSTICK_XML_NODE_PROFILE, &controller) ==
        tinyxml2::XML_SUCCESS)
      controllerId = controller;
  }
}

bool CJoystickMapper::DeserializeButton(const tinyxml2::XMLElement* pButton,
                                        std::string& feature,
                                        JOYSTICK::ANALOG_STICK_DIRECTION& dir,
                                        unsigned int& holdtimeMs,
                                        std::set<std::string>& hotkeys,
                                        std::string& actionStr)
{
  const char* szButton = pButton->Value();
  if (szButton != nullptr)
  {
    const char* szAction = nullptr;

    const auto* actionNode = pButton->FirstChild();
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
    dir = JOYSTICK::ANALOG_STICK_DIRECTION::NONE;
    const char* szDirection = pButton->Attribute(JOYSTICK_XML_ATTR_DIRECTION);
    if (szDirection != nullptr)
      dir = JOYSTICK::CJoystickTranslator::TranslateAnalogStickDirection(szDirection);

    // Process holdtime parameter
    holdtimeMs = 0;
    const char* strHoldTime;
    if (pButton->QueryStringAttribute(JOYSTICK_XML_ATTR_HOLDTIME, &strHoldTime) ==
        tinyxml2::XML_SUCCESS)
    {
      std::istringstream ss(strHoldTime);
      ss >> holdtimeMs;
    }

    // Process hotkeys
    hotkeys.clear();
    const char* strHotkeys;
    if (pButton->QueryStringAttribute(JOYSTICK_XML_ATTR_HOTKEY, &strHotkeys) ==
        tinyxml2::XML_SUCCESS)
    {
      std::vector<std::string> vecHotkeys = StringUtils::Split(strHotkeys, ",");
      for (auto& hotkey : vecHotkeys)
        hotkeys.insert(std::move(hotkey));
    }

    return true;
  }

  return false;
}
