/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include <ctime>
#include <stdio.h>
#include <stdlib.h>

#include "os-dependent.h"

#include "client.h"
#include "timers.h"
#include "channels.h"
#include "recordings.h"
#include "epg.h"
#include "utils.h"
#include "pvrclient-mediaportal.h"
#include "SingleLock.h"
#include "lib/tinyxml/tinyxml.h"

#ifdef TSREADER
#include "lib/tsreader/TSReader.h"
#endif
#include "FileUtils.h"

using namespace std;
using namespace ADDON;

/* Globals */
int g_iTVServerXBMCBuild = 0;

/* PVR client version (don't forget to update also the addon.xml and the Changelog.txt files) */
#define PVRCLIENT_MEDIAPORTAL_VERSION_STRING    "1.2.1.109"

/* TVServerXBMC plugin supported versions */
#define TVSERVERXBMC_MIN_VERSION_STRING         "1.1.0.70"
#define TVSERVERXBMC_MIN_VERSION_BUILD          70
#define TVSERVERXBMC_RECOMMENDED_VERSION_STRING "1.1.x.109 or 1.2.1.109"
#define TVSERVERXBMC_RECOMMENDED_VERSION_BUILD  109

/************************************************************/
/** Class interface */

cPVRClientMediaPortal::cPVRClientMediaPortal()
{
  m_iCurrentChannel        = -1;
  m_iCurrentCard           = 0;
  m_tcpclient              = new MPTV::Socket(MPTV::af_inet, MPTV::pf_inet, MPTV::sock_stream, MPTV::tcp);
  m_bConnected             = false;
  m_bStop                  = true;
  m_bTimeShiftStarted      = false;
  m_BackendUTCoffset       = 0;
  m_BackendTime            = 0;
  m_bStop                  = true;
#ifdef TSREADER
  m_noSignalStreamSize     = 0;
  m_noSignalStreamReadPos  = 0;
  m_bPlayingNoSignal       = false;
  m_tsreader               = NULL;
#endif
#ifndef TARGET_WINDOWS
  m_mutex.Initialize(); //workaround for pthread mutex crash.
#endif
}

cPVRClientMediaPortal::~cPVRClientMediaPortal()
{
  XBMC->Log(LOG_DEBUG, "->~cPVRClientMediaPortal()");
  if (m_bConnected)
    Disconnect();
  SAFE_DELETE(m_tcpclient);
}

string cPVRClientMediaPortal::SendCommand(string command)
{
  int code;
  vector<string> lines;
  CSingleLock critsec(m_mutex);

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
  }

  return lines[lines.size()-1].c_str();
}

bool cPVRClientMediaPortal::SendCommand2(string command, int& code, vector<string>& lines)
{
  CSingleLock critsec(m_mutex);

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
  }
  else
  {
    string result = lines[lines.size()-1];
    lines.clear();

    Tokenize(result, lines, ",");

    return true;
  }
}

bool cPVRClientMediaPortal::Connect()
{
  string result;

  /* Open Connection to MediaPortal Backend TV Server via the XBMC TV Server plugin */
  XBMC->Log(LOG_INFO, "Mediaportal pvr addon " PVRCLIENT_MEDIAPORTAL_VERSION_STRING " connecting to %s:%i", g_szHostname.c_str(), g_iPort);

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

  if (result.length() == 0)
    return false;

  if(result.find("Unexpected protocol") != std::string::npos)
  {
    XBMC->Log(LOG_ERROR, "TVServer does not accept protocol: PVRclientXBMC:0-1");
    return false;
  }
  else
  {
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
        XBMC->Log(LOG_ERROR, "Your TVServerXBMC version '%s' is too old. Please upgrade to '%s' or higher!", fields[1].c_str(), TVSERVERXBMC_MIN_VERSION_STRING);
        XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30050), fields[1].c_str(), TVSERVERXBMC_MIN_VERSION_STRING);
        return false;
      }
      else
      {
        XBMC->Log(LOG_INFO, "Your TVServerXBMC version is '%s'", fields[1].c_str());
        
        // Advice to upgrade:
        if( g_iTVServerXBMCBuild < TVSERVERXBMC_RECOMMENDED_VERSION_BUILD )
        {
          XBMC->Log(LOG_INFO, "It is adviced to upgrade your TVServerXBMC version '%s' to '%s' or higher!", fields[1].c_str(), TVSERVERXBMC_RECOMMENDED_VERSION_STRING);
        }
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "Your TVServerXBMC version is too old. Please upgrade to '%s' or higher!", TVSERVERXBMC_MIN_VERSION_STRING);
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30051), TVSERVERXBMC_MIN_VERSION_STRING);
      return false;
    }
  }

  /* Store connection string */
  char buffer[512];
  snprintf(buffer, 512, "%s:%i", g_szHostname.c_str(), g_iPort);
  m_ConnectionString = buffer;

  /* Retrieve card settings (needed for Live TV and recordings folders) */
  if ( g_iTVServerXBMCBuild >= 106 )
  {
    int code;
    vector<string> lines;

    if ( SendCommand2("GetCardSettings\n", code, lines) )
    {
      m_cCards.ParseLines(lines);
    }
  }

  m_bConnected = true;

  // Read the genre string to type/subtype translation file:
  if(g_bReadGenre)
  {
    string sGenreFile = g_szClientPath + PATH_SEPARATOR_CHAR + "resources" + PATH_SEPARATOR_CHAR + "genre_translation.xml";

    LoadGenreXML(sGenreFile);
  }

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
#ifdef TSREADER
      if (m_tsreader)
      {
        m_tsreader->Close();
        SAFE_DELETE(m_tsreader);
      }
