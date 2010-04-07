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

#include <stdlib.h>
#include <vdr/recording.h>
#include <vdr/channels.h>
#include <vdr/videodir.h>
#include <vdr/plugin.h>
#include <vdr/timers.h>
#include <vdr/menu.h>

#include "config.h"
#include "cmdcontrol.h"
#include "connection.h"
#include "recplayer.h"
#include "vdrcommand.h"

cCmdControl::cCmdControl()
 {
  req         = NULL;
  resp        = NULL;

  Start();
}

cCmdControl::~cCmdControl()
{
  Cancel(1);
  m_Wait.Signal();
}

bool cCmdControl::recvRequest(cRequestPacket* newRequest)
{
  req_queue.push(newRequest);
  m_Wait.Signal();
  LOGCONSOLE("recvReq set req and signalled");
  return true;
}

void cCmdControl::Action(void)
{
  int errorCnt = 0;

  if (req_queue.size() != 0)
  {
    esyslog("VNSI-Error: Response handler thread started with still present packets in queue, exiting thread");
    return;
  }

  while (Running())
  {
    LOGCONSOLE("threadMethod waiting");
    m_Wait.Wait();  // unlocks, waits, relocks
    if (req_queue.size() == 0)
    {
      if (!Running())
        break;

      errorCnt++;
      if (errorCnt >= 5 || !Running())
      {
        esyslog("VNSI-Error: Response handler signaled more as 5 times without packets in queue, exiting thread");
        break;
      }
      continue;
    }

    errorCnt = 0;

    // signalled with something in queue
    LOGCONSOLE("thread woken with req, queue size: %i", req_queue.size());

    while (req_queue.size())
    {
      req = req_queue.front();
      req_queue.pop();

      if (!processPacket())
      {
        esyslog("VNSI-Error: Response handler failed during processPacket, exiting thread");
        continue;
      }
    }
  }
}

bool cCmdControl::processPacket()
{
  resp = new cResponsePacket();
  if (!resp->init(req->getRequestID()))
  {
    esyslog("VNSI-Error: Response packet init fail");
    delete resp;
    delete req;
    return false;
  }

  bool result = false;
  switch(req->getOpCode())
  {
    /** OPCODE 1 - 19: VNSI network functions for general purpose */
    case VDR_LOGIN:
      result = process_Login();
      break;

    case VDR_GETTIME:
      result = process_GetTime();
      break;


    /** OPCODE 20 - 39: VNSI network functions for live streaming */
    /** NOTE: Live streaming opcodes are handled by cConnection::Action(void) */


    /** OPCODE 40 - 59: VNSI network functions for recording streaming */
    case VDR_RECSTREAM_OPEN:
      result = processRecStream_Open();
      break;

    case VDR_RECSTREAM_CLOSE:
      result = processRecStream_Close();
      break;

    case VDR_RECSTREAM_GETBLOCK:
      result = processRecStream_GetBlock();
      break;

    case VDR_RECSTREAM_POSTOFRAME:
      result = processRecStream_PositionFromFrameNumber();
      break;

    case VDR_RECSTREAM_FRAMETOPOS:
      result = processRecStream_FrameNumberFromPosition();
      break;

    case VDR_RECSTREAM_GETIFRAME:
      result = processRecStream_GetIFrame();
      break;


    /** OPCODE 60 - 79: VNSI network functions for channel access */
    case VDR_CHANNELS_GROUPSCOUNT:
      result = processCHANNELS_GroupsCount();
      break;

    case VDR_CHANNELS_GETCOUNT:
      result = processCHANNELS_ChannelsCount();
      break;

    case VDR_CHANNELS_GETGROUPS:
      result = processCHANNELS_GroupList();
      break;

    case VDR_CHANNELS_GETCHANNELS:
      result = processCHANNELS_GetChannels();
      break;


    /** OPCODE 80 - 99: VNSI network functions for timer access */
    case VDR_TIMER_GETCOUNT:
      result = processTIMER_GetCount();
      break;

    case VDR_TIMER_GET:
      result = processTIMER_Get();
      break;

    case VDR_TIMER_GETLIST:
      result = processTIMER_GetList();
      break;

    case VDR_TIMER_ADD:
      result = processTIMER_Add();
      break;

    case VDR_TIMER_DELETE:
      result = processTIMER_Delete();
      break;

    case VDR_TIMER_UPDATE:
      result = processTIMER_Update();
      break;


    /** OPCODE 100 - 119: VNSI network functions for recording access */
    case VDR_RECORDINGS_DISKSIZE:
      result = processRECORDINGS_GetDiskSpace();
      break;

    case VDR_RECORDINGS_GETCOUNT:
      result = processRECORDINGS_GetCount();
      break;

    case VDR_RECORDINGS_GETLIST:
      result = processRECORDINGS_GetList();
      break;

    case VDR_RECORDINGS_GETINFO:
      result = processRECORDINGS_GetInfo();
      break;

    case VDR_RECORDINGS_DELETE:
      result = processRECORDINGS_Delete();
      break;

    case VDR_RECORDINGS_MOVE:
      result = processRECORDINGS_Move();
      break;


    /** OPCODE 120 - 139: VNSI network functions for epg access and manipulating */
    case VDR_EPG_GETFORCHANNEL:
      result = processEPG_GetForChannel();
      break;
  }

  delete resp;
  resp = NULL;

  delete req;
  req = NULL;

  return result;
}


