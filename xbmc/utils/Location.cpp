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

#include "system.h"
#include "Location.h"
#include "network/Network.h"
#include "filesystem/FileCurl.h"
#include "XMLUtils.h"
#include "jsoncpp/include/json/json.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "log.h"
#include <vector>

#define IPINFODB_API_KEY "ba574d48a23cd6509fda7f907da179567cfcdfa801699e1727d8e9599c0f0e5d"

using namespace Json;
using namespace XFILE;

CLocation g_locationManager;

bool CLocationJob::DoWork()
{
  // wait for the network
  if (!g_application.getNetwork().IsAvailable(true))
    return false;

  // try wifi geolocation, and if that fails use IP geolocation
  if (!GetLocationByWifi())
  {
    if (!GetLocationByIP())
    {
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_LOCATION_FAILED);
      g_windowManager.SendThreadMessage(msg);
      return false;
    }
  }

  // sometimes Google and IPInfoDB don't return zip code information, so query it separately if needed
  if (!m_info.latitude.IsEmpty() && !m_info.longitude.IsEmpty())
    ReverseGeocode(m_info.latitude, m_info.longitude);

  // timestamp the data
  CDateTime time = CDateTime::GetCurrentDateTime();
  m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);

  // send a message that we're done
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_LOCATION_FETCHED);
  g_windowManager.SendThreadMessage(msg);

  return true;
}

/* Geolocation by scanning for nearby wifi networks and using Google Gears to get lat,lon */
bool CLocationJob::GetLocationByWifi()
{
  // Scan for wireless networks
  std::vector<NetworkAccessPoint> vec_networks;
  std::vector<CNetworkInterface*> vec_interfaces = g_application.getNetwork().GetInterfaceList();
  for(std::vector<CNetworkInterface*>::iterator it = vec_interfaces.begin(); it != vec_interfaces.end(); it++)
  {
    std::vector<NetworkAccessPoint> interface_accesspoints = (*it)->GetAccessPoints();
    if (interface_accesspoints.size() != 0)
    {
      if (vec_networks.size() != 0)
        std::copy(interface_accesspoints.begin(), interface_accesspoints.end(), vec_networks.begin());
      else
        vec_networks.swap(interface_accesspoints);
    }
  }
  if (vec_networks.size() == 0)
    return false;
  
  // Build the JSON query
  CStdString network_list, strPostData;
  for(std::vector<NetworkAccessPoint>::iterator it = vec_networks.begin(); it != vec_networks.end(); it++)
  {
    network_list.AppendFormat(network_list.IsEmpty()?"%s":",%s", it->toJson().c_str());
  }
  strPostData.Format("{\"host\":\"xbmc.org\",\"version\":\"1.1.0\",\"request_address\":true,\"wifi_towers\":[%s]}", network_list);

  // Reverse geocode our location
  CStdString json;
  XFILE::CFileCurl httpUtil;
  if (!httpUtil.Post("http://www.google.com/loc/json", strPostData, json))
  {
    CLog::Log(LOGERROR, "LOCATION: Google geolocation failed!");
    return false;
  }

  JSON_API Reader reader;
  JSON_API Value root;
  if (!reader.parse(json, root, false))
  {
    CLog::Log(LOGERROR, "LOCATION: %s", reader.getFormatedErrorMessages().c_str());
    return false;
  }
  if (!root.isObject() || !root.size())
  {
    CLog::Log(LOGERROR, "LOCATION: Google geolocation failed: Empty response");
    return false;
  }

  // Extract data from response
  JSON_API Value location = root["location"];
  if (location.isObject() && location.size())
  {
    m_info.latitude.Format("%.6f", location["latitude"].asDouble());
    m_info.longitude.Format("%.6f", location["longitude"].asDouble());
    JSON_API Value address = location["address"];
    if (address.isObject() && address.size())
    {
      m_info.countryCode = address["country_code"].asString();
      m_info.countryName = address["country"].asString();
      m_info.regionName = address["region"].asString();
      m_info.city = address["city"].asString();
      m_info.zipPostalCode = address["postal_code"].asString();
    }
  }
  return true;
}

