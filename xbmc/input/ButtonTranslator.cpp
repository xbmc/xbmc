/*
 *      Copyright (C) 2005-2015 Team XBMC
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ButtonTranslator.h"

#include <algorithm>
#include <utility>

#include "ActionTranslator.h"
#include "AppTranslator.h"
#include "CustomControllerTranslator.h"
#include "GamepadTranslator.h"
#include "IRTranslator.h"
#include "JoystickTranslator.h"
#include "KeyboardTranslator.h"
#include "MouseTranslator.h"
#include "TouchTranslator.h"
#include "WindowTranslator.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/XBMCTinyXML.h"

using namespace XFILE;

CButtonTranslator& CButtonTranslator::GetInstance()
{
  static CButtonTranslator sl_instance;
  return sl_instance;
}

CButtonTranslator::CButtonTranslator() :
  m_customControllerTranslator(new CCustomControllerTranslator),
  m_irTranslator(new CIRTranslator),
  m_touchTranslator(new CTouchTranslator)
{
  m_deviceList.clear();
  m_Loaded = false;
}

CButtonTranslator::~CButtonTranslator()
{
}

// Add the supplied device name to the list of connected devices
void CButtonTranslator::AddDevice(std::string& strDevice)
{
  // Only add the device if it isn't already in the list
  std::list<std::string>::iterator it;
  for (it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
    if (*it == strDevice)
      return;

  // Add the device
  m_deviceList.push_back(strDevice);
  m_deviceList.sort();

  // New device added so reload the key mappings
  Load();
}

void CButtonTranslator::RemoveDevice(std::string& strDevice)
{
  // Find the device
  std::list<std::string>::iterator it;
  for (it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
    if (*it == strDevice)
      break;
  if (it == m_deviceList.end())
    return;

  // Remove the device
  m_deviceList.remove(strDevice);

  // Device removed so reload the key mappings
  Load();
}

bool CButtonTranslator::Load(bool AlwaysLoad)
{
  m_translatorMap.clear();
  m_customControllerTranslator->Clear();

  // Directories to search for keymaps. They're applied in this order,
  // so keymaps in profile/keymaps/ override e.g. system/keymaps
  static const char* DIRS_TO_CHECK[] = {
    "special://xbmc/system/keymaps/",
    "special://masterprofile/keymaps/",
    "special://profile/keymaps/"
  };
  bool success = false;

  for (unsigned int dirIndex = 0; dirIndex < ARRAY_SIZE(DIRS_TO_CHECK); ++dirIndex)
  {
    if (XFILE::CDirectory::Exists(DIRS_TO_CHECK[dirIndex]))
    {
      CFileItemList files;
      XFILE::CDirectory::GetDirectory(DIRS_TO_CHECK[dirIndex], files, ".xml");
      // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
      files.Sort(SortByFile, SortOrderAscending);
      for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
      {
        if (!files[fileIndex]->m_bIsFolder)
          success |= LoadKeymap(files[fileIndex]->GetPath());
      }

      // Load mappings for any HID devices we have connected
      std::list<std::string>::iterator it;
      for (it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
      {
        std::string devicedir = DIRS_TO_CHECK[dirIndex];
        devicedir.append(*it);
        devicedir.append("/");
        if( XFILE::CDirectory::Exists(devicedir) )
        {
          CFileItemList files;
          XFILE::CDirectory::GetDirectory(devicedir, files, ".xml");
          // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
          files.Sort(SortByFile, SortOrderAscending);
          for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
          {
            if (!files[fileIndex]->m_bIsFolder)
              success |= LoadKeymap(files[fileIndex]->GetPath());
          }
        }
      }
    }
  }

  if (!success)
  {
    CLog::Log(LOGERROR, "Error loading keymaps from: %s or %s or %s", DIRS_TO_CHECK[0], DIRS_TO_CHECK[1], DIRS_TO_CHECK[2]);
    return false;
  }

  m_irTranslator->Load();

  // Done!
  m_Loaded = true;
  return true;
}

bool CButtonTranslator::LoadKeymap(const std::string &keymapPath)
{
  CXBMCTinyXML xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", keymapPath.c_str());
  if (!xmlDoc.LoadFile(keymapPath))
  {
    CLog::Log(LOGERROR, "Error loading keymap: %s, Line %d\n%s", keymapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  TiXmlElement* pRoot = xmlDoc.RootElement();
  if (!pRoot)
  {
    CLog::Log(LOGERROR, "Error getting keymap root: %s", keymapPath.c_str());
    return false;
  }
  std::string strValue = pRoot->Value();
  if ( strValue != "keymap")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <keymap>", keymapPath.c_str());
    return false;
  }
  // run through our window groups
  TiXmlNode* pWindow = pRoot->FirstChild();
  while (pWindow)
  {
    if (pWindow->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      int windowID = WINDOW_INVALID;
      const char *szWindow = pWindow->Value();
      if (szWindow)
      {
        if (strcmpi(szWindow, "global") == 0)
          windowID = -1;
        else
          windowID = CWindowTranslator::TranslateWindow(szWindow);
      }
      MapWindowActions(pWindow, windowID);
    }
    pWindow = pWindow->NextSibling();
  }

  return true;
}

int CButtonTranslator::TranslateLircRemoteString(const char* szDevice, const char *szButton)
{
  return m_irTranslator->TranslateIRRemoteString(szDevice, szButton);
}

bool CButtonTranslator::TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction)
{
  unsigned int actionId = ACTION_NONE;

  // Try to get the action from the current window
  if (!m_customControllerTranslator->TranslateCustomControllerString(windowId, controllerName, buttonId, actionId, strAction))
  {
    // If it's invalid, try to get it from a fallback window or the global map
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(windowId);
    if (fallbackWindow > -1)
      m_customControllerTranslator->TranslateCustomControllerString(fallbackWindow, controllerName, buttonId, actionId, strAction);

    // Still no valid action? Use global map
    if (action == ACTION_NONE)
      m_customControllerTranslator->TranslateCustomControllerString(-1, controllerName, buttonId, actionId, strAction);
  }

  if (actionId != ACTION_NONE)
  {
    action = actionId;
    return true;
  }

  return false;
}

bool CButtonTranslator::TranslateTouchAction(int window, int touchAction, int touchPointers, int &action, std::string &actionString)
{
  if (touchAction < 0)
    return false;

  unsigned int actionId = ACTION_NONE;

  if (!m_touchTranslator->TranslateTouchAction(window, touchAction, touchPointers, actionId, actionString))
  {
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
    if (fallbackWindow > -1)
      m_touchTranslator->TranslateTouchAction(fallbackWindow, touchAction, touchPointers, actionId, actionString);

    if (actionId == ACTION_NONE)
      m_touchTranslator->TranslateTouchAction(-1, touchAction, touchPointers, actionId, actionString);
  }

  action = actionId;
  return actionId != ACTION_NONE;
}

int CButtonTranslator::GetActionCode(int window, int action)
{
  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return 0;

  buttonMap::const_iterator it2 = it->second.find(action);
  if (it2 == it->second.end())
    return 0;

  return it2->second.id;
}

CAction CButtonTranslator::GetAction(int window, const CKey &key, bool fallback)
{
  std::string strAction;
  // try to get the action from the current window
  int actionID = GetActionCode(window, key, strAction);
  // if it's invalid, try to get it from the global map
  if (actionID == 0 && fallback)
  {
    //! @todo Refactor fallback logic
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
    if (fallbackWindow > -1)
      actionID = GetActionCode(fallbackWindow, key, strAction);
    // still no valid action? use global map
    if (actionID == 0)
      actionID = GetActionCode( -1, key, strAction);
  }
  // Now fill our action structure
  CAction action(actionID, strAction, key);
  return action;
}

CAction CButtonTranslator::GetGlobalAction(const CKey &key)
{
  return GetAction(-1, key, true);
}

bool CButtonTranslator::HasLongpressMapping(int window, const CKey &key)
{
  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it != m_translatorMap.end())
  {
    uint32_t code = key.GetButtonCode();
    code |= CKey::MODIFIER_LONG;
    buttonMap::const_iterator it2 = (*it).second.find(code);

    if (it2 != (*it).second.end())
      return it2->second.id != ACTION_NOOP;

#ifdef TARGET_POSIX
    // Some buttoncodes changed in Hardy
    if ((code & KEY_VKEY) == KEY_VKEY && (code & 0x0F00))
    {
      code &= ~0x0F00;
      it2 = (*it).second.find(code);
      if (it2 != (*it).second.end())
        return true;
    }
#endif
  }

  // no key mapping found for the current window do the fallback handling
  if (window > -1)
  {
    // first check if we have a fallback for the window
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
    if (fallbackWindow > -1 && HasLongpressMapping(fallbackWindow, key))
      return true;

    // fallback to default section
    return HasLongpressMapping(-1, key);
  }

  return false;
}

unsigned int CButtonTranslator::GetHoldTimeMs(int window, const CKey &key, bool fallback /* = true */)
{
  unsigned int holdtimeMs = 0;

  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it != m_translatorMap.end())
  {
    uint32_t code = key.GetButtonCode();

    buttonMap::const_iterator it2 = (*it).second.find(code);

    if (it2 != (*it).second.end())
    {
      holdtimeMs = (*it2).second.holdtimeMs;
    }
    else if (fallback)
    {
      //! @todo Refactor fallback logic
      int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
      if (fallbackWindow > -1)
        holdtimeMs = GetHoldTimeMs(fallbackWindow, key, false);
      else
      {
        // still no valid action? use global map
        holdtimeMs = GetHoldTimeMs(-1, key, false);
      }
    }
  }
  else if (fallback)
  {
    //! @todo Refactor fallback logic
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
    if (fallbackWindow > -1)
      holdtimeMs = GetHoldTimeMs(fallbackWindow, key, false);
    else
    {
      // still no valid action? use global map
      holdtimeMs = GetHoldTimeMs(-1, key, false);
    }
  }

  return holdtimeMs;
}