#endif
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
    if(!Connect())
    {
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

  if (m_BackendName.length() == 0)
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

  if(m_BackendVersion.length() == 0)
  {
    m_BackendVersion = SendCommand("GetVersion:\n");
  }

  XBMC->Log(LOG_DEBUG, "GetBackendVersion: %s", m_BackendVersion.c_str());

  return m_BackendVersion.c_str();
}

const char* cPVRClientMediaPortal::GetConnectionString(void)
{
  XBMC->Log(LOG_DEBUG, "GetConnectionString: %s", m_ConnectionString.c_str());
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

    if(fields.size() >= 2)
    {
      *iTotal = (long long) atoi(fields[0].c_str());
      *iUsed = (long long) atoi(fields[1].c_str());
    }
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

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("GetTime:\n");

  if (result.length() == 0)
    return PVR_ERROR_SERVER_ERROR;

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
  }
  else
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
      memset(&broadcast, 0, sizeof(EPG_TAG));
      epg.SetGenreMap(&m_genremap);

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
            broadcast.iUniqueBroadcastId  = epg.UniqueId();
            broadcast.strTitle            = epg.Title();
            broadcast.iChannelNumber      = channel.iChannelNumber;
            broadcast.startTime           = epg.StartTime();
            broadcast.endTime             = epg.EndTime();
            broadcast.strPlotOutline      = epg.ShortText();
            broadcast.strPlot             = epg.Description();
            broadcast.strIconPath         = "";
            broadcast.iGenreType          = epg.GenreType();
            broadcast.iGenreSubType       = epg.GenreSubType();
            broadcast.strGenreDescription = epg.Genre();
            broadcast.firstAired          = epg.OriginalAirDate();
            broadcast.iParentalRating     = epg.ParentalRating();
            broadcast.iStarRating         = epg.StarRating();
            broadcast.bNotify             = false;
            broadcast.iSeriesNumber       = epg.SeriesNumber();
            broadcast.iEpisodeNumber      = epg.EpisodeNumber();
            broadcast.iEpisodePartNumber  = atoi(epg.EpisodePart());
            broadcast.strEpisodeName      = epg.EpisodeName();

            PVR->TransferEpgEntry(handle, &broadcast);
          }
          epg.Reset();
        }
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "No EPG items found for channel %i", channel.iUniqueId);
    }
  }
  else
  {
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
  CStdString      stream;
  bool            bCheckForThumbs = false;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if(bRadio)
  {
    if(!g_bRadioEnabled)
    {
      XBMC->Log(LOG_INFO, "Fetching radio channels is disabled.");
      return PVR_ERROR_NO_ERROR;
    }

    if (g_szRadioGroup.length() > 0)
    {
      XBMC->Log(LOG_DEBUG, "GetChannels(radio) for radio group: '%s'", g_szRadioGroup.c_str());
      command.Format("ListRadioChannels:%s\n", uri::encode(uri::PATH_TRAITS, g_szRadioGroup).c_str());
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetChannels(radio) all channels");
      command = "ListRadioChannels\n";
    }
  }
  else
  {
    if (g_szTVGroup.length() > 0)
    {
      XBMC->Log(LOG_DEBUG, "GetChannels(tv) for TV group: '%s'", g_szTVGroup.c_str());
      command.Format("ListTVChannels:%s\n", uri::encode(uri::PATH_TRAITS, g_szTVGroup).c_str());
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetChannels(tv) all channels");
      command = "ListTVChannels\n";
    }
  }

  if( !SendCommand2(command.c_str(), code, lines) )
    return PVR_ERROR_SERVER_ERROR;

#ifdef TARGET_WINDOWS
  /* Check if we can find the MediaPortal channel logo folders on this machine */
  std::string strIconName;
  std::string strThumbPath;
  std::string strProgramData;

  if (OS::GetEnvironmentVariable("PROGRAMDATA", strProgramData) == true)
    strThumbPath = strProgramData + "\\Team MediaPortal\\MediaPortal\\Thumbs\\";
  else
  {
    if (OS::Version() >= OS::WindowsVista)
    {
      /* Windows Vista/7/Server 2008 */
      strThumbPath = "C:\\ProgramData\\Team MediaPortal\\MediaPortal\\Thumbs\\";
    }
    else
    {
      /* Windows XP */
      strThumbPath = "C:\\Documents and Settings\\All Users\\Application Data\\Team MediaPortal\\MediaPortal\\thumbs\\";
    }
  }

  if (bRadio)
    strThumbPath += "Radio\\";
  else
    strThumbPath += "TV\\logos\\";

  bCheckForThumbs = OS::CFile::Exists(strThumbPath);
#endif

  memset(&tag, 0, sizeof(PVR_CHANNEL));

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0)
    {
      if(bRadio)
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. Empty/non existing radio group '%s'?", g_szRadioGroup.c_str());
      else
        XBMC->Log(LOG_DEBUG, "TVServer returned no data. Empty/non existing tv group '%s'?", g_szTVGroup.c_str());
      break;
    }

    uri::decode(data);

    cChannel channel;
    if( channel.Parse(data) )
    {
      tag.iUniqueId = channel.UID();
      tag.iChannelNumber = g_iTVServerXBMCBuild >= 102 ? channel.ExternalID() : channel.UID();
      tag.strChannelName = channel.Name();
#ifdef TARGET_WINDOWS
      if (bCheckForThumbs)
      {
        strIconName = strThumbPath + ToThumbFileName(channel.Name()) + ".png";
        if ( OS::CFile::Exists(strIconName) )
        {
          tag.strIconPath = strIconName.c_str();
        }
        else
        {
          tag.strIconPath = "";
        }
      }
#else
      tag.strIconPath = "";
#endif
      tag.iEncryptionSystem = channel.Encrypted();
      tag.bIsRadio = bRadio; //TODO:(channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
      tag.bIsHidden = false;
//      tag.bIsRecording = false;

#ifndef TSREADER
      if(channel.IsWebstream())
      {
        tag.strStreamURL = channel.URL();
      }
      else
      {
        //Use GetLiveStreamURL to fetch an rtsp stream
        if(bRadio)
          stream.Format("pvr://stream/radio/%i.ts", tag.iUniqueId);
        else
          stream.Format("pvr://stream/tv/%i.ts", tag.iUniqueId);
        tag.strStreamURL = stream.c_str();
      }
      tag.strInputFormat = "";
#else
      if(channel.IsWebstream())
      {
        tag.strStreamURL = channel.URL();
        tag.strInputFormat = "";
      }
      else
      {
        //Use OpenLiveStream to read from the timeshift .ts file or an rtsp stream
        tag.strStreamURL = "";
        tag.strInputFormat = (!bRadio) ? "video/x-mpegts" : "";
      }
#endif
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
    if (g_bRadioEnabled)
    {
      XBMC->Log(LOG_DEBUG, "GetChannelGroups for radio");
      if (!SendCommand2("ListRadioGroups\n", code, lines))
        return PVR_ERROR_SERVER_ERROR;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Skipping GetChannelGroups for radio. Radio support is disabled.");
      return PVR_ERROR_NO_ERROR;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "RequestChannelList for TV group:%s", g_szTVGroup.c_str());
    if (!SendCommand2("ListRadioGroups\n", code, lines))
      return PVR_ERROR_SERVER_ERROR;
  }

  memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0)
    {
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
    if (g_bRadioEnabled)
    {
      XBMC->Log(LOG_DEBUG, "GetChannelGroupMembers: for radio group '%s'", group.strGroupName);
      command.Format("ListRadioChannels:%s\n", uri::encode(uri::PATH_TRAITS, group.strGroupName).c_str());
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Skipping GetChannelGroupMembers for radio. Radio support is disabled.");
      return PVR_ERROR_NO_ERROR;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "GetChannelGroupMembers: for tv group '%s'", group.strGroupName);
    command.Format("ListTVChannels:%s\n", uri::encode(uri::PATH_TRAITS, group.strGroupName).c_str());
  }

  if (!SendCommand2(command.c_str(), code, lines))
    return PVR_ERROR_SERVER_ERROR;

  memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0)
    {
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


      // Don't add encrypted channels when FTA only option is turned on
      if( (!g_bOnlyFTA) || (channel.Encrypted()==false))
      {
        XBMC->Log(LOG_DEBUG, "GetChannelGroupMembers: add channel %s to group '%s' (Backend channel uid=%d, channelnr=%d)",
          channel.Name(), group.strGroupName, tag.iChannelUniqueId, tag.iChannelNumber);
        PVR->TransferChannelGroupMember(handle, &tag);
      }
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

  if( result.length() == 0 )
  {
    XBMC->Log(LOG_DEBUG, "Backend returned no recordings" );
    return PVR_ERROR_NO_ERROR;
  }

  Tokenize(result, lines, ",");

  memset(&tag, 0, sizeof(PVR_RECORDING));

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    uri::decode(data);

    XBMC->Log(LOG_DEBUG, "RECORDING: %s", data.c_str() );

    CStdString strRecordingId;
    cRecording recording;
    recording.SetCardSettings(&m_cCards);

    if (recording.ParseLine(data))
    {
      strRecordingId.Format("%i", recording.Index());

      tag.strRecordingId = strRecordingId.c_str();
      tag.strTitle       = recording.Title();
      tag.strDirectory   = recording.Directory(); // used in XBMC as directory structure below "Recordings"
      tag.strPlotOutline = g_iTVServerXBMCBuild >= 105 ? recording.EpisodeName() : tag.strTitle;
      tag.strPlot        = recording.Description();
      tag.strChannelName = recording.ChannelName();
      tag.recordingTime  = recording.StartTime();
      tag.iDuration      = (int) recording.Duration();
      tag.iPriority      = 0; // only available for schedules, not for recordings
      tag.iLifetime      = recording.Lifetime();
      tag.iGenreType     = 0; //TODO?
      tag.iGenreSubType  = 0; //TODO?

      if (g_bUseRecordingsDir == true)
      {
        // Replace path by given path in g_szRecordingsDir
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
        // Use rtsp url
#ifdef TSREADER
        tag.strStreamURL = "";
#else
        tag.strStreamURL    = recording.Stream();
#endif
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

  snprintf(command, 256, "DeleteRecordedTV:%s\n", recording.strRecordingId);

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

  snprintf(command, 512, "UpdateRecording:%s|%s\n",
    recording.strRecordingId,
    uri::encode(uri::PATH_TRAITS, recording.strTitle).c_str());

  result = SendCommand(command);

  if(result.find("True") == string::npos)
  {
    XBMC->Log(LOG_DEBUG, "RenameRecording(%s) to %s [failed]", recording.strRecordingId, recording.strTitle);
    return PVR_ERROR_NOT_DELETED;
  }
  XBMC->Log(LOG_DEBUG, "RenameRecording(%s) to %s [done]", recording.strRecordingId, recording.strTitle);

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

  if (result.length() > 0)
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

  if (result.length() == 0)
    return PVR_ERROR_SERVER_ERROR;

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

  XBMC->Log(LOG_DEBUG, "DeleteTimer: About to delete MediaPortal schedule index=%i", timer.iClientIndex);
  result = SendCommand(command);

  if(result.find("True") ==  string::npos)
  {
    XBMC->Log(LOG_DEBUG, "DeleteTimer %i [failed]", timer.iClientIndex);
    return PVR_ERROR_NOT_DELETED;
  }
  XBMC->Log(LOG_DEBUG, "DeleteTimer %i [done]", timer.iClientIndex);

  // Although XBMC deletes this timer, we still have to trigger XBMC to update its timer list to
  // remove the timer from the XBMC list
  PVR->TriggerTimerUpdate();
  // When deleting a currently active (recording) timer, we need to refresh also the recording list
  PVR->TriggerRecordingUpdate();

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
  PVR->TriggerRecordingUpdate();

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
//
// The rtsp code from ffmpeg does not function well enough for this addon.
// Therefore the new TSReader version uses the Live555 library here to open rtsp
// urls or it can read directly from the timeshift buffer file.
bool cPVRClientMediaPortal::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  string result;
  char   command[256] = "";
  const char* sResolveRTSPHostname = booltostring(g_bResolveRTSPHostname);
  vector<string> timeshiftfields;

  XBMC->Log(LOG_DEBUG, "->OpenLiveStream(uid=%i)", channelinfo.iUniqueId);
  if (!IsUp())
  {
    m_iCurrentChannel = -1;
    return false;
  }

  if (((int)channelinfo.iUniqueId) == m_iCurrentChannel)
    return true;
  else
    m_iCurrentChannel = -1; // make sure that it is not a valid channel nr in case it will fail lateron

  // Start the timeshift
  if (g_iTVServerXBMCBuild>=90)
  {
    // Use the optimized TimeshiftChannel call (don't stop a running timeshift)
    snprintf(command, 256, "TimeshiftChannel:%i|%s|False\n", channelinfo.iUniqueId, sResolveRTSPHostname);
  }
  else
  {
    // Closing existing timeshift streams will be done in the MediaPortal TV
    // Server plugin, so we can request the new channel stream directly without
    // stopping the existing stream
    snprintf(command, 256, "TimeshiftChannel:%i|%s\n", channelinfo.iUniqueId, sResolveRTSPHostname);
  }
  result = SendCommand(command);

  if (result.find("ERROR") != std::string::npos || result.length() == 0)
  {
    XBMC->Log(LOG_ERROR, "Could not start the timeshift for channel uid=%i. %s", channelinfo.iUniqueId, result.c_str());
    if (g_iTVServerXBMCBuild>=109)
    {
      int tvresult;

      Tokenize(result, timeshiftfields, "|");
      //[0] = string error message
      //[1] = TvResult

      //For TVServer 1.2.1:
      //enum TvResult
      //{
      //  Succeeded = 0, (this is not an error)
      //  AllCardsBusy = 1,
      //  ChannelIsScrambled = 2,
      //  NoVideoAudioDetected = 3,
      //  NoSignalDetected = 4,
      //  UnknownError = 5,
      //  UnableToStartGraph = 6,
      //  UnknownChannel = 7,
      //  NoTuningDetails = 8,
      //  ChannelNotMappedToAnyCard = 9,
      //  CardIsDisabled = 10,
      //  ConnectionToSlaveFailed = 11,
      //  NotTheOwner = 12,
      //  GraphBuildingFailed = 13,
      //  SWEncoderMissing = 14,
      //  NoFreeDiskSpace = 15,
      //  NoPmtFound = 16,
      //};

      tvresult = atoi(timeshiftfields[1].c_str());
      // Display one of the localized error messages 30060-30075
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30059 + (int) tvresult));
    }
    else
    {
      if (result.find("[ERROR]: TVServer answer: ") != std::string::npos)
      {
        //Skip first part: "[ERROR]: TVServer answer: "
        XBMC->QueueNotification(QUEUE_ERROR, "TVServer: %s", result.substr(26).c_str());
      }
      else
      {
        //Skip first part: "[ERROR]: "
        XBMC->QueueNotification(QUEUE_ERROR, result.substr(7).c_str());
      }
    }
    m_iCurrentChannel = -1;
    return false;
  }
  else
  {
    Tokenize(result, timeshiftfields, "|");

    //[0] = rtsp url
    //[1] = original (unresolved) rtsp url
    //[2] = timeshift buffer filename
    //[3] = card id (TVServerXBMC build >= 106)

    m_PlaybackURL = timeshiftfields[0];
    XBMC->Log(LOG_INFO, "Channel stream URL: %s, timeshift buffer: %s", m_PlaybackURL.c_str(), timeshiftfields[2].c_str());

    if (g_iSleepOnRTSPurl > 0)
    {
      XBMC->Log(LOG_DEBUG, "Sleeping %i ms before opening stream: %s", g_iSleepOnRTSPurl, timeshiftfields[0].c_str());
      usleep(g_iSleepOnRTSPurl * 1000);
    }

    // Check the returned stream URL. When the URL is an rtsp stream, we need
    // to close it again after watching to stop the timeshift.
    // A radio web stream (added to the TV Server) will return the web stream
    // URL without starting a timeshift.
    if(timeshiftfields[0].compare(0,4, "rtsp") == 0)
    {
      m_bTimeShiftStarted = true;
    }

#ifdef TSREADER
    if (m_tsreader != NULL)
    {
      if (g_iTVServerXBMCBuild >=90 )
      { // Continue with the existing TsReader.
        XBMC->Log(LOG_INFO, "Re-using existing TsReader...");
#ifdef TARGET_WINDOWS
        if(g_bDirectTSFileRead)
        {
          // Timeshift buffer
          return m_tsreader->OnZap(timeshiftfields[2].c_str());
        }
        else
#endif //TARGET_WINDOWS
        {
          // RTSP url
          return true; //Fast forward seek (OnZap) does not for RTSP
        }
      }
      else
      {
        XBMC->Log(LOG_INFO, "Close existing TsReader and create a new one...");
        m_tsreader->Close();
        delete m_tsreader;
        m_tsreader = new CTsReader();
      }
    }
    else
    {
      XBMC->Log(LOG_INFO, "Creating a new TsReader...");
      m_tsreader = new CTsReader();
    }
#ifdef TARGET_WINDOWS
    // Reading directly from the timeshift buffer is only supported under Windows at the moment
    if (g_bDirectTSFileRead)
    { // Timeshift buffer
      if (g_szTimeshiftDir.length() > 0)
      {
        m_tsreader->SetCardSettings(&m_cCards);
        m_tsreader->SetDirectory(g_szTimeshiftDir);
      }
      if ( m_tsreader->Open(timeshiftfields[2].c_str()) != S_OK )
      {
        SAFE_DELETE(m_tsreader);
        return false;
      }
    }
    else
#endif //TARGET_WINDOWS
    {
      // use the RTSP url and live555
      if ( m_tsreader->Open(timeshiftfields[0].c_str()) != S_OK)
      {
        SAFE_DELETE(m_tsreader);
        return false;
      }
      usleep(400000);
    }
#endif //TSREADER

    // at this point everything is ready for playback
    m_iCurrentChannel = (int) channelinfo.iUniqueId;
    if (g_iTVServerXBMCBuild>=106)
    {
      m_iCurrentCard = atoi(timeshiftfields[3].c_str());
    }
  }
  return true;
}

