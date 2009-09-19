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
#include "AddonDll.h"
#include "DllPVRClient.h"

class CPVRClient : public IPVRClient,
                   public ADDON::CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>
{
public:
  CPVRClient(const AddonProps &props);
  ~CPVRClient();

  // DLL related
  virtual bool Init();
  virtual void DeInit();
  virtual void ReInit();

  // IPVRClient //////////////////////////////////////////////////////////////
  /* Server */
  virtual long GetID();
  virtual PVR_ERROR GetProperties(PVR_SERVERPROPS *props);
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
  /* Recordings */
  virtual int GetNumRecordings(void);
  /* Timers */
  virtual int GetNumTimers(void);
  virtual PVR_ERROR GetTimers(VECTVTIMERS &timers);
  virtual PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo);
  virtual PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false);
  virtual PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname);
  virtual PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo);

protected:
  long m_clientID;
  bool m_ReadyToUse;
  CStdString m_hostName;
  CStdString m_backendName;
  CStdString m_backendVersion;

  CCriticalSection    m_critSection;
  IPVRClientCallback *m_manager;

private:
  /* PVR API Callback functions */
  static void PVREventCallback(void *userData, const PVR_EVENT pvrevent, const char *msg);
  static void PVRTransferChannelEntry(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *chan);

  
};

typedef std::vector<CPVRClient*> VECCLIENTS;
