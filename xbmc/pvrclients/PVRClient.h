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
#include "../utils/AddonDll.h"
#include "DllPVRClient.h"
#include "../addons/lib/addon_local.h"

class IPVRClientCallback
{
public:
  virtual void OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)=0;
};

class CPVRClient : public ADDON::CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>
{
public:
  CPVRClient(const ADDON::AddonProps& props);
  ~CPVRClient();

  bool Create(long clientID, IPVRClientCallback *pvrCB);
  void Destroy();
  bool ReCreate();

  /* DLL related */
  bool ReadyToUse() { return m_ReadyToUse; }
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue);

  /* Server */
  long GetID();
  PVR_ERROR GetProperties(PVR_SERVERPROPS *props);

  /* General */
  const std::string GetBackendName();
  const std::string GetBackendVersion();
  const std::string GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);

  /* TV Guide */
  PVR_ERROR GetEPGForChannel(const cPVRChannelInfoTag &channelinfo, cPVREpg *epg, time_t start, time_t end);

  /* Channels */
  int GetNumChannels();
  PVR_ERROR GetChannelList(cPVRChannels &channels, bool radio);
  PVR_ERROR GetChannelSettings(cPVRChannelInfoTag *result);
  PVR_ERROR UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo);
  PVR_ERROR AddChannel(const cPVRChannelInfoTag &info);
  PVR_ERROR DeleteChannel(unsigned int number);
  PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);

  /* Recordings */
  int GetNumRecordings(void);
  PVR_ERROR GetAllRecordings(cPVRRecordings *results);
  PVR_ERROR DeleteRecording(const cPVRRecordingInfoTag &recinfo);
  PVR_ERROR RenameRecording(const cPVRRecordingInfoTag &recinfo, CStdString &newname);

  /* Timers */
  int GetNumTimers(void);
  PVR_ERROR GetAllTimers(cPVRTimers *results);
  PVR_ERROR AddTimer(const cPVRTimerInfoTag &timerinfo);
  PVR_ERROR DeleteTimer(const cPVRTimerInfoTag &timerinfo, bool force = false);
  PVR_ERROR RenameTimer(const cPVRTimerInfoTag &timerinfo, CStdString &newname);
  PVR_ERROR UpdateTimer(const cPVRTimerInfoTag &timerinfo);

  bool OpenLiveStream(const cPVRChannelInfoTag &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(BYTE* buf, int buf_size);
  __int64 SeekLiveStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthLiveStream(void);
  int GetCurrentClientChannel();
  bool SwitchChannel(const cPVRChannelInfoTag &channelinfo);
  bool SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  bool OpenRecordedStream(const cPVRRecordingInfoTag &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);

protected:
  bool                  m_ReadyToUse;
  IPVRClientCallback   *m_manager;
  AddonCB              *m_callbacks;
  CStdString            m_hostName;
  CCriticalSection      m_critSection;

private:
  void WriteClientChannelInfo(const cPVRChannelInfoTag &channelinfo, PVR_CHANNEL &tag);
  void WriteClientTimerInfo(const cPVRTimerInfoTag &timerinfo, PVR_TIMERINFO &tag);
  void WriteClientRecordingInfo(const cPVRRecordingInfoTag &recordinginfo, PVR_RECORDINGINFO &tag);
  static void PVRTransferEpgEntry(void *userData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
  static void PVRTransferChannelEntry(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *channel);
  static void PVRTransferTimerEntry(void *userData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
  static void PVRTransferRecordingEntry(void *userData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
  static void PVREventCallback(void *userData, const PVR_EVENT pvrevent, const char *msg);
};

typedef std::vector<CPVRClient*> VECCLIENTS;
