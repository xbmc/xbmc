/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "Peripheral.h"
#include "AddonManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "filesystem/Directory.h"
#include "guilib/WindowIDs.h"

#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
#include "input/SDLJoystick.h"
#endif

using namespace std;
using namespace XFILE;

namespace ADDON
{

CPeripheral::CPeripheral(const cp_extension_t *ext)
  : CAddon(ext), m_conditional(false), m_reference(false)
{
  if (ext)
  {
    m_conditional = CAddonMgr::Get().GetExtValue(ext->configuration, "@conditional").Equals("true");
    m_reference = CAddonMgr::Get().GetExtValue(ext->configuration, "@reference").Equals("true");
  }
}


bool CPeripheral::LoadKeymaps()
{
  CStdString path = URIUtils::AddFileToFolder(Path(),"keymaps/");
  CStdString path2 = URIUtils::AddFileToFolder(Profile(),"keymaps/");

  if (!CDirectory::Exists(path) && !CDirectory::Exists(path2))
    return true;

  CFileItemList files;
  XFILE::CDirectory::GetDirectory(path, files, ".xml");
  if (CDirectory::Exists(path2))
  {
    CFileItemList files2;
    XFILE::CDirectory::GetDirectory(path2, files2, ".xml");
    files.Append(files2);
  }

  bool result=false;
  for (int i=0;i<files.Size();++i)
  {
    CStdString keymapPath = files[i]->GetPath();
    CXBMCTinyXML xmlDoc;
    CLog::Log(LOGINFO, "Loading %s", keymapPath.c_str());
    if (!xmlDoc.LoadFile(keymapPath))
    {
      CLog::Log(LOGERROR, "Error loading keymap: %s, Line %d\n%s", 
                keymapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
      continue;
    }
    TiXmlElement* pRoot = xmlDoc.RootElement();
    if (!pRoot)
    {
      CLog::Log(LOGERROR, "Error getting keymap root: %s", keymapPath.c_str());
      continue;
    }
    CStdString strValue = pRoot->Value();
    if ( strValue != "keymap")
    {
      CLog::Log(LOGERROR, "%s Doesn't contain <keymap>", keymapPath.c_str());
      continue;
    }
    // run through our window groups
    vector<string> names;
    string type = pRoot->Attribute("device");
    TiXmlNode* pWindow = pRoot->FirstChild();
    while (pWindow)
    {
      if (pWindow->Type() == TiXmlNode::TINYXML_ELEMENT)
      {
        int windowID = WINDOW_INVALID;
        const char *szWindow = pWindow->Value();
        if (szWindow)
        {
          if (strcasecmp(szWindow, "global") == 0)
            windowID = -1;
          else if (strcasecmp(szWindow, "alias") == 0)
          {
            names.push_back(pWindow->FirstChild()->Value());
            pWindow = pWindow->NextSibling();
            continue;
          }
          else
            windowID = CButtonTranslator::TranslateWindow(szWindow);
        }
        if (type == "gamepad"    || type == "remote" ||
            type == "mouse"      || type == "keyboard" ||
            type == "appcommand" || type == "universalremote")
        {
          MapWindowActions(pWindow, windowID, type);
        }
        else if (type == "touchscreen")
          MapTouchActions(windowID, pWindow);
#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
        else if (type == "joystick")
        {
          MapJoystickActions(windowID, pWindow, names);
        }
#endif
        result = true;
      }
      pWindow = pWindow->NextSibling();
    }
  }

  return result;
}

void CPeripheral::MapWindowActions(TiXmlNode *pWindow, int windowID,
                                   const std::string& type)
{
  if (!pWindow || windowID == WINDOW_INVALID)
    return;

  CButtonTranslator::buttonMap map;
  std::map<int, CButtonTranslator::buttonMap>::iterator it = m_translatorMap.find(windowID);
  if (it != m_translatorMap.end())
  {
    map = it->second;
    m_translatorMap.erase(it);
  }

  TiXmlElement *pButton = pWindow->FirstChildElement();

  while (pButton)
  {
    uint32_t buttonCode=0;
    if (type == "remote")
      buttonCode = CButtonTranslator::TranslateRemoteString(pButton->Value());
    else if (type == "universalremote")
      buttonCode = CButtonTranslator::TranslateUniversalRemoteString(pButton->Value());
    else if (type == "keyboard")
      buttonCode = CButtonTranslator::TranslateKeyboardButton(pButton);
    else if (type == "mouse")
      buttonCode = CButtonTranslator::TranslateMouseCommand(pButton->Value());
    else if (type == "appcommand")
      buttonCode = CButtonTranslator::TranslateAppCommand(pButton->Value());

    if (buttonCode && pButton->FirstChild())
      CButtonTranslator::MapAction(buttonCode, pButton->FirstChild()->Value(), map);
    pButton = pButton->NextSiblingElement();
  }

  // add our map to our table
  if (map.size() > 0)
    m_translatorMap.insert(make_pair(windowID, map));
}

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
void CPeripheral::MapJoystickActions(int windowID, TiXmlNode *pJoystick,
                                     const vector<string>& names)
{
  map<int, string> buttonMap;
  map<int, string> axisMap;
  map<int, string> hatMap;

  // parse map
  TiXmlElement *pButton = pJoystick->FirstChildElement();
  int id = 0;
  const char* szType;
  const char *szAction;
  while (pButton)
  {
    szType = pButton->Value();
    szAction = pButton->GetText();
    if (szAction == NULL)
      szAction = "";
    if (szType)
    {
      if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS) && id>=0 && id<=256)
      {
        if (strcmpi(szType, "button")==0)
        {
          buttonMap[id] = string(szAction);
        }
        else if (strcmpi(szType, "axis")==0)
        {
          int limit = 0;
          if (pButton->QueryIntAttribute("limit", &limit) == TIXML_SUCCESS)
          {
            if (limit==-1)
              axisMap[-id] = string(szAction);
            else if (limit==1)
              axisMap[id] = string(szAction);
            else if (limit==0)
              axisMap[id|0xFFFF0000] = string(szAction);
            else
            {
              axisMap[id] = string(szAction);
              axisMap[-id] = string(szAction);
              CLog::Log(LOGERROR, "Error in joystick map, invalid limit specified %d for axis %d", limit, id);
            }
          }
          else
          {
            axisMap[id] = string(szAction);
            axisMap[-id] = string(szAction);
          }
        }
        else if (strcmpi(szType, "hat")==0)
        {
          string position;
          if (pButton->QueryValueAttribute("position", &position) == TIXML_SUCCESS)
          {
            uint32_t hatID = id|0xFFF00000;
            if (position.compare("up") == 0)
              hatMap[(JACTIVE_HAT_UP<<16)|hatID] = string(szAction);
            else if (position.compare("down") == 0)
              hatMap[(JACTIVE_HAT_DOWN<<16)|hatID] = string(szAction);
            else if (position.compare("right") == 0)
              hatMap[(JACTIVE_HAT_RIGHT<<16)|hatID] = string(szAction);
            else if (position.compare("left") == 0)
              hatMap[(JACTIVE_HAT_LEFT<<16)|hatID] = string(szAction);
            else
              CLog::Log(LOGERROR, "Error in joystick map, invalid position specified %s for axis %d", position.c_str(), id);
          }
        }
        else
          CLog::Log(LOGERROR, "Error reading joystick map element, unknown button type: %s", szType);
      }
      else
        CLog::Log(LOGERROR, "Error reading joystick map element, Invalid id: %d", id);
    }
    else
      CLog::Log(LOGERROR, "Error reading joystick map element, skipping");

    pButton = pButton->NextSiblingElement();
  }
  vector<string>::const_iterator it = names.begin();
  while (it!=names.end())
  {
    m_joystickButtonMap[*it][windowID] = buttonMap;
    m_joystickAxisMap[*it][windowID] = axisMap;
    m_joystickHatMap[*it][windowID] = hatMap;
    it++;
  }
}
#endif

