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
#include "pvrclients/PVRClient-mythtv.h"
#include "GUISettings.h"
#include "Application.h"
#include "utils/EPG.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "utils/EPGInfoTag.h"
#include "PlayList.h"

using namespace PLAYLIST;

#define XBMC_PVRMANAGER_VERSION "0.1"

CPVRManager* CPVRManager::m_instance=NULL;
bool CPVRManager::m_hasRecordings = false;
bool CPVRManager::m_isRecording = false;
bool CPVRManager::m_hasScheduled = false;
bool CPVRManager::m_hasTimers = false;

/*****************************************************************************/
CPVRManager::~CPVRManager()
{
  CLog::Log(LOGINFO,"pvrmanager destroyed");
  ///TODO info manager -> PVR offline
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
  ///TODO initialize each
  // for now, it's just 1 client
  IPVRClient* mythInterface = new PVRClientMythTv(0, this);
  DWORD mythClientID = 0;
  m_clients.insert(std::make_pair(mythClientID, mythInterface));

  // request each client's basic properties
  GetClientProperties();

  // now that clients have been initialized, we check connectivity
  if (!CheckClientConnection())
    return;

  // check if there are new channels since last connection
  UpdateChannelsList();

  // check if we need to update the db
  UpdateChannelData();

  // retrieve timers, scheduled recordings & conflicts
  GetRecordingSchedules();
  GetUpcomingRecordings();
  /*GetConflicting();*/

  // finally sync info for the infomanager
  SyncInfo();

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
  m_instance = NULL;
}


bool CPVRManager::CheckClientConnection()
{
  std::map< DWORD, IPVRClient* >::iterator clientItr = m_clients.begin();
  while (clientItr != m_clients.end())
  {
    // first find the connection string
    CURL connString = GetConnString((*clientItr).first);
    (*clientItr).second->SetConnString(connString);

    // signal client to connect to backend
    (*clientItr).second->Connect();

    // check client has connected
    if ((*clientItr).second->IsUp())
      clientItr++;
    else
      m_clients.erase(clientItr);
  }

  if (m_clients.empty())
  {
    CLog::Log(LOGERROR, "PVR: no clients could connect");
    return false;
  }
  
  return true;
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
  port = g_guiSettings.GetInt("pvrmanager.serverport");
  
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
  m_clientProps.insert(std::make_pair(clientID, props));
}

CStdString CPVRManager::GetNextRecording()
{
  return m_nextRecordingDateTime;
}

bool CPVRManager::IsConnected()
{
  if (m_clients.empty())
    return false;

  return true;
}

const char* CPVRManager::TranslateInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_NOW_RECORDING_CLIENT) return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NOW_RECORDING_TITLE) return m_nowRecordingTitle;
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME) return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NEXT_RECORDING_CLIENT) return m_nextRecordingClient;
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE) return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;
  return "";
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
      SyncInfo();
      break;
  }
}

void CPVRManager::FillChannelData(DWORD clientID, PVRCLIENT_PROGRAMME* data, int count)
{
  if (count < 1)
    return;

  CFileItemList chanData;

  for (int i = 0; i < count; i++)
  {
    CEPGInfoTag broadcast(clientID);
    broadcast.m_strTitle        = data[i].title;
    broadcast.m_strPlotOutline  = data[i].subtitle;
    broadcast.m_strPlot         = data[i].description;
    broadcast.m_startTime       = data[i].starttime;
    broadcast.m_endTime         = data[i].endtime;
    broadcast.m_strChannel      = data[i].name;
    broadcast.m_bouquetNum      = data[i].bouquet;
    broadcast.m_seriesID        = data[i].seriesid;
    broadcast.m_episodeID       = data[i].episodeid;
    broadcast.m_strGenre        = data[i].category;
    broadcast.m_recStatus       = (RecStatus) data[i].rec_status;
    broadcast.m_transCodeStatus = (TranscodingStatus) data[i].event_flags;

    CFileItemPtr prog(new CFileItem(broadcast));
    chanData.Add(prog);
  }

  m_database.Open();
  m_database.AddChannelData(chanData);
  m_database.Close();
}

void CPVRManager::UpdateChannelsList()
{
  m_database.Open();
  for (unsigned i = 0; i < m_clients.size(); i++)
  {
    UpdateChannelsList(i);
  }
  m_database.Close();
}

