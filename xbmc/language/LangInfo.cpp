/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LangInfo.h"

#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "weather/WeatherManager.h"

#include <algorithm>
#include <array>
#include <span>

namespace
{
std::string GetDateStringWithFormat(const CDateTime& date, const std::string& format)
{
  // Return the formatted date together with the format used.
  // eg: '1/02/2003 (D/MM/YYYY)'
  return date.GetAsLocalizedDate(format) + " (" + format + ")";
}
} // namespace

namespace KODI::LANGUAGE
{

static std::string shortDateFormats[] = {
    // clang-format off
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
  "YYYY.MM.DD",
  // short date formats with abbreviated month and 2 digit year
  "D-mmm-YY",
  "DD-mmm-YY",
  "D mmm YY",
  "DD mmm YY",
    // clang-format on
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

struct TemperatureInfo
{
  CTemperature::Unit unit;
  std::string name;
};

static const auto temperatureInfo = std::array{TemperatureInfo{CTemperature::UnitFahrenheit, "f"},
                                               TemperatureInfo{CTemperature::UnitKelvin, "k"},
                                               TemperatureInfo{CTemperature::UnitCelsius, "c"},
                                               TemperatureInfo{CTemperature::UnitReaumur, "re"},
                                               TemperatureInfo{CTemperature::UnitRankine, "ra"},
                                               TemperatureInfo{CTemperature::UnitRomer, "ro"},
                                               TemperatureInfo{CTemperature::UnitDelisle, "de"},
                                               TemperatureInfo{CTemperature::UnitNewton, "n"}};

#define TEMP_UNIT_STRINGS         20027

struct SpeedInfo
{
  CSpeed::Unit unit;
  std::string name;
};

static const auto speedInfo = std::array{
    SpeedInfo{CSpeed::UnitKilometresPerHour, "kmh"},
    SpeedInfo{CSpeed::UnitMetresPerMinute, "mpmin"},
    SpeedInfo{CSpeed::UnitMetresPerSecond, "mps"},
    SpeedInfo{CSpeed::UnitFeetPerHour, "fth"},
    SpeedInfo{CSpeed::UnitFeetPerMinute, "ftm"},
    SpeedInfo{CSpeed::UnitFeetPerSecond, "fts"},
    SpeedInfo{CSpeed::UnitMilesPerHour, "mph"},
    SpeedInfo{CSpeed::UnitKnots, "kts"},
    SpeedInfo{CSpeed::UnitBeaufort, "beaufort"},
    SpeedInfo{CSpeed::UnitInchPerSecond, "inchs"},
    SpeedInfo{CSpeed::UnitYardPerSecond, "yards"},
    SpeedInfo{CSpeed::UnitFurlongPerFortnight, "fpf"},
};

#define SPEED_UNIT_STRINGS        20200

#define SETTING_REGIONAL_DEFAULT  "regional"

static std::string ToTimeFormat(bool use24HourClock, bool singleHour, bool meridiem)
{
  if (use24HourClock)
    return singleHour ? TIME_FORMAT_SINGLE_24 : TIME_FORMAT_DOUBLE_24;

  if (!meridiem)
    return singleHour ? TIME_FORMAT_SINGLE_12 : TIME_FORMAT_DOUBLE_12;

  return StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(12382),
      ToTimeFormat(false, singleHour, false));
}

static std::string ToSettingTimeFormat(const CDateTime& time, const std::string& timeFormat)
{
  return StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20036),
      time.GetAsLocalizedTime(timeFormat, true), timeFormat);
}

static CTemperature::Unit StringToTemperatureUnit(const std::string& temperatureUnit)
{
  std::string unit(temperatureUnit);
  StringUtils::ToLower(unit);

  const auto it = std::ranges::find_if(temperatureInfo,
                                       [&unit](const auto& info) { return info.name == unit; });
  return it != temperatureInfo.end() ? it->unit : CTemperature::UnitCelsius;
}

static CSpeed::Unit StringToSpeedUnit(const std::string& speedUnit)
{
  std::string unit(speedUnit);
  StringUtils::ToLower(unit);

  const auto it =
      std::ranges::find_if(speedInfo, [&unit](const auto& info) { return info.name == unit; });
  return it != speedInfo.end() ? it->unit : CSpeed::UnitKilometresPerHour;
}

CLangInfo::CRegion::CRegion()
{
  SetDefaults();
}

