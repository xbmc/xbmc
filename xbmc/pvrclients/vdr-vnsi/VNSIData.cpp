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
#include "vnsicommand.h"
#include "utils/StdString.h"

using namespace ADDON;
using namespace PLATFORM;

cVNSIData::cVNSIData()
 : m_aborting(false)
{
}

cVNSIData::~cVNSIData()
{
  Abort();
  StopThread();
  Close();
}

bool cVNSIData::Open(const std::string& hostname, int port, const char* name)
{
  m_aborting = false;

  if(!cVNSISession::Open(hostname, port, name))
    return false;

  return true;
}

bool cVNSIData::Login()
{
  if(!cVNSISession::Login())
    return false;

  CreateThread();
  return true;
}

void cVNSIData::Abort()
{
  CLockObject lock(m_mutex);
  m_aborting = true;
  cVNSISession::Abort();
}

void cVNSIData::SignalConnectionLost()
{
  CLockObject lock(m_mutex);

  if(m_aborting)
    return;

  cVNSISession::SignalConnectionLost();
}

void cVNSIData::OnDisconnect()
{
  XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30044));
  PVR->TriggerTimerUpdate();
}

void cVNSIData::OnReconnect()
{
  XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(30045));

  EnableStatusInterface(g_bHandleMessages);

  PVR->TriggerChannelUpdate();
  PVR->TriggerTimerUpdate();
  PVR->TriggerRecordingUpdate();
}

cResponsePacket* cVNSIData::ReadResult(cRequestPacket* vrp)
{
  m_mutex.Lock();

  SMessage &message(m_queue[vrp->getSerial()]);
  message.event = new CEvent;
  message.pkt   = NULL;

  m_mutex.Unlock();

  if(!cVNSISession::TransmitMessage(vrp))
  {
    m_queue.erase(vrp->getSerial());
    return NULL;
  }

  if (!message.event->Wait(g_iConnectTimeout * 1000))
  {
    XBMC->Log(LOG_ERROR, "%s - request timed out after %d seconds", __FUNCTION__, g_iConnectTimeout);
  }

  m_mutex.Lock();

  cResponsePacket* vresp = message.pkt;
  delete message.event;

  m_queue.erase(vrp->getSerial());

  m_mutex.Unlock();

  return vresp;
}

bool cVNSIData::GetDriveSpace(long long *total, long long *used)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECORDINGS_DISKSIZE))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
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
  if (!vrp.init(VNSI_SCAN_SUPPORTED))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VNSI_RET_OK ? true : false;
}

bool cVNSIData::EnableStatusInterface(bool onOff)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_ENABLESTATUSINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == VNSI_RET_OK ? true : false;
}

int cVNSIData::GetChannelsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_CHANNELS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cVNSIData::GetChannelsList(PVR_HANDLE handle, bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_CHANNELS_GETCHANNELS))
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
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
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

bool cVNSIData::GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_EPG_GETFORCHANNEL))
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
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return false;
  }

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
    delete[] tag.strTitle;
    delete[] tag.strPlotOutline;
    delete[] tag.strPlot;
  }

  delete vresp;
  return true;
}


/** OPCODE's 60 - 69: VNSI network functions for timer access */

int cVNSIData::GetTimersCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_TIMER_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cVNSIData::GetTimerInfo(unsigned int timernumber, PVR_TIMER &tag)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_TIMER_GET))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  if (!vrp.add_U32(timernumber))
    return PVR_ERROR_UNKNOWN;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode != VNSI_RET_OK)
  {
    delete vresp;
    if (returnCode == VNSI_RET_DATAUNKNOWN)
      return PVR_ERROR_NOT_POSSIBLE;
    else if (returnCode == VNSI_RET_ERROR)
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

bool cVNSIData::GetTimersList(PVR_HANDLE handle)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_TIMER_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
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

PVR_ERROR cVNSIData::AddTimer(const PVR_TIMER &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_TIMER_ADD))
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
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == VNSI_RET_DATALOCKED)
    return PVR_ERROR_ALREADY_PRESENT;
  else if (returnCode == VNSI_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == VNSI_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::DeleteTimer(const PVR_TIMER &timerinfo, bool force)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_TIMER_DELETE))
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

  if (returnCode == VNSI_RET_DATALOCKED)
    return PVR_ERROR_NOT_DELETED;
  if (returnCode == VNSI_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == VNSI_RET_DATAINVALID)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VNSI_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::RenameTimer(const PVR_TIMER &timerinfo, const char *newname)
{
  PVR_TIMER timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.iClientIndex, timerinfo1);
  if (ret != PVR_ERROR_NO_ERROR)
    return ret;

  timerinfo1.strTitle = newname;
  return UpdateTimer(timerinfo1);
}

