#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "utils/Thread.h"
#include "utils/Addon.h"
#include "pvrclients/IPVRClient.h"
#include "utils/GUIInfoManager.h"
#include "TVTimerInfoTag.h"
#include "TVChannelInfoTag.h"
#include "TVDatabase.h"

#include <vector>

typedef std::map< long, IPVRClient* >           CLIENTMAP;
typedef std::map< long, IPVRClient* >::iterator CLIENTMAPITR;

class CPVRManager : public IPVRClientCallback
                  , public ADDON::IAddonCallback
                  , private CThread 
{
public:
  ~CPVRManager();

  void Start();
  void Stop();

  /* Manager access */
  static void RemoveInstance();
  static void ReleaseInstance();
  static CPVRManager* GetInstance();
  
  /* addon specific */
  bool RequestRemoval(const ADDON::CAddon* addon);

  /* Event Handling */
  bool LoadClients();
  void OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg);

  /* Thread handling */
  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  /* Timer handling */
  int GetNumTimers();
  int GetAllTimers(CFileItemList* results);
  bool AddTimer(const CFileItem &item);
  bool DeleteTimer(const CFileItem &item, bool force = false);
  bool RenameTimer(const CFileItem &item, CStdString &newname);
  bool UpdateTimer(const CFileItem &item);
  CDateTime NextTimerDate(void);

  // info manager
  const char* TranslateInfo(DWORD dwInfo);
  static bool HasTimer()  { return m_hasTimer;  };
  static bool IsRecording()   { return m_isRecording; };
  static bool HasRecordings() { return m_hasRecordings; };

  static CLIENTMAP* Clients() { return &m_clients; }

protected:
  CPVRManager();

  void SyncInfo(); // synchronize InfoManager related stuff

  bool CheckClientConnections();
  void LostConnection();

  CURL GetConnString(long clientID);
  void GetClientProperties(); // call GetClientProperties(long clientID) for each client connected
  void GetClientProperties(long clientID); // request the PVR_SERVERPROPS struct from each client

  void GetTimers(); // call GetTimers(long clientID) for each client connected, active or otherwise
  int  GetTimers(long clientID); // update the list of timers for the client specified, active or otherwise

  void GetRecordings(); // call GetRecordings(long clientID) for each client connected
  int  GetRecordings(long clientID); // update the list of active & completed recordings for the client specified

  CStdString PrintStatus(RecStatus status); // convert a RecStatus into a human readable string
  CStdString PrintStatusDescription(RecStatus status); // convert a RecStatus into a more verbose human readable string

private:
  friend class CEPG;

  static CPVRManager* m_instance;

  static CLIENTMAP m_clients; // pointer to each enabled client's interface
  std::map< long, PVR_SERVERPROPS > m_clientProps; // store the properties of each client locally
  
  static CCriticalSection m_timersSection;
  static CCriticalSection m_epgSection;
  static CCriticalSection m_clientsSection;

  /* threaded tasks */
  bool m_running;
  bool m_bRefreshSettings;

  unsigned m_numRecordings;
  unsigned m_numUpcomingSchedules;
  unsigned m_numTimers;

  static bool m_isRecording;
  static bool m_hasRecordings;
  static bool m_hasTimer;
  static bool m_hasTimers;

  std::map< long, VECTVTIMERS > m_timers;

  CStdString  m_nextRecordingDateTime;
  CStdString  m_nextRecordingClient;
  CStdString  m_nextRecordingTitle;
  CStdString  m_nowRecordingDateTime;
  CStdString  m_nowRecordingClient;
  CStdString  m_nowRecordingTitle;

  CEPG       *m_EPG;
  CTVDatabase m_database;
};