/** OPCODE 1 - 19: VNSI network functions for general purpose */

bool cCmdControl::process_Login() /* OPCODE 1 */
{
  if (req->getDataLength() <= 4) return false;

  uint32_t protocolVersion  = req->extract_U32();
  bool netLog               = req->extract_U8();
  const char *clientName    = req->extract_String();

  if (protocolVersion != VNSIProtocolVersion)
  {
    esyslog("VNSI-Error: Client '%s' have a not allowed protocol version '%lu', terminating client", clientName, protocolVersion);
    delete clientName;
    return false;
  }

  isyslog("VNSI: Welcome client '%s' with protocol version '%lu'", clientName, protocolVersion);

  if (netLog)
    req->getClient()->EnableNetLog(true, clientName);

  // Send the login reply
  time_t timeNow        = time(NULL);
  struct tm* timeStruct = localtime(&timeNow);
  int timeOffset        = timeStruct->tm_gmtoff;

  resp->add_U32(VNSIProtocolVersion);
  resp->add_U32(timeNow);
  resp->add_S32(timeOffset);
  resp->add_String("VDR-Network-Streaming-Interface (VNSI) Server");
  resp->add_String(VNSI_SERVER_VERSION);
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

  req->getClient()->SetLoggedIn(true);
  delete clientName;
  return true;
}

bool cCmdControl::process_GetTime() /* OPCODE 2 */
{
  time_t timeNow        = time(NULL);
  struct tm* timeStruct = localtime(&timeNow);
  int timeOffset        = timeStruct->tm_gmtoff;

  resp->add_U32(timeNow);
  resp->add_S32(timeOffset);
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}


/** OPCODE 20 - 39: VNSI network functions for live streaming */
/** NOTE: Live streaming opcodes are handled by cConnection::Action(void) */


/** OPCODE 40 - 59: VNSI network functions for recording streaming */

