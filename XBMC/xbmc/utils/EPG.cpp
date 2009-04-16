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
#include "EPG.h"
#include "GUISettings.h"

using namespace XFILE;

CEPG*                               CEPG::m_instance = NULL;
std::queue< CEPG::CEPGTask >              CEPG::m_tasks;
std::map< long, IPVRClient* >       CEPG::m_clients;
std::vector< IEPGObserver* >        CEPG::m_observers;
CTVDatabase                         CEPG::m_database;
CCriticalSection                    CEPG::m_tasksSection;
CCriticalSection                    CEPG::m_clientsSection;
CCriticalSection                    CEPG::m_obsSection;
CCriticalSection                    CEPG::m_dbSection;
CCriticalSection                    CEPG::m_epgSection;

CEPG* CEPG::Get()
{
  if (!m_instance)
  {
    m_instance = new CEPG(CPVRManager::GetInstance());
  }

  /* updates client list */
  CSingleLock lock(m_clientsSection);
  m_clients = CPVRManager::GetInstance()->Clients();
  return m_instance;
}

CEPG::CEPG(CPVRManager* manager)
{
  m_isRunning = false;
  m_manager = manager;
}

CEPG::~CEPG()
{
  m_manager = NULL;
}

void CEPG::OnStartup()
{

}

void CEPG::OnExit()
{
  int problem = 0;
}

void CEPG::Attach(IEPGObserver* obs)
{
  CSingleLock lock(m_obsSection);
  m_observers.push_back(obs);
}

void CEPG::Detach(IEPGObserver* obs)
{
  CSingleLock lock(m_obsSection);
  std::vector<IEPGObserver*>::iterator itr = m_observers.begin();
  while (itr != m_observers.end())
  {
    if (*itr = obs)
      itr = m_observers.erase(itr);
    else
      itr++;
  }
}

void CEPG::Close()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance = NULL;
  }
}

void CEPG::Process()
{
  CSingleLock lock(m_tasksSection);
  CLog::Log(LOGDEBUG, "PVR: Begin Processing %u Tasks", m_tasks.size());
  m_isRunning = true;

  while (!m_bStop && m_tasks.size())
  {
    CEPGTask task = m_tasks.front();
    m_tasks.pop();
    lock.Leave();

    switch (task.m_task) {
    case GET_EPG_FOR_CHANNEL:
        UpdateEPGTask(task.m_channel);
        CLog::Log(LOGDEBUG, "PVR: client_%u finished epg update for channel %u", task.m_clientID, task.m_channel);
        break;
    case UPDATE_CLIENT_CHANNELS:
      UpdateChannelsTask(task.m_clientID);
      CLog::Log(LOGDEBUG, "PVR: client_%u finished channel update", task.m_clientID);
      break;
    default:
      break;
    }
    lock.Enter();
  }

  m_isRunning = false;
  CLog::Log(LOGDEBUG, "PVR: Finished Processing Tasks");
}

void CEPG::UpdateChannels()
{
  CSingleLock clientsLock(m_clientsSection);
  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    CEPGTask task;
    IPVRClient* client = (*itr).second;
    task.m_clientID = (*itr).first;
    clientsLock.Leave();

    if (!client)
      // client is configured by user, but non-functional
      return;

    task.m_task = UPDATE_CLIENT_CHANNELS;

    CSingleLock taskLock(m_tasksSection);
    m_tasks.push(task);

    CSingleLock clientLock(client->m_critSection);

    if (!client->m_isRunning)
    {
      Create(false, THREAD_MINSTACKSIZE);
      CStdString msg;
      msg.Format("PVR: client_%u channel update", task.m_clientID); 
      SetName(msg.c_str());
      SetPriority(-10);
    }
    clientsLock.Enter();
    itr++;
  }
}

