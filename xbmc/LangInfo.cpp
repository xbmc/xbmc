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

#include "LangInfo.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/LanguageResource.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Weather.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <algorithm>

using namespace std;
using namespace PVR;

static std::string shortDateFormats[] = {
  // short date formats using "/"
  "DD/MM/YYYY",
  "MM/DD/YYYY",
  "YYYY/MM/DD",
  "D/M/YYYY",
  // short date formats using "-"
  "DD-MM-YYYY",
  "MM-DD-YYYY",
  "YYYY-MM-DD",
  "YYYY-M-D",
  // short date formats using "."
  "DD.MM.YYYY",
  "DD.M.YYYY",
  "D.M.YYYY",
  "D. M. YYYY",
  "YYYY.MM.DD"
};

#define SHORT_DATE_FORMATS_SIZE   sizeof(shortDateFormats) / sizeof(std::string)

static std::string longDateFormats[] = {
  "DDDD, D MMMM YYYY",
  "DDDD, DD MMMM YYYY",
  "DDDD, D. MMMM YYYY",
  "DDDD, DD. MMMM YYYY",
  "DDDD, MMMM D, YYYY",
  "DDDD, MMMM DD, YYYY",
  "DDDD D MMMM YYYY",
  "DDDD DD MMMM YYYY",
  "DDDD D. MMMM YYYY",
  "DDDD DD. MMMM YYYY",
  "D. MMMM YYYY",
  "DD. MMMM YYYY",
  "D. MMMM. YYYY",
  "DD. MMMM. YYYY",
  "YYYY. MMMM. D"
};

#define LONG_DATE_FORMATS_SIZE    sizeof(longDateFormats) / sizeof(std::string)

#define TIME_FORMAT_MM_SS         ":mm:ss"
#define TIME_FORMAT_SINGLE_12     "h" TIME_FORMAT_MM_SS
#define TIME_FORMAT_DOUBLE_12     "hh" TIME_FORMAT_MM_SS
#define TIME_FORMAT_SINGLE_24     "H" TIME_FORMAT_MM_SS
#define TIME_FORMAT_DOUBLE_24     "HH" TIME_FORMAT_MM_SS

#define TIME_FORMAT_12HOURS       "12hours"
#define TIME_FORMAT_24HOURS       "24hours"

typedef struct TemperatureInfo {
  CTemperature::Unit unit;
  std::string name;
} TemperatureInfo;

static TemperatureInfo temperatureInfo[] = {
  { CTemperature::UnitFahrenheit, "f" },
  { CTemperature::UnitKelvin,     "k" },
  { CTemperature::UnitCelsius,    "c" },
  { CTemperature::UnitReaumur,    "re" },
  { CTemperature::UnitRankine,    "ra" },
  { CTemperature::UnitRomer,      "ro" },
  { CTemperature::UnitDelisle,    "de" },
  { CTemperature::UnitNewton,     "n" }
};

#define TEMPERATURE_INFO_SIZE     sizeof(temperatureInfo) / sizeof(TemperatureInfo)
#define TEMP_UNIT_STRINGS         20027

typedef struct SpeedInfo {
  CSpeed::Unit unit;
  std::string name;
} SpeedInfo;

static SpeedInfo speedInfo[] = {
  { CSpeed::UnitKilometresPerHour,    "kmh" },
  { CSpeed::UnitMetresPerMinute,      "mpmin" },
  { CSpeed::UnitMetresPerSecond,      "mps" },
  { CSpeed::UnitFeetPerHour,          "fth" },
  { CSpeed::UnitFeetPerMinute,        "ftm" },
  { CSpeed::UnitFeetPerSecond,        "fts" },
  { CSpeed::UnitMilesPerHour,         "mph" },
  { CSpeed::UnitKnots,                "kts" },
  { CSpeed::UnitBeaufort,             "beaufort" },
  { CSpeed::UnitInchPerSecond,        "inchs" },
  { CSpeed::UnitYardPerSecond,        "yards" },
  { CSpeed::UnitFurlongPerFortnight,  "fpf" }
};

#define SPEED_INFO_SIZE           sizeof(speedInfo) / sizeof(SpeedInfo)
#define SPEED_UNIT_STRINGS        20200

#define SETTING_REGIONAL_DEFAULT  "regional"

static std::string ToTimeFormat(bool use24HourClock, bool singleHour, bool meridiem)
{
  if (use24HourClock)
    return singleHour ? TIME_FORMAT_SINGLE_24 : TIME_FORMAT_DOUBLE_24;

  if (!meridiem)
    return singleHour ? TIME_FORMAT_SINGLE_12 : TIME_FORMAT_DOUBLE_12;

  return StringUtils::Format(g_localizeStrings.Get(12382).c_str(), ToTimeFormat(false, singleHour, false).c_str());
}

static std::string ToSettingTimeFormat(const CDateTime& time, const std::string& timeFormat)
{
  return StringUtils::Format(g_localizeStrings.Get(20036).c_str(), time.GetAsLocalizedTime(timeFormat, true).c_str(), timeFormat.c_str());
}

static std::string ToSettingTimeFormat(const CDateTime& time, bool use24HourClock, bool singleHour, bool meridiem)
{
  return ToSettingTimeFormat(time, ToTimeFormat(use24HourClock, singleHour, meridiem));
}

static CTemperature::Unit StringToTemperatureUnit(const std::string& temperatureUnit)
{
  std::string unit(temperatureUnit);
  StringUtils::ToLower(unit);

  for (size_t i = 0; i < TEMPERATURE_INFO_SIZE; i++)
  {
    const TemperatureInfo& info = temperatureInfo[i];
    if (info.name == unit)
      return info.unit;
  }

  return CTemperature::UnitCelsius;
}

