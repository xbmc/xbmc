/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>

#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"
#include <string>

// forward references

class TiXmlElement;
class CFileItem;
class CPlayerCoreConfig;
class CPlayerSelectionRule;
class CProfilesManager;
class CSettings;
class IPlayer;
class IPlayerCallback;

class CPlayerCoreFactory : public ISettingsHandler
{
public:
  CPlayerCoreFactory(CSettings &settings,
                     const CProfilesManager &profileManager);
  CPlayerCoreFactory(const CPlayerCoreFactory&) = delete;
  CPlayerCoreFactory& operator=(CPlayerCoreFactory const&) = delete;
  ~CPlayerCoreFactory() override;

  void OnSettingsLoaded() override;

  IPlayer* CreatePlayer(const std::string& nameId, IPlayerCallback& callback) const;
  void GetPlayers(const CFileItem& item, std::vector<std::string>&players) const;   //Players supporting the specified file
  void GetPlayers(std::vector<std::string>&players, bool audio, bool video) const;  //All audio players and/or video players
  void GetPlayers(std::vector<std::string>&players) const;                          //All players
  void GetPlayers(std::vector<std::string>&players, std::string &type) const;
  void GetRemotePlayers(std::vector<std::string>&players) const;                    //All remote players we can attach to
  std::string GetPlayerType(const std::string &player) const;
  bool PlaysAudio(const std::string &player) const;
  bool PlaysVideo(const std::string &player) const;

  std::string GetDefaultPlayer(const CFileItem& item) const;
  std::string SelectPlayerDialog(const std::vector<std::string>&players, float posX = 0, float posY = 0) const;
  std::string SelectPlayerDialog(float posX, float posY) const;
  void OnPlayerDiscovered(const std::string& id, const std::string& name);
  void OnPlayerRemoved(const std::string& id);

private:
  // Construction parameters
  CSettings &m_settings;
  const CProfilesManager &m_profileManager;

  int GetPlayerIndex(const std::string& strCoreName) const;
  std::string GetPlayerName(size_t idx) const;

  bool LoadConfiguration(const std::string &file, bool clear);

  std::vector<CPlayerCoreConfig *> m_vecPlayerConfigs;
  std::vector<CPlayerSelectionRule *> m_vecCoreSelectionRules;
  CCriticalSection m_section;
};
