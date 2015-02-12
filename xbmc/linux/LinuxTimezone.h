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
#include <string>
#include <vector>
#include <map>

class CSetting;

class CLinuxTimezone : public ISettingCallback, public ISettingsHandler
{
public:
   CLinuxTimezone();

   virtual void OnSettingChanged(const CSetting *setting);

   virtual void OnSettingsLoaded();

   std::string GetOSConfiguredTimezone();

   std::vector<std::string> GetCounties();
   std::vector<std::string> GetTimezonesByCountry(const std::string& country);
   std::string GetCountryByTimezone(const std::string& timezone);

   void SetTimezone(std::string timezone);
   int m_IsDST;

   static void SettingOptionsTimezoneCountriesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
   static void SettingOptionsTimezonesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);

private:
   std::vector<std::string> m_counties;
   std::map<std::string, std::string> m_countryByCode;
   std::map<std::string, std::string> m_countryByName;

   std::map<std::string, std::vector<std::string> > m_timezonesByCountryCode;
   std::map<std::string, std::string> m_countriesByTimezoneName;
};

extern CLinuxTimezone g_timezone;

#endif
