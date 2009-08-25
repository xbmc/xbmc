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

#include "stdafx.h"
#include "Directory.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "Application.h"
#include "utils/EPG.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "utils/TVEPGInfoTag.h"
#include "PlayList.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "utils/AddonManager.h"

/* GUI Messages includes */
//TODO move to GUIWindow*
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "GUIWindowManager.h"

using namespace ADDON;


/************************************************************/

#define XBMC_PVRMANAGER_VERSION "0.2"

using namespace DIRECTORY;

std::map< long, IPVRClient* > CPVRManager::m_clients;
CPVRManager* CPVRManager::m_instance    = NULL;
bool CPVRManager::m_hasRecordings       = false;
bool CPVRManager::m_isRecording         = false;
bool CPVRManager::m_hasTimer            = false;
bool CPVRManager::m_hasTimers           = false;
CCriticalSection CPVRManager::m_epgSection;
CCriticalSection CPVRManager::m_timersSection;
CCriticalSection CPVRManager::m_clientsSection;

/************************************************************/
/** Class handling */

CPVRManager::CPVRManager()
{
  InitializeCriticalSection(&m_timersSection);
  InitializeCriticalSection(&m_epgSection);
}

CPVRManager::~CPVRManager()
{
  DeleteCriticalSection(&m_timersSection);
  DeleteCriticalSection(&m_epgSection);
  CLog::Log(LOGINFO,"PVR: destroyed");
}

void CPVRManager::Start()
{
  CSingleLock lock(m_clientsSection);
  /* First remove any clients */
  if (!m_clients.empty())
    m_clients.clear();

  if (!g_guiSettings.GetBool("pvr.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVR: PVRManager starting");

  m_infoToggleStart = NULL;
  m_infoToggleCurrent = 0;

  /* attach a CEPG singleton */
  CSingleLock epgLock(m_epgSection);
  m_EPG = CEPG::Get();
  epgLock.Leave();

  /* Discover, load and create chosen Client add-on's. */
  //TODO CAddonMgr::Get()->RegisterAddonCallback(ADDON_PVRDLL, this);
  if (!LoadClients())
  {
    CLog::Log(LOGERROR, "PVR: couldn't load clients");
    return;
  }

  /* spawn a thread */
//  Create(false, THREAD_MINSTACKSIZE);
//  SetName("PVRManager Updater");
//  SetPriority(-15);

  CLog::Log(LOGNOTICE, "PVR: PVRManager started. Clients loaded = %u", m_clients.size());
}

void CPVRManager::Stop()
{
  CSingleLock lock(m_clientsSection);

  CAddonMgr::Get()->UnregisterAddonCallback(ADDON_PVRDLL);
  StopThread();

  m_infoToggleStart = NULL;
  m_infoToggleCurrent = 0;

  for (unsigned int i=0; i < m_clients.size(); i++)
  {
    delete m_clients[i];
  }
  m_clients.clear();
//  m_EPG->Close();

  CLog::Log(LOGNOTICE, "PVR: PVRManager stopped");
}

/************************************************************/
/** Manager access */

CPVRManager* CPVRManager::GetInstance()
{
  if (!m_instance)
    m_instance = new CPVRManager();

  return m_instance;
}

void CPVRManager::RemoveInstance()
{
  delete m_instance;
  m_instance = NULL;
}

/************************************************************/
/** Thread handling */

void CPVRManager::OnStartup()
{
}

void CPVRManager::OnExit()
{
  CEPG::Get()->Close();
}

void CPVRManager::Process()
{
  /* as PVRManager has (re)started grab channel list(s) again */
  CEPG::Get()->UpdateChannels();
  /*CEPG::Get()->UpdateEPG();*/

  CDateTime lastEPGUpdate = NULL;
  CDateTime lastChannelUpdate = NULL;
  CDateTime lastScan   = NULL;

  while (!m_bStop)
  {

    Sleep(4000);
  }
}

/************************************************************/
/** Client handling */

bool CPVRManager::LoadClients()
{
  VECADDONS addons;
  // call update
  CAddonMgr::Get()->GetAddons(ADDON_PVRDLL, addons);

  /* Make sure addon's are loaded */
  if (addons.empty())
    return false;

  m_database.Open();

  /* load the clients */
  for (unsigned i=0; i<addons.size(); i++)
  {
    const AddonPtr clientAddon = addons.at(i);

    /* Add client to TV-Database to identify different backend types,
     * if client is already added his id is given.
     */
    long clientID = m_database.AddClient(clientAddon->Name(), clientAddon->UUID());
    if (clientID == -1)
    {
      CLog::Log(LOGERROR, "PVR: Can't Add/Get PVR Client '%s' to TV Database", clientAddon->Name().c_str());
      continue;
    }

    /* Load the Client library's and inside them into Client list if
     * success. Client initialization is also performed during loading.
     */
    IPVRClient *client = NULL;
    /*clientAddon->
    client = boost::dynamic_pointer_cast<IPVRClient>(clientAddon);*/
    if (client)
    {
      m_clients.insert(std::make_pair(client->GetID(), client));
    }
  }

  m_database.Close();

  // Request each client's basic properties
  GetClientProperties();

  return !m_clients.empty();
}

void CPVRManager::GetClientProperties()
{
  m_clientProps.clear();
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    GetClientProperties((*itr).first);
    itr++;
  }
}

