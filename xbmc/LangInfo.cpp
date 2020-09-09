/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangInfo.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/LanguageResource.h"
#include "addons/RepositoryUpdater.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/CharsetConverter.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "weather/WeatherManager.h"

#include <algorithm>
#include <stdexcept>

using namespace PVR;
using namespace KODI::MESSAGING;

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

static CTemperature::Unit StringToTemperatureUnit(const std::string& temperatureUnit)
{
  std::string unit(temperatureUnit);
  StringUtils::ToLower(unit);

  for (const TemperatureInfo& info : temperatureInfo)
  {
    if (info.name == unit)
      return info.unit;
  }

  return CTemperature::UnitCelsius;
}

static CSpeed::Unit StringToSpeedUnit(const std::string& speedUnit)
{
  std::string unit(speedUnit);
  StringUtils::ToLower(unit);

  for (const SpeedInfo& info : speedInfo)
  {
    if (info.name == unit)
      return info.unit;
  }

  return CSpeed::UnitKilometresPerHour;
}

struct SortLanguage
{
  bool operator()(const StringSettingOption &left, const StringSettingOption &right) const
  {
    std::string strLeft = left.label;
    std::string strRight = right.label;
    StringUtils::ToLower(strLeft);
    StringUtils::ToLower(strRight);

    return strLeft.compare(strRight) < 0;
  }
};

CLangInfo::CRegion::CRegion()
{
  SetDefaults();
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

void CLangInfo::CRegion::SetGlobalLocale()
{
  std::string strLocale;
  if (m_strRegionLocaleName.length() > 0)
  {
#ifdef TARGET_WINDOWS
    std::string strLang, strRegion;
    g_LangCodeExpander.ConvertToISO6391(m_strLangLocaleName, strLang);
    g_LangCodeExpander.ConvertToISO6391(m_strRegionLocaleName, strRegion);
    strLocale = strLang + "-" + strRegion;
#else
    strLocale = m_strLangLocaleName + "_" + m_strRegionLocaleName;
#endif
#ifdef TARGET_POSIX
    strLocale += ".UTF-8";
#endif
  }
  g_langInfo.m_originalLocale = std::locale(std::locale::classic(), new custom_numpunct(m_cDecimalSep, m_cThousandsSep, m_strGrouping));

  CLog::Log(LOGDEBUG, "trying to set locale to %s", strLocale.c_str());

  // We need to set the locale to only change the collate. Otherwise,
  // decimal separator is changed depending of the current language
  // (ie. "," in French or Dutch instead of "."). This breaks atof() and
  // others similar functions.
#if !(defined(TARGET_FREEBSD) || defined(TARGET_DARWIN_OSX) || defined(__UCLIBC__))
  // on FreeBSD, darwin and uClibc-based systems libstdc++ is compiled with
  // "generic" locale support
  std::locale current_locale = std::locale::classic(); // C-Locale
  try
  {
    std::locale lcl = std::locale(strLocale.c_str());
    strLocale = lcl.name();
    current_locale = current_locale.combine< std::collate<wchar_t> >(lcl);
    current_locale = current_locale.combine< std::ctype<wchar_t> >(lcl);
    current_locale = current_locale.combine< std::time_get<wchar_t> >(lcl);
    current_locale = current_locale.combine< std::time_put<wchar_t> >(lcl);

    assert(std::use_facet< std::numpunct<char> >(current_locale).decimal_point() == '.');

  } catch(...) {
    current_locale = std::locale::classic();
    strLocale = "C";
  }

  g_langInfo.m_systemLocale = current_locale; //! @todo move to CLangInfo class
  g_langInfo.m_collationtype = 0;
  std::locale::global(current_locale);
#endif

#ifndef TARGET_WINDOWS
  if (setlocale(LC_COLLATE, strLocale.c_str()) == NULL ||
      setlocale(LC_CTYPE, strLocale.c_str()) == NULL ||
      setlocale(LC_TIME, strLocale.c_str()) == NULL)
  {
    strLocale = "C";
    setlocale(LC_COLLATE, strLocale.c_str());
    setlocale(LC_CTYPE, strLocale.c_str());
    setlocale(LC_TIME, strLocale.c_str());
  }
#else
  std::wstring strLocaleW;
  g_charsetConverter.utf8ToW(strLocale, strLocaleW);
  if (_wsetlocale(LC_COLLATE, strLocaleW.c_str()) == NULL ||
      _wsetlocale(LC_CTYPE, strLocaleW.c_str()) == NULL ||
      _wsetlocale(LC_TIME, strLocaleW.c_str()) == NULL)
  {
    strLocale = "C";
    strLocaleW = L"C";
    _wsetlocale(LC_COLLATE, strLocaleW.c_str());
    _wsetlocale(LC_CTYPE, strLocaleW.c_str());
    _wsetlocale(LC_TIME, strLocaleW.c_str());
  }
#endif

  g_charsetConverter.resetSystemCharset();
  CLog::Log(LOGINFO, "global locale set to %s", strLocale.c_str());

#ifdef TARGET_ANDROID
  // Force UTF8 for, e.g., vsnprintf
  setlocale(LC_ALL, "C.UTF-8");
#endif
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
  m_collationtype = 0;
}

CLangInfo::~CLangInfo() = default;

void CLangInfo::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_AUDIOLANGUAGE)
    SetAudioLanguage(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_SUBTITLELANGUAGE)
    SetSubtitleLanguage(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_LANGUAGE)
  {
    if (!SetLanguage(std::static_pointer_cast<const CSettingString>(setting)->GetValue()))
      std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_LOCALE_LANGUAGE))->Reset();
  }
  else if (settingId == CSettings::SETTING_LOCALE_COUNTRY)
    SetCurrentRegion(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_SHORTDATEFORMAT)
    SetShortDateFormat(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_LONGDATEFORMAT)
    SetLongDateFormat(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_TIMEFORMAT)
    SetTimeFormat(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_USE24HOURCLOCK)
  {
    Set24HourClock(std::static_pointer_cast<const CSettingString>(setting)->GetValue());

    // update the time format
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_TIMEFORMAT, PrepareTimeFormat(GetTimeFormat(), m_use24HourClock));
  }
  else if (settingId == CSettings::SETTING_LOCALE_TEMPERATUREUNIT)
    SetTemperatureUnit(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == CSettings::SETTING_LOCALE_SPEEDUNIT)
    SetSpeedUnit(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
}