void CLangInfo::CRegion::SetDefaults()
{
  m_strName="N/A";

  m_strDateFormatShort="DD/MM/YYYY";
  m_strDateFormatLong="DDDD, D MMMM YYYY";
  m_strTimeFormat="HH:mm:ss";
  m_tempUnit = CTemperature::UnitCelsius;
  m_speedUnit = CSpeed::UnitKilometresPerHour;
}

void CLangInfo::CRegion::SetTemperatureUnit(const std::string& strUnit)
{
  m_tempUnit = StringToTemperatureUnit(strUnit);
}

void CLangInfo::CRegion::SetSpeedUnit(const std::string& strUnit)
{
  m_speedUnit = StringToSpeedUnit(strUnit);
}

void CLangInfo::CRegion::SetGlobalLocale(CLangInfo& langInfo)
{
  // A platform locale pairs the interface language with the place the selected region is for,
  // which are two separate choices - a British pack with the Australian region is en_AU. The
  // language is the interface's, so it comes from CLanguage rather than being copied onto every
  // region; the territory is this region's.
  const std::string language{CLanguage::GetInstance().UI().AsIso6391()};
  const std::string territory{m_territory.AsIso3166_1Alpha2()};

  // The name a platform's locale database answers to
#ifdef TARGET_WINDOWS
  static constexpr std::string_view separator{"-"};
#else
  static constexpr std::string_view separator{"_"};
#endif
  std::string strLocale{territory.empty() ? language
                                          : language + std::string{separator} + territory};
#ifdef TARGET_POSIX
  if (!strLocale.empty())
    strLocale += ".UTF-8";
#endif
  langInfo.m_originalLocale = std::locale(
      std::locale::classic(), new custom_numpunct(m_cDecimalSep, m_cThousandsSep, m_strGrouping));

  CLog::Log(LOGDEBUG, "trying to set locale to {}", strLocale);

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

  langInfo.m_systemLocale = current_locale; //! @todo move to CLangInfo class
  langInfo.m_collationtype = 0;
  std::locale::global(current_locale);
#endif

#ifndef TARGET_WINDOWS
  if (setlocale(LC_COLLATE, strLocale.c_str()) == nullptr ||
      setlocale(LC_CTYPE, strLocale.c_str()) == nullptr ||
      setlocale(LC_TIME, strLocale.c_str()) == nullptr)
  {
    strLocale = "C";
    setlocale(LC_COLLATE, strLocale.c_str());
    setlocale(LC_CTYPE, strLocale.c_str());
    setlocale(LC_TIME, strLocale.c_str());
  }
#else
  std::wstring strLocaleW;
  g_charsetConverter.utf8ToW(strLocale, strLocaleW);
  if (_wsetlocale(LC_COLLATE, strLocaleW.c_str()) == nullptr ||
      _wsetlocale(LC_CTYPE, strLocaleW.c_str()) == nullptr ||
      _wsetlocale(LC_TIME, strLocaleW.c_str()) == nullptr)
  {
    strLocale = "C";
    strLocaleW = L"C";
    _wsetlocale(LC_COLLATE, strLocaleW.c_str());
    _wsetlocale(LC_CTYPE, strLocaleW.c_str());
    _wsetlocale(LC_TIME, strLocaleW.c_str());
  }
#endif

  g_charsetConverter.resetSystemCharset();
  CLog::Log(LOGINFO, "global locale set to {}", strLocale);

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

void CLangInfo::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_COUNTRY)
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
    settings->SetString(CSettings::SETTING_LOCALE_TIMEFORMAT,
                        PrepareTimeFormat(GetTimeFormat(), m_use24HourClock));
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

