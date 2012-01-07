/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
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

#include <stdlib.h>
#include <stdio.h>

#include <vdr/recording.h>
#include <vdr/channels.h>
#include <vdr/videodir.h>
#include <vdr/plugin.h>
#include <vdr/timers.h>
#include <vdr/menu.h>
#include <vdr/device.h>

#include "config.h"
#include "vnsicommand.h"
#include "recordingscache.h"
#include "vnsiclient.h"
#include "receiver.h"
#include "vnsiserver.h"
#include "recplayer.h"
#include "requestpacket.h"
#include "responsepacket.h"
#include "hash.h"
#include "wirbelscanservice.h" /// copied from modified wirbelscan plugin
                               /// must be hold up to date with wirbelscan


static bool IsRadio(const cChannel* channel)
{
  bool isRadio = false;

  // assume channels without VPID & APID are video channels
  if (channel->Vpid() == 0 && channel->Apid(0) == 0)
    isRadio = false;
  // channels without VPID are radio channels (channels with VPID 1 are encrypted radio channels)
  else if (channel->Vpid() == 0 || channel->Vpid() == 1)
    isRadio = true;

  return isRadio;
}

cMutex cVNSIClient::m_timerLock;

cVNSIClient::cVNSIClient(int fd, unsigned int id, const char *ClientAdr)
{
  m_Id                      = id;
  m_Streamer                = NULL;
  m_isStreaming             = false;
  m_ClientAddress           = ClientAdr;
  m_StatusInterfaceEnabled  = false;
  m_RecPlayer               = NULL;
  m_req                     = NULL;
  m_resp                    = NULL;
  m_processSCAN_Response    = NULL;
  m_processSCAN_Socket      = NULL;


  m_socket.set_handle(fd);

  Start();
}

cVNSIClient::~cVNSIClient()
{
  DEBUGLOG("%s", __FUNCTION__);
  StopChannelStreaming();
  m_socket.close(); // force closing connection
  Cancel(10);
  DEBUGLOG("done");
}

void cVNSIClient::Action(void)
{
  uint32_t kaTimeStamp;
  uint32_t channelID;
  uint32_t requestID;
  uint32_t opcode;
  uint32_t dataLength;
  uint8_t* data;

  while (Running())
  {
    if (!m_socket.read((uint8_t*)&channelID, sizeof(uint32_t))) break;
    channelID = ntohl(channelID);

    if (channelID == 1)
    {
      if (!m_socket.read((uint8_t*)&requestID, sizeof(uint32_t), 10000)) break;
      requestID = ntohl(requestID);

      if (!m_socket.read((uint8_t*)&opcode, sizeof(uint32_t), 10000)) break;
      opcode = ntohl(opcode);

      if (!m_socket.read((uint8_t*)&dataLength, sizeof(uint32_t), 10000)) break;
      dataLength = ntohl(dataLength);
      if (dataLength > 200000) // a random sanity limit
      {
        ERRORLOG("dataLength > 200000!");
        break;
      }

      if (dataLength)
      {
        data = (uint8_t*)malloc(dataLength);
        if (!data)
        {
          ERRORLOG("Extra data buffer malloc error");
          break;
        }

        if (!m_socket.read(data, dataLength, 10000))
        {
          ERRORLOG("Could not read data");
          free(data);
          break;
        }
      }
      else
      {
        data = NULL;
      }

      DEBUGLOG("Received chan=%u, ser=%u, op=%u, edl=%u", channelID, requestID, opcode, dataLength);

      if (!m_loggedIn && (opcode != VNSI_LOGIN))
      {
        ERRORLOG("Clients must be logged in before sending commands! Aborting.");
        if (data) free(data);
        break;
      }

      cRequestPacket* req = new cRequestPacket(requestID, opcode, data, dataLength);

      processRequest(req);
    }
    else if (channelID == 3)
    {
      if (!m_socket.read((uint8_t*)&kaTimeStamp, sizeof(uint32_t), 1000)) break;
      kaTimeStamp = ntohl(kaTimeStamp);

      uint8_t buffer[8];
      *(uint32_t*)&buffer[0] = htonl(3); // KA CHANNEL
      *(uint32_t*)&buffer[4] = htonl(kaTimeStamp);
      if (!m_socket.write(buffer, 8))
      {
        ERRORLOG("Could not send back KA reply");
        break;
      }
    }
    else
    {
      ERRORLOG("Incoming channel number unknown");
      break;
    }
  }

  /* If thread is ended due to closed connection delete a
     possible running stream here */
  StopChannelStreaming();
}

bool cVNSIClient::StartChannelStreaming(const cChannel *channel, uint32_t timeout)
{
  m_Streamer    = new cLiveStreamer(timeout);
  m_isStreaming = m_Streamer->StreamChannel(channel, 50, &m_socket, m_resp);
  return m_isStreaming;
}

void cVNSIClient::StopChannelStreaming()
{
  m_isStreaming = false;
  if (m_Streamer)
  {
    delete m_Streamer;
    m_Streamer = NULL;
  }
}

void cVNSIClient::TimerChange(const cTimer *Timer, eTimerChange Change)
{
  TimerChange();
}

void cVNSIClient::TimerChange()
{
  cMutexLock lock(&m_msgLock);

  if (m_StatusInterfaceEnabled)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_TIMERCHANGE))
    {
      delete resp;
      return;
    }

    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

void cVNSIClient::ChannelChange()
{
  cMutexLock lock(&m_msgLock);

  if (!m_StatusInterfaceEnabled)
    return;

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStatus(VNSI_STATUS_CHANNELCHANGE))
  {
    delete resp;
    return;
  }

  resp->finalise();
  m_socket.write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::RecordingsChange()
{
  cMutexLock lock(&m_msgLock);

  if (!m_StatusInterfaceEnabled)
    return;

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStatus(VNSI_STATUS_RECORDINGSCHANGE))
  {
    delete resp;
    return;
  }

  resp->finalise();
  m_socket.write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::Recording(const cDevice *Device, const char *Name, const char *FileName, bool On)
{
  cMutexLock lock(&m_msgLock);

  if (m_StatusInterfaceEnabled)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_RECORDING))
    {
      delete resp;
      return;
    }

    resp->add_U32(Device->CardIndex());
    resp->add_U32(On);
    if (Name)
      resp->add_String(Name);
    else
      resp->add_String("");

    if (FileName)
      resp->add_String(FileName);
    else
      resp->add_String("");

    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

