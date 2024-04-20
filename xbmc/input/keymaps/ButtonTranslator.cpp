/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ButtonTranslator.h"

#include "AppTranslator.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "guilib/WindowIDs.h"
#include "input/WindowTranslator.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "input/keyboard/Key.h"
#include "input/keyboard/KeyIDs.h"
#include "input/keymaps/interfaces/IKeyMapper.h"
#include "input/keymaps/joysticks/GamepadTranslator.h"
#include "input/keymaps/keyboard/KeyboardTranslator.h"
#include "input/keymaps/remote/CustomControllerTranslator.h"
#include "input/keymaps/remote/IRTranslator.h"
#include "input/mouse/MouseTranslator.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace KEYMAP;

// Add the supplied device name to the list of connected devices
bool CButtonTranslator::AddDevice(const std::string& strDevice)
{
  // Only add the device if it isn't already in the list
  if (m_deviceList.find(strDevice) != m_deviceList.end())
    return false;

  // Add the device
  m_deviceList.insert(strDevice);

  // New device added so reload the key mappings
  Load();

  return true;
}

bool CButtonTranslator::RemoveDevice(const std::string& strDevice)
{
  // Find the device
  auto it = m_deviceList.find(strDevice);
  if (it == m_deviceList.end())
    return false;

  // Remove the device
  m_deviceList.erase(it);

  // Device removed so reload the key mappings
  Load();

  return true;
}

