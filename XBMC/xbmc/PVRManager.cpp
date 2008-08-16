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

#include "stdafx.h"
#include "PVRManager.h"
#include "utils/PVRClient-mythtv.h"
#include "GUISettings.h"
#include "Application.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "utils/EPGInfoTag.h"
#include "Playlist.h"

using namespace PLAYLIST;

#define XBMC_PVRMANAGER_VERSION "0.1"

CPVRManager* CPVRManager::m_instance=NULL;
bool CPVRManager::m_hasRecordings = false;
bool CPVRManager::m_hasSchedules = false;
bool CPVRManager::m_hasTimers = false;

/*****************************************************************************/
CPVRManager::~CPVRManager()
{
  CLog::Log(LOGINFO,"pvrmanager destroyed");
  // info manager -> PVR offline
}

/** Public *******************************************************************/
void CPVRManager::RemoveInstance()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance=NULL;
  }
}

void CPVRManager::Start()
{
  // determine which plugins are enabled

  // init and push each IPVRClient into m_clients
  CURL connString;
  connString.SetHostName(g_guiSettings.GetString("pvrmanager.serverip"));
  connString.SetUserName(g_guiSettings.GetString("pvrmanager.username"));
  connString.SetPassword(g_guiSettings.GetString("pvrmanager.password"));
  connString.SetPort(atoi(g_guiSettings.GetString("pvrmanager.serverport").c_str()));

  // load list of clients
  m_clients.push_back(new PVRClientMythTv(0, this, connString)); /// clientID of 0

  CFileItemList data;
  m_clients[0]->GetEPGForChannel(0, 0, data);
  UpdateAll(); // update everything
  CLog::Log(LOGNOTICE, "PVR: pvrmanager started. Clients loaded = %u", m_clients.size());
}

void CPVRManager::Stop()
{
  for (unsigned i=0; i < m_clients.size(); i++)
  {
    delete m_clients[i];
  }
  m_clients.clear();
  CLog::Log(LOGNOTICE, "PVR: pvrmanager stopped");
}

CPVRManager* CPVRManager::GetInstance()
{
  if (!m_instance)
    m_instance = new CPVRManager();

  return m_instance;
}

void CPVRManager::ReleaseInstance()
{
  m_instance = NULL; /// check is this enough?
}

CStdString CPVRManager::GetNextRecording()
{
  CStdString label;
  if (m_scheduledRecordings.size() > 0)
    label = "Neighbours Mon 11th";

  return label;
}

void CPVRManager::OnMessage(DWORD clientID, int event, const std::string& data)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  switch (event) {
    case PVRCLIENT_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%u unknown event (error?)", __FUNCTION__, clientID);
      break;
    case PVRCLIENT_EVENT_SCHEDULE_CHANGE:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%u schedule change", __FUNCTION__, clientID);
      GetRecordingSchedules(clientID);
      GetUpcomingRecordings(clientID);
      /*GetConflicting(clientID);*/
      break;
  }
}

void CPVRManager::UpdateAll()
{
  m_hasSchedules = false;

  for (unsigned i=0; i < m_clients.size(); i++)
  {
    UpdateClient(i); // not right confuses clientID with position in m_clients
  }
}

PVRSCHEDULES CPVRManager::GetScheduled()
{
  return m_scheduledRecordings;
}

PVRSCHEDULES CPVRManager::GetConflicting()
{
  return m_conflictingSchedules;
}

 /** Protected ***************************************************************/

void CPVRManager::UpdateClient( DWORD clientID )
{
  m_numUpcomingSchedules = GetUpcomingRecordings(clientID);  // not right confuses clientID with position in m_clients?
  m_numTimers = GetRecordingSchedules(clientID);
  SyncInfo();
}

void CPVRManager::SyncInfo()
{
  m_numUpcomingSchedules > 0 ? m_hasSchedules = true : m_hasSchedules = false;
  m_numRecordings > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  m_numTimers > 0 ? m_hasTimers = true : m_hasTimers = false;
}

CStdString CPVRManager::PrintStatus(RecStatus status)
{
  CStdString string;
  switch (status) {
    case rsDeleted:
    case rsStopped:
    case rsRecorded:
    case rsRecording:
    case rsWillRecord:
    case rsUnknown:
    case rsDontRecord:
    case rsPrevRecording:
    case rsCurrentRecording:
    case rsEarlierRecording:
    case rsTooManyRecordings:
    case rsCancelled:
    case rsConflict:
    case rsLaterShowing:
    case rsRepeat:
    case rsLowDiskspace:
    case rsTunerBusy:
      string = g_localizeStrings.Get(17100 + status);
      break;
    default:
      string = g_localizeStrings.Get(17100); // "unknown"
  };
  
  return string;
}

int CPVRManager::GetRecordingSchedules( DWORD clientID )
{
  CFileItemList schedules;
  if(m_clients[clientID]->GetRecordingSchedules(schedules))
  {
    if(m_recordingSchedules.count(clientID != 0))
    {
      m_recordingSchedules.erase(clientID);
      m_recordingSchedules.insert(std::make_pair(clientID, &schedules));
    }
    else
      m_recordingSchedules.insert(std::make_pair(clientID, &schedules));
  }
  CLog::Log(LOGINFO, "PVR: Client%u found %u timers", clientID, schedules.Size()); 
  int size = schedules.Size();
  for (int i=0; i <size; i++)
  {
    CLog::Log(LOGDEBUG, "PVR: Timer - Client%u - %s : %s", clientID, schedules[i]->GetEPGInfoTag()->m_strTitle.c_str(), PrintStatus(schedules[i]->GetEPGInfoTag()->m_recStatus).c_str());
  }
  return size;
}

int CPVRManager::GetUpcomingRecordings( DWORD clientID )
{
  CFileItemList scheduled;
  if(m_clients[clientID]->GetUpcomingRecordings(scheduled))
  {
    if(m_scheduledRecordings.count(clientID != 0))
    {
      m_scheduledRecordings.erase(clientID);
      m_scheduledRecordings.insert(std::make_pair(clientID, &scheduled));
    }
    else
      m_scheduledRecordings.insert(std::make_pair(clientID, &scheduled));
  }
  CLog::Log(LOGINFO, "PVR: Client%u found %u recordings scheduled", clientID, scheduled.Size());

  int size = scheduled.Size();
  for (int i=0; i <size; i++)
  {
    CLog::Log(LOGDEBUG, "PVR: Scheduled Recordings - Client%u - %s : %s", clientID, scheduled[i]->GetEPGInfoTag()->m_strTitle.c_str(), PrintStatus(scheduled[i]->GetEPGInfoTag()->m_recStatus).c_str());
  }
  return size;
}