/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifdef TARGET_ANDROID
#include "platform/android/bionic_supplement/bionic_supplement.h"
#endif
#include "PlatformDefs.h"
#include "PosixTimezone.h"
#include "utils/SystemInfo.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "XBDateTime.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include <stdlib.h>

#define USE_OS_TZDB 0
#define HAS_REMOTE_API 0
#include <date/tz.h>

#include "platform/MessagePrinter.h"

#include "filesystem/File.h"

void CPosixTimezone::Init()
{
  XFILE::CFileStream zonetab;

  if(!zonetab.Open("special://xbmc/addons/resource.timezone/resources/tzdata/zone.tab"))
  {
    CMessagePrinter::DisplayMessage("failed to open zone.tab");
    return;
  }

  std::string countryCode;
  std::string timezoneName;

  std::vector<std::string> tokens;

  for (std::string s; std::getline(zonetab, s);)
  {
    tokens.clear();
    std::string line = s;
    StringUtils::Trim(line);

    if (line.length() == 0)
      continue;

    if (line[0] == '#')
      continue;

    StringUtils::Tokenize(line, tokens, " \t");
    if (tokens.size() < 3)
      continue;

    countryCode = tokens[0];
    timezoneName = tokens[2];

    if (m_timezonesByCountryCode.count(countryCode) == 0)
    {
      std::vector<std::string> timezones;
      timezones.push_back(timezoneName);
      m_timezonesByCountryCode[countryCode] = timezones;
    }
    else
    {
      std::vector<std::string>& timezones = m_timezonesByCountryCode[countryCode];
      timezones.push_back(timezoneName);
    }

    m_countriesByTimezoneName[timezoneName] = countryCode;
  }

  XFILE::CFileStream isotab;

  if (!isotab.Open("special://xbmc/addons/resource.timezone/resources/tzdata/iso3166.tab"))
  {
    CMessagePrinter::DisplayMessage("failed to open iso3166.tab");
    return;
  }

  std::string countryName;

  for (std::string s; std::getline(isotab, s);)
  {
    tokens.clear();
    std::string line = s;
    StringUtils::Trim(line);

    if (line.length() == 0)
      continue;

    if (line[0] == '#')
      continue;

    StringUtils::Tokenize(line, tokens, "\t");
    if (tokens.size() < 2)
      continue;

    countryCode = tokens[0];
    countryName = tokens[1];

    m_countries.push_back(countryName);
    m_countryByCode[countryCode] = countryName;
    m_countryByName[countryName] = countryCode;
  }

  sort(m_countries.begin(), m_countries.end(), sortstringbyname());
}

void CPosixTimezone::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_TIMEZONE)
  {
    SetTimezone(std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  }
  else if (settingId == CSettings::SETTING_LOCALE_TIMEZONECOUNTRY)
  {
    // nothing to do here. Changing locale.timezonecountry will trigger an
    // update of locale.timezone and automatically adjust its value
    // and execute OnSettingChanged() for it as well (see above)
  }
}

void CPosixTimezone::OnSettingsLoaded()
{
  SetTimezone(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_TIMEZONE));
}

std::vector<std::string> CPosixTimezone::GetCountries()
{
  return m_countries;
}

std::vector<std::string> CPosixTimezone::GetTimezonesByCountry(const std::string& country)
{
  return m_timezonesByCountryCode[m_countryByName[country]];
}

std::string CPosixTimezone::GetCountryByTimezone(const std::string& timezone)
{
#if defined(TARGET_DARWIN)
  return "?";
#endif

  return m_countryByCode[m_countriesByTimezoneName[timezone]];
}

void CPosixTimezone::SetTimezone(const std::string& timezoneName)
{
#if defined(TARGET_DARWIN)
  return;
#endif

  setenv("TZ", timezoneName.c_str(), 1);
  tzset();
}

std::string CPosixTimezone::GetOSConfiguredTimezone()
{
  return date::get_tzdb().current_zone()->name();
}

void CPosixTimezone::SettingOptionsTimezoneCountriesFiller(
    const std::shared_ptr<const CSetting>& setting,
    std::vector<StringSettingOption>& list,
    std::string& current,
    void* data)
{
  std::vector<std::string> countries = g_timezone.GetCountries();
  for (const auto& country : countries)
    list.emplace_back(country, country);
}

void CPosixTimezone::SettingOptionsTimezonesFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current,
                                                   void* data)
{
  current = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  bool found = false;
  std::vector<std::string> timezones = g_timezone.GetTimezonesByCountry(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  for (const auto& timezone : timezones)
  {
    if (!found && StringUtils::EqualsNoCase(timezone, current))
      found = true;

    list.emplace_back(timezone, timezone);
  }

  if (!found && !timezones.empty())
    current = timezones[0];
}

CPosixTimezone g_timezone;