void CPVRManager::GetClientProperties(long clientID)
{
  if (!m_clients[clientID])
    return;

  PVR_SERVERPROPS props;
  if (m_clients[clientID]->GetProperties(&props) == PVR_ERROR_NO_ERROR)
  {
    // store the client's properties
    m_clientProps.insert(std::make_pair(clientID, props));
  }
}

#define INFO_TOGGLE_TIME    1500
const char* CPVRManager::TranslateInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_NOW_RECORDING_CHANNEL) return m_nowRecordingChannel;
  else if (dwInfo == PVR_NOW_RECORDING_TITLE) return m_nowRecordingTitle;
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME) return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL) return m_nextRecordingChannel;
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE) return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;
  else if (dwInfo == PVR_BACKEND_NAME) return m_backendName;
  else if (dwInfo == PVR_BACKEND_VERSION) return m_backendVersion;
  else if (dwInfo == PVR_BACKEND_HOST) return m_backendHost;
  else if (dwInfo == PVR_BACKEND_DISKSPACE) return m_backendDiskspace;
  else if (dwInfo == PVR_BACKEND_CHANNELS) return m_backendChannels;
  else if (dwInfo == PVR_BACKEND_TIMERS) return m_backendTimers;
  else if (dwInfo == PVR_BACKEND_RECORDINGS) return m_backendRecordings;
  else if (dwInfo == PVR_BACKEND_NUMBER)
  {
    if (m_infoToggleStart == 0)
    {
      m_infoToggleStart = timeGetTime();
      m_infoToggleCurrent = 0;
    }
    else
    {
      if (timeGetTime() - m_infoToggleStart > INFO_TOGGLE_TIME)
      {
        if (m_clients.size() > 0)
        {
          m_infoToggleCurrent++;
          if (m_infoToggleCurrent > m_clients.size()-1)
            m_infoToggleCurrent = 0;

          CLIENTMAPITR itr = m_clients.begin();
          for (unsigned int i = 0; i < m_infoToggleCurrent; i++)
            itr++;

          long long kBTotal = 0;
          long long kBUsed  = 0;
          if (m_clients[(*itr).first]->GetDriveSpace(&kBTotal, &kBUsed) == PVR_ERROR_NO_ERROR)
          {
            kBTotal /= 1024; // Convert to MBytes
            kBUsed /= 1024;  // Convert to MBytes
            m_backendDiskspace.Format("%s %0.f GByte - %s: %0.f GByte", g_localizeStrings.Get(18055), (float) kBTotal / 1024, g_localizeStrings.Get(156), (float) kBUsed / 1024);
          }
          else
          {
            m_backendDiskspace = g_localizeStrings.Get(18074);
          }

          int NumChannels = m_clients[(*itr).first]->GetNumChannels();
          if (NumChannels >= 0)
            m_backendChannels.Format("%i", NumChannels);
          else
            m_backendChannels = g_localizeStrings.Get(161);

          int NumTimers = m_clients[(*itr).first]->GetNumTimers();
          if (NumTimers >= 0)
            m_backendTimers.Format("%i", NumTimers);
          else
            m_backendTimers = g_localizeStrings.Get(161);

          int NumRecordings = m_clients[(*itr).first]->GetNumRecordings();
          if (NumRecordings >= 0)
            m_backendRecordings.Format("%i", NumRecordings);
          else
            m_backendRecordings = g_localizeStrings.Get(161);

          m_backendName         = m_clients[(*itr).first]->GetBackendName();
          m_backendVersion      = m_clients[(*itr).first]->GetBackendVersion();
          m_backendHost         = m_clients[(*itr).first]->GetConnectionString();
        }
        else
        {
          m_backendName         = "";
          m_backendVersion      = "";
          m_backendHost         = "";
          m_backendDiskspace    = "";
          m_backendTimers       = "";
          m_backendRecordings   = "";
          m_backendChannels     = "";
        }
        m_infoToggleStart = timeGetTime();
      }
    }

    CStdString backendClients;
    if (m_clients.size() > 0)
      backendClients.Format("%u %s %u",m_infoToggleCurrent+1 ,g_localizeStrings.Get(20163), m_clients.size());
    else
      backendClients = g_localizeStrings.Get(14023);

    return backendClients;
  }
  return "";
}

