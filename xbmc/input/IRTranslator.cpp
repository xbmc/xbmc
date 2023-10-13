/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IRTranslator.h"

#include "ServiceBroker.h"
#include "input/remote/IRRemote.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <cstring>
#include <memory>
#include <stdlib.h>
#include <vector>

void CIRTranslator::Load(const std::string& irMapName)
{
  if (irMapName.empty())
    return;

  Clear();

  bool success = false;

  std::string irMapPath = URIUtils::AddFileToFolder("special://xbmc/system/", irMapName);
  if (CFileUtils::Exists(irMapPath))
    success |= LoadIRMap(irMapPath);
  else
    CLog::Log(LOGDEBUG, "CIRTranslator::Load - no system {} found, skipping", irMapName);

  irMapPath =
      CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem(irMapName);
  if (CFileUtils::Exists(irMapPath))
    success |= LoadIRMap(irMapPath);
  else
    CLog::Log(LOGDEBUG, "CIRTranslator::Load - no userdata {} found, skipping", irMapName);

  if (!success)
    CLog::Log(LOGERROR, "CIRTranslator::Load - unable to load remote map {}", irMapName);
}

bool CIRTranslator::LoadIRMap(const std::string& irMapPath)
{
  std::string remoteMapTag = URIUtils::GetFileName(irMapPath);
  size_t lastindex = remoteMapTag.find_last_of('.');
  if (lastindex != std::string::npos)
    remoteMapTag.resize(lastindex);
  StringUtils::ToLower(remoteMapTag);

  // Load our xml file, and fill up our mapping tables
  CXBMCTinyXML2 xmlDoc;

  // Load the config file
  CLog::Log(LOGINFO, "Loading {}", irMapPath);
  if (!xmlDoc.LoadFile(irMapPath))
  {
    CLog::Log(LOGERROR, "{}, Line {}\n{}", irMapPath, xmlDoc.ErrorLineNum(), xmlDoc.ErrorStr());
    return false;
  }

  auto* pRoot = xmlDoc.RootElement();
  if (pRoot == nullptr)
    return false;

  std::string strValue = pRoot->Value();
  if (strValue != remoteMapTag)
  {
    CLog::Log(LOGERROR, "{} Doesn't contain <{}>", irMapPath, remoteMapTag);
    return false;
  }

  // Run through our window groups
  auto* pRemote = pRoot->FirstChild();
  while (pRemote != nullptr)
  {
    if (pRemote->ToElement())
    {
      const char* szRemote = pRemote->Value();
      if (szRemote != nullptr)
      {
        auto* pAttr = pRemote->ToElement()->FirstAttribute();
        if (pAttr != nullptr)
          MapRemote(pRemote, pAttr->Value());
      }
    }
    pRemote = pRemote->NextSibling();
  }

  return true;
}

void CIRTranslator::MapRemote(tinyxml2::XMLNode* pRemote, const std::string& szDevice)
{
  CLog::Log(LOGINFO, "* Adding remote mapping for device '{}'", szDevice);

  std::vector<std::string> remoteNames;

  auto it = m_irRemotesMap.find(szDevice);
  if (it == m_irRemotesMap.end())
    m_irRemotesMap[szDevice] = std::make_shared<IRButtonMap>();

  const std::shared_ptr<IRButtonMap>& buttons = m_irRemotesMap[szDevice];

  auto* pButton = pRemote->FirstChildElement();
  while (pButton != nullptr)
  {
    if (!pButton->NoChildren())
    {
      if (std::strcmp(pButton->Value(), "altname") == 0)
        remoteNames.push_back(pButton->FirstChild()->Value());
      else
        (*buttons)[pButton->FirstChild()->Value()] = pButton->Value();
    }
    pButton = pButton->NextSiblingElement();
  }

  for (const auto& remoteName : remoteNames)
  {
    CLog::Log(LOGINFO, "* Linking remote mapping for '{}' to '{}'", szDevice, remoteName);
    m_irRemotesMap[remoteName] = buttons;
  }
}

