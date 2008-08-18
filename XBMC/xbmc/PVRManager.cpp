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
#include "utils/EPG.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "utils/EPGInfoTag.h"
#include "Playlist.h"

using namespace PLAYLIST;
using namespace PVR;

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
  ///TODO determine which plugins are enabled
  ///TODO init and push each IPVRClient onto m_clients

  // load list of client(s)
  m_clients.clear();
  m_clients.push_back(new PVRClientMythTv(0, this)); /// example clientID of 0

  // for now get myth connection settings only
  GetClientProperties();

  // now that clients have been initialized, we check connectivity
  std::vector< IPVRClient* >::iterator clientItr = m_clients.begin();
  while (clientItr != m_clients.end())
  {
    // first find the connection string
    CURL connString = GetConnString((*clientItr)->GetID());
    (*clientItr)->SetConnString(connString);

    // signal client to connect to backend
    (*clientItr)->Connect();

    // check client has connected
    if ((*clientItr)->IsUp())
      clientItr++;
    else
      m_clients.erase(clientItr);
  }

  if (m_clients.empty())
  {
    CLog::Log(LOGERROR, "PVR: no clients are connected");
    Stop();
    return;
  }

  // check if there are new channels since last connection
  UpdateChannelsList();

  // check if we need to update the db
  UpdateChannelData();

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

CURL CPVRManager::GetConnString(DWORD clientID)
{
  CURL connString;

  // set client defaults
  connString.SetHostName(m_clientProps[clientID].DefaultHostname);
  connString.SetUserName(m_clientProps[clientID].DefaultUser);
  connString.SetPassword(m_clientProps[clientID].DefaultPassword);
  connString.SetPort(m_clientProps[clientID].DefaultPort);

  CStdString host, user, pass, port;
  host = g_guiSettings.GetString("pvrmanager.serverip");
  user = g_guiSettings.GetString("pvrmanager.username");
  pass = g_guiSettings.GetString("pvrmanager.password");
  port = g_guiSettings.GetString("pvrmanager.serverport");
  
  if (!host.IsEmpty())
  {
    connString.SetHostName(host);
  }
  if (!user.IsEmpty())
  {
    connString.SetUserName(user);
  }
  if (!pass.IsEmpty())
  {
    connString.SetPassword(pass);
  }
  if (!port.IsEmpty())
  {
    connString.SetPort(atoi(port));
  }

  return connString;
}

void CPVRManager::GetClientProperties()
{
  m_clientProps.clear();
  for (unsigned i=0; i < m_clients.size(); i++)
  {
    GetClientProperties(i);
  }
}

void CPVRManager::GetClientProperties(DWORD clientID)
{
  PVRCLIENT_PROPS props;
  props = m_clients[clientID]->GetProperties();
  m_clientProps.push_back(props); ///TODO !! broken, only one client suppored should really map clientID to props
}

CStdString CPVRManager::GetNextRecording()
{
  CStdString label;
  CDateTime now;
  now = CDateTime::GetCurrentDateTime();
  unsigned i = m_scheduledRecordings.size();
  if ( i > 0)
  {
    
  }

  return label;
}

void CPVRManager::OnClientMessage(DWORD clientID, int event, const std::string& data)
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

void CPVRManager::UpdateChannelsList()
{
  m_database.Open();
  for (unsigned i = 0; i < m_clients.size(); i++)
  {
    UpdateChannelsList(i);
  }
  m_database.Close();
  // get number of days of data available
  // if less than mindays available 
  // get days needed
  // if (newchannels)
  // get now + days needed
}

void CPVRManager::UpdateChannelsList(DWORD clientID)
{
  m_database.Open();

  unsigned numClientChans, numDbChans;
  numClientChans = m_clients[clientID]->GetNumChannels();
  numDbChans = m_database.GetNumChannels(0);

  bool newChannels;

  if (numClientChans > numDbChans)
  {
    // client has found new channels
    newChannels = true;

    // get list of channels from client
    EPGData clientChannels;
    m_clients[clientID]->GetChannelList(clientChannels); ///TODO check for returned error message

    //// and from the database
    //EPGData dbChannels;
    //m_database.GetChannelList(clientID, dbChannels);

    std::vector< CStdString > newChannels;

    // for each item, check if it exists in db, otherwise add
    EPGData::iterator chanItr = clientChannels.begin();
    while (chanItr != clientChannels.end())
    {
      if (!m_database.HasChannel(clientID, (*chanItr)->Name()))
        //! don't have this channel, add it to db
      {
        // bouquets check
        long idBouquet, idChannel;
        CStdString strBouquet;

        if(m_clientProps[clientID].HasBouquets)
        {
          CStdString strName = (*chanItr)->Name().c_str();
          const char* name = strName.c_str();
          int bouquet = m_clients[clientID]->GetBouquetForChannel(const_cast<char *>(name));
          strBouquet = m_clients[clientID]->GetBouquetName(bouquet);
        }
        else
        {
          idBouquet = 0;
          strBouquet = "Default";
        }

        // this updates idBouquet & idChannel, because these are set by the DB!!
        m_database.NewChannel(clientID, idBouquet, idChannel, strBouquet, (*chanItr)->Name(), 
          (*chanItr)->Name(), (*chanItr)->Number(), (*chanItr)->IconPath());

        // update the CTVChannel with the correct chanID
        (*chanItr)->SetID(idChannel);
      }
      chanItr++;
    }

    // 
  }

  m_database.Close();
}

void CPVRManager::UpdateChannelData()
{
  for (unsigned i = 0; i < m_clients.size(); i++)
  {
    UpdateChannelData(i);
  }
}

void CPVRManager::UpdateChannelData(DWORD clientID)
{
  m_database.Open();
  CDateTime dataEnd, now;
  now = CDateTime::GetCurrentDateTime();
  dataEnd = m_database.GetDataEnd(clientID);
  CDateTimeSpan  minimum = CDateTimeSpan::SetDateTimeSpan(g_guiSettings.GetInt("pvrmanager.daystodisplay"), 0, 0, 0);
  if (dataEnd < now + minimum)
  {

  }


  m_database.Close();
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

//void CPVRManager::UpdateClient( DWORD clientID )
//{
//  m_numUpcomingSchedules = GetUpcomingRecordings(clientID);  /// confusing clientID with position in m_clients?
//  m_numTimers = GetRecordingSchedules(clientID);
//  SyncInfo();
//}

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
    if(m_recordingSchedules.count(clientID) != 0)
    { /* this client has a list stored already */
      m_recordingSchedules.erase(clientID);
      m_recordingSchedules.insert(std::make_pair(clientID, &schedules));
    }
    else
      m_recordingSchedules.insert(std::make_pair(clientID, &schedules));
  }
  int size = schedules.Size();
  return size;
}

int CPVRManager::GetUpcomingRecordings( DWORD clientID )
{
  CFileItemList scheduled;
  if(m_clients[clientID]->GetUpcomingRecordings(scheduled))
  {
    if(m_scheduledRecordings.count(clientID) != 0)
    { /* this client has a list stored already */
      m_scheduledRecordings.erase(clientID);
      m_scheduledRecordings.insert(std::make_pair(clientID, &scheduled));
    }
    else
      m_scheduledRecordings.insert(std::make_pair(clientID, &scheduled));
  }
  int size = scheduled.Size();
  return size;
}