/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "client.h"
#include "timers.h"
#include "channels.h"
#include "recordings.h"
#include "epg.h"
#include "utils.h"
#include "pvrclient-mediaportal.h"
#include <ctime>
#include <stdio.h>
#include <stdlib.h>

#include "libPlatform/os-dependent.h"

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

using namespace std;

//Globals
int g_iTVServerXBMCBuild = 0;

/* TVServerXBMC plugin supported versions */
#define TVSERVERXBMC_MIN_VERSION_STRING         "1.1.0.70"
#define TVSERVERXBMC_MIN_VERSION_BUILD          70
#define TVSERVERXBMC_RECOMMENDED_VERSION_STRING "1.1.x.104"
#define TVSERVERXBMC_RECOMMENDED_VERSION_BUILD  104

/************************************************************/
/** Class interface */

cPVRClientMediaPortal::cPVRClientMediaPortal()
{
  m_iCurrentChannel        = 1;
  m_tcpclient              = new MPTV::Socket(MPTV::af_inet, MPTV::pf_inet, MPTV::sock_stream, MPTV::tcp);
  m_bConnected             = false;
  m_bStop                  = true;
  m_bTimeShiftStarted      = false;
  m_BackendUTCoffset       = 0;
  m_BackendTime            = 0;
  m_bStop                  = true;
}

cPVRClientMediaPortal::~cPVRClientMediaPortal()
{
  XBMC->Log(LOG_DEBUG, "->~cPVRClientMediaPortal()");
  if (m_bConnected)
    Disconnect();
  delete_null(m_tcpclient);
}

string cPVRClientMediaPortal::SendCommand(string command)
{
  int code;
  vector<string> lines;

  if ( !m_tcpclient->send(command) )
  {
    if ( !m_tcpclient->is_valid() ) {
      // Connection lost, try to reconnect
      if ( Connect() ) {
        // Resend the command
        if (!m_tcpclient->send(command))
        {
          XBMC->Log(LOG_ERROR, "SendCommand('%s') failed.", command.c_str());
          return "";
        }
      }
    }
  }

  string response;
  if ( !m_tcpclient->ReadResponse(code, lines) )
  {
    XBMC->Log(LOG_ERROR, "SendCommand - Failed with code: %d (%s)", code, lines[lines.size()-1].c_str());
  } //else
  //{
  //  XBMC->Log(LOG_DEBUG, "cPVRClientMediaPortal::SendCommand('%s') response: %s", command.c_str(), lines[lines.size()-1].c_str());
  //}
  return lines[lines.size()-1].c_str();
}

bool cPVRClientMediaPortal::SendCommand2(string command, int& code, vector<string>& lines)
{
  if ( !m_tcpclient->send(command) )
  {
    if ( !m_tcpclient->is_valid() )
    {
      // Connection lost, try to reconnect
      if ( Connect() )
      {
        // Resend the command
        if (!m_tcpclient->send(command))
        {
          XBMC->Log(LOG_ERROR, "SendCommand2('%s') failed.", command.c_str());
          return false;
        }
      }
    }
  }

  if (!m_tcpclient->ReadResponse(code, lines))
  {
    XBMC->Log(LOG_ERROR, "SendCommand - Failed with code: %d (%s)", code, lines[lines.size()-1].c_str());
    return false;
  } else {
    string result = lines[lines.size()-1];
    lines.clear();
    //XBMC->Log(LOG_DEBUG, "cPVRClientMediaPortal::SendCommand('%s') response: %s", command.c_str(), result.c_str());

    Tokenize(result, lines, ",");

    return true;
  }
}

