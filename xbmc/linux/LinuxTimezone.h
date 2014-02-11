#ifndef LINUX_TIMEZONE_
#define LINUX_TIMEZONE_

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "utils/StdString.h"
#include <vector>
#include <map>

class CSetting;

class CLinuxTimezone : public ISettingCallback, public ISettingsHandler
{
public:
   CLinuxTimezone();

   virtual void OnSettingChanged(const CSetting *setting);

   virtual void OnSettingsLoaded();

   CStdString GetOSConfiguredTimezone();

   std::vector<CStdString> GetCounties();
   std::vector<CStdString> GetTimezonesByCountry(const CStdString country);
   CStdString GetCountryByTimezone(const CStdString timezone);

   void SetTimezone(CStdString timezone);
   int m_IsDST;

   static void SettingOptionsTimezoneCountriesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current);
   static void SettingOptionsTimezonesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current);

private:
   std::vector<CStdString> m_counties;
   std::map<CStdString, CStdString> m_countryByCode;
   std::map<CStdString, CStdString> m_countryByName;

   std::map<CStdString, std::vector<CStdString> > m_timezonesByCountryCode;
   std::map<CStdString, CStdString> m_countriesByTimezoneName;
};

extern CLinuxTimezone g_timezone;

#endif
