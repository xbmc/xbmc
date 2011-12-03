/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#include "XVDRData.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "xvdrcommand.h"

extern "C" {
#include "libTcpSocket/os-dependent_socket.h"
}

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

cXVDRData::cXVDRData()
 : m_aborting(false)
{
}

cXVDRData::~cXVDRData()
{
  Abort();
  Cancel(1);
  Close();
}

bool cXVDRData::Open(const std::string& hostname, int port, const char* name)
{
  m_aborting = false;

  if(!cXVDRSession::Open(hostname, port, name))
    return false;

  if(name != NULL) {
    SetDescription(name);
  }
  return true;
}

bool cXVDRData::Login()
{
  if(!cXVDRSession::Login())
    return false;

  Start();
  return true;
}

void cXVDRData::Abort()
{
  CMD_LOCK;
  m_aborting = true;
  cXVDRSession::Abort();
}

void cXVDRData::SignalConnectionLost()
{
  CMD_LOCK;

  if(m_aborting)
    return;

  cXVDRSession::SignalConnectionLost();
}

void cXVDRData::OnDisconnect()
{
  XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30044));
  PVR->TriggerTimerUpdate();
}

void cXVDRData::OnReconnect()
{
  XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(30045));

  EnableStatusInterface(g_bHandleMessages);

  PVR->TriggerChannelUpdate();
  PVR->TriggerTimerUpdate();
  PVR->TriggerRecordingUpdate();
}

cResponsePacket* cXVDRData::ReadResult(cRequestPacket* vrp)
{
  m_Mutex.Lock();

  SMessage &message(m_queue[vrp->getSerial()]);
  message.event = new cCondWait();
  message.pkt   = NULL;

  m_Mutex.Unlock();

  if(!cXVDRSession::SendMessage(vrp))
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

bool cXVDRData::GetDriveSpace(long long *total, long long *used)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_DISKSIZE))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
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

bool cXVDRData::SupportChannelScan()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_SCAN_SUPPORTED))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == XVDR_RET_OK ? true : false;
}

bool cXVDRData::EnableStatusInterface(bool onOff)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_ENABLESTATUSINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == XVDR_RET_OK ? true : false;
}

bool cXVDRData::SetUpdateChannels(uint8_t method)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_UPDATECHANNELS)) return false;
  if (!vrp.add_U8(method)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_INFO, "Setting channel update method not supported by server. Consider updating the XVDR server.");
    return false;
  }

  XBMC->Log(LOG_INFO, "Channel update method set to %i", method);

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == XVDR_RET_OK ? true : false;
}

int cXVDRData::GetChannelsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cXVDRData::GetChannelsList(PVR_HANDLE handle, bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELS_GETCHANNELS))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }
  if (!vrp.add_U32(radio))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't add parameter to cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(tag));

    tag.iChannelNumber    = vresp->extract_U32();
    tag.strChannelName    = vresp->extract_String();
    tag.iUniqueId         = vresp->extract_U32();
                            vresp->extract_U32(); // still here for compatibility
    tag.iEncryptionSystem = vresp->extract_U32();
                            vresp->extract_U32(); // uint32_t vtype - currently unused
    tag.bIsRadio          = radio;
    tag.strInputFormat    = "";
    tag.strStreamURL      = "";
    tag.strIconPath       = "";
    tag.bIsHidden         = false;

    PVR->TransferChannelEntry(handle, &tag);
    delete[] tag.strChannelName;
  }

  delete vresp;
  return true;
}