void CIRTranslator::Clear()
{
  m_irRemotesMap.clear();
}

unsigned int CIRTranslator::TranslateButton(const std::string& szDevice,
                                            const std::string& szButton)
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
  if (StringUtils::CompareNoCase((*it2).second, "obc", 3) == 0)
    return TranslateUniversalRemoteString((*it2).second);

  return TranslateString((*it2).second);
}

uint32_t CIRTranslator::TranslateString(std::string strButton)
{
  if (strButton.empty())
    return 0;

  uint32_t buttonCode = 0;

  StringUtils::ToLower(strButton);

  if (strButton == "left")
    buttonCode = XINPUT_IR_REMOTE_LEFT;
  else if (strButton == "right")
    buttonCode = XINPUT_IR_REMOTE_RIGHT;
  else if (strButton == "up")
    buttonCode = XINPUT_IR_REMOTE_UP;
  else if (strButton == "down")
    buttonCode = XINPUT_IR_REMOTE_DOWN;
  else if (strButton == "select")
    buttonCode = XINPUT_IR_REMOTE_SELECT;
  else if (strButton == "back")
    buttonCode = XINPUT_IR_REMOTE_BACK;
  else if (strButton == "menu")
    buttonCode = XINPUT_IR_REMOTE_MENU;
  else if (strButton == "info")
    buttonCode = XINPUT_IR_REMOTE_INFO;
  else if (strButton == "display")
    buttonCode = XINPUT_IR_REMOTE_DISPLAY;
  else if (strButton == "title")
    buttonCode = XINPUT_IR_REMOTE_TITLE;
  else if (strButton == "play")
    buttonCode = XINPUT_IR_REMOTE_PLAY;
  else if (strButton == "pause")
    buttonCode = XINPUT_IR_REMOTE_PAUSE;
  else if (strButton == "reverse")
    buttonCode = XINPUT_IR_REMOTE_REVERSE;
  else if (strButton == "forward")
    buttonCode = XINPUT_IR_REMOTE_FORWARD;
  else if (strButton == "skipplus")
    buttonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
  else if (strButton == "skipminus")
    buttonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
  else if (strButton == "stop")
    buttonCode = XINPUT_IR_REMOTE_STOP;
  else if (strButton == "zero")
    buttonCode = XINPUT_IR_REMOTE_0;
  else if (strButton == "one")
    buttonCode = XINPUT_IR_REMOTE_1;
  else if (strButton == "two")
    buttonCode = XINPUT_IR_REMOTE_2;
  else if (strButton == "three")
    buttonCode = XINPUT_IR_REMOTE_3;
  else if (strButton == "four")
    buttonCode = XINPUT_IR_REMOTE_4;
  else if (strButton == "five")
    buttonCode = XINPUT_IR_REMOTE_5;
  else if (strButton == "six")
    buttonCode = XINPUT_IR_REMOTE_6;
  else if (strButton == "seven")
    buttonCode = XINPUT_IR_REMOTE_7;
  else if (strButton == "eight")
    buttonCode = XINPUT_IR_REMOTE_8;
  else if (strButton == "nine")
    buttonCode = XINPUT_IR_REMOTE_9;
  // Additional keys from the media center extender for xbox remote
  else if (strButton == "power")
    buttonCode = XINPUT_IR_REMOTE_POWER;
  else if (strButton == "mytv")
    buttonCode = XINPUT_IR_REMOTE_MY_TV;
  else if (strButton == "mymusic")
    buttonCode = XINPUT_IR_REMOTE_MY_MUSIC;
  else if (strButton == "mypictures")
    buttonCode = XINPUT_IR_REMOTE_MY_PICTURES;
  else if (strButton == "myvideo")
    buttonCode = XINPUT_IR_REMOTE_MY_VIDEOS;
  else if (strButton == "record")
    buttonCode = XINPUT_IR_REMOTE_RECORD;
  else if (strButton == "start")
    buttonCode = XINPUT_IR_REMOTE_START;
  else if (strButton == "volumeplus")
    buttonCode = XINPUT_IR_REMOTE_VOLUME_PLUS;
  else if (strButton == "volumeminus")
    buttonCode = XINPUT_IR_REMOTE_VOLUME_MINUS;
  else if (strButton == "channelplus")
    buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton == "channelminus")
    buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton == "pageplus")
    buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton == "pageminus")
    buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton == "mute")
    buttonCode = XINPUT_IR_REMOTE_MUTE;
  else if (strButton == "recordedtv")
    buttonCode = XINPUT_IR_REMOTE_RECORDED_TV;
  else if (strButton == "guide")
    buttonCode = XINPUT_IR_REMOTE_GUIDE;
  else if (strButton == "livetv")
    buttonCode = XINPUT_IR_REMOTE_LIVE_TV;
  else if (strButton == "liveradio")
    buttonCode = XINPUT_IR_REMOTE_LIVE_RADIO;
  else if (strButton == "epgsearch")
    buttonCode = XINPUT_IR_REMOTE_EPG_SEARCH;
  else if (strButton == "star")
    buttonCode = XINPUT_IR_REMOTE_STAR;
  else if (strButton == "hash")
    buttonCode = XINPUT_IR_REMOTE_HASH;
  else if (strButton == "clear")
    buttonCode = XINPUT_IR_REMOTE_CLEAR;
  else if (strButton == "enter")
    buttonCode = XINPUT_IR_REMOTE_ENTER;
  else if (strButton == "xbox")
    buttonCode = XINPUT_IR_REMOTE_DISPLAY; // Same as display
  else if (strButton == "playlist")
    buttonCode = XINPUT_IR_REMOTE_PLAYLIST;
  else if (strButton == "teletext")
    buttonCode = XINPUT_IR_REMOTE_TELETEXT;
  else if (strButton == "red")
    buttonCode = XINPUT_IR_REMOTE_RED;
  else if (strButton == "green")
    buttonCode = XINPUT_IR_REMOTE_GREEN;
  else if (strButton == "yellow")
    buttonCode = XINPUT_IR_REMOTE_YELLOW;
  else if (strButton == "blue")
    buttonCode = XINPUT_IR_REMOTE_BLUE;
  else if (strButton == "subtitle")
    buttonCode = XINPUT_IR_REMOTE_SUBTITLE;
  else if (strButton == "language")
    buttonCode = XINPUT_IR_REMOTE_LANGUAGE;
  else if (strButton == "eject")
    buttonCode = XINPUT_IR_REMOTE_EJECT;
  else if (strButton == "contentsmenu")
    buttonCode = XINPUT_IR_REMOTE_CONTENTS_MENU;
  else if (strButton == "rootmenu")
    buttonCode = XINPUT_IR_REMOTE_ROOT_MENU;
  else if (strButton == "topmenu")
    buttonCode = XINPUT_IR_REMOTE_TOP_MENU;
  else if (strButton == "dvdmenu")
    buttonCode = XINPUT_IR_REMOTE_DVD_MENU;
  else if (strButton == "print")
    buttonCode = XINPUT_IR_REMOTE_PRINT;
  else
    CLog::Log(LOGERROR, "Remote Translator: Can't find button {}", strButton);
  return buttonCode;
}

uint32_t CIRTranslator::TranslateUniversalRemoteString(const std::string& szButton)
{
  if (szButton.empty() || szButton.length() < 4 || StringUtils::CompareNoCase(szButton, "obc", 3))
    return 0;

  const char* szCode = szButton.c_str() + 3;

  // Button Code is 255 - OBC (Original Button Code) of the button
  uint32_t buttonCode = 255 - atol(szCode);
  if (buttonCode > 255)
    buttonCode = 0;

  return buttonCode;
}