bool cCmdControl::processRecStream_Open() /* OPCODE 40 */
{
  const char *fileName  = req->extract_String();
  cRecording *recording = Recordings.GetByName(fileName);

  LOGCONSOLE("%s: recording pointer %p", fileName, recording);

  if (recording)
  {
    req->getClient()->m_RecPlayer = new cRecPlayer(recording);

    resp->add_U32(VDR_RET_OK);
    resp->add_U32(req->getClient()->m_RecPlayer->getLengthFrames());
    resp->add_U64(req->getClient()->m_RecPlayer->getLengthBytes());

#if VDRVERSNUM < 10703
    resp->add_U8(true);//added for TS
#else
    resp->add_U8(recording->IsPesRecording());//added for TS
#endif

    LOGCONSOLE("written totalLength");
  }
  else
  {
    resp->add_U32(VDR_RET_DATAUNKNOWN);

    LOGCONSOLE("recording '%s' not found", fileName);
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processRecStream_Close() /* OPCODE 41 */
{
  if (req->getClient()->m_RecPlayer)
  {
    delete req->getClient()->m_RecPlayer;
    req->getClient()->m_RecPlayer = NULL;
  }

  resp->add_U32(VDR_RET_OK);
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processRecStream_GetBlock() /* OPCODE 42 */
{
  if (req->getClient()->IsStreaming())
  {
    esyslog("VNSI-Error: Get block called during live streaming");
    return false;
  }

  if (!req->getClient()->m_RecPlayer)
  {
    esyslog("VNSI-Error: Get block called when no recording open");
    return false;
  }

  uint64_t position  = req->extract_U64();
  uint32_t amount    = req->extract_U32();

//  LOGCONSOLE("getblock pos = %llu length = %lu", position, amount);

  uint8_t sendBuffer[amount];
  uint32_t amountReceived = req->getClient()->m_RecPlayer->getBlock(&sendBuffer[0], position, amount);

  if (!amountReceived)
  {
    resp->add_U32(0);
    LOGCONSOLE("written 4(0) as getblock got 0");
  }
  else
  {
    resp->copyin(sendBuffer, amountReceived);
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
//  LOGCONSOLE("Finished getblock, have sent %lu", resp->getLen());
  return true;
}

bool cCmdControl::processRecStream_PositionFromFrameNumber() /* OPCODE 43 */
{
  uint64_t retval       = 0;
  uint32_t frameNumber  = req->extract_U32();

  if (req->getClient()->m_RecPlayer)
    retval = req->getClient()->m_RecPlayer->positionFromFrameNumber(frameNumber);

  resp->add_U64(retval);
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

  LOGCONSOLE("Wrote posFromFrameNum reply to client");
  return true;
}

bool cCmdControl::processRecStream_FrameNumberFromPosition() /* OPCODE 44 */
{
  uint32_t retval   = 0;
  uint64_t position = req->extract_U64();

  if (req->getClient()->m_RecPlayer)
    retval = req->getClient()->m_RecPlayer->frameNumberFromPosition(position);

  resp->add_U32(retval);
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

  LOGCONSOLE("Wrote frameNumFromPos reply to client");
  return true;
}

bool cCmdControl::processRecStream_GetIFrame() /* OPCODE 45 */
{
  bool success            = false;
  uint32_t frameNumber    = req->extract_U32();
  uint32_t direction      = req->extract_U32();
  uint64_t rfilePosition  = 0;
  uint32_t rframeNumber   = 0;
  uint32_t rframeLength   = 0;

  if (req->getClient()->m_RecPlayer)
    success = req->getClient()->m_RecPlayer->getNextIFrame(frameNumber, direction, &rfilePosition, &rframeNumber, &rframeLength);

  // returns file position, frame number, length
  if (success)
  {
    resp->add_U64(rfilePosition);
    resp->add_U32(rframeNumber);
    resp->add_U32(rframeLength);
  }
  else
  {
    resp->add_U32(0);
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

  LOGCONSOLE("Wrote GNIF reply to client %llu %lu %lu", rfilePosition, rframeNumber, rframeLength);
  return true;
}


/** OPCODE 60 - 79: VNSI network functions for channel access */

bool cCmdControl::processCHANNELS_GroupsCount() /* OPCODE 60 */
{
  int count = 0;
  for (int curr = Channels.GetNextGroup(-1); curr >= 0; curr = Channels.GetNextGroup(curr))
    count++;

  resp->add_U32(count);

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processCHANNELS_ChannelsCount() /* OPCODE 61 */
{
  int count = Channels.MaxNumber();

  resp->add_U32(count);

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processCHANNELS_GroupList() /* OPCODE 62 */
{
  if (req->getDataLength() != 4) return false;

  bool radio = req->extract_U32();

  int countInGroup = 0;
  int index        = 0;
  const cChannel* group = NULL;
  cCharSetConv toUTF8;
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
    if (channel->GroupSep())
    {
      if (countInGroup)
      {
        resp->add_U32(index);
        resp->add_U32(countInGroup);
        resp->add_String(toUTF8.Convert(group->Name()));
      }
      group = channel;
      countInGroup = 0;
      index++;
    }
    else if (group)
    {
      if (radio)
      {
        if (!channel->Vpid() && channel->Apid(0))
          countInGroup++;
      }
      else
      {
        if (channel->Vpid())
          countInGroup++;
      }
    }
  }

  if (countInGroup)
  {
    resp->add_U32(index);
    resp->add_U32(countInGroup);
    resp->add_String(group->Name());
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processCHANNELS_GetChannels() /* OPCODE 63 */
{
  if (req->getDataLength() != 4) return false;

  bool radio = req->extract_U32();

  int groupIndex = 0;
  const cChannel* group = NULL;
  cCharSetConv toUTF8;
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
    if (channel->GroupSep())
    {
      group = channel;
      groupIndex++;
    }
    else
    {
      bool isRadio = false;

      if (channel->Vpid())
        isRadio = false;
      else if (channel->Apid(0))
        isRadio = true;
      else
        continue;

      if (radio != isRadio)
        continue;

      resp->add_U32(channel->Number());
      resp->add_String(toUTF8.Convert(channel->Name()));
      resp->add_U32(channel->Sid());
      resp->add_U32(groupIndex);
      resp->add_U32(channel->Ca());
      resp->add_U32(channel->Vtype());
    }
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}


/** OPCODE 80 - 99: VNSI network functions for timer access */

bool cCmdControl::processTIMER_GetCount() /* OPCODE 80 */
{
  int count = Timers.Count();

  resp->add_U32(count);

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processTIMER_Get() /* OPCODE 81 */
{
  uint32_t number = req->extract_U32();

  cCharSetConv toUTF8;
  int numTimers = Timers.Count();
  if (numTimers > 0)
  {
    cTimer *timer = Timers.Get(number-1);
    if (timer)
    {
      resp->add_U32(VDR_RET_OK);

      resp->add_U32(timer->Index()+1);
      resp->add_U32(timer->HasFlags(tfActive));
      resp->add_U32(timer->Recording());
      resp->add_U32(timer->Pending());
      resp->add_U32(timer->Priority());
      resp->add_U32(timer->Lifetime());
      resp->add_U32(timer->Channel()->Number());
      resp->add_U32(timer->StartTime());
      resp->add_U32(timer->StopTime());
      resp->add_U32(timer->Day());
      resp->add_U32(timer->WeekDays());
      resp->add_String(toUTF8.Convert(timer->File()));
    }
    else
      resp->add_U32(VDR_RET_DATAUNKNOWN);
  }
  else
    resp->add_U32(VDR_RET_DATAUNKNOWN);

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processTIMER_GetList() /* OPCODE 82 */
{
  cCharSetConv toUTF8;
  cTimer *timer;
  int numTimers = Timers.Count();

  resp->add_U32(numTimers);

  for (int i = 0; i < numTimers; i++)
  {
    timer = Timers.Get(i);
    if (!timer)
      continue;

    resp->add_U32(timer->Index()+1);
    resp->add_U32(timer->HasFlags(tfActive));
    resp->add_U32(timer->Recording());
    resp->add_U32(timer->Pending());
    resp->add_U32(timer->Priority());
    resp->add_U32(timer->Lifetime());
    resp->add_U32(timer->Channel()->Number());
    resp->add_U32(timer->StartTime());
    resp->add_U32(timer->StopTime());
    resp->add_U32(timer->Day());
    resp->add_U32(timer->WeekDays());
    resp->add_String(toUTF8.Convert(timer->File()));
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processTIMER_Add() /* OPCODE 83 */
{
  uint32_t flags      = req->extract_U32() > 0 ? tfActive : tfNone;
  uint32_t priority   = req->extract_U32();
  uint32_t lifetime   = req->extract_U32();
  uint32_t number     = req->extract_U32();
  time_t startTime    = req->extract_U32();
  time_t stopTime     = req->extract_U32();
  time_t day          = req->extract_U32();
  uint32_t weekdays   = req->extract_U32();
  const char *file    = req->extract_String();
  const char *aux     = req->extract_String();

  struct tm tm_r;
  struct tm *time = localtime_r(&startTime, &tm_r);
  if (day <= 0)
    day = cTimer::SetTime(startTime, 0);
  int start = time->tm_hour * 100 + time->tm_min;
  time = localtime_r(&stopTime, &tm_r);
  int stop = time->tm_hour * 100 + time->tm_min;

  cString buffer = cString::sprintf("%u:%i:%s:%04d:%04d:%d:%d:%s:%s\n", flags, number, *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux);

  cTimer *timer = new cTimer;
  if (timer->Parse(buffer))
  {
    cTimer *t = Timers.GetTimer(timer);
    if (!t)
    {
      Timers.Add(timer);
      Timers.SetModified();
      isyslog("VNSI: Timer %s added", *timer->ToDescr());
      resp->add_U32(VDR_RET_OK);
      resp->finalise();
      req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
      return true;
    }
    else
    {
      esyslog("VNSI-Error: Timer already defined: %d %s", t->Index() + 1, *t->ToText());
      resp->add_U32(VDR_RET_DATALOCKED);
    }
  }
  else
  {
    esyslog("VNSI-Error: Error in timer settings");
    resp->add_U32(VDR_RET_DATAINVALID);
  }

  delete timer;

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processTIMER_Delete() /* OPCODE 84 */
{
  uint32_t number = req->extract_U32();
  bool     force  = req->extract_U32();

  if (number <= 0 || number > Timers.Count())
  {
    esyslog("VNSI-Error: Unable to delete timer - invalid timer identifier");
    resp->add_U32(VDR_RET_DATAINVALID);
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
            esyslog("VNSI-Error: Timer \"%i\" is recording and can be deleted (use force=1 to stop it)", number);
            resp->add_U32(VDR_RET_RECRUNNING);
            resp->finalise();
            req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
            return true;
          }
        }
        isyslog("VNSI: Deleting timer %s", *timer->ToDescr());
        Timers.Del(timer);
        Timers.SetModified();
        resp->add_U32(VDR_RET_OK);
      }
      else
      {
        esyslog("VNSI-Error: Unable to delete timer - timers being edited at VDR");
        resp->add_U32(VDR_RET_DATALOCKED);
      }
    }
    else
    {
      esyslog("VNSI-Error: Unable to delete timer - invalid timer identifier");
      resp->add_U32(VDR_RET_DATAINVALID);
    }
  }
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processTIMER_Update() /* OPCODE 85 */
{
  int length      = req->getDataLength();
  uint32_t index  = req->extract_U32();
  bool active     = req->extract_U32();

  cTimer *timer = Timers.Get(index - 1);
  if (!timer)
  {
    esyslog("VNSI-Error: Timer \"%lu\" not defined", index);
    resp->add_U32(VDR_RET_DATAUNKNOWN);
    resp->finalise();
    req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
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
    uint32_t priority   = req->extract_U32();
    uint32_t lifetime   = req->extract_U32();
    uint32_t number     = req->extract_U32();
    time_t startTime    = req->extract_U32();
    time_t stopTime     = req->extract_U32();
    time_t day          = req->extract_U32();
    uint32_t weekdays   = req->extract_U32();
    const char *file    = req->extract_String();
    const char *aux     = req->extract_String();

    struct tm tm_r;
    struct tm *time = localtime_r(&startTime, &tm_r);
    if (day <= 0)
      day = cTimer::SetTime(startTime, 0);
    int start = time->tm_hour * 100 + time->tm_min;
    time = localtime_r(&stopTime, &tm_r);
    int stop = time->tm_hour * 100 + time->tm_min;

    cString buffer = cString::sprintf("%u:%i:%s:%04d:%04d:%d:%d:%s:%s\n", flags, number, *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file, aux);
    if (!t.Parse(buffer))
    {
      esyslog("VNSI-Error: Error in timer settings");
      resp->add_U32(VDR_RET_DATAINVALID);
      resp->finalise();
      req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
      return true;
    }
  }

  *timer = t;
  Timers.SetModified();

  resp->add_U32(VDR_RET_OK);
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}


/** OPCODE 100 - 119: VNSI network functions for recording access */

bool cCmdControl::processRECORDINGS_GetDiskSpace() /* OPCODE 100 */
{
  int FreeMB;
  int Percent = VideoDiskSpace(&FreeMB);
  int Total   = (FreeMB / (100 - Percent)) * 100;

  resp->add_U32(Total);
  resp->add_U32(FreeMB);
  resp->add_U32(Percent);

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processRECORDINGS_GetCount() /* OPCODE 101 */
{
  int count = 0;
  bool recordings = Recordings.Load();
  Recordings.Sort();
  if (recordings)
  {
    cRecording *recording = Recordings.Last();
    count = recording->Index() + 1;
  }

  resp->add_U32(count);

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processRECORDINGS_GetList() /* OPCODE 102 */
{
  cCharSetConv toUTF8;
  cRecordings Recordings;
  Recordings.Load();

  resp->add_String(VideoDirectory);
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
    LOGCONSOLE("GRI: RC: recordingStart=%lu recordingDuration=%lu", recordingStart, recordingDuration);

    resp->add_U32(recordingStart);
    resp->add_U32(recordingDuration);
    resp->add_U32(recording->priority);
    resp->add_U32(recording->lifetime);
    resp->add_String(recording->Info()->ChannelName() ? toUTF8.Convert(recording->Info()->ChannelName()) : "");
    if (!isempty(recording->Info()->Title()))
      resp->add_String(toUTF8.Convert(recording->Info()->Title()));
    else
      resp->add_String("");
    if (!isempty(recording->Info()->ShortText()))
      resp->add_String(toUTF8.Convert(recording->Info()->ShortText()));
    else
      resp->add_String("");
    if (!isempty(recording->Info()->Description()))
      resp->add_String(toUTF8.Convert(recording->Info()->Description()));
    else
      resp->add_String("");

    resp->add_String(toUTF8.Convert(recording->FileName()));
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processRECORDINGS_GetInfo() /* OPCODE 103 */
{
  cCharSetConv toUTF8;
  const char *fileName  = req->extract_String();

  cRecordings Recordings;
  Recordings.Load(); // probably have to do this

  cRecording *recording = Recordings.GetByName(fileName);
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
  LOGCONSOLE("GRI: RC: recordingStart=%lu recordingDuration=%lu", recordingStart, recordingDuration);

  resp->add_U32(recordingStart);
  resp->add_U32(recordingDuration);
  resp->add_U32(recording->priority);
  resp->add_U32(recording->lifetime);
  resp->add_String(recording->Info()->ChannelName() ? toUTF8.Convert(recording->Info()->ChannelName()) : "");
  if (!isempty(recording->Info()->Title()))
    resp->add_String(toUTF8.Convert(recording->Info()->Title()));
  else
    resp->add_String("");
  if (!isempty(recording->Info()->ShortText()))
    resp->add_String(toUTF8.Convert(recording->Info()->ShortText()));
  else
    resp->add_String("");
  if (!isempty(recording->Info()->Description()))
    resp->add_String(toUTF8.Convert(recording->Info()->Description()));
  else
    resp->add_String("");

#if APIVERSNUM < 10703
  resp->add_double((double)FRAMESPERSEC);
#else
  resp->add_double((double)recording->Info()->FramesPerSecond());
#endif

  if (event != NULL)
  {
    if (event->Vps())
      resp->add_U32(event->Vps());
    else
      resp->add_U32(0);
  }
  else
    resp->add_U32(0);

  const cComponents* components = recording->Info()->Components();
  if (components)
  {
    resp->add_U32(components->NumComponents());

    tComponent* component;
    for (int i = 0; i < components->NumComponents(); i++)
    {
      component = components->Component(i);

      LOGCONSOLE("GRI: C: %i %u %u %s %s", i, component->stream, component->type, component->language, component->description);

      resp->add_U8(component->stream);
      resp->add_U8(component->type);

      if (component->language)
        resp->add_String(component->language);
      else
        resp->add_String("");

      if (component->description)
        resp->add_String(component->description);
      else
        resp->add_String("");
    }
  }
  else
    resp->add_U32(0);

  // Done. send it
  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

  LOGCONSOLE("Written getrecinfo");
  return true;
}

bool cCmdControl::processRECORDINGS_Delete() /* OPCODE 104 */
{
  const char *recName = req->extract_String();

  cRecordings Recordings;
  Recordings.Load(); // probably have to do this

  cRecording* recording = Recordings.GetByName(recName);

  LOGCONSOLE("recording pointer %p", recording);

  if (recording)
  {
    LOGCONSOLE("deleting recording: %s", recording->Name());

    cRecordControl *rc = cRecordControls::GetRecordControl(recording->FileName());
    if (!rc)
    {
      if (recording->Delete())
      {
        // Copy svdrdeveldevelp's way of doing this, see if it works
        Recordings.DelByName(recording->FileName());
        isyslog("VNSI: Recording \"%s\" deleted", recording->FileName());
        resp->add_U32(VDR_RET_OK);
      }
      else
      {
        esyslog("VNSI-Error: Error while deleting recording!");
        resp->add_U32(VDR_RET_ERROR);
      }
    }
    else
    {
      esyslog("VNSI-Error: Recording \"%s\" is in use by timer %d", recording->Name(), rc->Timer()->Index() + 1);
      resp->add_U32(VDR_RET_DATALOCKED);
    }
  }
  else
  {
    esyslog("VNSI-Error: Error in recording name \"%s\"", recName);
    resp->add_U32(VDR_RET_DATAUNKNOWN);
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  return true;
}

bool cCmdControl::processRECORDINGS_Move() /* OPCODE 105 */
{
  LOGCONSOLE("Process move recording");
  const char *fileName  = req->extract_String();
  const char *newPath   = req->extract_String();

  cRecordings Recordings;
  Recordings.Load(); // probably have to do this

  cRecording* recording = Recordings.GetByName(fileName);

  LOGCONSOLE("recording pointer %p", recording);

  if (recording)
  {
    cRecordControl *rc = cRecordControls::GetRecordControl(recording->FileName());
    if (!rc)
    {
      LOGCONSOLE("moving recording: %s", recording->Name());
      LOGCONSOLE("moving recording: %s", recording->FileName());
      LOGCONSOLE("to: %s", newPath);

      const char* t = recording->FileName();

      char* dateDirName = NULL;   int k;
      char* titleDirName = NULL;  int j;

      // Find the datedirname
      for(k = strlen(t) - 1; k >= 0; k--)
      {
        if (t[k] == '/')
        {
          LOGCONSOLE("l1: %i", strlen(&t[k+1]) + 1);
          dateDirName = new char[strlen(&t[k+1]) + 1];
          strcpy(dateDirName, &t[k+1]);
          break;
        }
      }

      // Find the titledirname
      for(j = k-1; j >= 0; j--)
      {
        if (t[j] == '/')
        {
          LOGCONSOLE("l2: %i", (k - j - 1) + 1);
          titleDirName = new char[(k - j - 1) + 1];
          memcpy(titleDirName, &t[j+1], k - j - 1);
          titleDirName[k - j - 1] = '\0';
          break;
        }
      }

      LOGCONSOLE("datedirname: %s", dateDirName);
      LOGCONSOLE("titledirname: %s", titleDirName);
      LOGCONSOLE("viddir: %s", VideoDirectory);

      char* newPathConv = new char[strlen(newPath)+1];
      strcpy(newPathConv, newPath);
      ExchangeChars(newPathConv, true);
      LOGCONSOLE("EC: %s", newPathConv);

      char* newContainer = new char[strlen(VideoDirectory) + strlen(newPathConv) + strlen(titleDirName) + 1];
      LOGCONSOLE("l10: %i", strlen(VideoDirectory) + strlen(newPathConv) + strlen(titleDirName) + 1);
      sprintf(newContainer, "%s%s%s", VideoDirectory, newPathConv, titleDirName);
      delete[] newPathConv;

      LOGCONSOLE("%s", newContainer);

      struct stat dstat;
      int statret = stat(newContainer, &dstat);
      if ((statret == -1) && (errno == ENOENT)) // Dir does not exist
      {
        LOGCONSOLE("new dir does not exist");
        int mkdirret = mkdir(newContainer, 0755);
        if (mkdirret != 0)
        {
          delete[] dateDirName;
          delete[] titleDirName;
          delete[] newContainer;

          resp->add_U32(VDR_RET_ERROR);
          resp->finalise();
          req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
          return true;
        }
      }
      else if ((statret == 0) && (! (dstat.st_mode && S_IFDIR))) // Something exists but it's not a dir
      {
        delete[] dateDirName;
        delete[] titleDirName;
        delete[] newContainer;

        resp->add_U32(VDR_RET_ERROR);
        resp->finalise();
        req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
        return true;
      }

      // Ok, the directory container has been made, or it pre-existed.

      char* newDir = new char[strlen(newContainer) + 1 + strlen(dateDirName) + 1];
      sprintf(newDir, "%s/%s", newContainer, dateDirName);

      LOGCONSOLE("doing rename '%s' '%s'", t, newDir);
      int renameret = rename(t, newDir);
      if (renameret == 0)
      {
        // Success. Test for remove old dir containter
        char* oldTitleDir = new char[k+1];
        memcpy(oldTitleDir, t, k);
        oldTitleDir[k] = '\0';
        LOGCONSOLE("len: %i, cp: %i, strlen: %i, oldtitledir: %s", k+1, k, strlen(oldTitleDir), oldTitleDir);
        rmdir(oldTitleDir); // can't do anything about a fail result at this point.
        delete[] oldTitleDir;
      }
      if (renameret == 0)
      {
        // Tell VDR
        Recordings.Update();
        // Success. Send a different packet from just a ulong
        resp->add_U32(VDR_RET_OK); // success
        resp->add_String(newDir);
      }
      else
      {
        resp->add_U32(VDR_RET_ERROR);
      }

      resp->finalise();
      req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

      delete[] dateDirName;
      delete[] titleDirName;
      delete[] newContainer;
      delete[] newDir;
    }
    else
    {
      resp->add_U32(VDR_RET_DATALOCKED);
      resp->finalise();
      req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
    }
  }
  else
  {
    resp->add_U32(VDR_RET_DATAUNKNOWN);
    resp->finalise();
    req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());
  }

  return true;
}


/** OPCODE 120 - 139: VNSI network functions for epg access and manipulating */

bool cCmdControl::processEPG_GetForChannel() /* OPCODE 120 */
{
  uint32_t channelNumber  = req->extract_U32();
  uint32_t startTime      = req->extract_U32();
  uint32_t duration       = req->extract_U32();

  LOGCONSOLE("get schedule called for channel %lu", channelNumber);

  cChannel* channel = Channels.GetByNumber(channelNumber);
  if (!channel)
  {
    resp->add_U32(0);
    resp->finalise();
    req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

    LOGCONSOLE("written 0 because channel = NULL");
    return true;
  }

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);
  if (!Schedules)
  {
    resp->add_U32(0);
    resp->finalise();
    req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

    LOGCONSOLE("written 0 because Schedule!s! = NULL");
    return true;
  }

  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
  if (!Schedule)
  {
    resp->add_U32(0);
    resp->finalise();
    req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

    LOGCONSOLE("written 0 because Schedule = NULL");
    return true;
  }

  const char* empty = "";
  bool atLeastOneEvent = false;

  uint32_t thisEventID;
  uint32_t thisEventTime;
  uint32_t thisEventDuration;
  uint32_t thisEventContent;
  uint32_t thisEventRating;
  const char* thisEventTitle;
  const char* thisEventSubTitle;
  const char* thisEventDescription;
  cCharSetConv toUTF8;

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

    if (!thisEventTitle)        thisEventTitle        = empty;
    if (!thisEventSubTitle)     thisEventSubTitle     = empty;
    if (!thisEventDescription)  thisEventDescription  = empty;

    resp->add_U32(thisEventID);
    resp->add_U32(thisEventTime);
    resp->add_U32(thisEventDuration);
    resp->add_U32(thisEventContent);
    resp->add_U32(thisEventRating);

    resp->add_String(toUTF8.Convert(thisEventTitle));
    resp->add_String(toUTF8.Convert(thisEventSubTitle));
    resp->add_String(toUTF8.Convert(thisEventDescription));

    atLeastOneEvent = true;
  }

  LOGCONSOLE("Got all event data");

  if (!atLeastOneEvent)
  {
    resp->add_U32(0);
    LOGCONSOLE("Written 0 because no data");
  }

  resp->finalise();
  req->getClient()->GetSocket()->write(resp->getPtr(), resp->getLen());

  LOGCONSOLE("written schedules packet");

  return true;
}

