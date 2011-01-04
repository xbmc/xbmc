/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "client.h"
#include "timers.h"
#include "channels.h"
#include "recordings.h"
#include "epg.h"
#include "utils.h"
#include "pvrclient-mediaportal.h"
#include <ctime>

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

using namespace std;

/************************************************************/
/** Class interface */

cPVRClientMediaPortal::cPVRClientMediaPortal()
{
  m_iCurrentChannel   = 1;
  m_tcpclient         = new Socket(af_inet, pf_inet, sock_stream, tcp);
  m_bConnected        = false;
  m_bStop             = true;
  m_bTimeShiftStarted = false;
  m_BackendUTCoffset  = 0;
  m_BackendTime       = 0;
  m_bStop             = true;
  m_bConnected        = false;
}

cPVRClientMediaPortal::~cPVRClientMediaPortal()
{
  XBMC->Log(LOG_DEBUG, "->~cPVRClientMediaPortal()");
  Disconnect();
  delete m_tcpclient;
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

  /* Open Connection to MediaPortal Backend TV Server via the XBMC TV Server plugin*/
  XBMC->Log(LOG_DEBUG, "Connect() - Connecting to %s:%i", m_sHostname.c_str(), m_iPort);

  if (!m_tcpclient->create())
  {
    XBMC->Log(LOG_ERROR, "Connect() - Could not connect create socket");
    return false;
  }

  if (!m_tcpclient->connect(m_sHostname, m_iPort))
  {
    XBMC->Log(LOG_ERROR, "Connect() - Could not connect to MPTV backend");
    return false;
  }

  m_tcpclient->set_non_blocking(1);

  result = SendCommand("PVRclientXBMC:0-1\n");

  if(result.find("Unexpected protocol") != std::string::npos)
  {
    XBMC->Log(LOG_DEBUG, "TVServer does not accept protocol: PVRclientXBMC:0-1");
    return false;
  } else {
    vector<string> fields;
    int major = 0, minor = 0, revision = 0 , build = 0;
    int count = 0;

    // Check the version of the TVServerXBMC plugin:
    Tokenize(result, fields, "|");
    if(fields.size() == 2)
    {
      // Ok, this TVServerXBMC version answers with a version string
      count = sscanf(fields[1].c_str(), "%d.%d.%d.%d", &major, &minor, &revision, &build);
      if( count < 4 )
      {
        XBMC->Log(LOG_ERROR, "Connect() - Could not parse the TVServerXBMC version string '%s'", fields[1].c_str());
      }
      // Check for the minimal requirement: 1.0.7.x
      if( major < 1 || minor < 0 || revision < 7 )
      {
        XBMC->Log(LOG_ERROR, "Warning: Your TVServerXBMC version '%s' is too old. Please upgrade to 1.0.7.0 or higher!", fields[1].c_str());
      }
    } else {
      XBMC->Log(LOG_ERROR, "Warning: Your TVServerXBMC version is too old. Please upgrade.");
    }
  }

  char buffer[512];
  snprintf(buffer, 512, "%s:%i", m_sHostname.c_str(), m_iPort);
  m_ConnectionString = buffer;

  m_bConnected = true;
  return true;
}

