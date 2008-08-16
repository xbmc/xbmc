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
#include "utils/IPVRClient.h"
#include "fileitem.h"

#include <vector>

typedef std::map< DWORD, CFileItemList* > PVRSCHEDULES;

class CEPGInfoTag;
enum RecStatus; /// forwarding an enum?

class CPVRManager 
      : public IPVRClientCallback
      , private CThread
{
public:
  ~CPVRManager();
  static void RemoveInstance();
  void OnMessage(DWORD clientID, int event, const std::string& data);

  // start/stop
  void Start();
  void Stop();
  static CPVRManager* GetInstance();
  static void   ReleaseInstance();
  static bool   IsInstantiated() { return m_instance != NULL; }

  // info manager
  CStdString GetNextRecording();
  bool IsConnected() { return true; };

  // pvrmanager status
  static bool HasRecordings() { return m_hasRecordings; }

  // backend's status

  // called from TV Guide window
  PVRSCHEDULES GetScheduled();
  //PVRSCHEDULES GetTimers();
  PVRSCHEDULES GetConflicting();


private:
  CPVRManager() { };
  static CPVRManager* m_instance;

  void UpdateAll(); // wrapper to update ALL clients
  void UpdateClient(DWORD clientID); // calls the specific client, asking it to update our tables
  void SyncInfo(); // synchronize all PVRManager status info
  CStdString PrintStatus(RecStatus status);

  int GetRecordingSchedules(DWORD clientID); //called by UpdateClient(clientID)
  int GetUpcomingRecordings(DWORD clientID); //called by UpdateClient(clientID)

  std::vector< IPVRClient* > m_clients;

  PVRSCHEDULES m_recordingSchedules; // list of all set recording schedules (including custom & manual)
  PVRSCHEDULES m_scheduledRecordings; // what will actually be recorded
  PVRSCHEDULES m_conflictingSchedules; // what is conflicting

  unsigned m_numRecordings;
  unsigned m_numUpcomingSchedules;
  unsigned m_numTimers;

  static bool m_hasRecordings;
  static bool m_hasSchedules;
  static bool m_hasTimers;
};

//extern CPVRManager g_pvrManager;
