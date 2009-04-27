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
#include "DllPVRClient.h"
#include "Thread.h"

class CPVRClient : public IPVRClient
{
public:
  CPVRClient(const long clientID, struct PVRClient* pClient, DllPVRClient* pDll, 
    const CStdString& strPVRClientName, ADDON::IAddonCallback *addonCB, IPVRClientCallback *pvrCB);
  ~CPVRClient();

  // DLL related
  bool Init();
  virtual void Remove();
  void GetSettings(std::vector<DllSetting> **vecSettings);
  void UpdateSetting(int num);
  void OnClientMessage(PVR_EVENT event);

  // IPVRClient //////////////////////////////////////////////////////////////
  virtual CCriticalSection* GetLock() { return &m_critSection; }
  /* Server */
  virtual long GetID();
  virtual PVR_ERROR GetProperties(PVR_SERVERPROPS *props);
  virtual PVR_ERROR Connect();
  virtual void Disconnect();
  virtual bool IsUp();
  /* General */
  virtual const std::string GetBackendName();
  virtual const std::string GetBackendVersion();
  virtual const std::string GetConnectionString();
  virtual PVR_ERROR GetDriveSpace(long long *total, long long *used);
  /* Bouquets */
  virtual int GetNumBouquets();
  virtual PVR_ERROR GetBouquetInfo(const unsigned int number, PVR_BOUQUET& info);
  /* Channels */
  virtual int GetNumChannels();
  virtual PVR_ERROR GetChannelList(VECCHANNELS &channels);
  /* EPG */
  virtual PVR_ERROR GetEPGForChannel(const unsigned int number, CFileItemList &epg, const CDateTime &start, const CDateTime &end);
  virtual PVR_ERROR GetEPGNowInfo(const unsigned int number, PVR_PROGINFO *result);
  virtual PVR_ERROR GetEPGNextInfo(const unsigned int number, PVR_PROGINFO *result);
  virtual PVR_ERROR GetEPGDataEnd(time_t end);
  /* Timers */
  virtual const unsigned int GetNumTimers(void);
  virtual PVR_ERROR GetTimers(VECTVTIMERS &timers);
  virtual PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo);
  virtual PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false);
  virtual PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname);
  virtual PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo);

  
protected:
  bool ConvertChannels(unsigned int num, PVR_CHANNEL **clientChans, VECCHANNELS &out);
  void ReleaseClientData(unsigned int num, PVR_CHANNEL **clientChans);
  const long m_clientID;
  std::auto_ptr<struct PVRClient> m_pClient;
  std::auto_ptr<DllPVRClient> m_pDll;
  CStdString m_clientName;
  CStdString m_hostName;
  CStdString m_backendName;
  CStdString m_backendVersion;

  CCriticalSection m_critSection;
  IPVRClientCallback* m_manager;

private:
  static void PVREventCallback(void *userData, const PVR_EVENT pvrevent, const char *msg);
  static void PVRLogCallback(void *userData, const PVR_LOG loglevel, const char *format, ... );
};

typedef std::vector<CPVRClient*> VECCLIENTS;
