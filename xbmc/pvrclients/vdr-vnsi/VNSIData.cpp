/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include "VNSIData.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vdrcommand.h"

#ifdef __WINDOWS__
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

cVNSIData::cVNSIData() : m_connectionLost(false)
{
}

cVNSIData::~cVNSIData()
{
  Close();
}

bool cVNSIData::Open(const std::string& hostname, int port)
{
  if(!m_session.Open(hostname, port))
    return false;

  // store connection data for TryReconnect()
  m_hostname = hostname;
  m_port = port;

  SetDescription("VNSI Data Listener");
  Start();
  return true;
}

void cVNSIData::Close()
{
  Cancel(1);
  m_session.Abort();
  m_session.Close();
}

bool cVNSIData::TryReconnect() {
  m_session.Abort();
  m_session.Close();

  if(!Open(m_hostname, m_port)) {
    return false;
  }

  XBMC->Log(LOG_DEBUG, "cVNSIData -- reconnected");
  return true;
}

cResponsePacket* cVNSIData::ReadResult(cRequestPacket* vrp)
{
  m_Mutex.Lock();

  SMessage &message(m_queue[vrp->getSerial()]);
  message.event = new cCondWait();
  message.pkt   = NULL;

  m_Mutex.Unlock();

  if(!m_session.SendMessage(vrp))
  {
    m_queue.erase(vrp->getSerial());
    return NULL;
  }

  message.event->Wait(g_iConnectTimeout * 1000);

  m_Mutex.Lock();

  cResponsePacket* vresp = message.pkt;
  delete message.event;

  m_queue.erase(vrp->getSerial());

  m_Mutex.Unlock();

  return vresp;
}

bool cVNSIData::GetTime(time_t *localTime, int *gmtOffset)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_GETTIME))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTime - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTime - Can't get response packed");
    return false;
  }

  uint32_t vdrTime       = vresp->extract_U32();
  int32_t  vdrTimeOffset = vresp->extract_S32();

  *localTime = vdrTime;
  *gmtOffset = vdrTimeOffset;

  delete vresp;
  return true;
}

bool cVNSIData::GetDriveSpace(long long *total, long long *used)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_DISKSIZE))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetDriveSpace - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetDriveSpace - Can't get response packed");
    return false;
  }

  uint32_t totalspace    = vresp->extract_U32();
  uint32_t freespace     = vresp->extract_U32();
  /* vresp->extract_U32(); percent not used */

  *total = totalspace;
  *used  = (totalspace - freespace);

  /* Convert from kBytes to Bytes */
  *total *= 1024;
  *used  *= 1024;

  delete vresp;
  return true;
}

bool cVNSIData::SupportChannelScan()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_SCAN_SUPPORTED))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::SupportChannelScan - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::SupportChannelScan - Can't get response packed");
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VDR_RET_OK ? true : false;
}

bool cVNSIData::EnableStatusInterface(bool onOff)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_ENABLESTATUSINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::EnableStatusInterface - Can't get response packed");
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VDR_RET_OK ? true : false;
}

bool cVNSIData::EnableOSDInterface(bool onOff)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_ENABLEOSDINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::EnableStatusInterface - Can't get response packed");
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VDR_RET_OK ? true : false;
}

int cVNSIData::GetChannelsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cVNSIData::GetChannelsList(PVRHANDLE handle, bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_CHANNELS_GETCHANNELS))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsList - Can't init cRequestPacket");
    return false;
  }
  if (!vrp.add_U32(radio))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsList - Can't add parameter to cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetChannelsList - Can't get response packed");
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(tag));

    tag.number        = vresp->extract_U32();
    tag.name          = vresp->extract_String();
    tag.callsign      = tag.name;
    tag.uid           = vresp->extract_U32();
    tag.bouquet       = vresp->extract_U32();
    tag.encryption    = vresp->extract_U32();
                        vresp->extract_U32(); // uint32_t vtype - currently unused
    tag.radio         = radio;
    tag.input_format  = "";
    tag.stream_url    = "";

    PVR->TransferChannelEntry(handle, &tag);
    delete[] tag.name;
  }

  delete vresp;
  return true;
}

