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
#include "pvrclients/IPVRClient.h"
#include "utils/GUIInfoManager.h"
#include "TVDatabase.h"

#include <vector>

typedef std::vector< std::pair< DWORD, CFileItemList* > > PVRSCHEDULES;

class CEPGInfoTag;
enum RecStatus; /// forwarding an enum? only VC++?

class CPVRManager 
      : public IPVRClientCallback
      /*, private CThread*/
{
public:
  ~CPVRManager();
  static void RemoveInstance();
  void OnClientMessage(DWORD clientID, int event, const std::string& data);
  void FillChannelData(DWORD clientID, PVRCLIENT_PROGRAMME* data, int count);

  // start/stop
  void Start();
  void Stop();
  static CPVRManager* GetInstance();
  static void   ReleaseInstance();
  static bool   IsInstantiated() { return m_instance != NULL; }

  // info manager
  const char* TranslateInfo(DWORD dwInfo);
  CStdString GetNextRecording();
  bool IsConnected();
  static bool HasScheduled()  { return m_hasScheduled;  };
  static bool IsRecording()   { return m_isRecording; };

  // pvrmanager status
  static bool HasRecordings() { return m_hasRecordings; };

  // backend's status

  // called from TV Guide window
  CEPG* GetEPG() { return m_EPG; };
  PVRSCHEDULES GetScheduled();
  //PVRSCHEDULES GetTimers();
  PVRSCHEDULES GetConflicting();

protected:
  void SyncInfo(); // synchronize InfoManager related stuff

  bool CheckClientConnection();

  CURL GetConnString(DWORD clientID);
  void GetClientProperties(); // call GetClientProperties(DWORD clientID) for each client connected
  void GetClientProperties(DWORD clientID); // request the PVRCLIENT_PROPS struct from each client

  void UpdateChannelsList(); // call UpdateChannelsList(DWORD clientID) for each client connected
  void UpdateChannelsList(DWORD clientID); // update the list of channels for the client specified

  void UpdateChannelData(); // call UpdateChannelData(DWORD clientID) for each client connected
  void UpdateChannelData(DWORD clientID); // update the guide data for the client specified

  void  GetRecordingSchedules(); // call GetRecordingSchedules(DWORD clientID) for each client connected, active or otherwise
  int  GetRecordingSchedules(DWORD clientID); // update the list of recording schedules for the client specified, active or otherwise

  void GetUpcomingRecordings(); // call GetUpcomingRecordings(DWORD clientID) for each client connected
  int  GetUpcomingRecordings(DWORD clientID); // update the list of upcoming recordings for the client specified

  CStdString PrintStatus(RecStatus status); // convert a RecStatus into a human readable string
  CStdString PrintStatusDescription(RecStatus status); // convert a RecStatus into a more verbose human readable string

private:
  CPVRManager() { };
  static CPVRManager* m_instance;
  
  std::map< DWORD, IPVRClient* >     m_clients; // pointer to each enabled client's interface
  std::map< DWORD, PVRCLIENT_PROPS > m_clientProps; // store the properties of each client locally
  
  PVRSCHEDULES m_recordingSchedules; // list of all set recording schedules (including custom & manual)
  PVRSCHEDULES m_scheduledRecordings; // what will actually be recorded
  PVRSCHEDULES m_conflictingSchedules; // what is conflicting

  unsigned m_numRecordings;
  unsigned m_numUpcomingSchedules;
  unsigned m_numTimers;

  static bool m_isRecording;
  static bool m_hasRecordings;
  static bool m_hasScheduled;
  static bool m_hasTimers;

  CStdString  m_nextRecordingDateTime;
  CStdString  m_nextRecordingClient;
  CStdString  m_nextRecordingTitle;
  CStdString  m_nowRecordingDateTime;
  CStdString  m_nowRecordingClient;
  CStdString  m_nowRecordingTitle;

  CEPG       *m_EPG;
  CTVDatabase m_database;
};
