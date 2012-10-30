/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "Weather.h"
#include "filesystem/ZipManager.h"
#include "XMLUtils.h"
#include "utils/POUtils.h"
#include "Temperature.h"
#include "network/Network.h"
#include "Application.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "XBDateTime.h"
#include "LangInfo.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/Directory.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "log.h"
#include "addons/AddonManager.h"
#include "interfaces/python/XBPython.h"

using namespace std;
using namespace ADDON;
using namespace XFILE;

#define LOCALIZED_TOKEN_FIRSTID    370
#define LOCALIZED_TOKEN_LASTID     395
#define LOCALIZED_TOKEN_FIRSTID2  1396
#define LOCALIZED_TOKEN_LASTID2   1449
#define LOCALIZED_TOKEN_FIRSTID3    11
#define LOCALIZED_TOKEN_LASTID3     17
#define LOCALIZED_TOKEN_FIRSTID4    71
#define LOCALIZED_TOKEN_LASTID4     97

/*
FIXME'S
>strings are not centered
*/

#define WEATHER_BASE_PATH "special://temp/weather/"
#define WEATHER_ICON_PATH "special://temp/weather/"
#define WEATHER_SOURCE_FILE "special://xbmc/media/weather.zip"

bool CWeatherJob::m_imagesOkay = false;

CWeatherJob::CWeatherJob(int location)
{
  m_location = location;
}

bool CWeatherJob::DoWork()
{
  // wait for the network
  if (!g_application.getNetwork().IsAvailable(true))
    return false;

  AddonPtr addon;
  if (!ADDON::CAddonMgr::Get().GetAddon(g_guiSettings.GetString("weather.addon"), addon, ADDON_SCRIPT_WEATHER))
    return false;

  // initialize our sys.argv variables
  std::vector<CStdString> argv;
  argv.push_back(addon->LibPath());

  CStdString strSetting;
  strSetting.Format("%i", m_location);
  argv.push_back(strSetting);

  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");
  // call our script, passing the areacode
  if (g_pythonParser.evalFile(argv[0], argv,addon))
  {
    while (true)
    {
      if (!g_pythonParser.isRunning(g_pythonParser.getScriptId(addon->LibPath().c_str())))
        break;
      Sleep(100);
    }
    if (!m_imagesOkay)
    {
      CDirectory::Create(WEATHER_BASE_PATH);
      g_ZipManager.ExtractArchive(WEATHER_SOURCE_FILE, WEATHER_BASE_PATH);
      m_imagesOkay = true;
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

void CWeatherJob::LocalizeOverviewToken(CStdString &token)
{
  // This routine is case-insensitive. 
  CStdString strLocStr = "";
  if (!token.IsEmpty())
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

void CWeatherJob::LocalizeOverview(CStdString &str)
{
  CStdStringArray words;
  StringUtils::SplitString(str, " ", words);
  str.clear();
  for (unsigned int i = 0; i < words.size(); i++)
  {
    LocalizeOverviewToken(words[i]);
    str += words[i] + " ";
  }
  str.TrimRight(" ");
}

// input param must be kmh
int CWeatherJob::ConvertSpeed(int curSpeed)
{
  switch (g_langInfo.GetSpeedUnit())
  {
  case CLangInfo::SPEED_UNIT_KMH:
    break;
  case CLangInfo::SPEED_UNIT_MPS:
    curSpeed=(int)(curSpeed * (1000.0 / 3600.0) + 0.5);
    break;
  case CLangInfo::SPEED_UNIT_MPH:
    curSpeed=(int)(curSpeed / (8.0 / 5.0));
    break;
  case CLangInfo::SPEED_UNIT_MPMIN:
    curSpeed=(int)(curSpeed * (1000.0 / 3600.0) + 0.5*60);
    break;
  case CLangInfo::SPEED_UNIT_FTH:
    curSpeed=(int)(curSpeed * 3280.8398888889f);
    break;
  case CLangInfo::SPEED_UNIT_FTMIN:
    curSpeed=(int)(curSpeed * 54.6805555556f);
    break;
  case CLangInfo::SPEED_UNIT_FTS:
    curSpeed=(int)(curSpeed * 0.911344f);
    break;
  case CLangInfo::SPEED_UNIT_KTS:
    curSpeed=(int)(curSpeed * 0.5399568f);
    break;
  case CLangInfo::SPEED_UNIT_INCHPS:
    curSpeed=(int)(curSpeed * 10.9361388889f);
    break;
  case CLangInfo::SPEED_UNIT_YARDPS:
    curSpeed=(int)(curSpeed * 0.3037814722f);
    break;
  case CLangInfo::SPEED_UNIT_FPF:
    curSpeed=(int)(curSpeed * 1670.25f);
    break;
  case CLangInfo::SPEED_UNIT_BEAUFORT:
    {
      float knot=(float)curSpeed * 0.5399568f; // to kts first
      if(knot<=1.0) curSpeed=0;
      if(knot>1.0 && knot<3.5) curSpeed=1;
      if(knot>=3.5 && knot<6.5) curSpeed=2;
      if(knot>=6.5 && knot<10.5) curSpeed=3;
      if(knot>=10.5 && knot<16.5) curSpeed=4;
      if(knot>=16.5 && knot<21.5) curSpeed=5;
      if(knot>=21.5 && knot<27.5) curSpeed=6;
      if(knot>=27.5 && knot<33.5) curSpeed=7;
      if(knot>=33.5 && knot<40.5) curSpeed=8;
      if(knot>=40.5 && knot<47.5) curSpeed=9;
      if(knot>=47.5 && knot<55.5) curSpeed=10;
      if(knot>=55.5 && knot<63.5) curSpeed=11;
      if(knot>=63.5 && knot<74.5) curSpeed=12;
      if(knot>=74.5 && knot<80.5) curSpeed=13;
      if(knot>=80.5 && knot<89.5) curSpeed=14;
      if(knot>=89.5) curSpeed=15;
    }
    break;
  default:
    assert(false);
  }

  return curSpeed;
}

void CWeatherJob::FormatTemperature(CStdString &text, int temp)
{
  CTemperature temperature = CTemperature::CreateFromCelsius(temp);
  text.Format("%.0f", temperature.ToLocale());
}

void CWeatherJob::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens

  // Try the strings PO file first
  CPODocument PODoc;
  if (PODoc.LoadFile("special://xbmc/language/English/strings.po"))
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
  CStdString strLanguagePath = "special://xbmc/language/English/strings.xml";

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strLanguagePath) || !xmlDoc.RootElement())
  {
    CLog::Log(LOGERROR, "Weather: unable to load %s: %s at line %d", strLanguagePath.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return;
  }

  CStdString strEncoding;
  XMLUtils::GetEncoding(&xmlDoc, strEncoding);

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->Value() != CStdString("strings"))
    return;

  const TiXmlElement *pChild = pRootElement->FirstChildElement();
  while (pChild)
  {
    CStdString strValue = pChild->Value();
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
          CStdString utf8Label;
          if (strEncoding.IsEmpty()) // Is language file utf8?
            utf8Label=pChild->FirstChild()->Value();
          else
            g_charsetConverter.stringCharsetToUtf8(strEncoding, pChild->FirstChild()->Value(), utf8Label);

          if (!utf8Label.IsEmpty())
            m_localizedTokens.insert(make_pair(utf8Label, id));
        }
      }
    }
    pChild = pChild->NextSiblingElement();
  }
}

