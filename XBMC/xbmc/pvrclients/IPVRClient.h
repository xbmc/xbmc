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

#include "pvrclients/PVRClientTypes.h"

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
class IPVRClient
{
public:
  IPVRClient(long clientID, IPVRClientCallback *callback){};
  virtual ~IPVRClient(){};

  virtual PVR_ERROR GetProperties(PVR_SERVERPROPS *props)=0;
  virtual PVR_ERROR SetUserSetting(const char *settingName, const void *settingValue)=0;
  virtual PVR_ERROR Connect()=0;
  virtual void Disconnect()=0;
  virtual bool IsUp()=0;

  virtual const std::string GetBackendName()=0;
  virtual const std::string GetBackendVersion()=0;
  virtual PVR_ERROR GetDriveSpace(long long *total, long long *used)=0;

  virtual PVR_ERROR GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start = NULL, time_t end = NULL)=0;
  virtual PVR_ERROR GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result)=0;
  virtual PVR_ERROR GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)=0;

  virtual int GetNumChannels()=0;
  virtual PVR_ERROR GetChannelList(VECCHANNELS &channels, bool radio)=0;
  virtual PVR_ERROR GetChannelSettings(CTVChannelInfoTag *result)=0;
  virtual PVR_ERROR UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)=0;
  virtual PVR_ERROR AddChannel(const CTVChannelInfoTag &info)=0;
  virtual PVR_ERROR DeleteChannel(unsigned int number)=0;
  virtual PVR_ERROR RenameChannel(unsigned int number, CStdString &newname)=0;
  virtual PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber)=0;

  virtual int GetNumRecordings(void)=0;
  virtual PVR_ERROR GetAllRecordings(VECRECORDINGS *results)=0;
  virtual PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo)=0;
  virtual PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname)=0;

  virtual int GetNumTimers(void)=0;
  virtual PVR_ERROR GetAllTimers(VECTVTIMERS *results)=0;
  virtual PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo)=0;
  virtual PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false)=0;
  virtual PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)=0;
  virtual PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo)=0;

  virtual bool OpenLiveStream(unsigned int channel)=0;
  virtual void CloseLiveStream()=0;
  virtual int ReadLiveStream(BYTE* buf, int buf_size)=0;
  virtual int GetCurrentClientChannel()=0;
  virtual bool SwitchChannel(unsigned int channel)=0;

  virtual bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo)=0;
  virtual void CloseRecordedStream(void)=0;
  virtual int ReadRecordedStream(BYTE* buf, int buf_size)=0;
  virtual __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET)=0;
  virtual __int64 LengthRecordedStream(void)=0;
};

