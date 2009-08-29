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
#include "Application.h"
#include "GUISettings.h"
#include "Util.h"
#include "GUIWindowTV.h"
#include "GUIWindowManager.h"
#include "utils/GUIInfoManager.h"
#include "settings/AddonSettings.h"
#include "PVRManager.h"
#include "pvrclients/PVRClientFactory.h"
#include "MusicInfoTag.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif

/* GUI Messages includes */
#include "GUIDialogOK.h"

#define CHANNELCHECKDELTA     600 // seconds before checking for changes inside channels list
#define TIMERCHECKDELTA       300 // seconds before checking for changes inside timers list
#define RECORDINGCHECKDELTA   450 // seconds before checking for changes inside recordings list

using namespace std;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace ADDON;

CLIENTMAP CPVRManager::m_clients;
CPVRManager* CPVRManager::m_instance              = NULL;
CFileItem* CPVRManager::m_currentPlayingChannel   = NULL;
CFileItem* CPVRManager::m_currentPlayingRecording = NULL;
bool CPVRManager::m_hasRecordings                 = false;
bool CPVRManager::m_isRecording                   = false;
bool CPVRManager::m_hasTimers                     = false;


CPVRTimeshiftRcvr::CPVRTimeshiftRcvr(IPVRClient *client)
{
  m_pFile         = NULL;
  m_client        = client;
  m_MaxSizeStatic = g_guiSettings.GetInt("pvrrecord.timeshiftcache") * 1024 * 1024;

  m_pFile = new CFile();
  if (!m_pFile)
  {
    return;
  }

  if (!m_pFile->OpenForWrite(g_guiSettings.GetString("pvrrecord.timeshiftpath")+"/.timeshift_cache.ts", true))
  {
    delete m_pFile;
    m_pFile = NULL;
    return;
  }
}

CPVRTimeshiftRcvr::~CPVRTimeshiftRcvr()
{
  StopThread();

  if (m_pFile)
  {
    m_pFile->Close();
    m_pFile->Delete(g_guiSettings.GetString("pvrrecord.timeshiftpath")+"/.timeshift_cache.ts");
    delete m_pFile;
    m_pFile = NULL;
  }
}

bool CPVRTimeshiftRcvr::StartReceiver()
{
  m_MaxSize   = m_MaxSizeStatic;
  m_written   = 0;
  m_position  = 0;
  m_pFile->Seek(0);

  Create();
  SetName("PVR Timeshift receiver");
  SetPriority(5);
  return true;
}

void CPVRTimeshiftRcvr::StopReceiver()
{
  StopThread();
}

void CPVRTimeshiftRcvr::SetClient(IPVRClient *client)
{
  m_client = client;
}

void CPVRTimeshiftRcvr::Process()
{
  while (!m_bStop)
  {
    int ret = m_client->ReadLiveStream(buf, sizeof(buf));
    m_position += m_pFile->Write(buf, ret);
    m_written  += ret;
    
    if (m_position >= m_MaxSizeStatic)
    {
      m_MaxSize  = m_position;
      m_position = 0;
      m_pFile->Seek(0);
    }

    Sleep(5);
  }
}

__int64 CPVRTimeshiftRcvr::GetMaxSize()
{ 
  return m_MaxSize;
}

int CPVRTimeshiftRcvr::WriteBuffer(BYTE* buf, int buf_size)
{
  int ret = m_pFile->Write(buf, buf_size);
  m_position += ret;

  if (m_position >= m_MaxSize)
  {
    m_position = 0;
    m_pFile->Seek(0);
  }

  return ret;
}



/************************************************************/
/** Class handling */

CPVRManager::CPVRManager()
{
  InitializeCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: created");
}

CPVRManager::~CPVRManager()
{
  DeleteCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: destroyed");
}

void CPVRManager::Start()
{
  /* First stop and remove any clients */
  if (!m_clients.empty())
    Stop();

  /* Check if TV is enabled under Settings->Video->TV->Enable */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVR: PVRManager starting");

  /* Reset Member variables and System Info swap counters */
  m_CurrentGroupID          = -1;
  m_currentPlayingChannel   = NULL;
  m_currentPlayingRecording = NULL;
  m_PreviousChannel[0]      = 1;
  m_PreviousChannel[1]      = 1;
  m_PreviousChannelIndex    = 0;
  m_LastChannelChanged      = timeGetTime();
  m_LastChannel             = 0;
  m_infoToggleStart         = NULL;
  m_infoToggleCurrent       = 0;
  m_TimeshiftReceiver       = NULL;
  m_pTimeshiftFile          = NULL;
  m_timeshift               = false;

  /* Discover, load and create chosen Client add-on's. */
  g_addonmanager.RegisterAddonCallback(ADDON_PVRDLL, this);
  if (!LoadClients())
  {
    CLog::Log(LOGERROR, "PVR: couldn't load clients");
    return;
  }

  /* Get TV Channels from Backends */
  PVRChannelsTV.Load(false);

  /* Get Radio Channels from Backends */
  PVRChannelsRadio.Load(true);

  /* Get Timers from Backends */
  PVRTimers.Load();

  /* Get Recordings from Backend */
  PVRRecordings.Load();

  /* Get Epg's from Backend */
  cPVREpgs::Load();

  /* Create the supervisor thread to do all background activities */
  Create();
  SetName("XBMC PVR Supervisor");
  SetPriority(-15);

  CLog::Log(LOGNOTICE, "PVR: PVRManager started. Clients loaded = %u", m_clients.size());
  return;
}

