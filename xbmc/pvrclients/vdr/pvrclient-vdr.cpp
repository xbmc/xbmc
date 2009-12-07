/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "pvrclient-vdr.h"
#include "pvrclient-vdr_os.h"

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

using namespace std;

pthread_mutex_t m_critSection;

bool cPVRClientVDR::m_bStop                     = true;
SOCKET cPVRClientVDR::m_socket_data             = INVALID_SOCKET;
SOCKET cPVRClientVDR::m_socket_video            = INVALID_SOCKET;
CVTPTransceiver *cPVRClientVDR::m_transceiver   = NULL;
bool cPVRClientVDR::m_bConnected                = false;

/************************************************************/
/** Class interface */

cPVRClientVDR::cPVRClientVDR()
{
  m_iCurrentChannel   = 1;
  m_transceiver       = new CVTPTransceiver();
  m_bConnected        = false;
  m_socket_video      = INVALID_SOCKET;
  m_socket_data       = INVALID_SOCKET;
  m_bStop             = true;

  pthread_mutex_init(&m_critSection, NULL);
}

cPVRClientVDR::~cPVRClientVDR()
{
  Disconnect();
}


/************************************************************/
/** Server handling */

PVR_ERROR cPVRClientVDR::GetProperties(PVR_SERVERPROPS *props)
{
  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportTV                 = true;
  props->SupportRadio              = true;
  props->SupportChannelSettings    = true;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;
  props->HandleInputStream         = true;
  props->HandleDemuxing            = false;

  return PVR_ERROR_NO_ERROR;
}

bool cPVRClientVDR::Connect()
{
  /* Open Streamdev-Server VTP-Connection to VDR Backend Server */
  if (!m_transceiver->Open(m_sHostname, m_iPort))
    return false;

  /* Check VDR streamdev is patched by calling a newly added command */
  if (GetNumChannels() == -1)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Detected unsupported Streamdev-Version");
    return false;
  }

  /* Get Data socket from VDR Backend */
  m_socket_data = m_transceiver->GetStreamData();

  /* If received socket is invalid, return */
  if (m_socket_data == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for data response");
    return false;
  }

  m_connectionString.Format("%s:%i", m_sHostname.c_str(), m_iPort);

  /* Start VTP Listening Thread */
  m_bStop = false;
#if defined(_WIN32) || defined(_WIN64)
  if (pthread_create(&m_thread, NULL, (PTHREAD_START_ROUTINE)&CallbackRcvThread, reinterpret_cast<void *>(this)) != 0)
#else
  if (pthread_create(&m_thread, NULL, &CallbackRcvThread, reinterpret_cast<void *>(this)) != 0)
#endif
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't start VDR Listening Thread");
    return false;
  }

  readNoSignalStream();

  m_bConnected = true;
  return true;
}

void cPVRClientVDR::Disconnect()
{
  m_bStop = true;
//  pthread_join(m_thread, NULL);

  m_bConnected = false;

  /* Check if  stream sockets are open, if yes close them */
  if (m_socket_data != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamData();
    closesocket(m_socket_data);
    m_socket_data = INVALID_SOCKET;
  }

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
    m_socket_video = INVALID_SOCKET;
  }

  /* Close Streamdev-Server VTP Backend Connection */
  m_transceiver->Close();
}

bool cPVRClientVDR::IsUp()
{
  if (m_bConnected || m_transceiver->IsOpen())
  {
    return true;
  }
  return false;
}