int cPVRClientMediaPortal::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
#ifdef TSREADER
  unsigned long read_wanted = iBufferSize;
  unsigned long read_done   = 0;
  static int read_timeouts  = 0;
  unsigned char* bufptr = pBuffer;

  //XBMC->Log(LOG_DEBUG, "->ReadLiveStream(buf_size=%i)", buf_size);
  if (!m_tsreader)
    return -1;

  while (read_done < (unsigned long) iBufferSize)
  {
    read_wanted = iBufferSize - read_done;

    if (m_tsreader->Read(bufptr, read_wanted, &read_wanted) > 0)
    {
      usleep(400000);
      read_timeouts++;
      return read_wanted; //writeNoSignalStream(buf, (buf_size - read_done));
    }
    read_done += read_wanted;

    if ( read_done < (unsigned long) iBufferSize )
    {
      if (read_timeouts > 25)
      {
        XBMC->Log(LOG_INFO, "No data in 1 second");
        read_timeouts = 0;
        m_bPlayingNoSignal = true;
        return read_done; //writeNoSignalStream(bufptr, read_wanted);
      }
      bufptr += read_wanted;
      read_timeouts++;
      usleep(40000);
    }
  }
  read_timeouts = 0;
  m_bPlayingNoSignal = false;
  return read_done;//TSReadDone*TS_SIZE;
