/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <string>

#include "kodi/c-api/addon-instance/game.h"

struct AddonInstance_Game;

namespace KODI
{
namespace RETRO
{
enum class RConsoleID;
}

namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientCheevos
{
public:
  CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct);

  bool RCGenerateHashFromFile(std::string& hash,
                              RETRO::RConsoleID consoleID,
                              const std::string& filePath);
  bool RCGetGameIDUrl(std::string& url, const std::string& hash);
  bool RCGetPatchFileUrl(std::string& url,
                         const std::string& username,
                         const std::string& token,
                         unsigned int gameID);
  void SetRetroAchievementsCredentials(const std::string& username, const std::string& token);
  bool RCPostRichPresenceUrl(std::string& url,
                             std::string& postData,
                             const std::string& username,
                             const std::string& token,
                             unsigned gameID,
                             const std::string& richPresence);
  void RCEnableRichPresence(const std::string& script);
  void RCGetRichPresenceEvaluation(std::string& evaluation, RETRO::RConsoleID consoleID);

  // RetroAchievements event receivers (addon -> Kodi)
  void OnGameLoaded(const game_rc_game_loaded& data);
  void OnAchievementTriggered(const game_rc_achievement_triggered& data);
  void OnGameCompleted(const std::string& title);
  void OnRichPresenceUpdated(const std::string& evaluation);
  void OnLoginResult(const game_rc_login_result& data);

  void ActivateAchievement(unsigned int cheevoId, const std::string& memAddrExpression);
  void GetAchievementUrlId(const std::function<void(const std::string& achievementUrl,
                                                    unsigned int cheevoId)>& callback);

  // When the game is reset, the runtime should also be reset
  void RCResetRuntime();

  bool GetMemory(unsigned int type, uint8_t*& data, size_t& size);

  // RetroAchievements event receivers (addon -> Kodi)

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