void CPVRManager::Stop()
{
  CLog::Log(LOGNOTICE, "PVR: PVRManager stoping");
  StopThread();

  for (unsigned int i = 0; i < m_clients.size(); i++)
  {
    delete m_clients[i];
  }
  m_clients.clear();
  m_clientsProps.clear();

  if (m_pTimeshiftFile)
  {
    delete m_pTimeshiftFile;
    m_pTimeshiftFile = NULL;
  }

  return;
}

bool CPVRManager::LoadClients()
{
  /* Get all PVR Add on's */
  VECADDONS *addons = g_addonmanager.GetAddonsFromType(ADDON_PVRDLL);

  /* Make sure addon's are loaded */
  if (addons == NULL || addons->empty())
    return false;

  m_database.Open();

  /* load the clients */
  CPVRClientFactory factory;
  for (unsigned i=0; i < addons->size(); i++)
  {
    const CAddon& clientAddon = addons->at(i);

    if (clientAddon.m_disabled) // ignore disabled addons
      continue;

    /* Add client to TV-Database to identify different backend types,
     * if client is already added his id is given.
     */
    long clientID = m_database.AddClient(clientAddon.m_strName, clientAddon.m_guid);
    if (clientID == -1)
    {
      CLog::Log(LOGERROR, "PVR: Can't Add/Get PVR Client '%s' to to TV Database", clientAddon.m_strName.c_str());
      continue;
    }

    /* Load the Client library's and inside them into Client list if
     * success. Client initialization is also performed during loading.
     */
    IPVRClient *client = factory.LoadPVRClient(clientAddon, clientID, this);
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

unsigned long CPVRManager::GetFirstClientID()
{
  CLIENTMAPITR itr = m_clients.begin();
  return m_clients[(*itr).first]->GetID();
}

void CPVRManager::GetClientProperties()
{
  m_clientsProps.clear();
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    GetClientProperties((*itr).first);
    itr++;
  }
}

void CPVRManager::GetClientProperties(long clientID)
{
  PVR_SERVERPROPS props;
  if (m_clients[clientID]->GetProperties(&props) == PVR_ERROR_NO_ERROR)
  {
    // store the client's properties
    m_clientsProps.insert(std::make_pair(clientID, props));
  }
}

bool CPVRManager::HaveActiveClients()
{
  if (m_clients.empty())
    return false;

  int ready = 0;
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->ReadyToUse())
      ready++;
    itr++;
  }
  return ready > 0 ? true : false;
}

void CPVRManager::Process()
{
  DWORD Now = timeGetTime();
  DWORD LastTVChannelCheck = Now;
  DWORD LastRadioChannelCheck = Now-CHANNELCHECKDELTA*1000/2;
  
  while (!m_bStop)
  {
    Now = timeGetTime();

    /* Check for new or updated TV Channels */
    if (Now - LastTVChannelCheck > CHANNELCHECKDELTA*1000) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating TV Channel list");
      PVRChannelsTV.Update();
      LastTVChannelCheck = Now;
    }
    /* Check for new or updated Radio Channels */
    if (Now - LastRadioChannelCheck > CHANNELCHECKDELTA*1000) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating Radio Channel list");
      PVRChannelsRadio.Update();
      LastRadioChannelCheck = Now;
    }    

    Sleep(1000);
  }
}


/************************************************************/
/** Manager access */

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

void CPVRManager::RemoveInstance()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance = NULL;
  }
}


/************************************************************/
/** Event handling */

#define INFO_TOGGLE_TIME    1500
const char* CPVRManager::TranslateInfo(DWORD dwInfo)
{
  if      (dwInfo == PVR_NOW_RECORDING_TITLE)     return m_nowRecordingTitle;
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)   return m_nowRecordingChannel;
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)  return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return m_nextRecordingChannel;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;
  else if (dwInfo == PVR_BACKEND_NAME)            return m_backendName;
  else if (dwInfo == PVR_BACKEND_VERSION)         return m_backendVersion;
  else if (dwInfo == PVR_BACKEND_HOST)            return m_backendHost;
  else if (dwInfo == PVR_BACKEND_DISKSPACE)       return m_backendDiskspace;
  else if (dwInfo == PVR_BACKEND_CHANNELS)        return m_backendChannels;
  else if (dwInfo == PVR_BACKEND_TIMERS)          return m_backendTimers;
  else if (dwInfo == PVR_BACKEND_RECORDINGS)      return m_backendRecordings;
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

    static CStdString backendClients;
    if (m_clients.size() > 0)
      backendClients.Format("%u %s %u",m_infoToggleCurrent+1 ,g_localizeStrings.Get(20163), m_clients.size());
    else
      backendClients = g_localizeStrings.Get(14023);

    return backendClients;
  }
  else if (dwInfo == PVR_TOTAL_DISKSPACE)
  {
    long long kBTotal = 0;
    long long kBUsed  = 0;
    CLIENTMAPITR itr = m_clients.begin();
    while (itr != m_clients.end())
    {
      long long clientKBTotal = 0;
      long long clientKBUsed  = 0;

      if (m_clients[(*itr).first]->GetDriveSpace(&clientKBTotal, &clientKBUsed) == PVR_ERROR_NO_ERROR)
      {
        kBTotal += clientKBTotal;
        kBUsed += clientKBUsed;
      }
      itr++;
    }
    kBTotal /= 1024; // Convert to MBytes
    kBUsed /= 1024;  // Convert to MBytes
    m_totalDiskspace.Format("%s %0.f GByte - %s: %0.f GByte", g_localizeStrings.Get(18055), (float) kBTotal / 1024, g_localizeStrings.Get(156), (float) kBUsed / 1024);
    return m_totalDiskspace;
  }
  else if (dwInfo == PVR_NEXT_TIMER)
  {
    cPVRTimerInfoTag *next = PVRTimers.GetNextActiveTimer();
    if (next != NULL)
    {
      m_nextTimer.Format("%s %s %s %s", g_localizeStrings.Get(18190)
                         , next->Start().GetAsLocalizedDate(true)
                         , g_localizeStrings.Get(18191)
                         , next->Start().GetAsLocalizedTime("HH:mm", false));
      return m_nextTimer;
    }
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
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld timers changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
    	  pTVWin->UpdateData(TV_WINDOW_TIMERS);
	  }
      break;

    case PVR_EVENT_RECORDINGS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld recording list changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
    	  pTVWin->UpdateData(TV_WINDOW_RECORDINGS);
	  }
      break;

    case PVR_EVENT_CHANNELS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld channel list changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
		{
    	  pTVWin->UpdateData(TV_WINDOW_CHANNELS_TV);
		  pTVWin->UpdateData(TV_WINDOW_CHANNELS_RADIO);
		}
	  }
      break;
    default:
      break;
  }
}

