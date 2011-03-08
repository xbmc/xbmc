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

#include "addons/Addon.h"
#include "addons/AddonDll.h"
#include "addons/DllPVRClient.h"
#include "pvr/epg/PVREpgContainer.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"

typedef std::vector<PVR_MENUHOOK> PVR_MENUHOOKS;

class IPVRClientCallback
{
public:
  virtual void OnClientMessage(const int clientID, const PVR_EVENT clientEvent, const char* msg)=0;
};

class CPVRClient : public ADDON::CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>
{
public:
  CPVRClient(const ADDON::AddonProps& props);
  CPVRClient(const cp_extension_t *ext);
  ~CPVRClient();

  bool Create(int clientID, IPVRClientCallback *pvrCB);
  void Destroy();
  bool ReCreate();

  /* DLL related */
  bool ReadyToUse() { return m_ReadyToUse; }
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue);

  /* Server */
  int GetID();
  PVR_ERROR GetProperties(PVR_SERVERPROPS *props);

  /* General */
  const std::string GetBackendName();
  const std::string GetBackendVersion();
  const std::string GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);
  PVR_ERROR StartChannelScan();
  int GetTimeCorrection() { return m_iTimeCorrection; }
  int GetClientID() { return m_pInfo->clientID; }
  bool HaveMenuHooks() { return m_menuhooks.size() > 0; }
  PVR_MENUHOOKS *GetMenuHooks() { return &m_menuhooks; }
  void CallMenuHook(const PVR_MENUHOOK &hook);

  /* TV Guide */
  PVR_ERROR GetEPGForChannel(const CPVRChannel &channelinfo, CPVREpg *epg, time_t start, time_t end, bool toDB = false);

  /* Channels */
  int GetNumChannels();
  PVR_ERROR GetChannelList(CPVRChannelGroup &channels, bool radio);

  /* Recordings */
  int GetNumRecordings(void);
  PVR_ERROR GetAllRecordings(CPVRRecordings *results);
  PVR_ERROR DeleteRecording(const CPVRRecording &recinfo);
  PVR_ERROR RenameRecording(const CPVRRecording &recinfo, const CStdString &newname);

  /* Timers */
  int GetNumTimers(void);
  PVR_ERROR GetAllTimers(CPVRTimers *results);
  PVR_ERROR AddTimer(const CPVRTimerInfoTag &timerinfo);
  PVR_ERROR DeleteTimer(const CPVRTimerInfoTag &timerinfo, bool force = false);
  PVR_ERROR RenameTimer(const CPVRTimerInfoTag &timerinfo, const CStdString &newname);
  PVR_ERROR UpdateTimer(const CPVRTimerInfoTag &timerinfo);

  bool OpenLiveStream(const CPVRChannel &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(void* lpBuf, int64_t uiBufSize);
  int64_t SeekLiveStream(int64_t iFilePosition, int iWhence = SEEK_SET);
  int64_t PositionLiveStream(void);
  int64_t LengthLiveStream(void);
  int GetCurrentClientChannel();
  bool SwitchChannel(const CPVRChannel &channelinfo);
  bool SignalQuality(PVR_SIGNALQUALITY &qualityinfo);
  const std::string GetLiveStreamURL(const CPVRChannel &channelinfo);

  bool OpenRecordedStream(const CPVRRecording &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(void* lpBuf, int64_t uiBufSize);
  int64_t SeekRecordedStream(int64_t iFilePosition, int iWhence = SEEK_SET);
  int64_t PositionRecordedStream(void);
  int64_t LengthRecordedStream(void);

  PVR_ERROR GetStreamProperties(PVR_STREAMPROPS *props);
  void DemuxReset();
  void DemuxAbort();
  void DemuxFlush();
  DemuxPacket* DemuxRead();

protected:
  bool                  m_ReadyToUse;
  IPVRClientCallback   *m_manager;
  CStdString            m_hostName;
  CCriticalSection      m_critSection;
  int                   m_iTimeCorrection;
  PVR_MENUHOOKS         m_menuhooks;

private:
  void WriteClientChannelInfo(const CPVRChannel &channelinfo, PVR_CHANNEL &tag);
  void WriteClientTimerInfo(const CPVRTimerInfoTag &timerinfo, PVR_TIMERINFO &tag);
  void WriteClientRecordingInfo(const CPVRRecording &recordinginfo, PVR_RECORDINGINFO &tag);
};

typedef std::vector<CPVRClient*> VECCLIENTS;