bool CLangInfo::Load(const std::string& langInfoPath)
{
  SetDefaults();

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(langInfoPath))
  {
    CLog::Log(LOGERROR, "unable to load {}: {} at line {}", langInfoPath, xmlDoc.ErrorStr(),
              xmlDoc.ErrorLineNum());
    return false;
  }

  const auto* pRootElement = xmlDoc.RootElement();
  if (std::string(pRootElement->Value()) != "language")
  {
    CLog::Log(LOGERROR, "{} Doesn't contain <language>", langInfoPath);
    return false;
  }

  // The root element's locale attribute states the language, which the pack already states as
  // its own locale and CLanguage holds. It is the language half of a region name, not a value
  // of its own, so nothing reads it here.

  const auto* pRegions = pRootElement->FirstChildElement("regions");
  if (pRegions && !pRegions->NoChildren())
  {
    const auto* pRegion = pRegions->FirstChildElement("region");
    while (pRegion)
    {
      CRegion region(m_defaultRegion);
      region.m_strName = XMLUtils::GetAttribute(pRegion, "name");
      if (region.m_strName.empty())
        region.m_strName = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
            10005); // Not available

      // The one place a region's place arrives as text, so the one place it is judged. A
      // langinfo.xml stating no place, or one naming nothing, leaves the region without one.
      if (pRegion->Attribute("locale"))
        region.m_territory = CTerritory::FromCode(pRegion->Attribute("locale"));

      const auto* pDateLong = pRegion->FirstChildElement("datelong");
      if (pDateLong && !pDateLong->NoChildren())
        region.m_strDateFormatLong = pDateLong->FirstChild()->Value();

      const auto* pDateShort = pRegion->FirstChildElement("dateshort");
      if (pDateShort && !pDateShort->NoChildren())
        region.m_strDateFormatShort = pDateShort->FirstChild()->Value();

      const auto* pTime = pRegion->FirstChildElement("time");
      if (pTime && !pTime->NoChildren())
      {
        region.m_strTimeFormat=pTime->FirstChild()->Value();
        region.m_strMeridiemSymbols[static_cast<int>(MeridiemSymbol::AM)] =
            XMLUtils::GetAttribute(pTime, "symbolAM");
        region.m_strMeridiemSymbols[static_cast<int>(MeridiemSymbol::PM)] =
            XMLUtils::GetAttribute(pTime, "symbolPM");
      }

      const auto* pTempUnit = pRegion->FirstChildElement("tempunit");
      if (pTempUnit && !pTempUnit->NoChildren())
        region.SetTemperatureUnit(pTempUnit->FirstChild()->Value());

      const auto* pSpeedUnit = pRegion->FirstChildElement("speedunit");
      if (pSpeedUnit && !pSpeedUnit->NoChildren())
        region.SetSpeedUnit(pSpeedUnit->FirstChild()->Value());

      const auto* pThousandsSep = pRegion->FirstChildElement("thousandsseparator");
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

      const auto* pDecimalSep = pRegion->FirstChildElement("decimalseparator");
      if (pDecimalSep)
      {
        if (!pDecimalSep->NoChildren())
          region.m_cDecimalSep = pDecimalSep->FirstChild()->Value()[0];
      }
      else
        region.m_cDecimalSep = '.';

      m_regions.insert(PAIR_REGIONS(region.m_strName, region));

      pRegion = pRegion->NextSiblingElement("region");
    }

    const std::string& strName = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_COUNTRY);
    SetCurrentRegion(strName);
  }

  return true;
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

void CLangInfo::SetDefaults()
{
  m_regions.clear();

  //Reset default region
  m_defaultRegion.SetDefaults();

  // Set the default region, we may be unable to load langinfo.xml
  m_currentRegion = &m_defaultRegion;

  m_systemLocale = std::locale::classic();
}

namespace
{
/*!
 * \brief A display name without the qualifier it carries in parentheses - a region profile named
 *        "USA (12h)" becomes "USA".
 * \note Text handling on text meant for a reader, which is all it is fit for. A language is never
 *       taken apart this way to find out what it is - CLanguageTag says that.
 */
std::string WithoutQualifier(const std::string& name)
{
  const size_t openParen = name.find('(');
  if (openParen == std::string::npos)
    return name;

  std::string base = name.substr(0, openParen);
  StringUtils::TrimRight(base);
  return base;
}
} // namespace