ADDON_STATUS CPVRManager::SetSetting(const CAddon* addon, const char *settingName, const void *settingValue)
{
  if (!addon)
    return STATUS_UNKNOWN;

  CLog::Log(LOGINFO, "PVR: set setting of clientName: %s, settingName: %s", addon->m_strName.c_str(), settingName);
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        return m_clients[(*itr).first]->SetSetting(settingName, settingValue);
      }
    }
    itr++;
  }
  return STATUS_UNKNOWN;
}

bool CPVRManager::RequestRestart(const CAddon* addon, bool datachanged)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "PVR: requested restart of clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        CLog::Log(LOGINFO, "PVR: restarting clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
        StopThread();
        if (m_clients[(*itr).first]->ReInit())
        {
          /* Get TV Channels from Backends */
          PVRChannelsTV.Update();

          /* Get Radio Channels from Backends */
          PVRChannelsRadio.Update();

          /* Get Timers from Backends */
          PVRTimers.Update();

          /* Get Recordings from Backend */
          PVRRecordings.Update();
        }
        Create();
      }
    }
    itr++;
  }
  return true;
}

bool CPVRManager::RequestRemoval(const CAddon* addon)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "PVR: requested removal of clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        CLog::Log(LOGINFO, "PVR: removing clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
        m_clients[(*itr).first]->Remove();
        m_clients.erase((*itr).first);
        return true;
      }
    }
    itr++;
  }

  return false;
}


bool CPVRManager::IsPlayingTV()
{
  if (!m_currentPlayingChannel)
    return false;

  return !m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio();
}

bool CPVRManager::IsPlayingRadio()
{
  if (!m_currentPlayingChannel)
    return false;

  return m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio();
}


/************************************************************/
/** Channel handling */

int CPVRManager::GetGroupList(CFileItemList* results)
{
  for (unsigned int i = 0; i < m_channel_group.size(); i++)
  {
    CFileItemPtr group(new CFileItem(m_channel_group[i].m_Title));
    group->m_strTitle = m_channel_group[i].m_Title;
    group->m_strPath.Format("%i", m_channel_group[i].m_ID);
    results->Add(group);
  }
  return m_channel_group.size();
}

void CPVRManager::AddGroup(const CStdString &newname)
{
  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_database.AddGroup(GetFirstClientID(), newname);
  m_database.GetGroupList(GetFirstClientID(), &m_channel_group);

  m_database.Close();
  LeaveCriticalSection(&m_critSection);
}