bool cPVRClientMediaPortal::Connect()
{
  string result;

  /* Open Connection to MediaPortal Backend TV Server via the XBMC TV Server plugin */
  XBMC->Log(LOG_INFO, "Connecting to %s:%i", g_szHostname.c_str(), g_iPort);

  if (!m_tcpclient->create())
  {
    XBMC->Log(LOG_ERROR, "Could not connect create socket");
    return false;
  }

  if (!m_tcpclient->connect(g_szHostname, g_iPort))
  {
    XBMC->Log(LOG_ERROR, "Could not connect to MPTV backend");
    return false;
  }

  m_tcpclient->set_non_blocking(1);
  XBMC->Log(LOG_INFO, "Connected to %s:%i", g_szHostname.c_str(), g_iPort);

  result = SendCommand("PVRclientXBMC:0-1\n");

  if(result.find("Unexpected protocol") != std::string::npos)
  {
    XBMC->Log(LOG_ERROR, "TVServer does not accept protocol: PVRclientXBMC:0-1");
    return false;
  } else {
    vector<string> fields;
    int major = 0, minor = 0, revision = 0;
    int count = 0;

    // Check the version of the TVServerXBMC plugin:
    Tokenize(result, fields, "|");
    if(fields.size() == 2)
    {
      // Ok, this TVServerXBMC version answers with a version string
      count = sscanf(fields[1].c_str(), "%d.%d.%d.%d", &major, &minor, &revision, &g_iTVServerXBMCBuild);
      if( count < 4 )
      {
        XBMC->Log(LOG_ERROR, "Could not parse the TVServerXBMC version string '%s'", fields[1].c_str());
        return false;
      }
      // Check for the minimal requirement: 1.1.0.70
      if( g_iTVServerXBMCBuild < TVSERVERXBMC_MIN_VERSION_BUILD ) //major < 1 || minor < 1 || revision < 0 || build < 70
      {
        XBMC->Log(LOG_ERROR, "Your TVServerXBMC version v%s is too old. Please upgrade to v%s or higher!", fields[1].c_str(), TVSERVERXBMC_MIN_VERSION_STRING);
        XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30050), fields[1].c_str(), TVSERVERXBMC_MIN_VERSION_STRING);
        return false;
      }
      else
      {
        XBMC->Log(LOG_INFO, "Your TVServerXBMC version is '%s'", fields[1].c_str());
        
        // Advice to upgrade:
        if( g_iTVServerXBMCBuild < TVSERVERXBMC_RECOMMENDED_VERSION_BUILD )
        {
          XBMC->Log(LOG_INFO, "It is adviced to upgrade your TVServerXBMC version v%s to v%s or higher!", fields[1].c_str(), TVSERVERXBMC_RECOMMENDED_VERSION_STRING);
        }
      }
    } else {
      XBMC->Log(LOG_ERROR, "Your TVServerXBMC version is too old. Please upgrade to v%s or higher!", TVSERVERXBMC_MIN_VERSION_STRING);
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30051), TVSERVERXBMC_MIN_VERSION_STRING);
      return false;
    }
  }

  char buffer[512];
  snprintf(buffer, 512, "%s:%i", g_szHostname.c_str(), g_iPort);
  m_ConnectionString = buffer;

  m_bConnected = true;
  return true;
}

void cPVRClientMediaPortal::Disconnect()
{
  string result;

  XBMC->Log(LOG_INFO, "Disconnect");

  if (m_tcpclient->is_valid() && m_bTimeShiftStarted)
  {
    result = SendCommand("IsTimeshifting:\n");

    if (result.find("True") != std::string::npos )
    {
      result = SendCommand("StopTimeshift:\n");
    }
  }

  m_bStop = true;

  m_tcpclient->close();

  m_bConnected = false;
}

/* IsUp()
 * \brief   Check whether we still have a connection with the TVServer. If not, try
 *          to reconnect
 * \return  True when a connection is available, False when even a reconnect failed
 */
bool cPVRClientMediaPortal::IsUp()
{
  if(!m_tcpclient->is_valid())
  {
    if(!Connect()) {
      return false;
    }
  }
  return true;
}

void* cPVRClientMediaPortal::Process(void*)
{
  XBMC->Log(LOG_DEBUG, "->Process() Not yet implemented");
  return NULL;
}


/************************************************************/
/** General handling */

// Used among others for the server name string in the "Recordings" view
const char* cPVRClientMediaPortal::GetBackendName(void)
{
  if (!m_tcpclient->is_valid())
  {
    return g_szHostname.c_str();
  }

  XBMC->Log(LOG_DEBUG, "->GetBackendName()");

  if(m_BackendName.length() == 0)
  {
    m_BackendName = "MediaPortal TV-server (";
    m_BackendName += SendCommand("GetBackendName:\n");
    m_BackendName += ")";
  }

  return m_BackendName.c_str();
}

const char* cPVRClientMediaPortal::GetBackendVersion(void)
{
  if (!IsUp())
    return "0.0";

  XBMC->Log(LOG_DEBUG, "->GetBackendVersion()");

  if(m_BackendVersion.length() == 0)
  {
    m_BackendVersion = SendCommand("GetVersion:\n");
  }

  return m_BackendVersion.c_str();
}

