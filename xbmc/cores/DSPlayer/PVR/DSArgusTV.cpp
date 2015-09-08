/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
*
*      Copyright (C) 2015 Romank
*      https://github.com/Romank1
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

#ifdef HAS_DS_PLAYER

#include "DSArgusTV.h"

CDSArgusTV::CDSArgusTV(const CStdString& strBackendBaseAddress, const CStdString& strBackendName) 
  : CDSPVRBackend(strBackendBaseAddress, strBackendName)
{
  CLog::Log(LOGNOTICE, "%s PVR Backend name: %s, Base Address: %s", __FUNCTION__, strBackendName.c_str(), strBackendBaseAddress.c_str());
}

CDSArgusTV::~CDSArgusTV(void)
{
  CLog::Log(LOGNOTICE, "%s", __FUNCTION__);
}

bool CDSArgusTV::ConvertStreamURLToTimeShiftFilePath(const CStdString& strUrl, CStdString& strTimeShiftFile)
{
  bool bReturn = false;

  if (strUrl.empty())
  {
    CLog::Log(LOGERROR, "%s Stream URL is not valid.", __FUNCTION__);
    return false;
  }

  if (!SupportsStreamConversion(strUrl))
  {
    CLog::Log(LOGERROR, "%s Stream Conversion is not supported.", __FUNCTION__);
    return false;
  }
  
  strTimeShiftFile.clear() ;
  CStdString strTimeShiftFilePath;
  
  CVariant json_response;
  bReturn = SendCommandToArgusTV_GET("ArgusTV/Control/GetLiveStreams", json_response);
  if (bReturn)
  {	  
    bReturn = false;
    if(json_response.isArray())
    {
      for (CVariant::const_iterator_array itemItr = json_response.begin_array(); itemItr != json_response.end_array(); itemItr++)
      {
        if (!(*itemItr).isObject())
          continue;
      
        if ((*itemItr).isMember("RtspUrl") && (*itemItr)["RtspUrl"].isString() && (*itemItr)["RtspUrl"].asString() == strUrl)
        {    
          if ((*itemItr).isMember("TimeshiftFile") && (*itemItr)["TimeshiftFile"].isString())
          {
            strTimeShiftFilePath = (*itemItr)["TimeshiftFile"].asString();
            bReturn = true;
          }
        }
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s Failed to get Timeshifting info from the backend", __FUNCTION__);
  }

  if (bReturn)
    bReturn = IsFileExistAndAccessible(strTimeShiftFilePath);
  
  if (bReturn)
  {
    CLog::Log(LOGNOTICE, "%s Rtsp Stream: %s converted to Timeshift File Path: %s", __FUNCTION__, strUrl.c_str(), strTimeShiftFilePath.c_str());
    strTimeShiftFile = strTimeShiftFilePath;
  }
  else
  {
    CLog::Log(LOGERROR, "%s Failed to converted Rtsp Stream: %s to Timeshift File Path", __FUNCTION__, strUrl.c_str());
  }

  return bReturn;
}

bool CDSArgusTV::SendCommandToArgusTV_GET(const CStdString& strCommand, CVariant &json_response)
{
  return JSONRPCSendCommand(REQUEST_GET, strCommand, "", json_response);
}

bool CDSArgusTV::SendCommandToArgusTV_POST(const CStdString& strCommand, const CStdString& strArguments, CVariant &json_response)
{
  return JSONRPCSendCommand(REQUEST_POST, strCommand, strArguments, json_response);
}

#endif