void* cPVRClientVDR::Process(void*)
{
  char   		 data[1024];
  fd_set         set_r, set_e;
  struct timeval tv;
  int            res;

  while (!m_bStop)
  {
	if ((!m_transceiver->IsOpen()) || (m_socket_data == INVALID_SOCKET))
	{
	  XBMC_log(LOG_ERROR, "cPVRClientVDR::Process - Loosed connectio to VDR");
	  m_bConnected = false;
	  return NULL;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&set_r);
	FD_ZERO(&set_e);
	FD_SET(m_socket_data, &set_r);
	FD_SET(m_socket_data, &set_e);
	res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
	if (res < 0)
	{
	  XBMC_log(LOG_ERROR, "cPVRClientVDR::Process - select failed");
	  continue;
	}

    if (res == 0)
      continue;

	res = recv(m_socket_data, (char*)data, sizeof(data), 0);
	if (res < 0)
	{
	  XBMC_log(LOG_ERROR, "cPVRClientVDR::Process - failed");
	  continue;
	}

	if (res == 0)
	   continue;

	CStdString respStr = data;
	if (respStr.find("MODT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("DELT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("ADDT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("MODC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else if (respStr.find("DELC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else if (respStr.find("ADDC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else
	{
	  XBMC_log(LOG_ERROR, "cPVRClientVDR::Process - Unkown respond command %s", respStr.c_str());
	}
  }
  return NULL;
}


/************************************************************/
/** General handling */

const char* cPVRClientVDR::GetBackendName()
{
  if (!m_transceiver->IsOpen())
    return "";

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT name", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  pthread_mutex_unlock(&m_critSection);
  return data.c_str();
}

const char* cPVRClientVDR::GetBackendVersion()
{
  if (!m_transceiver->IsOpen())
    return "";

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT version", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);

  pthread_mutex_unlock(&m_critSection);
  return data.c_str();
}

const char* cPVRClientVDR::GetConnectionString()
{
  return m_connectionString.c_str();
}

PVR_ERROR cPVRClientVDR::GetDriveSpace(long long *total, long long *used)
{
  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT disk", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  size_t found = data.find("MB");

  if (found != CStdString::npos)
  {
    *total = atol(data.c_str()) * 1024;
    data.erase(0, found + 3);
    *used = atol(data.c_str()) * 1024;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** EPG handling */

PVR_ERROR cPVRClientVDR::RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start, time_t end)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  cEpg           epg;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (start != 0)
    sprintf(buffer, "LSTE %d from %lu to %lu", channel.number, (long)start, (long)end);
  else
    sprintf(buffer, "LSTE %d", channel.number);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    bool isEnd = epg.ParseLine(str_result.c_str());
    if (isEnd && epg.StartTime() != 0)
    {
      PVR_PROGINFO broadcast;
      broadcast.channum         = channel.number;
      broadcast.uid             = epg.UniqueId();
      broadcast.title           = epg.Title();
      broadcast.subtitle        = epg.ShortText();
      broadcast.description     = epg.Description();
      broadcast.starttime       = epg.StartTime();
      broadcast.endtime         = epg.EndTime();
      broadcast.genre_type      = epg.GenreType();
      broadcast.genre_sub_type  = epg.GenreSubType();
      PVR_transfer_epg_entry(handle, &broadcast);
      epg.Reset();
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int cPVRClientVDR::GetNumChannels()
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT channels", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR cPVRClientVDR::RequestChannelList(PVRHANDLE handle, bool radio)
{
  vector<string> lines;
  int            code;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  while (!m_transceiver->SendCommand("LSTC", code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    cChannel channel;
    channel.Parse(str_result.c_str());

    /* Ignore channels without streams */
    if (m_bNoBadChannels && channel.Vpid() == 0 && channel.Apid(0) == 0 && channel.Dpid(0) == 0)
      continue;

    PVR_CHANNEL tag;
    tag.uid         = channel.Sid();
    tag.number      = channel.Number();
    tag.name        = channel.Name();
    tag.callsign    = channel.Name();
    tag.iconpath    = "";
    tag.encryption  = channel.Ca();
    tag.radio       = (channel.Vpid() == 0) && (channel.Apid(0) != 0) ? true : false;
    tag.hide        = false;
    tag.recording   = false;
    tag.bouquet     = 0;
    tag.multifeed   = false;
    tag.stream_url  = "";

    if (radio == tag.radio)
      PVR_transfer_channel_entry(handle, &tag);
  }

  pthread_mutex_unlock(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Record handling **/

int cPVRClientVDR::GetNumRecordings(void)
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT records", code, lines))
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get recordings count");
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR cPVRClientVDR::RequestRecordingsList(PVRHANDLE handle)
{
  vector<string> linesShort;
  int            code;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("LSTR", code, linesShort))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it = linesShort.begin(); it != linesShort.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    /* Convert to UTF8 string format */
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    cRecording recording;
    if (recording.ParseEntryLine(str_result.c_str()))
    {
      char           buffer[1024];
      vector<string> linesDetails;

      sprintf(buffer, "LSTR %d", recording.Index());
      if (!m_transceiver->SendCommand(buffer, code, linesDetails))
        continue;

      for (vector<string>::iterator it2 = linesDetails.begin(); it2 != linesDetails.end(); it2++)
      {
        string& data2(*it2);
        CStdString str_details = data2;

        /* Convert to UTF8 string format */
        if (m_bCharsetConv)
          XBMC_unknown_to_utf8(str_details);

        recording.ParseLine(str_details.c_str());
      }

      PVR_RECORDINGINFO tag;
      tag.index           = recording.Index();
      tag.channel_name    = recording.ChannelName();
      tag.lifetime        = recording.Lifetime();
      tag.priority        = recording.Priority();
      tag.recording_time  = recording.StartTime();
      tag.duration        = recording.Duration();
      tag.subtitle        = recording.ShortText();
      tag.description     = recording.Description();
      tag.stream_url      = "";

      CStdString fileName = XBMC_make_legal_filename(recording.FileName());
      size_t found = fileName.find_last_of("~");
      if (found != CStdString::npos)
      {
        CStdString title = fileName.substr(found+1);
        tag.title = title.c_str();

        CStdString dir = fileName.substr(0,found);
        dir.Replace('~','/');
        tag.directory = dir.c_str();
      }
      else
      {
        tag.title = fileName;
        tag.directory = "";
      }

      PVR_transfer_recording_entry(handle, &tag);
    }
  }
  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientVDR::DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTR %d", recinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 215)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "DELR %d", recinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientVDR::RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTR %d", recinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }
  else if (code != 215)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  CStdString renamedName = recinfo.directory;
  if (renamedName != "" && renamedName[renamedName.size()-1] != '/')
    renamedName += "/";
  renamedName += newname;
  renamedName.Replace('/','~');;

  sprintf(buffer, "RENR %d %s", recinfo.index, renamedName.c_str());
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }
  else if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Timer handling */

int cPVRClientVDR::GetNumTimers(void)
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT timers", code, lines))
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get timers count");
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR cPVRClientVDR::RequestTimerList(PVRHANDLE handle)
{
  vector<string> lines;
  int            code;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("LSTT", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    /**
     * VDR Format given by LSTT:
     * 250-1 1:6:2008-10-27:0013:0055:50:99:Zeiglers wunderbare Welt des Fußballs:
     * 250 2 0:15:2008-10-26:2000:2138:50:99:ZDFtheaterkanal:
     * 250 3 1:6:MTWTFS-:2000:2129:50:99:WDR Köln:
     */

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    cTimer timer;
    timer.Parse(str_result.c_str());

    PVR_TIMERINFO tag;
    tag.index = timer.Index();
    tag.active = timer.HasFlags(tfActive);
    tag.channelNum = timer.Channel();
    tag.firstday = timer.FirstDay();
    tag.starttime = timer.StartTime();
    tag.endtime = timer.StopTime();
    tag.recording = timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
    tag.title = timer.File();
    tag.priority = timer.Priority();
    tag.lifetime = timer.Lifetime();
    tag.repeat = timer.WeekDays() == 0 ? false : true;
    tag.repeatflags = timer.WeekDays();

    PVR_transfer_timer_entry(handle, &tag);
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientVDR::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  vector<string>  lines;
  int             code;
  char            buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timernumber);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  CStdString str_result = data;

  if (m_bCharsetConv)
    XBMC_unknown_to_utf8(str_result);

  cTimer timer;
  timer.Parse(str_result.c_str());

  tag.index = timer.Index();
  tag.active = timer.HasFlags(tfActive);
  tag.channelNum = timer.Channel();
  tag.firstday = timer.FirstDay();
  tag.starttime = timer.StartTime();
  tag.endtime = timer.StopTime();
  tag.recording = timer.HasFlags(tfRecording) || timer.HasFlags(tfInstant);
  tag.title = timer.File();
  tag.priority = timer.Priority();
  tag.lifetime = timer.Lifetime();
  tag.repeat = timer.WeekDays() == 0 ? false : true;
  tag.repeatflags = timer.WeekDays();

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientVDR::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  cTimer timer(&timerinfo);

  pthread_mutex_lock(&m_critSection);

  if (timerinfo.index == -1)
  {
    sprintf(buffer, "NEWT %s", timer.ToText().c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }
  }
  else
  {
    // Modified timer
    sprintf(buffer, "LSTT %d", timerinfo.index);
    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }

    sprintf(buffer, "MODT %d %s", timerinfo.index, timer.ToText().c_str());
    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientVDR::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timerinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  lines.erase(lines.begin(), lines.end());

  if (force)
  {
    sprintf(buffer, "DELT %d FORCE", timerinfo.index);
  }
  else
  {
    sprintf(buffer, "DELT %d", timerinfo.index);
  }

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    vector<string>::iterator it = lines.begin();
    string& data(*it);
    CStdString str_result = data;
    pthread_mutex_unlock(&m_critSection);

    int found = str_result.find("is recording", 0);

    if (found != -1)
    {
      return PVR_ERROR_RECORDING_RUNNING;
    }
    else
    {
      return PVR_ERROR_NOT_DELETED;
    }
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientVDR::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  PVR_TIMERINFO timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.index, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.title = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR cPVRClientVDR::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  if (timerinfo.index == -1)
    return PVR_ERROR_NOT_SAVED;

  cTimer timer(&timerinfo);

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timerinfo.index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MODT %d %s", timerinfo.index, timer.ToText().c_str());
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */

bool cPVRClientVDR::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  if (!m_transceiver->IsOpen())
    return false;

  pthread_mutex_lock(&m_critSection);

  /* Check if a another stream is opened, if yes first close them */
  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);

    if (m_iCurrentChannel < 0)
      m_transceiver->AbortStreamRecording();
    else
      m_transceiver->AbortStreamLive();

    closesocket(m_socket_video);
  }

  /* Get Stream socked from VDR Backend */
  m_socket_video = m_transceiver->GetStreamLive(channelinfo.number);
  if (m_socket_video == INVALID_SOCKET)
  {
    /* retry one time otherwise If received socket is invalid, return */
    m_socket_video = m_transceiver->GetStreamLive(channelinfo.number);
    if (m_socket_video == INVALID_SOCKET)
    {
      XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for live tv");
      pthread_mutex_unlock(&m_critSection);
      return false;
    }
  }

  m_iCurrentChannel       = channelinfo.number;
  m_noSignalStreamReadPos = 0;
  m_playingNoSignal       = false;

  pthread_mutex_unlock(&m_critSection);
  return true;
}

void cPVRClientVDR::CloseLiveStream()
{
  if (!m_transceiver->IsOpen())
    return;

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
  }

  pthread_mutex_unlock(&m_critSection);
  return;
}

int cPVRClientVDR::ReadLiveStream(BYTE* buf, int buf_size)
{
  if (!m_transceiver->IsOpen())
    return 0;

  if (m_socket_video == INVALID_SOCKET)
    return 0;

  fd_set         set_r, set_e;
  struct timeval tv;
  int            res;

  tv.tv_sec = m_playingNoSignal ? 0 : 2;
  tv.tv_usec = m_playingNoSignal ? 25000 : 0;
  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket_video, &set_r);
  FD_SET(m_socket_video, &set_e);

  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::Read - select failed");
    return 0;
  }
  if (res == 0)
  {
    if (!m_playingNoSignal)
      XBMC_log(LOG_ERROR, "cPVRClientVDR::Read - timeout waiting for data");
    return writeNoSignalStream(buf, buf_size);
  }

  res = recv(m_socket_video, (char*)buf, (size_t)buf_size, MSG_WAITALL);
  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::Read - failed");
    return 0;
  }
  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::Read - eof");
    return 0;
  }

  m_playingNoSignal = false;
  return res;
}

