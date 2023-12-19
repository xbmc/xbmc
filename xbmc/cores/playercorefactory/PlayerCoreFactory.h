/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

// forward references

class TiXmlElement;
class CFileItem;
class CPlayerCoreConfig;
class CPlayerSelectionRule;
class CProfileManager;
class CSettings;
class IPlayer;
class IPlayerCallback;

class CPlayerCoreFactory : public ISettingsHandler
{
public:
  CPlayerCoreFactory(const CProfileManager &profileManager);
  CPlayerCoreFactory(const CPlayerCoreFactory&) = delete;
  CPlayerCoreFactory& operator=(CPlayerCoreFactory const&) = delete;
  ~CPlayerCoreFactory() override;

  void OnSettingsLoaded() override;

  std::shared_ptr<IPlayer> CreatePlayer(const std::string& nameId, IPlayerCallback& callback) const;
  void GetPlayers(const CFileItem& item, std::vector<std::string>&players) const;   //Players supporting the specified file
  void GetPlayers(std::vector<std::string>&players, bool audio, bool video) const;  //All audio players and/or video players
  void GetPlayers(std::vector<std::string>&players) const;                          //All players
  void GetPlayers(std::vector<std::string>&players, std::string &type) const;
  void GetRemotePlayers(std::vector<std::string>&players) const;                    //All remote players we can attach to
  std::string GetPlayerType(const std::string &player) const;
  bool IsExternalPlayer(const std::string& player) const;
  bool IsRemotePlayer(const std::string& player) const;
  bool PlaysAudio(const std::string &player) const;
  bool PlaysVideo(const std::string &player) const;

  std::string GetDefaultPlayer(const CFileItem& item) const;
  std::string SelectPlayerDialog(const std::vector<std::string>&players, float posX = 0, float posY = 0) const;
  std::string SelectPlayerDialog(float posX, float posY) const;
  void OnPlayerDiscovered(const std::string& id, const std::string& name);
  void OnPlayerRemoved(const std::string& id);

private:
  // Construction parameters
  std::shared_ptr<CSettings> m_settings;
  const CProfileManager &m_profileManager;

  int GetPlayerIndex(const std::string& strCoreName) const;
  std::string GetPlayerName(size_t idx) const;

  bool LoadConfiguration(const std::string &file, bool clear);

  std::vector<std::unique_ptr<CPlayerCoreConfig>> m_vecPlayerConfigs;
  std::list<std::unique_ptr<CPlayerSelectionRule>> m_vecCoreSelectionRules;
  mutable CCriticalSection m_section;
};
