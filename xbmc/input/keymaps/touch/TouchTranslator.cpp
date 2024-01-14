/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TouchTranslator.h"

#include "input/WindowTranslator.h" //! @todo
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <map>

#include <tinyxml2.h>

using namespace KODI;
using namespace KEYMAP;

namespace
{
using ActionName = std::string;
using TouchCommandID = unsigned int;
} // namespace

#define TOUCH_COMMAND_NONE 0

static const std::map<ActionName, TouchCommandID> TouchCommands = {
    {"tap", ACTION_TOUCH_TAP},
    {"longpress", ACTION_TOUCH_LONGPRESS},
    {"pan", ACTION_GESTURE_PAN},
    {"zoom", ACTION_GESTURE_ZOOM},
    {"rotate", ACTION_GESTURE_ROTATE},
    {"swipeleft", ACTION_GESTURE_SWIPE_LEFT},
    {"swiperight", ACTION_GESTURE_SWIPE_RIGHT},
    {"swipeup", ACTION_GESTURE_SWIPE_UP},
    {"swipedown", ACTION_GESTURE_SWIPE_DOWN}};

void CTouchTranslator::MapActions(int windowID, const tinyxml2::XMLNode* pTouch)
{
  if (pTouch == nullptr)
    return;

  TouchActionMap map;

  // Check if there already is a touch map for the window ID
  auto it = m_touchMap.find(windowID);
  if (it != m_touchMap.end())
  {
    // Get the existing touch map and remove it from the window mapping as it
    // will be inserted later on
    map = std::move(it->second);
    m_touchMap.erase(it);
  }

  const auto* pTouchElem = pTouch->ToElement();
  if (pTouchElem == nullptr)
    return;

  const auto* pButton = pTouchElem->FirstChildElement();
  while (pButton != nullptr)
  {
    CTouchAction action;
    unsigned int touchActionKey = TranslateTouchCommand(pButton, action);
    if (touchActionKey != ACTION_NONE)
    {
      // check if there already is a mapping for the parsed action
      // and remove it if necessary
      TouchActionMap::iterator actionIt = map.find(touchActionKey);
      if (actionIt != map.end())
        map.erase(actionIt);

      map.insert(std::make_pair(touchActionKey, std::move(action)));
    }

    pButton = pButton->NextSiblingElement();
  }

  // add the modified touch map with the window ID
  if (!map.empty())
    m_touchMap.insert(std::make_pair(windowID, std::move(map)));
}

void CTouchTranslator::Clear()
{
  m_touchMap.clear();
}

bool CTouchTranslator::TranslateTouchAction(
    int window, int touchAction, int touchPointers, int& action, std::string& actionString)
{
  if (touchAction < 0)
    return false;

  unsigned int actionId = ACTION_NONE;

  // handle virtual windows
  window = CWindowTranslator::GetVirtualWindow(window);

  if (!TranslateAction(window, touchAction, touchPointers, actionId, actionString))
  {
    // if it's invalid, try to get it from fallback windows or the global map (window == -1)
    while (actionId == ACTION_NONE && window > -1)
    {
      window = CWindowTranslator::GetFallbackWindow(window);
      TranslateAction(window, touchAction, touchPointers, actionId, actionString);
    }
  }

  action = actionId;
  return actionId != ACTION_NONE;
}

bool CTouchTranslator::TranslateAction(int window,
                                       unsigned int touchCommand,
                                       int touchPointers,
                                       unsigned int& actionId,
                                       std::string& actionString)
{
  unsigned int touchActionKey = GetTouchActionKey(touchCommand, touchPointers);

  actionId = GetActionID(window, touchActionKey, actionString);

  return actionId != ACTION_NONE;
}

unsigned int CTouchTranslator::GetActionID(WindowID window,
                                           TouchActionKey touchActionKey,
                                           std::string& actionString)
{
  auto windowIt = m_touchMap.find(window);
  if (windowIt == m_touchMap.end())
    return ACTION_NONE;

  auto touchIt = windowIt->second.find(touchActionKey);
  if (touchIt == windowIt->second.end())
    return ACTION_NONE;

  actionString = touchIt->second.strAction;
  return touchIt->second.actionId;
}

unsigned int CTouchTranslator::TranslateTouchCommand(const tinyxml2::XMLElement* pButton,
                                                     CTouchAction& action)
{
  const char* szButton = pButton->Value();
  if (szButton == nullptr || pButton->FirstChild() == nullptr)
    return ACTION_NONE;

  const char* szAction = pButton->FirstChild()->Value();
  if (szAction == nullptr)
    return ACTION_NONE;

  std::string strTouchCommand = szButton;
  StringUtils::ToLower(strTouchCommand);

  // Handle direction
  const char* attrVal = pButton->Attribute("direction");
  if (attrVal != nullptr)
    strTouchCommand += attrVal;

  // Lookup command
  unsigned int touchCommandId = TOUCH_COMMAND_NONE;
  auto it = TouchCommands.find(strTouchCommand);
  if (it != TouchCommands.end())
    touchCommandId = it->second;

  if (touchCommandId == TOUCH_COMMAND_NONE)
  {
    CLog::Log(LOGERROR, "{}: Can't find touch command {}", __FUNCTION__, szButton);
    return ACTION_NONE;
  }

  // Handle pointers
  int pointers = 1;
  attrVal = pButton->Attribute("pointers");
  if (attrVal != nullptr)
    pointers = (int)strtol(attrVal, nullptr, 0);

  unsigned int touchActionKey = GetTouchActionKey(touchCommandId, pointers);

  action.strAction = szAction;
  if (!ACTION::CActionTranslator::TranslateString(action.strAction, action.actionId) ||
      action.actionId == ACTION_NONE)
    return ACTION_NONE;

  return touchActionKey;
}

unsigned int CTouchTranslator::GetTouchActionKey(unsigned int touchCommandId, int touchPointers)
{
  if (touchPointers <= 0)
    touchPointers = 1;

  return touchCommandId + touchPointers - 1;
}