int cPVRClientVDR::GetCurrentClientChannel()
{
  return m_iCurrentChannel;
}

bool cPVRClientVDR::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  if (!m_transceiver->IsOpen())
    return false;

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
  }

  m_socket_video = m_transceiver->GetStreamLive(channelinfo.number);
  if (m_socket_video == INVALID_SOCKET)
  {
    /* retry one time */
    m_socket_video = m_transceiver->GetStreamLive(channelinfo.number);
    if (m_socket_video == INVALID_SOCKET)
    {
      XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for live tv");
      pthread_mutex_unlock(&m_critSection);
      return false;
    }
  }

  m_iCurrentChannel       = channelinfo.number;
  m_noSignalStreamReadPos = 0;
  m_playingNoSignal       = false;

  pthread_mutex_unlock(&m_critSection);
  return true;
}

PVR_ERROR cPVRClientVDR::SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[32];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTQ %i", m_iCurrentChannel);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    const char *s = str_result.c_str();
    if (!strncasecmp(s, "Device", 6))
      strncpy(qualityinfo.frontend_name, s + 9, sizeof(qualityinfo.frontend_name));
    else if (!strncasecmp(s, "Status", 6))
      strncpy(qualityinfo.frontend_status, s + 9, sizeof(qualityinfo.frontend_status));
    else if (!strncasecmp(s, "Signal", 6))
      qualityinfo.signal = (uint16_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "SNR", 3))
      qualityinfo.snr = (uint16_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "BER", 3))
      qualityinfo.ber = (uint32_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "UNC", 3))
      qualityinfo.unc = (uint32_t)strtol(s + 9, NULL, 16);
    else if (!strncasecmp(s, "Video", 5))
      qualityinfo.video_bitrate = strtod(s + 9, NULL);
    else if (!strncasecmp(s, "Audio", 5))
      qualityinfo.audio_bitrate = strtod(s + 9, NULL);
    else if (!strncasecmp(s, "Dolby", 5))
      qualityinfo.dolby_bitrate = strtod(s + 9, NULL);
  }
  pthread_mutex_unlock(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Record stream handling */

bool cPVRClientVDR::OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  if (!m_transceiver->IsOpen())
  {
    return false;
  }

  pthread_mutex_lock(&m_critSection);

  /* Check if a another stream is opened, if yes first close them */

  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);

    if (m_iCurrentChannel < 0)
      m_transceiver->AbortStreamRecording();
    else
      m_transceiver->AbortStreamLive();

    closesocket(m_socket_video);
  }

  /* Get Stream socked from VDR Backend */
  m_socket_video = m_transceiver->GetStreamRecording(recinfo.index, &m_currentPlayingRecordBytes, &m_currentPlayingRecordFrames);

  pthread_mutex_unlock(&m_critSection);

  if (!m_currentPlayingRecordBytes)
  {
    return false;
  }

  /* If received socket is invalid, return */
  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for recording");
    return false;
  }

  m_iCurrentChannel              = -1;
  m_currentPlayingRecordPosition = 0;
  return true;
}

