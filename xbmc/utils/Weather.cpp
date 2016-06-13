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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "Weather.h"

#include <utility>

#include "addons/AddonManager.h"
#include "addons/GUIDialogAddonSettings.h"
#include "Application.h"
#include "filesystem/Directory.h"
#include "filesystem/ZipManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "GUIUserMessages.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "LangInfo.h"
#include "log.h"
#include "network/Network.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "utils/POUtils.h"
#include "utils/Temperature.h"
#include "XBDateTime.h"
#include "XMLUtils.h"
#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

using namespace ADDON;
using namespace XFILE;

#define LOCALIZED_TOKEN_FIRSTID    370
#define LOCALIZED_TOKEN_LASTID     395
#define LOCALIZED_TOKEN_FIRSTID2  1350
#define LOCALIZED_TOKEN_LASTID2   1449
#define LOCALIZED_TOKEN_FIRSTID3    11
#define LOCALIZED_TOKEN_LASTID3     17
#define LOCALIZED_TOKEN_FIRSTID4    71
#define LOCALIZED_TOKEN_LASTID4     97

static const std::string IconAddonPath = "resource://resource.images.weathericons.default";

/*
FIXME'S
>strings are not centered
*/

CWeatherJob::CWeatherJob(int location)
{
  m_location = location;
}

bool CWeatherJob::DoWork()
{
  // wait for the network
  if (!g_application.getNetwork().IsAvailable())
    return false;

  AddonPtr addon;
  if (!ADDON::CAddonMgr::GetInstance().GetAddon(CSettings::GetInstance().GetString(CSettings::SETTING_WEATHER_ADDON), addon, ADDON_SCRIPT_WEATHER))
    return false;

  // initialize our sys.argv variables
  std::vector<std::string> argv;
  argv.push_back(addon->LibPath());

  std::string strSetting = StringUtils::Format("%i", m_location);
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
      Sleep(100);
    }

    SetFromProperties();

    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WEATHER_FETCHED);
    g_windowManager.SendThreadMessage(msg);
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
  text = StringUtils::Format("%.0f", temperature.To(g_langInfo.GetTemperatureUnit()));
}

void CWeatherJob::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens
  std::string language = LANGUAGE_DEFAULT;
  CSettingString* languageSetting = static_cast<CSettingString*>(CSettings::GetInstance().GetSetting(CSettings::SETTING_LOCALE_LANGUAGE));
  if (languageSetting != NULL)
    language = languageSetting->GetDefault();

  // Try the strings PO file first
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

    CLog::Log(LOGDEBUG, "POParser: loaded %i weather tokens", counter);
    return;
  }

  CLog::Log(LOGDEBUG,
            "Weather: no PO string file available, to load English tokens, "
            "fallback to strings.xml file");

  // We load the tokens from the strings.xml file
  std::string strLanguagePath = URIUtils::AddFileToFolder(CLangInfo::GetLanguagePath(language), "strings.xml");

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strLanguagePath) || !xmlDoc.RootElement())
  {
    CLog::Log(LOGERROR, "Weather: unable to load %s: %s at line %d", strLanguagePath.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->ValueStr() != "strings")
    return;

  const TiXmlElement *pChild = pRootElement->FirstChildElement();
  while (pChild)
  {
    std::string strValue = pChild->ValueStr();
    if (strValue == "string")
    { // Load new style language file with id as attribute
      const char* attrId = pChild->Attribute("id");
      if (attrId && !pChild->NoChildren())
      {
        int id = atoi(attrId);
        if ((LOCALIZED_TOKEN_FIRSTID  <= id && id <= LOCALIZED_TOKEN_LASTID)  ||
            (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2) ||
            (LOCALIZED_TOKEN_FIRSTID3 <= id && id <= LOCALIZED_TOKEN_LASTID3) ||
            (LOCALIZED_TOKEN_FIRSTID4 <= id && id <= LOCALIZED_TOKEN_LASTID4))
        {
          std::string utf8Label(pChild->FirstChild()->ValueStr());
          if (!utf8Label.empty())
            m_localizedTokens.insert(make_pair(utf8Label, id));
        }
      }
    }
    pChild = pChild->NextSiblingElement();
  }
}

static std::string ConstructPath(std::string in) // copy intended
{
  if (in.find("/") != std::string::npos || in.find("\\") != std::string::npos)
    return in;
  if (in.empty() || in == "N/A")
    in = "na.png";

  return URIUtils::AddFileToFolder(IconAddonPath, in);
}