void CLangInfo::OnSettingsLoaded()
{
  // set the temperature and speed units based on the settings
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  SetShortDateFormat(settings->GetString(CSettings::SETTING_LOCALE_SHORTDATEFORMAT));
  SetLongDateFormat(settings->GetString(CSettings::SETTING_LOCALE_LONGDATEFORMAT));
  Set24HourClock(settings->GetString(CSettings::SETTING_LOCALE_USE24HOURCLOCK));
  SetTimeFormat(settings->GetString(CSettings::SETTING_LOCALE_TIMEFORMAT));
  SetTemperatureUnit(settings->GetString(CSettings::SETTING_LOCALE_TEMPERATUREUNIT));
  SetSpeedUnit(settings->GetString(CSettings::SETTING_LOCALE_SPEEDUNIT));
}

bool CLangInfo::Load(const std::string& strLanguage)
{
  SetDefaults();

  std::string strFileName = GetLanguageInfoPath(strLanguage);

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
    if (!g_LangCodeExpander.ConvertISO6391ToISO6392B(m_defaultRegion.m_strLangLocaleName, m_defaultRegion.m_strLangLocaleName, true))
      m_defaultRegion.m_strLangLocaleName = "";
  }

  if (!g_LangCodeExpander.ConvertWindowsLanguageCodeToISO6392B(m_defaultRegion.m_strLangLocaleName, m_languageCodeGeneral))
    m_languageCodeGeneral = "";