static CSpeed::Unit StringToSpeedUnit(const std::string& speedUnit)
{
  std::string unit(speedUnit);
  StringUtils::ToLower(unit);

  for (size_t i = 0; i < SPEED_INFO_SIZE; i++)
  {
    const SpeedInfo& info = speedInfo[i];
    if (info.name == unit)
      return info.unit;
  }

  return CSpeed::UnitKilometresPerHour;
}

struct SortLanguage
{
  bool operator()(const std::pair<std::string, std::string> &left, const std::pair<std::string, std::string> &right)
  {
    std::string strLeft = left.first;
    std::string strRight = right.first;
    StringUtils::ToLower(strLeft);
    StringUtils::ToLower(strRight);

    return strLeft.compare(strRight) < 0;
  }
};

CLangInfo::CRegion::CRegion(const CRegion& region):
  m_strLangLocaleName(region.m_strLangLocaleName),
  m_strLangLocaleCodeTwoChar(region.m_strLangLocaleCodeTwoChar),
  m_strRegionLocaleName(region.m_strRegionLocaleName),
  m_strName(region.m_strName),
  m_strDateFormatLong(region.m_strDateFormatLong),
  m_strDateFormatShort(region.m_strDateFormatShort),
  m_strTimeFormat(region.m_strTimeFormat),
  m_strTimeZone(region.m_strTimeZone)
{
  m_strMeridiemSymbols[MeridiemSymbolPM] = region.m_strMeridiemSymbols[MeridiemSymbolPM];
  m_strMeridiemSymbols[MeridiemSymbolAM] = region.m_strMeridiemSymbols[MeridiemSymbolAM];
  m_tempUnit=region.m_tempUnit;
  m_speedUnit=region.m_speedUnit;
}

CLangInfo::CRegion::CRegion()
{
  SetDefaults();
}

CLangInfo::CRegion::~CRegion()
{

}

void CLangInfo::CRegion::SetDefaults()
{
  m_strName="N/A";
  m_strLangLocaleName = "English";
  m_strLangLocaleCodeTwoChar = "en";

  m_strDateFormatShort="DD/MM/YYYY";
  m_strDateFormatLong="DDDD, D MMMM YYYY";
  m_strTimeFormat="HH:mm:ss";
  m_tempUnit = CTemperature::UnitCelsius;
  m_speedUnit = CSpeed::UnitKilometresPerHour;
  m_strTimeZone.clear();
}

void CLangInfo::CRegion::SetTemperatureUnit(const std::string& strUnit)
{
  m_tempUnit = StringToTemperatureUnit(strUnit);
}

void CLangInfo::CRegion::SetSpeedUnit(const std::string& strUnit)
{
  m_speedUnit = StringToSpeedUnit(strUnit);
}

void CLangInfo::CRegion::SetTimeZone(const std::string& strTimeZone)
{
  m_strTimeZone = strTimeZone;
}

// set the locale associated with this region global. This affects string
// sorting & transformations
void CLangInfo::CRegion::SetGlobalLocale()
{
  std::string strLocale;
  if (m_strRegionLocaleName.length() > 0)
  {
    strLocale = m_strLangLocaleName + "_" + m_strRegionLocaleName;
#ifdef TARGET_POSIX
    strLocale += ".UTF-8";
#endif
  }

  CLog::Log(LOGDEBUG, "trying to set locale to %s", strLocale.c_str());

  // We need to set the locale to only change the collate. Otherwise,
  // decimal separator is changed depending of the current language
  // (ie. "," in French or Dutch instead of "."). This breaks atof() and
  // others similar functions.
#if defined(TARGET_FREEBSD) || defined(TARGET_DARWIN_OSX) || defined(__UCLIBC__)
  // on FreeBSD, darwin and uClibc-based systems libstdc++ is compiled with
  // "generic" locale support
  if (setlocale(LC_COLLATE, strLocale.c_str()) == NULL
  || setlocale(LC_CTYPE, strLocale.c_str()) == NULL)
  {
    strLocale = "C";
    setlocale(LC_COLLATE, strLocale.c_str());
    setlocale(LC_CTYPE, strLocale.c_str());
  }
#else
  locale current_locale = locale::classic(); // C-Locale
  try
  {
    locale lcl = locale(strLocale.c_str());
    strLocale = lcl.name();
    current_locale = current_locale.combine< collate<wchar_t> >(lcl);
    current_locale = current_locale.combine< ctype<wchar_t> >(lcl);

    assert(use_facet< numpunct<char> >(current_locale).decimal_point() == '.');

  } catch(...) {
    current_locale = locale::classic();
    strLocale = "C";
  }

  g_langInfo.m_systemLocale = current_locale; // TODO: move to CLangInfo class
  locale::global(current_locale);
#endif
  g_charsetConverter.resetSystemCharset();
  CLog::Log(LOGINFO, "global locale set to %s", strLocale.c_str());
}

CLangInfo::CLangInfo()
{
  SetDefaults();
  m_shortDateFormat = m_defaultRegion.m_strDateFormatShort;
  m_longDateFormat = m_defaultRegion.m_strDateFormatLong;
  m_timeFormat = m_defaultRegion.m_strTimeFormat;
  m_use24HourClock = DetermineUse24HourClockFromTimeFormat(m_defaultRegion.m_strTimeFormat);
  m_temperatureUnit = m_defaultRegion.m_tempUnit;
  m_speedUnit = m_defaultRegion.m_speedUnit;
}

CLangInfo::~CLangInfo()
{
}