void CEPG::UpdateChannelsTask(long clientID)
{
  VECCHANNELS channels;
  VECCHANNELS dbChannels;

  CLog::Log(LOGDEBUG, "PVR: client_%u starting channel update", clientID);

  /* grab any known channels for this client from the db*/
  CSingleLock dbLock(m_dbSection);
  m_database.Open();
  m_database.GetChannelList(clientID, dbChannels, false);

  /* close and remove locks on db as client may take some time */
  m_database.Close();
  dbLock.Leave();

  /* request channels from client */
  CSingleLock clientsLock(m_clientsSection);
  IPVRClient* client = m_clients[clientID];

  CSingleLock clientLock(client->m_critSection);
  PVR_ERROR err = client->GetChannelList(channels);
  clientLock.Leave();

  if(err == PVR_ERROR_NO_ERROR)
  {
    /*
    * First we look for moved channels on backend (other backend number)
    * and delete no more present channels inside database.
    * Problem:
    * If a channel on client is renamed, it is deleted from Database
    * and later added as new channel and loose his Group Information
    */
    dbLock.Enter();
    m_database.Open();

    for (unsigned int i = 0; i < dbChannels.size(); i++)
    {
      bool found = false;

      for (unsigned int j = 0; j < channels.size(); j++)
      {
        if (dbChannels[i].m_strChannel == channels[j].m_strChannel)
        {
          if (dbChannels[i].m_iClientNum != channels[j].m_iClientNum)
          {
            dbChannels[i].m_iClientNum = channels[j].m_iClientNum;
            dbChannels[i].m_encrypted = channels[j].m_encrypted;

            m_database.UpdateChannel(clientID, dbChannels[i]);
            CLog::Log(LOGINFO,"PVR: Updated TV channel %s", dbChannels[i].m_strChannel.c_str());
          }

          found = true;
          channels.erase(channels.begin()+j);
          break;
        }
      }

      if (!found)
      {
        CLog::Log(LOGINFO,"PVR: Removing TV channel %s (no longer present)", dbChannels[i].m_strChannel.c_str());
        m_database.RemoveChannel(clientID, dbChannels[i]);

        dbChannels.erase(dbChannels.begin()+i);
        i--;
      }
    }
  }
  else
  {
    // couldn't get channel list
    CLog::Log(LOG_ERROR, "PVR: client: %ld Error receiving channel list", clientID);
    return;
  }

  /* 
  * Now we add new channels to frontend
  * All entries now present in the temp lists, are new entries
  */

  for (unsigned int i = 0; i < channels.size(); i++)
  {
    channels[i].m_iIdChannel = m_database.AddChannel(clientID, channels[i]);
    dbChannels.push_back(channels[i]);
    CLog::Log(LOGINFO,"PVRManager: Added TV channel %s", channels[i].m_strChannel.c_str());
  }

  /* finished with db for now */
  m_database.Close();
  dbLock.Leave();


  CSingleLock epg(m_epgSection);
  m_grid = dbChannels;
  
  /* notify observers entire channel list has been updated */
  NotifyObs(0);

  CLog::Log(LOGDEBUG, "PVR: client_%u finished channel update", clientID);
  CLog::Log(LOGDEBUG, "PVR: channels: %u", m_grid.size());
}

void CEPG::UpdateEPG()
{
  CSingleLock clientsLock(m_clientsSection);
  VECCHANNELS::iterator itr = m_grid.begin();
  while (itr != m_grid.end())
  {
    IPVRClient* client = m_clients[(*itr).m_clientID];

    if (!client)
      continue;
    
    CEPGTask task;
    task.m_clientID = (*itr).m_clientID;
    clientsLock.Leave();

    task.m_task = GET_EPG_FOR_CHANNEL;
    task.m_channel = (*itr).m_iClientNum;

    CSingleLock taskLock(m_tasksSection);
    m_tasks.push(task);

    CSingleLock clientLock(client->m_critSection);

    if (!client->m_isRunning)
    {
      Create(false, THREAD_MINSTACKSIZE);
      CStdString msg;
      msg.Format("PVR: client_%u epg update", task.m_clientID); 
      SetName(msg.c_str());
      SetPriority(-10);
    }
    clientsLock.Enter();
    itr++;
  }
}

void CEPG::UpdateEPGTask(unsigned int channelNo)
{
  CDateTime now = CDateTime::GetCurrentDateTime();
  int difference_from_GMT;

  CSingleLock clientLock(m_clientsSection);
  long clientID = m_grid[channelNo].m_clientID;

  IPVRClient* client = m_clients[clientID];

  if (!client)
    return;

  CDateTimeSpan span;
  span.SetDateTimeSpan(0, 0, g_guiSettings.GetInt("pvrmenu.lingertime")*60, 0);
  CDateTime start = now - span;
  span.SetDateTimeSpan(g_guiSettings.GetInt("pvrmenu.daystodisplay"), 0, 0, 0);
  CDateTime end   = now + span;
  
  CFileItemList epg;
  client->GetEPGForChannel(channelNo, epg, start, end);
}

void CEPG::NotifyObs(unsigned int channelNo)
{
  CSingleLock lock(m_obsSection);
  std::vector<IEPGObserver*>::iterator itr = m_observers.begin();
  while (itr != m_observers.end())
  {
   (*itr)->OnChannelUpdated(channelNo);
   itr++;
  }
}