bool CButtonTranslator::Load()
{
  Clear();

  // Directories to search for keymaps. They're applied in this order,
  // so keymaps in profile/keymaps/ override e.g. system/keymaps
  static std::vector<std::string> DIRS_TO_CHECK = {"special://xbmc/system/keymaps/",
                                                   "special://masterprofile/keymaps/",
                                                   "special://profile/keymaps/"};

  bool success = false;

  for (const auto& dir : DIRS_TO_CHECK)
  {
    if (XFILE::CDirectory::Exists(dir))
    {
      CFileItemList files;
      XFILE::CDirectory::GetDirectory(dir, files, ".xml", XFILE::DIR_FLAG_DEFAULTS);
      // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
      files.Sort(SortByFile, SortOrderAscending);
      for (int fileIndex = 0; fileIndex < files.Size(); ++fileIndex)
      {
        if (!files[fileIndex]->m_bIsFolder)
          success |= LoadKeymap(files[fileIndex]->GetPath());
      }

      // Load mappings for any HID devices we have connected
      for (const auto& device : m_deviceList)
      {
        std::string devicedir = dir;
        devicedir.append(device);
        devicedir.append("/");
        if (XFILE::CDirectory::Exists(devicedir))
        {
          CFileItemList files;
          XFILE::CDirectory::GetDirectory(devicedir, files, ".xml", XFILE::DIR_FLAG_DEFAULTS);
          // Sort the list for filesystem based priorities, e.g. 01-keymap.xml,
          // 02-keymap-overrides.xml
          files.Sort(SortByFile, SortOrderAscending);
          for (int fileIndex = 0; fileIndex < files.Size(); ++fileIndex)
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
    CLog::Log(LOGERROR, "Error loading keymaps from: {} or {} or {}", DIRS_TO_CHECK[0],
              DIRS_TO_CHECK[1], DIRS_TO_CHECK[2]);
    return false;
  }

  // Done!
  return true;
}

bool CButtonTranslator::LoadKeymap(const std::string& keymapPath)
{
  CXBMCTinyXML2 xmlDoc;

  CLog::Log(LOGINFO, "Loading {}", keymapPath);
  if (!xmlDoc.LoadFile(keymapPath))
  {
    CLog::Log(LOGERROR, "Error loading keymap: {}, Line {}\n{}", keymapPath, xmlDoc.ErrorLineNum(),
              xmlDoc.ErrorStr());
    return false;
  }

  auto* pRoot = xmlDoc.RootElement();
  if (pRoot == nullptr)
  {
    CLog::Log(LOGERROR, "Error getting keymap root: {}", keymapPath);
    return false;
  }

  std::string strValue = pRoot->Value();
  if (strValue != "keymap")
  {
    CLog::Log(LOGERROR, "{} Doesn't contain <keymap>", keymapPath);
    return false;
  }

  // run through our window groups
  auto* pWindow = pRoot->FirstChild();
  while (pWindow != nullptr)
  {
    if (pWindow->ToElement())
    {
      int windowID = WINDOW_INVALID;
      const char* szWindow = pWindow->Value();
      if (szWindow != nullptr)
      {
        if (StringUtils::CompareNoCase(szWindow, "global") == 0)
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

CAction CButtonTranslator::GetAction(int window, const CKey& key, bool fallback)
{
  std::string strAction;

  // handle virtual windows
  window = CWindowTranslator::GetVirtualWindow(window);

  // try to get the action from the current window
  unsigned int actionID = GetActionCode(window, key, strAction);

  if (fallback)
  {
    // if it's invalid, try to get it from fallback windows or the global map (window == -1)
    while (actionID == ACTION_NONE && window > -1)
    {
      window = CWindowTranslator::GetFallbackWindow(window);
      actionID = GetActionCode(window, key, strAction);
    }
  }

  return CAction(actionID, strAction, key);
}

bool CButtonTranslator::HasLongpressMapping(int window, const CKey& key)
{
  // handle virtual windows
  window = CWindowTranslator::GetVirtualWindow(window);
  return HasLongpressMapping_Internal(window, key);
}

bool CButtonTranslator::HasLongpressMapping_Internal(int window, const CKey& key)
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
    if (fallbackWindow > -1 && HasLongpressMapping_Internal(fallbackWindow, key))
      return true;

    // fallback to default section
    return HasLongpressMapping_Internal(-1, key);
  }

  return false;
}

unsigned int CButtonTranslator::GetActionCode(int window,
                                              const CKey& key,
                                              std::string& strAction) const
{
  uint32_t code = key.GetButtonCode();

  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return ACTION_NONE;

  buttonMap::const_iterator it2 = (*it).second.find(code);
  unsigned int action = ACTION_NONE;
  if (it2 == (*it).second.end() &&
      code & CKey::MODIFIER_LONG) // If long action not found, try short one
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
    CLog::Log(LOGDEBUG, "{}: Trying Hardy keycode for {:#04x}", __FUNCTION__, code);
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

void CButtonTranslator::MapAction(uint32_t buttonCode, const std::string& szAction, buttonMap& map)
{
  unsigned int action = ACTION_NONE;
  if (!ACTION::CActionTranslator::TranslateString(szAction, action) || buttonCode == 0)
    return; // no valid action, or an invalid buttoncode

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

void CButtonTranslator::MapWindowActions(const tinyxml2::XMLNode* pWindow, int windowID)
{
  if (pWindow == nullptr || windowID == WINDOW_INVALID)
    return;

  static const std::vector<std::string> types = {"gamepad",  "remote", "universalremote",
                                                 "keyboard", "mouse",  "appcommand"};

  for (const auto& type : types)
  {
    for (auto* pDevice = pWindow->FirstChildElement(type.c_str()); pDevice != nullptr;
         pDevice = pDevice->NextSiblingElement(type.c_str()))
    {
      buttonMap map;
      std::map<int, buttonMap>::iterator it = m_translatorMap.find(windowID);
      if (it != m_translatorMap.end())
      {
        map = std::move(it->second);
        m_translatorMap.erase(it);
      }

      const auto* pButton = pDevice->FirstChildElement();

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

  for (const auto& it : m_buttonMappers)
  {
    const std::string& device = it.first;
    IKeyMapper* mapper = it.second;

    // Map device actions
    auto* pDevice = pWindow->FirstChildElement(device.c_str());
    while (pDevice)
    {
      mapper->MapActions(windowID, pDevice);
      pDevice = pDevice->NextSiblingElement(device.c_str());
    }
  }
}

void CButtonTranslator::Clear()
{
  m_translatorMap.clear();

  for (auto it : m_buttonMappers)
    it.second->Clear();
}

void CButtonTranslator::RegisterMapper(const std::string& device, IKeyMapper* mapper)
{
  m_buttonMappers[device] = mapper;
}

void CButtonTranslator::UnregisterMapper(const IKeyMapper* mapper)
{
  for (auto it = m_buttonMappers.begin(); it != m_buttonMappers.end(); ++it)
  {
    if (it->second == mapper)
    {
      m_buttonMappers.erase(it);
      break;
    }
  }
}

uint32_t CButtonTranslator::TranslateString(const std::string& strMap, const std::string& strButton)
{
  if (strMap == "KB") // standard keyboard map
  {
    return CKeyboardTranslator::TranslateString(strButton);
  }
  else if (strMap == "XG") // xbox gamepad map
  {
    return CGamepadTranslator::TranslateString(strButton);
  }
  else if (strMap == "R1") // xbox remote map
  {
    return CIRTranslator::TranslateString(strButton);
  }
  else if (strMap == "R2") // xbox universal remote map
  {
    return CIRTranslator::TranslateUniversalRemoteString(strButton);
  }
  else
  {
    return 0;
  }
}