void CLangInfo::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "locale.audiolanguage")
    SetAudioLanguage(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.subtitlelanguage")
    SetSubtitleLanguage(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.language")
  {
    if (!SetLanguage(((CSettingString*)setting)->GetValue()))
      ((CSettingString*)CSettings::Get().GetSetting("locale.language"))->Reset();
  }
  else if (settingId == "locale.country")
    SetCurrentRegion(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.shortdateformat")
    SetShortDateFormat(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.longdateformat")
    SetLongDateFormat(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.timeformat")
    SetTimeFormat(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.use24hourclock")
  {
    Set24HourClock(((CSettingString*)setting)->GetValue());

    // update the time format
    CSettings::Get().SetString("locale.timeformat", PrepareTimeFormat(GetTimeFormat(), m_use24HourClock));
  }
  else if (settingId == "locale.temperatureunit")
    SetTemperatureUnit(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.speedunit")
    SetSpeedUnit(((CSettingString*)setting)->GetValue());
}

void CLangInfo::OnSettingsLoaded()
{
  // set the temperature and speed units based on the settings
  SetShortDateFormat(CSettings::Get().GetString("locale.shortdateformat"));
  SetLongDateFormat(CSettings::Get().GetString("locale.longdateformat"));
  Set24HourClock(CSettings::Get().GetString("locale.use24hourclock"));
  SetTimeFormat(CSettings::Get().GetString("locale.timeformat"));
  SetTemperatureUnit(CSettings::Get().GetString("locale.temperatureunit"));
  SetSpeedUnit(CSettings::Get().GetString("locale.speedunit"));
}

bool CLangInfo::Load(const std::string& strLanguage)
{
  SetDefaults();

  string strFileName = GetLanguageInfoPath(strLanguage);

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strFileName))
  {
    CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  // get the matching language addon
  m_languageAddon = GetLanguageAddon(strLanguage);
  if (m_languageAddon == NULL)
  {
    CLog::Log(LOGERROR, "Unknown language %s", strLanguage.c_str());
    return false;
  }

  // get some language-specific information from the language addon
  m_strGuiCharSet = m_languageAddon->GetGuiCharset();
  m_forceUnicodeFont = m_languageAddon->ForceUnicodeFont();
  m_strSubtitleCharSet = m_languageAddon->GetSubtitleCharset();
  m_strDVDMenuLanguage = m_languageAddon->GetDvdMenuLanguage();
  m_strDVDAudioLanguage = m_languageAddon->GetDvdAudioLanguage();
  m_strDVDSubtitleLanguage = m_languageAddon->GetDvdSubtitleLanguage();
  m_sortTokens = m_languageAddon->GetSortTokens();


  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->ValueStr() != "language")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <language>", strFileName.c_str());
    return false;
  }

  if (pRootElement->Attribute("locale"))
    m_defaultRegion.m_strLangLocaleName = pRootElement->Attribute("locale");

#ifdef TARGET_WINDOWS
  // Windows need 3 chars isolang code
  if (m_defaultRegion.m_strLangLocaleName.length() == 2)
  {
    if (!g_LangCodeExpander.ConvertISO6391ToISO6392T(m_defaultRegion.m_strLangLocaleName, m_defaultRegion.m_strLangLocaleName, true))
      m_defaultRegion.m_strLangLocaleName = "";
  }

  if (!g_LangCodeExpander.ConvertWindowsLanguageCodeToISO6392T(m_defaultRegion.m_strLangLocaleName, m_languageCodeGeneral))
    m_languageCodeGeneral = "";
#else
  if (m_defaultRegion.m_strLangLocaleName.length() != 3)
  {
    if (!g_LangCodeExpander.ConvertToISO6392T(m_defaultRegion.m_strLangLocaleName, m_languageCodeGeneral))
      m_languageCodeGeneral = "";
  }
  else
    m_languageCodeGeneral = m_defaultRegion.m_strLangLocaleName;
#endif

  std::string tmp;
  if (g_LangCodeExpander.ConvertToISO6391(m_defaultRegion.m_strLangLocaleName, tmp))
    m_defaultRegion.m_strLangLocaleCodeTwoChar = tmp;

  const TiXmlNode *pRegions = pRootElement->FirstChild("regions");
  if (pRegions && !pRegions->NoChildren())
  {
    const TiXmlElement *pRegion=pRegions->FirstChildElement("region");
    while (pRegion)
    {
      CRegion region(m_defaultRegion);
      region.m_strName = XMLUtils::GetAttribute(pRegion, "name");
      if (region.m_strName.empty())
        region.m_strName="N/A";

      if (pRegion->Attribute("locale"))
        region.m_strRegionLocaleName = pRegion->Attribute("locale");

#ifdef TARGET_WINDOWS
      // Windows need 3 chars regions code
      if (region.m_strRegionLocaleName.length() == 2)
      {
        if (!g_LangCodeExpander.ConvertISO36111Alpha2ToISO36111Alpha3(region.m_strRegionLocaleName, region.m_strRegionLocaleName))
          region.m_strRegionLocaleName = "";
      }
#endif

      const TiXmlNode *pDateLong=pRegion->FirstChild("datelong");
      if (pDateLong && !pDateLong->NoChildren())
        region.m_strDateFormatLong=pDateLong->FirstChild()->ValueStr();

      const TiXmlNode *pDateShort=pRegion->FirstChild("dateshort");
      if (pDateShort && !pDateShort->NoChildren())
        region.m_strDateFormatShort=pDateShort->FirstChild()->ValueStr();

      const TiXmlElement *pTime=pRegion->FirstChildElement("time");
      if (pTime && !pTime->NoChildren())
      {
        region.m_strTimeFormat=pTime->FirstChild()->Value();
        region.m_strMeridiemSymbols[MeridiemSymbolAM] = XMLUtils::GetAttribute(pTime, "symbolAM");
        region.m_strMeridiemSymbols[MeridiemSymbolPM] = XMLUtils::GetAttribute(pTime, "symbolPM");
      }

      const TiXmlNode *pTempUnit=pRegion->FirstChild("tempunit");
      if (pTempUnit && !pTempUnit->NoChildren())
        region.SetTemperatureUnit(pTempUnit->FirstChild()->ValueStr());

      const TiXmlNode *pSpeedUnit=pRegion->FirstChild("speedunit");
      if (pSpeedUnit && !pSpeedUnit->NoChildren())
        region.SetSpeedUnit(pSpeedUnit->FirstChild()->ValueStr());

      const TiXmlNode *pTimeZone=pRegion->FirstChild("timezone");
      if (pTimeZone && !pTimeZone->NoChildren())
        region.SetTimeZone(pTimeZone->FirstChild()->ValueStr());

      m_regions.insert(PAIR_REGIONS(region.m_strName, region));

      pRegion=pRegion->NextSiblingElement("region");
    }

    const std::string& strName = CSettings::Get().GetString("locale.country");
    SetCurrentRegion(strName);
  }
  g_charsetConverter.reinitCharsetsFromSettings();

  return true;
}

std::string CLangInfo::GetLanguagePath(const std::string &language)
{
  if (language.empty())
    return "";

  std::string addonId = ADDON::CLanguageResource::GetAddonId(language);

  std::string path = URIUtils::AddFileToFolder(GetLanguagePath(), addonId);
  URIUtils::AddSlashAtEnd(path);

  return path;
}

std::string CLangInfo::GetLanguageInfoPath(const std::string &language)
{
  if (language.empty())
    return "";

  return URIUtils::AddFileToFolder(GetLanguagePath(language), "langinfo.xml");
}

void CLangInfo::LoadTokens(const TiXmlNode* pTokens, set<std::string>& vecTokens)
{
  if (pTokens && !pTokens->NoChildren())
  {
    const TiXmlElement *pToken = pTokens->FirstChildElement("token");
    while (pToken)
    {
      std::string strSep= " ._";
      if (pToken->Attribute("separators"))
        strSep = pToken->Attribute("separators");
      if (pToken->FirstChild() && pToken->FirstChild()->Value())
      {
        if (strSep.empty())
          vecTokens.insert(pToken->FirstChild()->ValueStr());
        else
          for (unsigned int i=0;i<strSep.size();++i)
            vecTokens.insert(pToken->FirstChild()->ValueStr() + strSep[i]);
      }
      pToken = pToken->NextSiblingElement();
    }
  }
}

void CLangInfo::SetDefaults()
{
  m_regions.clear();

  //Reset default region
  m_defaultRegion.SetDefaults();

  // Set the default region, we may be unable to load langinfo.xml
  m_currentRegion = &m_defaultRegion;

  m_systemLocale = std::locale::classic();

  m_forceUnicodeFont = false;
  m_strGuiCharSet = "CP1252";
  m_strSubtitleCharSet = "CP1252";
  m_strDVDMenuLanguage = "en";
  m_strDVDAudioLanguage = "en";
  m_strDVDSubtitleLanguage = "en";
  m_sortTokens.clear();
  
  m_languageCodeGeneral = "eng";
}

std::string CLangInfo::GetGuiCharSet() const
{
  CSettingString* charsetSetting = static_cast<CSettingString*>(CSettings::Get().GetSetting("locale.charset"));
  if (charsetSetting->IsDefault())
    return m_strGuiCharSet;

  return charsetSetting->GetValue();
}

std::string CLangInfo::GetSubtitleCharSet() const
{
  CSettingString* charsetSetting = static_cast<CSettingString*>(CSettings::Get().GetSetting("subtitles.charset"));
  if (charsetSetting->IsDefault())
    return m_strSubtitleCharSet;

  return charsetSetting->GetValue();
}

LanguageResourcePtr CLangInfo::GetLanguageAddon(const std::string& locale /* = "" */) const
{
  if (locale.empty() ||
     (m_languageAddon != NULL && (locale.compare(m_languageAddon->ID()) == 0 || m_languageAddon->GetLocale().Equals(locale))))
    return m_languageAddon;

  std::string addonId = ADDON::CLanguageResource::GetAddonId(locale);
  if (addonId.empty())
    addonId = CSettings::Get().GetString("locale.language");

  ADDON::AddonPtr addon;
  if (ADDON::CAddonMgr::Get().GetAddon(addonId, addon, ADDON::ADDON_RESOURCE_LANGUAGE, true) && addon != NULL)
    return std::dynamic_pointer_cast<ADDON::CLanguageResource>(addon);

  return NULL;
}

std::string CLangInfo::GetEnglishLanguageName(const std::string& locale /* = "" */) const
{
  LanguageResourcePtr addon = GetLanguageAddon(locale);
  if (addon == NULL)
    return "";

  return addon->Name();
}

bool CLangInfo::SetLanguage(const std::string &strLanguage /* = "" */, bool reloadServices /* = true */)
{
  bool fallback;
  return SetLanguage(fallback, strLanguage, reloadServices);
}

bool CLangInfo::SetLanguage(bool& fallback, const std::string &strLanguage /* = "" */, bool reloadServices /* = true */)
{
  fallback = false;

  std::string language = strLanguage;
  if (language.empty())
  {
    language = CSettings::Get().GetString("locale.language");

    if (language.empty())
    {
      CLog::Log(LOGFATAL, "CLangInfo: cannot load empty language.");
      return false;
    }
  }

  LanguageResourcePtr languageAddon = GetLanguageAddon(language);
  if (languageAddon == NULL)
  {
    CLog::Log(LOGWARNING, "CLangInfo: unable to load language \"%s\". Trying to determine matching language addon...", language.c_str());

    // we may have to fall back to the default language
    std::string defaultLanguage = static_cast<CSettingString*>(CSettings::Get().GetSetting("locale.language"))->GetDefault();
    std::string newLanguage = defaultLanguage;

    // try to determine a language addon matching the given language in name
    if (!ADDON::CLanguageResource::FindLanguageAddonByName(language, newLanguage))
    {
      CLog::Log(LOGWARNING, "CLangInfo: unable to find an installed language addon matching \"%s\". Trying to find an installable language...", language.c_str());

      bool foundMatchingAddon = false;
      CAddonDatabase addondb;
      if (addondb.Open())
      {
        // update the addon repositories to check if there's a matching language addon available for download
        CAddonInstaller::Get().UpdateRepos(true, true);

        ADDON::VECADDONS languageAddons;
        if (addondb.GetAddons(languageAddons, ADDON::ADDON_RESOURCE_LANGUAGE) && !languageAddons.empty())
        {
          // try to get the proper language addon by its name from all available language addons
          if (ADDON::CLanguageResource::FindLanguageAddonByName(language, newLanguage, languageAddons))
          {
            if (CAddonInstaller::Get().Install(newLanguage, true, "", false, false))
            {
              CLog::Log(LOGINFO, "CLangInfo: successfully installed language addon \"%s\" matching current language \"%s\"", newLanguage.c_str(), language.c_str());
              foundMatchingAddon = true;
            }
            else
              CLog::Log(LOGERROR, "CLangInfo: failed to installed language addon \"%s\" matching current language \"%s\"", newLanguage.c_str(), language.c_str());
          }
          else
            CLog::Log(LOGERROR, "CLangInfo: unable to match old language \"%s\" to any available language addon", language.c_str());
        }
        else
          CLog::Log(LOGERROR, "CLangInfo: no language addons available to match against \"%s\"", language.c_str());
      }
      else
        CLog::Log(LOGERROR, "CLangInfo: unable to open addon database to look for a language addon matching \"%s\"", language.c_str());

      // if the new language matches the default language we are loading the
      // default language as a fallback
      if (!foundMatchingAddon && newLanguage == defaultLanguage)
      {
        CLog::Log(LOGINFO, "CLangInfo: fall back to the default language \"%s\"", defaultLanguage.c_str());
        fallback = true;
      }
    }

    if (!CSettings::Get().SetString("locale.language", newLanguage))
      return false;

    CSettings::Get().Save();
    return true;
  }

  CLog::Log(LOGINFO, "CLangInfo: loading %s language information...", language.c_str());
  if (!Load(language))
  {
    CLog::LogF(LOGFATAL, "CLangInfo: failed to load %s language information", language.c_str());
    return false;
  }

  CLog::Log(LOGINFO, "CLangInfo: loading %s language strings...", language.c_str());
  if (!g_localizeStrings.Load(GetLanguagePath(), language))
  {
    CLog::LogF(LOGFATAL, "CLangInfo: failed to load %s language strings", language.c_str());
    return false;
  }

  if (reloadServices)
  {
    // also tell our weather and skin to reload as these are localized
    g_weatherManager.Refresh();
    g_PVRManager.LocalizationChanged();
    CApplicationMessenger::Get().ExecBuiltIn("ReloadSkin", false);
  }

  return true;
}

// three char language code (not win32 specific)
const std::string& CLangInfo::GetAudioLanguage() const
{
  if (!m_audioLanguage.empty())
    return m_audioLanguage;

  return m_languageCodeGeneral;
}

void CLangInfo::SetAudioLanguage(const std::string& language)
{
  if (language.empty()
    || StringUtils::EqualsNoCase(language, "default")
    || StringUtils::EqualsNoCase(language, "original")
    || !g_LangCodeExpander.ConvertToISO6392T(language, m_audioLanguage))
    m_audioLanguage.clear();
}

// three char language code (not win32 specific)
const std::string& CLangInfo::GetSubtitleLanguage() const
{
  if (!m_subtitleLanguage.empty())
    return m_subtitleLanguage;

  return m_languageCodeGeneral;
}

void CLangInfo::SetSubtitleLanguage(const std::string& language)
{
  if (language.empty()
    || StringUtils::EqualsNoCase(language, "default")
    || StringUtils::EqualsNoCase(language, "original")
    || !g_LangCodeExpander.ConvertToISO6392T(language, m_subtitleLanguage))
    m_subtitleLanguage.clear();
}

// two character codes as defined in ISO639
const std::string CLangInfo::GetDVDMenuLanguage() const
{
  std::string code;
  if (!g_LangCodeExpander.ConvertToISO6391(m_currentRegion->m_strLangLocaleName, code))
    code = m_strDVDMenuLanguage;
  
  return code;
}

// two character codes as defined in ISO639
const std::string CLangInfo::GetDVDAudioLanguage() const
{
  std::string code;
  if (!g_LangCodeExpander.ConvertToISO6391(m_audioLanguage, code))
    code = m_strDVDAudioLanguage;
  
  return code;
}

// two character codes as defined in ISO639
const std::string CLangInfo::GetDVDSubtitleLanguage() const
{
  std::string code;
  if (!g_LangCodeExpander.ConvertToISO6391(m_subtitleLanguage, code))
    code = m_strDVDSubtitleLanguage;
  
  return code;
}

const CLocale& CLangInfo::GetLocale() const
{
  LanguageResourcePtr language = GetLanguageAddon();
  if (language != NULL)
    return language->GetLocale();

  return CLocale::Empty;
}

const std::string& CLangInfo::GetRegionLocale() const
{
  return m_currentRegion->m_strRegionLocaleName;
}

// Returns the format string for the date of the current language
const std::string& CLangInfo::GetDateFormat(bool bLongDate /* = false */) const
{
  if (bLongDate)
    return GetLongDateFormat();

  return GetShortDateFormat();
}

void CLangInfo::SetDateFormat(const std::string& dateFormat, bool bLongDate /* = false */)
{
  if (bLongDate)
    SetLongDateFormat(dateFormat);
  else
    SetShortDateFormat(dateFormat);
}

const std::string& CLangInfo::GetShortDateFormat() const
{
  return m_shortDateFormat;
}

void CLangInfo::SetShortDateFormat(const std::string& shortDateFormat)
{
  std::string newShortDateFormat = shortDateFormat;
  if (shortDateFormat == SETTING_REGIONAL_DEFAULT)
    newShortDateFormat = m_currentRegion->m_strDateFormatShort;

  m_shortDateFormat = newShortDateFormat;
}

const std::string& CLangInfo::GetLongDateFormat() const
{
  return m_longDateFormat;
}

void CLangInfo::SetLongDateFormat(const std::string& longDateFormat)
{
  std::string newLongDateFormat = longDateFormat;
  if (longDateFormat == SETTING_REGIONAL_DEFAULT)
    newLongDateFormat = m_currentRegion->m_strDateFormatShort;

  m_longDateFormat = newLongDateFormat;
}

// Returns the format string for the time of the current language
const std::string& CLangInfo::GetTimeFormat() const
{
  return m_timeFormat;
}

void CLangInfo::SetTimeFormat(const std::string& timeFormat)
{
  std::string newTimeFormat = timeFormat;
  if (timeFormat == SETTING_REGIONAL_DEFAULT)
    newTimeFormat = m_currentRegion->m_strTimeFormat;

  m_timeFormat = PrepareTimeFormat(newTimeFormat, m_use24HourClock);
}

bool CLangInfo::Use24HourClock() const
{
  return m_use24HourClock;
}

void CLangInfo::Set24HourClock(bool use24HourClock)
{
  m_use24HourClock = use24HourClock;
}

void CLangInfo::Set24HourClock(const std::string& str24HourClock)
{
  bool use24HourClock = false;
  if (str24HourClock == TIME_FORMAT_12HOURS)
    use24HourClock = false;
  else if (str24HourClock == TIME_FORMAT_24HOURS)
    use24HourClock = true;
  else if (str24HourClock == SETTING_REGIONAL_DEFAULT)
  {
    Set24HourClock(m_currentRegion->m_strTimeFormat);
    return;
  }
  else
    use24HourClock = DetermineUse24HourClockFromTimeFormat(str24HourClock);

  if (m_use24HourClock == use24HourClock)
    return;

  m_use24HourClock = use24HourClock;
}

const std::string& CLangInfo::GetTimeZone() const
{
  return m_currentRegion->m_strTimeZone;
}

// Returns the AM/PM symbol of the current language
const std::string& CLangInfo::GetMeridiemSymbol(MeridiemSymbol symbol) const
{
  // nothing to return if we use 24-hour clock
  if (m_use24HourClock)
    return StringUtils::Empty;

  return MeridiemSymbolToString(symbol);
}

const std::string& CLangInfo::MeridiemSymbolToString(MeridiemSymbol symbol)
{
  switch (symbol)
  {
  case MeridiemSymbolAM:
    return g_localizeStrings.Get(378);

  case MeridiemSymbolPM:
    return g_localizeStrings.Get(379);

  default:
    break;
  }

  return StringUtils::Empty;
}

// Fills the array with the region names available for this language
void CLangInfo::GetRegionNames(vector<string>& array)
{
  for (ITMAPREGIONS it=m_regions.begin(); it!=m_regions.end(); ++it)
  {
    std::string strName=it->first;
    if (strName=="N/A")
      strName=g_localizeStrings.Get(416);
    array.push_back(strName);
  }
}

// Set the current region by its name, names from GetRegionNames() are valid.
// If the region is not found the first available region is set.
void CLangInfo::SetCurrentRegion(const std::string& strName)
{
  ITMAPREGIONS it=m_regions.find(strName);
  if (it!=m_regions.end())
    m_currentRegion=&it->second;
  else if (!m_regions.empty())
    m_currentRegion=&m_regions.begin()->second;
  else
    m_currentRegion=&m_defaultRegion;

  m_currentRegion->SetGlobalLocale();

  if (CSettings::Get().GetString("locale.shortdateformat") == SETTING_REGIONAL_DEFAULT)
    SetShortDateFormat(m_currentRegion->m_strDateFormatShort);
  if (CSettings::Get().GetString("locale.longdateformat") == SETTING_REGIONAL_DEFAULT)
    SetLongDateFormat(m_currentRegion->m_strDateFormatLong);
  if (CSettings::Get().GetString("locale.use24hourclock") == SETTING_REGIONAL_DEFAULT)
    Set24HourClock(m_currentRegion->m_strTimeFormat);
  if (CSettings::Get().GetString("locale.timeformat") == SETTING_REGIONAL_DEFAULT)
    SetTimeFormat(m_currentRegion->m_strTimeFormat);
  if (CSettings::Get().GetString("locale.temperatureunit") == SETTING_REGIONAL_DEFAULT)
    SetTemperatureUnit(m_currentRegion->m_tempUnit);
  if (CSettings::Get().GetString("locale.speedunit") == SETTING_REGIONAL_DEFAULT)
    SetSpeedUnit(m_currentRegion->m_speedUnit);
}

// Returns the current region set for this language
const std::string& CLangInfo::GetCurrentRegion() const
{
  return m_currentRegion->m_strName;
}

CTemperature::Unit CLangInfo::GetTemperatureUnit() const
{
  return m_temperatureUnit;
}

void CLangInfo::SetTemperatureUnit(CTemperature::Unit temperatureUnit)
{
  if (m_temperatureUnit == temperatureUnit)
    return;

  m_temperatureUnit = temperatureUnit;

  // need to reset our weather as temperatures need re-translating
  g_weatherManager.Refresh();
}

void CLangInfo::SetTemperatureUnit(const std::string& temperatureUnit)
{
  CTemperature::Unit unit = CTemperature::UnitCelsius;
  if (temperatureUnit == SETTING_REGIONAL_DEFAULT)
    unit = m_currentRegion->m_tempUnit;
  else
    unit = StringToTemperatureUnit(temperatureUnit);

  SetTemperatureUnit(unit);
}

std::string CLangInfo::GetTemperatureAsString(const CTemperature& temperature) const
{
  if (!temperature.IsValid())
    return g_localizeStrings.Get(13205); // "Unknown"

  CTemperature::Unit temperatureUnit = GetTemperatureUnit();
  return StringUtils::Format("%s%s", temperature.ToString(temperatureUnit).c_str(), GetTemperatureUnitString().c_str());
}

// Returns the temperature unit string for the current language
const std::string& CLangInfo::GetTemperatureUnitString() const
{
  return GetTemperatureUnitString(m_temperatureUnit);
}

const std::string& CLangInfo::GetTemperatureUnitString(CTemperature::Unit temperatureUnit)
{
  return g_localizeStrings.Get(TEMP_UNIT_STRINGS + temperatureUnit);
}

void CLangInfo::SetSpeedUnit(CSpeed::Unit speedUnit)
{
  if (m_speedUnit == speedUnit)
    return;

  m_speedUnit = speedUnit;

  // need to reset our weather as speeds need re-translating
  g_weatherManager.Refresh();
}

void CLangInfo::SetSpeedUnit(const std::string& speedUnit)
{
  CSpeed::Unit unit = CSpeed::UnitKilometresPerHour;
  if (speedUnit == SETTING_REGIONAL_DEFAULT)
    unit = m_currentRegion->m_speedUnit;
  else
    unit = StringToSpeedUnit(speedUnit);

  SetSpeedUnit(unit);
}

CSpeed::Unit CLangInfo::GetSpeedUnit() const
{
  return m_speedUnit;
}

std::string CLangInfo::GetSpeedAsString(const CSpeed& speed) const
{
  if (!speed.IsValid())
    return g_localizeStrings.Get(13205); // "Unknown"

  return StringUtils::Format("%s%s", speed.ToString(GetSpeedUnit()).c_str(), GetSpeedUnitString().c_str());
}

// Returns the speed unit string for the current language
const std::string& CLangInfo::GetSpeedUnitString() const
{
  return GetSpeedUnitString(m_speedUnit);
}

const std::string& CLangInfo::GetSpeedUnitString(CSpeed::Unit speedUnit)
{
  return g_localizeStrings.Get(SPEED_UNIT_STRINGS + speedUnit);
}

std::set<std::string> CLangInfo::GetSortTokens() const
{
  std::set<std::string> sortTokens = m_sortTokens;
  sortTokens.insert(g_advancedSettings.m_vecTokens.begin(), g_advancedSettings.m_vecTokens.end());

  return sortTokens;
}

bool CLangInfo::DetermineUse24HourClockFromTimeFormat(const std::string& timeFormat)
{
  // if the time format contains a "h" it's 12-hour and otherwise 24-hour clock format
  if (timeFormat.find("h") != std::string::npos)
    return false;

  return true;
}

bool CLangInfo::DetermineUseMeridiemFromTimeFormat(const std::string& timeFormat)
{
  // if the time format contains "xx" it's using meridiem
  if (timeFormat.find("xx") != std::string::npos)
    return true;

  return false;
}

std::string CLangInfo::PrepareTimeFormat(const std::string& timeFormat, bool use24HourClock)
{
  std::string preparedTimeFormat = timeFormat;
  if (use24HourClock)
  {
    // replace all "h" with "H"
    StringUtils::Replace(preparedTimeFormat, 'h', 'H');

    // remove any "xx" for meridiem
    StringUtils::Replace(preparedTimeFormat, "x", "");
  }
  else
    // replace all "H" with "h"
    StringUtils::Replace(preparedTimeFormat, 'H', 'h');

  StringUtils::Trim(preparedTimeFormat);

  return preparedTimeFormat;
}

void CLangInfo::SettingOptionsLanguageNamesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  // find languages...
  ADDON::VECADDONS addons;
  if (!ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_RESOURCE_LANGUAGE, addons, true))
    return;

  for (ADDON::VECADDONS::const_iterator addon = addons.begin(); addon != addons.end(); ++addon)
    list.push_back(make_pair((*addon)->Name(), (*addon)->Name()));

  sort(list.begin(), list.end(), SortLanguage());
}

void CLangInfo::SettingOptionsISO6391LanguagesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  // get a list of language names
  vector<string> languages = g_LangCodeExpander.GetLanguageNames(CLangCodeExpander::ISO_639_1, true);
  sort(languages.begin(), languages.end(), sortstringbyname());
  for (std::vector<std::string>::const_iterator language = languages.begin(); language != languages.end(); ++language)
    list.push_back(make_pair(*language, *language));
}

void CLangInfo::SettingOptionsStreamLanguagesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(make_pair(g_localizeStrings.Get(308), "original"));
  list.push_back(make_pair(g_localizeStrings.Get(309), "default"));

  std::string dummy;
  SettingOptionsISO6391LanguagesFiller(NULL, list, dummy, NULL);
}

void CLangInfo::SettingOptionsRegionsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  vector<string> regions;
  g_langInfo.GetRegionNames(regions);
  sort(regions.begin(), regions.end(), sortstringbyname());

  bool match = false;
  for (unsigned int i = 0; i < regions.size(); ++i)
  {
    std::string region = regions[i];
    list.push_back(make_pair(region, region));

    if (!match && region == ((CSettingString*)setting)->GetValue())
    {
      match = true;
      current = region;
    }
  }

  if (!match && regions.size() > 0)
    current = regions[0];
}

void CLangInfo::SettingOptionsShortDateFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& shortDateFormatSetting = static_cast<const CSettingString*>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();

  list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(20035).c_str(), now.GetAsLocalizedDate(g_langInfo.m_currentRegion->m_strDateFormatShort).c_str()), SETTING_REGIONAL_DEFAULT));
  if (shortDateFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (size_t i = 0; i < SHORT_DATE_FORMATS_SIZE; i++)
  {
    const std::string& shortDateFormat = shortDateFormats[i];
    list.push_back(std::make_pair(now.GetAsLocalizedDate(shortDateFormat), shortDateFormat));

    if (!match && shortDateFormatSetting == shortDateFormat)
    {
      match = true;
      current = shortDateFormat;
    }
  }

  if (!match && !list.empty())
    current = list[0].second;
}