int CButtonTranslator::GetActionCode(int window, const CKey &key, std::string &strAction) const
{
  uint32_t code = key.GetButtonCode();

  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return 0;
  buttonMap::const_iterator it2 = (*it).second.find(code);
  int action = 0;
  if (it2 == (*it).second.end() && code & CKey::MODIFIER_LONG) // If long action not found, try short one
  {
    code &= ~CKey::MODIFIER_LONG;
    it2 = (*it).second.find(code);
  }
  if (it2 != (*it).second.end())
  {
    action = (*it2).second.id;
    strAction = (*it2).second.strID;
  }
#ifdef TARGET_POSIX
  // Some buttoncodes changed in Hardy
  if (action == 0 && (code & KEY_VKEY) == KEY_VKEY && (code & 0x0F00))
  {
    CLog::Log(LOGDEBUG, "%s: Trying Hardy keycode for %#04x", __FUNCTION__, code);
    code &= ~0x0F00;
    it2 = (*it).second.find(code);
    if (it2 != (*it).second.end())
    {
      action = (*it2).second.id;
      strAction = (*it2).second.strID;
    }
  }
#endif
  return action;
}

void CButtonTranslator::MapAction(uint32_t buttonCode, const char *szAction, unsigned int holdtimeMs, buttonMap &map)
{
  unsigned int action = ACTION_NONE;
  if (!CActionTranslator::TranslateActionString(szAction, action) || buttonCode == 0)
    return;   // no valid action, or an invalid buttoncode

  // have a valid action, and a valid button - map it.
  // check to see if we've already got this (button,action) pair defined
  buttonMap::iterator it = map.find(buttonCode);
  if (it == map.end() || (*it).second.id != action || (*it).second.strID != szAction)
  {
    // NOTE: This multimap is only being used as a normal map at this point (no support
    //       for multiple actions per key)
    if (it != map.end())
      map.erase(it);
    CButtonAction button;
    button.id = action;
    button.strID = szAction;
    button.holdtimeMs = holdtimeMs;
    map.insert(std::pair<uint32_t, CButtonAction>(buttonCode, button));
  }
}