PVR_ERROR cVNSIData::UpdateTimer(const PVR_TIMER &timerinfo)
{
  // use timer margin to calculate start/end times
  uint32_t starttime = timerinfo.startTime - timerinfo.iMarginStart*60;
  uint32_t endtime = timerinfo.endTime + timerinfo.iMarginEnd*60;

  cRequestPacket vrp;
  if (!vrp.init(VNSI_TIMER_UPDATE))        return PVR_ERROR_UNKNOWN;
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
  if (returnCode == VNSI_RET_DATAUNKNOWN)
    return PVR_ERROR_NOT_POSSIBLE;
  else if (returnCode == VNSI_RET_DATAINVALID)
    return PVR_ERROR_NOT_SAVED;
  else if (returnCode == VNSI_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

int cVNSIData::GetRecordingsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECORDINGS_GETCOUNT))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cVNSIData::GetRecordingsList(PVR_HANDLE handle)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECORDINGS_GETLIST))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  cResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't get response packed", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  CStdString strRecordingId;
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
    strRecordingId.Format("%i", vresp->extract_U32());
    tag.strRecordingId  = strRecordingId.c_str();
    tag.strStreamURL    = "";

    PVR->TransferRecordingEntry(handle, &tag);

    delete[] tag.strChannelName;
    delete[] tag.strTitle;
    delete[] tag.strPlotOutline;
    delete[] tag.strPlot;
    delete[] tag.strDirectory;
  }

  delete vresp;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cVNSIData::RenameRecording(const PVR_RECORDING& recinfo, const char* newname)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECORDINGS_RENAME))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // add uid
  XBMC->Log(LOG_DEBUG, "%s - uid: %s", __FUNCTION__, recinfo.strRecordingId);
  if (!vrp.add_U32(atoi(recinfo.strRecordingId)))
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

PVR_ERROR cVNSIData::DeleteRecording(const PVR_RECORDING& recinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_RECORDINGS_DELETE))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  if (!vrp.add_U32(atoi(recinfo.strRecordingId)))
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
    case VNSI_RET_DATALOCKED:
      return PVR_ERROR_NOT_DELETED;

    case VNSI_RET_RECRUNNING:
      return PVR_ERROR_RECORDING_RUNNING;

    case VNSI_RET_DATAINVALID:
      return PVR_ERROR_NOT_POSSIBLE;

    case VNSI_RET_ERROR:
      return PVR_ERROR_SERVER_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

bool cVNSIData::OnResponsePacket(cResponsePacket* pkt)
{
  return false;
}

bool cVNSIData::SendPing()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  cRequestPacket vrp;
  if (!vrp.init(VNSI_PING))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cResponsePacket* vresp = cVNSISession::ReadResult(&vrp);
  delete vresp;

  return (vresp != NULL);
}

void *cVNSIData::Process()
{
  uint32_t lastPing = 0;

  cResponsePacket* vresp;

  while (!IsStopped())
  {
    // try to reconnect
    if(ConnectionLost() && !TryReconnect())
    {
      Sleep(1000);
      continue;
    }

    // if there's anything in the buffer, read it
    if ((vresp = cVNSISession::ReadMessage(5)) == NULL)
    {
      Sleep(5);
      continue;
    }

    // check if the connection is still up
    if (vresp == NULL)
    {
      if(time(NULL) - lastPing > 5)
      {
        lastPing = time(NULL);

//        if(!SendPing())
//          SignalConnectionLost();
      }
      continue;
    }

    // CHANNEL_REQUEST_RESPONSE

    if (vresp->getChannelID() == VNSI_CHANNEL_REQUEST_RESPONSE)
    {
      CLockObject lock(m_mutex);
      SMessages::iterator it = m_queue.find(vresp->getRequestID());
      if (it != m_queue.end())
      {
        it->second.pkt = vresp;
        it->second.event->Broadcast();
      }
      else
      {
        delete vresp;
      }
    }

    // CHANNEL_STATUS

    else if (vresp->getChannelID() == VNSI_CHANNEL_STATUS)
    {
      if (vresp->getRequestID() == VNSI_STATUS_MESSAGE)
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
      else if (vresp->getRequestID() == VNSI_STATUS_RECORDING)
      {
                          vresp->extract_U32(); // device currently unused
        uint32_t on     = vresp->extract_U32();
        char* str1      = vresp->extract_String();
        char* str2      = vresp->extract_String();

        PVR->Recording(str1, str2, on!=0?true:false);
        PVR->TriggerTimerUpdate();

        delete[] str1;
        delete[] str2;
      }
      else if (vresp->getRequestID() == VNSI_STATUS_TIMERCHANGE)
      {
        XBMC->Log(LOG_DEBUG, "Server requested timer update");
        PVR->TriggerTimerUpdate();
      }
      else if (vresp->getRequestID() == VNSI_STATUS_CHANNELCHANGE)
      {
        XBMC->Log(LOG_DEBUG, "Server requested channel update");
        PVR->TriggerChannelUpdate();
      }
      else if (vresp->getRequestID() == VNSI_STATUS_RECORDINGSCHANGE)
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
  return NULL;
}

int cVNSIData::GetChannelGroupCount(bool automatic)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_CHANNELGROUP_GETCOUNT))
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

bool cVNSIData::GetChannelGroupList(PVR_HANDLE handle, bool bRadio)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_CHANNELGROUP_LIST))
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
    tag.bIsRadio = vresp->extract_U8()!=0?true:false;
    PVR->TransferChannelGroup(handle, &tag);

    delete[] tag.strGroupName;
  }

  delete vresp;
  return true;
}

bool cVNSIData::GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_CHANNELGROUP_MEMBERS))
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