void CLangInfo::SettingOptionsLongDateFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& longDateFormatSetting = static_cast<const CSettingString*>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();

  list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(20035).c_str(), now.GetAsLocalizedDate(g_langInfo.m_currentRegion->m_strDateFormatLong).c_str()), SETTING_REGIONAL_DEFAULT));
  if (longDateFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (size_t i = 0; i < LONG_DATE_FORMATS_SIZE; i++)
  {
    const std::string& longDateFormat = longDateFormats[i];
    list.push_back(std::make_pair(now.GetAsLocalizedDate(longDateFormat), longDateFormat));

    if (!match && longDateFormatSetting == longDateFormat)
    {
      match = true;
      current = longDateFormat;
    }
  }

  if (!match && !list.empty())
    current = list[0].second;
}

void CLangInfo::SettingOptionsTimeFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& timeFormatSetting = static_cast<const CSettingString*>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();
  bool use24hourFormat = g_langInfo.Use24HourClock();

  list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(20035).c_str(), ToSettingTimeFormat(now, g_langInfo.m_currentRegion->m_strTimeFormat).c_str()), SETTING_REGIONAL_DEFAULT));
  if (timeFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  if (use24hourFormat)
  {
    list.push_back(std::make_pair(ToSettingTimeFormat(now, TIME_FORMAT_SINGLE_24), TIME_FORMAT_SINGLE_24));
    if (timeFormatSetting == TIME_FORMAT_SINGLE_24)
    {
      current = TIME_FORMAT_SINGLE_24;
      match = true;
    }

    list.push_back(std::make_pair(ToSettingTimeFormat(now, TIME_FORMAT_DOUBLE_24), TIME_FORMAT_DOUBLE_24));
    if (timeFormatSetting == TIME_FORMAT_DOUBLE_24)
    {
      current = TIME_FORMAT_DOUBLE_24;
      match = true;
    }
  }
  else
  {
    list.push_back(std::make_pair(ToSettingTimeFormat(now, TIME_FORMAT_SINGLE_12), TIME_FORMAT_SINGLE_12));
    if (timeFormatSetting == TIME_FORMAT_SINGLE_12)
    {
      current = TIME_FORMAT_SINGLE_12;
      match = true;
    }

    list.push_back(std::make_pair(ToSettingTimeFormat(now, TIME_FORMAT_DOUBLE_12), TIME_FORMAT_DOUBLE_12));
    if (timeFormatSetting == TIME_FORMAT_DOUBLE_12)
    {
      current = TIME_FORMAT_DOUBLE_12;
      match = true;
    }

    std::string timeFormatSingle12Meridiem = ToTimeFormat(false, true, true);
    list.push_back(std::make_pair(ToSettingTimeFormat(now, timeFormatSingle12Meridiem), timeFormatSingle12Meridiem));
    if (timeFormatSetting == timeFormatSingle12Meridiem)
    {
      current = timeFormatSingle12Meridiem;
      match = true;
    }

    std::string timeFormatDouble12Meridiem = ToTimeFormat(false, false, true);
    list.push_back(std::make_pair(ToSettingTimeFormat(now, timeFormatDouble12Meridiem), timeFormatDouble12Meridiem));
    if (timeFormatSetting == timeFormatDouble12Meridiem)
    {
      current = timeFormatDouble12Meridiem;
      match = true;
    }
  }

  if (!match && !list.empty())
    current = list[0].second;
}