void CPeripheral::MapTouchActions(int windowID, TiXmlNode *pTouch)
{
  if (pTouch == NULL)
    return;

  CButtonTranslator::buttonMap map;
  // check if there already is a touch map for the window ID
  std::map<int, CButtonTranslator::buttonMap>::iterator it = m_touchMap.find(windowID);
  if (it != m_touchMap.end())
  {
    // get the existing touch map and remove it from the window mapping
    // as it will be inserted later on
    map = it->second;
    m_touchMap.erase(it);
  }

  uint32_t actionId = 0;
  TiXmlElement *pTouchElem = pTouch->ToElement();
  if (pTouchElem == NULL)
    return;

  TiXmlElement *pButton = pTouchElem->FirstChildElement();
  while (pButton != NULL)
  {
    CButtonAction action;
    actionId = CButtonTranslator::TranslateTouchCommand(pButton, action);
    if (actionId > 0)
    {
      // check if there already is a mapping for the parsed action
      // and remove it if necessary
      CButtonTranslator::buttonMap::iterator actionIt = map.find(actionId);
      if (actionIt != map.end())
        map.erase(actionIt);

      map.insert(std::make_pair(actionId, action));
    }

    pButton = pButton->NextSiblingElement();
  }

  // add the modified touch map with the window ID
  if (map.size() > 0)
    m_touchMap.insert(make_pair(windowID, map));
}

}