const char* cPVRClientMediaPortal::GetConnectionString(void)
{
  XBMC->Log(LOG_DEBUG, "->GetConnectionString()");

  return m_ConnectionString.c_str();
}

PVR_ERROR cPVRClientMediaPortal::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  string result;
  vector<string> fields;

  *iTotal = 0;
  *iUsed = 0;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if ( g_iTVServerXBMCBuild >= 100)
  {
    result = SendCommand("GetDriveSpace:\n");

    Tokenize(result, fields, "|");

    *iTotal = (long long) atoi(fields[0].c_str());
    *iUsed = (long long) atoi(fields[1].c_str());
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::GetMPTVTime(time_t *localTime, int *gmtOffset)
{
  string result;
  vector<string> fields;
  int year = 0, month = 0, day = 0;
  int hour = 0, minute = 0, second = 0;
  int count = 0;
  struct tm timeinfo;

  //XBMC->Log(LOG_DEBUG, "->GetMPTVTime");

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("GetTime:\n");

  Tokenize(result, fields, "|");

  if(fields.size() >= 3)
  {
    //[0] date + time TV Server
    //[1] UTC offset hours
    //[2] UTC offset minutes
    //From CPVREpg::CPVREpg(): Expected PVREpg GMT offset is in seconds
    m_BackendUTCoffset = ((atoi(fields[1].c_str()) * 60) + atoi(fields[2].c_str())) * 60;

    count = sscanf(fields[0].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if(count == 6)
    {
      //timeinfo = *localtime ( &rawtime );
      XBMC->Log(LOG_DEBUG, "GetMPTVTime: time from MP TV Server: %d-%d-%d %d:%d:%d, offset %d seconds", year, month, day, hour, minute, second, m_BackendUTCoffset );
      timeinfo.tm_hour = hour;
      timeinfo.tm_min = minute;
      timeinfo.tm_sec = second;
      timeinfo.tm_year = year - 1900;
      timeinfo.tm_mon = month - 1;
      timeinfo.tm_mday = day;
      timeinfo.tm_isdst = -1; //Actively determines whether DST is in effect from the specified time and the local time zone.
      // Make the other fields empty:
      timeinfo.tm_wday = 0;
      timeinfo.tm_yday = 0;

      m_BackendTime = mktime(&timeinfo);

      if(m_BackendTime < 0)
      {
        XBMC->Log(LOG_DEBUG, "GetMPTVTime: Unable to convert string '%s' into date+time", fields[0].c_str());
        return PVR_ERROR_SERVER_ERROR;
      }

      XBMC->Log(LOG_DEBUG, "GetMPTVTime: localtime %s", asctime(localtime(&m_BackendTime)));
      XBMC->Log(LOG_DEBUG, "GetMPTVTime: gmtime    %s", asctime(gmtime(&m_BackendTime)));

      *localTime = m_BackendTime;
      *gmtOffset = m_BackendUTCoffset;
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      return PVR_ERROR_SERVER_ERROR;
    }
  } else
    return PVR_ERROR_SERVER_ERROR;
}

/************************************************************/
/** EPG handling */

PVR_ERROR cPVRClientMediaPortal::GetEpg(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  vector<string> lines;
  char           command[256];
  string         result;
  cEpg           epg;
  EPG_TAG        broadcast;
  struct tm      starttime;
  struct tm      endtime;

  starttime = *gmtime( &iStart );
  endtime = *gmtime( &iEnd );
  XBMC->Log(LOG_DEBUG, "->RequestEPGForChannel(%i)", channel.iUniqueId);

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if (g_iTVServerXBMCBuild >= 104)
  {
    // Request (extended) EPG data for the given period
    snprintf(command, 256, "GetEPG:%i|%04d-%02d-%02dT%02d:%02d:%02d.0Z|%04d-%02d-%02dT%02d:%02d:%02d.0Z\n",
            channel.iUniqueId,                                                 //Channel id
            starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date     [2..4]
            starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time     [5..7]
            endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date       [8..10]
            endtime.tm_hour, endtime.tm_min, endtime.tm_sec);                  //End time       [11..13]
  }
  else
  {
    // This version does not yet return all EPG fields
    snprintf(command, 256, "GetEPG:%i\n", channel.iUniqueId);
  }

  result = SendCommand(command);

  if(result.compare(0,5, "ERROR") != 0)
  {
    if( result.length() != 0)
    {
      memset(&broadcast, NULL, sizeof(EPG_TAG));

      Tokenize(result, lines, ",");

      XBMC->Log(LOG_DEBUG, "Found %i EPG items for channel %i\n", lines.size(), channel.iUniqueId);

      for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
      {
        string& data(*it);

        if( data.length() > 0)
        {
          uri::decode(data);

          bool isEnd = epg.ParseLine(data);

          if (isEnd && epg.StartTime() != 0)
          {
            broadcast.iUniqueBroadcastId = epg.UniqueId();
            broadcast.strTitle           = epg.Title();
            broadcast.iChannelNumber     = channel.iChannelNumber;
            broadcast.startTime          = epg.StartTime();
            broadcast.endTime            = epg.EndTime();
            broadcast.strPlotOutline     = epg.ShortText();
            broadcast.strPlot            = epg.Description();
            broadcast.strIconPath        = "";
            broadcast.iGenreType         = epg.GenreType();
            broadcast.iGenreSubType      = epg.GenreSubType();
            //broadcast.genre_text       = epg.Genre();
            broadcast.firstAired         = epg.OriginalAirDate();
            broadcast.iParentalRating    = epg.ParentalRating();
            broadcast.iStarRating        = epg.StarRating();
            broadcast.bNotify            = false;
            broadcast.iSeriesNumber      = epg.SeriesNumber();
            broadcast.iEpisodeNumber     = epg.EpisodeNumber();
            broadcast.iEpisodePartNumber = atoi(epg.EpisodePart());
            broadcast.strEpisodeName     = epg.EpisodeName();

            PVR->TransferEpgEntry(handle, &broadcast);
          }
          epg.Reset();
        }
      }
    } else {
      XBMC->Log(LOG_DEBUG, "No EPG items found for channel %i", channel.iUniqueId);
    }
  } else {
    XBMC->Log(LOG_DEBUG, "RequestEPGForChannel(%i) %s", channel.iUniqueId, result.c_str());
  }

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int cPVRClientMediaPortal::GetNumChannels(void)
{
  string result;
  //CStdString      command;

  if (!IsUp())
    return -1;

  //command.Format("GetChannelCount:%s\n", g_sTVGroup.c_str());
  // Get the total channel count (radio+tv)
  // It is only used to check whether XBMC should request the channel list
  result = SendCommand("GetChannelCount:\n");

  return atol(result.c_str());
}

PVR_ERROR cPVRClientMediaPortal::GetChannels(PVR_HANDLE handle, bool bRadio)
{
  vector<string>  lines;
  CStdString      command;
  int             code;
  PVR_CHANNEL     tag;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if(bRadio)
  {
    XBMC->Log(LOG_DEBUG, "RequestChannelList for Radio group:%s", g_szRadioGroup.c_str());
    command.Format("ListRadioChannels:%s\n", uri::encode(uri::PATH_TRAITS, g_szRadioGroup).c_str());
  } else {
    XBMC->Log(LOG_DEBUG, "RequestChannelList for TV group:%s", g_szTVGroup.c_str());
    command.Format("ListTVChannels:%s\n", uri::encode(uri::PATH_TRAITS, g_szTVGroup).c_str());
  }
  SendCommand2(command.c_str(), code, lines);

  memset(&tag, NULL, sizeof(PVR_CHANNEL));

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0) {
      if(bRadio)
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. Empty/non existing radio group '%s'?", g_szRadioGroup.c_str());
      else
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. Empty/non existing tv group '%s'?", g_szTVGroup.c_str());
      break;
    }

    uri::decode(data);
    //if(bRadio) {
    //  XBMC->Log(LOG_DEBUG, "Radio channel: %s", data.c_str() );
    //} else {
    //  XBMC->Log(LOG_DEBUG, "TV channel: %s", data.c_str() );
    //}

    cChannel channel;
    if( channel.Parse(data) )
    {
      tag.iUniqueId = channel.UID();
      tag.iChannelNumber = g_iTVServerXBMCBuild >= 102 ? channel.ExternalID() : channel.UID();
      tag.strChannelName = channel.Name();
      tag.strIconPath = "";
      tag.iEncryptionSystem = channel.Encrypted();
      tag.bIsRadio = bRadio; //TODO:(channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
      tag.bIsHidden = false;
//      tag.bIsRecording = false;

      if(channel.IsWebstream())
      {
        tag.strStreamURL = channel.URL();
      }
      else
      {
        //Use GetLiveStreamURL to fetch an rtsp stream
        if(bRadio)
          tag.strStreamURL = "pvr://stream/radio/%i.ts"; //stream.c_str();
        else
          tag.strStreamURL = "pvr://stream/tv/%i.ts"; //stream.c_str();
      }
      tag.strInputFormat = "";

      if( (!g_bOnlyFTA) || (tag.iEncryptionSystem==0))
      {
        PVR->TransferChannelEntry(handle, &tag);
      }
    }
  }

  //pthread_mutex_unlock(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Channel group handling **/

int cPVRClientMediaPortal::GetChannelGroupsAmount(void)
{
  // Not directly possible at the moment
  XBMC->Log(LOG_DEBUG, "GetChannelGroupsAmount: TODO");

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  // just tell XBMC that we have groups
  return 1;
  //return -1; // not implemented
}

PVR_ERROR cPVRClientMediaPortal::GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  vector<string>  lines;
  int code;
  PVR_CHANNEL_GROUP tag;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if(bRadio)
  {
    XBMC->Log(LOG_DEBUG, "GetChannelGroups for radio");
    SendCommand2("ListRadioGroups\n", code, lines);
  } else {
    XBMC->Log(LOG_DEBUG, "RequestChannelList for TV group:%s", g_szTVGroup.c_str());
    SendCommand2("ListRadioGroups\n", code, lines);
  }

  memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0) {
      if(bRadio)
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. No radio groups found?");
      else
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. No TVo groups found?");
      break;
    }

    uri::decode(data);

    tag.bIsRadio = bRadio;
    tag.strGroupName = data.c_str();

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  //TODO: code below is similar to GetChannels code. Refactor and combine...
  vector<string>           lines;
  CStdString               command;
  int                      code;
  PVR_CHANNEL_GROUP_MEMBER tag;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if(group.bIsRadio)
  {
    XBMC->Log(LOG_DEBUG, "%s: for group '%s', radio=%i", __FUNCTION__, group.strGroupName, group.bIsRadio);
    command.Format("ListRadioChannels:%s\n", uri::encode(uri::PATH_TRAITS, group.strGroupName).c_str());
  } else {
    XBMC->Log(LOG_DEBUG, "%s: for group '%s', radio=%i", __FUNCTION__, group.strGroupName, group.bIsRadio);
    command.Format("ListTVChannels:%s\n", uri::encode(uri::PATH_TRAITS, group.strGroupName).c_str());
  }
  SendCommand2(command.c_str(), code, lines);

  memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0) {
      if(group.bIsRadio)
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. Empty/non existing radio group '%s'?", g_szRadioGroup.c_str());
      else
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. Empty/non existing tv group '%s'?", g_szTVGroup.c_str());
      break;
    }

    uri::decode(data);

    cChannel channel;
    if( channel.Parse(data) )
    {
      tag.iChannelUniqueId = channel.UID();
      tag.iChannelNumber = g_iTVServerXBMCBuild >= 102 ? channel.ExternalID() : channel.UID();
      tag.strGroupName = group.strGroupName;

      XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
        __FUNCTION__, channel.Name(), tag.iChannelUniqueId, group.strGroupName, channel.UID());

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Record handling **/

