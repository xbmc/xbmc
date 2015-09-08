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

#include "DSMediaPortal.h"
#include "utils/uri.h"

CDSMediaPortal::CDSMediaPortal(const CStdString& strBackendBaseAddress, const CStdString& strBackendName) 
  : CDSPVRBackend(strBackendBaseAddress, strBackendName)
  , m_pCardsSettings(NULL)
{
  CLog::Log(LOGNOTICE, "%s PVR Backend name: %s, Base Address: %s", __FUNCTION__, strBackendName.c_str(), strBackendBaseAddress.c_str());
}

CDSMediaPortal::~CDSMediaPortal(void)
{
  CLog::Log(LOGNOTICE, "%s", __FUNCTION__);
  SAFE_DELETE(m_pCardsSettings);
}

bool CDSMediaPortal::ConvertStreamURLToTimeShiftFilePath(const CStdString& strUrl, CStdString& strTimeShiftFile)
{
  bool bReturn = false;
  CStdString strResponse;
  strTimeShiftFile.clear();
  CStdString strTimeShiftFilePath;
  CStdString strResolvedUrl;

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

  // Try to resolve url
  if (!ResolveHostName(strUrl, strResolvedUrl))
    strResolvedUrl = strUrl;

  // Try to get timeshift file, if failed - try to find pvr.mediaportal.tvserver 
  // addon user name and get timeshift info again.
  bReturn = ConvertRtspStreamUrlToTimeShiftFilePath(strResolvedUrl, strTimeShiftFilePath);
  if (!bReturn)
  {
    // Find pvr.mediaportal.tvserver addon user name
    bReturn = SendCommandToMPTVServer("GetUserName:TimeshiftingUsers\n", strResponse);
    if (bReturn)
    {
      // Find TimeShift file path
      std::vector<std::string> timeshiftingUserNames;
      StringUtils::Tokenize(strResponse, timeshiftingUserNames, ",");
      bReturn = false;
      for (unsigned int i = 0; i < timeshiftingUserNames.size(); i++)
      {
        if (SendCommandToMPTVServer("SetUserName:" + timeshiftingUserNames[i] + "\n", strResponse))
        {
          if (ConvertRtspStreamUrlToTimeShiftFilePath(strResolvedUrl, strTimeShiftFilePath))
          {
            strTimeShiftFile = strTimeShiftFilePath;
            CLog::Log(LOGNOTICE, "%s MediaPortal TVServer user name found: %s ", __FUNCTION__, timeshiftingUserNames[i].c_str());
            bReturn = true;
            break;
          }
        }
        else
        {
          CLog::Log(LOGERROR, "%s Failed to MediaPortal TVServer failed to Set User Name: %s", __FUNCTION__, timeshiftingUserNames[i].c_str());
          break;
        }
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s Failed to get Timeshifting User names from MediaPortal TVServer.", __FUNCTION__);
    }
  }

  if (bReturn)
  {
    CStdString strUNCPath;
    if (TranslatePathToUNC(strTimeShiftFilePath, strUNCPath))
      strTimeShiftFilePath = strUNCPath;

    bReturn = IsFileExistAndAccessible(strTimeShiftFilePath);
  }

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

bool  CDSMediaPortal::GetRecordingStreamURL(const CStdString& strRecordingId, CStdString& strRecordingUrl, bool bGetUNCPath /* = false */)
{
  bool bReturn = false;
  CStdString strResponse;
  CStdString strRecUrl;
  strRecordingUrl.clear();

  if (strRecordingId.empty())
  {
    CLog::Log(LOGERROR, "%s Recording Id is not valid.", __FUNCTION__);
    return false;
  }

  // GetRecordingInfo with RTSP url  (TVServerXBMC v1.1.0.90 or higher)
  bReturn = SendCommandToMPTVServer("GetRecordingInfo:" + strRecordingId + "|True\n", strResponse);
  CLog::Log(LOGDEBUG, "%s Response message: %s", __FUNCTION__, strResponse.c_str());
  /*
   * TVServerXBMC serverinterface.cs
   * XBMC pvr side:
   * index, channelname, lifetime, priority, start time, duration
   * title, subtitle, description, stream_url, directory
   *
   * [0] index / mediaportal recording id
   * [1] start time
   * [2] end time
   * [3] channel name
   * [4] title
   * [5] description
   * [6] stream_url (resolved hostname if resolveHostnames = True)
   * [7] filename (we can bypass rtsp streaming when XBMC and the TV server are on the same machine)
   * [8] lifetime (mediaportal keep until?)
   * [9] original unresolved stream_url if resolveHostnames = True, otherwise this field is missing
  */
  if (bReturn && !strResponse.empty() && strResponse.find("[ERROR]") == std::string::npos)
  {
    bReturn = false;
    // Make sure that the empty fields will not be ignored by "Tokenize" function.
    strResponse.Replace("||", "|_|");
    std::vector<std::string> recordingInfo;
    StringUtils::Tokenize(strResponse, recordingInfo, "|");
    if (recordingInfo.size() >= 8)
    {
      if (bGetUNCPath)
      {    
        // Recording UNC Path 
        strRecUrl = recordingInfo[7];
        CStdString strUNCPath;
        if (TranslatePathToUNC(strRecUrl, strUNCPath))
          strRecUrl = strUNCPath;

        if (CURL::IsFullPath(strRecUrl) && IsFileExistAndAccessible(strRecUrl))
          bReturn = true;
      }
      else
      {
        // Recording RTSP url 
        strRecUrl = recordingInfo[6]; 
        if (URIUtils::IsInternetStream(strRecUrl))
        {
          CStdString strResolvedRecUrl;
          if (ResolveHostName(strRecUrl, strResolvedRecUrl))
            strRecUrl = strResolvedRecUrl;
          bReturn = true;
        }
      }
    } 
    else
    {
      CLog::Log(LOGDEBUG, "%s PVR Backend response message is invalid.", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s Failed to get recording info from PVR Backend, recording id: %s", __FUNCTION__, strRecordingId.c_str());
  }

  if (bReturn)
  {
    CLog::Log(LOGNOTICE, "%s Recording Stream URL: %s", __FUNCTION__, strRecUrl.c_str());
    strRecordingUrl = strRecUrl;
  }
  else
  {
    CLog::Log(LOGERROR, "%s Failed to get recording stream URL from PVR Backend.", __FUNCTION__);
  }

  return bReturn;
}

bool CDSMediaPortal::SendCommandToMPTVServer(const CStdString& strCommand, CStdString & strResponse)
{
  bool bReturn = true;
  strResponse.clear();
  CStdString strResponseMessage;

  if (!TCPClientIsConnected())
    bReturn = ConnectToMPTVServer();
    
  if (bReturn)
    bReturn = TCPClientSendCommand(strCommand, strResponseMessage);

  if (bReturn)
    strResponse = strResponseMessage;
  
  return bReturn;
}

bool CDSMediaPortal::ConnectToMPTVServer()
{ 
  bool bReturn = false;
  CStdString strResponseMessage;
  
  bReturn = TCPClientConnect();
  if (bReturn)
  {
    // Send connect message to mp tvserver
    bReturn = TCPClientSendCommand("PVRclientXBMC:0-1\n", strResponseMessage);
    if (bReturn)
    {      
      if (strResponseMessage.length() == 0)
        bReturn = false;
      
      if (bReturn && strResponseMessage.find("Unexpected protocol") != std::string::npos)
      {
        CLog::Log(LOGERROR, "%s TVServer does not accept protocol: PVRclientXBMC:0-1", __FUNCTION__);
        bReturn = false;
      }
    }
  }
  
  if (bReturn)
    LoadCardSettings();
  else
    CLog::Log(LOGERROR, "%s Failed to connect to MediaPortal TVServer!", __FUNCTION__);

  return bReturn;
}

bool CDSMediaPortal::ConvertRtspStreamUrlToTimeShiftFilePath(const CStdString& strUrl, CStdString& strTimeShiftFile)
{
  /*
  "True|rtsp://HostName:554/stream5.2|\\\\HOSTNAME-PC\\Timeshift\\live5-2.ts.tsbuffer|ChannelInfo" - new version contains UNC timeshift path
  "True|rtsp://HostName:554/stream5.2|ChannelInfo"                                                 - old version without UNC timeshift path
  */

  bool bReturn = false;
  strTimeShiftFile.clear();
  CStdString strResponse;
  CStdString strRtspUrl;
  CStdString strResolvedRtspUrl;

  bReturn = SendCommandToMPTVServer("IsTimeshifting:\n", strResponse);
  if (bReturn && strResponse.find("True") != std::string::npos)
  {
    // Find TimeShift file path
    std::vector<std::string> tokens;
    StringUtils::Tokenize(strResponse, tokens, "|");
    bReturn = false;
        
    if (tokens.size() > 2 && tokens[0] == "True" )
    {
      strRtspUrl = tokens[1];
      // From TVServerKodi: 
      // "Workaround for MP TVserver bug when using default port for rtsp => returns rtsp://ip:0"
      strRtspUrl.Replace(":554", "");
      strRtspUrl.Replace(":0", "");

      if (!ResolveHostName(strRtspUrl, strResolvedRtspUrl))
        strResolvedRtspUrl = "";
      
      if (strRtspUrl == strUrl || strResolvedRtspUrl == strUrl)
      {
        if (tokens[2].find(".ts.tsbuffer") != std::string::npos)
        {
          strTimeShiftFile = tokens[2];
          bReturn = true;
        }
      }
    }
  }
  else
  {
    bReturn = false;
    CLog::Log(LOGDEBUG, "%s Failed to get Timeshifting info from the MediaPortal TVServer", __FUNCTION__);
  }
  
  if (bReturn)
    CLog::Log(LOGDEBUG, "%s Timeshift File found: %s", __FUNCTION__, strTimeShiftFile.c_str());

  return bReturn;
}

bool CDSMediaPortal::LoadCardSettings()
{
  bool bReturn = false;
  CStdString strResponseMessage;

  CLog::Log(LOGDEBUG, "%s Loading card settings from the MediaPortal TVServer", __FUNCTION__);
  
  // Retrieve card settings (needed for Live TV and recordings folders)
  bReturn = TCPClientSendCommand("GetCardSettings\n", strResponseMessage);
  if (bReturn)
  {
    if (strResponseMessage.length() == 0)
      bReturn = false;

    if (bReturn && strResponseMessage.find("[ERROR]:") != std::string::npos)
    {
      CLog::Log(LOGERROR, "%s TVServerXBMC error: %s", __FUNCTION__);
      bReturn = false;
    }
  }

  if (bReturn)
  {
    vector<string> lines;
    StringUtils::Tokenize(strResponseMessage, lines, ",");
    if (!m_pCardsSettings) 
      m_pCardsSettings = new CDSMediaPortalCards;
    bReturn = m_pCardsSettings->ParseLines(lines);
  }

  if (bReturn)
    CLog::Log(LOGDEBUG, "%s Card settings successfully loaded.", __FUNCTION__);
  else
    CLog::Log(LOGERROR, "%s Failed to load card settings.", __FUNCTION__);

  return bReturn;
}

bool CDSMediaPortal::TranslatePathToUNC(const CStdString& strLocalFilePath, CStdString& strTranslatedFilePath)
{
  CStdString strFilePath = strLocalFilePath;
  strTranslatedFilePath.clear();

  // Can we access the given file already?
  if (CDSFile::Exists(strFilePath, NULL))
  {
    CLog::Log(LOGDEBUG, "%s Found the file at: %s", __FUNCTION__, strFilePath.c_str());
    strTranslatedFilePath = strLocalFilePath;
    return true;
  }

  CLog::Log(LOGDEBUG, "%s Cannot access '%s' directly. Assuming multiseat mode. Need to translate to UNC filename.", __FUNCTION__, strFilePath.c_str());
  
  if (!m_pCardsSettings || m_pCardsSettings->size() <= 0)
  {
    CLog::Log(LOGERROR, "%s Cards Settings not found", __FUNCTION__);
    return false;
  }

  bool bFound = false;
  // Path to tsbuffer? (only for Live TV / Radio). Check for an UNC path (e.g. \\tvserver\timeshift)
  if (StringUtils::EndsWithNoCase(strFilePath, ".ts.tsbuffer"))
  {
    for (CDSMediaPortalCards::iterator it = m_pCardsSettings->begin(); it < m_pCardsSettings->end(); ++it)
    {
      // Determine whether the first part of the timeshift filename is shared with this card
      if (StringUtils::StartsWith(strFilePath, it->TimeshiftFolder))
      {
        if (!it->TimeshiftFolderUNC.empty())
        {
          // Remove the original base path and replace it with the given path
          strFilePath.Replace(it->TimeshiftFolder.c_str(), it->TimeshiftFolderUNC.c_str());
          bFound = true;
          break;
        }
      }
    }
  }
  else
  {
    // This is a recording. Check for an UNC path (e.g. \\tvserver\recordings)
    for (CDSMediaPortalCards::iterator it = m_pCardsSettings->begin(); it < m_pCardsSettings->end(); ++it)
    {
      // Determine whether the first part of the recording filename is shared with this card
      if (StringUtils::StartsWith(strFilePath, it->RecordingFolder))
      {
        if (!it->RecordingFolderUNC.empty())
        {
          // Remove the original base path and replace it with the given path
          strFilePath.Replace(it->RecordingFolder.c_str(), it->RecordingFolderUNC.c_str());
          bFound = true;
          break;
        }
      }
    }    
  }

  if (bFound)
  {
    CLog::Log(LOGDEBUG, "%s Translate path %s -> %s", __FUNCTION__, strLocalFilePath.c_str(), strFilePath.c_str());
    strTranslatedFilePath = strFilePath;
  }
  else
  {
    CLog::Log(LOGERROR, "%s Could not find a network share for '%s'. Check your TVServerXBMC settings!", __FUNCTION__, strLocalFilePath.c_str());
  }

  return bFound;
}


bool CDSMediaPortalCards::ParseLines(vector<string>& lines)
{
  if (lines.empty())
  {
    CLog::Log(LOGERROR, "%s No card settings found.", __FUNCTION__);
    return false;
  }

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); ++it)
  {
    string data = *it;

    if (!data.empty())
    {
      vector<string> fields;
      MPCard card;

      uri::decode(data);
      StringUtils::Tokenize(data, fields, "|");

      // field 0 = idCard
      // field 1 = devicePath
      // field 2 = name
      // field 3 = priority
      // field 4 = grabEPG
      // field 5 = lastEpgGrab (2000-01-01 00:00:00 = infinite)
      // field 6 = recordingFolder
      // field 7 = idServer
      // field 8 = enabled
      // field 9 = camType
      // field 10 = timeshiftingFolder
      // field 11 = recordingFormat
      // field 12 = decryptLimit
      // field 13 = preload
      // field 14 = CAM
      // field 15 = NetProvider
      // field 16 = stopgraph
      // field 17 = UNC path recording folder (when shared)
      // field 18 = UNC path timeshift folder (when shared)
      if (fields.size() < 17)
        return false;

      card.IdCard = atoi(fields[0].c_str());
      card.DevicePath = fields[1];
      card.Name = fields[2];
      card.Priority = atoi(fields[3].c_str());
      card.GrabEPG = (CStdString(fields[4]).MakeLower().compare("false") == 0) ? false : true;
      card.LastEpgGrab.SetFromDateString(fields[5]);
      card.RecordingFolder = fields[6];
      card.IdServer = atoi(fields[7].c_str());
      card.Enabled = (CStdString(fields[8]).MakeLower().compare("false") == 0) ? false : true;
      card.CamType = atoi(fields[9].c_str());
      card.TimeshiftFolder = fields[10];
      card.RecordingFormat = atoi(fields[11].c_str());
      card.DecryptLimit = atoi(fields[12].c_str());
      card.Preload = (CStdString(fields[13]).MakeLower().compare("false") == 0) ? false : true;
      card.CAM = (CStdString(fields[14]).MakeLower().compare("false") == 0) ? false : true;
      card.NetProvider = atoi(fields[15].c_str());
      card.StopGraph = (CStdString(fields[16]).MakeLower().compare("false") == 0) ? false : true;

      if (fields.size() >= 19) // since TVServerXBMC build 115
      {
        card.RecordingFolderUNC = fields[17];
        card.TimeshiftFolderUNC = fields[18];
        if (card.RecordingFolderUNC.empty())
        {
          CLog::Log(LOGNOTICE, "%s Warning: no recording share defined in the TVServerXBMC settings for card '%s'", __FUNCTION__, card.Name.c_str());
        }
        if (card.TimeshiftFolderUNC.empty())
        {
          CLog::Log(LOGNOTICE, "Warning: no timeshift share defined in the TVServerXBMC settings for card '%s'", __FUNCTION__, card.Name.c_str());
        }
      }
      else
      {
        card.RecordingFolderUNC = "";
        card.TimeshiftFolderUNC = "";
      }

      push_back(card);
    }
  }

  return true;
}

bool CDSMediaPortalCards::GetCard(int id, MPCard& card)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IdCard == id)
    {
      card = at(i);
      return true;
    }
  }

  card.IdCard = -1;
  return false;
}

#endif
