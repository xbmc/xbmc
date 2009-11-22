#pragma once
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

#include "../utils/Addon.h"
#include "../utils/PVREpg.h"
#include "../utils/PVRChannels.h"
#include "../utils/PVRTimers.h"
#include "../utils/PVRRecordings.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRManager;
class CEPG;

/**
* IPVRClientCallback Class
*/
class IPVRClientCallback
{
public:
  virtual void OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)=0;
};


/**
* IPVRClient PVR Client control class
*/
class IPVRClient : public ADDON::CAddon
{
public:
  IPVRClient(long clientID, const ADDON::CAddon& addon, IPVRClientCallback *pvrCB)
    : CAddon(addon) {};
  virtual ~IPVRClient(){};

  virtual long GetID(void)=0;
  virtual PVR_ERROR GetProperties(PVR_SERVERPROPS *props)=0;
  virtual ADDON_STATUS GetStatus(void)=0;
  virtual bool ReInit()=0;
  virtual bool ReadyToUse()=0;

  virtual const std::string GetBackendName(void)=0;
  virtual const std::string GetBackendVersion(void)=0;
  virtual const std::string GetConnectionString()=0;
  virtual PVR_ERROR GetDriveSpace(long long *total, long long *used)=0;

  virtual PVR_ERROR GetEPGForChannel(const cPVRChannelInfoTag &channelinfo, cPVREpg *epg, time_t start = NULL, time_t end = NULL)=0;

  virtual int GetNumChannels()=0;
  virtual PVR_ERROR GetChannelList(cPVRChannels &channels, bool radio)=0;
  virtual PVR_ERROR GetChannelSettings(cPVRChannelInfoTag *result)=0;
  virtual PVR_ERROR UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo)=0;
  virtual PVR_ERROR AddChannel(const cPVRChannelInfoTag &info)=0;
  virtual PVR_ERROR DeleteChannel(unsigned int number)=0;
  virtual PVR_ERROR RenameChannel(unsigned int number, CStdString &newname)=0;
  virtual PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber)=0;

  virtual int GetNumRecordings(void)=0;
  virtual PVR_ERROR GetAllRecordings(cPVRRecordings *results)=0;
  virtual PVR_ERROR DeleteRecording(const cPVRRecordingInfoTag &recinfo)=0;
  virtual PVR_ERROR RenameRecording(const cPVRRecordingInfoTag &recinfo, CStdString &newname)=0;

  virtual int GetNumTimers(void)=0;
  virtual PVR_ERROR GetAllTimers(cPVRTimers *results)=0;
  virtual PVR_ERROR AddTimer(const cPVRTimerInfoTag &timerinfo)=0;
  virtual PVR_ERROR DeleteTimer(const cPVRTimerInfoTag &timerinfo, bool force = false)=0;
  virtual PVR_ERROR RenameTimer(const cPVRTimerInfoTag &timerinfo, CStdString &newname)=0;
  virtual PVR_ERROR UpdateTimer(const cPVRTimerInfoTag &timerinfo)=0;

  virtual bool OpenLiveStream(const cPVRChannelInfoTag &channelinfo)=0;
  virtual void CloseLiveStream()=0;
  virtual int ReadLiveStream(BYTE* buf, int buf_size)=0;
  virtual __int64 SeekLiveStream(__int64 pos, int whence=SEEK_SET)=0;
  virtual __int64 LengthLiveStream(void)=0;
  virtual int GetCurrentClientChannel()=0;
  virtual bool SwitchChannel(const cPVRChannelInfoTag &channelinfo)=0;

  virtual bool OpenRecordedStream(const cPVRRecordingInfoTag &recinfo)=0;
  virtual void CloseRecordedStream(void)=0;
  virtual int ReadRecordedStream(BYTE* buf, int buf_size)=0;
  virtual __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET)=0;
  virtual __int64 LengthRecordedStream(void)=0;

  virtual bool OpenTVDemux(PVRDEMUXHANDLE handle, const cPVRChannelInfoTag &channelinfo)=0;
  virtual bool OpenRecordingDemux(PVRDEMUXHANDLE handle, const cPVRRecordingInfoTag &recinfo)=0;
  virtual void DisposeDemux()=0;
  virtual void ResetDemux()=0;
  virtual void FlushDemux()=0;
  virtual void AbortDemux()=0;
  virtual void SetDemuxSpeed(int iSpeed)=0;
  virtual demux_packet_t* ReadDemux()=0;
  virtual bool SeekDemuxTime(int time, bool backwords, double* startpts)=0;
  virtual int GetDemuxStreamLength()=0;
};
