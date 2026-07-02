/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameSettings.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
const std::string SETTING_GAMES_ENABLE = "gamesgeneral.enable";
const std::string SETTING_GAMES_SHOW_OSD_HELP = "gamesgeneral.showosdhelp";
const std::string SETTING_GAMES_ENABLEAUTOSAVE = "gamesgeneral.enableautosave";
const std::string SETTING_GAMES_ENABLEREWIND = "gamesgeneral.enablerewind";
const std::string SETTING_GAMES_REWINDTIME = "gamesgeneral.rewindtime";
const std::string SETTING_GAMES_ACHIEVEMENTS_USERNAME = "gamesachievements.username";
const std::string SETTING_GAMES_ACHIEVEMENTS_PASSWORD = "gamesachievements.password";
const std::string SETTING_GAMES_ACHIEVEMENTS_TOKEN = "gamesachievements.token";
const std::string SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN = "gamesachievements.loggedin";

// FIX 1: Use HTTPS (not HTTP)
// FIX 2: Use r=login2 (r=login is deprecated and may return unexpected results)
// Localised string IDs for RetroAchievements notifications
// 35264 = "RetroAchievements" (heading)
// 35266 = "Failed to contact server"
// 35267 = "Invalid response from server"
// 35282 = "achievements unlocked"
// 35283 = "Logged in as {0:s}"
// 35284 = "Session expired. Please log in again in Settings."
// 35285 = "Could not reach retroachievements.org. Check your network."
// 35286 = "Invalid response from server."
// 35287 = "RetroAchievements Login Failed"
constexpr auto LOGIN_TO_RETRO_ACHIEVEMENTS_URL_TEMPLATE =
    "https://retroachievements.org/dorequest.php?r=login2&u={}&p={}";

// FIX 3: IsAccountVerified used g=0 which always 404s.
//        Replaced with r=getusersummary which is a lightweight authenticated
//        endpoint that confirms the token is valid without needing a game ID.
constexpr auto VERIFY_ACCOUNT_URL_TEMPLATE =
    "https://retroachievements.org/dorequest.php?r=getusersummary&u={}&t={}&a=1";

constexpr auto SUCCESS = "Success";
constexpr auto TOKEN = "Token";
} // namespace

CGameSettings::CGameSettings()
{
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_settings->RegisterCallback(this, {SETTING_GAMES_ENABLEREWIND, SETTING_GAMES_REWINDTIME,
                                      SETTING_GAMES_ACHIEVEMENTS_USERNAME,
                                      SETTING_GAMES_ACHIEVEMENTS_PASSWORD,
                                      SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN});

  // On startup reset logged-in flag if token is missing
  const std::string token = m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN);
  if (token.empty() && m_settings->GetBool(SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN))
  {
    CLog::Log(LOGWARNING, "CGameSettings: saved token is empty, resetting logged-in state");
    m_settings->SetBool(SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN, false);
    m_settings->Save();
  }
}

CGameSettings::~CGameSettings()
{
  m_settings->UnregisterCallback(this);
}

bool CGameSettings::GamesEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::ShowOSDHelp()
{
  return m_settings->GetBool(SETTING_GAMES_SHOW_OSD_HELP);
}

void CGameSettings::SetShowOSDHelp(bool bShow)
{
  if (m_settings->GetBool(SETTING_GAMES_SHOW_OSD_HELP) != bShow)
  {
    m_settings->SetBool(SETTING_GAMES_SHOW_OSD_HELP, bShow);

    //! @todo Asynchronous save
    m_settings->Save();
  }
}

void CGameSettings::ToggleGames()
{
  m_settings->ToggleBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::AutosaveEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLEAUTOSAVE);
}

bool CGameSettings::RewindEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLEREWIND);
}

unsigned int CGameSettings::MaxRewindTimeSec()
{
  int rewindTimeSec = m_settings->GetInt(SETTING_GAMES_REWINDTIME);

  return static_cast<unsigned int>(std::max(rewindTimeSec, 0));
}

std::string CGameSettings::GetRAUsername() const
{
  return m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_USERNAME);
}

std::string CGameSettings::GetRAToken() const
{
  return m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN);
}

void CGameSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_GAMES_ENABLEREWIND || settingId == SETTING_GAMES_REWINDTIME)
  {
    SetChanged();
    NotifyObservers(ObservableMessageSettingsChanged);
  }
  else if (settingId == SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN &&
           std::dynamic_pointer_cast<const CSettingBool>(setting)->GetValue())
  {
    const std::string username = m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_USERNAME);
    const std::string password = m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_PASSWORD);
    std::string token = m_settings->GetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN);

    token = LoginToRA(username, password, std::move(token));

    m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN, token);

    if (!token.empty())
    {
      m_settings->SetBool(SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN, true);
    }
    else
    {
      // Login failed — clear the logged-in flag so the UI reflects the failure
      m_settings->SetBool(SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN, false);
    }

    m_settings->Save();
  }
  else if (settingId == SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN &&
           !std::dynamic_pointer_cast<const CSettingBool>(setting)->GetValue())
  {
    m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN, "");
    m_settings->Save();
  }
  else if (settingId == SETTING_GAMES_ACHIEVEMENTS_USERNAME ||
           settingId == SETTING_GAMES_ACHIEVEMENTS_PASSWORD)
  {
    m_settings->SetBool(SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN, false);
    m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_TOKEN, "");
    m_settings->Save();
  }
}

std::string CGameSettings::LoginToRA(const std::string& username,
                                     const std::string& password,
                                     std::string token) const
{
  if (username.empty() || password.empty())
    return token;

  // Use CFile::LoadFile with User-Agent set via CURL protocol option.
  // LoadFile reads the response body even on HTTP 4xx so we get the
  // server's actual error message for wrong passwords (e.g. HTTP 401).
  CURL loginUrl(StringUtils::Format(LOGIN_TO_RETRO_ACHIEVEMENTS_URL_TEMPLATE,
                                   CURL::Encode(username),
                                   CURL::Encode(password)));
  loginUrl.SetProtocolOption("user-agent", CSysInfo::GetUserAgent());
  loginUrl.SetProtocolOption("failonerror", "false");

  CLog::Log(LOGDEBUG, "CGameSettings::LoginToRA -- logging in as '{}'", username);

  XFILE::CFile request;
  std::vector<uint8_t> responseBytes;
  if (request.LoadFile(loginUrl, responseBytes) > 0)
  {
    std::string strResponse(responseBytes.begin(), responseBytes.end());
    CVariant data(CVariant::VariantTypeObject);
    if (CJSONVariantParser::Parse(strResponse, data))
    {
      if (data[SUCCESS].asBoolean())
      {
        token = data[TOKEN].asString();

        CLog::Log(LOGINFO, "CGameSettings::LoginToRA -- logged in successfully as '{}'", username);

        CServiceBroker::GetEventLog()->AddWithNotification(EventPtr(new CNotificationEvent(
            35264, StringUtils::Format("Logged in as {}", username), EventLevel::Information)));
      }
      else
      {
        token.clear();

        const std::string errorMsg = data["Error"].asString();
        CLog::Log(LOGWARNING, "CGameSettings::LoginToRA -- server rejected: {}", errorMsg);

        CServiceBroker::GetEventLog()->AddWithNotification(EventPtr(new CNotificationEvent(
            35264, errorMsg.empty() ? std::string("Incorrect username or password.") : errorMsg,
            EventLevel::Error)));
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CGameSettings::LoginToRA -- invalid response: {}", strResponse);

      CServiceBroker::GetEventLog()->AddWithNotification(
          EventPtr(new CNotificationEvent(35264, 35267, EventLevel::Error)));
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CGameSettings::LoginToRA -- failed to contact server");

    CServiceBroker::GetEventLog()->AddWithNotification(
        EventPtr(new CNotificationEvent(35264, 35266, EventLevel::Error)));
  }
  return token;
}

bool CGameSettings::IsAccountVerified(const std::string& username, const std::string& token) const
{
  // FIX 3: Original code used r=patch&g=0 which always 404s (no game with ID 0).
  //        This caused IsAccountVerified to always return false, showing the
  //        "not verified" error even for valid accounts.
  //
  //        Replaced with r=getusersummary which is a lightweight endpoint
  //        that confirms the token is valid. We request a=1 (1 recent achievement)
  //        to minimise the response payload.
  XFILE::CFile request;
  const CURL verifyUrl(StringUtils::Format(VERIFY_ACCOUNT_URL_TEMPLATE,
                                          CURL::Encode(username),
                                          CURL::Encode(token)));

  CLog::Log(LOGDEBUG, "CGameSettings::IsAccountVerified -- verifying token for '{}'", username);

  std::vector<uint8_t> response;
  if (request.LoadFile(verifyUrl, response) > 0)
  {
    std::string strResponse(response.begin(), response.end());
    CVariant data(CVariant::VariantTypeObject);

    if (CJSONVariantParser::Parse(strResponse, data))
    {
      const bool verified = data[SUCCESS].asBoolean();
      CLog::Log(LOGDEBUG, "CGameSettings::IsAccountVerified -- result: {}", verified);
      return verified;
    }
  }

  CLog::Log(LOGERROR, "CGameSettings::IsAccountVerified -- verification request failed");
  return false;
}

bool CGameSettings::GetAchievementsLoggedIn() const
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      SETTING_GAMES_ACHIEVEMENTS_LOGGED_IN);
}
