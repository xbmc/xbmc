/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientCheevos.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "cores/RetroPlayer/cheevos/RConsoleIDs.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "games/addons/GameClient.h"

namespace
{
// C ABI expects a raw callback + context pointer, so we bridge to std::function here
void __cdecl GetCheevoUrlIdCallback(const void* context,
                                    const char* achievementUrl,
                                    unsigned int cheevoId)
{
  if (context == nullptr)
    return;

  const auto* callback = static_cast<
      const std::function<void(const std::string& achievementUrl, unsigned int cheevoId)>*>(
      context);
  if (!(*callback))
    return;

  try
  {
    (*callback)(achievementUrl != nullptr ? std::string{achievementUrl} : std::string{}, cheevoId);
  }
  catch (...)
  {
    // Never allow exceptions to unwind through the C ABI callback boundary
  }
}
} // namespace

using namespace KODI;
using namespace GAME;

namespace
{
constexpr unsigned int TOAST_DISPLAY_MS = 6000;
constexpr unsigned int TOAST_MESSAGE_MS = 500;
constexpr const char* SETTING_RA_LOGGED_IN = "gamesachievements.loggedin";
constexpr const char* RA_BADGE_CACHE = "special://profile/addon_data/game.retroachievements/badges/";
constexpr const char* RA_ICON_CACHE = "special://profile/addon_data/game.retroachievements/icons/";
} // namespace

CGameClientCheevos::CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct)
  : m_gameClient(gameClient),
    m_struct(addonStruct)
{
}

bool CGameClientCheevos::RCGenerateHashFromFile(std::string& hash,
                                                RETRO::RConsoleID consoleID,
                                                const std::string& filePath)
{
  char* _hash = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->RCGenerateHashFromFile(
            &m_struct, &_hash, static_cast<unsigned int>(consoleID), filePath.c_str()),
        "RCGenerateHashFromFile()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGenerateHashFromFile()");
  }

  if (_hash)
  {
    hash = _hash;
    m_struct.toAddon->FreeString(&m_struct, _hash);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCGetGameIDUrl(std::string& url, const std::string& hash)
{
  char* _url = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetGameIDUrl(&m_struct, &_url, hash.c_str()),
                          "RCGetGameIDUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetGameIDUrl()");
  }

  if (_url)
  {
    url = _url;
    m_struct.toAddon->FreeString(&m_struct, _url);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCGetPatchFileUrl(std::string& url,
                                           const std::string& username,
                                           const std::string& token,
                                           unsigned int gameID)
{
  char* _url = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetPatchFileUrl(
                              &m_struct, &_url, username.c_str(), token.c_str(), gameID),
                          "RCGetPatchFileUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetPatchFileUrl()");
  }

  if (_url)
  {
    url = _url;
    m_struct.toAddon->FreeString(&m_struct, _url);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCPostRichPresenceUrl(std::string& url,
                                               std::string& postData,
                                               const std::string& username,
                                               const std::string& token,
                                               unsigned gameID,
                                               const std::string& richPresence)
{
  char* _url = nullptr;
  char* _postData = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCPostRichPresenceUrl(
                              &m_struct, &_url, &_postData, username.c_str(), token.c_str(), gameID,
                              richPresence.c_str()),
                          "RCPostRichPresenceUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCPostRichPresenceUrl()");
  }

  if (_url)
  {
    url = _url;
    m_struct.toAddon->FreeString(&m_struct, _url);
  }
  if (_postData)
  {
    postData = _postData;
    m_struct.toAddon->FreeString(&m_struct, _postData);
  }

  return error == GAME_ERROR_NO_ERROR;
}

void CGameClientCheevos::SetRetroAchievementsCredentials(const std::string& username,
                                                         const std::string& token)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;
  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->SetRetroAchievementsCredentials(
                              &m_struct, username.c_str(), token.c_str()),
                          "SetRetroAchievementsCredentials()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetRetroAchievementsCredentials()");
  }
}

void CGameClientCheevos::RCEnableRichPresence(const std::string& script)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCEnableRichPresence(&m_struct, script.c_str()),
                          "RCEnableRichPresence()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCEnableRichPresence()");
  }
}

void CGameClientCheevos::RCGetRichPresenceEvaluation(std::string& evaluation,
                                                     RETRO::RConsoleID consoleID)
{
  char* _evaluation = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetRichPresenceEvaluation(
                              &m_struct, &_evaluation, static_cast<unsigned int>(consoleID)),
                          "RCGetRichPresenceEvaluation()");

    if (_evaluation)
    {
      evaluation = _evaluation;
      m_struct.toAddon->FreeString(&m_struct, _evaluation);
    }
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetRichPresenceEvaluation()");
  }
}

void CGameClientCheevos::ActivateAchievement(unsigned int cheevoId,
                                             const std::string& memAddrExpression)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->ActivateAchievement(&m_struct, cheevoId,
                                                                        memAddrExpression.c_str()),
                          "ActivateAchievement()");
  }
  catch (...)
  {
    m_gameClient.LogException("ActivateAchievement()");
  }
}

void CGameClientCheevos::GetAchievementUrlId(
    const std::function<void(const std::string& achievementUrl, unsigned int cheevoId)>& callback)
{
  if (!callback)
    return;

  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->GetCheevoUrlId(&m_struct, GetCheevoUrlIdCallback, &callback),
        "GetCheevoUrlId()");
  }
  catch (...)
  {
    m_gameClient.LogException("GetCheevoUrlId()");
  }
}

void CGameClientCheevos::RCResetRuntime()
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCResetRuntime(&m_struct), "RCResetRuntime()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCResetRuntime()");
  }
}


// RetroAchievements event receivers

void CGameClientCheevos::OnGameLoaded(const game_rc_game_loaded& data)
{
  const std::string gameTitle = (data.title != nullptr) ? data.title : "";
  const std::string countMsg =
      StringUtils::Format("{}/{} achievements", data.num_unlocked, data.num_achievements);

  CLog::Log(LOGINFO, "CGameClientCheevos: loaded '{}' ({}, {} leaderboards)",
            gameTitle, countMsg, data.num_leaderboards);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                        gameTitle, countMsg, TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
}

void CGameClientCheevos::OnAchievementTriggered(const game_rc_achievement_triggered& data)
{
  const std::string title = (data.title != nullptr) ? data.title : "Achievement Unlocked!";
  CLog::Log(LOGINFO, "CGameClientCheevos: achievement triggered id={} '{}'", data.id, title);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                        "Achievement Unlocked!", title, TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
}

void CGameClientCheevos::OnGameCompleted(const std::string& title)
{
  CLog::Log(LOGINFO, "CGameClientCheevos: mastered '{}'", title);
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                        "Mastered!", title, TOAST_DISPLAY_MS, false, TOAST_MESSAGE_MS);
}

void CGameClientCheevos::OnRichPresenceUpdated(const std::string& evaluation)
{
  CLog::Log(LOGDEBUG, "CGameClientCheevos: rich presence: {}", evaluation);
  auto& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.SetAchievementRichPresence(evaluation);
}

void CGameClientCheevos::OnLoginResult(const game_rc_login_result& data)
{
  if (data.success)
  {
    const std::string username = (data.username != nullptr) ? data.username : "";
    CLog::Log(LOGINFO, "CGameClientCheevos: logged in as '{}' ({} points)",
              username, data.points);

    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetBool(SETTING_RA_LOGGED_IN, true);
    settings->Save();
  }
  else
  {
    const std::string error = (data.error_message != nullptr) ? data.error_message : "unknown";
    CLog::Log(LOGWARNING, "CGameClientCheevos: login failed: {}", error);
  }
}