void CLangInfo::SettingOptions24HourClockFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& clock24HourFormatSetting = static_cast<const CSettingString*>(setting)->GetValue();

  // determine the 24-hour clock format of the regional setting
  int regionalClock24HourFormatLabel = DetermineUse24HourClockFromTimeFormat(g_langInfo.m_currentRegion->m_strTimeFormat) ? 12384 : 12383;
  list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(20035).c_str(), g_localizeStrings.Get(regionalClock24HourFormatLabel).c_str()), SETTING_REGIONAL_DEFAULT));
  if (clock24HourFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  list.push_back(std::make_pair(g_localizeStrings.Get(12383), TIME_FORMAT_12HOURS));
  if (clock24HourFormatSetting == TIME_FORMAT_12HOURS)
  {
    current = TIME_FORMAT_12HOURS;
    match = true;
  }

  list.push_back(std::make_pair(g_localizeStrings.Get(12384), TIME_FORMAT_24HOURS));
  if (clock24HourFormatSetting == TIME_FORMAT_24HOURS)
  {
    current = TIME_FORMAT_24HOURS;
    match = true;
  }

  if (!match && !list.empty())
    current = list[0].second;
}

void CLangInfo::SettingOptionsTemperatureUnitsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& temperatureUnitSetting = static_cast<const CSettingString*>(setting)->GetValue();

  list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(20035).c_str(), GetTemperatureUnitString(g_langInfo.m_currentRegion->m_tempUnit).c_str()), SETTING_REGIONAL_DEFAULT));
  if (temperatureUnitSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (size_t i = 0; i < TEMPERATURE_INFO_SIZE; i++)
  {
    const TemperatureInfo& info = temperatureInfo[i];
    list.push_back(std::make_pair(GetTemperatureUnitString(info.unit), info.name));

    if (!match && temperatureUnitSetting == info.name)
    {
      match = true;
      current = info.name;
    }
  }

  if (!match && !list.empty())
    current = list[0].second;
}

void CLangInfo::SettingOptionsSpeedUnitsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& speedUnitSetting = static_cast<const CSettingString*>(setting)->GetValue();

  list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(20035).c_str(), GetSpeedUnitString(g_langInfo.m_currentRegion->m_speedUnit).c_str()), SETTING_REGIONAL_DEFAULT));
  if (speedUnitSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (size_t i = 0; i < SPEED_INFO_SIZE; i++)
  {
    const SpeedInfo& info = speedInfo[i];
    list.push_back(std::make_pair(GetSpeedUnitString(info.unit), info.name));

    if (!match && speedUnitSetting == info.name)
    {
      match = true;
      current = info.name;
    }
  }

  if (!match && !list.empty())
    current = list[0].second;
}