bool CPVRManager::RenameGroup(unsigned int GroupId, const CStdString &newname)
{
  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_database.RenameGroup(GetFirstClientID(), GroupId, newname);
  m_database.GetGroupList(GetFirstClientID(), &m_channel_group);

  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::DeleteGroup(unsigned int GroupId)
{
  EnterCriticalSection(&m_critSection);
//  m_database.Open();
//
//  m_database.DeleteGroup(GetFirstClientID(), GroupId);
//
//  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
//  {
//    if (PVRChannelsTV[i].GroupID() == GroupId)
//    {
//      PVRChannelsTV[i].m_iGroupID = 0;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsTV[i]);
//    }
//  }
//  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
//  {
//    if (PVRChannelsRadio[i].GroupID() == GroupId)
//    {
//      PVRChannelsRadio[i].m_iGroupID = 0;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsRadio[i]);
//    }
//  }
//  m_database.GetGroupList(GetFirstClientID(), &m_channel_group);
//  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::ChannelToGroup(unsigned int number, unsigned int GroupId, bool radio)
{
//  if (!radio)
//  {
//    if (((int) number <= PVRChannelsTV.size()+1) && (number != 0))
//    {
//      EnterCriticalSection(&m_critSection);
//      m_database.Open();
//      PVRChannelsTV[number-1].m_iGroupID = GroupId;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsTV[number-1]);
//      m_database.Close();
//      LeaveCriticalSection(&m_critSection);
//      return true;
//    }
//  }
//  else
//  {
//    if (((int) number <= PVRChannelsRadio.size()+1) && (number != 0))
//    {
//      EnterCriticalSection(&m_critSection);
//      m_database.Open();
//      PVRChannelsRadio[number-1].m_iGroupID = GroupId;
//      m_database.UpdateChannel(GetFirstClientID(), PVRChannelsRadio[number-1]);
//      m_database.Close();
//      LeaveCriticalSection(&m_critSection);
//      return true;
//    }
//  }
  return false;
}

int CPVRManager::GetPrevGroupID(int current_group_id)
{
  if (m_channel_group.size() == 0)
    return -1;

  if ((current_group_id == -1) || (current_group_id == 0))
    return m_channel_group[m_channel_group.size()-1].m_ID;

  for (unsigned int i = 0; i < m_channel_group.size(); i++)
  {
    if (current_group_id == m_channel_group[i].m_ID)
    {
      if (i != 0)
        return m_channel_group[i-1].m_ID;
      else
        return -1;
    }
  }
  return -1;
}

int CPVRManager::GetNextGroupID(int current_group_id)
{
  unsigned int i = 0;

  if (m_channel_group.size() == 0)
    return -1;

  if ((current_group_id == 0) || (current_group_id == -1))
    return m_channel_group[0].m_ID;

  if (m_channel_group.size() == 0)
    return -1;

  for (; i < m_channel_group.size(); i++)
  {
    if (current_group_id == m_channel_group[i].m_ID)
      break;
  }

  if (i >= m_channel_group.size()-1)
    return -1;
  else
    return m_channel_group[i+1].m_ID;
}

CStdString CPVRManager::GetGroupName(int GroupId)
{
  if (GroupId == -1)
    return g_localizeStrings.Get(593);

  for (unsigned int i = 0; i < m_channel_group.size(); i++)
  {
    if (GroupId == m_channel_group[i].m_ID)
      return m_channel_group[i].m_Title;
  }

  return g_localizeStrings.Get(593);
}

int CPVRManager::GetFirstChannelForGroupID(int GroupId, bool radio)
{
  if (GroupId == -1)
    return 1;

  if (!radio)
  {
    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      if (PVRChannelsTV[i].GroupID() == GroupId)
        return i+1;
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      if (PVRChannelsRadio[i].GroupID() == GroupId)
        return i+1;
    }
  }
  return 1;
}


/************************************************************/
/**  Backend Channel handling **/

bool CPVRManager::AddBackendChannel(const CFileItem &item)
{
  CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
  return false;
}

bool CPVRManager::DeleteBackendChannel(unsigned int index)
{
  CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
  return false;
}

bool CPVRManager::RenameBackendChannel(unsigned int index, CStdString &newname)
{
  CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
  return false;
}

bool CPVRManager::MoveBackendChannel(unsigned int index, unsigned int newindex)
{
  CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
  return false;
}

bool CPVRManager::UpdateBackendChannel(const CFileItem &item)
{
  CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
  return false;
}



/************************************************************/
/** Teletext handling **/

bool CPVRManager::TeletextPagePresent(const CFileItem &item, int Page, int subPage)
{
  /* Check if a cPVRChannelInfoTag is inside file item */
  if (!item.IsTVChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: TeletextPagePresent no TVChannelTag given!");
    return false;
  }

  const cPVRChannelInfoTag* tag = item.GetTVChannelInfoTag();

  EnterCriticalSection(&m_critSection);
  bool ret = m_clients[tag->ClientID()]->TeletextPagePresent(tag->ClientNumber(), Page, subPage);
  LeaveCriticalSection(&m_critSection);
  return ret;
}

bool CPVRManager::GetTeletextPage(const CFileItem &item, int Page, int subPage, BYTE* buf)
{
  /* Check if a cPVRTimerInfoTag is inside file item */
  if (!item.IsTVChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: GetTeletextPage no TVChannelTag given!");
    return false;
  }

  const cPVRChannelInfoTag* tag = item.GetTVChannelInfoTag();

  EnterCriticalSection(&m_critSection);
  bool ret = m_clients[tag->ClientID()]->ReadTeletextPage(buf, tag->ClientNumber(), Page, subPage);
  LeaveCriticalSection(&m_critSection);
  return ret;
}


/************************************************************/
/** Live stream handling **/

bool CPVRManager::OpenLiveStream(unsigned int channel, bool radio)
{
  m_scanStart = timeGetTime();

  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;

  if (!radio)
    m_currentPlayingChannel = new CFileItem(PVRChannelsTV[channel-1]);
  else
    m_currentPlayingChannel = new CFileItem(PVRChannelsRadio[channel-1]);

  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (!m_clients[tag->ClientID()]->OpenLiveStream(tag->ClientNumber()))
  {
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
    LeaveCriticalSection(&m_critSection);
    return false;
  }

  SetCurrentPlayingProgram(*m_currentPlayingChannel);
  m_bPaused           = false;
  m_timeshiftTimeDiff = 0;
      
  /* Start Timeshift buffering */
  if (g_guiSettings.GetBool("pvrrecord.timeshift"))
  {
    if (g_guiSettings.GetString("pvrrecord.timeshiftpath") != "")
    {
      unsigned int flags  = READ_BUFFERED;
      m_TimeshiftReceiver = new CPVRTimeshiftRcvr(m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]);
      m_pTimeshiftFile    = new CFile();
      m_timeshiftDelta    = 0;
      m_timeshiftReaded   = 0;
      if (m_pTimeshiftFile->Open(g_guiSettings.GetString("pvrrecord.timeshiftpath")+"/.timeshift_cache.ts", flags))
      {
        m_timeshift = true;
        m_TimeshiftReceiver->StartReceiver();
      }
    }
  }

  LeaveCriticalSection(&m_critSection);
  return true;
}