bool cVNSIData::GetEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_EPG_GETFORCHANNEL))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetEPGForChannel - Can't init cRequestPacket");
    return false;
  }
  if (!vrp.add_U32(channel.number) || !vrp.add_U32(start) || !vrp.add_U32(end - start))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetEPGForChannel - Can't add parameter to cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetEPGForChannel - Can't get response packed");
    return false;
  }

  while (!vresp->end())
  {
    PVR_PROGINFO tag;
    memset(&tag, 0 , sizeof(tag));

    tag.channum         = channel.number;
    tag.uid             = vresp->extract_U32();
    tag.starttime       = vresp->extract_U32();
    tag.endtime         = tag.starttime + vresp->extract_U32();
    uint32_t content    = vresp->extract_U32();
    tag.genre_type      = content & 0xF0;
    tag.genre_sub_type  = content & 0x0F;
    tag.parental_rating = vresp->extract_U32();
    tag.title           = vresp->extract_String();
    tag.subtitle        = vresp->extract_String();
    tag.description     = vresp->extract_String();

    PVR->TransferEpgEntry(handle, &tag);
    delete[] tag.title;
    delete[] tag.subtitle;
    delete[] tag.description;
  }

  delete vresp;
  return true;
}


/** OPCODE's 60 - 69: VNSI network functions for timer access */

int cVNSIData::GetTimersCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cVNSIData::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_GET))           return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timernumber))          return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode != VDR_RET_OK)
  {
    delete vresp;
    if (returnCode == VDR_RET_DATAUNKNOWN)
      return PVR_ERROR_NOT_POSSIBLE;
    else if (returnCode == VDR_RET_ERROR)
      return PVR_ERROR_SERVER_ERROR;
  }

  tag.index       = vresp->extract_U32();
  tag.active      = vresp->extract_U32();
                    vresp->extract_U32(); // uint32_t recording - currently unused
                    vresp->extract_U32(); // uint32_t pending - currently unused
  tag.priority    = vresp->extract_U32();
  tag.lifetime    = vresp->extract_U32();
  tag.channelNum  = vresp->extract_U32();
  tag.channelUid  = vresp->extract_U32();
  tag.starttime   = vresp->extract_U32();
  tag.endtime     = vresp->extract_U32();
  tag.firstday    = vresp->extract_U32();
  tag.repeatflags = vresp->extract_U32();
  tag.repeat      = tag.repeatflags == 0 ? false : true;
  tag.title       = vresp->extract_String();
  tag.directory   = "";

  delete[] tag.title;
  delete vresp;
  return PVR_ERROR_NO_ERROR;
}

bool cVNSIData::GetTimersList(PVRHANDLE handle)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersList - Can't init cRequestPacket");
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "cVNSIData::GetTimersList - Can't get response packed");
    return false;
  }

  uint32_t numTimers = vresp->extract_U32();
  if (numTimers > 0)
  {
    while (!vresp->end())
    {
      PVR_TIMERINFO tag;
      tag.index       = vresp->extract_U32();
      tag.active      = vresp->extract_U32();
                        vresp->extract_U32(); // uint32_t recording - currently unused
                        vresp->extract_U32(); // uint32_t pending - currently unused
      tag.priority    = vresp->extract_U32();
      tag.lifetime    = vresp->extract_U32();
      tag.channelNum  = vresp->extract_U32();
      tag.channelUid  = vresp->extract_U32();
      tag.starttime   = vresp->extract_U32();
      tag.endtime     = vresp->extract_U32();
      tag.firstday    = vresp->extract_U32();
      tag.repeatflags = vresp->extract_U32();
      tag.repeat      = tag.repeatflags == 0 ? false : true;
      tag.title       = vresp->extract_String();
      tag.directory   = "";

      PVR->TransferTimerEntry(handle, &tag);
      delete[] tag.title;
    }
  }
  delete vresp;
  return true;
}