bool cXVDRData::GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_EPG_GETFORCHANNEL))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }
  if (!vrp.add_U32(channel.iUniqueId) || !vrp.add_U32(start) || !vrp.add_U32(end - start))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't add parameter to cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  if (!vresp->serverError())
  {
    while (!vresp->end())
    {
      EPG_TAG tag;
      memset(&tag, 0 , sizeof(tag));

      tag.iChannelNumber      = channel.iChannelNumber;
      tag.iUniqueBroadcastId  = vresp->extract_U32();
      tag.startTime           = vresp->extract_U32();
      tag.endTime             = tag.startTime + vresp->extract_U32();
      uint32_t content        = vresp->extract_U32();
      tag.iGenreType          = content & 0xF0;
      tag.iGenreSubType       = content & 0x0F;
      tag.strGenreDescription = "";
      tag.iParentalRating     = vresp->extract_U32();
      tag.strTitle            = vresp->extract_String();
      tag.strPlotOutline      = vresp->extract_String();
      tag.strPlot             = vresp->extract_String();

      PVR->TransferEpgEntry(handle, &tag);
      if (tag.strTitle)
        delete[] tag.strTitle;
      if (tag.strPlotOutline)
        delete[] tag.strPlotOutline;
      if (tag.strPlot)
        delete[] tag.strPlot;
    }
  }

  delete vresp;
  return true;
}


/** OPCODE's 60 - 69: XVDR network functions for timer access */

int cXVDRData::GetTimersCount()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cXVDRData::GetTimerInfo(unsigned int timernumber, PVR_TIMER &tag)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GET))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  if (!vrp.add_U32(timernumber))
    return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode != XVDR_RET_OK)
  {
    delete vresp;
    if (returnCode == XVDR_RET_DATAUNKNOWN)
      return PVR_ERROR_NOT_POSSIBLE;
    else if (returnCode == XVDR_RET_ERROR)
      return PVR_ERROR_SERVER_ERROR;
  }

  tag.iClientIndex      = vresp->extract_U32();
  int iActive           = vresp->extract_U32();
  int iRecording        = vresp->extract_U32();
  int iPending          = vresp->extract_U32();
  if (iRecording)
    tag.state = PVR_TIMER_STATE_RECORDING;
  else if (iPending || iActive)
    tag.state = PVR_TIMER_STATE_SCHEDULED;
  else
    tag.state = PVR_TIMER_STATE_CANCELLED;
  tag.iPriority         = vresp->extract_U32();
  tag.iLifetime         = vresp->extract_U32();
                          vresp->extract_U32(); // channel number - unused
  tag.iClientChannelUid = vresp->extract_U32();
  tag.startTime         = vresp->extract_U32();
  tag.endTime           = vresp->extract_U32();
  tag.firstDay          = vresp->extract_U32();
  tag.iWeekdays         = vresp->extract_U32();
  tag.bIsRepeating      = tag.iWeekdays == 0 ? false : true;
  tag.strTitle          = vresp->extract_String();
  tag.strDirectory            = "";

  delete[] tag.strTitle;
  delete vresp;
  return PVR_ERROR_NO_ERROR;
}

bool cXVDRData::GetTimersList(PVR_HANDLE handle)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t numTimers = vresp->extract_U32();
  if (numTimers > 0)
  {
    while (!vresp->end())
    {
      PVR_TIMER tag;
      tag.iClientIndex      = vresp->extract_U32();
      int iActive           = vresp->extract_U32();
      int iRecording        = vresp->extract_U32();
      int iPending          = vresp->extract_U32();
      if (iRecording)
        tag.state = PVR_TIMER_STATE_RECORDING;
      else if (iPending || iActive)
        tag.state = PVR_TIMER_STATE_SCHEDULED;
      else
        tag.state = PVR_TIMER_STATE_CANCELLED;
      tag.iPriority         = vresp->extract_U32();
      tag.iLifetime         = vresp->extract_U32();
                              vresp->extract_U32(); // channel number - unused
      tag.iClientChannelUid = vresp->extract_U32();
      tag.startTime         = vresp->extract_U32();
      tag.endTime           = vresp->extract_U32();
      tag.firstDay          = vresp->extract_U32();
      tag.iWeekdays         = vresp->extract_U32();
      tag.bIsRepeating      = tag.iWeekdays == 0 ? false : true;
      tag.strTitle          = vresp->extract_String();
      tag.strDirectory      = "";
      tag.iMarginStart      = 0;
      tag.iMarginEnd        = 0;

      PVR->TransferTimerEntry(handle, &tag);
      delete[] tag.strTitle;
    }
  }
  delete vresp;
  return true;
}