void CPVRManager::OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  switch (clientEvent) {
    case PVR_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%ld unknown event : %s", __FUNCTION__, clientID, msg);
      break;

    case PVR_EVENT_TIMERS_CHANGE:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%ld timers changed", __FUNCTION__, clientID);
      /*GetTimers();*/
      /*GetConflicting(clientID);*/
      /*SyncInfo();*/
      break;

    case PVR_EVENT_RECORDINGS_CHANGE:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%ld recording list changed", __FUNCTION__, clientID);
      /*GetTimers();
      GetRecordings();
      SyncInfo();*/
      break;
    default:
      break;
  }
}

/************************************************************/
/** Timer handling **/

int CPVRManager::GetNumTimers()
{
  return m_timers.size();
}

int CPVRManager::GetAllTimers(CFileItemList* results)
{
  CSingleLock lock(m_timersSection);

  for (unsigned int c =0; c < m_timers.size(); c++)
  {
    for (unsigned int i = 0; i < m_timers[i].size(); ++i)
    {
      CFileItemPtr timer(new CFileItem(m_timers[c][i]));
      results->Add(timer);
    }
  }

  lock.Leave();

  results->Sort(SORT_METHOD_DATE, SORT_ORDER_ASC);
  return results->Size();
}

bool CPVRManager::AddTimer(const CFileItem &item)
{
  /* Check if a CTVTimerInfoTag is inside file item */
  if (!item.IsTVTimer())
  {
    CLog::Log(LOGERROR, "CPVRManager: UpdateTimer no TVInfoTag given!");
    return false;
  }

  const CTVTimerInfoTag *timer = item.GetTVTimerInfoTag();
  long id = timer->m_clientID;

  CSingleLock lock(m_clientsSection);
  IPVRClient* client = m_clients[id];

  PVR_ERROR err = client->AddTimer(timer);

  if (err == PVR_ERROR_NO_ERROR)
  {
    return true;
  }
  else if (err == PVR_ERROR_SERVER_ERROR)
  {
    /* print info dialog "Server error!" */
    CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
    LostConnection();
    return false;
  }
  else if (err == PVR_ERROR_NOT_SYNC)
  {
    /* print info dialog "Timers not in sync!" */
    CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
    return false;
  }
  else if (err == PVR_ERROR_NOT_SAVED)
  {
    /* print info dialog "Couldn't delete timer!" */
    CGUIDialogOK::ShowAndGetInput(18100,18806,18803,0);
    return false;
  }
  else if (err == PVR_ERROR_ALREADY_PRESENT)
  {
    /* print info dialog */
    CGUIDialogOK::ShowAndGetInput(18100,18806,0,18814);
    return false;
  }
  else
  {
    /* print info dialog "Unknown error!" */
    CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
    return false;
  }
}