int cPVRClientMediaPortal::GetNumRecordings(void)
{
  string            result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("GetRecordingCount:\n");

  return atol(result.c_str());
}

PVR_ERROR cPVRClientMediaPortal::GetRecordings(PVR_HANDLE handle)
{
  vector<string>  lines;
  string          result;
  PVR_RECORDING   tag;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if(g_bResolveRTSPHostname == false)
  {
    result = SendCommand("ListRecordings:False\n");
  }
  else
  {
    result = SendCommand("ListRecordings\n");
  }

  Tokenize(result, lines, ",");

  memset(&tag, NULL, sizeof(PVR_RECORDING));

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    uri::decode(data);

    XBMC->Log(LOG_DEBUG, "RECORDING: %s", data.c_str() );

    ///* Convert to UTF8 string format */
    //if (m_bCharsetConv)
    //  XBMC_unknown_to_utf8(str_result);

    cRecording recording;
    if (recording.ParseLine(data))
    {
      tag.iClientIndex   = recording.Index();
      tag.strTitle       = recording.Title();
      tag.strDirectory   = ""; //used in XBMC as directory structure below "Server X - hostname"
      tag.strPlotOutline = recording.Description();
      tag.strPlot        = tag.strTitle;
      tag.strChannelName = recording.ChannelName();
      tag.recordingTime  = recording.StartTime();
      tag.iDuration      = (int) recording.Duration();
      tag.iPriority      = 0; //TODO? recording.Priority();
      tag.iLifetime      = MAXLIFETIME; //TODO: recording.Lifetime();
      tag.iGenreType     = 0; //TODO?
      tag.iGenreSubType  = 0; //TODO?

      if (g_bUseRecordingsDir == true)
      { //Replace path by given path in g_szRecordingsDir
        if (g_szRecordingsDir.length() > 0)
        {
          recording.SetDirectory(g_szRecordingsDir);
          tag.strStreamURL  = recording.FilePath();
        }
        else
        {
          tag.strStreamURL  = recording.FilePath();
        }
      }
      else
      {
        tag.strStreamURL    = recording.Stream();
      }

      PVR->TransferRecordingEntry(handle, &tag);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::DeleteRecording(const PVR_RECORDING &recording)
{
  char            command[256];
  string          result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 256, "DeleteRecordedTV:%i\n", recording.iClientIndex);

  result = SendCommand(command);

  if(result.find("True") ==  string::npos)
  {
    return PVR_ERROR_NOT_DELETED;
  }

  // Although XBMC initiates the deletion of this recording, we still have to trigger XBMC to update its
  // recordings list to remove the recording at the XBMC side
  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::RenameRecording(const PVR_RECORDING &recording)
{
  char           command[512];
  string         result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 512, "UpdateRecording:%i|%s\n",
    recording.iClientIndex,
    uri::encode(uri::PATH_TRAITS, recording.strTitle).c_str());

  result = SendCommand(command);

  if(result.find("True") == string::npos)
  {
    XBMC->Log(LOG_DEBUG, "RenameRecording(%i) to %s [failed]", recording.iClientIndex, recording.strTitle);
    return PVR_ERROR_NOT_DELETED;
  }
  XBMC->Log(LOG_DEBUG, "RenameRecording(%i) to %s [done]", recording.iClientIndex, recording.strTitle);

  // Although XBMC initiates the rename of this recording, we still have to trigger XBMC to update its
  // recordings list to see the renamed recording at the XBMC side
  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Timer handling */

int cPVRClientMediaPortal::GetNumTimers(void)
{
  string            result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("GetScheduleCount:\n");

  return atol(result.c_str());
}

PVR_ERROR cPVRClientMediaPortal::GetTimers(PVR_HANDLE handle)
{
  vector<string>  lines;
  string          result;
  PVR_TIMER       tag;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("ListSchedules:\n");

  if(result.length() > 0)
  {
    Tokenize(result, lines, ",");

    memset(&tag, 0, sizeof(PVR_TIMER));

    for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
    {
      string& data(*it);
      uri::decode(data);

      XBMC->Log(LOG_DEBUG, "SCHEDULED: %s", data.c_str() );

      cTimer timer;

      if(timer.ParseLine(data.c_str()) == true)
      {
        timer.GetPVRtimerinfo(tag);
        PVR->TransferTimerEntry(handle, &tag);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::GetTimerInfo(unsigned int timernumber, PVR_TIMER &timerinfo)
{
  string         result;
  char           command[256];

  XBMC->Log(LOG_DEBUG, "->GetTimerInfo(%i)", timernumber);

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 256, "GetScheduleInfo:%i\n", timernumber);

  result = SendCommand(command);

  cTimer timer;
  if( timer.ParseLine(result.c_str()) == false )
  {
    XBMC->Log(LOG_DEBUG, "GetTimerInfo(%i) parsing server response failed. Response: %s", timernumber, result.c_str());
    return PVR_ERROR_SERVER_ERROR;
  }

  timer.GetPVRtimerinfo(timerinfo);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::AddTimer(const PVR_TIMER &timerinfo)
{
  string         result;

#ifdef _TIME32_T_DEFINED
  XBMC->Log(LOG_DEBUG, "->AddTimer Channel: %i, starttime: %i endtime: %i program: %s", timerinfo.iClientChannelUid, timerinfo.startTime, timerinfo.endTime, timerinfo.strTitle);
#else
  XBMC->Log(LOG_DEBUG, "->AddTimer Channel: %i, 64 bit times not yet supported!", timerinfo.iClientChannelUid);
#endif

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  cTimer timer(timerinfo);

  result = SendCommand(timer.AddScheduleCommand());

  if(result.find("True") ==  string::npos)
  {
    XBMC->Log(LOG_DEBUG, "AddTimer for channel: %i [failed]", timerinfo.iClientChannelUid);
    return PVR_ERROR_NOT_SAVED;
  }
  XBMC->Log(LOG_DEBUG, "AddTimer for channel: %i [done]", timerinfo.iClientChannelUid);

  // Although XBMC adds this timer, we still have to trigger XBMC to update its timer list to
  // see this new timer at the XBMC side
  PVR->TriggerTimerUpdate();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  char           command[256];
  string         result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 256, "DeleteSchedule:%i\n",timer.iClientIndex);

  if (timer.iClientIndex == -1)
  {
    XBMC->Log(LOG_DEBUG, "DeleteTimer: schedule index = -1", timer.iClientIndex);
    return PVR_ERROR_NOT_DELETED;
  } else {
    XBMC->Log(LOG_DEBUG, "DeleteTimer: About to delete MediaPortal schedule index=%i", timer.iClientIndex);
    result = SendCommand(command);

    if(result.find("True") ==  string::npos)
    {
      XBMC->Log(LOG_DEBUG, "DeleteTimer %i [failed]", timer.iClientIndex);
      return PVR_ERROR_NOT_DELETED;
    }
    XBMC->Log(LOG_DEBUG, "DeleteTimer %i [done]", timer.iClientIndex);

  }

  // Although XBMC deletes this timer, we still have to trigger XBMC to update its timer list to
  // remove the timer from the XBMC list
  PVR->TriggerTimerUpdate();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::UpdateTimer(const PVR_TIMER &timerinfo)
{
  string         result;

#ifdef _TIME32_T_DEFINED
  XBMC->Log(LOG_DEBUG, "->UpdateTimer Index: %i Channel: %i, starttime: %i endtime: %i program: %s", timerinfo.iClientIndex, timerinfo.iClientChannelUid, timerinfo.startTime, timerinfo.endTime, timerinfo.strTitle);
#else
  XBMC->Log(LOG_DEBUG, "->UpdateTimer Channel: %i, 64 bit times not yet supported!", timerinfo.iClientChannelUid);
#endif

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  cTimer timer(timerinfo);

  result = SendCommand(timer.UpdateScheduleCommand());
  if(result.find("True") ==  string::npos)
  {
    XBMC->Log(LOG_DEBUG, "UpdateTimer for channel: %i [failed]", timerinfo.iClientChannelUid);
    return PVR_ERROR_NOT_SAVED;
  }
  XBMC->Log(LOG_DEBUG, "UpdateTimer for channel: %i [done]", timerinfo.iClientChannelUid);

  // Although XBMC changes this timer, we still have to trigger XBMC to update its timer list to
  // see the timer changes at the XBMC side
  PVR->TriggerTimerUpdate();

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */

// The MediaPortal TV Server uses rtsp streams which XBMC can handle directly
// via the dvdplayer (ffmpeg) so we don't need to open the streams in this
// pvr addon.
// However, we still need to request the stream URL for the channel we want
// to watch as it is not known on beforehand.
// Most of the times it is the same URL for each selected channel. Only the
// stream itself changes. Example URL: rtsp://tvserverhost/stream2.0
// The number 2.0 may change when the tvserver is streaming multiple tv channels
// at the same time.
bool cPVRClientMediaPortal::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  // Just return true for now. XBMC will later ask for the stream URL and then we will
  // actually ask MediaPortal for the rtsp URL.
  // TODO: optimization: request the channel stream already here and return it to
  //       XBMC via GetLiveStreamURL
  return true;
}

int cPVRClientMediaPortal::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return 0;
}

void cPVRClientMediaPortal::CloseLiveStream(void)
{
  string result;

  if (!IsUp())
     return;

  if (m_bTimeShiftStarted)
  {
    result = SendCommand("StopTimeshift:\n");
    XBMC->Log(LOG_INFO, "CloseLiveStream: %s", result.c_str());
    m_bTimeShiftStarted = false;
  } else {
    XBMC->Log(LOG_DEBUG, "CloseLiveStream: Nothing to do.");
  }
}


bool cPVRClientMediaPortal::SwitchChannel(const PVR_CHANNEL &channel)
{
  XBMC->Log(LOG_DEBUG, "->SwitchChannel(%i)", channel.iChannelNumber);

  return OpenLiveStream(channel);
}


int cPVRClientMediaPortal::GetCurrentClientChannel()
{
  XBMC->Log(LOG_DEBUG, "->GetCurrentClientChannel");
  return m_iCurrentChannel;
}

PVR_ERROR cPVRClientMediaPortal::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  //XBMC->Log(LOG_DEBUG, "->SignalQuality(): Not yet supported.");

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Record stream handling */
// MediaPortal recordings are also rtsp streams. Main difference here with
// respect to the live tv streams is that the URLs for the recordings
// can be requested on beforehand (done in the TVserverXBMC plugin).
// These URLs are stored in the field PVR_RECORDINGINFO_OLD.stream_url
bool cPVRClientMediaPortal::OpenRecordedStream(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "->OpenRecordedStream(index=%i)", recording.iClientIndex);
  if (!IsUp())
     return false;

  return false;
}

void cPVRClientMediaPortal::CloseRecordedStream(void)
{
  string result;

  if (!IsUp())
     return;

}

int cPVRClientMediaPortal::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return -1;
}

/*
 * \brief Request the stream URL for live tv/live radio.
 * The MediaPortal TV Server will try to open the requested channel for
 * time-shifting and when successful it will start an rtsp:// stream for this
 * channel and return the URL for this stream.
 */
const char* cPVRClientMediaPortal::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  unsigned int channel = channelinfo.iUniqueId;

  string result;
  char   command[256] = "";

  XBMC->Log(LOG_DEBUG, "->GetLiveStreamURL(%i)", channel);
  if (!IsUp())
  {
    return false;
  }

  // Closing existing timeshift streams will be done in the MediaPortal TV
  // Server plugin, so we can request the new channel stream directly without
  // stopping the existing stream

  if(g_bResolveRTSPHostname == false)
  {
    if (g_iTVServerXBMCBuild < 90)
    { //old way
      // RTSP URL may contain a hostname, XBMC will do the IP resolve
      snprintf(command, 256, "TimeshiftChannel:%i|False\n", channel);
    } else {
      //Faster, skip StopTimeShift
      snprintf(command, 256, "TimeshiftChannel:%i|False|False\n", channel);
    }
  }
  else
  {
    if (g_iTVServerXBMCBuild < 90)
    { //old way
      // RTSP URL will always contain an IP address, TVServerXBMC will
      // do the IP resolve
      snprintf(command, 256, "TimeshiftChannel:%i|True\n", channel);
    } else {
      //Faster, skip StopTimeShift
      snprintf(command, 256, "TimeshiftChannel:%i|True|False\n", channel);
    }
  }
  result = SendCommand(command);

  if (result.find("ERROR") != std::string::npos || result.length() == 0)
  {
    XBMC->Log(LOG_ERROR, "Could not stream channel %i. %s", channel, result.c_str());
    if (result.find("[ERROR]: TVServer answer: ") != std::string::npos)
    {
      // Skip first part: "[ERROR]: TVServer answer: "
      XBMC->QueueNotification(QUEUE_ERROR, "TVServer: %s", result.substr(26).c_str());
    }
    else
    {
      // Skip first part: "[ERROR]: "
      XBMC->QueueNotification(QUEUE_ERROR, result.substr(7).c_str());
    }
    return "";
  }
  else
  {
    if (g_iSleepOnRTSPurl > 0)
    {
      XBMC->Log(LOG_DEBUG, "Sleeping %i ms before opening stream: %s", g_iSleepOnRTSPurl, result.c_str());
      usleep(g_iSleepOnRTSPurl * 1000);
    }

    vector<string> timeshiftfields;

    Tokenize(result, timeshiftfields, "|");

    m_PlaybackURL = timeshiftfields[0];
    XBMC->Log(LOG_INFO, "Sending channel stream URL '%s' to XBMC for playback", m_PlaybackURL.c_str());
    m_iCurrentChannel = channel;

    // Check the returned stream URL. When the URL is an rtsp stream, we need
    // to close it again after watching to stop the timeshift.
    // A radio web stream (added to the TV Server) will return the web stream
    // URL without starting a timeshift.
    if(timeshiftfields[0].compare(0,4, "rtsp") == 0)
    {
      m_bTimeShiftStarted = true;
    }
    return m_PlaybackURL.c_str();
  }
}