void cVNSIClient::OsdStatusMessage(const char *Message)
{
  cMutexLock lock(&m_msgLock);

  if (m_StatusInterfaceEnabled && Message)
  {
    /* Ignore this messages */
    if (strcasecmp(Message, trVDR("Channel not available!")) == 0) return;
    else if (strcasecmp(Message, trVDR("Delete timer?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Delete recording?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Press any key to cancel shutdown")) == 0) return;
    else if (strcasecmp(Message, trVDR("Press any key to cancel restart")) == 0) return;
    else if (strcasecmp(Message, trVDR("Editing - shut down anyway?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Recording - shut down anyway?")) == 0) return;
    else if (strcasecmp(Message, trVDR("shut down anyway?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Recording - restart anyway?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Editing - restart anyway?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Delete channel?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Timer still recording - really delete?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Delete marks information?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Delete resume information?")) == 0) return;
    else if (strcasecmp(Message, trVDR("CAM is in use - really reset?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Really restart?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Stop recording?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Cancel editing?")) == 0) return;
    else if (strcasecmp(Message, trVDR("Cutter already running - Add to cutting queue?")) == 0) return;
    else if (strcasecmp(Message, trVDR("No index-file found. Creating may take minutes. Create one?")) == 0) return;

    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_MESSAGE))
    {
      delete resp;
      return;
    }

    resp->add_U32(0);
    resp->add_String(Message);
    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

bool cVNSIClient::processRequest(cRequestPacket* req)
{
  cMutexLock lock(&m_msgLock);

  m_req = req;
  m_resp = new cResponsePacket();
  if (!m_resp->init(m_req->getRequestID()))
  {
    ERRORLOG("Response packet init fail");
    delete m_resp;
    delete m_req;
    m_resp = NULL;
    m_req = NULL;
    return false;
  }

  bool result = false;
  switch(m_req->getOpCode())
  {
    /** OPCODE 1 - 19: VNSI network functions for general purpose */
    case VNSI_LOGIN:
      result = process_Login();
      break;

    case VNSI_GETTIME:
      result = process_GetTime();
      break;

    case VNSI_ENABLESTATUSINTERFACE:
      result = process_EnableStatusInterface();
      break;

    case VNSI_PING:
      result = process_Ping();
      break;


    /** OPCODE 20 - 39: VNSI network functions for live streaming */
    case VNSI_CHANNELSTREAM_OPEN:
      result = processChannelStream_Open();
      break;

    case VNSI_CHANNELSTREAM_CLOSE:
      result = processChannelStream_Close();
      break;


    /** OPCODE 40 - 59: VNSI network functions for recording streaming */
    case VNSI_RECSTREAM_OPEN:
      result = processRecStream_Open();
      break;

    case VNSI_RECSTREAM_CLOSE:
      result = processRecStream_Close();
      break;

    case VNSI_RECSTREAM_GETBLOCK:
      result = processRecStream_GetBlock();
      break;

    case VNSI_RECSTREAM_POSTOFRAME:
      result = processRecStream_PositionFromFrameNumber();
      break;

    case VNSI_RECSTREAM_FRAMETOPOS:
      result = processRecStream_FrameNumberFromPosition();
      break;

    case VNSI_RECSTREAM_GETIFRAME:
      result = processRecStream_GetIFrame();
      break;


    /** OPCODE 60 - 79: VNSI network functions for channel access */
    case VNSI_CHANNELS_GETCOUNT:
      result = processCHANNELS_ChannelsCount();
      break;

    case VNSI_CHANNELS_GETCHANNELS:
      result = processCHANNELS_GetChannels();
      break;

    case VNSI_CHANNELGROUP_GETCOUNT:
      result = processCHANNELS_GroupsCount();
      break;

    case VNSI_CHANNELGROUP_LIST:
      result = processCHANNELS_GroupList();
      break;

    case VNSI_CHANNELGROUP_MEMBERS:
      result = processCHANNELS_GetGroupMembers();
      break;

    /** OPCODE 80 - 99: VNSI network functions for timer access */
    case VNSI_TIMER_GETCOUNT:
      result = processTIMER_GetCount();
      break;

    case VNSI_TIMER_GET:
      result = processTIMER_Get();
      break;

    case VNSI_TIMER_GETLIST:
      result = processTIMER_GetList();
      break;

    case VNSI_TIMER_ADD:
      result = processTIMER_Add();
      break;

    case VNSI_TIMER_DELETE:
      result = processTIMER_Delete();
      break;

    case VNSI_TIMER_UPDATE:
      result = processTIMER_Update();
      break;


    /** OPCODE 100 - 119: VNSI network functions for recording access */
    case VNSI_RECORDINGS_DISKSIZE:
      result = processRECORDINGS_GetDiskSpace();
      break;

    case VNSI_RECORDINGS_GETCOUNT:
      result = processRECORDINGS_GetCount();
      break;

    case VNSI_RECORDINGS_GETLIST:
      result = processRECORDINGS_GetList();
      break;

    case VNSI_RECORDINGS_RENAME:
      result = processRECORDINGS_Rename();
      break;

    case VNSI_RECORDINGS_DELETE:
      result = processRECORDINGS_Delete();
      break;


    /** OPCODE 120 - 139: VNSI network functions for epg access and manipulating */
    case VNSI_EPG_GETFORCHANNEL:
      result = processEPG_GetForChannel();
      break;


    /** OPCODE 140 - 159: VNSI network functions for channel scanning */
    case VNSI_SCAN_SUPPORTED:
      result = processSCAN_ScanSupported();
      break;

    case VNSI_SCAN_GETCOUNTRIES:
      result = processSCAN_GetCountries();
      break;

    case VNSI_SCAN_GETSATELLITES:
      result = processSCAN_GetSatellites();
      break;

    case VNSI_SCAN_START:
      result = processSCAN_Start();
      break;

    case VNSI_SCAN_STOP:
      result = processSCAN_Stop();
      break;
  }

  delete m_resp;
  m_resp = NULL;

  delete m_req;
  m_req = NULL;

  return result;
}


