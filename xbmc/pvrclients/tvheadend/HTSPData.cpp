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

#include "HTSPData.h"

extern "C" {
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
}

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

cHTSPData::cHTSPData()
{
}

cHTSPData::~cHTSPData()
{
  Close();
}

bool cHTSPData::Open(CStdString hostname, int port, CStdString user, CStdString pass, long timeout)
{
  if(!m_session.Connect(hostname, port))
    return false;

  if(m_session.GetProtocol() < 2)
  {
    XBMC->Log(LOG_ERROR, "Incompatible protocol version %d", m_session.GetProtocol());
    return false;
  }

  if(!user.IsEmpty())
    m_session.Auth(user, pass);

  if(!m_session.SendEnableAsync())
    return false;

  SetDescription("HTSP Data Listener");
  Start();

  m_started.Wait(timeout);
  return Running();
}

void cHTSPData::Close()
{
  Cancel(3);
  m_session.Abort();
  m_session.Close();
}

bool cHTSPData::CheckConnection()
{
  return true;
}

htsmsg_t* cHTSPData::ReadResult(htsmsg_t* m)
{
  m_Mutex.Lock();
  unsigned    seq (m_session.AddSequence());

  SMessage &message(m_queue[seq]);
  message.event = new cCondWait();
  message.msg   = NULL;

  m_Mutex.Unlock();
  htsmsg_add_u32(m, "seq", seq);
  if(!m_session.SendMessage(m))
  {
    m_queue.erase(seq);
    return NULL;
  }

  if(!message.event->Wait(2000))
    XBMC->Log(LOG_ERROR, "cHTSPData::ReadResult - Timeout waiting for response");
  m_Mutex.Lock();

  m =    message.msg;
  delete message.event;

  m_queue.erase(seq);

  m_Mutex.Unlock();
  return m;
}

bool cHTSPData::GetDriveSpace(long long *total, long long *used)
{
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getDiskSpace");
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::GetDriveSpace - failed to get getDiskSpace");
    return false;
  }

  int64_t freespace;
  if (htsmsg_get_s64(msg, "freediskspace", &freespace) != 0)
    return false;

  int64_t totalspace;
  if (htsmsg_get_s64(msg, "totaldiskspace", &totalspace) != 0)
    return false;

  *total = totalspace / 1024;
  *used  = (totalspace - freespace) / 1024;
  return true;
}

bool cHTSPData::GetTime(time_t *localTime, int *gmtOffset)
{
  XBMC->Log(LOG_DEBUG, "cHTSPData::GetTime()");

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getSysTime");
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::GetTime - failed to get sysTime");
    return false;
  }

  unsigned int secs;
  if (htsmsg_get_u32(msg, "time", &secs) != 0)
    return false;

  int offset;
  if (htsmsg_get_s32(msg, "timezone", &offset) != 0)
    return false;

  *localTime = secs;
  *gmtOffset = -offset*60; /* XBMC need timezone difference in minutes from GMT */
  return true;
}

int cHTSPData::GetNumChannels()
{
  return GetChannels().size();
}

