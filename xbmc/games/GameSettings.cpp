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
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
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

constexpr auto LOGIN_TO_RETRO_ACHIEVEMENTS_URL_TEMPLATE =
    "http://retroachievements.org/dorequest.php?r=login&u={}&p={}";
constexpr auto GET_PATCH_DATA_URL_TEMPLATE =
    "http://retroachievements.org/dorequest.php?r=patch&u={}&t={}&g=0";
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
      if (settingId == SETTING_GAMES_ACHIEVEMENTS_PASSWORD)
        m_settings->SetString(SETTING_GAMES_ACHIEVEMENTS_PASSWORD, "");
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

  XFILE::CFile request;
  const CURL loginUrl(
      StringUtils::Format(LOGIN_TO_RETRO_ACHIEVEMENTS_URL_TEMPLATE, username, password));

  std::vector<uint8_t> response;
  if (request.LoadFile(loginUrl, response) > 0)
  {
    std::string strResponse(response.begin(), response.end());
    CVariant data(CVariant::VariantTypeObject);
    if (CJSONVariantParser::Parse(strResponse, data))
    {
      if (data[SUCCESS].asBoolean())
      {
        token = data[TOKEN].asString();
        if (!IsAccountVerified(username, token))
        {
          token.clear();
          // "RetroAchievements", "Your account is not verified, please check your emails to complete your sign up"
          CServiceBroker::GetEventLog()->AddWithNotification(
              EventPtr(new CNotificationEvent(35264, 35270, EventLevel::Error)));
        }
      }
      else
      {
        token.clear();

        // "RetroAchievements", "Incorrect User/Password!"
        CServiceBroker::GetEventLog()->AddWithNotification(
            EventPtr(new CNotificationEvent(35264, 35265, EventLevel::Error)));
      }
    }
    else
    {
      // "RetroAchievements", "Invalid response from server"
      CServiceBroker::GetEventLog()->AddWithNotification(
          EventPtr(new CNotificationEvent(35264, 35267, EventLevel::Error)));

      CLog::Log(LOGERROR, "Invalid server response: {}", strResponse);
    }
  }
  else
  {
    // "RetroAchievements", "Failed to contact server"
    CServiceBroker::GetEventLog()->AddWithNotification(
        EventPtr(new CNotificationEvent(35264, 35266, EventLevel::Error)));
  }
  return token;
}

bool CGameSettings::IsAccountVerified(const std::string& username, const std::string& token) const
{
  XFILE::CFile request;
  const CURL getPatchFileUrl(StringUtils::Format(GET_PATCH_DATA_URL_TEMPLATE, username, token));
  std::vector<uint8_t> response;
  if (request.LoadFile(getPatchFileUrl, response) > 0)
  {
    std::string strResponse(response.begin(), response.end());
    CVariant data(CVariant::VariantTypeObject);

    if (CJSONVariantParser::Parse(strResponse, data))
    {
      return data[SUCCESS].asBoolean();
    }
  }

  return false;
}