#else
  if (m_defaultRegion.m_strLangLocaleName.length() != 3)
  {
    if (!g_LangCodeExpander.ConvertToISO6392B(m_defaultRegion.m_strLangLocaleName, m_languageCodeGeneral))
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
        region.m_strName=g_localizeStrings.Get(10005); // Not available

      if (pRegion->Attribute("locale"))
        region.m_strRegionLocaleName = pRegion->Attribute("locale");

#ifdef TARGET_WINDOWS
      // Windows need 3 chars regions code
      if (region.m_strRegionLocaleName.length() == 2)
      {
        if (!g_LangCodeExpander.ConvertISO31661Alpha2ToISO31661Alpha3(region.m_strRegionLocaleName, region.m_strRegionLocaleName))
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

      const TiXmlElement *pThousandsSep = pRegion->FirstChildElement("thousandsseparator");
      if (pThousandsSep)
      {
        if (!pThousandsSep->NoChildren())
        {
          region.m_cThousandsSep = pThousandsSep->FirstChild()->Value()[0];
          if (pThousandsSep->Attribute("groupingformat"))
            region.m_strGrouping = StringUtils::BinaryStringToString(pThousandsSep->Attribute("groupingformat"));
          else
            region.m_strGrouping = "\3";
        }
      }
      else
      {
        region.m_cThousandsSep = ',';
        region.m_strGrouping = "\3";
      }

      const TiXmlElement *pDecimalSep = pRegion->FirstChildElement("decimalseparator");
      if (pDecimalSep)
      {
        if (!pDecimalSep->NoChildren())
          region.m_cDecimalSep = pDecimalSep->FirstChild()->Value()[0];
      }
      else
        region.m_cDecimalSep = '.';

      m_regions.insert(PAIR_REGIONS(region.m_strName, region));

      pRegion=pRegion->NextSiblingElement("region");
    }

    const std::string& strName = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_COUNTRY);
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

bool CLangInfo::UseLocaleCollation()
{
  if (m_collationtype == 0)
  {
    // Determine collation to use. When using MySQL/MariaDB or a platform that does not support
    // locale language collation then use accent folding internal equivalent of utf8_general_ci
    m_collationtype = 1;
    if (!StringUtils::EqualsNoCase(
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseMusic.type,
            "mysql") &&
        !StringUtils::EqualsNoCase(
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseVideo.type,
            "mysql") &&
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_useLocaleCollation)
    {
      // Check that locale collation facet is implemented on the platform
      const std::collate<wchar_t>& coll = std::use_facet<std::collate<wchar_t>>(m_systemLocale);
      wchar_t lc = L'z';
      wchar_t rc = 0x00E2; // Latin small letter a with circumflex
      int comp_result = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
      if (comp_result > 0)
        // Latin small letter a with circumflex put before z - collation works
        m_collationtype = 2;
    }
  }
  return m_collationtype == 2;
}

void CLangInfo::LoadTokens(const TiXmlNode* pTokens, std::set<std::string>& vecTokens)
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
  std::shared_ptr<CSettingString> charsetSetting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_LOCALE_CHARSET));
  if (charsetSetting == NULL || charsetSetting->IsDefault())
    return m_strGuiCharSet;

  return charsetSetting->GetValue();
}

std::string CLangInfo::GetSubtitleCharSet() const
{
  std::shared_ptr<CSettingString> charsetSetting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_SUBTITLES_CHARSET));
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
    addonId = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE);

  ADDON::AddonPtr addon;
  if (CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, ADDON::ADDON_RESOURCE_LANGUAGE, true) && addon != NULL)
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