void CButtonTranslator::MapWindowActions(TiXmlNode *pWindow, int windowID)
{
  if (!pWindow || windowID == WINDOW_INVALID) 
    return;

  TiXmlNode* pDevice;

  const char* types[] = {"gamepad", "remote", "universalremote", "keyboard", "mouse", "appcommand", "joystick", NULL};
  for (int i = 0; types[i]; ++i)
  {
    std::string type(types[i]);

    for (pDevice = pWindow->FirstChild(type);
         pDevice != nullptr;
         pDevice = pDevice->NextSiblingElement(type))
    {
      buttonMap map;
      std::map<int, buttonMap>::iterator it = m_translatorMap.find(windowID);
      if (it != m_translatorMap.end())
      {
        map = it->second;
        m_translatorMap.erase(it);
      }

      TiXmlElement *pButton = pDevice->FirstChildElement();

      while (pButton)
      {
        uint32_t buttonCode=0;
        unsigned int holdtimeMs = 0;

        if (type == "gamepad")
            buttonCode = CGamepadTranslator::TranslateGamepadString(pButton->Value());
        else if (type == "remote")
            buttonCode = CIRTranslator::TranslateRemoteString(pButton->Value());
        else if (type == "universalremote")
            buttonCode = CIRTranslator::TranslateUniversalRemoteString(pButton->Value());
        else if (type == "keyboard")
            buttonCode = CKeyboardTranslator::TranslateKeyboardButton(pButton);
        else if (type == "mouse")
            buttonCode = CMouseTranslator::TranslateMouseCommand(pButton);
        else if (type == "appcommand")
            buttonCode = CAppTranslator::TranslateAppCommand(pButton->Value());
        else if (type == "joystick")
          buttonCode = CJoystickTranslator::TranslateJoystickCommand(pDevice, pButton, holdtimeMs);

        if (buttonCode)
        {
          if (pButton->FirstChild() && pButton->FirstChild()->Value()[0])
            MapAction(buttonCode, pButton->FirstChild()->Value(), holdtimeMs, map);
          else
          {
            buttonMap::iterator it = map.find(buttonCode);
            while (it != map.end())
            {
              map.erase(it);
              it = map.find(buttonCode);
            }
          }
        }
        pButton = pButton->NextSiblingElement();
      }

      // add our map to our table
      if (!map.empty())
        m_translatorMap.insert(std::pair<int, buttonMap>( windowID, map));
    }
  }

  if ((pDevice = pWindow->FirstChild("touch")) != NULL)
  {
    // map touch actions
    while (pDevice)
    {
      m_touchTranslator->MapTouchActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("touch");
    }
  }

  if ((pDevice = pWindow->FirstChild("customcontroller")) != NULL)
  {
    // map custom controller actions
    while (pDevice)
    {
      m_customControllerTranslator->MapCustomControllerActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("customcontroller");
    }
  }

}

void CButtonTranslator::Clear()
{
  m_translatorMap.clear();

  m_irTranslator->Clear();
  m_customControllerTranslator->Clear();
  m_touchTranslator->Clear();

  m_Loaded = false;
}
