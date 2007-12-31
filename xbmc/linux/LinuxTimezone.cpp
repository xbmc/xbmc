#include <time.h>
#include "LinuxTimezone.h"
#include "Util.h"
#include "getdelim.h"

CLinuxTimezone::CLinuxTimezone()
{
   char* line = NULL;
   size_t linelen;
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
   
   // Load countries 
   fp = fopen("/usr/share/zoneinfo/iso3166.tab", "r");
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
                  
         countryCode = s.Left(2);
         countryName = s.Mid(i);
         
         m_counties.push_back(countryName);
         m_countryByCode[countryCode] = countryName;
         m_countryByName[countryName] = countryCode;
      }
   
      sort(m_counties.begin(), m_counties.end(), sortstringbyname());

      fclose(fp);
   }
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
#ifdef __APPLE__
  return CStdString("?");
#else
   return m_countryByCode[m_countriesByTimezoneName[timezone]];
#endif
}

void CLinuxTimezone::SetTimezone(CStdString timezoneName)
{
   static char env_var[255];
   sprintf(env_var, "TZ=:%s", timezoneName.c_str());
   putenv(env_var);
   tzset();
}

CStdString CLinuxTimezone::GetOSConfiguredTimezone()
{
   char timezoneName[255];
   FILE* fp = fopen("/etc/timezone", "r");
   if (fp)
   {
      fgets(timezoneName, sizeof(timezoneName), fp);
      timezoneName[strlen(timezoneName)-1] = '\0';
      fclose(fp);
   }
   
   return timezoneName;   
}

CLinuxTimezone g_timezone;
