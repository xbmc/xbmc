/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "ActionIDs.h"
#include "ActionTranslator.h"
#include "AppTranslator.h"
#include "CustomControllerTranslator.h"
#include "GamepadTranslator.h"
#include "IRTranslator.h"
#include "Key.h"
#include "KeyboardTranslator.h"
#include "KeymapEnvironment.h"
#include "MouseTranslator.h"
#include "TouchTranslator.h"
#include "WindowKeymap.h"
#include "WindowTranslator.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickIDs.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

using namespace KODI;

// --- Joystick XML deserialization --------------------------------------------

namespace
{
  void DeserializeJoystickNode(const TiXmlNode* pDevice, std::string &controllerId)
  {
    controllerId = DEFAULT_CONTROLLER_ID;

    const TiXmlElement* deviceElem = pDevice->ToElement();
    if (deviceElem != nullptr)
      deviceElem->QueryValueAttribute("profile", &controllerId);
  }

  bool DeserializeButton(const TiXmlElement *pButton, std::string &feature, JOYSTICK::ANALOG_STICK_DIRECTION &dir, unsigned int& holdtimeMs, std::set<std::string>& hotkeys, std::string &actionStr)
  {
    const char *szButton = pButton->Value();
    if (szButton != nullptr)
    {
      const char *szAction = pButton->FirstChild()->Value();
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
      const char *szDirection = pButton->Attribute("direction");
      if (szDirection != nullptr)
        dir = JOYSTICK::CJoystickTranslator::TranslateDirection(szDirection);

      // Process holdtime parameter
      holdtimeMs = 0;
      std::string strHoldTime;
      if (pButton->QueryValueAttribute("holdtime", &strHoldTime) == TIXML_SUCCESS)
      {
        std::stringstream ss(std::move(strHoldTime));
        ss >> holdtimeMs;
      }

      // Process hotkeys
      hotkeys.clear();
      std::string strHotkeys;
      if (pButton->QueryValueAttribute("hotkey", &strHotkeys) == TIXML_SUCCESS)
      {
        std::vector<std::string> vecHotkeys = StringUtils::Split(strHotkeys, ",");
        for (auto& hotkey : vecHotkeys)
          hotkeys.insert(std::move(hotkey));
      }
      
      return true;
    }

    return false;
  }
}

// --- CButtonTranslator -------------------------------------------------------

CButtonTranslator& CButtonTranslator::GetInstance()
{
  static CButtonTranslator sl_instance;
  return sl_instance;
}

CButtonTranslator::CButtonTranslator() :
  m_keymapEnvironment(new CKeymapEnvironment),
  m_customControllerTranslator(new CCustomControllerTranslator),
  m_irTranslator(new CIRTranslator),
  m_touchTranslator(new CTouchTranslator)
{
}

// Add the supplied device name to the list of connected devices
void CButtonTranslator::AddDevice(std::string& strDevice)
{
  // Only add the device if it isn't already in the list
  if (m_deviceList.find(strDevice) != m_deviceList.end())
    return;

  // Add the device
  m_deviceList.insert(strDevice);

  // New device added so reload the key mappings
  Load();
}

void CButtonTranslator::RemoveDevice(std::string& strDevice)
{
  // Find the device
  auto it = m_deviceList.find(strDevice);
  if (it == m_deviceList.end())
    return;

  // Remove the device
  m_deviceList.erase(it);

  // Device removed so reload the key mappings
  Load();
}