bool CLangInfo::SetLanguage(std::string language /* = "" */, bool reloadServices /* = true */)
{
  if (language.empty())
    language = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE);

  auto& addonMgr = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;

  // Find the chosen language add-on if it's enabled
  if (!addonMgr.GetAddon(language, addon, ADDON::ADDON_RESOURCE_LANGUAGE, true))
  {
    if (addonMgr.IsAddonDisabled(addon->ID()) && !addonMgr.EnableAddon(addon->ID()))
    {
      CLog::Log(LOGWARNING,
                "CLangInfo::{}: could not find or enable language add-on '{}', loading default...",
                __func__, language);
      language = std::static_pointer_cast<const CSettingString>(
                     CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
                         CSettings::SETTING_LOCALE_LANGUAGE))
                     ->GetDefault();

      if (!addonMgr.GetAddon(language, addon, ADDON::ADDON_RESOURCE_LANGUAGE, false))
      {
        CLog::Log(LOGFATAL, "CLangInfo::{}: could not find default language add-on '{}'", __func__,
                  language);
        return false;
      }
    }
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

  ADDON::VECADDONS addons;
  if (CServiceBroker::GetAddonMgr().GetInstalledAddons(addons))
  {
    const std::string locale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE);
    for (const auto& addon : addons)
    {
      const std::string path = URIUtils::AddFileToFolder(addon->Path(), "resources", "language/");
      g_localizeStrings.LoadAddonStrings(path, locale, addon->ID());
    }
  }

  if (reloadServices)
  {
    // also tell our weather and skin to reload as these are localized
    CServiceBroker::GetWeatherManager().Refresh();
    CServiceBroker::GetPVRManager().LocalizationChanged();
    CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "ReloadSkin");
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
    || StringUtils::EqualsNoCase(language, "mediadefault")
    || !g_LangCodeExpander.ConvertToISO6392B(language, m_audioLanguage))
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
    || !g_LangCodeExpander.ConvertToISO6392B(language, m_subtitleLanguage))
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

const std::locale& CLangInfo::GetOriginalLocale() const
{
  return m_originalLocale;
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
void CLangInfo::GetRegionNames(std::vector<std::string>& array)
{
  for (const auto &region : m_regions)
  {
    std::string strName=region.first;
    if (strName=="N/A")
      strName=g_localizeStrings.Get(10005); // Not available
    array.emplace_back(std::move(strName));
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

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetString(CSettings::SETTING_LOCALE_SHORTDATEFORMAT) == SETTING_REGIONAL_DEFAULT)
    SetShortDateFormat(m_currentRegion->m_strDateFormatShort);
  if (settings->GetString(CSettings::SETTING_LOCALE_LONGDATEFORMAT) == SETTING_REGIONAL_DEFAULT)
    SetLongDateFormat(m_currentRegion->m_strDateFormatLong);
  if (settings->GetString(CSettings::SETTING_LOCALE_USE24HOURCLOCK) == SETTING_REGIONAL_DEFAULT)
  {
    Set24HourClock(m_currentRegion->m_strTimeFormat);

    // update the time format
    SetTimeFormat(settings->GetString(CSettings::SETTING_LOCALE_TIMEFORMAT));
  }
  if (settings->GetString(CSettings::SETTING_LOCALE_TIMEFORMAT) == SETTING_REGIONAL_DEFAULT)
    SetTimeFormat(m_currentRegion->m_strTimeFormat);
  if (settings->GetString(CSettings::SETTING_LOCALE_TEMPERATUREUNIT) == SETTING_REGIONAL_DEFAULT)
    SetTemperatureUnit(m_currentRegion->m_tempUnit);
  if (settings->GetString(CSettings::SETTING_LOCALE_SPEEDUNIT) == SETTING_REGIONAL_DEFAULT)
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

  // refresh weather manager as temperatures need re-translating
  // NOTE: this could be called before our service manager is up
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetWeatherManager().Refresh();
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

  // refresh weather manager as speeds need re-translating
  // NOTE: this could be called before our service manager is up
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetWeatherManager().Refresh();
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
  for (const auto& t : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_vecTokens)
    sortTokens.insert(t);

  return sortTokens;
}

bool CLangInfo::DetermineUse24HourClockFromTimeFormat(const std::string& timeFormat)
{
  // if the time format contains a "h" it's 12-hour and otherwise 24-hour clock format
  return timeFormat.find("h") == std::string::npos;
}

bool CLangInfo::DetermineUseMeridiemFromTimeFormat(const std::string& timeFormat)
{
  // if the time format contains "xx" it's using meridiem
  return timeFormat.find("xx") != std::string::npos;
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

void CLangInfo::SettingOptionsLanguageNamesFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  // find languages...
  ADDON::VECADDONS addons;
  if (!CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON::ADDON_RESOURCE_LANGUAGE))
    return;

  for (const auto &addon : addons)
    list.emplace_back(addon->Name(), addon->Name());

  sort(list.begin(), list.end(), SortLanguage());
}

