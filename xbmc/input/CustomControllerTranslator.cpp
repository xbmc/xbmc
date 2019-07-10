/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CustomControllerTranslator.h"

#include "WindowTranslator.h" //! @todo
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

void CCustomControllerTranslator::MapActions(int windowID, const TiXmlNode *pCustomController)
{
  CustomControllerButtonMap buttonMap;
  std::string controllerName;

  const TiXmlElement *pController = pCustomController->ToElement();
  if (pController != nullptr)
  {
    // Transform loose name to new family, including altnames
    const char *name = pController->Attribute("name");
    if (name != nullptr)
      controllerName = name;
  }

  if (controllerName.empty())
  {
    CLog::Log(LOGERROR, "Missing attribute \"name\" for tag \"customcontroller\"");
    return;
  }

  // Parse map
  const TiXmlElement *pButton = pCustomController->FirstChildElement();
  int id = 0;
  while (pButton != nullptr)
  {
    std::string action;
    if (!pButton->NoChildren())
      action = pButton->FirstChild()->ValueStr();

    if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS) && id >= 0)
    {
      buttonMap[id] = action;
    }
    else
      CLog::Log(LOGERROR, "Error reading customController map element, Invalid id: %d", id);

    pButton = pButton->NextSiblingElement();
  }

  // Add/overwrite button with mapped actions
  for (auto button : buttonMap)
    m_customControllersMap[controllerName][windowID][button.first] = std::move(button.second);
}

void CCustomControllerTranslator::Clear()
{
  m_customControllersMap.clear();
}

bool CCustomControllerTranslator::TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction)
{
  unsigned int actionId = ACTION_NONE;

  // handle virtual windows
  windowId = CWindowTranslator::GetVirtualWindow(windowId);

  // Try to get the action from the current window
  if (!TranslateString(windowId, controllerName, buttonId, actionId, strAction))
  {
    // if it's invalid, try to get it from fallback windows or the global map (windowId == -1)
    while (actionId == ACTION_NONE && windowId > -1)
    {
      windowId = CWindowTranslator::GetFallbackWindow(windowId);
      TranslateString(windowId, controllerName, buttonId, actionId, strAction);
    }
  }

  action = actionId;
  return actionId != ACTION_NONE;
}

bool CCustomControllerTranslator::TranslateString(int windowId, const std::string& controllerName, int buttonId, unsigned int& actionId, std::string& strAction)
{
  // Resolve the correct custom controller
  auto it = m_customControllersMap.find(controllerName);
  if (it == m_customControllersMap.end())
    return false;

  const CustomControllerWindowMap &windowMap = it->second;
  auto it2 = windowMap.find(windowId);
  if (it2 != windowMap.end())
  {
    const CustomControllerButtonMap &buttonMap = it2->second;
    auto it3 = buttonMap.find(buttonId);
    if (it3 != buttonMap.end())
    {
      strAction = it3->second;
      CActionTranslator::TranslateString(strAction, actionId);
    }
  }

  return actionId != ACTION_NONE;
}
