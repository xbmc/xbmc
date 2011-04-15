#pragma once

/*
 *      Copyright (C) 2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "InfoLoader.h"
#include "StdString.h"

#include <map>

class TiXmlElement;

// Contains a blank string if the information is unknown
#define LOCATION_UPDATE_TIME     40
#define LOCATION_COUNTRY_CODE    41 // e.g. "US"
#define LOCATION_COUNTRY_NAME    42 // e.g. "United States"
#define LOCATION_REGION_NAME     43 // State or territory
#define LOCATION_CITY            44
#define LOCATION_ZIP_POSTAL_CODE 45 // Generally more specific than weather ID
#define LOCATION_LATITUDE        46 // float
#define LOCATION_LONGITUDE       47 // float
#define LOCATION_TIMEZONE_NAME   48 // e.g. "America/Los_Angeles"
#define LOCATION_GMT_OFFSET      49 // In minutes!
#define LOCATION_DST_OFFSET      50 // In minutes!
#define LOCATION_DST_OBSERVED    51 // "1" or "0", depending on if DST is currently in observance
#define LOCATION_IP_ADDRESS      52 // external IP address
#define LOCATION_WEATHER_ID      53 // Cooperative Station Number, e.g. "USCA1024"


class CLocationInfo
{
public:
  CStdString lastUpdateTime;
  CStdString countryCode;
  CStdString countryName;
  CStdString regionName;
  CStdString city;
  CStdString zipPostalCode;
  CStdString latitude;
  CStdString longitude;
  CStdString timezoneName;
  CStdString gmtOffset;
  CStdString dstOffset;
  CStdString dstObserved;
  CStdString ipAddress;
  CStdString weatherId;

  void Reset()
  {
    lastUpdateTime = "";
    countryCode = "";
    countryName = "";
    regionName = "";
    city = "";
    zipPostalCode = "";
    latitude = "";
    longitude = "";
    timezoneName = "";
    gmtOffset = "";
    dstOffset = "";
    dstObserved = "";
    ipAddress = "";
    weatherId = "";
  };
};

class CLocationJob : public CJob
{
public:
  virtual bool DoWork();
  const CLocationInfo &GetInfo() const { return m_info; };

private:
  bool GetLocationByWifi();
  bool GetLocationByIP();
  bool ReverseGeocode(CStdString lat, CStdString lon);
  static void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString &value, const CStdString& strDefaultValue = "");

  CLocationInfo m_info;
};

class CLocation : public CInfoLoader
{
public:
  CLocation(void);
  virtual ~CLocation(void);

  bool IsFetched();
  void Reset();

  const CStdString &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  void SetProvider(CStdString strProvider) { m_strProvider = strProvider; };
  CStdString GetProvider() const { return m_strProvider; };

protected:
  virtual CJob *GetJob() const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  virtual CStdString TranslateInfo(int info) const;

private:
  CStdString m_strProvider;
  CLocationInfo m_info;
};

extern CLocation g_locationManager;