void cPVRClientVDR::CloseRecordedStream(void)
{
  if (!m_transceiver->IsOpen())
    return;

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamRecording();
    closesocket(m_socket_video);
  }

  pthread_mutex_unlock(&m_critSection);

  return;
}

int cPVRClientVDR::ReadRecordedStream(BYTE* buf, int buf_size)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  if (m_currentPlayingRecordPosition + buf_size > m_currentPlayingRecordBytes)
    return 0;

  sprintf(buffer, "READ %llu %u", (unsigned long long)m_currentPlayingRecordPosition, buf_size);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return 0;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  amountReceived = atol(data.c_str());

  fd_set         set_r, set_e;

  struct timeval tv;
  int            res;

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket_video, &set_r);
  FD_SET(m_socket_video, &set_e);
  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::ReadRecordedStream - select failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::ReadRecordedStream - timeout waiting for data");
    return 0;
  }

  res = recv(m_socket_video, (char*)buf, (size_t)buf_size, MSG_WAITALL);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::ReadRecordedStream - failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::ReadRecordedStream - eof");
    return 0;
  }

  m_currentPlayingRecordPosition += res;

  return res;
}

__int64 cPVRClientVDR::SeekRecordedStream(__int64 pos, int whence)
{

  if (!m_transceiver->IsOpen())
  {
    return 0;
  }

  __int64 nextPos = m_currentPlayingRecordPosition;

  switch (whence)
  {

    case SEEK_SET:
      nextPos = pos;
      break;

    case SEEK_CUR:
      nextPos += pos;
      break;

    case SEEK_END:

      if (m_currentPlayingRecordBytes)
        nextPos = m_currentPlayingRecordBytes - pos;
      else
        return -1;

      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos > (__int64) m_currentPlayingRecordBytes)
  {
    return 0;
  }

  m_currentPlayingRecordPosition = nextPos;

  return m_currentPlayingRecordPosition;
}