/** OPCODE 1 - 19: VNSI network functions for general purpose */

bool cVNSIClient::process_Login() /* OPCODE 1 */
{
  if (m_req->getDataLength() <= 4) return false;

  m_protocolVersion      = m_req->extract_U32();
                           m_req->extract_U8();
  const char *clientName = m_req->extract_String();

  if (m_protocolVersion > VNSI_PROTOCOLVERSION)
  {
    ERRORLOG("Client '%s' have a not allowed protocol version '%u', terminating client", clientName, m_protocolVersion);
    delete[] clientName;
    return false;
  }

  INFOLOG("Welcome client '%s' with protocol version '%u'", clientName, m_protocolVersion);

  // Send the login reply
  time_t timeNow        = time(NULL);
  struct tm* timeStruct = localtime(&timeNow);
  int timeOffset        = timeStruct->tm_gmtoff;

  m_resp->add_U32(VNSI_PROTOCOLVERSION);
  m_resp->add_U32(timeNow);
  m_resp->add_S32(timeOffset);
  m_resp->add_String("VDR-Network-Streaming-Interface (VNSI) Server");
  m_resp->add_String(VNSI_SERVER_VERSION);
  m_resp->finalise();
  SetLoggedIn(true);
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  delete[] clientName;

  return true;
}