#else
  return 0;
#endif //TSREADER
}

void cPVRClientMediaPortal::CloseLiveStream(void)
{
  string result;

  if (!IsUp())
     return;

  if (m_bTimeShiftStarted)
  {
#ifdef TSREADER
    if (m_tsreader)
    {
      m_tsreader->Close();
      SAFE_DELETE(m_tsreader);
    }
#endif
    result = SendCommand("StopTimeshift:\n");
    XBMC->Log(LOG_INFO, "CloseLiveStream: %s", result.c_str());
    m_bTimeShiftStarted = false;
    m_iCurrentChannel = -1;
    m_iCurrentCard = 0;
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "CloseLiveStream: Nothing to do.");
  }
}


bool cPVRClientMediaPortal::SwitchChannel(const PVR_CHANNEL &channel)
{
  if (((int)channel.iUniqueId) == m_iCurrentChannel)
    return true;

#ifdef TSREADER
  XBMC->Log(LOG_DEBUG, "SwitchChannel(uid=%i) tsreader: open a new live stream", channel.iUniqueId);

  if ((g_iTVServerXBMCBuild < 90))
  {
    if (m_tsreader)
    {
      //Only remove the TSReader for TVServerXBMC older than v1.1.0.90
      m_tsreader->Close();
      SAFE_DELETE(m_tsreader);
    }
  }

  return OpenLiveStream(channel);
#else
  XBMC->Log(LOG_DEBUG, "SwitchChannel(uid=%i) ffmpeg rtsp: nothing to be done here... GetLiveSteamURL() should fetch a new rtsp url from the backend.", channel.iUniqueId);
#endif
  return false;
}