/* Geolocation by using IPInfoDB with the current external IP address */
bool CLocationJob::GetLocationByIP()
{
  // Set to true for timezone/offset information
  const bool bTimezone = false;

  // Download our location
  CLog::Log(LOGINFO, "LOCATION: Downloading location from IPInfoDB.com");
  XFILE::CFileCurl httpUtil;
  CStdString strURL, xml;
  strURL.Format("http://api.ipinfodb.com/v2/ip_query.php?key="IPINFODB_API_KEY"&timezone=%s", bTimezone?"true":"false");
  if (!httpUtil.Get(strURL, xml))
  {
    CLog::Log(LOGERROR, "LOCATION: Location download failed!");
    return false;
  }
  
  // Load the xml file
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(xml))
  {
    CLog::Log(LOGERROR, "LOCATION: Unable to get data - invalid XML");
    return false;
  }
  TiXmlElement *pResponse = xmlDoc.RootElement();
  if (!pResponse)
  {
    CLog::Log(LOGERROR, "LOCATION: Unable to get data - invalid XML");
    return false;
  }
  
  CStdString status;
  GetString(pResponse, "Status", status);
  // If Status is not OK display the error message
  if (!status.Equals("OK"))
  {
    CLog::Log(LOGERROR, "LOCATION: Unable to get data: %s", status.IsEmpty() ? "Unknown error" : status.c_str());
    return false;
  }

  // Extract data from response
  GetString(pResponse, "CountryCode", m_info.countryCode);
  GetString(pResponse, "CountryName", m_info.countryName);
  GetString(pResponse, "RegionName", m_info.regionName);
  GetString(pResponse, "City", m_info.city);
  GetString(pResponse, "ZipPostalCode", m_info.zipPostalCode);
  GetString(pResponse, "Latitude", m_info.latitude);
  GetString(pResponse, "Longitude", m_info.longitude);
  if (bTimezone)
  {
    GetString(pResponse, "TimezoneName", m_info.timezoneName);
    GetString(pResponse, "Gmtoffset", m_info.gmtOffset);
    GetString(pResponse, "Dstoffset", m_info.dstOffset);
    GetString(pResponse, "Isdst", m_info.dstObserved);
  }
  GetString(pResponse, "Ip", m_info.ipAddress);

  // Clean up the data a little
  if (m_info.latitude.Equals("0") || m_info.longitude.Equals("0"))
  {
    m_info.latitude = "";
    m_info.longitude = "";
  }
  if (bTimezone)
  {
    // Convert from seconds to minutes
    if (!m_info.gmtOffset.IsEmpty())
      m_info.gmtOffset.Format("%d", atoi(m_info.gmtOffset.c_str()) / 60);
    if (!m_info.dstOffset.IsEmpty())
      m_info.dstOffset.Format("%d", atoi(m_info.dstOffset.c_str()) / 60);
  }
  return true;
}

