#ifndef LINUX_TIMEZONE_
#define LINUX_TIMEZONE_

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

#include "utils/StdString.h"
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
   int m_IsDST;
private:
   std::vector<CStdString> m_counties;
   std::map<CStdString, CStdString> m_countryByCode;
   std::map<CStdString, CStdString> m_countryByName;

   std::map<CStdString, std::vector<CStdString> > m_timezonesByCountryCode;
   std::map<CStdString, CStdString> m_countriesByTimezoneName;
};

extern CLinuxTimezone g_timezone;

#endif
