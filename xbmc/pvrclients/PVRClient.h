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

#include "IPVRClient.h"
#include "../utils/Addon.h"
#include "DllPVRClient.h"
#include "../addons/lib/addon_local.h"

class CPVRClient : public IPVRClient
{
public:
  CPVRClient(const long clientID, struct PVRClient* pClient, DllPVRClient* pDll,
               const ADDON::CAddon& addon, IPVRClientCallback *pvrCB);
  ~CPVRClient();

  /* DLL related */
  bool Init();
  void DeInit();
  bool ReInit();
  bool ReadyToUse() { return m_ReadyToUse; }
  virtual ADDON_STATUS GetStatus();
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue);

  /* Server */
  virtual long GetID();
  virtual PVR_ERROR GetProperties(PVR_SERVERPROPS *props);

  /* General */
  virtual const std::string GetBackendName();
  virtual const std::string GetBackendVersion();
  virtual const std::string GetConnectionString();
  virtual PVR_ERROR GetDriveSpace(long long *total, long long *used);

  /* TV Guide */
  virtual PVR_ERROR GetEPGForChannel(const cPVRChannelInfoTag &channelinfo, cPVREpg *epg, time_t start, time_t end);

  /* Channels */
  virtual int GetNumChannels();
  virtual PVR_ERROR GetChannelList(cPVRChannels &channels, bool radio);
  virtual PVR_ERROR GetChannelSettings(cPVRChannelInfoTag *result);
  virtual PVR_ERROR UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo);
  virtual PVR_ERROR AddChannel(const cPVRChannelInfoTag &info);
  virtual PVR_ERROR DeleteChannel(unsigned int number);
  virtual PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
  virtual PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);

  /* Recordings */
  virtual int GetNumRecordings(void);
  virtual PVR_ERROR GetAllRecordings(cPVRRecordings *results);
  virtual PVR_ERROR DeleteRecording(const cPVRRecordingInfoTag &recinfo);
  virtual PVR_ERROR RenameRecording(const cPVRRecordingInfoTag &recinfo, CStdString &newname);

  /* Timers */
  virtual int GetNumTimers(void);
  virtual PVR_ERROR GetAllTimers(cPVRTimers *results);
  virtual PVR_ERROR AddTimer(const cPVRTimerInfoTag &timerinfo);
  virtual PVR_ERROR DeleteTimer(const cPVRTimerInfoTag &timerinfo, bool force = false);
  virtual PVR_ERROR RenameTimer(const cPVRTimerInfoTag &timerinfo, CStdString &newname);
  virtual PVR_ERROR UpdateTimer(const cPVRTimerInfoTag &timerinfo);

  virtual bool OpenLiveStream(const cPVRChannelInfoTag &channelinfo);
  virtual void CloseLiveStream();
  virtual int ReadLiveStream(BYTE* buf, int buf_size);
  virtual __int64 SeekLiveStream(__int64 pos, int whence=SEEK_SET);
  virtual __int64 LengthLiveStream(void);
  virtual int GetCurrentClientChannel();
  virtual bool SwitchChannel(const cPVRChannelInfoTag &channelinfo);
  virtual bool SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  virtual bool OpenRecordedStream(const cPVRRecordingInfoTag &recinfo);
  virtual void CloseRecordedStream(void);
  virtual int ReadRecordedStream(BYTE* buf, int buf_size);
  virtual __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  virtual __int64 LengthRecordedStream(void);

protected:
  std::auto_ptr<struct PVRClient> m_pClient;
  std::auto_ptr<DllPVRClient> m_pDll;
  const                 long m_clientID;
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