bool CLocationJob::ReverseGeocode(CStdString lat, CStdString lon)
{
  // First, check to see what info we don't have
  unsigned char infoNeeded = 0;
  const unsigned char COUNTRYCODE = 1 << 0; // country, short_name
  const unsigned char COUNTRYNAME = 1 << 1; // country, long_name
  const unsigned char REGIONNAME  = 1 << 2; // administrative_area_level_1
  const unsigned char CITY        = 1 << 3; // locality or sublocality
  const unsigned char POSTALCODE  = 1 << 4; // postal_code
  if (m_info.countryCode.IsEmpty())   infoNeeded |= COUNTRYCODE;
  if (m_info.countryName.IsEmpty())   infoNeeded |= COUNTRYNAME;
  if (m_info.regionName.IsEmpty())    infoNeeded |= REGIONNAME;
  if (m_info.city.IsEmpty())          infoNeeded |= CITY;
  if (m_info.zipPostalCode.IsEmpty()) infoNeeded |= POSTALCODE;

  // No extra info is available through the Google Maps API
  if (!infoNeeded)
    return true;

  // Reverse geocode our location
  CLog::Log(LOGINFO, "LOCATION: Reverse geocoding coordinates (%s, %s)", m_info.latitude.c_str(), m_info.longitude.c_str());
  CStdString url, json;
  url.Format("http://maps.googleapis.com/maps/api/geocode/json?latlng=%s,%s&sensor=false", lat.c_str(), lon.c_str());
  XFILE::CFileCurl httpUtil;
  if (!httpUtil.Get(url, json))
  {
    CLog::Log(LOGERROR, "LOCATION: Reverse geocoding failed!");
    return false;
  }

  JSON_API Reader reader;
  JSON_API Value root;
  if (!reader.parse(json, root, false))
  {
    CLog::Log(LOGERROR, "LOCATION: %s", reader.getFormatedErrorMessages().c_str());
    return false;
  }
  if (!root.isObject() || !root.size())
  {
    CLog::Log(LOGERROR, "LOCATION: Reverse geocoding failed: Empty response");
    return false;
  }
  if (root["status"].asString() != "OK")
  {
    CLog::Log(LOGERROR, "LOCATION: Reverse geocoding failed: %s", root["status"].asString().c_str());
    return false;
  }

  // Extract data from response
  for (unsigned int i = 0; infoNeeded && i < root["results"].size(); ++i)
  {
    JSON_API Value result = root["results"][i];
    for (unsigned int j = 0; infoNeeded && j < result.size(); ++j)
    {
      JSON_API Value address_component = result["address_components"][j];
      for (unsigned int k = 0; k < address_component.size(); ++k)
      {
        JSON_API Value type = address_component["types"][k];
        if ((infoNeeded & COUNTRYCODE) && type.asString() == "country")
        {
          m_info.countryCode = address_component["short_name"].asString();
          infoNeeded &= ~COUNTRYCODE;
          // No break
        }
        if ((infoNeeded & COUNTRYNAME) && type.asString() == "country")
        {
          m_info.countryName = address_component["long_name"].asString();
          infoNeeded &= ~COUNTRYNAME;
          break;
        }
        else if ((infoNeeded & REGIONNAME) && type.asString() == "administrative_area_level_1")
        {
          m_info.regionName = address_component["short_name"].asString(); // Abbreviated state
          infoNeeded &= ~REGIONNAME;
          break;
        }
        else if ((infoNeeded & CITY) && (type.asString() == "locality" || type.asString() == "sublocality"))
        {
          m_info.city = address_component["long_name"].asString();
          infoNeeded &= ~CITY;
          break;
        }
        else if ((infoNeeded & POSTALCODE) && type.asString() == "postal_code")
        {
          m_info.zipPostalCode = address_component["long_name"].asString();
          infoNeeded &= ~POSTALCODE;
          break;
        }
      }
    }
  }
  return true;
}

void CLocationJob::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString &value)
{
  if (!XMLUtils::GetString(pRootElement, strTagName.c_str(), value) || value.Equals("-"))
    value = "";
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLocation::CLocation(void) : CInfoLoader(24 * 60 * 60 * 1000) // 1 day
{
  Reset();
}

CLocation::~CLocation(void)
{
}

/*!
  \brief Marks the info as stale by blanking everything
 */
void CLocation::Reset()
{
  m_info.Reset();
}

/*!
  \brief  Checks if info is available. Has the side effect of fetching info
  \return true if info is currently available
 */
bool CLocation::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !GetLastUpdateTime().IsEmpty();
}

/*!
  \brief  Translates a location info ID into a value
  \param  info
  \return the info or "" if unknown
 */
CStdString CLocation::TranslateInfo(int info) const
{
  if (info == LOCATION_UPDATE_TIME) return m_info.lastUpdateTime;
  else if (info == LOCATION_COUNTRY_CODE) return m_info.countryCode;
  else if (info == LOCATION_COUNTRY_NAME) return m_info.countryName;
  else if (info == LOCATION_REGION_NAME) return m_info.regionName;
  else if (info == LOCATION_CITY) return m_info.city;
  else if (info == LOCATION_ZIP_POSTAL_CODE) return m_info.zipPostalCode;
  else if (info == LOCATION_LATITUDE) return m_info.latitude;
  else if (info == LOCATION_LONGITUDE) return m_info.longitude;
  else if (info == LOCATION_TIMEZONE_NAME) return m_info.timezoneName;
  else if (info == LOCATION_GMT_OFFSET) return m_info.gmtOffset;
  else if (info == LOCATION_DST_OFFSET) return m_info.dstOffset;
  else if (info == LOCATION_DST_OBSERVED) return m_info.dstObserved;
  else if (info == LOCATION_IP_ADDRESS) return m_info.ipAddress;
  else if (info == LOCATION_WEATHER_ID) return m_info.weatherId;
  return "";
}

CJob *CLocation::GetJob() const
{
  return new CLocationJob();
}

/*!
  \param jobId   unused
  \param success if this is false, then the location info hasn't been updated
  \param job     a reference to the job that just reached completion
 */
void CLocation::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // if success is false then job->GetInfo() is empty, so no need to store it
  if (success)
    m_info = ((CLocationJob *)job)->GetInfo();
  CInfoLoader::OnJobComplete(jobID, success, job);
}