int cPVRClientMediaPortal::GetCurrentClientChannel()
{
  XBMC->Log(LOG_DEBUG, "GetCurrentClientChannel: uid=%i", m_iCurrentChannel);
  return m_iCurrentChannel;
}

PVR_ERROR cPVRClientMediaPortal::GetSignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (g_iTVServerXBMCBuild < 108)
  {
    // Not yet supported
    return PVR_ERROR_NO_ERROR;
  }

  vector<string>  lines;
  string          result;

  result = SendCommand("GetSignalQuality\n");

  if (result.length() > 0)
  {
    int signallevel = 0;
    int signalquality = 0;

    if (sscanf(result.c_str(),"%i|%i", &signallevel, &signalquality) == 2)
    {
      signalStatus.iSignal = (int) (signallevel * 655.35); // 100% is 0xFFFF 65535
      signalStatus.iSNR = (int) (signalquality * 655.35); // 100% is 0xFFFF 65535
      signalStatus.iBER = 0;
      strncpy(signalStatus.strAdapterStatus, "timeshifting", 1023); // hardcoded for now...
      // TODO: fetch the name of the correct card and not just the first one...
      strncpy(signalStatus.strAdapterName, m_cCards[m_iCurrentCard].Name.c_str(), 1023); //Size buffer is 1024 in xbmc_pvr_types.h
    }
  }
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
  XBMC->Log(LOG_DEBUG, "->OpenRecordedStream(index=%s)", recording.strRecordingId);
  if (!IsUp())
     return false;