void CPVRManager::UpdateChannelsList(DWORD clientID)
{
  std::map< DWORD, PVRCLIENT_PROPS >::iterator currPropItr = m_clientProps.find(clientID);

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
    PVRCLIENT_CHANNEL* clientChannels;
    m_clients[clientID]->GetChannelList(clientChannels); ///TODO check for returned error message


    std::vector< CStdString > newChannels;

    // for each item, check if it exists in db, otherwise add
    for (unsigned i=0; i<numClientChans; i++)
    {
      if (!m_database.HasChannel(clientID, clientChannels[i].Name))
        // don't have this channel, add it to db
      {
        // bouquets check
        CStdString strBouquet;

        if(m_clientProps[clientID].HasBouquets)
        {
          int bouquet = m_clients[clientID]->GetBouquetForChannel(clientChannels[i].Name);
          strBouquet = m_clients[clientID]->GetBouquetName(bouquet);
        }
        else
        {
          strBouquet = "Default";
        }

        // the database updates idBouquet & idChannel in the process
        m_database.NewChannel(clientID, strBouquet, clientChannels[i].Name, 
          clientChannels[i].Name, clientChannels[i].Number, clientChannels[i].IconPath);
      }
    }
  }
  else if (numDbChans > numClientChans)
  {
    ///TODO need to remove stale channels
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

  CDateTimeSpan  minimum;
  minimum.SetDateTimeSpan(g_guiSettings.GetInt("pvrmanager.daystodisplay"), 0, 0, 0);

  if (dataEnd < now + minimum)
  {
    EPGData channels;
    m_database.GetChannelList(clientID, channels);
    EPGData::iterator itr = channels.begin();
    while (itr != channels.end())
    {
      CTVChannel* channel = *itr;
      CFileItemList* data = new CFileItemList(channel->Name());
      m_clients[clientID]->GetEPGForChannel(channel->GetBouquetID(), channel->Number());
      itr++;
    }

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
//  m_numUpcomingSchedules = GetUpcomingRecordings(clientID);  ///TODO confusing clientID with position in m_clients?
//  m_numTimers = GetRecordingSchedules(clientID);
//  SyncInfo();
//}

void CPVRManager::SyncInfo()
{
  m_numUpcomingSchedules > 0 ? m_hasScheduled = true : m_hasScheduled = false;
  m_numRecordings > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  m_numTimers > 0 ? m_hasTimers = true : m_hasTimers = false;

  if (m_hasScheduled)
  {
    CDateTime nextRec;
    for (PVRSCHEDULES::iterator itr = m_scheduledRecordings.begin(); itr != m_scheduledRecordings.end(); itr++)
    {
      if(!(*itr).second->IsEmpty())
      { /* this client has recordings scheduled */
        CFileItemPtr item = (*itr).second->Get(0); // first item will be next scheduled recording
        if (nextRec > item->GetEPGInfoTag()->m_startTime || !nextRec.IsValid())
        {
          nextRec = item->GetEPGInfoTag()->m_startTime;
          m_nextRecordingTitle = item->GetEPGInfoTag()->m_strTitle;
          m_nextRecordingClient = m_clientProps[(*itr).first].Name;
          m_nextRecordingDateTime = nextRec.GetAsLocalizedDateTime(false, false);
        }
      }
    }
  }

  if (m_isRecording)
  {
    CDateTime nextRec;
    for (PVRSCHEDULES::iterator itr = m_scheduledRecordings.begin(); itr != m_scheduledRecordings.end(); itr++)
    {
      if(!(*itr).second->IsEmpty())
      { /* this client has recordings scheduled */
        CFileItemPtr item = (*itr).second->Get(0); // first item will be next scheduled recording
        if (nextRec > item->GetEPGInfoTag()->m_startTime || !nextRec.IsValid())
        {
          nextRec = item->GetEPGInfoTag()->m_startTime;
          m_nextRecordingTitle = item->GetEPGInfoTag()->m_strTitle;
          m_nextRecordingClient = m_clientProps[(*itr).first].Name;
          m_nextRecordingDateTime = nextRec.GetAsLocalizedDateTime(true, false);
        }
      }
    }
  }
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
      string = g_localizeStrings.Get(17100); // 17100 is "unknown"
  };
  
  return string;
}

void CPVRManager::GetRecordingSchedules()
{
  m_database.Open();
  m_numTimers = 0;

  for (unsigned i = 0; i < m_clients.size(); i++)
  {
    m_numTimers += GetRecordingSchedules(i);
  }
  m_database.Close();
}

void CPVRManager::GetUpcomingRecordings()
{
  m_database.Open();
  m_numUpcomingSchedules = 0;

  for (unsigned i = 0; i < m_clients.size(); i++)
  {
    m_numUpcomingSchedules += GetUpcomingRecordings(i);
  }
  m_database.Close();
}

int CPVRManager::GetRecordingSchedules( DWORD clientID )
{
  CFileItemList* schedules = new CFileItemList();

  if(m_clients[clientID]->GetRecordingSchedules(schedules))
  {
    schedules->Sort(SORT_METHOD_DATE, SORT_ORDER_ASC);

    PVRSCHEDULES::iterator itr = m_recordingSchedules.begin();
    while (itr != m_recordingSchedules.end())
    {
      if (clientID == (*itr).first)
      {
        m_recordingSchedules.erase(itr);
      }
    }
    m_recordingSchedules.push_back(std::make_pair(clientID, schedules));
  }
  int size = schedules->Size();
  return size;
}

int CPVRManager::GetUpcomingRecordings( DWORD clientID )
{
  CFileItemList* scheduled = new CFileItemList();

  if(m_clients[clientID]->GetUpcomingRecordings(scheduled))
  {
    scheduled->Sort(SORT_METHOD_DATE, SORT_ORDER_ASC);

    PVRSCHEDULES::iterator itr = m_scheduledRecordings.begin();
    while (itr != m_scheduledRecordings.end())
    {
      if (clientID == (*itr).first)
      {
        m_scheduledRecordings.erase(itr);
      }
    }
    m_scheduledRecordings.push_back(std::make_pair(clientID, scheduled));
  }
  int size = scheduled->Size();
  return size;
}
