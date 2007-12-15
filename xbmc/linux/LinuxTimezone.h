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
   
   vector<CStdString> GetCounties();
   vector<CStdString> GetTimezonesByCountry(const CStdString country);
   CStdString GetCountryByTimezone(const CStdString timezone);

   void SetTimezone(CStdString timezone);
   
private:
   vector<CStdString> m_counties;
   map<CStdString, CStdString> m_countryByCode;
   map<CStdString, CStdString> m_countryByName;
   
   map<CStdString, vector<CStdString> > m_timezonesByCountryCode;
   map<CStdString, CStdString> m_countriesByTimezoneName;
};

extern CLinuxTimezone g_timezone;

#endif