bool cVNSIClient::process_GetTime() /* OPCODE 2 */
{
  time_t timeNow        = time(NULL);
  struct tm* timeStruct = localtime(&timeNow);
  int timeOffset        = timeStruct->tm_gmtoff;

  m_resp->add_U32(timeNow);
  m_resp->add_S32(timeOffset);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::process_EnableStatusInterface()
{
  bool enabled = m_req->extract_U8();

  SetStatusInterface(enabled);

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::process_Ping() /* OPCODE 7 */
{
  m_resp->add_U32(1);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}


/** OPCODE 20 - 39: VNSI network functions for live streaming */

bool cVNSIClient::processChannelStream_Open() /* OPCODE 20 */
{
  uint32_t uid = m_req->extract_U32();
  uint32_t timeout = m_req->extract_U32();

  if(timeout == 0)
    timeout = VNSIServerConfig.stream_timeout;

  if (m_isStreaming)
    StopChannelStreaming();

  Channels.Lock(false);
  const cChannel *channel = NULL;

  // try to find channel by uid first
  channel = FindChannelByUID(uid);
  Channels.Unlock();

  // try channelnumber
  if (channel == NULL)
    channel = Channels.GetByNumber(uid);

  if (channel == NULL) {
    ERRORLOG("Can't find channel %08x", uid);
    m_resp->add_U32(VNSI_RET_DATAINVALID);
  }
  else
  {
    if (StartChannelStreaming(channel, timeout))
    {
      INFOLOG("Started streaming of channel %s (timeout %i seconds)", channel->Name(), timeout);
      // return here without sending the response
      // (was already done in cLiveStreamer::StreamChannel)
      return true;
    }

    DEBUGLOG("Can't stream channel %s", channel->Name());
    m_resp->add_U32(VNSI_RET_DATALOCKED);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return false;
}

bool cVNSIClient::processChannelStream_Close() /* OPCODE 21 */
{
  if (m_isStreaming)
    StopChannelStreaming();

  return true;
}


/** OPCODE 40 - 59: VNSI network functions for recording streaming */

bool cVNSIClient::processRecStream_Open() /* OPCODE 40 */
{
  cRecording *recording = NULL;

  if(m_protocolVersion >= 2) {
    uint32_t uid = m_req->extract_U32();
    recording = cRecordingsCache::GetInstance().Lookup(uid);
  }
  else {
    const char *fileName = m_req->extract_String();
    recording = Recordings.GetByName(fileName);
    delete[] fileName;
  }

  if (recording && m_RecPlayer == NULL)
  {
    m_RecPlayer = new cRecPlayer(recording);

    m_resp->add_U32(VNSI_RET_OK);
    m_resp->add_U32(m_RecPlayer->getLengthFrames());
    m_resp->add_U64(m_RecPlayer->getLengthBytes());

#if VDRVERSNUM < 10703
    m_resp->add_U8(true);//added for TS
#else
    m_resp->add_U8(recording->IsPesRecording());//added for TS
#endif
  }
  else
  {
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
    ERRORLOG("%s - unable to start recording !", __FUNCTION__);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processRecStream_Close() /* OPCODE 41 */
{
  if (m_RecPlayer)
  {
    delete m_RecPlayer;
    m_RecPlayer = NULL;
  }

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRecStream_GetBlock() /* OPCODE 42 */
{
  if (m_isStreaming)
  {
    ERRORLOG("Get block called during live streaming");
    return false;
  }

  if (!m_RecPlayer)
  {
    ERRORLOG("Get block called when no recording open");
    return false;
  }

  uint64_t position  = m_req->extract_U64();
  uint32_t amount    = m_req->extract_U32();

  uint8_t* p = m_resp->reserve(amount);
  uint32_t amountReceived = m_RecPlayer->getBlock(p, position, amount);

  if(amount > amountReceived) m_resp->unreserve(amount - amountReceived);

  if (!amountReceived)
  {
    m_resp->add_U32(0);
    DEBUGLOG("written 4(0) as getblock got 0");
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRecStream_PositionFromFrameNumber() /* OPCODE 43 */
{
  uint64_t retval       = 0;
  uint32_t frameNumber  = m_req->extract_U32();

  if (m_RecPlayer)
    retval = m_RecPlayer->positionFromFrameNumber(frameNumber);

  m_resp->add_U64(retval);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  DEBUGLOG("Wrote posFromFrameNum reply to client");
  return true;
}

bool cVNSIClient::processRecStream_FrameNumberFromPosition() /* OPCODE 44 */
{
  uint32_t retval   = 0;
  uint64_t position = m_req->extract_U64();

  if (m_RecPlayer)
    retval = m_RecPlayer->frameNumberFromPosition(position);

  m_resp->add_U32(retval);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  DEBUGLOG("Wrote frameNumFromPos reply to client");
  return true;
}

bool cVNSIClient::processRecStream_GetIFrame() /* OPCODE 45 */
{
  bool success            = false;
  uint32_t frameNumber    = m_req->extract_U32();
  uint32_t direction      = m_req->extract_U32();
  uint64_t rfilePosition  = 0;
  uint32_t rframeNumber   = 0;
  uint32_t rframeLength   = 0;

  if (m_RecPlayer)
    success = m_RecPlayer->getNextIFrame(frameNumber, direction, &rfilePosition, &rframeNumber, &rframeLength);

  // returns file position, frame number, length
  if (success)
  {
    m_resp->add_U64(rfilePosition);
    m_resp->add_U32(rframeNumber);
    m_resp->add_U32(rframeLength);
  }
  else
  {
    m_resp->add_U32(0);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  DEBUGLOG("Wrote GNIF reply to client %llu %u %u", rfilePosition, rframeNumber, rframeLength);
  return true;
}


/** OPCODE 60 - 79: VNSI network functions for channel access */

bool cVNSIClient::processCHANNELS_ChannelsCount() /* OPCODE 61 */
{
  Channels.Lock(false);
  int count = Channels.MaxNumber();
  Channels.Unlock();

  m_resp->add_U32(count);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GetChannels() /* OPCODE 63 */
{
  if (m_req->getDataLength() != 4) return false;

  bool radio = m_req->extract_U32();

  Channels.Lock(false);

  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
    if (radio != IsRadio(channel))
      continue;

    // skip invalid channels
    if (channel->Sid() == 0)
      continue;

    m_resp->add_U32(channel->Number());
    m_resp->add_String(m_toUTF8.Convert(channel->Name()));
    if(m_protocolVersion >= 2) {
      m_resp->add_U32(CreateChannelUID(channel));
    }
    else {
      m_resp->add_U32(channel->Sid());
    }
    m_resp->add_U32(0); // groupindex unused
    m_resp->add_U32(channel->Ca());
#if APIVERSNUM >= 10701
    m_resp->add_U32(channel->Vtype());
#else
    m_resp->add_U32(2);
#endif
  }

  Channels.Unlock();

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processCHANNELS_GroupsCount()
{
  uint32_t type = m_req->extract_U32();

  Channels.Lock(false);

  m_channelgroups[0].clear();
  m_channelgroups[1].clear();

  switch(type)
  {
    // get groups defined in channels.conf
    default:
    case 0:
      CreateChannelGroups(false);
      break;
    // automatically create groups
    case 1:
      CreateChannelGroups(true);
      break;
  }

  Channels.Unlock();

  uint32_t count = m_channelgroups[0].size() + m_channelgroups[1].size();

  m_resp->add_U32(count);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GroupList()
{
  uint32_t radio = m_req->extract_U8();
  std::map<std::string, ChannelGroup>::iterator i;

  for(i = m_channelgroups[radio].begin(); i != m_channelgroups[radio].end(); i++)
  {
    m_resp->add_String(i->second.name.c_str());
    m_resp->add_U8(i->second.radio);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GetGroupMembers()
{
  char* groupname = m_req->extract_String();
  uint32_t radio = m_req->extract_U8();
  int index = 0;

  // unknown group
  if(m_channelgroups[radio].find(groupname) == m_channelgroups[radio].end())
  {
    delete[] groupname;
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    return true;
  }

  bool automatic = m_channelgroups[radio][groupname].automatic;
  std::string name;

  Channels.Lock(false);

  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {

    if(automatic && !channel->GroupSep())
      name = channel->Provider();
    else
    {
      if(channel->GroupSep())
      {
        name = channel->Name();
        continue;
      }
    }

    if(name.empty())
      continue;

    if(IsRadio(channel) != radio)
      continue;

    if(name == groupname)
    {
      m_resp->add_U32(CreateChannelUID(channel));
      m_resp->add_U32(++index);
    }
  }

  Channels.Unlock();

  delete[] groupname;
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

void cVNSIClient::CreateChannelGroups(bool automatic)
{
  std::string groupname;

  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
    bool isRadio = IsRadio(channel);

    if(automatic && !channel->GroupSep())
      groupname = channel->Provider();
    else if(!automatic && channel->GroupSep())
      groupname = channel->Name();

    if(groupname.empty())
      continue;

    if(m_channelgroups[isRadio].find(groupname) == m_channelgroups[isRadio].end())
    {
      ChannelGroup group;
      group.name = groupname;
      group.radio = isRadio;
      group.automatic = automatic;
      m_channelgroups[isRadio][groupname] = group;
    }
  }
}

/** OPCODE 80 - 99: VNSI network functions for timer access */

bool cVNSIClient::processTIMER_GetCount() /* OPCODE 80 */
{
  cMutexLock lock(&m_timerLock);

  int count = Timers.Count();

  m_resp->add_U32(count);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Get() /* OPCODE 81 */
{
  cMutexLock lock(&m_timerLock);

  uint32_t number = m_req->extract_U32();

  int numTimers = Timers.Count();
  if (numTimers > 0)
  {
    cTimer *timer = Timers.Get(number-1);
    if (timer)
    {
      m_resp->add_U32(VNSI_RET_OK);

      m_resp->add_U32(timer->Index()+1);
      m_resp->add_U32(timer->HasFlags(tfActive));
      m_resp->add_U32(timer->Recording());
      m_resp->add_U32(timer->Pending());
      m_resp->add_U32(timer->Priority());
      m_resp->add_U32(timer->Lifetime());
      m_resp->add_U32(timer->Channel()->Number());
      if(m_protocolVersion >= 2) {
        m_resp->add_U32(CreateChannelUID(timer->Channel()));
      }
      m_resp->add_U32(timer->StartTime());
      m_resp->add_U32(timer->StopTime());
      m_resp->add_U32(timer->Day());
      m_resp->add_U32(timer->WeekDays());
      m_resp->add_String(m_toUTF8.Convert(timer->File()));
    }
    else
      m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
  }
  else
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_GetList() /* OPCODE 82 */
{
  cMutexLock lock(&m_timerLock);

  cTimer *timer;
  int numTimers = Timers.Count();

  m_resp->add_U32(numTimers);

  for (int i = 0; i < numTimers; i++)
  {
    timer = Timers.Get(i);
    if (!timer)
      continue;

    m_resp->add_U32(timer->Index()+1);
    m_resp->add_U32(timer->HasFlags(tfActive));
    m_resp->add_U32(timer->Recording());
    m_resp->add_U32(timer->Pending());
    m_resp->add_U32(timer->Priority());
    m_resp->add_U32(timer->Lifetime());
    m_resp->add_U32(timer->Channel()->Number());
    if(m_protocolVersion >= 2) {
      m_resp->add_U32(CreateChannelUID(timer->Channel()));
    }
    m_resp->add_U32(timer->StartTime());
    m_resp->add_U32(timer->StopTime());
    m_resp->add_U32(timer->Day());
    m_resp->add_U32(timer->WeekDays());
    m_resp->add_String(m_toUTF8.Convert(timer->File()));
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Add() /* OPCODE 83 */
{
  cMutexLock lock(&m_timerLock);

  uint32_t flags      = m_req->extract_U32() > 0 ? tfActive : tfNone;
  uint32_t priority   = m_req->extract_U32();
  uint32_t lifetime   = m_req->extract_U32();
  uint32_t channelid  = m_req->extract_U32();
  time_t startTime    = m_req->extract_U32();
  time_t stopTime     = m_req->extract_U32();
  time_t day          = m_req->extract_U32();
  uint32_t weekdays   = m_req->extract_U32();
  const char *file    = m_req->extract_String();
  const char *aux     = m_req->extract_String();

  // handle instant timers
  if(startTime == -1 || startTime == 0)
  {
    startTime = time(NULL);
  }

  struct tm tm_r;
  struct tm *time = localtime_r(&startTime, &tm_r);
  if (day <= 0)
    day = cTimer::SetTime(startTime, 0);
  int start = time->tm_hour * 100 + time->tm_min;
  time = localtime_r(&stopTime, &tm_r);
  int stop = time->tm_hour * 100 + time->tm_min;

  cString buffer;
  if(m_protocolVersion == 1) {
    buffer = cString::sprintf("%u:%i:%s:%04d:%04d:%d:%d:%s:%s\n", flags, channelid, *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux);
  }
  else {
    const cChannel* channel = FindChannelByUID(channelid);
    if(channel != NULL) {
      buffer = cString::sprintf("%u:%s:%s:%04d:%04d:%d:%d:%s:%s\n", flags, (const char*)channel->GetChannelID().ToString(), *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux);
    }
  } 

  delete[] file;
  delete[] aux;

  cTimer *timer = new cTimer;
  if (timer->Parse(buffer))
  {
    cTimer *t = Timers.GetTimer(timer);
    if (!t)
    {
      Timers.Add(timer);
      Timers.SetModified();
      INFOLOG("Timer %s added", *timer->ToDescr());
      m_resp->add_U32(VNSI_RET_OK);
      m_resp->finalise();
      m_socket.write(m_resp->getPtr(), m_resp->getLen());
      return true;
    }
    else
    {
      ERRORLOG("Timer already defined: %d %s", t->Index() + 1, *t->ToText());
      m_resp->add_U32(VNSI_RET_DATALOCKED);
    }
  }
  else
  {
    ERRORLOG("Error in timer settings");
    m_resp->add_U32(VNSI_RET_DATAINVALID);
  }

  delete timer;

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Delete() /* OPCODE 84 */
{
  cMutexLock lock(&m_timerLock);

  uint32_t number = m_req->extract_U32();
  bool     force  = m_req->extract_U32();

  if (number <= 0 || number > (uint32_t)Timers.Count())
  {
    ERRORLOG("Unable to delete timer - invalid timer identifier");
    m_resp->add_U32(VNSI_RET_DATAINVALID);
  }
  else
  {
    cTimer *timer = Timers.Get(number-1);
    if (timer)
    {
      if (!Timers.BeingEdited())
      {
        if (timer->Recording())
        {
          if (force)
          {
            timer->Skip();
            cRecordControls::Process(time(NULL));
          }
          else
          {
            ERRORLOG("Timer \"%i\" is recording and can be deleted (use force=1 to stop it)", number);
            m_resp->add_U32(VNSI_RET_RECRUNNING);
            m_resp->finalise();
            m_socket.write(m_resp->getPtr(), m_resp->getLen());
            return true;
          }
        }
        INFOLOG("Deleting timer %s", *timer->ToDescr());
        Timers.Del(timer);
        Timers.SetModified();
        m_resp->add_U32(VNSI_RET_OK);
      }
      else
      {
        ERRORLOG("Unable to delete timer - timers being edited at VDR");
        m_resp->add_U32(VNSI_RET_DATALOCKED);
      }
    }
    else
    {
      ERRORLOG("Unable to delete timer - invalid timer identifier");
      m_resp->add_U32(VNSI_RET_DATAINVALID);
    }
  }
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Update() /* OPCODE 85 */
{
  cMutexLock lock(&m_timerLock);

  int length      = m_req->getDataLength();
  uint32_t index  = m_req->extract_U32();
  bool active     = m_req->extract_U32();

  cTimer *timer = Timers.Get(index - 1);
  if (!timer)
  {
    ERRORLOG("Timer \"%u\" not defined", index);
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    return true;
  }

  cTimer t = *timer;

  if (length == 8)
  {
    if (active)
      t.SetFlags(tfActive);
    else
      t.ClrFlags(tfActive);
  }
  else
  {
    uint32_t flags      = active ? tfActive : tfNone;
    uint32_t priority   = m_req->extract_U32();
    uint32_t lifetime   = m_req->extract_U32();
    uint32_t channelid  = m_req->extract_U32();
    time_t startTime    = m_req->extract_U32();
    time_t stopTime     = m_req->extract_U32();
    time_t day          = m_req->extract_U32();
    uint32_t weekdays   = m_req->extract_U32();
    const char *file    = m_req->extract_String();
    const char *aux     = m_req->extract_String();

    struct tm tm_r;
    struct tm *time = localtime_r(&startTime, &tm_r);
    if (day <= 0)
      day = cTimer::SetTime(startTime, 0);
    int start = time->tm_hour * 100 + time->tm_min;
    time = localtime_r(&stopTime, &tm_r);
    int stop = time->tm_hour * 100 + time->tm_min;

    cString buffer;
    if(m_protocolVersion == 1) {
      buffer = cString::sprintf("%u:%i:%s:%04d:%04d:%d:%d:%s:%s\n", flags, channelid, *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux);
    }
    else {
      const cChannel* channel = FindChannelByUID(channelid);
      if(channel != NULL) {
        buffer = cString::sprintf("%u:%s:%s:%04d:%04d:%d:%d:%s:%s\n", flags, (const char*)channel->GetChannelID().ToString(), *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux);
      }
    }

    delete[] file;
    delete[] aux;

    if (!t.Parse(buffer))
    {
      ERRORLOG("Error in timer settings");
      m_resp->add_U32(VNSI_RET_DATAINVALID);
      m_resp->finalise();
      m_socket.write(m_resp->getPtr(), m_resp->getLen());
      return true;
    }
  }

  *timer = t;
  Timers.SetModified();

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}


/** OPCODE 100 - 119: VNSI network functions for recording access */

bool cVNSIClient::processRECORDINGS_GetDiskSpace() /* OPCODE 100 */
{
  int FreeMB;
  int Percent = VideoDiskSpace(&FreeMB);
  int Total   = (FreeMB / (100 - Percent)) * 100;

  m_resp->add_U32(Total);
  m_resp->add_U32(FreeMB);
  m_resp->add_U32(Percent);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRECORDINGS_GetCount() /* OPCODE 101 */
{
  Recordings.Load();
  m_resp->add_U32(Recordings.Count());

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRECORDINGS_GetList() /* OPCODE 102 */
{
  cMutexLock lock(&m_timerLock);

  if(m_protocolVersion == 1) {
    m_resp->add_String(VideoDirectory);
  }

  for (cRecording *recording = Recordings.First(); recording; recording = Recordings.Next(recording))
  {
#if APIVERSNUM >= 10705
    const cEvent *event = recording->Info()->GetEvent();
#else
    const cEvent *event = NULL;
#endif

    time_t recordingStart    = 0;
    int    recordingDuration = 0;
    if (event)
    {
      recordingStart    = event->StartTime();
      recordingDuration = event->Duration();
    }
    else
    {
      cRecordControl *rc = cRecordControls::GetRecordControl(recording->FileName());
      if (rc)
      {
        recordingStart    = rc->Timer()->StartTime();
        recordingDuration = rc->Timer()->StopTime() - recordingStart;
      }
      else
      {
        recordingStart = recording->start;
      }
    }
    DEBUGLOG("GRI: RC: recordingStart=%lu recordingDuration=%i", recordingStart, recordingDuration);

    // recording_time
    m_resp->add_U32(recordingStart);

    // duration
    m_resp->add_U32(recordingDuration);

    // priority
    m_resp->add_U32(recording->priority);

    // lifetime
    m_resp->add_U32(recording->lifetime);

    // channel_name
    m_resp->add_String(recording->Info()->ChannelName() ? m_toUTF8.Convert(recording->Info()->ChannelName()) : "");

    char* fullname = strdup(recording->Name());
    char* recname = strrchr(fullname, FOLDERDELIMCHAR);
    char* directory = NULL;

    if(recname == NULL) {
      recname = fullname;
    }
    else {
      *recname = 0;
      recname++;
      directory = fullname;
    }

    // title
    m_resp->add_String(m_toUTF8.Convert(recname));

    // subtitle
    if (!isempty(recording->Info()->ShortText()))
      m_resp->add_String(m_toUTF8.Convert(recording->Info()->ShortText()));
    else
      m_resp->add_String("");

    // description
    if (!isempty(recording->Info()->Description()))
      m_resp->add_String(m_toUTF8.Convert(recording->Info()->Description()));
    else
      m_resp->add_String("");

    // directory
    if(m_protocolVersion >= 2) {
      if(directory != NULL) {
        char* p = directory;
        while(*p != 0) {
          if(*p == FOLDERDELIMCHAR) *p = '/';
          if(*p == '_') *p = ' ';
          p++;
        }
        while(*directory == '/') directory++;
      }

      m_resp->add_String((isempty(directory)) ? "" : m_toUTF8.Convert(directory));
    }

    // filename / uid of recording
    if(m_protocolVersion >= 2) {
      uint32_t uid = cRecordingsCache::GetInstance().Register(recording);
      m_resp->add_U32(uid);
    }
    else {
      cString filename = recording->FileName();
      m_resp->add_String(filename);
    }

    free(fullname);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRECORDINGS_Rename() /* OPCODE 103 */
{
  uint32_t    uid          = m_req->extract_U32();
  char*       newtitle     = m_req->extract_String();
  cRecording* recording    = cRecordingsCache::GetInstance().Lookup(uid);
  int         r            = VNSI_RET_DATAINVALID;

  if(recording != NULL) {
    // get filename and remove last part (recording time)
    char* filename_old = strdup((const char*)recording->FileName());
    char* sep = strrchr(filename_old, '/');
    if(sep != NULL) {
      *sep = 0;
    }

    // replace spaces in newtitle
    strreplace(newtitle, ' ', '_');
    char* filename_new = new char[512];
    strncpy(filename_new, filename_old, 512);
    sep = strrchr(filename_new, '/');
    if(sep != NULL) {
      sep++;
      *sep = 0;
    }
    strncat(filename_new, newtitle, 512);

    INFOLOG("renaming recording '%s' to '%s'", filename_old, filename_new);
    r = rename(filename_old, filename_new);
    Recordings.Update();

    free(filename_old);
    delete[] filename_new;
  }

  m_resp->add_U32(r);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processRECORDINGS_Delete() /* OPCODE 104 */
{
  cString recName;
  cRecording* recording = NULL;

  if(m_protocolVersion >= 2) {
    uint32_t uid = m_req->extract_U32();
    recording = cRecordingsCache::GetInstance().Lookup(uid);
  }
  else {
    const char* temp = m_req->extract_String();
    recName = temp;
    recording = Recordings.GetByName(recName);
    delete[] temp;
  }


  if (recording)
  {
    DEBUGLOG("deleting recording: %s", recording->Name());

    cRecordControl *rc = cRecordControls::GetRecordControl(recording->FileName());
    if (!rc)
    {
      if (recording->Delete())
      {
        // Copy svdrdeveldevelp's way of doing this, see if it works
        Recordings.DelByName(recording->FileName());
        INFOLOG("Recording \"%s\" deleted", recording->FileName());
        m_resp->add_U32(VNSI_RET_OK);
      }
      else
      {
        ERRORLOG("Error while deleting recording!");
        m_resp->add_U32(VNSI_RET_ERROR);
      }
    }
    else
    {
      ERRORLOG("Recording \"%s\" is in use by timer %d", recording->Name(), rc->Timer()->Index() + 1);
      m_resp->add_U32(VNSI_RET_DATALOCKED);
    }
  }
  else
  {
    ERRORLOG("Error in recording name \"%s\"", (const char*)recName);
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}


/** OPCODE 120 - 139: VNSI network functions for epg access and manipulating */

bool cVNSIClient::processEPG_GetForChannel() /* OPCODE 120 */
{
  uint32_t channelNumber  = 0;
  uint32_t channelUID  = 0;

  if(m_protocolVersion == 1) {
    channelNumber = m_req->extract_U32();
  }
  else {
    channelUID = m_req->extract_U32();
  }

  uint32_t startTime      = m_req->extract_U32();
  uint32_t duration       = m_req->extract_U32();

  Channels.Lock(false);

  const cChannel* channel = NULL;

  if(m_protocolVersion == 1) {
    channel = Channels.GetByNumber(channelNumber);
    DEBUGLOG("get schedule called for channel %u", channelNumber);
  }
  else {
    channel = FindChannelByUID(channelUID);
    if(channel != NULL) {
      DEBUGLOG("get schedule called for channel '%s'", (const char*)channel->GetChannelID().ToString());
    }
  }

  if (!channel)
  {
    m_resp->add_U32(0);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    Channels.Unlock();

    ERRORLOG("written 0 because channel = NULL");
    return true;
  }

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);
  if (!Schedules)
  {
    m_resp->add_U32(0);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    Channels.Unlock();

    DEBUGLOG("written 0 because Schedule!s! = NULL");
    return true;
  }

  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
  if (!Schedule)
  {
    m_resp->add_U32(0);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    Channels.Unlock();

    DEBUGLOG("written 0 because Schedule = NULL");
    return true;
  }

  bool atLeastOneEvent = false;

  uint32_t thisEventID;
  uint32_t thisEventTime;
  uint32_t thisEventDuration;
  uint32_t thisEventContent;
  uint32_t thisEventRating;
  const char* thisEventTitle;
  const char* thisEventSubTitle;
  const char* thisEventDescription;

  for (const cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event))
  {
    thisEventID           = event->EventID();
    thisEventTitle        = event->Title();
    thisEventSubTitle     = event->ShortText();
    thisEventDescription  = event->Description();
    thisEventTime         = event->StartTime();
    thisEventDuration     = event->Duration();
#if defined(USE_PARENTALRATING) || defined(PARENTALRATINGCONTENTVERSNUM)
    thisEventContent      = event->Contents();
    thisEventRating       = 0;
#elif APIVERSNUM >= 10711
    thisEventContent      = event->Contents();
    thisEventRating       = event->ParentalRating();
#else
    thisEventContent      = 0;
    thisEventRating       = 0;
#endif

    //in the past filter
    if ((thisEventTime + thisEventDuration) < (uint32_t)time(NULL)) continue;

    //start time filter
    if ((thisEventTime + thisEventDuration) <= startTime) continue;

    //duration filter
    if (duration != 0 && thisEventTime >= (startTime + duration)) continue;

    if (!thisEventTitle)        thisEventTitle        = "";
    if (!thisEventSubTitle)     thisEventSubTitle     = "";
    if (!thisEventDescription)  thisEventDescription  = "";

    m_resp->add_U32(thisEventID);
    m_resp->add_U32(thisEventTime);
    m_resp->add_U32(thisEventDuration);
    m_resp->add_U32(thisEventContent);
    m_resp->add_U32(thisEventRating);

    m_resp->add_String(m_toUTF8.Convert(thisEventTitle));
    m_resp->add_String(m_toUTF8.Convert(thisEventSubTitle));
    m_resp->add_String(m_toUTF8.Convert(thisEventDescription));

    atLeastOneEvent = true;
  }

  Channels.Unlock();
  DEBUGLOG("Got all event data");

  if (!atLeastOneEvent)
  {
    m_resp->add_U32(0);
    DEBUGLOG("Written 0 because no data");
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  DEBUGLOG("written schedules packet");

  return true;
}


/** OPCODE 140 - 169: VNSI network functions for channel scanning */

bool cVNSIClient::processSCAN_ScanSupported() /* OPCODE 140 */
{
  /** Note: Using "WirbelScanService-StopScan-v1.0" to detect
            a present service interface in wirbelscan plugin,
            it returns true if supported */
  cPlugin *p = cPluginManager::GetPlugin("wirbelscan");
  if (p && p->Service("WirbelScanService-StopScan-v1.0", NULL))
    m_resp->add_U32(VNSI_RET_OK);
  else
    m_resp->add_U32(VNSI_RET_NOTSUPPORTED);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_GetCountries() /* OPCODE 141 */
{
  if (!m_processSCAN_Response)
  {
    m_processSCAN_Response = m_resp;
    cPlugin *p = cPluginManager::GetPlugin("wirbelscan");
    if (p)
    {
      m_resp->add_U32(VNSI_RET_OK);
      p->Service("WirbelScanService-GetCountries-v1.0", (void*) processSCAN_AddCountry);
    }
    else
    {
      m_resp->add_U32(VNSI_RET_NOTSUPPORTED);
    }
    m_processSCAN_Response = NULL;
  }
  else
  {
    m_resp->add_U32(VNSI_RET_DATALOCKED);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_GetSatellites() /* OPCODE 142 */
{
  if (!m_processSCAN_Response)
  {
    m_processSCAN_Response = m_resp;
    cPlugin *p = cPluginManager::GetPlugin("wirbelscan");
    if (p)
    {
      m_resp->add_U32(VNSI_RET_OK);
      p->Service("WirbelScanService-GetSatellites-v1.0", (void*) processSCAN_AddSatellite);
    }
    else
    {
      m_resp->add_U32(VNSI_RET_NOTSUPPORTED);
    }
    m_processSCAN_Response = NULL;
  }
  else
  {
    m_resp->add_U32(VNSI_RET_DATALOCKED);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_Start() /* OPCODE 143 */
{
  WirbelScanService_DoScan_v1_0 svc;
  svc.type              = (scantype_t)m_req->extract_U32();
  svc.scan_tv           = (bool)m_req->extract_U8();
  svc.scan_radio        = (bool)m_req->extract_U8();
  svc.scan_fta          = (bool)m_req->extract_U8();
  svc.scan_scrambled    = (bool)m_req->extract_U8();
  svc.scan_hd           = (bool)m_req->extract_U8();
  svc.CountryIndex      = (int)m_req->extract_U32();
  svc.DVBC_Inversion    = (int)m_req->extract_U32();
  svc.DVBC_Symbolrate   = (int)m_req->extract_U32();
  svc.DVBC_QAM          = (int)m_req->extract_U32();
  svc.DVBT_Inversion    = (int)m_req->extract_U32();
  svc.SatIndex          = (int)m_req->extract_U32();
  svc.ATSC_Type         = (int)m_req->extract_U32();
  svc.SetPercentage     = processSCAN_SetPercentage;
  svc.SetSignalStrength = processSCAN_SetSignalStrength;
  svc.SetDeviceInfo     = processSCAN_SetDeviceInfo;
  svc.SetTransponder    = processSCAN_SetTransponder;
  svc.NewChannel        = processSCAN_NewChannel;
  svc.IsFinished        = processSCAN_IsFinished;
  svc.SetStatus         = processSCAN_SetStatus;
  m_processSCAN_Socket  = &m_socket;

  cPlugin *p = cPluginManager::GetPlugin("wirbelscan");
  if (p)
  {
    if (p->Service("WirbelScanService-DoScan-v1.0", (void*) &svc))
      m_resp->add_U32(VNSI_RET_OK);
    else
      m_resp->add_U32(VNSI_RET_ERROR);
  }
  else
  {
    m_resp->add_U32(VNSI_RET_NOTSUPPORTED);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_Stop() /* OPCODE 144 */
{
  cPlugin *p = cPluginManager::GetPlugin("wirbelscan");
  if (p)
  {
    p->Service("WirbelScanService-StopScan-v1.0", NULL);
    m_resp->add_U32(VNSI_RET_OK);
  }
  else
  {
    m_resp->add_U32(VNSI_RET_NOTSUPPORTED);
  }
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

cResponsePacket *cVNSIClient::m_processSCAN_Response = NULL;
cxSocket *cVNSIClient::m_processSCAN_Socket = NULL;

void cVNSIClient::processSCAN_AddCountry(int index, const char *isoName, const char *longName)
{
  m_processSCAN_Response->add_U32(index);
  m_processSCAN_Response->add_String(isoName);
  m_processSCAN_Response->add_String(longName);
}

void cVNSIClient::processSCAN_AddSatellite(int index, const char *shortName, const char *longName)
{
  m_processSCAN_Response->add_U32(index);
  m_processSCAN_Response->add_String(shortName);
  m_processSCAN_Response->add_String(longName);
}

void cVNSIClient::processSCAN_SetPercentage(int percent)
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_PERCENTAGE))
  {
    delete resp;
    return;
  }
  resp->add_U32(percent);
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::processSCAN_SetSignalStrength(int strength, bool locked)
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_SIGNAL))
  {
    delete resp;
    return;
  }
  strength *= 100;
  strength /= 0xFFFF;
  resp->add_U32(strength);
  resp->add_U32(locked);
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::processSCAN_SetDeviceInfo(const char *Info)
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_DEVICE))
  {
    delete resp;
    return;
  }
  resp->add_String(Info);
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::processSCAN_SetTransponder(const char *Info)
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_TRANSPONDER))
  {
    delete resp;
    return;
  }
  resp->add_String(Info);
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::processSCAN_NewChannel(const char *Name, bool isRadio, bool isEncrypted, bool isHD)
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_NEWCHANNEL))
  {
    delete resp;
    return;
  }
  resp->add_U32(isRadio);
  resp->add_U32(isEncrypted);
  resp->add_U32(isHD);
  resp->add_String(Name);
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::processSCAN_IsFinished()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_FINISHED))
  {
    delete resp;
    return;
  }
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  m_processSCAN_Socket = NULL;
  delete resp;
}

void cVNSIClient::processSCAN_SetStatus(int status)
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initScan(VNSI_SCANNER_STATUS))
  {
    delete resp;
    return;
  }
  resp->add_U32(status);
  resp->finalise();
  m_processSCAN_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}