void CPVRManager::CloseLiveStream()
{
  if (!m_currentPlayingChannel)
    return;

  EnterCriticalSection(&m_critSection);

  /* Stop the Timeshift receiving if active */
  if (m_timeshift)
  {
    m_pTimeshiftFile->Close();
    delete m_pTimeshiftFile;
    delete m_TimeshiftReceiver;
    m_TimeshiftReceiver = NULL;
    m_pTimeshiftFile    = NULL;
    m_timeshift         = false;
  }

  /* Close the Client connection */
  m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->CloseLiveStream();
  delete m_currentPlayingChannel;
  m_currentPlayingChannel = NULL;

  LeaveCriticalSection(&m_critSection);
  return;
}

bool CPVRManager::PauseLiveStream(bool DoPause, double dTime)
{
  /* Save the XBMC runtime if Pause is pressed to calculate
     difference between shifted and true position */
  if (DoPause)
    m_timeshiftTimePause = timeGetTime() - m_timeshiftTimeDiff*1000;
  m_bPaused = DoPause;
}

int CPVRManager::ReadLiveStream(BYTE* buf, int buf_size)
{
  /* Check for a open channel indicated by the m_currentPlayingChannel pointer */
  if (!m_currentPlayingChannel)
    return 0;

  /* Check stream for available video or audio data, if after the scantime no stream
     is present playback is canceled and returns to the window */
  if (m_scanStart)
  {
    if (timeGetTime() - m_scanStart > g_guiSettings.GetInt("pvrplayback.scantime")*1000)
      return 0;
    else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
      m_scanStart = NULL;
  }
  
  /* Process Timeshift based stream reading from buffer file */
  if (m_timeshift)
  {
    if (!m_pTimeshiftFile) return -1;

    DWORD now = timeGetTime();

    /* Never read behind current write position inside cache */
    while (m_timeshiftReaded+buf_size+131072 > m_TimeshiftReceiver->GetWritten())
    {
      if (timeGetTime() - now > 5*1000)
      {
        CLog::Log(LOGERROR,"PVR: Timeshift Cache Position timeout");
        return 0;
      }
      Sleep(5);
    }
    
    /* Reduce buffer size to prevent partly read behind range */
    int tmp = m_TimeshiftReceiver->GetMaxSize() - m_pTimeshiftFile->GetPosition();
    if (tmp > 0 && tmp < buf_size)
      buf_size = tmp;

REPEAT_READ:
    int ret = m_pTimeshiftFile->Read(buf, buf_size);
    if (ret <= 0)
    {
      if (timeGetTime() - now > 5*1000)
      {
        CLog::Log(LOGERROR,"PVR: Timeshift Cache Read timeout");
        return 0;
      }
      
      Sleep(5);
      goto REPEAT_READ;
    }

    /* Check if we are at end of buffer file, if yes wrap around and
       start at beginning */
    if (m_pTimeshiftFile->GetPosition() >= m_TimeshiftReceiver->GetMaxSize())
    {
      CLog::Log(LOGDEBUG,"PVR: Timeshift Cache wrap around");
      m_pTimeshiftFile->Seek(0);
    }

    m_timeshiftReaded += ret;
    return ret;
  }
  /* Read stream directly from client without a buffer file */
  else
  {
    return m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->ReadLiveStream(buf, buf_size);
  }
}

__int64 CPVRManager::SeekLiveStream(__int64 pos, int whence)
{
  return 0;
}

int CPVRManager::GetCurrentChannel(bool radio)
{
  if (!m_currentPlayingChannel)
    return 1;

  return m_currentPlayingChannel->GetTVChannelInfoTag()->Number();
}

CFileItem *CPVRManager::GetCurrentChannelItem()
{
  return m_currentPlayingChannel;
}

PVR_SERVERPROPS *CPVRManager::GetCurrentClientProps()
{
  if (!m_currentPlayingChannel)
    return NULL;

  return &m_clientsProps[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()];
}



bool CPVRManager::ChannelSwitch(unsigned int iChannel)
{
  if (!m_currentPlayingChannel)
    return false;

  cPVRChannels *channels;
  if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  if (iChannel > channels->size()+1)
  {
    CGUIDialogOK::ShowAndGetInput(18100,18105,0,0);
    return false;
  }

  EnterCriticalSection(&m_critSection);

  /* If Timeshift is active reset all relevant data and stop the receiver */
  if (m_timeshift)
  {
    m_timeshiftDelta  = 0;
    m_timeshiftReaded = 0;
    m_TimeshiftReceiver->StopReceiver();
    m_pTimeshiftFile->Seek(0);
  }
  m_bPaused           = false;
  m_timeshiftTimeDiff = 0;

  /* Perform Channelswitch */
  if (!m_clients[channels->at(iChannel-1).ClientID()]->SwitchChannel(channels->at(iChannel-1).ClientNumber()))
  {
    CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
    LeaveCriticalSection(&m_critSection);
    return false;
  }
  
  /* Start the Timeshift receiver again if active */
  if (m_timeshift)
  {
    m_TimeshiftReceiver->SetClient(m_clients[channels->at(iChannel-1).ClientID()]);
    m_TimeshiftReceiver->StartReceiver();
  }

  /* Update the Playing channel data and the current epg data */
  delete m_currentPlayingChannel;
  m_currentPlayingChannel = new CFileItem(channels->at(iChannel-1));
  m_scanStart = timeGetTime();
  SetCurrentPlayingProgram(*m_currentPlayingChannel);

  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::ChannelUp(unsigned int *newchannel)
{
  if (m_currentPlayingChannel)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    int currentTVChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel += 1;

      if (currentTVChannel > channels->size())
        currentTVChannel = 1;

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != channels->at(currentTVChannel-1).GroupID()))
        continue;

      /* If Timeshift is active reset all relevant data and stop the receiver */
      if (m_timeshift)
      {
        m_timeshiftDelta  = 0;
        m_timeshiftReaded = 0;
        m_TimeshiftReceiver->StopReceiver();
        m_pTimeshiftFile->Seek(0);
      }
      m_bPaused           = false;
      m_timeshiftTimeDiff = 0;

      /* Perform Channelswitch */
      if (m_clients[channels->at(currentTVChannel-1).ClientID()]->SwitchChannel(channels->at(currentTVChannel-1).ClientNumber()))
      {
        /* Update the Playing channel data and the current epg data */
        m_scanStart = timeGetTime();
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(channels->at(currentTVChannel-1));
        SetCurrentPlayingProgram(*m_currentPlayingChannel);

        /* Start the Timeshift receiver again if active */
        if (m_timeshift)
        {
          m_TimeshiftReceiver->SetClient(m_clients[channels->at(currentTVChannel-1).ClientID()]);
          m_TimeshiftReceiver->StartReceiver();
        }
        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    LeaveCriticalSection(&m_critSection);
  }

  return false;
}