__int64 cPVRClientVDR::LengthRecordedStream(void)
{
  return m_currentPlayingRecordBytes;
}


/*************************************************************/
/** VDR to XBMC Callback functions                          **/
/**                                                         **/
/** If streamdev see a relevant event it send a callback    **/
/** message to the this client.                             **/
/*************************************************************/

void* cPVRClientVDR::CallbackRcvThread(void* arg)
{
  char   		      data[1024];
  fd_set          set_r, set_e;
  struct timeval  tv;
  int             ret;
  cPVRClientVDR  *PVRClientVDR = reinterpret_cast<cPVRClientVDR *>(arg) ;
  memset(data,0,1024);

  XBMC_log(LOG_DEBUG, "cPVRClientVDR::CallbackRcvThread - VDR to XBMC Data receiving thread started");

  while (!m_bStop)
  {
    if ((!m_transceiver->IsOpen()) || (m_socket_data == INVALID_SOCKET))
    {
      XBMC_log(LOG_ERROR, "cPVRClientVDR::CallbackRcvThread - Loosed connection to VDR");
      m_bConnected = false;
      return NULL;
    }

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(m_socket_data, &set_r);
    FD_SET(m_socket_data, &set_e);
    ret = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
    if (ret < 0)
    {
      XBMC_log(LOG_ERROR, "cPVRClientVDR::CallbackRcvThread - select failed");
      continue;
    }
    else if (ret == 0)
      continue;

    ret = recv(m_socket_data, (char*)data, sizeof(data), 0);
    if (ret < 0)
    {
      XBMC_log(LOG_ERROR, "cPVRClientVDR::CallbackRcvThread - receive failed");
      continue;
    }
    else if (ret == 0)
      continue;

    /* Check the received command and perform associated action*/
    PVRClientVDR->VDRToXBMCCommand(data);
    memset(data,0,1024);
  }
  return NULL;
}

