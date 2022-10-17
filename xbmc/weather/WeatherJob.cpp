/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherJob.h"

#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/POUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#define LOCALIZED_TOKEN_FIRSTID    370
#define LOCALIZED_TOKEN_LASTID     395
#define LOCALIZED_TOKEN_FIRSTID2  1350
#define LOCALIZED_TOKEN_LASTID2   1449
#define LOCALIZED_TOKEN_FIRSTID3    11
#define LOCALIZED_TOKEN_LASTID3     17
#define LOCALIZED_TOKEN_FIRSTID4    71
#define LOCALIZED_TOKEN_LASTID4     97

using namespace ADDON;

using namespace std::chrono_literals;

CWeatherJob::CWeatherJob(int location)
{
  m_location = location;
}

bool CWeatherJob::DoWork()
{
  // wait for the network
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
              CSettings::SETTING_WEATHER_ADDON),
          addon, AddonType::SCRIPT_WEATHER, OnlyEnabled::CHOICE_YES))
    return false;

  // initialize our sys.argv variables
  std::vector<std::string> argv;
  argv.push_back(addon->LibPath());

  std::string strSetting = std::to_string(m_location);
  argv.push_back(strSetting);

  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");
  // call our script, passing the areacode
  int scriptId = -1;
  if ((scriptId = CScriptInvocationManager::GetInstance().ExecuteAsync(argv[0], addon, argv)) >= 0)
  {
    while (true)
    {
      if (!CScriptInvocationManager::GetInstance().IsRunning(scriptId))
        break;
      KODI::TIME::Sleep(100ms);
    }

    SetFromProperties();

    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WEATHER_FETCHED);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

  return true;
}

const CWeatherInfo &CWeatherJob::GetInfo() const
{
  return m_info;
}

void CWeatherJob::LocalizeOverviewToken(std::string &token)
{
  // This routine is case-insensitive.
  std::string strLocStr;
  if (!token.empty())
  {
    ilocalizedTokens i;
    i = m_localizedTokens.find(token);
    if (i != m_localizedTokens.end())
    {
      strLocStr = g_localizeStrings.Get(i->second);
    }
  }
  if (strLocStr == "")
    strLocStr = token; //if not found, let fallback
  token = strLocStr;
}

void CWeatherJob::LocalizeOverview(std::string &str)
{
  std::vector<std::string> words = StringUtils::Split(str, " ");
  for (std::vector<std::string>::iterator i = words.begin(); i != words.end(); ++i)
    LocalizeOverviewToken(*i);
  str = StringUtils::Join(words, " ");
}

void CWeatherJob::FormatTemperature(std::string &text, double temp)
{
  CTemperature temperature = CTemperature::CreateFromCelsius(temp);
  text = StringUtils::Format("{:.0f}", temperature.To(g_langInfo.GetTemperatureUnit()));
}

void CWeatherJob::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens
  std::string language = LANGUAGE_DEFAULT;
  std::shared_ptr<CSettingString> languageSetting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_LOCALE_LANGUAGE));
  if (languageSetting != NULL)
    language = languageSetting->GetDefault();

  // Load the strings.po file
  CPODocument PODoc;
  if (PODoc.LoadFile(URIUtils::AddFileToFolder(CLangInfo::GetLanguagePath(language), "strings.po")))
  {
    int counter = 0;

    while (PODoc.GetNextEntry())
    {
      if (PODoc.GetEntryType() != ID_FOUND)
        continue;

      uint32_t id = PODoc.GetEntryID();
      PODoc.ParseEntry(ISSOURCELANG);

      if (id > LOCALIZED_TOKEN_LASTID2) break;
      if ((LOCALIZED_TOKEN_FIRSTID  <= id && id <= LOCALIZED_TOKEN_LASTID)  ||
          (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2) ||
          (LOCALIZED_TOKEN_FIRSTID3 <= id && id <= LOCALIZED_TOKEN_LASTID3) ||
          (LOCALIZED_TOKEN_FIRSTID4 <= id && id <= LOCALIZED_TOKEN_LASTID4))
      {
        if (!PODoc.GetMsgid().empty())
        {
          m_localizedTokens.insert(make_pair(PODoc.GetMsgid(), id));
          counter++;
        }
      }
    }

    CLog::Log(LOGDEBUG, "POParser: loaded {} weather tokens", counter);
    return;
  }
}

std::string CWeatherJob::ConstructPath(std::string in) // copy intended
{
  if (in.find('/') != std::string::npos || in.find('\\') != std::string::npos)
    return in;
  if (in.empty() || in == "N/A")
    in = "na.png";

  return URIUtils::AddFileToFolder(ICON_ADDON_PATH, in);
}

void CWeatherJob::SetFromProperties()
{
  // Load in our tokens if necessary
  if (m_localizedTokens.empty())
    LoadLocalizedToken();

  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);
    m_info.currentConditions = window->GetProperty("Current.Condition").asString();
    m_info.currentIcon = ConstructPath(window->GetProperty("Current.OutlookIcon").asString());
    LocalizeOverview(m_info.currentConditions);
    FormatTemperature(m_info.currentTemperature,
                      strtod(window->GetProperty("Current.Temperature").asString().c_str(), nullptr));
    FormatTemperature(m_info.currentFeelsLike,
                      strtod(window->GetProperty("Current.FeelsLike").asString().c_str(), nullptr));
    m_info.currentUVIndex = window->GetProperty("Current.UVIndex").asString();
    LocalizeOverview(m_info.currentUVIndex);
    CSpeed speed = CSpeed::CreateFromKilometresPerHour(strtol(window->GetProperty("Current.Wind").asString().c_str(),0,10));
    std::string direction = window->GetProperty("Current.WindDirection").asString();
    if (direction == "CALM")
      m_info.currentWind = g_localizeStrings.Get(1410);
    else
    {
      LocalizeOverviewToken(direction);
      m_info.currentWind = StringUtils::Format(g_localizeStrings.Get(434), direction,
                                               (int)speed.To(g_langInfo.GetSpeedUnit()),
                                               g_langInfo.GetSpeedUnitString());
    }
    std::string windspeed = StringUtils::Format("{} {}", (int)speed.To(g_langInfo.GetSpeedUnit()),
                                                g_langInfo.GetSpeedUnitString());
    window->SetProperty("Current.WindSpeed",windspeed);
    FormatTemperature(m_info.currentDewPoint,
                      strtod(window->GetProperty("Current.DewPoint").asString().c_str(), nullptr));
    if (window->GetProperty("Current.Humidity").asString().empty())
      m_info.currentHumidity.clear();
    else
      m_info.currentHumidity =
          StringUtils::Format("{}%", window->GetProperty("Current.Humidity").asString());
    m_info.location = window->GetProperty("Current.Location").asString();
    for (int i=0;i<NUM_DAYS;++i)
    {
      std::string strDay = StringUtils::Format("Day{}.Title", i);
      m_info.forecast[i].m_day = window->GetProperty(strDay).asString();
      LocalizeOverviewToken(m_info.forecast[i].m_day);
      strDay = StringUtils::Format("Day{}.HighTemp", i);
      FormatTemperature(m_info.forecast[i].m_high,
                        strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day{}.LowTemp", i);
      FormatTemperature(m_info.forecast[i].m_low,
                        strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day{}.OutlookIcon", i);
      m_info.forecast[i].m_icon = ConstructPath(window->GetProperty(strDay).asString());
      strDay = StringUtils::Format("Day{}.Outlook", i);
      m_info.forecast[i].m_overview = window->GetProperty(strDay).asString();
      LocalizeOverview(m_info.forecast[i].m_overview);
    }
  }
}