bool CPVRManager::DeleteTimer(const CFileItem &item, bool force)
{
  const CTVTimerInfoTag timer = item.GetTVTimerInfoTag();
  long id = timer.m_clientID;

  CSingleLock lock(m_clientsSection);
  IPVRClient* client = m_clients[id];

  if (m_clientProps[id].SupportTimers)
  {
    PVR_ERROR err = PVR_ERROR_NO_ERROR;

    if (force)
      err = client->DeleteTimer(timer, true);
    else
      err = client->DeleteTimer(timer);

    if (err == PVR_ERROR_RECORDING_RUNNING)
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);

      if (pDialog)
      {
        pDialog->SetHeading(122);
        pDialog->SetLine(0, "");
        pDialog->SetLine(1, 18162);
        pDialog->SetLine(2, "");
        pDialog->DoModal();

        if (!pDialog->IsConfirmed()) return false;

        err = client->DeleteTimer(timer, true);
      }
    }

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
      LostConnection();
      return false;
    }
    else if (err == PVR_ERROR_NOT_SYNC)
    {
      /* print info dialog "Timers not in sync!" */
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
      return false;
    }
    else if (err == PVR_ERROR_NOT_DELETED)
    {
      /* print info dialog "Couldn't delete timer!" */
      CGUIDialogOK::ShowAndGetInput(18100,18802,18803,0);
      return false;
    }
    else
    {
      /* print info dialog "Unknown error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
      return false;
    }
  }
  else
  {
    return false;
  }
}

bool CPVRManager::RenameTimer(const CFileItem &item, CStdString &newname)
{
  CTVTimerInfoTag timer = item.GetTVTimerInfoTag();
  long id = timer.m_clientID;

  CSingleLock lock(m_clientsSection);
  IPVRClient* client = m_clients[id];

  if (m_clientProps[id].SupportTimers)
  {
    PVR_ERROR err = m_clients[id]->RenameTimer(timer, newname);

    if (err == PVR_ERROR_NOT_IMPLEMENTED)
    {
      timer.m_strTitle = newname;
      err = client->UpdateTimer(timer);
    }

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
      LostConnection();
      return false;
    }
    else if (err == PVR_ERROR_NOT_SYNC)
    {
      /* print info dialog "Timers not in sync!" */
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
      return false;
    }
    else if (err == PVR_ERROR_NOT_SAVED)
    {
      /* print info dialog "Couldn't delete timer!" */
      CGUIDialogOK::ShowAndGetInput(18100,18806,18803,0);
      return false;
    }
    else
    {
      /* print info dialog "Unknown error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
      return false;
    }
  }

  return false;
}

bool CPVRManager::UpdateTimer(const CFileItem &item)
{
  CTVTimerInfoTag timer = item.GetTVTimerInfoTag();
  long id = timer.m_clientID;

  CSingleLock lock(m_clientsSection);
  IPVRClient* client = m_clients[id];

  if (client && m_clientProps[id].SupportTimers)
  {
    PVR_ERROR err = client->UpdateTimer(timer);

    /* Check for errors and inform the user */
    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
      LostConnection();
      return false;
    }
    else if (err == PVR_ERROR_NOT_SYNC)
    {
      /* print info dialog "Timers not in sync!" */
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
      return false;
    }
    else if (err == PVR_ERROR_NOT_SAVED)
    {
      /* print info dialog "Couldn't save timer!" */
      CGUIDialogOK::ShowAndGetInput(18100,18806,18803,0);
      return false;
    }
    else
    {
      /* print info dialog "Unknown error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
      return false;
    }
  }
  else
  {
    return false;
  }
}

CDateTime CPVRManager::NextTimerDate(void)
{
  /*CTVTimerInfoTag timer = item.GetTVTimerInfoTag();
  long id = timer.m_clientID;

  CSingleLock lock(m_clientsSection);
  IPVRClient* client = m_clients[id];

  if (m_client)
  {
    CDateTime nextRec = NULL;

    if (m_timers.size() == 0)
      return NULL;

    for (unsigned int i = 0; i < m_timers.size(); i++)
    {
      if (nextRec < m_timers[i].m_StartTime && m_timers[i].m_Active)
      {
        nextRec = m_timers[i].m_StartTime;
      }
    }
    return nextRec;
  }*/

  return NULL;
}