std::string CLangInfo::GetLanguageAs(CLanguageTag::Notation format, bool withRegion) const
{
  const CLanguageTag& ui{CLanguage::GetInstance().UI()};

  std::string language;
  switch (format)
  {
    case CLanguageTag::ENGLISH_NAME:
      // The name the pack declares for itself, which is the one a user has seen named
      language = CLanguage::GetInstance().PackName();
      break;
    case CLanguageTag::ISO_NAME:
      language = ui.ToEnglishLanguageName();
      break;
    case CLanguageTag::ISO_639_1:
      language = ui.AsIso6391();
      break;
    case CLanguageTag::ISO_639_2:
      language = ui.AsIso6392B();
      break;
  }

  // The region, named in the notation the language format implies
  const auto regionInFormat = [this](CLanguageTag::Notation fmt)
  {
    switch (fmt)
    {
      case CLanguageTag::ISO_639_1:
        return GetRegionTerritory().AsIso3166_1Alpha2();
      case CLanguageTag::ISO_639_2:
        return GetRegionTerritory().AsIso3166_1Alpha3();
      case CLanguageTag::ISO_NAME:
        return WithoutQualifier(GetCurrentRegion());
      default:
        return GetCurrentRegion();
    }
  };

  // Some languages have no code in the requested ISO 639 format - Asturian, for one, has no
  // ISO 639-1 code - and the conversion then yields nothing to join a region to
  if (withRegion && !language.empty())
  {
    if (const std::string regionCode{regionInFormat(format)}; !regionCode.empty())
      language += "-" + regionCode;
  }

  return language;
}

const CTerritory& CLangInfo::GetRegionTerritory() const
{
  return m_currentRegion->m_territory;
}

const std::locale& CLangInfo::GetOriginalLocale() const
{
  return m_originalLocale;
}

// Returns the format string for the date of the current language
const std::string& CLangInfo::GetDateFormat(bool bLongDate /* = false */) const
{
  return bLongDate ? GetLongDateFormat() : GetShortDateFormat();
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
    case MeridiemSymbol::AM:
      return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(378);

    case MeridiemSymbol::PM:
      return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(379);

    default:
      break;
  }

  return StringUtils::Empty;
}

// Fills the array with the region names available for this language
void CLangInfo::GetRegionNames(std::vector<std::string>& array) const
{
  std::ranges::transform(m_regions, std::back_inserter(array),
                         [&rc = CServiceBroker::GetResourcesComponent()](const auto& region)
                         {
                           return region.first == "N/A"
                                      ? rc.GetLocalizeStrings().Get(10005) // Not available
                                      : region.first;
                         });
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

  m_currentRegion->SetGlobalLocale(*this);

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
  SetTemperatureUnit(temperatureUnit == SETTING_REGIONAL_DEFAULT
                         ? m_currentRegion->m_tempUnit
                         : StringToTemperatureUnit(temperatureUnit));
}

std::string CLangInfo::GetTemperatureAsString(const CTemperature& temperature) const
{
  if (!temperature.IsValid())
    return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13205); // "Unknown"

  CTemperature::Unit temperatureUnit = GetTemperatureUnit();
  return StringUtils::Format("{}{}", temperature.ToString(temperatureUnit),
                             GetTemperatureUnitString());
}

// Returns the temperature unit string for the current language
const std::string& CLangInfo::GetTemperatureUnitString() const
{
  return GetTemperatureUnitString(m_temperatureUnit);
}

const std::string& CLangInfo::GetTemperatureUnitString(CTemperature::Unit temperatureUnit)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(TEMP_UNIT_STRINGS +
                                                                          temperatureUnit);
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
  SetSpeedUnit(speedUnit == SETTING_REGIONAL_DEFAULT ? m_currentRegion->m_speedUnit
                                                     : StringToSpeedUnit(speedUnit));
}

CSpeed::Unit CLangInfo::GetSpeedUnit() const
{
  return m_speedUnit;
}

// Returns the speed unit string for the current language
const std::string& CLangInfo::GetSpeedUnitString() const
{
  return GetSpeedUnitString(m_speedUnit);
}

const std::string& CLangInfo::GetSpeedUnitString(CSpeed::Unit speedUnit)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(SPEED_UNIT_STRINGS +
                                                                          speedUnit);
}