#ifdef TSREADER

  std::string recfile = "";

  if (g_iTVServerXBMCBuild < 90)
  { //TVServerXBMC older than v1.1.0.90
    vector<string>  lines;
    string          result;

    result = SendCommand("ListRecordings:False\n");

    Tokenize(result, lines, ",");

    for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
    {
      string& data(*it);
      uri::decode(data);

      ///* Convert to UTF8 string format */
      //if (m_bCharsetConv)
      //  XBMC_unknown_to_utf8(str_result);

      cRecording myrecording;
      if (myrecording.ParseLine(data))
      {
        if( myrecording.Index() == atoi(recording.strRecordingId))
        {
          XBMC->Log(LOG_DEBUG, "RECORDING: %s", data.c_str() );

          if (g_bUseRecordingsDir == true)
          { //Replace path by given path in g_szRecordingsDir
            if (g_szRecordingsDir.length() > 0)
            {
              myrecording.SetDirectory(g_szRecordingsDir);
              recfile = myrecording.FilePath();
            }
            else
            {
              recfile  = myrecording.FilePath();
            }
          }
          else
          {
            recfile = myrecording.Stream();
          }

        }
        break;
      }
    }
  }
  else
  {
    // TVServerXBMC v1.1.0.90 or higher
    string         result;
    char           command[256];

    if(g_bUseRecordingsDir)
      snprintf(command, 256, "GetRecordingInfo:%s|False\n", recording.strRecordingId);
    else
      snprintf(command, 256, "GetRecordingInfo:%s|True\n", recording.strRecordingId);
    result = SendCommand(command);

    if(result.length() > 0)
    {
      cRecording myrecording;
      if (myrecording.ParseLine(result))
      {
        XBMC->Log(LOG_DEBUG, "RECORDING: %s", result.c_str() );

        if (g_bUseRecordingsDir == true)
        { //Replace path by given path in g_szRecordingsDir
          if (g_szRecordingsDir.length() > 0)
          {
            myrecording.SetDirectory(g_szRecordingsDir);
            recfile = myrecording.FilePath();
          }
          else
          {
            recfile  = myrecording.FilePath();
          }
        }
        else
        {
          recfile = myrecording.Stream();
        }
      }
    }
  }

  if (recfile.length() > 0)
  {
    m_tsreader = new CTsReader();
    if ( m_tsreader->Open(recfile.c_str()) != S_OK )
      return false;
    else
      return true;
  }
