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

#include "CustomControllerTranslator.h"
#include "ActionIDs.h"
#include "ActionTranslator.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"

void CCustomControllerTranslator::MapCustomControllerActions(int windowID, const TiXmlNode *pCustomController)
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

bool CCustomControllerTranslator::TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, unsigned int& actionId, std::string& strAction)
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
      CActionTranslator::TranslateActionString(strAction.c_str(), actionId);
    }
  }

  return actionId != ACTION_NONE;
}