PVR_ERROR cXVDRData::AddTimer(const PVR_TIMER &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_ADD))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // add directory in front of the title
  std::string path;
  if(timerinfo.strDirectory != NULL && strlen(timerinfo.strDirectory) > 0) {
    path += timerinfo.strDirectory;
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

  if(timerinfo.strTitle != NULL) {
    path += timerinfo.strTitle;
  }

  // replace directory separators
  for(std::size_t i=0; i<path.size(); i++) {
    if(path[i] == '/' || path[i] == '\\') {
      path[i] = '~';
    }
  }

  if(path.empty()) {
    XBMC->Log(LOG_ERROR, "%s - Empty filename !", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // use timer margin to calculate start/end times
  uint32_t starttime = timerinfo.startTime - timerinfo.iMarginStart*60;
  uint32_t endtime = timerinfo.endTime + timerinfo.iMarginEnd*60;

  if (!vrp.add_U32(timerinfo.state == PVR_TIMER_STATE_SCHEDULED))     return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iPriority))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iLifetime))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iClientChannelUid)) return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(starttime))  return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(endtime))    return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.bIsRepeating ? timerinfo.firstDay : 0))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iWeekdays))return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(path.c_str()))      return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == XVDR_RET_DATALOCKED)
    return PVR_ERROR_ALREADY_PRESENT;
  else if (returnCode == XVDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == XVDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::DeleteTimer(const PVR_TIMER &timerinfo, bool force)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_DELETE))
    return PVR_ERROR_UNKNOWN;

  if (!vrp.add_U32(timerinfo.iClientIndex))
    return PVR_ERROR_UNKNOWN;

  if (!vrp.add_U32(force))
    return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if (returnCode == XVDR_RET_DATALOCKED)
    return PVR_ERROR_NOT_DELETED;
  if (returnCode == XVDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == XVDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == XVDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::RenameTimer(const PVR_TIMER &timerinfo, const char *newname)
{
  PVR_TIMER timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.iClientIndex, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.strTitle = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR cXVDRData::UpdateTimer(const PVR_TIMER &timerinfo)
{
  // use timer margin to calculate start/end times
  uint32_t starttime = timerinfo.startTime - timerinfo.iMarginStart*60;
  uint32_t endtime = timerinfo.endTime + timerinfo.iMarginEnd*60;

  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_UPDATE))        return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iClientIndex))      return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.state == PVR_TIMER_STATE_SCHEDULED))     return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iPriority))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iLifetime))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iClientChannelUid)) return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(starttime))  return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(endtime))    return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.bIsRepeating ? timerinfo.firstDay : 0))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iWeekdays))return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(timerinfo.strTitle))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == XVDR_RET_DATAUNKNOWN)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == XVDR_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == XVDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

int cXVDRData::GetRecordingsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cXVDRData::GetRecordingsList(PVR_HANDLE handle)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  while (!vresp->end())
  {
    PVR_RECORDING tag;
    tag.recordingTime   = vresp->extract_U32();
    tag.iDuration       = vresp->extract_U32();
    tag.iPriority       = vresp->extract_U32();
    tag.iLifetime       = vresp->extract_U32();
    tag.strChannelName  = vresp->extract_String();
    tag.strTitle        = vresp->extract_String();
    tag.strPlotOutline  = vresp->extract_String();
    tag.strPlot         = vresp->extract_String();
    tag.strDirectory    = vresp->extract_String();
    tag.strRecordingId  = vresp->extract_String();
    tag.strStreamURL    = "";
    tag.iGenreType      = 0;
    tag.iGenreSubType   = 0;

    PVR->TransferRecordingEntry(handle, &tag);

    delete[] tag.strChannelName;
    delete[] tag.strTitle;
    delete[] tag.strPlotOutline;
    delete[] tag.strPlot;
    delete[] tag.strDirectory;
    delete[] tag.strRecordingId;
  }

  delete vresp;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::RenameRecording(const PVR_RECORDING& recinfo, const char* newname)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_RENAME))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // add uid
  XBMC->Log(LOG_DEBUG, "%s - uid: %s", __FUNCTION__, recinfo.strRecordingId);
  if (!vrp.add_String(recinfo.strRecordingId))
    return PVR_ERROR_UNKNOWN;

  // add new title
  if (!vrp.add_String(newname))
    return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_SERVER_ERROR;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if(returnCode != 0)
   return PVR_ERROR_NOT_POSSIBLE;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::DeleteRecording(const PVR_RECORDING& recinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_DELETE))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  if (!vrp.add_String(recinfo.strRecordingId))
    return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  switch(returnCode)
  {
    case XVDR_RET_DATALOCKED:
      return PVR_ERROR_NOT_DELETED;

    case XVDR_RET_RECRUNNING:
      return PVR_ERROR_RECORDING_RUNNING;

    case XVDR_RET_DATAINVALID:
      return PVR_ERROR_NOT_POSSIBLE;

    case XVDR_RET_ERROR:
      return PVR_ERROR_SERVER_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

bool cXVDRData::OnResponsePacket(cResponsePacket* pkt)
{
  return false;
}

bool cXVDRData::SendPing()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  cRequestPacket vrp;
  if (!vrp.init(XVDR_PING))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = cXVDRSession::ReadResult(&vrp);
  delete vresp;

  return (vresp != NULL);
}

