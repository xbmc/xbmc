/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "PlayerCoreFactory.h"
#include "threads/SingleLock.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "URL.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "PlayerCoreConfig.h"
#include "PlayerSelectionRule.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include <sstream>

#define PLAYERCOREFACTORY_XML "playercorefactory.xml"

CPlayerCoreFactory::CPlayerCoreFactory() = default;

CPlayerCoreFactory::~CPlayerCoreFactory()
{
  for(std::vector<CPlayerCoreConfig *>::iterator it = m_vecPlayerConfigs.begin(); it != m_vecPlayerConfigs.end(); ++it)
    delete *it;
  for(std::vector<CPlayerSelectionRule *>::iterator it = m_vecCoreSelectionRules.begin(); it != m_vecCoreSelectionRules.end(); ++it)
    delete *it;
}

CPlayerCoreFactory& CPlayerCoreFactory::GetInstance()
{
  static CPlayerCoreFactory sPlayerCoreFactory;
  return sPlayerCoreFactory;
}

void CPlayerCoreFactory::OnSettingsLoaded()
{
  LoadConfiguration("special://xbmc/system/" PLAYERCOREFACTORY_XML, true);
  LoadConfiguration(CProfilesManager::GetInstance().GetUserDataItem(PLAYERCOREFACTORY_XML), false);
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const std::string& nameId, IPlayerCallback& callback) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(nameId);
  
  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return nullptr;

  return m_vecPlayerConfigs[idx]->CreatePlayer(callback);
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players) const
{
  CSingleLock lock(m_section);
  players.clear();
  for (auto conf: m_vecPlayerConfigs)
  {
    if (conf->m_bPlaysAudio || conf->m_bPlaysVideo)
      players.push_back(conf->m_name);
  }
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players, const bool audio, const bool video) const
{
  CSingleLock lock(m_section);
  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: for video=%d, audio=%d", video, audio);

  for (auto conf: m_vecPlayerConfigs)
  {
    if (audio == conf->m_bPlaysAudio && video == conf->m_bPlaysVideo)
    {
      if (std::find(players.begin(), players.end(), conf->m_name) != players.end())
        continue;
      
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding player: %s", conf->m_name.c_str());
      players.push_back(conf->m_name);
    }
  }
}

void CPlayerCoreFactory::GetPlayers(const CFileItem& item, std::vector<std::string>&players) const
{
  CURL url(item.GetPath());

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers(%s)", CURL::GetRedacted(item.GetPath()).c_str());

  std::vector<std::string>validPlayers;
  GetPlayers(validPlayers);

  // Process rules
  for (auto rule: m_vecCoreSelectionRules)
    rule->GetPlayers(item, validPlayers, players);

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: matched {0} rules with players", players.size());

  // Process defaults

  // Set video default player. Check whether it's video first (overrule audio and
  // game check). Also push these players in case it is NOT audio or game either.
  if (item.IsVideo() || (!item.IsAudio() && !item.IsGame()))
  {
    int idx = GetPlayerIndex("videodefaultplayer");
    if (idx > -1)
    {
      std::string eVideoDefault = GetPlayerName(idx);
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding videodefaultplayer (%s)", eVideoDefault.c_str());
      players.push_back(eVideoDefault);
    }
    GetPlayers(players, false, true);  // Video-only players
    GetPlayers(players, true, true);   // Audio & video players
  }

  // Set audio default player
  // Pushback all audio players in case we don't know the type
  if (item.IsAudio())
  {
    int idx = GetPlayerIndex("audiodefaultplayer");
    if (idx > -1)
    {
      std::string eAudioDefault = GetPlayerName(idx);
      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding audiodefaultplayer (%s)", eAudioDefault.c_str());
        players.push_back(eAudioDefault);
    }
    GetPlayers(players, true, false); // Audio-only players
    GetPlayers(players, true, true);  // Audio & video players
  }

  if (item.IsGame())
  {
    CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding retroplayer");
    players.push_back("RetroPlayer");
  }

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: added {0} players", players.size());
}