bool CPVRManager::ChannelDown(unsigned int *newchannel)
{
  if (m_currentPlayingChannel)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    int currentTVChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel -= 1;

      if (currentTVChannel <= 0)
        currentTVChannel = channels->size();

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != channels->at(currentTVChannel-1).GroupID()))
        continue;

      /* If Timeshift is active reset all relevant data and stop the receiver */
      if (m_timeshift)
      {
        m_timeshiftDelta  = 0;
        m_timeshiftReaded = 0;
        m_TimeshiftReceiver->StopReceiver();
        m_pTimeshiftFile->Seek(0);
      }
      m_bPaused           = false;
      m_timeshiftTimeDiff = 0;

      /* Perform Channelswitch */
      if (m_clients[channels->at(currentTVChannel-1).ClientID()]->SwitchChannel(channels->at(currentTVChannel-1).ClientNumber()))
      {
        /* Update the Playing channel data and the current epg data */
        m_scanStart = timeGetTime();
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(channels->at(currentTVChannel-1));
        SetCurrentPlayingProgram(*m_currentPlayingChannel);

        /* Start the Timeshift receiver again if active */
        if (m_timeshift)
        {
          m_TimeshiftReceiver->SetClient(m_clients[channels->at(currentTVChannel-1).ClientID()]);
          m_TimeshiftReceiver->StartReceiver();
        }
        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    LeaveCriticalSection(&m_critSection);
  }

  return false;
}

int CPVRManager::GetTotalTime()
{
  if (!m_currentPlayingChannel)
    return 0;

  return m_currentPlayingChannel->GetTVChannelInfoTag()->GetDuration() * 1000;
}

int CPVRManager::GetStartTime()
{
  if (!m_currentPlayingChannel)
    return 0;

  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (tag->EndTime() < CDateTime::GetCurrentDateTime())
  {
    SetCurrentPlayingProgram(*m_currentPlayingChannel);

    if (UpdateItem(*m_currentPlayingChannel))
    {
      g_application.CurrentFileItem() = *m_currentPlayingChannel;
      g_infoManager.SetCurrentItem(*m_currentPlayingChannel);
    }
  }
  
  /* Correct EPG Time with difference to paused Timeshift stream */
  if (m_timeshift && m_bPaused)
    m_timeshiftTimeDiff = (timeGetTime() - m_timeshiftTimePause) / 1000;
  
  CDateTimeSpan time = CDateTime::GetCurrentDateTime() - tag->StartTime();
  return time.GetDays()    * 1000 * 60 * 60 * 24
       + time.GetHours()   * 1000 * 60 * 60
       + time.GetMinutes() * 1000 * 60
       + time.GetSeconds() * 1000
       - m_timeshiftTimeDiff * 1000;
}

bool CPVRManager::UpdateItem(CFileItem& item)
{
  if (!item.IsTVChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: UpdateItem no TVChannelTag given!");
    return false;
  }

  cPVRChannelInfoTag* tag = item.GetTVChannelInfoTag();
  cPVRChannelInfoTag* current = m_currentPlayingChannel->GetTVChannelInfoTag();

  tag->m_strAlbum         = current->Name();
  tag->m_strTitle         = current->m_strTitle;
  tag->m_strOriginalTitle = current->m_strTitle;
  tag->m_strPlotOutline   = current->m_strPlotOutline;
  tag->m_strPlot          = current->m_strPlot;
  tag->m_strGenre         = current->m_strGenre;
  tag->m_strPath          = current->Path();
  tag->m_strShowTitle.Format("%i", current->Number());
  tag->SetNextTitle(current->NextTitle());
  tag->SetPath(current->Path());

  item.m_strTitle = current->Name();
  item.m_dateTime = current->StartTime();
  item.m_strPath  = current->Path();

  CDateTimeSpan span = current->StartTime() - current->EndTime();
  StringUtils::SecondsToTimeString(span.GetSeconds() + span.GetMinutes() * 60 + span.GetHours() * 3600,
                                   tag->m_strRuntime,
                                   TIME_FORMAT_GUESS);

  if (current->Icon() != "")
  {
    item.SetThumbnailImage(current->Icon());
  }
  else
  {
    item.SetThumbnailImage("");
    item.FillInDefaultIcon();
  }

  g_infoManager.SetCurrentItem(item);
  g_application.CurrentFileItem().m_strPath = item.m_strPath;
  return true;
}

void CPVRManager::SetPlayingGroup(int GroupId)
{
  m_CurrentGroupID = GroupId;
}

