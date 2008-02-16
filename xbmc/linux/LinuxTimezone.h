#ifndef LINUX_TIMEZONE_
#define LINUX_TIMEZONE_

#include <StdString.h>
#include <vector>
#include <map>

class CLinuxTimezone
{
public:
   CLinuxTimezone();
   CStdString GetOSConfiguredTimezone();
   
   std::vector<CStdString> GetCounties();
   std::vector<CStdString> GetTimezonesByCountry(const CStdString country);
   CStdString GetCountryByTimezone(const CStdString timezone);

   void SetTimezone(CStdString timezone);
   
private:
   std::vector<CStdString> m_counties;
   std::map<CStdString, CStdString> m_countryByCode;
   std::map<CStdString, CStdString> m_countryByName;
   
   std::map<CStdString, std::vector<CStdString> > m_timezonesByCountryCode;
   std::map<CStdString, CStdString> m_countriesByTimezoneName;
};

extern CLinuxTimezone g_timezone;

#endif
