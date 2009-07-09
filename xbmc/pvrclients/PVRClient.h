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
  void ReInit();
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
  virtual PVR_ERROR GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end);
  virtual PVR_ERROR GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result);
  virtual PVR_ERROR GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result);

  /* Channels */
  virtual int GetNumChannels();
  virtual PVR_ERROR GetChannelList(VECCHANNELS &channels, bool radio);
  virtual PVR_ERROR GetChannelSettings(CTVChannelInfoTag *result);
  virtual PVR_ERROR UpdateChannelSettings(const CTVChannelInfoTag &chaninfo);
  virtual PVR_ERROR AddChannel(const CTVChannelInfoTag &info);
  virtual PVR_ERROR DeleteChannel(unsigned int number);
  virtual PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
  virtual PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);

  /* Recordings */
  virtual int GetNumRecordings(void);
  virtual PVR_ERROR GetAllRecordings(VECRECORDINGS *results);
  virtual PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo);
  virtual PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname);

  /* Timers */
  virtual int GetNumTimers(void);
  virtual PVR_ERROR GetAllTimers(VECTVTIMERS *results);
  virtual PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo);
  virtual PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false);
  virtual PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname);
  virtual PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo);

  virtual bool OpenLiveStream(unsigned int channel);
  virtual void CloseLiveStream();
  virtual int ReadLiveStream(BYTE* buf, int buf_size);
  virtual int GetCurrentClientChannel();
  virtual bool SwitchChannel(unsigned int channel);

  virtual bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo);
  virtual void CloseRecordedStream(void);
  virtual int ReadRecordedStream(BYTE* buf, int buf_size);
  virtual __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  virtual __int64 LengthRecordedStream(void);
  
  virtual bool TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage);
  virtual bool ReadTeletextPage(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage);

protected:
  std::auto_ptr<struct PVRClient> m_pClient;
  std::auto_ptr<DllPVRClient> m_pDll;
  const                 long m_clientID;
  bool                  m_ReadyToUse;
  IPVRClientCallback   *m_manager;
  AddonCB              *m_callbacks;
  CStdString            m_hostName;

private:
  void WriteClientTimerInfo(const CTVTimerInfoTag &timerinfo, PVR_TIMERINFO &tag);
  static void PVRTransferTimerEntry(void *userData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
  static void PVREventCallback(void *userData, const PVR_EVENT pvrevent, const char *msg);
};

typedef std::vector<CPVRClient*> VECCLIENTS;