void cPVRClientMediaPortal::Disconnect()
{
  string result;

  XBMC->Log(LOG_DEBUG, "->Disconnect()");

  if (m_tcpclient->is_valid() && m_bTimeShiftStarted)
  {
    result = SendCommand("IsTimeshifting:\n");

    if (result.find("True") != std::string::npos )
    {
      result = SendCommand("StopTimeshift:\n");
    }
  }

  result = SendCommand("CloseConnection:\n");

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

const char* cPVRClientMediaPortal::GetBackendName()
{
  if (!m_tcpclient->is_valid())
    return m_sHostname.c_str();

  XBMC->Log(LOG_DEBUG, "->GetBackendName()");

  if(m_BackendName.length() == 0)
  {
    m_BackendName = SendCommand("GetBackendName:\n");
  }

  return m_BackendName.c_str();
}

const char* cPVRClientMediaPortal::GetBackendVersion()
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

const char* cPVRClientMediaPortal::GetConnectionString()
{
  XBMC->Log(LOG_DEBUG, "->GetConnectionString()");

  return m_ConnectionString.c_str();
}

PVR_ERROR cPVRClientMediaPortal::GetDriveSpace(long long *total, long long *used)
{
  XBMC->Log(LOG_DEBUG, "->GetDriveSpace(): Todo implement me...");

  *total = 0;
  *used = 0;

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

  if(fields.size() == 3)
  {
    //From CPVREpg::CPVREpg(): Expected PVREpg GMT offset is in seconds
    m_BackendUTCoffset = ((atoi(fields[1].c_str()) * 60) + atoi(fields[2].c_str())) * 60;

    count = sscanf(fields[0].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if(count == 6)
    {
      //timeinfo = *localtime ( &rawtime );
      timeinfo.tm_hour = hour;
      timeinfo.tm_min = minute;
      timeinfo.tm_sec = second;
      timeinfo.tm_year = year - 1900;
      timeinfo.tm_mon = month - 1;
      timeinfo.tm_mday = day;
      // Make the other fields empty:
      timeinfo.tm_isdst = 0;
      timeinfo.tm_wday = 0;
      timeinfo.tm_yday = 0;

      m_BackendTime = mktime(&timeinfo);

      if(m_BackendTime < 0)
      {
        XBMC->Log(LOG_DEBUG, "GetMPTVTime: Unable to convert string '%s' into date+time", fields[0].c_str());
        return PVR_ERROR_SERVER_ERROR;
      }

      XBMC->Log(LOG_DEBUG, "GetMPTVTime: %s, offset: %i seconds", asctime(gmtime(&m_BackendTime)), m_BackendUTCoffset );

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

PVR_ERROR cPVRClientMediaPortal::RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start, time_t end)
{
  vector<string> lines;
  char           command[256];
  string         result;
  cEpg           epg;
  PVR_PROGINFO   broadcast;

  XBMC->Log(LOG_DEBUG, "->RequestEPGForChannel(%i)", channel.number);

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 256, "GetEPG:%i\n", channel.number);

  result = SendCommand(command);

  if(result.compare(0,5, "ERROR") != 0)
  {
    Tokenize(result, lines, ",");

    for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
    {
      string& data(*it);

      //CStdString str_result = data;
      //
      //if (m_bCharsetConv)
      //  XBMC_unknown_to_utf8(str_result);

      if( data.length() > 0) {
        uri::decode(data);

        bool isEnd = epg.ParseLine(data);

        if (isEnd && epg.StartTime() != 0)
        {
          broadcast.channum         = channel.number;
          broadcast.uid             = epg.UniqueId();
          broadcast.title           = epg.Title();
          broadcast.subtitle        = epg.ShortText();
          broadcast.description     = epg.Description();
          broadcast.starttime       = epg.StartTime();
          broadcast.endtime         = epg.EndTime();
          broadcast.genre_type      = epg.GenreType();
          broadcast.genre_sub_type  = epg.GenreSubType();
          broadcast.parental_rating = 0;
          PVR->TransferEpgEntry(handle, &broadcast);
        }
        epg.Reset();
      }
    }
  } else {
    XBMC->Log(LOG_DEBUG, "RequestEPGForChannel(%i) %s", channel.number, result.c_str());
  }

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int cPVRClientMediaPortal::GetNumChannels()
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

PVR_ERROR cPVRClientMediaPortal::RequestChannelList(PVRHANDLE handle, int radio)
{
  vector<string>  lines;
  CStdString      command;
  int             code;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  if(radio)
  {
    XBMC->Log(LOG_DEBUG, "RequestChannelList for Radio group:%s", g_sRadioGroup.c_str());
    command.Format("ListRadioChannels:%s\n", g_sRadioGroup.c_str());
  } else {
    XBMC->Log(LOG_DEBUG, "RequestChannelList for TV group:%s", g_sTVGroup.c_str());
    command.Format("ListTVChannels:%s\n", g_sTVGroup.c_str());
  }
  SendCommand2(command.c_str(), code, lines);

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);

    if (data.length() == 0) {
      break;
    }

    uri::decode(data);
    //if(radio) {
    //  XBMC->Log(LOG_DEBUG, "Radio channel: %s", data.c_str() );
    //} else {
    //  XBMC->Log(LOG_DEBUG, "TV channel: %s", data.c_str() );
    //}

    cChannel channel;
    if( channel.Parse(data) )
    {
      PVR_CHANNEL tag;
      tag.uid = channel.UID();
      tag.number = channel.UID(); //channel.ExternalID();
      tag.name = channel.Name();
      tag.callsign = "";
      tag.iconpath = "";
      tag.encryption = channel.Encrypted();
      tag.radio = (radio > 0 ? true : false) ; //TODO:(channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
      tag.hide = false;
      tag.recording = false;
      tag.bouquet = 0;
      tag.multifeed = false;
      tag.input_format = "";

      if(radio)
        tag.stream_url = "pvr://stream/radio/%i.ts"; //stream.c_str();
      else
        tag.stream_url = "pvr://stream/tv/%i.ts"; //stream.c_str();

      PVR->TransferChannelEntry(handle, &tag);
    }
  }

  //pthread_mutex_unlock(&m_critSection);
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

PVR_ERROR cPVRClientMediaPortal::RequestRecordingsList(PVRHANDLE handle)
{
  vector<string>  lines;
  string          result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("ListRecordings:\n");

  Tokenize(result, lines, ",");

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
      PVR_RECORDINGINFO tag;
      tag.index           = recording.Index();
      tag.channel_name    = recording.ChannelName();
      tag.lifetime        = MAXLIFETIME; //TODO: recording.Lifetime();
      tag.priority        = 0; //TODO? recording.Priority();
      tag.recording_time  = recording.StartTime();
      tag.duration        = (int) recording.Duration();
      tag.description     = recording.Description();
      tag.stream_url      = recording.Stream();
      tag.title           = recording.Title();
      tag.subtitle        = tag.title;
      tag.directory       = "";

      PVR->TransferRecordingEntry(handle, &tag);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  char            command[256];
  string          result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 256, "DeleteRecordedTV:%i\n", recinfo.index);

  result = SendCommand(command);

  if(result.find("True") ==  string::npos)
  {
    return PVR_ERROR_NOT_DELETED;
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  char           command[512];
  string         result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 512, "UpdateRecording:%i,%s\n",
    recinfo.index,
    newname);

  result = SendCommand(command);

  if(result.find("True") == string::npos)
  {
    XBMC->Log(LOG_DEBUG, "RenameRecording(%i) to %s [failed]", recinfo.index, newname);
    return PVR_ERROR_NOT_DELETED;
  }
  XBMC->Log(LOG_DEBUG, "RenameRecording(%i) to %s [done]", recinfo.index, newname);

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

PVR_ERROR cPVRClientMediaPortal::RequestTimerList(PVRHANDLE handle)
{
  vector<string>  lines;
  string          result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  result = SendCommand("ListSchedules:\n");

  Tokenize(result, lines, ",");

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    uri::decode(data);

    XBMC->Log(LOG_DEBUG, "SCHEDULED: %s", data.c_str() );

    cTimer timer;
    timer.ParseLine(data.c_str());

    //TODO: finish me...
    PVR_TIMERINFO tag;
    tag.index       = timer.Index();
    tag.active      = true; //false; //timer.HasFlags(tfActive);
    tag.channelNum  = timer.Channel();
    tag.firstday    = 0; //timer.FirstDay();
    tag.starttime   = timer.StartTime();
    tag.endtime     = timer.StopTime();
    tag.recording   = 0; //timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
    tag.title       = timer.Title();
    tag.directory   = timer.Dir();
    tag.priority    = timer.Priority();
    tag.lifetime    = 0; //timer.Lifetime();
    tag.repeat      = false; //timer.WeekDays() == 0 ? false : true;
    tag.repeatflags = 0;//timer.WeekDays();

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  string         result;
  char           command[256];

  XBMC->Log(LOG_DEBUG, "->GetTimerInfo(%i)", timernumber);

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command,256, "GetTimerInfo:%i\n", timernumber);

  result = SendCommand(command);

  cTimer timer;
  timer.ParseLine(result.c_str());
  //TODO: finish me...
  tag.index = timer.Index();
  tag.active = true; //false; //timer.HasFlags(tfActive);
  tag.channelNum = timer.Channel();
  tag.firstday = 0; //timer.FirstDay();
  tag.starttime = timer.StartTime();
  tag.endtime = timer.StopTime();
  tag.recording = 0; //timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
  tag.title = timer.Title();
  tag.priority = timer.Priority();
  tag.lifetime = 0; //timer.Lifetime();
  tag.repeat = false; //timer.WeekDays() == 0 ? false : true;
  tag.repeatflags = 0;//timer.WeekDays();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  char           command[1024];
  string         result;

#ifdef _TIME32_T_DEFINED
  XBMC->Log(LOG_DEBUG, "->AddTimer Channel: %i, starttime: %i endtime: %i program: %s", timerinfo.channelNum, timerinfo.starttime, timerinfo.endtime, timerinfo.title);
#else
  XBMC->Log(LOG_DEBUG, "->AddTimer Channel: %i, 64 bit times not yet supported!", timerinfo.channelNum);
#endif

  struct tm starttime;
  struct tm endtime;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  starttime = *gmtime( &timerinfo.starttime );
  XBMC->Log(LOG_DEBUG, "Start time %s", asctime(&starttime));

  endtime = *gmtime( &timerinfo.endtime );
  XBMC->Log(LOG_DEBUG, "End time %s", asctime(&endtime));


  CStdString title = timerinfo.title;
  title.Replace("|","");  //Remove commas from title field

  snprintf(command, 1024, "AddSchedule:%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
          timerinfo.channelNum,                                              //Channel number
          title.c_str(),                                                     //Program title
          starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date
          starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time
          endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date
          endtime.tm_hour, endtime.tm_min, endtime.tm_sec);                  //End time

  if (timerinfo.index == -1)
  {
    result = SendCommand(command);

    if(result.find("True") ==  string::npos)
    {
      XBMC->Log(LOG_DEBUG, "AddTimer for channel: %i [failed]", timerinfo.channelNum);
      return PVR_ERROR_NOT_SAVED;
    }
    XBMC->Log(LOG_DEBUG, "AddTimer for channel: %i [done]", timerinfo.channelNum);
  }
  else
  {
    // Modified timer
    XBMC->Log(LOG_DEBUG, "AddTimer Modify timer for channel: %i; Not yet supported!");
    return PVR_ERROR_NOT_SAVED;
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  char           command[256];
  string         result;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  snprintf(command, 256, "DeleteSchedule:%i\n",timerinfo.index);

  if (timerinfo.index == -1)
  {
    XBMC->Log(LOG_DEBUG, "DeleteTimer: schedule index = -1", timerinfo.index);
    return PVR_ERROR_NOT_DELETED;
  } else {
    XBMC->Log(LOG_DEBUG, "DeleteTimer: About to delete MediaPortal schedule index=%i", timerinfo.index);
    result = SendCommand(command);

    if(result.find("True") ==  string::npos)
    {
      XBMC->Log(LOG_DEBUG, "DeleteTimer %i [failed]", timerinfo.index);
      return PVR_ERROR_NOT_DELETED;
    }
    XBMC->Log(LOG_DEBUG, "DeleteTimer %i [done]", timerinfo.index);

  }

  //  return PVR_ERROR_SERVER_ERROR;
  //  return PVR_ERROR_NOT_SYNC;
  //    return PVR_ERROR_RECORDING_RUNNING;
  //    return PVR_ERROR_NOT_DELETED;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientMediaPortal::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  XBMC->Log(LOG_DEBUG, "RenameTimer %i for channel: %i; Not yet supported!", timerinfo.index, timerinfo.channelNum);
  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  PVR_TIMERINFO timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.index, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.title = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR cPVRClientMediaPortal::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  char           command[1024];
  string         result;

  struct tm starttime;
  struct tm endtime;

  //TODO: timerinfo.file bevat troep. Nakijken. => gefixed
  //TODO: bij opname Journaal 18:00-18:15 => 17:58-19:25 ??? Hmmz, dit klopt niet toch? => gefixed

#ifdef _TIME32_T_DEFINED
  XBMC->Log(LOG_DEBUG, "->Updateimer Index: %i Channel: %i, starttime: %i endtime: %i program: %s", timerinfo.index, timerinfo.channelNum, timerinfo.starttime, timerinfo.endtime, timerinfo.title);
#else
  XBMC->Log(LOG_DEBUG, "->UpdateTimer Channel: %i, 64 bit times not yet supported!", timerinfo.channelNum);
#endif

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  starttime = *gmtime( &timerinfo.starttime );
  XBMC->Log(LOG_DEBUG, "Start time %s", asctime(&starttime));

  endtime = *gmtime( &timerinfo.endtime );
  XBMC->Log(LOG_DEBUG, "End time %s", asctime(&endtime));

  CStdString title = timerinfo.title;
  title.Replace(",","");  //Remove commas from title field

  snprintf(command, 1024, "UpdateSchedule:%i|%i|%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
          timerinfo.index,                                                   //Schedule index
          timerinfo.active,                                                  //Active
          timerinfo.channelNum,                                              //Channel number
          title.c_str(),                                                     //Program title
          starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date
          starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time
          endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date
          endtime.tm_hour, endtime.tm_min, endtime.tm_sec);                  //End time

  result = SendCommand(command);

  if(result.find("True") ==  string::npos)
  {
    XBMC->Log(LOG_DEBUG, "AddTimer for channel: %i [failed]", timerinfo.channelNum);
    return PVR_ERROR_NOT_SAVED;
  }
  XBMC->Log(LOG_DEBUG, "AddTimer for channel: %i [done]", timerinfo.channelNum);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */

// The MediaPortal TV Server uses rtsp streams which XBMC can handle directly
// so we don't need to open the streams in this pvr addon.
// However, we still need to request the stream URL for the channel we want
// to watch as it is not known on beforehand.
// Most of the times it is the same URL for each selected channel. Only the
// stream itself changes. Example URL: rtsp://tvserverhost/stream2.0
// The number 2.0 may change when the tvserver is streaming multiple tv channels
// at the same time.
bool cPVRClientMediaPortal::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  unsigned int channel = channelinfo.number;

  XBMC->Log(LOG_DEBUG, "->OpenLiveStream(%i): Not supported for this PVR addon.", channel);

  return false;
}

void cPVRClientMediaPortal::CloseLiveStream()
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

int cPVRClientMediaPortal::ReadLiveStream(unsigned char* buf, int buf_size)
{
    return 0;
}

int cPVRClientMediaPortal::GetCurrentClientChannel()
{
  XBMC->Log(LOG_DEBUG, "->GetCurrentClientChannel");
  return m_iCurrentChannel;
}

bool cPVRClientMediaPortal::SwitchChannel(const PVR_CHANNEL &channelinfo)
{

  XBMC->Log(LOG_DEBUG, "->SwitchChannel(%i)", channelinfo.number);
  string rtsp_url = GetLiveStreamURL(channelinfo);

  if(rtsp_url.length() > 0)
  {
    m_bTimeShiftStarted = false; //debug test
    return true;
  }
  else
    return false;
}

PVR_ERROR cPVRClientMediaPortal::SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  //XBMC->Log(LOG_DEBUG, "->SignalQuality(): Not yet supported.");

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Record stream handling */
// MediaPortal recordings are also rtsp streams. Main difference here with
// respect to the live tv streams is that the URLs for the recordings
// can be requested on beforehand (done in the TVserverXBMC plugin).
// These URLs are stored in the field PVR_RECORDINGINFO.stream_url
bool cPVRClientMediaPortal::OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  XBMC->Log(LOG_DEBUG, "->OpenRecordedStream(index=%i)", recinfo.index);
  if (!IsUp())
     return false;

  return true;
}

void cPVRClientMediaPortal::CloseRecordedStream(void)
{
  return;
}

int cPVRClientMediaPortal::ReadRecordedStream(unsigned char* buf, int buf_size)
{
  return 0;
}

long long cPVRClientMediaPortal::SeekRecordedStream(long long pos, int whence)
{
  return 0;
}

long long cPVRClientMediaPortal::LengthRecordedStream(void)
{
  return 0;
}

/*
 * \brief Request the stream URL for live tv/live radio.
 * The MediaPortal TV Server will try to open the requested channel for
 * time-shifting and when successful it will start an rtsp:// stream for this
 * channel and return the URL for this stream.
 */
const char* cPVRClientMediaPortal::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  unsigned int channel = channelinfo.number;

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

  if(m_bResolveRTSPHostname == false)
  {
    // RTSP URL may contain a hostname, XBMC will do the IP resolve
    snprintf(command, 256, "TimeshiftChannel:%i|False\n", channel);
  }
  else
  {
    // RTSP URL will always contain an IP address, TVServerXBMC will
    // do the IP resolve
    snprintf(command, 256, "TimeshiftChannel:%i\n", channel);
  }
  result = SendCommand(command);

  if (result.find("ERROR") != std::string::npos || result.length() == 0)
  {
    XBMC->Log(LOG_ERROR, "Could not stream channel %i. %s", channel, result.c_str());
    return "";
  }
  else
  {
    if (m_iSleepOnRTSPurl > 0)
    {
      XBMC->Log(LOG_INFO, "Sleeping %i ms before opening stream: %s", m_iSleepOnRTSPurl, result.c_str());
      usleep(m_iSleepOnRTSPurl * 1000);
    }

    XBMC->Log(LOG_INFO, "Channel stream URL: %s", result.c_str());
    m_iCurrentChannel = channel;
    m_ConnectionString = result;

    // Check the returned stream URL. When the URL is an rtsp stream, we need
    // to close it again after watching to stop the timeshift.
    // A radio web stream (added to the TV Server) will return the web stream
    // URL without starting a timeshift.
    if(result.compare(0,4, "rtsp") == 0)
    {
      m_bTimeShiftStarted = true;
    }
    return m_ConnectionString.c_str();
  }
}
