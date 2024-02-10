/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerCoreFactory.h"

#include "FileItem.h"
#include "PlayerCoreConfig.h"
#include "PlayerSelectionRule.h"
#include "URL.h"
#include "cores/IPlayerCallback.h"
#include "cores/VideoPlayer/Interface/InputStreamConstants.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"

#include <mutex>
#include <sstream>

#define PLAYERCOREFACTORY_XML "playercorefactory.xml"

using namespace KODI;

CPlayerCoreFactory::CPlayerCoreFactory(const CProfileManager& profileManager)
  : m_settings(CServiceBroker::GetSettingsComponent()->GetSettings()),
    m_profileManager(profileManager)
{
  if (m_settings->IsLoaded())
    OnSettingsLoaded();

  m_settings->GetSettingsManager()->RegisterSettingsHandler(this);
}

CPlayerCoreFactory::~CPlayerCoreFactory()
{
  m_settings->GetSettingsManager()->UnregisterSettingsHandler(this);
}

void CPlayerCoreFactory::OnSettingsLoaded()
{
  LoadConfiguration("special://xbmc/system/" PLAYERCOREFACTORY_XML, true);
  LoadConfiguration(m_profileManager.GetUserDataItem(PLAYERCOREFACTORY_XML), false);
}

std::shared_ptr<IPlayer> CPlayerCoreFactory::CreatePlayer(const std::string& nameId,
                                                          IPlayerCallback& callback) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  size_t idx = GetPlayerIndex(nameId);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return nullptr;

  return m_vecPlayerConfigs[idx]->CreatePlayer(callback);
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  players.clear();
  for (auto& conf : m_vecPlayerConfigs)
  {
    if (conf->m_bPlaysAudio || conf->m_bPlaysVideo)
      players.emplace_back(conf->m_name);
  }
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players, const bool audio, const bool video) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: for video={}, audio={}", video, audio);

  for (auto& conf : m_vecPlayerConfigs)
  {
    if (audio == conf->m_bPlaysAudio && video == conf->m_bPlaysVideo)
    {
      if (std::find(players.begin(), players.end(), conf->m_name) != players.end())
        continue;

      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding player: {}", conf->m_name);
      players.emplace_back(conf->m_name);
    }
  }
}

void CPlayerCoreFactory::GetPlayers(const CFileItem& item, std::vector<std::string>&players) const
{
  CURL url(item.GetDynPath());

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers({})", CURL::GetRedacted(item.GetDynPath()));

  enum class ForcedPlayer
  {
    NONE,
    VIDEO_DEFAULT,
    AUDIO_DEFAULT
  };

  ForcedPlayer defaultInputstreamPlayerOverride = ForcedPlayer::NONE;

  // If we are using an inpustream add-on
  if (!item.GetProperty(STREAM_PROPERTY_INPUTSTREAM).empty())
  {
    if (!item.GetProperty(STREAM_PROPERTY_INPUTSTREAM_PLAYER).empty())
    {
      const std::string inputstreamPlayerOverride =
          item.GetProperty(STREAM_PROPERTY_INPUTSTREAM_PLAYER).asString();

      if (inputstreamPlayerOverride == "videodefaultplayer")
        defaultInputstreamPlayerOverride = ForcedPlayer::VIDEO_DEFAULT;
      else if ((inputstreamPlayerOverride == "audiodefaultplayer"))
        defaultInputstreamPlayerOverride = ForcedPlayer::AUDIO_DEFAULT;
    }
  }

  std::vector<std::string>validPlayers;
  GetPlayers(validPlayers);

  // Process rules
  for (auto& rule : m_vecCoreSelectionRules)
    rule->GetPlayers(item, validPlayers, players);

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: matched {0} rules with players", players.size());

  // Process defaults

  // Set video default player. Check whether it's video first (overrule audio and
  // game check). Also push these players in case it is NOT audio or game either.
  //
  // If an inputstream add-on is used, first check if we have an override to use
  // "videodefaultplayer"
  if (defaultInputstreamPlayerOverride == ForcedPlayer::VIDEO_DEFAULT ||
      (defaultInputstreamPlayerOverride == ForcedPlayer::NONE &&
       (VIDEO::IsVideo(item) || (!MUSIC::IsAudio(item) && !item.IsGame()))))
  {
    int idx = GetPlayerIndex("videodefaultplayer");
    if (idx > -1)
    {
      const std::string videoDefault = GetPlayerName(idx);
      if (std::find(players.cbegin(), players.cend(), videoDefault) == players.cend())
      {
        players.emplace_back(videoDefault);
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding videodefaultplayer ({})",
                  videoDefault);
      }
    }
    GetPlayers(players, false, true);  // Video-only players
    GetPlayers(players, true, true);   // Audio & video players
  }

  // Set audio default player
  // Pushback all audio players in case we don't know the type
  if (defaultInputstreamPlayerOverride == ForcedPlayer::AUDIO_DEFAULT ||
      (defaultInputstreamPlayerOverride == ForcedPlayer::NONE && MUSIC::IsAudio(item)))
  {
    int idx = GetPlayerIndex("audiodefaultplayer");
    if (idx > -1)
    {
      const std::string audioDefault = GetPlayerName(idx);
      if (std::find(players.cbegin(), players.cend(), audioDefault) == players.cend())
      {
        players.emplace_back(audioDefault);
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding audiodefaultplayer ({})",
                  audioDefault);
      }
    }
    GetPlayers(players, true, false); // Audio-only players
    GetPlayers(players, true, true);  // Audio & video players
  }

  if (item.IsGame())
  {
    CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding retroplayer");
    players.emplace_back("RetroPlayer");
  }

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: added {0} players", players.size());
}