bool cPVRClientVDR::VDRToXBMCCommand(char *Cmd)
{
	char *param = NULL;

	if (Cmd != NULL)
	{
		if ((param = strchr(Cmd, ' ')) != NULL)
			*(param++) = '\0';
		else
			param = Cmd + strlen(Cmd);
	}
	else
	{
    XBMC_log(LOG_ERROR, "cPVRClientVDR::VDRToXBMCCommand - called without command from %s:%d", m_sHostname.c_str(), m_iPort);
		return false;
	}

/* !!! DISABLED UNTIL STREAMDEV HAVE SUPPORT FOR IT INSIDE CVS !!!
	if      (strcasecmp(Cmd, "MODT") == 0) return CallBackMODT(param);
	else if (strcasecmp(Cmd, "DELT") == 0) return CallBackDELT(param);
	else if (strcasecmp(Cmd, "ADDT") == 0) return CallBackADDT(param);
*/
	if (!m_bHandleMessages)
    return true;

	if (strcasecmp(Cmd, "SMSG") == 0) return CallBackSMSG(param);
	else if (strcasecmp(Cmd, "IMSG") == 0) return CallBackIMSG(param);
	else if (strcasecmp(Cmd, "WMSG") == 0) return CallBackWMSG(param);
	else if (strcasecmp(Cmd, "EMSG") == 0) return CallBackEMSG(param);
	else
	{
    XBMC_log(LOG_ERROR, "cPVRClientVDR::VDRToXBMCCommand - Unkown respond command %s", Cmd);
		return false;
	}
}