int CPlayerCoreFactory::GetPlayerIndex(const std::string& strCoreName) const
{
  CSingleLock lock(m_section);
  if (!strCoreName.empty())
  {
    // Dereference "*default*player" aliases
    std::string strRealCoreName;
    if (StringUtils::EqualsNoCase(strCoreName, "audiodefaultplayer"))
      strRealCoreName = g_advancedSettings.m_audioDefaultPlayer;
    else if (StringUtils::EqualsNoCase(strCoreName, "videodefaultplayer"))
      strRealCoreName = g_advancedSettings.m_videoDefaultPlayer;
    else
      strRealCoreName = strCoreName;

    for(size_t i = 0; i < m_vecPlayerConfigs.size(); i++)
    {
      if (StringUtils::EqualsNoCase(m_vecPlayerConfigs[i]->GetName(), strRealCoreName))
        return i;
    }
    CLog::Log(LOGWARNING, "CPlayerCoreFactory::GetPlayer(%s): no such player: %s", strCoreName.c_str(), strRealCoreName.c_str());
  }
  return -1;
}

std::string CPlayerCoreFactory::GetPlayerName(size_t idx) const
{
  CSingleLock lock(m_section);
  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return "";

  return m_vecPlayerConfigs[idx]->m_name;
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players, std::string &type) const
{
  CSingleLock lock(m_section);
  for (auto config: m_vecPlayerConfigs)
  {
    if (config->m_type != type)
      continue;
    players.push_back(config->m_name);
  }
}

void CPlayerCoreFactory::GetRemotePlayers(std::vector<std::string>&players) const
{
  CSingleLock lock(m_section);
  for (auto config: m_vecPlayerConfigs)
  {
    if (config->m_type != "remote")
      continue;
    players.push_back(config->m_name);
  }
}

std::string CPlayerCoreFactory::GetPlayerType(const std::string& player) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return "";

  return m_vecPlayerConfigs[idx]->m_type;
}

bool CPlayerCoreFactory::PlaysAudio(const std::string& player) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return false;

  return m_vecPlayerConfigs[idx]->m_bPlaysAudio;
}

bool CPlayerCoreFactory::PlaysVideo(const std::string& player) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return false;

  return m_vecPlayerConfigs[idx]->m_bPlaysVideo;
}

std::string CPlayerCoreFactory::GetDefaultPlayer(const CFileItem& item) const
{
  std::vector<std::string>players;
  GetPlayers(item, players);

  //If we have any players return the first one
  if (!players.empty())
    return players.at(0);

  return "";
}

std::string CPlayerCoreFactory::SelectPlayerDialog(const std::vector<std::string>&players, float posX, float posY) const
{
  CContextButtons choices;
  if (players.size())
  {
    //Add default player
    std::string strCaption = players[0];
    strCaption += " (";
    strCaption += g_localizeStrings.Get(13278);
    strCaption += ")";
    choices.Add(0, strCaption);

    //Add all other players
    for (unsigned int i = 1; i < players.size(); i++)
      choices.Add(i, players[i]);

    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (choice >= 0)
      return players[choice];
  }
  return "";
}

std::string CPlayerCoreFactory::SelectPlayerDialog(float posX, float posY) const
{
  std::vector<std::string>players;
  GetPlayers(players);
  return SelectPlayerDialog(players, posX, posY);
}

