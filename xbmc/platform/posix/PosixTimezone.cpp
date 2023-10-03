/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PosixTimezone.h"

#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <ctime>

#include "PlatformDefs.h"

CPosixTimezone::CPosixTimezone()
{
   char* line = NULL;
   size_t linelen = 0;
   int nameonfourthfield = 0;
   std::string s;
   std::vector<std::string> tokens;

   // Load timezones
   FILE* fp = fopen("/usr/share/zoneinfo/zone.tab", "r");
   if (fp)
   {
      std::string countryCode;
      std::string timezoneName;

      while (getdelim(&line, &linelen, '\n', fp) > 0)
      {
         tokens.clear();
         s = line;
         StringUtils::Trim(s);

         if (s.length() == 0)
            continue;

         if (s[0] == '#')
            continue;

         StringUtils::Tokenize(s, tokens, " \t");
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
      fclose(fp);
   }

   if (line)
   {
     free(line);
     line = NULL;
     linelen = 0;
   }

   // Load countries
   fp = fopen("/usr/share/zoneinfo/iso3166.tab", "r");
   if (!fp)
   {
      fp = fopen("/usr/share/misc/iso3166", "r");
      nameonfourthfield = 1;
   }
   if (fp)
   {
      std::string countryCode;
      std::string countryName;

      while (getdelim(&line, &linelen, '\n', fp) > 0)
      {
         s = line;
         StringUtils::Trim(s);

        //! @todo STRING_CLEANUP
         if (s.length() == 0)
            continue;

         if (s[0] == '#')
            continue;

         // Search for the first non space from the 2nd character and on
         int i = 2;
         while (s[i] == ' ' || s[i] == '\t') i++;

         if (nameonfourthfield)
         {
            // skip three letter
            while (s[i] != ' ' && s[i] != '\t') i++;
            while (s[i] == ' ' || s[i] == '\t') i++;
            // skip number
            while (s[i] != ' ' && s[i] != '\t') i++;
            while (s[i] == ' ' || s[i] == '\t') i++;
         }

         countryCode = s.substr(0, 2);
         countryName = s.substr(i);

         m_counties.push_back(countryName);
         m_countryByCode[countryCode] = countryName;
         m_countryByName[countryName] = countryCode;
      }
      sort(m_counties.begin(), m_counties.end(), sortstringbyname());
      fclose(fp);
   }
   free(line);
}

void CPosixTimezone::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_TIMEZONE)
  {
    SetTimezone(std::static_pointer_cast<const CSettingString>(setting)->GetValue());

    CDateTime::ResetTimezoneBias();
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
  CDateTime::ResetTimezoneBias();
}

std::vector<std::string> CPosixTimezone::GetCounties()
{
   return m_counties;
}

std::vector<std::string> CPosixTimezone::GetTimezonesByCountry(const std::string& country)
{
   return m_timezonesByCountryCode[m_countryByName[country]];
}

std::string CPosixTimezone::GetCountryByTimezone(const std::string& timezone)
{
#if defined(TARGET_DARWIN)
   return "?";
#else
   return m_countryByCode[m_countriesByTimezoneName[timezone]];
#endif
}

void CPosixTimezone::SetTimezone(const std::string& timezoneName)
{
#if !defined(TARGET_DARWIN)
  bool use_timezone = true;
#else
  bool use_timezone = false;
#endif

  if (use_timezone)
  {
    static char env_var[255];
    snprintf(env_var, sizeof(env_var), "TZ=:%s", timezoneName.c_str());
    putenv(env_var);
    tzset();
  }
}

std::string CPosixTimezone::GetOSConfiguredTimezone()
{
  std::string timezoneName;

  // try Slackware approach first
  timezoneName = ReadFromLocaltime("/etc/localtime-copied-from");

  // RHEL and maybe other distros make /etc/localtime a symlink
  if (timezoneName.empty())
    timezoneName = ReadFromLocaltime("/etc/localtime");

  // now try Debian approach
  if (timezoneName.empty())
    timezoneName = ReadFromTimezone("/etc/timezone");

  return timezoneName;
}

std::string CPosixTimezone::ReadFromLocaltime(const std::string_view filename)
{
  char path[PATH_MAX];
  if(realpath(filename.data(), path) == nullptr)
    return "";

  // Read the timezone starting from the second last occurrence of /
  std::string str = path;
  size_t pos = str.rfind('/');
  if (pos == std::string::npos)
    return "";

  pos = str.rfind('/', pos - 1);
  if (pos == std::string::npos)
    return "";

  return str.substr(pos + 1);
}

std::string CPosixTimezone::ReadFromTimezone(const std::string_view filename)
{
  std::string timezoneName;
  std::FILE* file = std::fopen(filename.data(), "r");

  if (file != nullptr)
  {
    char tz[255];
    if (std::fgets(tz, sizeof(tz), file) != nullptr)
    {
      timezoneName = tz;
    }

    std::fclose(file);
  }

  return timezoneName;
}

void CPosixTimezone::SettingOptionsTimezoneCountriesFiller(
    const std::shared_ptr<const CSetting>& setting,
    std::vector<StringSettingOption>& list,
    std::string& current,
    void* data)
{
  std::vector<std::string> countries = g_timezone.GetCounties();
  for (unsigned int i = 0; i < countries.size(); i++)
    list.emplace_back(countries[i], countries[i]);
}

void CPosixTimezone::SettingOptionsTimezonesFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current,
                                                   void* data)
{
  current = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  bool found = false;
  std::vector<std::string> timezones = g_timezone.GetTimezonesByCountry(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  for (unsigned int i = 0; i < timezones.size(); i++)
  {
    if (!found && StringUtils::EqualsNoCase(timezones[i], current))
      found = true;

    list.emplace_back(timezones[i], timezones[i]);
  }

  if (!found && !timezones.empty())
    current = timezones[0];
}

CPosixTimezone g_timezone;