static CStdString ConstructPath(std::string in) // copy intended
{
  if (in.find("/") != std::string::npos || in.find("\\") != std::string::npos)
    return in;
  if (in.empty() || in == "N/A")
    in = "na.png";

  return URIUtils::AddFileToFolder(WEATHER_ICON_PATH,in);
}

void CWeatherJob::SetFromProperties()
{
  // Load in our tokens if necessary
  if (!m_localizedTokens.size())
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
        strtol(window->GetProperty("Current.Temperature").asString().c_str(),0,10));
    FormatTemperature(m_info.currentFeelsLike,
        strtol(window->GetProperty("Current.FeelsLike").asString().c_str(),0,10));
    m_info.currentUVIndex = window->GetProperty("Current.UVIndex").asString();
    LocalizeOverview(m_info.currentUVIndex);
    int speed = ConvertSpeed(strtol(window->GetProperty("Current.Wind").asString().c_str(),0,10));
    CStdString direction = window->GetProperty("Current.WindDirection").asString();
    if (direction == "CALM")
      m_info.currentWind = g_localizeStrings.Get(1410);
    else
    {
      LocalizeOverviewToken(direction);
      m_info.currentWind.Format(g_localizeStrings.Get(434).c_str(),
          direction, speed, g_langInfo.GetSpeedUnitString().c_str());
    }
    CStdString windspeed;
    windspeed.Format("%i %s",speed,g_langInfo.GetSpeedUnitString().c_str());
    window->SetProperty("Current.WindSpeed",windspeed);
    FormatTemperature(m_info.currentDewPoint,
        strtol(window->GetProperty("Current.DewPoint").asString().c_str(),0,10));
    if (window->GetProperty("Current.Humidity").asString().empty())
      m_info.currentHumidity.clear();
    else
      m_info.currentHumidity.Format("%s%%",window->GetProperty("Current.Humidity").asString().c_str());
    m_info.location = window->GetProperty("Current.Location").asString();
    for (int i=0;i<NUM_DAYS;++i)
    {
      CStdString strDay;
      strDay.Format("Day%i.Title",i);
      m_info.forecast[i].m_day = window->GetProperty(strDay).asString();
      LocalizeOverviewToken(m_info.forecast[i].m_day);
      strDay.Format("Day%i.HighTemp",i);
      FormatTemperature(m_info.forecast[i].m_high,
                    strtol(window->GetProperty(strDay).asString().c_str(),0,10));
      strDay.Format("Day%i.LowTemp",i);
      FormatTemperature(m_info.forecast[i].m_low,
                    strtol(window->GetProperty(strDay).asString().c_str(),0,10));
      strDay.Format("Day%i.OutlookIcon",i);
      m_info.forecast[i].m_icon = ConstructPath(window->GetProperty(strDay).asString());
      strDay.Format("Day%i.Outlook",i);
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

CStdString CWeather::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
    return URIUtils::AddFileToFolder(WEATHER_ICON_PATH,"na.png");

  return CInfoLoader::BusyInfo(info);
}

CStdString CWeather::TranslateInfo(int info) const
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
CStdString CWeather::GetLocation(int iLocation)
{
  CGUIWindow* window = g_windowManager.GetWindow(WINDOW_WEATHER);
  if (window)
  {
    CStdString setting;
    setting.Format("Location%i", iLocation);
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
  return !m_info.lastUpdateTime.IsEmpty();
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
  g_guiSettings.SetInt("weather.currentlocation", iLocation);
  g_settings.Save();
}

/*!
 \brief Retrieves the current location index from the settings
 \return the active location index (will be in the range [1..MAXLOCATION])
 */
int CWeather::GetArea() const
{
  return g_guiSettings.GetInt("weather.currentlocation");
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