bool CLangInfo::DetermineUse24HourClockFromTimeFormat(const std::string& timeFormat)
{
  // if the time format contains a "h" it's 12-hour and otherwise 24-hour clock format
  return timeFormat.find('h') == std::string::npos;
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

void CLangInfo::SettingOptionsRegionsFiller(const SettingConstPtr& setting,
                                            std::vector<StringSettingOption>& list,
                                            std::string& current,
                                            const CLangInfo& langInfo)
{
  std::vector<std::string> regions;
  langInfo.GetRegionNames(regions);
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

void CLangInfo::SettingOptionsShortDateFormatsFiller(const SettingConstPtr& setting,
                                                     std::vector<StringSettingOption>& list,
                                                     std::string& current,
                                                     const CLangInfo& langInfo)
{
  bool match = false;
  const std::string& shortDateFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();

  list.emplace_back(
      StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20035),
          GetDateStringWithFormat(now, langInfo.m_currentRegion->m_strDateFormatShort)),
      SETTING_REGIONAL_DEFAULT);

  if (shortDateFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (const std::string& shortDateFormat : shortDateFormats)
  {
    list.emplace_back(GetDateStringWithFormat(now, shortDateFormat), shortDateFormat);

    if (!match && shortDateFormatSetting == shortDateFormat)
    {
      match = true;
      current = shortDateFormat;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptionsLongDateFormatsFiller(const SettingConstPtr& setting,
                                                    std::vector<StringSettingOption>& list,
                                                    std::string& current,
                                                    const CLangInfo& langInfo)
{
  bool match = false;
  const std::string& longDateFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();

  list.emplace_back(
      StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20035),
          GetDateStringWithFormat(now, langInfo.m_currentRegion->m_strDateFormatLong)),
      SETTING_REGIONAL_DEFAULT);

  if (longDateFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  for (const std::string& longDateFormat : longDateFormats)
  {
    list.emplace_back(GetDateStringWithFormat(now, longDateFormat), longDateFormat);

    if (!match && longDateFormatSetting == longDateFormat)
    {
      match = true;
      current = longDateFormat;
    }
  }

  if (!match && !list.empty())
    current = list[0].value;
}

void CLangInfo::SettingOptionsTimeFormatsFiller(const SettingConstPtr& setting,
                                                std::vector<StringSettingOption>& list,
                                                std::string& current,
                                                const CLangInfo& langInfo)
{
  bool match = false;
  const std::string& timeFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  CDateTime now = CDateTime::GetCurrentDateTime();
  bool use24hourFormat = langInfo.Use24HourClock();

  list.emplace_back(
      StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20035),
                          ToSettingTimeFormat(now, langInfo.m_currentRegion->m_strTimeFormat)),
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

void CLangInfo::SettingOptions24HourClockFormatsFiller(const SettingConstPtr& setting,
                                                       std::vector<StringSettingOption>& list,
                                                       std::string& current,
                                                       const CLangInfo& langInfo)
{
  bool match = false;
  const std::string& clock24HourFormatSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  // determine the 24-hour clock format of the regional setting
  int regionalClock24HourFormatLabel =
      DetermineUse24HourClockFromTimeFormat(langInfo.m_currentRegion->m_strTimeFormat) ? 12384
                                                                                       : 12383;
  list.emplace_back(
      StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20035),
                          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                              regionalClock24HourFormatLabel)),
      SETTING_REGIONAL_DEFAULT);
  if (clock24HourFormatSetting == SETTING_REGIONAL_DEFAULT)
  {
    match = true;
    current = SETTING_REGIONAL_DEFAULT;
  }

  list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(12383),
                    TIME_FORMAT_12HOURS);
  if (clock24HourFormatSetting == TIME_FORMAT_12HOURS)
  {
    current = TIME_FORMAT_12HOURS;
    match = true;
  }

  list.emplace_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(12384),
                    TIME_FORMAT_24HOURS);
  if (clock24HourFormatSetting == TIME_FORMAT_24HOURS)
  {
    current = TIME_FORMAT_24HOURS;
    match = true;
  }

  if (!match)
    current = list[0].value;
}

void CLangInfo::SettingOptionsTemperatureUnitsFiller(const SettingConstPtr& setting,
                                                     std::vector<StringSettingOption>& list,
                                                     std::string& current,
                                                     const CLangInfo& langInfo)
{
  bool match = false;
  const std::string& temperatureUnitSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  list.emplace_back(
      StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20035),
                          GetTemperatureUnitString(langInfo.m_currentRegion->m_tempUnit)),
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

void CLangInfo::SettingOptionsSpeedUnitsFiller(const SettingConstPtr& setting,
                                               std::vector<StringSettingOption>& list,
                                               std::string& current,
                                               const CLangInfo& langInfo)
{
  bool match = false;
  const std::string& speedUnitSetting = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

  list.emplace_back(
      StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20035),
                          GetSpeedUnitString(langInfo.m_currentRegion->m_speedUnit)),
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
} // namespace KODI::LANGUAGE