int CPVRManager::GetPlayingGroup()
{
  return m_CurrentGroupID;
}


/************************************************************/
/** Recorded stream handling **/

bool CPVRManager::OpenRecordedStream(unsigned int record)
{
  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  m_currentPlayingRecording = new CFileItem(PVRRecordings[record-1]);

  if (m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->OpenRecordedStream(PVRRecordings[record-1]))
  {
    LeaveCriticalSection(&m_critSection);
    return true;
  }

  LeaveCriticalSection(&m_critSection);
  return false;
}

void CPVRManager::CloseRecordedStream(void)
{
  if (!m_currentPlayingRecording)
    return;

  EnterCriticalSection(&m_critSection);

  m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->CloseRecordedStream();
  delete m_currentPlayingRecording;
  m_currentPlayingRecording = NULL;

  LeaveCriticalSection(&m_critSection);
  return;
}

int CPVRManager::ReadRecordedStream(BYTE* buf, int buf_size)
{
  EnterCriticalSection(&m_critSection);
  int ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->ReadRecordedStream(buf, buf_size);
  LeaveCriticalSection(&m_critSection);
  return ret;
}

__int64 CPVRManager::SeekRecordedStream(__int64 pos, int whence)
{
  return m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->SeekRecordedStream(pos, whence);
}

__int64 CPVRManager::LengthRecordedStream(void)
{
  return m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->LengthRecordedStream();
}

bool CPVRManager::IsRecording(unsigned int channel, bool radio)
{
  if (!radio)
  {
    return PVRChannelsTV[channel-1].IsRecording();
  }
  else
  {
    return PVRChannelsRadio[channel-1].IsRecording();
  }

  return false;
}

bool CPVRManager::RecordChannel(unsigned int channel, bool bOnOff, bool radio)
{
  if (!radio)
  {
    if (bOnOff && PVRChannelsTV[channel-1].IsRecording() == false)
    {
      cPVRTimerInfoTag newtimer(true);
      CFileItem *item = new CFileItem(newtimer);

      if (!cPVRTimers::AddTimer(*item))
      {
        CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
        return true;
      }

      PVRChannelsTV[channel-1].SetRecording(true);
    }
    else if (PVRChannelsTV[channel-1].IsRecording() == true)
    {
      for (unsigned int i = 0; i < PVRTimers.size(); ++i)
      {
        if ((PVRTimers[i].Number() == PVRChannelsTV[channel-1].Number()) &&
            (PVRTimers[i].Start() <= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[i].Stop() >= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[i].IsRepeating() != true) && (PVRTimers[i].Active() == true))
        {
          cPVRTimers::DeleteTimer(PVRTimers[i], true);
        }
      }

      PVRChannelsTV[channel-1].SetRecording(false);
    }
  }
  else
  {
    if (bOnOff && PVRChannelsRadio[channel-1].IsRecording() == false)
    {
      cPVRTimerInfoTag newtimer(true);
      CFileItem *item = new CFileItem(newtimer);

      if (!cPVRTimers::AddTimer(*item))
      {
        CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
        return true;
      }

      PVRChannelsRadio[channel-1].SetRecording(true);
    }
    else if (PVRChannelsRadio[channel-1].IsRecording() == true)
    {
      for (unsigned int i = 0; i < PVRTimers.size(); ++i)
      {
        if ((PVRTimers[i].Number() == PVRChannelsRadio[channel-1].Number()) &&
            (PVRTimers[i].Start() <= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[i].Stop() >= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[i].IsRepeating() != true) && (PVRTimers[i].Active() == true))
        {
          cPVRTimers::DeleteTimer(PVRTimers[i], true);
        }
      }

      PVRChannelsRadio[channel-1].SetRecording(false);
    }
  }
  return false;
}


/************************************************************/
/** Internal handling **/

void CPVRManager::SyncInfo()
{
  PVRRecordings.GetNumRecordings() > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  PVRTimers.GetNumTimers()     > 0 ? m_hasTimers     = true : m_hasTimers = false;
  m_isRecording = false;

  if (m_hasTimers)
  {
    cPVRTimerInfoTag *nextRec = PVRTimers.GetNextActiveTimer();

    m_nextRecordingTitle    = nextRec->Title();
    m_nextRecordingChannel  = PVRChannelsTV.GetNameForChannel(nextRec->Number());
    m_nextRecordingDateTime = nextRec->Start().GetAsLocalizedDateTime(false, false);

    if (nextRec->IsRecording() == true)
    {
      m_isRecording = true;
      CLog::Log(LOGDEBUG, "%s - PVR: next timer is currently recording", __FUNCTION__);
    }
    else
    {
      m_isRecording = false;
    }
  }

  if (m_isRecording)
  {
    m_nowRecordingTitle = m_nextRecordingTitle;
    m_nowRecordingDateTime = m_nextRecordingDateTime;
    m_nowRecordingChannel = m_nextRecordingChannel;
    CLog::Log(LOGDEBUG, "%s - PVR: data of active recording is used: '%s', '%s', '%s'", __FUNCTION__,
                        m_nowRecordingTitle.c_str(),
                        m_nextRecordingDateTime.c_str(),
                        m_nextRecordingChannel.c_str());
  }
  else
  {
    m_nowRecordingTitle.clear();
    m_nowRecordingDateTime.clear();
    m_nowRecordingChannel.clear();
  }
}