#endif

  return false;
}

void cPVRClientMediaPortal::CloseRecordedStream(void)
{
  string result;

  if (!IsUp())
     return;

#ifdef TSREADER
  if (m_tsreader)
  {
    XBMC->Log(LOG_DEBUG, "CloseRecordedStream: Stop TSReader...");
    m_tsreader->Close();
    SAFE_DELETE(m_tsreader);
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "CloseRecordedStream: Nothing to do.");
  }
#endif
}

int cPVRClientMediaPortal::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
#ifdef TSREADER
  unsigned long read_wanted = iBufferSize;
  unsigned long read_done   = 0;
  unsigned char* bufptr = pBuffer;

  while (read_done < (unsigned long) iBufferSize)
  {
    read_wanted = iBufferSize - read_done;
    if (!m_tsreader)
      return -1;

    if (m_tsreader->Read(bufptr, read_wanted, &read_wanted) > 0)
    {
      usleep(20000);
      return read_wanted; //writeNoSignalStream(buf, (buf_size - read_done));
    }
    read_done += read_wanted;

    if ( read_done < (unsigned long) iBufferSize )
    {
      bufptr += read_wanted;
      usleep(20000);
    }
  }
  //read_timeouts = 0;
  m_bPlayingNoSignal = false;
  return read_done;//TSReadDone*TS_SIZE;