/** Protected ***************************************************************/

void CPVRManager::SyncInfo()
{
  m_numUpcomingSchedules > 0 ? m_hasTimer = true : m_hasTimer = false;
  m_numRecordings > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  m_numTimers > 0 ? m_hasTimers = true : m_hasTimers = false;
  m_isRecording = false;

  if (m_hasTimer)
  {
    CDateTime nextRec;
    std::map<long, VECTVTIMERS>::iterator itr = m_timers.begin();
    while (itr != m_timers.end())
    {
      if(!(*itr).second.empty())
      { /* this client has recordings scheduled */
        CTVTimerInfoTag *timer = &(*itr).second[0]; // first item will be next scheduled recording
        if (nextRec > timer->m_StartTime)
        {
          nextRec = timer->m_StartTime;
          m_nextRecordingTitle = timer->m_strTitle;
          m_nextRecordingChannel = timer->m_strChannel;
          m_nextRecordingDateTime = nextRec.GetAsLocalizedDateTime(false, false);
          if (timer->m_recStatus)
            m_isRecording = true;
          else
            m_isRecording = false;
        }
      }

      itr++;
    }
  }

  if (m_isRecording)
  {
    m_nowRecordingTitle = m_nextRecordingTitle;
    m_nowRecordingChannel = m_nextRecordingChannel;
    m_nowRecordingDateTime = m_nextRecordingDateTime;
  }
  else
  {
    m_nowRecordingTitle.clear();
    m_nowRecordingChannel.clear();
    m_nowRecordingDateTime.clear();
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

void CPVRManager::GetTimers()
{
  m_numTimers = 0;
  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    m_numTimers += GetTimers((*itr).first);
    itr++;
  }
}

void CPVRManager::GetRecordings()
{
  m_numRecordings = 0;

  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    m_numRecordings += GetRecordings((*itr).first);
    itr++;
  }
}

int CPVRManager::GetTimers(long clientID)
{
  VECTVTIMERS timers;
  PVR_ERROR err;

  if (!m_clients[clientID])
    return 0;

  err = m_clients[clientID]->GetTimers(timers);

  if(err == PVR_ERROR_NO_ERROR)
  {
    // first check there are no stored timers for this client
    CSingleLock timerLock(m_timersSection);
    std::map< long, VECTVTIMERS >::iterator itr = m_timers.begin();
    while (itr != m_timers.end())
    {
      if (clientID == (*itr).first)
      {
        m_timers.erase(itr);
        if (m_timers.empty())
          break;
      }
      else
        itr++;
    }

    // store the timers for this client
    m_timers.insert(std::make_pair(clientID, timers));
  }
  return timers.size();
}

int CPVRManager::GetRecordings(long clientID)
{
  /*CFileItemList* recordings = new CFileItemList();
  m_clients[clientID]->GetRecordings(recordings);
  return recordings->Size();*/
  return 0;
}

void CPVRManager::LostConnection()
{
  /* Set lost flag */
  /*m_bConnectionLost = true;*/

  /* And inform the user about the lost connection */
  CGUIDialogOK::ShowAndGetInput(18090,0,18093,0);
  return;
}