void CPVRManager::SetCurrentPlayingProgram(CFileItem& item)
{
  /* Check if a cPVRChannelInfoTag is inside file item */
  if (!item.IsTVChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: SetCurrentPlayingProgram no TVChannelTag given!");
    return;
  }

  cPVRChannelInfoTag* tag = item.GetTVChannelInfoTag();
  if (tag != NULL)
  {
    if (tag->Number() != m_LastChannel)
    {
      m_LastChannel = tag->Number();
      m_LastChannelChanged = timeGetTime();
    }

    if (tag->Number() != m_PreviousChannel[m_PreviousChannelIndex])
      m_PreviousChannel[m_PreviousChannelIndex ^= 1] = tag->Number();

    const CTVEPGInfoTag *epgnow = NULL;
    const CTVEPGInfoTag *epgnext = NULL;
    cPVREpgsLock EpgsLock;
    cPVREpgs *s = (cPVREpgs *)cPVREpgs::EPGs(EpgsLock);
    if (s)
    {
      epgnow = s->GetEPG(tag, true)->GetInfoTagNow();
      epgnext = s->GetEPG(tag, true)->GetInfoTagNext();
    }

    if (epgnow)
    {
      tag->m_strTitle          = epgnow->Title();
      tag->m_strOriginalTitle  = epgnow->Title();
      tag->m_strPlotOutline    = epgnow->PlotOutline();
      tag->m_strPlot           = epgnow->Plot();
      tag->m_strGenre          = epgnow->Genre();
      tag->SetStartTime(epgnow->Start());
      tag->SetEndTime(epgnow->End());
      tag->SetDuration(epgnow->Duration());
      if (epgnext)
        tag->SetNextTitle(epgnext->Title());
      else
        tag->SetNextTitle("");

      if (tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
        tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;

      CDateTimeSpan span = tag->StartTime() - tag->EndTime();

      StringUtils::SecondsToTimeString(span.GetSeconds()
                                       + span.GetMinutes() * 60.
                                       + span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);
    }
    else
    {
      tag->m_strTitle          = g_localizeStrings.Get(18074);
      tag->m_strOriginalTitle  = g_localizeStrings.Get(18074);
      tag->m_strPlotOutline    = "";
      tag->m_strPlot           = "";
      tag->m_strGenre          = "";
      tag->SetStartTime(CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0));
      tag->SetEndTime(CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0));
      tag->SetDuration(CDateTimeSpan(0, 1, 0, 0));
      tag->SetNextTitle("");
    }

    if (tag->IsRadio())
    {
      CMusicInfoTag* musictag = item.GetMusicInfoTag();

      musictag->SetURL(tag->Path());
      musictag->SetTitle(tag->m_strTitle);
      musictag->SetArtist(tag->Name());
    //    musictag->SetAlbum(tag->m_strBouquet);
      musictag->SetAlbumArtist(tag->Name());
      musictag->SetGenre(tag->m_strGenre);
      musictag->SetDuration(tag->GetDuration());
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }

    tag->m_strAlbum = tag->Name();
    tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
    tag->m_iEpisode = 0;
    tag->m_strShowTitle.Format("%i", tag->Number());

    item.m_strTitle = tag->Name();
    item.m_dateTime = tag->StartTime();
    item.m_strPath  = tag->Path();
  }
}

int CPVRManager::GetPreviousChannel()
{
  if (m_currentPlayingChannel == NULL)
    return -1;

  int LastChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->Number();

  if (m_PreviousChannel[m_PreviousChannelIndex ^ 1] == LastChannel || LastChannel != m_PreviousChannel[0] && LastChannel != m_PreviousChannel[1])
    m_PreviousChannelIndex ^= 1;

  return m_PreviousChannel[m_PreviousChannelIndex ^= 1];
}



/************************************************************/
/** PVR Client internal demuxer access, is used if inside  **/
/** PVR_SERVERPROPS the HandleDemuxing is true             **/

bool CPVRManager::OpenDemux(PVRDEMUXHANDLE handle)
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->OpenTVDemux(handle, *m_currentPlayingChannel->GetTVChannelInfoTag());
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->OpenRecordingDemux(handle, *m_currentPlayingRecording->GetTVRecordingInfoTag());
}

void CPVRManager::DisposeDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->DisposeDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->DisposeDemux();
}

void CPVRManager::ResetDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->ResetDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->ResetDemux();
}

void CPVRManager::FlushDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->FlushDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->FlushDemux();
}

void CPVRManager::AbortDemux()
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->AbortDemux();
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->AbortDemux();
}

void CPVRManager::SetDemuxSpeed(int iSpeed)
{
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->SetDemuxSpeed(iSpeed);
  else if (m_currentPlayingRecording)
    m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->SetDemuxSpeed(iSpeed);
}

demux_packet_t* CPVRManager::ReadDemux()
{
  demux_packet_t *ret = NULL;

  if (m_currentPlayingChannel)
    ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->ReadDemux();
  else if (m_currentPlayingRecording)
    ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->ReadDemux();
  
  return ret;
}

bool CPVRManager::SeekDemuxTime(int time, bool backwords, double* startpts)
{
  bool ret = false;

  if (m_currentPlayingChannel)
    ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->SeekDemuxTime(time, backwords, startpts);
  else if (m_currentPlayingRecording)
    ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->SeekDemuxTime(time, backwords, startpts);
  
  return ret;
}

int CPVRManager::GetDemuxStreamLength()
{
  int ret = 0;

  if (m_currentPlayingChannel)
    ret = m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->ClientID()]->GetDemuxStreamLength();
  else if (m_currentPlayingRecording)
    ret = m_clients[m_currentPlayingRecording->GetTVRecordingInfoTag()->ClientID()]->GetDemuxStreamLength();
  
  return ret;
}