void cXVDRData::Action()
{
  uint32_t lastPing = 0;

  cResponsePacket* vresp;

  while (Running())
  {
    // try to reconnect
    if(ConnectionLost() && !TryReconnect())
    {
      SleepMs(1000);
      continue;
   }

    // read message
    vresp = cXVDRSession::ReadMessage();

    // check if the connection is still up
    if (vresp == NULL)
    {
      if(time(NULL) - lastPing > 5)
      {
        lastPing = time(NULL);

        if(!SendPing())
          SignalConnectionLost();
      }
      continue;
    }

    // CHANNEL_REQUEST_RESPONSE

    if (vresp->getChannelID() == XVDR_CHANNEL_REQUEST_RESPONSE)
    {

      CMD_LOCK;
      SMessages::iterator it = m_queue.find(vresp->getRequestID());
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

    else if (vresp->getChannelID() == XVDR_CHANNEL_STATUS)
    {
      if (vresp->getRequestID() == XVDR_STATUS_MESSAGE)
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
      else if (vresp->getRequestID() == XVDR_STATUS_RECORDING)
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
      else if (vresp->getRequestID() == XVDR_STATUS_TIMERCHANGE)
      {
        XBMC->Log(LOG_DEBUG, "Server requested timer update");
        PVR->TriggerTimerUpdate();
      }
      else if (vresp->getRequestID() == XVDR_STATUS_CHANNELCHANGE)
      {
        XBMC->Log(LOG_DEBUG, "Server requested channel update");
        PVR->TriggerChannelUpdate();
      }
      else if (vresp->getRequestID() == XVDR_STATUS_RECORDINGSCHANGE)
      {
        XBMC->Log(LOG_DEBUG, "Server requested recordings update");
        PVR->TriggerRecordingUpdate();
      }

      delete vresp;
    }

    // UNKOWN CHANNELID

    else if (!OnResponsePacket(vresp))
    {
      XBMC->Log(LOG_ERROR, "%s - Rxd a response packet on channel %lu !!", __FUNCTION__, vresp->getChannelID());
      delete vresp;
    }
  }
}

int cXVDRData::GetChannelGroupCount(bool automatic)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return 0;
  }

  if (!vrp.add_U32(automatic))
  {
    return 0;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return 0;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cXVDRData::GetChannelGroupList(PVR_HANDLE handle, bool bRadio)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_LIST))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  vrp.add_U8(bRadio);

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL_GROUP tag;

    tag.strGroupName = vresp->extract_String();
    tag.bIsRadio = vresp->extract_U8();
    PVR->TransferChannelGroup(handle, &tag);

    delete[] tag.strGroupName;
  }

  delete vresp;
  return true;
}

bool cXVDRData::GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_MEMBERS))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  vrp.add_String(group.strGroupName);
  vrp.add_U8(group.bIsRadio);

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL_GROUP_MEMBER tag;
    tag.strGroupName = group.strGroupName;
    tag.iChannelUniqueId = vresp->extract_U32();
    tag.iChannelNumber = vresp->extract_U32();

    PVR->TransferChannelGroupMember(handle, &tag);
  }

  delete vresp;
  return true;
}