bool CButtonTranslator::Load(bool AlwaysLoad)
{
  m_translatorMap.clear();
  m_customControllerTranslator->Clear();

  // Directories to search for keymaps. They're applied in this order,
  // so keymaps in profile/keymaps/ override e.g. system/keymaps
  static std::vector<std::string> DIRS_TO_CHECK = {
    "special://xbmc/system/keymaps/",
    "special://masterprofile/keymaps/",
    "special://profile/keymaps/"
  };

  bool success = false;

  for (const auto& dir : DIRS_TO_CHECK)
  {
    if (XFILE::CDirectory::Exists(dir))
    {
      CFileItemList files;
      XFILE::CDirectory::GetDirectory(dir, files, ".xml");
      // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
      files.Sort(SortByFile, SortOrderAscending);
      for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
      {
        if (!files[fileIndex]->m_bIsFolder)
          success |= LoadKeymap(files[fileIndex]->GetPath());
      }

      // Load mappings for any HID devices we have connected
      std::list<std::string>::iterator it;
      for (const auto& device : m_deviceList)
      {
        std::string devicedir = dir;
        devicedir.append(device);
        devicedir.append("/");
        if (XFILE::CDirectory::Exists(devicedir))
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
    CLog::Log(LOGERROR, "Error loading keymaps from: %s or %s or %s",
      DIRS_TO_CHECK[0].c_str(), DIRS_TO_CHECK[1].c_str(), DIRS_TO_CHECK[2].c_str());
    return false;
  }

  m_irTranslator->Load();

  // Done!
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
  if (pRoot == nullptr)
  {
    CLog::Log(LOGERROR, "Error getting keymap root: %s", keymapPath.c_str());
    return false;
  }

  std::string strValue = pRoot->Value();
  if (strValue != "keymap")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <keymap>", keymapPath.c_str());
    return false;
  }

  // run through our window groups
  TiXmlNode* pWindow = pRoot->FirstChild();
  while (pWindow != nullptr)
  {
    if (pWindow->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      int windowID = WINDOW_INVALID;
      const char *szWindow = pWindow->Value();
      if (szWindow != nullptr)
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

int CButtonTranslator::TranslateLircRemoteString(const std::string &szDevice, const std::string &szButton)
{
  return m_irTranslator->TranslateButton(szDevice, szButton);
}

bool CButtonTranslator::TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction)
{
  unsigned int actionId = ACTION_NONE;

  // Try to get the action from the current window
  if (!m_customControllerTranslator->TranslateString(windowId, controllerName, buttonId, actionId, strAction))
  {
    // If it's invalid, try to get it from a fallback window or the global map
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(windowId);
    if (fallbackWindow > -1)
      m_customControllerTranslator->TranslateString(fallbackWindow, controllerName, buttonId, actionId, strAction);

    // Still no valid action? Use global map
    if (action == ACTION_NONE)
      m_customControllerTranslator->TranslateString(-1, controllerName, buttonId, actionId, strAction);
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

  if (!m_touchTranslator->TranslateAction(window, touchAction, touchPointers, actionId, actionString))
  {
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
    if (fallbackWindow > -1)
      m_touchTranslator->TranslateAction(fallbackWindow, touchAction, touchPointers, actionId, actionString);

    if (actionId == ACTION_NONE)
      m_touchTranslator->TranslateAction(-1, touchAction, touchPointers, actionId, actionString);
  }

  action = actionId;
  return actionId != ACTION_NONE;
}

CAction CButtonTranslator::GetAction(int window, const CKey &key, bool fallback)
{
  std::string strAction;

  // try to get the action from the current window
  unsigned int actionID = GetActionCode(window, key, strAction);

  // if it's invalid, try to get it from the global map
  if (actionID == ACTION_NONE && fallback)
  {
    //! @todo Refactor fallback logic
    int fallbackWindow = CWindowTranslator::GetFallbackWindow(window);
    if (fallbackWindow > -1)
      actionID = GetActionCode(fallbackWindow, key, strAction);

    // still no valid action? use global map
    if (actionID == ACTION_NONE)
      actionID = GetActionCode(-1, key, strAction);
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

unsigned int CButtonTranslator::GetActionCode(int window, const CKey &key, std::string &strAction) const
{
  uint32_t code = key.GetButtonCode();

  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return ACTION_NONE;

  buttonMap::const_iterator it2 = (*it).second.find(code);
  unsigned int action = ACTION_NONE;
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
  if (action == ACTION_NONE && (code & KEY_VKEY) == KEY_VKEY && (code & 0x0F00))
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

void CButtonTranslator::MapAction(uint32_t buttonCode, const std::string &szAction, buttonMap &map)
{
  unsigned int action = ACTION_NONE;
  if (!CActionTranslator::TranslateString(szAction, action) || buttonCode == 0)
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
    map.insert(std::pair<uint32_t, CButtonAction>(buttonCode, button));
  }
}

void CButtonTranslator::MapWindowActions(const TiXmlNode *pWindow, int windowID)
{
  if (pWindow == nullptr || windowID == WINDOW_INVALID) 
    return;

  const TiXmlNode *pDevice;

  static const std::vector<std::string> types = {"gamepad", "remote", "universalremote", "keyboard", "mouse", "appcommand"};

  for (const auto& type : types)
  {
    for (pDevice = pWindow->FirstChild(type);
         pDevice != nullptr;
         pDevice = pDevice->NextSiblingElement(type))
    {
      buttonMap map;
      std::map<int, buttonMap>::iterator it = m_translatorMap.find(windowID);
      if (it != m_translatorMap.end())
      {
        map = std::move(it->second);
        m_translatorMap.erase(it);
      }

      const TiXmlElement *pButton = pDevice->FirstChildElement();

      while (pButton != nullptr)
      {
        uint32_t buttonCode = 0;

        if (type == "gamepad")
            buttonCode = CGamepadTranslator::TranslateString(pButton->Value());
        else if (type == "remote")
            buttonCode = CIRTranslator::TranslateString(pButton->Value());
        else if (type == "universalremote")
            buttonCode = CIRTranslator::TranslateUniversalRemoteString(pButton->Value());
        else if (type == "keyboard")
            buttonCode = CKeyboardTranslator::TranslateButton(pButton);
        else if (type == "mouse")
            buttonCode = CMouseTranslator::TranslateCommand(pButton);
        else if (type == "appcommand")
            buttonCode = CAppTranslator::TranslateAppCommand(pButton->Value());

        if (buttonCode != 0)
        {
          if (pButton->FirstChild() && pButton->FirstChild()->Value()[0])
            MapAction(buttonCode, pButton->FirstChild()->Value(), map);
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
        m_translatorMap.insert(std::make_pair(windowID, std::move(map)));
    }
  }

  // map touch actions
  pDevice = pWindow->FirstChild("touch");
  while (pDevice != nullptr)
  {
    m_touchTranslator->MapActions(windowID, pDevice);
    pDevice = pDevice->NextSibling("touch");
  }

  // map custom controller actions
  pDevice = pWindow->FirstChild("customcontroller");
  while (pDevice != nullptr)
  {
    m_customControllerTranslator->MapActions(windowID, pDevice);
    pDevice = pDevice->NextSibling("customcontroller");
  }

  // Map joystick actions
  pDevice = pWindow->FirstChild("joystick");
  while (pDevice != nullptr)
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

    pDevice = pDevice->NextSibling("joystick");
  }
}

void CButtonTranslator::Clear()
{
  m_translatorMap.clear();

  m_irTranslator->Clear();
  m_customControllerTranslator->Clear();
  m_touchTranslator->Clear();
  m_joystickKeymaps.clear();
  m_controllerIds.clear();
}

const IWindowKeymap *CButtonTranslator::JoystickKeymap(const std::string &controllerId) const
{
  IWindowKeymap *keymap = nullptr;

  auto it = m_joystickKeymaps.find(controllerId);
  if (it != m_joystickKeymaps.end())
    keymap = it->second.get();

  return keymap;
}