int CPlayerCoreFactory::GetPlayerIndex(const std::string& strCoreName) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (!strCoreName.empty())
  {
    // Dereference "*default*player" aliases
    std::string strRealCoreName;
    if (StringUtils::EqualsNoCase(strCoreName, "audiodefaultplayer"))
      strRealCoreName = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_audioDefaultPlayer;
    else if (StringUtils::EqualsNoCase(strCoreName, "videodefaultplayer"))
      strRealCoreName = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoDefaultPlayer;
    else
      strRealCoreName = strCoreName;

    for(size_t i = 0; i < m_vecPlayerConfigs.size(); i++)
    {
      if (StringUtils::EqualsNoCase(m_vecPlayerConfigs[i]->GetName(), strRealCoreName))
        return i;
    }
    CLog::Log(LOGWARNING, "CPlayerCoreFactory::GetPlayer({}): no such player: {}", strCoreName,
              strRealCoreName);
  }
  return -1;
}

std::string CPlayerCoreFactory::GetPlayerName(size_t idx) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return "";

  return m_vecPlayerConfigs[idx]->m_name;
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players, std::string &type) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (auto& config : m_vecPlayerConfigs)
  {
    if (config->m_type != type)
      continue;
    players.emplace_back(config->m_name);
  }
}

void CPlayerCoreFactory::GetRemotePlayers(std::vector<std::string>&players) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (auto& config : m_vecPlayerConfigs)
  {
    if (config->m_type != "remote")
      continue;
    players.emplace_back(config->m_name);
  }
}

std::string CPlayerCoreFactory::GetPlayerType(const std::string& player) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return "";

  return m_vecPlayerConfigs[idx]->m_type;
}

bool CPlayerCoreFactory::IsExternalPlayer(const std::string& player) const
{
  return (GetPlayerType(player) == "external");
}

bool CPlayerCoreFactory::IsRemotePlayer(const std::string& player) const
{
  return (GetPlayerType(player) == "remote");
}

bool CPlayerCoreFactory::PlaysAudio(const std::string& player) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return false;

  return m_vecPlayerConfigs[idx]->m_bPlaysAudio;
}

bool CPlayerCoreFactory::PlaysVideo(const std::string& player) const
{
  std::unique_lock<CCriticalSection> lock(m_section);
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
  std::unique_lock<CCriticalSection> lock(m_section);

  CLog::Log(LOGINFO, "Loading player core factory settings from {}.", file);
  if (!CFileUtils::Exists(file))
  { // tell the user it doesn't exist
    CLog::Log(LOGINFO, "{} does not exist. Skipping.", file);
    return false;
  }

  CXBMCTinyXML playerCoreFactoryXML;
  if (!playerCoreFactoryXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading {}, Line {} ({})", file, playerCoreFactoryXML.ErrorRow(),
              playerCoreFactoryXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pConfig = playerCoreFactoryXML.RootElement();
  if (pConfig == NULL)
  {
    CLog::Log(LOGERROR, "Error loading {}, Bad structure", file);
    return false;
  }

  if (clear)
  {
    m_vecPlayerConfigs.clear();
    m_vecCoreSelectionRules.clear();

    // Builtin players
    auto VideoPlayer = std::make_unique<CPlayerCoreConfig>("VideoPlayer", "video", nullptr);
    VideoPlayer->m_bPlaysAudio = true;
    VideoPlayer->m_bPlaysVideo = true;
    m_vecPlayerConfigs.emplace_back(std::move(VideoPlayer));

    auto paplayer = std::make_unique<CPlayerCoreConfig>("PAPlayer", "music", nullptr);
    paplayer->m_bPlaysAudio = true;
    m_vecPlayerConfigs.emplace_back(std::move(paplayer));

    auto retroPlayer = std::make_unique<CPlayerCoreConfig>("RetroPlayer", "game", nullptr);
    m_vecPlayerConfigs.emplace_back(std::move(retroPlayer));
  }

  if (StringUtils::CompareNoCase(pConfig->Value(), "playercorefactory") != 0)
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
        m_vecPlayerConfigs.emplace_back(
            std::make_unique<CPlayerCoreConfig>(playername, internaltype, pPlayer));
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
      if (StringUtils::CompareNoCase(szAction, "append") == 0)
      {
        m_vecCoreSelectionRules.emplace_back(std::make_unique<CPlayerSelectionRule>(pRule));
      }
      else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
      {
        m_vecCoreSelectionRules.emplace_front(std::make_unique<CPlayerSelectionRule>(pRule));
      }
      else
      {
        m_vecCoreSelectionRules.clear();
        m_vecCoreSelectionRules.emplace_back(std::make_unique<CPlayerSelectionRule>(pRule));
      }
    }
    else
    {
      m_vecCoreSelectionRules.emplace_back(std::make_unique<CPlayerSelectionRule>(pRule));
    }

    pRule = pRule->NextSiblingElement("rules");
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGINFO, "Loaded playercorefactory configuration");

  return true;
}

void CPlayerCoreFactory::OnPlayerDiscovered(const std::string& id, const std::string& name)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (auto& playerConfig : m_vecPlayerConfigs)
  {
    if (playerConfig->GetId() == id)
    {
      playerConfig->m_name = name;
      playerConfig->m_type = "remote";
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

  auto player = std::make_unique<CPlayerCoreConfig>(playername, "remote", nullptr, id);
  player->m_bPlaysAudio = true;
  player->m_bPlaysVideo = true;
  m_vecPlayerConfigs.emplace_back(std::move(player));
}

void CPlayerCoreFactory::OnPlayerRemoved(const std::string& id)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (auto& playerConfig : m_vecPlayerConfigs)
  {
    if (playerConfig->GetId() == id)
      playerConfig->m_type = "";
  }
}