#else
  return -1;
#endif
}

/*
 * \brief Request the stream URL for live tv/live radio.
 * The MediaPortal TV Server will try to open the requested channel for
 * time-shifting and when successful it will start an rtsp:// stream for this
 * channel and return the URL for this stream.
 */
const char* cPVRClientMediaPortal::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  string result;

  XBMC->Log(LOG_DEBUG, "->GetLiveStreamURL(uid=%i)", channelinfo.iUniqueId);

  if (!OpenLiveStream(channelinfo))
  {
    return "";
  }
  else
  {
    return m_PlaybackURL.c_str();
  }
}

bool cPVRClientMediaPortal::LoadGenreXML(const std::string &filename)
{
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(filename))
  {
    XBMC->Log(LOG_DEBUG, "unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  XBMC->Log(LOG_DEBUG, "Opened %s to read genre string to type/subtype translation table", filename.c_str());

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);
  string sGenre;
  const char* sGenreType = NULL;
  const char* sGenreSubType = NULL;
  genre_t genre;

  // block: genrestrings
  pElem = hDoc.FirstChildElement("genrestrings").Element();
  // should always have a valid root but handle gracefully if it does
  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <genrestrings> element");    
    return false;
  }

  //This should hold: pElem->Value() == "genrestrings"

  // save this for later
  hRoot=TiXmlHandle(pElem);

  // iterate through all genre elements
  TiXmlElement* pGenreNode = hRoot.FirstChildElement("genre").Element();
  //This should hold: pGenreNode->Value() == "genre"

  if (!pGenreNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <genre> element");
    return false;
  }

  for (; pGenreNode != NULL; pGenreNode = pGenreNode->NextSiblingElement("genre"))
  {
    const char* sGenreString = pGenreNode->GetText();

    if (sGenreString)
    {
      sGenreType = pGenreNode->Attribute("type");
      sGenreSubType = pGenreNode->Attribute("subtype");

      if ((sGenreType) && (strlen(sGenreType) > 2))
      {
        if(sscanf(sGenreType + 2, "%x", &genre.type) != 1)
          genre.type = 0;
      }
      else
      {
        genre.type = 0;
      }

      if ((sGenreSubType) && (strlen(sGenreSubType) > 2 ))
      {
        if(sscanf(sGenreSubType + 2, "%x", &genre.subtype) != 1)
          genre.subtype = 0;
      }
      else
      {
        genre.subtype = 0;
      }

      if (genre.type > 0)
      {
        XBMC->Log(LOG_DEBUG, "Genre '%s' => 0x%x, 0x%x", sGenreString, genre.type, genre.subtype);
        m_genremap.insert(std::pair<std::string, genre_t>(sGenreString, genre));
      }
    }
  }

  return true;
}