bool CPlayerCoreFactory::LoadConfiguration(const std::string &file, bool clear)
{
  CSingleLock lock(m_section);

  CLog::Log(LOGNOTICE, "Loading player core factory settings from %s.", file.c_str());
  if (!XFILE::CFile::Exists(file))
  { // tell the user it doesn't exist
    CLog::Log(LOGNOTICE, "%s does not exist. Skipping.", file.c_str());
    return false;
  }

  CXBMCTinyXML playerCoreFactoryXML;
  if (!playerCoreFactoryXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", file.c_str(), playerCoreFactoryXML.ErrorRow(), playerCoreFactoryXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pConfig = playerCoreFactoryXML.RootElement();
  if (pConfig == NULL)
  {
    CLog::Log(LOGERROR, "Error loading %s, Bad structure", file.c_str());
    return false;
  }

  if (clear)
  {
    for (auto config: m_vecPlayerConfigs)
      delete config;
    m_vecPlayerConfigs.clear();

    for (auto rule: m_vecCoreSelectionRules)
      delete rule;
    m_vecCoreSelectionRules.clear();

    // Builtin players
    CPlayerCoreConfig* VideoPlayer = new CPlayerCoreConfig("VideoPlayer", "video", nullptr);
    VideoPlayer->m_bPlaysAudio = true;
    VideoPlayer->m_bPlaysVideo = true;
    m_vecPlayerConfigs.push_back(VideoPlayer);

    CPlayerCoreConfig* paplayer = new CPlayerCoreConfig("PAPlayer", "music", nullptr);
    paplayer->m_bPlaysAudio = true;
    m_vecPlayerConfigs.push_back(paplayer);

    CPlayerCoreConfig* retroPlayer = new CPlayerCoreConfig("RetroPlayer", "game", nullptr);
    m_vecPlayerConfigs.push_back(retroPlayer);
  }

  if (!pConfig || strcmpi(pConfig->Value(), "playercorefactory") != 0)
  {
    CLog::Log(LOGERROR, "Error loading configuration, no <playercorefactory> node");
    return false;
  }

  TiXmlElement *pPlayers = pConfig->FirstChildElement("players");
  if (pPlayers)
  {
    TiXmlElement* pPlayer = pPlayers->FirstChildElement("player");
    while (pPlayer)
    {
      std::string name = XMLUtils::GetAttribute(pPlayer, "name");
      std::string type = XMLUtils::GetAttribute(pPlayer, "type");
      if (type.empty()) type = name;
      StringUtils::ToLower(type);

      std::string internaltype;
      if (type == "videoplayer")
        internaltype = "video";
      else if (type == "paplayer")
        internaltype = "music";
      else if (type == "externalplayer")
        internaltype = "external";

      int count = 0;
      std::string playername = name;
      while (GetPlayerIndex(playername) >= 0)
      {
        count++;
        std::stringstream itoa;
        itoa << count;
        playername = name + itoa.str();
      }

      if (!internaltype.empty())
      {
        m_vecPlayerConfigs.push_back(new CPlayerCoreConfig(playername, internaltype, pPlayer));
      }

      pPlayer = pPlayer->NextSiblingElement("player");
    }
  }

  TiXmlElement *pRule = pConfig->FirstChildElement("rules");
  while (pRule)
  {
    const char* szAction = pRule->Attribute("action");
    if (szAction)
    {
      if (stricmp(szAction, "append") == 0)
      {
        m_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
      }
      else if (stricmp(szAction, "prepend") == 0)
      {
        m_vecCoreSelectionRules.insert(m_vecCoreSelectionRules.begin(), 1, new CPlayerSelectionRule(pRule));
      }
      else
      {
        m_vecCoreSelectionRules.clear();
        m_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
      }
    }
    else
    {
      m_vecCoreSelectionRules.push_back(new CPlayerSelectionRule(pRule));
    }

    pRule = pRule->NextSiblingElement("rules");
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGNOTICE, "Loaded playercorefactory configuration");

  return true;
}

void CPlayerCoreFactory::OnPlayerDiscovered(const std::string& id, const std::string& name)
{
  CSingleLock lock(m_section);
  std::vector<CPlayerCoreConfig *>::iterator it;
  for (it = m_vecPlayerConfigs.begin(); it != m_vecPlayerConfigs.end(); ++it)
  {
    if ((*it)->GetId() == id)
    {
      (*it)->m_name  = name;
      (*it)->m_type = "remote";
      return;
    }
  }

  int count = 0;
  std::string playername = name;
  while (GetPlayerIndex(playername) >= 0)
  {
    count++;
    std::stringstream itoa;
    itoa << count;
    playername = name + itoa.str();
  }

  CPlayerCoreConfig* player = new CPlayerCoreConfig(playername, "remote", nullptr, id);
  player->m_bPlaysAudio = true;
  player->m_bPlaysVideo = true;
  m_vecPlayerConfigs.push_back(player);
}

void CPlayerCoreFactory::OnPlayerRemoved(const std::string& id)
{
  CSingleLock lock(m_section);
  std::vector<CPlayerCoreConfig *>::iterator it;
  for(it = m_vecPlayerConfigs.begin(); it != m_vecPlayerConfigs.end(); ++it)
  {
    if ((*it)->GetId() == id)
      (*it)->m_type = "";
  }
}