void CWeatherJob::SetFromProperties()
{
  // Load in our tokens if necessary
  if (m_localizedTokens.empty())
    LoadLocalizedToken();

  CGUIWindow* window = g_windowManager.GetWindow(WINDOW_WEATHER);
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
      m_info.currentWind = StringUtils::Format(g_localizeStrings.Get(434).c_str(),
          direction.c_str(), (int)speed.To(g_langInfo.GetSpeedUnit()), g_langInfo.GetSpeedUnitString().c_str());
    }
    std::string windspeed = StringUtils::Format("%i %s", (int)speed.To(g_langInfo.GetSpeedUnit()), g_langInfo.GetSpeedUnitString().c_str());
    window->SetProperty("Current.WindSpeed",windspeed);
    FormatTemperature(m_info.currentDewPoint,
        strtod(window->GetProperty("Current.DewPoint").asString().c_str(), nullptr));
    if (window->GetProperty("Current.Humidity").asString().empty())
      m_info.currentHumidity.clear();
    else
      m_info.currentHumidity = StringUtils::Format("%s%%", window->GetProperty("Current.Humidity").asString().c_str());
    m_info.location = window->GetProperty("Current.Location").asString();
    for (int i=0;i<NUM_DAYS;++i)
    {
      std::string strDay = StringUtils::Format("Day%i.Title",i);
      m_info.forecast[i].m_day = window->GetProperty(strDay).asString();
      LocalizeOverviewToken(m_info.forecast[i].m_day);
      strDay = StringUtils::Format("Day%i.HighTemp",i);
      FormatTemperature(m_info.forecast[i].m_high,
                    strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day%i.LowTemp",i);
      FormatTemperature(m_info.forecast[i].m_low,
                    strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day%i.OutlookIcon",i);
      m_info.forecast[i].m_icon = ConstructPath(window->GetProperty(strDay).asString());
      strDay = StringUtils::Format("Day%i.Outlook",i);
      m_info.forecast[i].m_overview = window->GetProperty(strDay).asString();
      LocalizeOverview(m_info.forecast[i].m_overview);
    }
  }
}

CWeather::CWeather(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  Reset();
}

CWeather::~CWeather(void)
{
}

std::string CWeather::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
    return URIUtils::AddFileToFolder(IconAddonPath, "na.png");

  return CInfoLoader::BusyInfo(info);
}

std::string CWeather::TranslateInfo(int info) const
{
  if (info == WEATHER_LABEL_CURRENT_COND) return m_info.currentConditions;
  else if (info == WEATHER_IMAGE_CURRENT_ICON) return m_info.currentIcon;
  else if (info == WEATHER_LABEL_CURRENT_TEMP) return m_info.currentTemperature;
  else if (info == WEATHER_LABEL_CURRENT_FEEL) return m_info.currentFeelsLike;
  else if (info == WEATHER_LABEL_CURRENT_UVID) return m_info.currentUVIndex;
  else if (info == WEATHER_LABEL_CURRENT_WIND) return m_info.currentWind;
  else if (info == WEATHER_LABEL_CURRENT_DEWP) return m_info.currentDewPoint;
  else if (info == WEATHER_LABEL_CURRENT_HUMI) return m_info.currentHumidity;
  else if (info == WEATHER_LABEL_LOCATION) return m_info.location;
  return "";
}

/*!
 \brief Retrieve the city name for the specified location from the settings
 \param iLocation the location index (can be in the range [1..MAXLOCATION])
 \return the city name (without the accompanying region area code)
 */
std::string CWeather::GetLocation(int iLocation)
{
  CGUIWindow* window = g_windowManager.GetWindow(WINDOW_WEATHER);
  if (window)
  {
    std::string setting = StringUtils::Format("Location%i", iLocation);
    return window->GetProperty(setting).asString();
  }
  return "";
}

void CWeather::Reset()
{
  m_info.Reset();
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !m_info.lastUpdateTime.empty();
}

const day_forecast &CWeather::GetForecast(int day) const
{
  return m_info.forecast[day];
}

/*!
 \brief Saves the specified location index to the settings. Call Refresh()
        afterwards to update weather info for the new location.
 \param iLocation the new location index (can be in the range [1..MAXLOCATION])
 */
void CWeather::SetArea(int iLocation)
{
  CSettings::GetInstance().SetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION, iLocation);
  CSettings::GetInstance().Save();
}

/*!
 \brief Retrieves the current location index from the settings
 \return the active location index (will be in the range [1..MAXLOCATION])
 */
int CWeather::GetArea() const
{
  return CSettings::GetInstance().GetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION);
}

CJob *CWeather::GetJob() const
{
  return new CWeatherJob(GetArea());
}

void CWeather::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = ((CWeatherJob *)job)->GetInfo();
  CInfoLoader::OnJobComplete(jobID, success, job);
}

void CWeather::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDON)
  {
    // clear "WeatherProviderLogo" property that some weather addons set
    CGUIWindow* window = g_windowManager.GetWindow(WINDOW_WEATHER);
    window->SetProperty("WeatherProviderLogo", "");
    Refresh();
  }
}

void CWeather::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDONSETTINGS)
  {
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(CSettings::GetInstance().GetString(CSettings::SETTING_WEATHER_ADDON), addon, ADDON_SCRIPT_WEATHER) && addon != NULL)
    { //! @todo maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
      Refresh();
    }
  }
}