void CLangInfo::SettingOptionsISO6391LanguagesFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  // get a list of language names
  std::vector<std::string> languages = g_LangCodeExpander.GetLanguageNames(CLangCodeExpander::ISO_639_1, true);
  sort(languages.begin(), languages.end(), sortstringbyname());
  for (const auto &language : languages)
    list.emplace_back(language, language);
}

void CLangInfo::SettingOptionsAudioStreamLanguagesFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  list.emplace_back(g_localizeStrings.Get(307), "mediadefault");
  list.emplace_back(g_localizeStrings.Get(308), "original");
  list.emplace_back(g_localizeStrings.Get(309), "default");

  AddLanguages(list);
}

void CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  list.emplace_back(g_localizeStrings.Get(231), "none");
  list.emplace_back(g_localizeStrings.Get(13207), "forced_only");
  list.emplace_back(g_localizeStrings.Get(308), "original");
  list.emplace_back(g_localizeStrings.Get(309), "default");

  AddLanguages(list);
}

void CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  list.emplace_back(g_localizeStrings.Get(308), "original");
  list.emplace_back(g_localizeStrings.Get(309), "default");

  AddLanguages(list);
}

void CLangInfo::SettingOptionsRegionsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  std::vector<std::string> regions;
  g_langInfo.GetRegionNames(regions);
  std::sort(regions.begin(), regions.end(), sortstringbyname());

  bool match = false;
  for (unsigned int i = 0; i < regions.size(); ++i)
  {
    std::string region = regions[i];
    list.emplace_back(region, region);

    if (!match && region == std::static_pointer_cast<const CSettingString>(setting)->GetValue())
    {
      match = true;
      current = region;
    }
  }

  if (!match && !regions.empty())
    current = regions[0];
}

void CLangInfo::SettingOptionsShortDateFormatsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& shortDateFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();

  list.emplace_back(
    StringUtils::Format(g_localizeStrings.Get(20035).c_str(),
                        now.GetAsLocalizedDate(g_langInfo.m_currentRegion->m_strDateFormatShort).c_str()),
    SETTING_REGIONAL_DEFAULT);
  if (shortDateFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (const std::string& shortDateFormat : shortDateFormats)
  {
    list.emplace_back(now.GetAsLocalizedDate(shortDateFormat), shortDateFormat);

    if (!match && shortDateFormatSetting == shortDateFormat)
    {
      match = true;
      current = shortDateFormat;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptionsLongDateFormatsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& longDateFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();

  list.emplace_back(
    StringUtils::Format(g_localizeStrings.Get(20035).c_str(),
                        now.GetAsLocalizedDate(g_langInfo.m_currentRegion->m_strDateFormatLong).c_str()),
    SETTING_REGIONAL_DEFAULT);
  if (longDateFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (const std::string& longDateFormat : longDateFormats)
  {
    list.emplace_back(now.GetAsLocalizedDate(longDateFormat), longDateFormat);

    if (!match && longDateFormatSetting == longDateFormat)
    {
      match = true;
      current = longDateFormat;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptionsTimeFormatsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& timeFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();
  bool use24hourFormat = g_langInfo.Use24HourClock();

  list.emplace_back(
    StringUtils::Format(g_localizeStrings.Get(20035).c_str(),
                        ToSettingTimeFormat(now, g_langInfo.m_currentRegion->m_strTimeFormat).c_str()),
    SETTING_REGIONAL_DEFAULT);
  if (timeFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  if (use24hourFormat)
  {
    list.emplace_back(ToSettingTimeFormat(now, TIME_FORMAT_SINGLE_24), TIME_FORMAT_SINGLE_24);
    if (timeFormatSetting == TIME_FORMAT_SINGLE_24)
    {
      current = TIME_FORMAT_SINGLE_24;
      match = true;
    }

    list.emplace_back(ToSettingTimeFormat(now, TIME_FORMAT_DOUBLE_24), TIME_FORMAT_DOUBLE_24);
    if (timeFormatSetting == TIME_FORMAT_DOUBLE_24)
    {
      current = TIME_FORMAT_DOUBLE_24;
      match = true;
    }
  }
  else
  {
    list.emplace_back(ToSettingTimeFormat(now, TIME_FORMAT_SINGLE_12), TIME_FORMAT_SINGLE_12);
    if (timeFormatSetting == TIME_FORMAT_SINGLE_12)
    {
      current = TIME_FORMAT_SINGLE_12;
      match = true;
    }

    list.emplace_back(ToSettingTimeFormat(now, TIME_FORMAT_DOUBLE_12), TIME_FORMAT_DOUBLE_12);
    if (timeFormatSetting == TIME_FORMAT_DOUBLE_12)
    {
      current = TIME_FORMAT_DOUBLE_12;
      match = true;
    }

    std::string timeFormatSingle12Meridiem = ToTimeFormat(false, true, true);
    list.emplace_back(ToSettingTimeFormat(now, timeFormatSingle12Meridiem), timeFormatSingle12Meridiem);
    if (timeFormatSetting == timeFormatSingle12Meridiem)
    {
      current = timeFormatSingle12Meridiem;
      match = true;
    }

    std::string timeFormatDouble12Meridiem = ToTimeFormat(false, false, true);
    list.emplace_back(ToSettingTimeFormat(now, timeFormatDouble12Meridiem), timeFormatDouble12Meridiem);
    if (timeFormatSetting == timeFormatDouble12Meridiem)
    {
      current = timeFormatDouble12Meridiem;
      match = true;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptions24HourClockFormatsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& clock24HourFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  // determine the 24-hour clock format of the regional setting
  int regionalClock24HourFormatLabel = DetermineUse24HourClockFromTimeFormat(g_langInfo.m_currentRegion->m_strTimeFormat) ? 12384 : 12383;
  list.emplace_back(StringUtils::Format(g_localizeStrings.Get(20035).c_str(),
                                        g_localizeStrings.Get(regionalClock24HourFormatLabel).c_str()),
                    SETTING_REGIONAL_DEFAULT);
  if (clock24HourFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  list.emplace_back(g_localizeStrings.Get(12383), TIME_FORMAT_12HOURS);
  if (clock24HourFormatSetting == TIME_FORMAT_12HOURS)
  {
    current = TIME_FORMAT_12HOURS;
    match = true;
  }

  list.emplace_back(g_localizeStrings.Get(12384), TIME_FORMAT_24HOURS);
  if (clock24HourFormatSetting == TIME_FORMAT_24HOURS)
  {
    current = TIME_FORMAT_24HOURS;
    match = true;
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptionsTemperatureUnitsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& temperatureUnitSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  list.emplace_back(StringUtils::Format(g_localizeStrings.Get(20035).c_str(),
                                        GetTemperatureUnitString(g_langInfo.m_currentRegion->m_tempUnit).c_str()),
                    SETTING_REGIONAL_DEFAULT);
  if (temperatureUnitSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (const TemperatureInfo& info : temperatureInfo)
  {
    list.emplace_back(GetTemperatureUnitString(info.unit), info.name);

    if (!match && temperatureUnitSetting == info.name)
    {
      match = true;
      current = info.name;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptionsSpeedUnitsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  bool match = false;
  const std::string& speedUnitSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  list.emplace_back(StringUtils::Format(g_localizeStrings.Get(20035).c_str(),
                                        GetSpeedUnitString(g_langInfo.m_currentRegion->m_speedUnit).c_str()),
                    SETTING_REGIONAL_DEFAULT);
  if (speedUnitSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (const SpeedInfo& info : speedInfo)
  {
    list.emplace_back(GetSpeedUnitString(info.unit), info.name);

    if (!match && speedUnitSetting == info.name)
    {
      match = true;
      current = info.name;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::AddLanguages(std::vector<StringSettingOption> &list)
{
  std::string dummy;
  std::vector<StringSettingOption> languages;
  SettingOptionsISO6391LanguagesFiller(NULL, languages, dummy, NULL);
  SettingOptionsLanguageNamesFiller(NULL, languages, dummy, NULL);

  // convert the vector to a set to remove duplicates
  std::set<StringSettingOption, SortLanguage> tmp(
    languages.begin(), languages.end(), SortLanguage());

  list.reserve(list.size() + tmp.size());
  list.insert(list.end(), tmp.begin(), tmp.end());
}
