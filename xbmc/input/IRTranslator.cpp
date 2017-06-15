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

#include "IRTranslator.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "system.h"
#include "XBIRRemote.h"

#include <stdlib.h>
#include <vector>

void CIRTranslator::Load()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  Clear();

  std::string irMapName;
#ifdef TARGET_POSIX
  irMapName = "Lircmap.xml";
#else
  irMapName = "IRSSmap.xml";
#endif

  bool success = false;

  std::string irMapPath = URIUtils::AddFileToFolder("special://xbmc/system/", irMapName);
  if (XFILE::CFile::Exists(irMapPath))
    success |= LoadIRMap(irMapPath);
  else
    CLog::Log(LOGDEBUG, "CIRTranslator::Load - no system %s found, skipping", irMapName.c_str());

  irMapPath = CProfilesManager::GetInstance().GetUserDataItem(irMapName);
  if (XFILE::CFile::Exists(irMapPath))
    success |= LoadIRMap(irMapPath);
  else
    CLog::Log(LOGDEBUG, "CIRTranslator::Load - no userdata %s found, skipping", irMapName.c_str());

  if (!success)
    CLog::Log(LOGERROR, "CIRTranslator::Load - unable to load remote map %s", irMapName.c_str());
#endif
}

bool CIRTranslator::LoadIRMap(const std::string &irMapPath)
{
  std::string remoteMapTag;
#ifdef TARGET_POSIX
  remoteMapTag = "irmap";
#else
  remoteMapTag = "irssmap";
#endif

  // Load our xml file, and fill up our mapping tables
  CXBMCTinyXML xmlDoc;

  // Load the config file
  CLog::Log(LOGINFO, "Loading %s", irMapPath.c_str());
  if (!xmlDoc.LoadFile(irMapPath))
  {
    CLog::Log(LOGERROR, "%s, Line %d\n%s", irMapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  std::string strValue = pRoot->Value();
  if (strValue != remoteMapTag)
  {
    CLog::Log(LOGERROR, "%sl Doesn't contain <%s>", irMapPath.c_str(), remoteMapTag.c_str());
    return false;
  }

  // Run through our window groups
  TiXmlNode* pRemote = pRoot->FirstChild();
  while (pRemote != nullptr)
  {
    if (pRemote->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      const char *szRemote = pRemote->Value();
      if (szRemote != nullptr)
      {
        TiXmlAttribute* pAttr = pRemote->ToElement()->FirstAttribute();
        if (pAttr != nullptr)
          MapRemote(pRemote, pAttr->Value());
      }
    }
    pRemote = pRemote->NextSibling();
  }

  return true;
}

void CIRTranslator::MapRemote(TiXmlNode *pRemote, const char* szDevice)
{
  CLog::Log(LOGINFO, "* Adding remote mapping for device '%s'", szDevice);

  std::vector<std::string> remoteNames;

  auto it = m_irRemotesMap.find(szDevice);
  if (it == m_irRemotesMap.end())
    m_irRemotesMap[szDevice].reset(new IRButtonMap);

  std::shared_ptr<IRButtonMap>& buttons = m_irRemotesMap[szDevice];

  TiXmlElement *pButton = pRemote->FirstChildElement();
  while (pButton != nullptr)
  {
    if (!pButton->NoChildren())
    {
      if (pButton->ValueStr() == "altname")
        remoteNames.push_back(pButton->FirstChild()->ValueStr());
      else
        (*buttons)[pButton->FirstChild()->ValueStr()] = pButton->ValueStr();
    }
    pButton = pButton->NextSiblingElement();
  }

  for (const auto& remoteName : remoteNames)
  {
    CLog::Log(LOGINFO, "* Linking remote mapping for '%s' to '%s'", szDevice, remoteName.c_str());
    m_irRemotesMap[remoteName] = buttons;
  }
}

void CIRTranslator::Clear()
{
  m_irRemotesMap.clear();
}

unsigned int CIRTranslator::TranslateButton(const char* szDevice, const char *szButton)
{
  // Find the device
  auto it = m_irRemotesMap.find(szDevice);
  if (it == m_irRemotesMap.end())
    return 0;

  // Find the button
  auto it2 = (*it).second->find(szButton);
  if (it2 == (*it).second->end())
    return 0;

  // Convert the button to code
  if (strnicmp((*it2).second.c_str(), "obc", 3) == 0)
    return TranslateUniversalRemoteString((*it2).second.c_str());

  return TranslateString((*it2).second.c_str());
}

uint32_t CIRTranslator::TranslateString(const char *szButton)
{
  if (!szButton) 
    return 0;

  uint32_t buttonCode = 0;

  std::string strButton = szButton;
  StringUtils::ToLower(strButton);

  if (strButton == "left") buttonCode = XINPUT_IR_REMOTE_LEFT;
  else if (strButton == "right") buttonCode = XINPUT_IR_REMOTE_RIGHT;
  else if (strButton == "up") buttonCode = XINPUT_IR_REMOTE_UP;
  else if (strButton == "down") buttonCode = XINPUT_IR_REMOTE_DOWN;
  else if (strButton == "select") buttonCode = XINPUT_IR_REMOTE_SELECT;
  else if (strButton == "back") buttonCode = XINPUT_IR_REMOTE_BACK;
  else if (strButton == "menu") buttonCode = XINPUT_IR_REMOTE_MENU;
  else if (strButton == "info") buttonCode = XINPUT_IR_REMOTE_INFO;
  else if (strButton == "display") buttonCode = XINPUT_IR_REMOTE_DISPLAY;
  else if (strButton == "title") buttonCode = XINPUT_IR_REMOTE_TITLE;
  else if (strButton == "play") buttonCode = XINPUT_IR_REMOTE_PLAY;
  else if (strButton == "pause") buttonCode = XINPUT_IR_REMOTE_PAUSE;
  else if (strButton == "reverse") buttonCode = XINPUT_IR_REMOTE_REVERSE;
  else if (strButton == "forward") buttonCode = XINPUT_IR_REMOTE_FORWARD;
  else if (strButton == "skipplus") buttonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
  else if (strButton == "skipminus") buttonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
  else if (strButton == "stop") buttonCode = XINPUT_IR_REMOTE_STOP;
  else if (strButton == "zero") buttonCode = XINPUT_IR_REMOTE_0;
  else if (strButton == "one") buttonCode = XINPUT_IR_REMOTE_1;
  else if (strButton == "two") buttonCode = XINPUT_IR_REMOTE_2;
  else if (strButton == "three") buttonCode = XINPUT_IR_REMOTE_3;
  else if (strButton == "four") buttonCode = XINPUT_IR_REMOTE_4;
  else if (strButton == "five") buttonCode = XINPUT_IR_REMOTE_5;
  else if (strButton == "six") buttonCode = XINPUT_IR_REMOTE_6;
  else if (strButton == "seven") buttonCode = XINPUT_IR_REMOTE_7;
  else if (strButton == "eight") buttonCode = XINPUT_IR_REMOTE_8;
  else if (strButton == "nine") buttonCode = XINPUT_IR_REMOTE_9;
  // Additional keys from the media center extender for xbox remote
  else if (strButton == "power") buttonCode = XINPUT_IR_REMOTE_POWER;
  else if (strButton == "mytv") buttonCode = XINPUT_IR_REMOTE_MY_TV;
  else if (strButton == "mymusic") buttonCode = XINPUT_IR_REMOTE_MY_MUSIC;
  else if (strButton == "mypictures") buttonCode = XINPUT_IR_REMOTE_MY_PICTURES;
  else if (strButton == "myvideo") buttonCode = XINPUT_IR_REMOTE_MY_VIDEOS;
  else if (strButton == "record") buttonCode = XINPUT_IR_REMOTE_RECORD;
  else if (strButton == "start") buttonCode = XINPUT_IR_REMOTE_START;
  else if (strButton == "volumeplus") buttonCode = XINPUT_IR_REMOTE_VOLUME_PLUS;
  else if (strButton == "volumeminus") buttonCode = XINPUT_IR_REMOTE_VOLUME_MINUS;
  else if (strButton == "channelplus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton == "channelminus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton == "pageplus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton == "pageminus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton == "mute") buttonCode = XINPUT_IR_REMOTE_MUTE;
  else if (strButton == "recordedtv") buttonCode = XINPUT_IR_REMOTE_RECORDED_TV;
  else if (strButton == "guide") buttonCode = XINPUT_IR_REMOTE_GUIDE;
  else if (strButton == "livetv") buttonCode = XINPUT_IR_REMOTE_LIVE_TV;
  else if (strButton == "liveradio") buttonCode = XINPUT_IR_REMOTE_LIVE_RADIO;
  else if (strButton == "epgsearch") buttonCode = XINPUT_IR_REMOTE_EPG_SEARCH;
  else if (strButton == "star") buttonCode = XINPUT_IR_REMOTE_STAR;
  else if (strButton == "hash") buttonCode = XINPUT_IR_REMOTE_HASH;
  else if (strButton == "clear") buttonCode = XINPUT_IR_REMOTE_CLEAR;
  else if (strButton == "enter") buttonCode = XINPUT_IR_REMOTE_ENTER;
  else if (strButton == "xbox") buttonCode = XINPUT_IR_REMOTE_DISPLAY; // Same as display
  else if (strButton == "playlist") buttonCode = XINPUT_IR_REMOTE_PLAYLIST;
  else if (strButton == "teletext") buttonCode = XINPUT_IR_REMOTE_TELETEXT;
  else if (strButton == "red") buttonCode = XINPUT_IR_REMOTE_RED;
  else if (strButton == "green") buttonCode = XINPUT_IR_REMOTE_GREEN;
  else if (strButton == "yellow") buttonCode = XINPUT_IR_REMOTE_YELLOW;
  else if (strButton == "blue") buttonCode = XINPUT_IR_REMOTE_BLUE;
  else if (strButton == "subtitle") buttonCode = XINPUT_IR_REMOTE_SUBTITLE;
  else if (strButton == "language") buttonCode = XINPUT_IR_REMOTE_LANGUAGE;
  else if (strButton == "eject") buttonCode = XINPUT_IR_REMOTE_EJECT;
  else if (strButton == "contentsmenu") buttonCode = XINPUT_IR_REMOTE_CONTENTS_MENU;
  else if (strButton == "rootmenu") buttonCode = XINPUT_IR_REMOTE_ROOT_MENU;
  else if (strButton == "topmenu") buttonCode = XINPUT_IR_REMOTE_TOP_MENU;
  else if (strButton == "dvdmenu") buttonCode = XINPUT_IR_REMOTE_DVD_MENU;
  else if (strButton == "print") buttonCode = XINPUT_IR_REMOTE_PRINT;
  else CLog::Log(LOGERROR, "Remote Translator: Can't find button %s", strButton.c_str());
  return buttonCode;
}

uint32_t CIRTranslator::TranslateUniversalRemoteString(const char *szButton)
{
  if (szButton == nullptr || strlen(szButton) < 4 || strnicmp(szButton, "obc", 3)) 
    return 0;

  const char *szCode = szButton + 3;

  // Button Code is 255 - OBC (Original Button Code) of the button
  uint32_t buttonCode = 255 - atol(szCode);
  if (buttonCode > 255) 
    buttonCode = 0;

  return buttonCode;
}