PVR_ERROR cHTSPData::RequestChannelList(PVRHANDLE handle, int radio)
{
  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  SChannels channels = GetChannels();
  for(SChannels::iterator it = channels.begin(); it != channels.end(); ++it)
  {
    SChannel& channel = it->second;

    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(tag));
    tag.uid           = channel.id;
    tag.number        = channel.id;
    tag.name          = channel.name.c_str();
    tag.callsign      = channel.name.c_str();
    tag.radio         = channel.radio;
    tag.encryption    = channel.caid;
    tag.input_format  = "";
    tag.stream_url    = "";
    tag.bouquet       = 0;

    if(((bool)radio) == tag.radio)
    {
      PVR->TransferChannelEntry(handle, &tag);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cHTSPData::RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  SChannels channels = GetChannels();

  if (channels.find(channel.uid) != channels.end())
  {
    time_t stop;

    SEvent event;
    event.id = channels[channel.uid].event;
    if (event.id == 0)
      return PVR_ERROR_NO_ERROR;

    do
    {
      bool success = GetEvent(event, event.id);
      if (success)
      {
        PVR_PROGINFO broadcast;
        memset(&broadcast, 0, sizeof(PVR_PROGINFO));

        broadcast.channum         = event.chan_id >= 0 ? event.chan_id : channel.number;
        broadcast.uid             = event.id;
        broadcast.title           = event.title.c_str();
        broadcast.subtitle        = event.title.c_str();
        broadcast.description     = event.descs.c_str();
        broadcast.starttime       = event.start;
        broadcast.endtime         = event.stop;
        broadcast.genre_type      = (event.content & 0x0F) << 4;
        broadcast.genre_sub_type  = event.content & 0xF0;
        PVR->TransferEpgEntry(handle, &broadcast);

        event.id = event.next;
        stop = event.stop;
      }
      else
        break;

    } while(end > stop);

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

SRecordings cHTSPData::GetDVREntries(bool recorded, bool scheduled)
{
  time_t localTime;
  int gmtOffset;
  GetTime(&localTime, &gmtOffset);

  CMD_LOCK;
  SRecordings recordings;

  for(SRecordings::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    SRecording recording = it->second;

    bool recordedOrRecording   = (unsigned int)localTime >= recording.start;
    bool scheduledOrRecording  = (unsigned int)localTime <= recording.stop;

    if ((scheduledOrRecording && scheduled) || (recordedOrRecording && recorded))
      recordings[recording.id] = recording;
  }

  return recordings;
}

int cHTSPData::GetNumRecordings()
{
  SRecordings recordings = GetDVREntries(true, false);
  return recordings.size();
}

PVR_ERROR cHTSPData::RequestRecordingsList(PVRHANDLE handle)
{
  time_t localTime;
  int gmtOffset;
  GetTime(&localTime, &gmtOffset);

  SRecordings recordings = GetDVREntries(true, false);

  for(SRecordings::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    SRecording recording = it->second;

    PVR_RECORDINGINFO tag;
    tag.index           = recording.id;
    tag.directory       = "/";
    tag.subtitle        = "";
    tag.channel_name    = "";
    tag.recording_time  = recording.start + gmtOffset;
    tag.duration        = recording.stop - recording.start;
    tag.description     = recording.description.c_str();
    tag.title           = recording.title.c_str();

    CStdString streamURL = "http://";
    {
      CMD_LOCK;
      SChannels::const_iterator itr = m_channels.find(recording.channel);
      if (itr != m_channels.end())
        tag.channel_name = itr->second.name.c_str();
      else
        tag.channel_name = "";

      if (g_szUsername != "")
      {
        streamURL += g_szUsername;
        if (g_szPassword != "")
        {
          streamURL += ":";
          streamURL += g_szPassword;
        }
        streamURL += "@";
      }
      streamURL.Format("%s%s:%i/dvrfile/%i", streamURL.c_str(), g_szHostname.c_str(), g_iPortHTTP, recording.id);
    }
    tag.stream_url      = streamURL.c_str();

    PVR->TransferRecordingEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cHTSPData::DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  XBMC->Log(LOG_DEBUG, "cHTSPData::DeleteRecording()");

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "deleteDvrEntry");
  htsmsg_add_u32(msg, "id", recinfo.index);
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::DeleteRecording - Failed to get deleteDvrEntry");
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::DeleteRecording - Failed to parse param");
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_DELETED;
}

int cHTSPData::GetNumTimers()
{
  SRecordings recordings = GetDVREntries(false, true);
  return recordings.size();
}

PVR_ERROR cHTSPData::RequestTimerList(PVRHANDLE handle)
{
  time_t localTime;
  int gmtOffset;
  GetTime(&localTime, &gmtOffset);

  SRecordings recordings = GetDVREntries(false, true);

  for(SRecordings::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    SRecording recording = it->second;

    PVR_TIMERINFO tag;
    tag.index       = recording.id;
    tag.channelNum  = recording.channel;
    tag.starttime   = recording.start;
    tag.endtime     = recording.stop;
    tag.active      = true;
    tag.recording   = (localTime > tag.starttime) && (localTime < tag.endtime);
    tag.title       = recording.title.c_str();
    tag.directory   = "/";
    tag.priority    = 0;
    tag.lifetime    = tag.endtime - tag.starttime;
    tag.repeat      = false;
    tag.repeatflags = 0;

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cHTSPData::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  XBMC->Log(LOG_DEBUG, "cHTSPData::DeleteRecording()");

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "deleteDvrEntry");
  htsmsg_add_u32(msg, "id", timerinfo.index);
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::DeleteRecording - Failed to get deleteDvrEntry");
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::DeleteRecording - Failed to parse param");
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_DELETED;
}

PVR_ERROR cHTSPData::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  XBMC->Log(LOG_DEBUG, "cHTSPData::AddTimer()");

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "addDvrEntry");
  htsmsg_add_u32(msg, "eventId", timerinfo.index);
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::AddTimer - Failed to get addDvrEntry");
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::AddTimer - Failed to parse param");
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_DELETED;
}

void cHTSPData::Action()
{
  XBMC->Log(LOG_DEBUG, "cHTSPData::Action() - Starting");

  htsmsg_t* msg;

  while (Running())
  {
    if((msg = m_session.ReadMessage()) == NULL)
      break;

    uint32_t seq;
    if(htsmsg_get_u32(msg, "seq", &seq) == 0)
    {
      CMD_LOCK;
      SMessages::iterator it = m_queue.find(seq);
      if(it != m_queue.end())
      {
        it->second.msg = msg;
        it->second.event->Signal();
        continue;
      }
    }

    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
    {
      htsmsg_destroy(msg);
      continue;
    }

    CMD_LOCK;
    if     (strstr(method, "channelAdd"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelRemove"))
      cHTSPSession::ParseChannelRemove(msg, m_channels);
    else if(strstr(method, "tagAdd"))
      cHTSPSession::ParseTagUpdate(msg, m_tags);
    else if(strstr(method, "tagUpdate"))
      cHTSPSession::ParseTagUpdate(msg, m_tags);
    else if(strstr(method, "tagRemove"))
      cHTSPSession::ParseTagRemove(msg, m_tags);
    else if(strstr(method, "initialSyncCompleted"))
      m_started.Signal();
    else if(strstr(method, "dvrEntryAdd") || strstr(method, "dvrEntryUpdate"))
      cHTSPSession::ParseDVREntryUpdate(msg, m_recordings);
    else if(strstr(method, "dvrEntryDelete"))
      cHTSPSession::ParseDVREntryDelete(msg, m_recordings);
    else
      XBMC->Log(LOG_DEBUG, "Unmapped action recieved '%s'", method);

    htsmsg_destroy(msg);
  }

  m_started.Signal();
  XBMC->Log(LOG_DEBUG, "cHTSPData::Action() - Exiting");
}

SChannels cHTSPData::GetChannels()
{
  return GetChannels(0);
}

SChannels cHTSPData::GetChannels(int tag)
{
  CMD_LOCK;
  if(tag == 0)
    return m_channels;

  STags::iterator it = m_tags.find(tag);
  if(it == m_tags.end())
  {
    SChannels channels;
    return channels;
  }
  return GetChannels(it->second);
}

SChannels cHTSPData::GetChannels(STag& tag)
{
  CMD_LOCK;
  SChannels channels;

  std::vector<int>::iterator it;
  for(it = tag.channels.begin(); it != tag.channels.end(); it++)
  {
    SChannels::iterator it2 = m_channels.find(*it);
    if(it2 == m_channels.end())
    {
      XBMC->Log(LOG_ERROR, "cHTSPData::GetChannels - tag points to unknown channel %d", *it);
      continue;
    }
    channels[*it] = it2->second;
  }
  return channels;
}

STags cHTSPData::GetTags()
{
  CMD_LOCK;
  return m_tags;
}

bool cHTSPData::GetEvent(SEvent& event, uint32_t id)
{
  if(id == 0)
  {
    event.Clear();
    return false;
  }

  SEvents::iterator it = m_events.find(id);
  if(it != m_events.end())
  {
    event = it->second;
    return true;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "cHTSPData::GetEvent - failed to get event %u", id);
    return false;
  }
  if(!cHTSPSession::ParseEvent(msg, id, event))
    return false;

  m_events[id] = event;
  return true;
}