PVR_ERROR cVNSIData::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_ADD))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::AddTimer - Can't init cRequestPacket");
    return PVR_ERROR_UNKOWN;
  }

  // add directory in front of the title
  std::string path;
  if(timerinfo.directory != NULL && strlen(timerinfo.directory) > 0) {
    path += timerinfo.directory;
    if(path == "/") {
      path.clear();
    }
    else if(path.size() > 1) {
      if(path[0] == '/') {
        path = path.substr(1);
      }
    }

    if(path.size() > 0 && path[path.size()-1] != '/') {
      path += "/";
    }
  }

  if(timerinfo.title != NULL) {
    path += timerinfo.title;
  }

  // replace directory separators
  for(std::size_t i=0; i<path.size(); i++) {
    if(path[i] == '/' || path[i] == '\\') {
      path[i] = '~';
    }
  }

  if(path.empty()) {
    XBMC->Log(LOG_ERROR, "cVNSIData::AddTimer - Empty filename !");
    return PVR_ERROR_UNKOWN;
  }

  if (!vrp.add_U32(timerinfo.active))     return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.priority))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.lifetime))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.channelNum)) return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.starttime))  return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.endtime))    return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeat ? timerinfo.firstday : 0))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeatflags))return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(path.c_str()))      return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "cVNSIData::AddTimer - Can't get response packed");
    return PVR_ERROR_UNKOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == VDR_RET_DATALOCKED)
    return PVR_ERROR_ALREADY_PRESENT;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_DELETE))
    return PVR_ERROR_UNKOWN;

  if (!vrp.add_U32(timerinfo.index))
    return PVR_ERROR_UNKOWN;

  if (!vrp.add_U32(force))
    return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if (returnCode == VDR_RET_DATALOCKED)
    return PVR_ERROR_NOT_DELETED;
  if (returnCode == VDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  PVR_TIMERINFO timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.index, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.title = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR cVNSIData::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_TIMER_UPDATE))        return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.index))      return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.active))     return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.priority))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.lifetime))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.channelNum)) return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.starttime))  return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.endtime))    return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeat ? timerinfo.firstday : 0))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_U32(timerinfo.repeatflags))return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(timerinfo.title))   return PVR_ERROR_UNKOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == VDR_RET_DATAUNKNOWN)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

