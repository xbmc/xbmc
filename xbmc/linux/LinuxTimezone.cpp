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

#include <time.h>
#include "system.h"
#ifdef TARGET_ANDROID
#include "android/bionic_supplement/bionic_supplement.h"
#endif
#include "PlatformInclude.h"
#include "LinuxTimezone.h"
#include "utils/SystemInfo.h"
#if defined(TARGET_DARWIN)
#include "OSXGNUReplacements.h"
#endif
#ifdef __FreeBSD__
#include "freebsd/FreeBSDGNUReplacements.h"
#endif

#include "Util.h"

using namespace std;

CLinuxTimezone::CLinuxTimezone() : m_IsDST(0)
{
   char* line = NULL;
   size_t linelen = 0;
   int nameonfourthfield = 0;
   CStdString s;
   vector<CStdString> tokens;

   // Load timezones
   FILE* fp = fopen("/usr/share/zoneinfo/zone.tab", "r");
   if (fp)
   {
      CStdString countryCode;
      CStdString timezoneName;

      while (getdelim(&line, &linelen, '\n', fp) > 0)
      {
         tokens.clear();
         s = line;
         s.TrimLeft(" \t").TrimRight(" \n");

         if (s.length() == 0)
            continue;

         if (s[0] == '#')
            continue;

         CUtil::Tokenize(s, tokens, " \t");
         if (tokens.size() < 3)
            continue;

         countryCode = tokens[0];
         timezoneName = tokens[2];

         if (m_timezonesByCountryCode.count(countryCode) == 0)
         {
            vector<CStdString> timezones;
            timezones.push_back(timezoneName);
            m_timezonesByCountryCode[countryCode] = timezones;
         }
         else
         {
            vector<CStdString>& timezones = m_timezonesByCountryCode[countryCode];
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
      CStdString countryCode;
      CStdString countryName;

      while (getdelim(&line, &linelen, '\n', fp) > 0)
      {
         s = line;
         s.TrimLeft(" \t").TrimRight(" \n");

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

         countryCode = s.Left(2);
         countryName = s.Mid(i);

         m_counties.push_back(countryName);
         m_countryByCode[countryCode] = countryName;
         m_countryByName[countryName] = countryCode;
      }
      sort(m_counties.begin(), m_counties.end(), sortstringbyname());
      fclose(fp);
   }
   free(line);
}

vector<CStdString> CLinuxTimezone::GetCounties()
{
   return m_counties;
}

vector<CStdString> CLinuxTimezone::GetTimezonesByCountry(const CStdString country)
{
   return m_timezonesByCountryCode[m_countryByName[country]];
}

CStdString CLinuxTimezone::GetCountryByTimezone(const CStdString timezone)
{
#if defined(TARGET_DARWIN)
   return CStdString("?");
#else
   return m_countryByCode[m_countriesByTimezoneName[timezone]];
#endif
}

void CLinuxTimezone::SetTimezone(CStdString timezoneName)
{
  bool use_timezone = false;
  
#if !defined(TARGET_DARWIN)
  use_timezone = true;
#else
  if (g_sysinfo.IsAppleTV2())
    use_timezone = true;  
#endif
  
  if (use_timezone)
  {
    static char env_var[255];
    sprintf(env_var, "TZ=:%s", timezoneName.c_str());
    putenv(env_var);
    tzset();
  }
}

CStdString CLinuxTimezone::GetOSConfiguredTimezone()
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

CLinuxTimezone g_timezone;