bool cPVRClientVDR::CallBackMODT(const char *Option)
{
  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
  return true;
}

bool cPVRClientVDR::CallBackDELT(const char *Option)
{
  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
  return true;
}

bool cPVRClientVDR::CallBackADDT(const char *Option)
{
  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
  return true;
}

bool cPVRClientVDR::CallBackSMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(text);
    PVR_event_callback(PVR_EVENT_MSG_STATUS, text.c_str());
    return true;
  }
  else
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::CallBackSMSG - missing option");
    return false;
  }
}

bool cPVRClientVDR::CallBackIMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(text);
    PVR_event_callback(PVR_EVENT_MSG_INFO, text.c_str());
    return true;
  }
  else
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::CallBackIMSG - missing option");
    return false;
  }
}

bool cPVRClientVDR::CallBackWMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(text);
    PVR_event_callback(PVR_EVENT_MSG_WARNING, text.c_str());
    return true;
  }
  else
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::CallBackWMSG - missing option");
    return false;
  }
}

bool cPVRClientVDR::CallBackEMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(text);
    PVR_event_callback(PVR_EVENT_MSG_ERROR, text.c_str());
    return true;
  }
  else
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::CallBackEMSG - missing option");
    return false;
  }
}

bool cPVRClientVDR::readNoSignalStream()
{
  CStdString noSignalFileName;
  noSignalFileName  = XBMC_translate_path(XBMC_get_addon_directory());
  noSignalFileName += "/resources/data/noSignal.mpg";

  FILE *const f = ::fopen(noSignalFileName.c_str(), "rb");
  if (f)
  {
    m_noSignalStreamSize = ::fread(&m_noSignalStreamData[0] + 9, 1, sizeof (m_noSignalStreamData) - 9 - 9 - 4, f);
    if (m_noSignalStreamSize == sizeof (m_noSignalStreamData) - 9 - 9 - 4)
    {
      XBMC_log(LOG_ERROR, "cPVRClientVDR::readNoSignalStream - '%s' exeeds limit of %ld bytes!", noSignalFileName.c_str(), (long)(sizeof (m_noSignalStreamData) - 9 - 9 - 4 - 1));
    }
    else if (m_noSignalStreamSize > 0)
    {
      m_noSignalStreamData[ 0 ] = 0x00;
      m_noSignalStreamData[ 1 ] = 0x00;
      m_noSignalStreamData[ 2 ] = 0x01;
      m_noSignalStreamData[ 3 ] = 0xe0;
      m_noSignalStreamData[ 4 ] = (m_noSignalStreamSize + 3) >> 8;
      m_noSignalStreamData[ 5 ] = (m_noSignalStreamSize + 3) & 0xff;
      m_noSignalStreamData[ 6 ] = 0x80;
      m_noSignalStreamData[ 7 ] = 0x00;
      m_noSignalStreamData[ 8 ] = 0x00;
      m_noSignalStreamSize += 9;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x01;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0xe0;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x07;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x80;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x01;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0xb7;
    }
    ::fclose(f);
    return true;
  }
  else
  {
    XBMC_log(LOG_ERROR, "cPVRClientVDR::readNoSignalStream - couldn't open '%s'!", noSignalFileName.c_str());
  }

  return false;
}

int cPVRClientVDR::writeNoSignalStream(BYTE* buf, int buf_size)
{
  int sizeToWrite = m_noSignalStreamSize-m_noSignalStreamReadPos;
  m_playingNoSignal = true;
  if (buf_size > sizeToWrite)
  {
    memcpy(buf, m_noSignalStreamData+m_noSignalStreamReadPos, sizeToWrite);
    m_noSignalStreamReadPos = 0;
    return sizeToWrite;
  }
  else
  {
    memcpy(buf, m_noSignalStreamData+m_noSignalStreamReadPos, buf_size);
    m_noSignalStreamReadPos += buf_size;
    return buf_size;
  }
}