int cVNSIData::GetRecordingsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsCount - Can't init cRequestPacket");
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsCount - Can't get response packed");
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cVNSIData::GetRecordingsList(PVRHANDLE handle)
{
  m_recIndex = 1;
  m_RecordsPaths.clear();

  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsList - Can't init cRequestPacket");
    return PVR_ERROR_UNKOWN;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "cVNSIData::GetRecordingsList - Can't get response packed");
    return PVR_ERROR_UNKOWN;
  }

  while (!vresp->end())
  {
    std::string title;

    PVR_RECORDINGINFO tag;
    tag.index           = m_recIndex++;
    tag.recording_time  = vresp->extract_U32();
    tag.duration        = vresp->extract_U32();
    tag.priority        = vresp->extract_U32();
    tag.lifetime        = vresp->extract_U32();
    tag.channel_name    = vresp->extract_String();
    tag.title           = vresp->extract_String();
    tag.subtitle        = vresp->extract_String();
    tag.description     = vresp->extract_String();
    tag.directory       = vresp->extract_String();
    tag.stream_url      = "";

    /* Save the given path name for later to translate the
       index numbers to the path name. */
    char* fileName  = vresp->extract_String();
    m_RecordsPaths.push_back(fileName);

    PVR->TransferRecordingEntry(handle, &tag);

    delete[] tag.channel_name;
    delete[] tag.title;
    delete[] tag.subtitle;
    delete[] tag.description;
    delete[] tag.directory;
    delete[] fileName;
  }

  delete vresp;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::DeleteRecording(const std::string& path)
{
  cRequestPacket vrp;
  if (!vrp.init(VDR_RECORDINGS_DELETE))
    return PVR_ERROR_UNKOWN;

  if (!vrp.add_String(path.c_str()))
    return PVR_ERROR_UNKOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if (returnCode == VDR_RET_DATALOCKED)
    return PVR_ERROR_NOT_DELETED;
  if (returnCode == VDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == VDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

void cVNSIData::Action()
{
  uint32_t channelID;
  uint32_t requestID;
  uint32_t userDataLength;
  uint8_t* userData;

  bool readSuccess;

  cResponsePacket* vresp;

  while (Running())
  {
    // read channelID
    readSuccess = readData((uint8_t*)&channelID, sizeof(uint32_t));

    // just wait if we're currently not connected
    if (m_connectionLost)
    {
      usleep(1000 * 1000); // 1000 ms to relax
      continue;
    }

    if (!readSuccess) continue; // no data was read but the connection is ok.

    // Data was read
    channelID = ntohl(channelID);

    // read requestID
    if (!readData((uint8_t*)&requestID, sizeof(uint32_t)))
    {
      m_connectionLost = true;
      continue;
    }
    requestID = ntohl(requestID);

    // read userDataLength
    if (!readData((uint8_t*)&userDataLength, sizeof(uint32_t)))
    {
      m_connectionLost = true;
      continue;
    }
    userDataLength = ntohl(userDataLength);
    if (userDataLength > 5000000) {
      m_connectionLost = true;
      continue; // how big can these packets get?
    }

    // read userData
    userData = NULL;
    if (userDataLength > 0)
    {
      userData = (uint8_t*)malloc(userDataLength);
      if (!userData) continue;
      if (!userData || !readData(userData, userDataLength))
      {
        free(userData);
        m_connectionLost = true;
        continue;
      }
    }

    // assemble response packet
    vresp = new cResponsePacket();
    vresp->setResponse(requestID, userData, userDataLength);

    // CHANNEL_REQUEST_RESPONSE

    if (channelID == CHANNEL_REQUEST_RESPONSE)
    {

      CMD_LOCK;
      SMessages::iterator it = m_queue.find(requestID);
      if (it != m_queue.end())
      {
        it->second.pkt = vresp;
        it->second.event->Signal();
      }
      else
      {
        delete vresp;
      }
    }

    // CHANNEL_STATUS

    else if (channelID == CHANNEL_STATUS)
    {
      if (requestID == VDR_STATUS_MESSAGE)
      {
        uint32_t type = vresp->extract_U32();
        char* msgstr  = vresp->extract_String();
        std::string text = msgstr;

        if (g_bCharsetConv)
          XBMC->UnknownToUTF8(text);

        if (type == 2)
          XBMC->QueueNotification(QUEUE_ERROR, text.c_str());
        if (type == 1)
          XBMC->QueueNotification(QUEUE_WARNING, text.c_str());
        else
          XBMC->QueueNotification(QUEUE_INFO, text.c_str());

        delete[] msgstr;
      }
      else if (requestID == VDR_STATUS_RECORDING)
      {
                          vresp->extract_U32(); // device currently unused
        uint32_t on     = vresp->extract_U32();
        char* str1      = vresp->extract_String();
        char* str2      = vresp->extract_String();

        PVR->Recording(str1, str2, on);
        PVR->TriggerTimerUpdate();

        delete[] str1;
        delete[] str2;
      }
      else if (requestID == VDR_STATUS_TIMERCHANGE)
      {
        PVR->TriggerTimerUpdate();
      }

      delete vresp;
    }

    // UNKOWN CHANNELID

    else
    {
      XBMC->Log(LOG_ERROR, "cVNSIData::Action() - Rxd a response packet on channel %lu !!", channelID);
    }
  }
}

bool cVNSIData::readData(uint8_t* buffer, int totalBytes)
{
  if(m_connectionLost)
    if(TryReconnect()) m_connectionLost = false;
  else
    return false;

  int ret = m_session.readData(buffer, totalBytes);
  if (ret == 1)
    return true;
  else if (ret == 0)
    return false;

  XBMC->Log(LOG_ERROR, "cVNSIData - connection lost !!!");
  m_connectionLost = true;
  return false;
}

const std::string& cVNSIData::GetRecordingPath(uint32_t index)
{
  static std::string empty;

  if (index == 0 || index > m_RecordsPaths.size())
    return empty;

  return m_RecordsPaths[index-1];
}
