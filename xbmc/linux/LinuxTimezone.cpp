/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <time.h>
#include "system.h"
#ifdef TARGET_ANDROID
#include "platform/android/bionic_supplement/bionic_supplement.h"
#endif
#include "PlatformInclude.h"
#include "LinuxTimezone.h"
#include "utils/SystemInfo.h"
#if defined(TARGET_DARWIN)
#include "platform/darwin/OSXGNUReplacements.h"
#endif
#ifdef TARGET_FREEBSD
#include "freebsd/FreeBSDGNUReplacements.h"
#endif

#include "Util.h"
#include "utils/StringUtils.h"
#include "XBDateTime.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include <stdlib.h>

#include <algorithm>

CLinuxTimezone::CLinuxTimezone() : m_IsDST(0)
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

void CLinuxTimezone::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_TIMEZONE)
  {
    SetTimezone(((CSettingString*)setting)->GetValue());

    CDateTime::ResetTimezoneBias();
  }
  else if (settingId == CSettings::SETTING_LOCALE_TIMEZONECOUNTRY)
  {
    // nothing to do here. Changing locale.timezonecountry will trigger an
    // update of locale.timezone and automatically adjust its value
    // and execute OnSettingChanged() for it as well (see above)
  }
}

void CLinuxTimezone::OnSettingsLoaded()
{
  SetTimezone(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_TIMEZONE));
  CDateTime::ResetTimezoneBias();
}

std::vector<std::string> CLinuxTimezone::GetCounties()
{
   return m_counties;
}

std::vector<std::string> CLinuxTimezone::GetTimezonesByCountry(const std::string& country)
{
   return m_timezonesByCountryCode[m_countryByName[country]];
}

std::string CLinuxTimezone::GetCountryByTimezone(const std::string& timezone)
{
#if defined(TARGET_DARWIN)
   return "?";
#else
   return m_countryByCode[m_countriesByTimezoneName[timezone]];
#endif
}

void CLinuxTimezone::SetTimezone(std::string timezoneName)
{
#if !defined(TARGET_DARWIN)
  bool use_timezone = true;
#else
  bool use_timezone = false;
#endif
  
  if (use_timezone)
  {
    static char env_var[255];
    sprintf(env_var, "TZ=:%s", timezoneName.c_str());
    putenv(env_var);
    tzset();
  }
}

std::string CLinuxTimezone::GetOSConfiguredTimezone()
{
   char timezoneName[255];

   // try Slackware approach first
   ssize_t rlrc = readlink("/etc/localtime-copied-from"
                           , timezoneName, sizeof(timezoneName)-1);
   if (rlrc != -1)
   {
     timezoneName[rlrc] = '\0';

     char* p = strrchr(timezoneName,'/');
     if (p)
     { // we want the previous '/'
       char* q = p;
       *q = 0;
       p = strrchr(timezoneName,'/');
       *q = '/';
       if (p)
         p++;
     }
     return p;
   }

   // now try Debian approach
   timezoneName[0] = 0;
   FILE* fp = fopen("/etc/timezone", "r");
   if (fp)
   {
      if (fgets(timezoneName, sizeof(timezoneName), fp))
        timezoneName[strlen(timezoneName)-1] = '\0';
      fclose(fp);
   }

   return timezoneName;
}

void CLinuxTimezone::SettingOptionsTimezoneCountriesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  std::vector<std::string> countries = g_timezone.GetCounties();
  for (unsigned int i = 0; i < countries.size(); i++)
    list.push_back(std::make_pair(countries[i], countries[i]));
}

void CLinuxTimezone::SettingOptionsTimezonesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  current = ((const CSettingString*)setting)->GetValue();
  bool found = false;
  std::vector<std::string> timezones = g_timezone.GetTimezonesByCountry(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  for (unsigned int i = 0; i < timezones.size(); i++)
  {
    if (!found && StringUtils::EqualsNoCase(timezones[i], current))
      found = true;

    list.push_back(std::make_pair(timezones[i], timezones[i]));
  }

  if (!found && !timezones.empty())
    current = timezones[0];
}

CLinuxTimezone g_timezone;
