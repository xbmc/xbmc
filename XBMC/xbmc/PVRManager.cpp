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
#include "GUIDialogYesNo.h"


using namespace std;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace ADDON;

std::map< long, IPVRClient* > CPVRManager::m_clients;
CPVRManager* CPVRManager::m_instance    = NULL;
CFileItem* CPVRManager::m_currentPlayingChannel = NULL;
bool CPVRManager::m_hasRecordings       = false;
bool CPVRManager::m_isRecording         = false;
bool CPVRManager::m_hasTimers           = false;


/************************************************************/
/** Class handling */

CPVRManager::CPVRManager()
{
  /* Initialize Member variables */
  m_HiddenChannels        = 0;
  m_CurrentGroupID        = -1;
  m_synchronized     	    = false;
  m_client                = NULL;
  m_currentPlayingChannel = NULL;

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

  /* Reset System Info swap counters */
  m_infoToggleStart = NULL;
  m_infoToggleCurrent = 0;

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
  if (m_clientProps.SupportTimers)
    PVRTimers.Load();

  /* Get Recordings from Backend */
  if (m_clientProps.SupportRecordings)
    PVRRecordings.Load();
    
  m_database.Open();

  /* Get Channelgroups */
  m_database.GetGroupList(m_currentClientID, &m_channel_group);

  SyncInfo();
  m_database.Close();

  Create();
  SetName("PVRManager Updater");
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

  m_HiddenChannels        = 0;
  m_CurrentGroupID        = -1;
  m_synchronized          = false;
  m_infoToggleStart       = NULL;
  m_infoToggleCurrent     = 0;

  return;
}

bool CPVRManager::LoadClients()
{
  VECADDONS *addons;
  // call update
  addons = g_addonmanager.GetAddonsFromType(ADDON_PVRDLL);

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
    IPVRClient *client;
    client = factory.LoadPVRClient(clientAddon, clientID, this);
    if (client)
    {
      m_clients.insert(std::make_pair(client->GetID(), client));
    }
  }
  
  m_database.Close();

  // Request each client's basic properties
  GetClientProperties();

  /// TEMPORARY CODE UNTIL WE SUPPORT MULTIPLE BACKENDS AT SAME TIME!!!
  {
    CLIENTMAPITR itr = m_clients.begin();
    m_client = m_clients[(*itr).first];
    m_client->GetProperties(&m_clientProps);
    m_currentClientID = m_client->GetID();
  }

  return !m_clients.empty();
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

void CPVRManager::Process()
{
  //GetChannels();
  m_synchronized = true;

  /* create EPG data structures */
  if (m_clientProps.SupportEPG)
  {
    time_t start;
    time_t end;
    CDateTime::GetCurrentDateTime().GetAsTime(start);
    CDateTime::GetCurrentDateTime().GetAsTime(end);
    start -= g_guiSettings.GetInt("pvrmenu.lingertime")*60;
    end   += g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      EnterCriticalSection(&m_critSection);
      m_client->GetEPGForChannel(PVRChannelsTV[i].m_iClientNum, PVRChannelsTV[i].m_EPG, start, end);
      LeaveCriticalSection(&m_critSection);
    }

    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      EnterCriticalSection(&m_critSection);
      m_client->GetEPGForChannel(PVRChannelsRadio[i].m_iClientNum, PVRChannelsRadio[i].m_EPG, start, end);
      LeaveCriticalSection(&m_critSection);
    }
  }

  CDateTime lastTVUpdate    = NULL;//CDateTime::GetCurrentDateTime();
  CDateTime lastRadioUpdate = CDateTime::GetCurrentDateTime();
  CDateTime lastScan        = CDateTime::GetCurrentDateTime();

  while (!m_bStop)
  {
    if (m_clientProps.SupportEPG)
    {
      if (lastTVUpdate+CDateTimeSpan(0, g_guiSettings.GetInt("pvrepg.epgupdate") / 60, g_guiSettings.GetInt("pvrepg.epgupdate") % 60, 0) < CDateTime::GetCurrentDateTime())
      {
        lastTVUpdate = CDateTime::GetCurrentDateTime();

        if (PVRChannelsTV.size() > 0)
        {
          time_t end;
          CDateTime::GetCurrentDateTime().GetAsTime(end);
		  end += (time_t)g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

          for (unsigned int i = 0; i < PVRChannelsTV.size(); ++i)
          {
		    CTVEPGInfoTag epgentry;
			time_t lastEntry = NULL;

			PVRChannelsTV[i].GetEPGLastEntry(&epgentry);
			if (epgentry.m_endTime.IsValid())
			{
			  epgentry.m_endTime.GetAsTime(lastEntry);
              PVRChannelsTV[i].CleanupEPG();
			}
		    EnterCriticalSection(&m_critSection);
		    m_client->GetEPGForChannel(PVRChannelsTV[i].m_iClientNum, PVRChannelsTV[i].m_EPG, lastEntry, end);
		    LeaveCriticalSection(&m_critSection);
          }
        }
      }
      if (m_clientProps.SupportRadio)
      {
        if (lastRadioUpdate+CDateTimeSpan(0, g_guiSettings.GetInt("pvrepg.epgupdate") / 60, g_guiSettings.GetInt("pvrepg.epgupdate") % 60+5, 0) < CDateTime::GetCurrentDateTime())
        {
          lastRadioUpdate = CDateTime::GetCurrentDateTime();

          if (PVRChannelsRadio.size() > 0)
          {
            time_t end;
            CDateTime::GetCurrentDateTime().GetAsTime(end);
		    end += (time_t)g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

            for (unsigned int i = 0; i < PVRChannelsRadio.size(); ++i)
            {
		      CTVEPGInfoTag epgentry;
			  time_t lastEntry = NULL;

			  PVRChannelsRadio[i].GetEPGLastEntry(&epgentry);
			  if (epgentry.m_endTime.IsValid())
			  {
			    epgentry.m_endTime.GetAsTime(lastEntry);
                PVRChannelsRadio[i].CleanupEPG();
			  }
		      EnterCriticalSection(&m_critSection);
		      m_client->GetEPGForChannel(PVRChannelsRadio[i].m_iClientNum, PVRChannelsRadio[i].m_EPG, lastEntry, end);
		      LeaveCriticalSection(&m_critSection);
            }
		  }
		}
	  }
	}
    /* Wait 30 seconds until start next change check */
    Sleep(30000);
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

    CStdString backendClients;
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
                         , next->m_StartTime.GetAsLocalizedDate(true)
                         , g_localizeStrings.Get(18191)
                         , next->m_StartTime.GetAsLocalizedTime("HH:mm", false));
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
	    ReceiveAllTimers();
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
    	  pTVWin->UpdateData(TV_WINDOW_TIMERS);
	  }
      break;

    case PVR_EVENT_RECORDINGS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld recording list changed", __FUNCTION__, clientID);
	    ReceiveAllRecordings();
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)m_gWindowManager.GetWindow(WINDOW_TV);
	    if (pTVWin)
    	  pTVWin->UpdateData(TV_WINDOW_RECORDINGS);
	  }
      break;

    case PVR_EVENT_CHANNELS_CHANGE:
	  {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld channel list changed", __FUNCTION__, clientID);
	    GetChannels();
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
        m_clients[(*itr).first]->ReInit();
        if (datachanged)
        {

        }
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


/************************************************************/
/** Feature flags */

bool CPVRManager::SupportEPG()
{
  return m_clientProps.SupportEPG;
}

bool CPVRManager::SupportRecording()
{
  return m_clientProps.SupportRecordings;
}

bool CPVRManager::SupportRadio()
{
  return m_clientProps.SupportRadio;
}

bool CPVRManager::SupportTimers()
{
  return m_clientProps.SupportTimers;
}

bool CPVRManager::SupportChannelSettings()
{
  return m_clientProps.SupportChannelSettings;
}

bool CPVRManager::SupportTeletext()
{
  if (m_clientProps.SupportTeletext)
  {
    return m_currentPlayingChannel->GetTVChannelInfoTag()->m_bTeletext;
  }
  return false;
}

bool CPVRManager::SupportDirector()
{
  return m_clientProps.SupportDirector;
}

bool CPVRManager::IsPlayingTV()
{ 
  if (!m_currentPlayingChannel)
    return false;

  return !m_currentPlayingChannel->GetTVChannelInfoTag()->m_radio;
}

bool CPVRManager::IsPlayingRadio()
{ 
  if (!m_currentPlayingChannel)
    return false;

  return m_currentPlayingChannel->GetTVChannelInfoTag()->m_radio;
}


/************************************************************/
/** EPG handling */

bool CPVRManager::GetEPGInfo(unsigned int number, CFileItem& now, CFileItem& next, bool radio)
{
  if (m_client)
  {
    EnterCriticalSection(&m_critSection);
    bool ok = false;

    if (!radio)
    {
      if (now.IsTVEPG())
        ok = PVRChannelsTV[number-1].GetEPGNowInfo(now.GetTVEPGInfoTag());

      if (next.IsTVEPG() && ok)
        ok = PVRChannelsTV[number-1].GetEPGNowInfo(next.GetTVEPGInfoTag());
    }
    else
    {
      if (now.IsTVEPG())
        ok = PVRChannelsRadio[number-1].GetEPGNowInfo(now.GetTVEPGInfoTag());

      if (next.IsTVEPG() && ok)
        ok = PVRChannelsRadio[number-1].GetEPGNowInfo(next.GetTVEPGInfoTag());
    }

    LeaveCriticalSection(&m_critSection);

    if (ok)
      return true;
  }
  /* print info dialog "Server error!" */
  CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
  return false;
}

int CPVRManager::GetEPGAll(CFileItemList* results, bool radio)
{
  EnterCriticalSection(&m_critSection);

  int cnt = 0;

  if (!radio)
  {
    for (unsigned int channel = 0; channel < PVRChannelsTV.size(); channel++)
    {
      if (PVRChannelsTV[channel].m_hide)
        continue;

      for (unsigned int i = 0; i < PVRChannelsTV[channel].m_EPG.size(); i++)
      {
        CTVEPGInfoTag epgentry(NULL);

        epgentry.m_strChannel        = PVRChannelsTV[channel].m_strChannel;
        epgentry.m_strTitle          = PVRChannelsTV[channel].m_EPG[i].m_strTitle;
        epgentry.m_strPlotOutline    = PVRChannelsTV[channel].m_EPG[i].m_strPlotOutline;
        epgentry.m_strPlot           = PVRChannelsTV[channel].m_EPG[i].m_strPlot;
        epgentry.m_GenreType         = PVRChannelsTV[channel].m_EPG[i].m_GenreType;
        epgentry.m_GenreSubType      = PVRChannelsTV[channel].m_EPG[i].m_GenreSubType;
        epgentry.m_strGenre          = PVRChannelsTV[channel].m_EPG[i].m_strGenre;
        epgentry.m_startTime         = PVRChannelsTV[channel].m_EPG[i].m_startTime;
        epgentry.m_endTime           = PVRChannelsTV[channel].m_EPG[i].m_endTime;
        epgentry.m_duration          = PVRChannelsTV[channel].m_EPG[i].m_duration;
        epgentry.m_channelNum        = PVRChannelsTV[channel].m_iChannelNum;
        epgentry.m_idChannel         = PVRChannelsTV[channel].m_iIdChannel;
        epgentry.m_isRadio           = PVRChannelsTV[channel].m_radio;
        epgentry.m_IconPath          = PVRChannelsTV[channel].m_IconPath;

        CFileItemPtr channel(new CFileItem(epgentry));
        results->Add(channel);
        cnt++;
      }
    }
  }
  else
  {
    for (unsigned int channel = 0; channel < PVRChannelsRadio.size(); channel++)
    {
      if (PVRChannelsRadio[channel].m_hide)
        continue;

      for (unsigned int i = 0; i < PVRChannelsRadio[channel].m_EPG.size(); i++)
      {
        CTVEPGInfoTag epgentry(NULL);

        epgentry.m_strChannel        = PVRChannelsRadio[channel].m_strChannel;
        epgentry.m_strTitle          = PVRChannelsRadio[channel].m_EPG[i].m_strTitle;
        epgentry.m_strPlotOutline    = PVRChannelsRadio[channel].m_EPG[i].m_strPlotOutline;
        epgentry.m_strPlot           = PVRChannelsRadio[channel].m_EPG[i].m_strPlot;
        epgentry.m_GenreType         = PVRChannelsRadio[channel].m_EPG[i].m_GenreType;
        epgentry.m_GenreSubType      = PVRChannelsRadio[channel].m_EPG[i].m_GenreSubType;
        epgentry.m_strGenre          = PVRChannelsRadio[channel].m_EPG[i].m_strGenre;
        epgentry.m_startTime         = PVRChannelsRadio[channel].m_EPG[i].m_startTime;
        epgentry.m_endTime           = PVRChannelsRadio[channel].m_EPG[i].m_endTime;
        epgentry.m_duration          = PVRChannelsRadio[channel].m_EPG[i].m_duration;
        epgentry.m_channelNum        = PVRChannelsRadio[channel].m_iChannelNum;
        epgentry.m_idChannel         = PVRChannelsRadio[channel].m_iIdChannel;

        CFileItemPtr channel(new CFileItem(epgentry));
        results->Add(channel);
        cnt++;
      }
    }
  }

  LeaveCriticalSection(&m_critSection);

  return cnt;
}

int CPVRManager::GetEPGNow(CFileItemList* results, bool radio)
{
  EnterCriticalSection(&m_critSection);

  int cnt = 0;

  if (!radio)
  {
    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      CTVEPGInfoTag epgnow(NULL);

      if (PVRChannelsTV[i].m_hide)
        continue;

      PVRChannelsTV[i].GetEPGNowInfo(&epgnow);

      CFileItemPtr channel(new CFileItem(epgnow));
      channel->SetLabel2(epgnow.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = PVRChannelsTV[i].m_strChannel;
      channel->SetThumbnailImage(PVRChannelsTV[i].m_IconPath);
      results->Add(channel);
      cnt++;
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      CTVEPGInfoTag epgnow(NULL);

      if (PVRChannelsRadio[i].m_hide)
        continue;

      PVRChannelsRadio[i].GetEPGNowInfo(&epgnow);

      CFileItemPtr channel(new CFileItem(epgnow));
      channel->SetLabel2(epgnow.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = PVRChannelsRadio[i].m_strChannel;
      channel->SetThumbnailImage(PVRChannelsRadio[i].m_IconPath);
      results->Add(channel);
      cnt++;
    }
  }

  LeaveCriticalSection(&m_critSection);

  return cnt;
}

int CPVRManager::GetEPGNext(CFileItemList* results, bool radio)
{
  EnterCriticalSection(&m_critSection);

  int cnt = 0;

  if (!radio)
  {
    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      CTVEPGInfoTag epgnext(NULL);

      if (PVRChannelsTV[i].m_hide)
        continue;

      PVRChannelsTV[i].GetEPGNextInfo(&epgnext);

      CFileItemPtr channel(new CFileItem(epgnext));
      channel->SetLabel2(epgnext.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = PVRChannelsTV[i].m_strChannel;
      channel->SetThumbnailImage(PVRChannelsTV[i].m_IconPath);
      results->Add(channel);
      cnt++;
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      CTVEPGInfoTag epgnext(NULL);

      if (PVRChannelsRadio[i].m_hide)
        continue;

      PVRChannelsRadio[i].GetEPGNextInfo(&epgnext);

      CFileItemPtr channel(new CFileItem(epgnext));
      channel->SetLabel2(epgnext.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = PVRChannelsRadio[i].m_strChannel;
      channel->SetThumbnailImage(PVRChannelsRadio[i].m_IconPath);
      results->Add(channel);
      cnt++;
    }
  }

  LeaveCriticalSection(&m_critSection);

  return cnt;
}

int CPVRManager::GetEPGChannel(unsigned int number, CFileItemList* results, bool radio)
{
  EnterCriticalSection(&m_critSection);

  int cnt = 0;

  if (!radio)
  {
    for (unsigned int i = 0; i < PVRChannelsTV[number-1].m_EPG.size(); i++)
    {
      CTVEPGInfoTag epgentry(NULL);

      epgentry.m_strChannel        = PVRChannelsTV[number-1].m_strChannel;
      epgentry.m_strTitle          = PVRChannelsTV[number-1].m_EPG[i].m_strTitle;
      epgentry.m_strPlotOutline    = PVRChannelsTV[number-1].m_EPG[i].m_strPlotOutline;
      epgentry.m_strPlot           = PVRChannelsTV[number-1].m_EPG[i].m_strPlot;
      epgentry.m_GenreType         = PVRChannelsTV[number-1].m_EPG[i].m_GenreType;
      epgentry.m_GenreSubType      = PVRChannelsTV[number-1].m_EPG[i].m_GenreSubType;
      epgentry.m_strGenre          = PVRChannelsTV[number-1].m_EPG[i].m_strGenre;
      epgentry.m_startTime         = PVRChannelsTV[number-1].m_EPG[i].m_startTime;
      epgentry.m_endTime           = PVRChannelsTV[number-1].m_EPG[i].m_endTime;
      epgentry.m_duration          = PVRChannelsTV[number-1].m_EPG[i].m_duration;
      epgentry.m_channelNum        = PVRChannelsTV[number-1].m_iChannelNum;
      epgentry.m_idChannel         = PVRChannelsTV[number-1].m_iIdChannel;
      epgentry.m_isRadio           = PVRChannelsTV[number-1].m_radio;

      CFileItemPtr channel(new CFileItem(epgentry));
      channel->SetLabel2(epgentry.m_startTime.GetAsLocalizedDateTime(false, false));
      results->Add(channel);
      cnt++;
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio[number-1].m_EPG.size(); i++)
    {
      CTVEPGInfoTag epgentry(NULL);

      epgentry.m_strChannel        = PVRChannelsRadio[number-1].m_strChannel;
      epgentry.m_strTitle          = PVRChannelsRadio[number-1].m_EPG[i].m_strTitle;
      epgentry.m_strPlotOutline    = PVRChannelsRadio[number-1].m_EPG[i].m_strPlotOutline;
      epgentry.m_strPlot           = PVRChannelsRadio[number-1].m_EPG[i].m_strPlot;
      epgentry.m_GenreType         = PVRChannelsRadio[number-1].m_EPG[i].m_GenreType;
      epgentry.m_GenreSubType      = PVRChannelsRadio[number-1].m_EPG[i].m_GenreSubType;
      epgentry.m_strGenre          = PVRChannelsRadio[number-1].m_EPG[i].m_strGenre;
      epgentry.m_startTime         = PVRChannelsRadio[number-1].m_EPG[i].m_startTime;
      epgentry.m_endTime           = PVRChannelsRadio[number-1].m_EPG[i].m_endTime;
      epgentry.m_duration          = PVRChannelsRadio[number-1].m_EPG[i].m_duration;
      epgentry.m_channelNum        = PVRChannelsRadio[number-1].m_iChannelNum;
      epgentry.m_idChannel         = PVRChannelsRadio[number-1].m_iIdChannel;

      CFileItemPtr channel(new CFileItem(epgentry));
      channel->SetLabel2(epgentry.m_startTime.GetAsLocalizedDateTime(false, false));
      results->Add(channel);
      cnt++;
    }
  }

  LeaveCriticalSection(&m_critSection);

  return cnt;
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

  m_database.AddGroup(m_currentClientID, newname);
  m_database.GetGroupList(m_currentClientID, &m_channel_group);

  m_database.Close();
  LeaveCriticalSection(&m_critSection);
}

bool CPVRManager::RenameGroup(unsigned int GroupId, const CStdString &newname)
{
  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_database.RenameGroup(m_currentClientID, GroupId, newname);
  m_database.GetGroupList(m_currentClientID, &m_channel_group);

  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::DeleteGroup(unsigned int GroupId)
{
  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_database.DeleteGroup(m_currentClientID, GroupId);

  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    if (PVRChannelsTV[i].m_iGroupID == GroupId)
    {
      PVRChannelsTV[i].m_iGroupID = 0;
      m_database.UpdateChannel(m_currentClientID, PVRChannelsTV[i]);
    }
  }
  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    if (PVRChannelsRadio[i].m_iGroupID == GroupId)
    {
      PVRChannelsRadio[i].m_iGroupID = 0;
      m_database.UpdateChannel(m_currentClientID, PVRChannelsRadio[i]);
    }
  }
  m_database.GetGroupList(m_currentClientID, &m_channel_group);
  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::ChannelToGroup(unsigned int number, unsigned int GroupId, bool radio)
{
  if (m_client)
  {
    if (!radio)
    {
      if (((int) number <= PVRChannelsTV.size()+1) && (number != 0))
      {
        EnterCriticalSection(&m_critSection);
        m_database.Open();
        PVRChannelsTV[number-1].m_iGroupID = GroupId;
        m_database.UpdateChannel(m_currentClientID, PVRChannelsTV[number-1]);
        m_database.Close();
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    else
    {
      if (((int) number <= PVRChannelsRadio.size()+1) && (number != 0))
      {
        EnterCriticalSection(&m_critSection);
        m_database.Open();
        PVRChannelsRadio[number-1].m_iGroupID = GroupId;
        m_database.UpdateChannel(m_currentClientID, PVRChannelsRadio[number-1]);
        m_database.Close();
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
  }
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
      if (PVRChannelsTV[i].m_iGroupID == GroupId)
        return i+1;
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      if (PVRChannelsRadio[i].m_iGroupID == GroupId)
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
/** Record handling **/

int CPVRManager::GetAllRecordings(CFileItemList* results)
{
  EnterCriticalSection(&m_critSection);
  ReceiveAllRecordings();

  for (unsigned int i = 0; i < PVRRecordings.size(); ++i)
  {
    if ((PVRRecordings[i].m_startTime < CDateTime::GetCurrentDateTime()) &&
        (PVRRecordings[i].m_endTime > CDateTime::GetCurrentDateTime()))
    {
      for (unsigned int j = 0; j < PVRTimers.size(); ++j)
      {
        if ((PVRTimers[j].m_channelNum == PVRRecordings[i].m_channelNum)  &&
            (PVRTimers[j].m_StartTime  <= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[j].m_StopTime   >= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[j].m_Repeat != true) && (PVRTimers[j].m_Active == true))
        {
          PVRRecordings[i].m_Summary.Format("%s", g_localizeStrings.Get(18069));
        }
      }
    }

    CFileItemPtr record(new CFileItem(PVRRecordings[i]));
    results->Add(record);
  }

  LeaveCriticalSection(&m_critSection);

  return PVRRecordings.size();
}

bool CPVRManager::DeleteRecording(const CFileItem &item)
{
  /* Check if a cPVRRecordingInfoTag is inside file item */
  if (!item.IsTVRecording())
  {
    CLog::Log(LOGERROR, "CPVRManager: DeleteRecording no RecordingInfoTag given!");
    return false;
  }

  const cPVRRecordingInfoTag* tag = item.GetTVRecordingInfoTag();

  try
  {
    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->m_clientID]->DeleteRecording(*tag);
  
    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    /* Update Recordings List */
    ReceiveAllRecordings();
    return true;
  }
  catch (PVR_ERROR err)
  {
    if (err == PVR_ERROR_SERVER_ERROR)
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0); /* print info dialog "Server error!" */
    else if (err == PVR_ERROR_NOT_SYNC)
      CGUIDialogOK::ShowAndGetInput(18100,18810,18803,0); /* print info dialog "Recordings not in sync!" */
    else if (err == PVR_ERROR_NOT_DELETED)
      CGUIDialogOK::ShowAndGetInput(18100,18811,18803,0); /* print info dialog "Couldn't delete recording!" */
    else
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0); /* print info dialog "Unknown error!" */
  }

  return false;
}

bool CPVRManager::RenameRecording(const CFileItem &item, CStdString &newname)
{
  /* Check if a cPVRRecordingInfoTag is inside file item */
  if (!item.IsTVRecording())
  {
    CLog::Log(LOGERROR, "CPVRManager: RenameRecording no RecordingInfoTag given!");
    return false;
  }

  const cPVRRecordingInfoTag* tag = item.GetTVRecordingInfoTag();

  try
  {
    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->m_clientID]->RenameRecording(*tag, newname);
  
    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    /* Update Recordings List */
    ReceiveAllRecordings();
    return true;
  }
  catch (PVR_ERROR err)
  {
    if (err == PVR_ERROR_SERVER_ERROR)
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0); /* print info dialog "Server error!" */
    else if (err == PVR_ERROR_NOT_SYNC)
      CGUIDialogOK::ShowAndGetInput(18100,18810,18803,0); /* print info dialog "Recordings not in sync!" */
    else if (err == PVR_ERROR_NOT_SAVED)
      CGUIDialogOK::ShowAndGetInput(18100,18811,18803,0); /* print info dialog "Couldn't delete recording!" */
    else
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0); /* print info dialog "Unknown error!" */
  }
  return false;
}

void CPVRManager::ReceiveAllRecordings()
{
  EnterCriticalSection(&m_critSection);
  
  /* Clear all current present Recordings inside list */
  PVRRecordings.erase(PVRRecordings.begin(), PVRRecordings.end());

  /* Go thru all clients and receive there Recordings */
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    /* Load only if the client have Recordings */
    if (m_clients[(*itr).first]->GetNumRecordings() > 0)
    {
      m_clients[(*itr).first]->GetAllRecordings(&PVRRecordings);
    }
    itr++;
  }

  LeaveCriticalSection(&m_critSection);
  return;
}


/************************************************************/
/** Timer handling **/

int CPVRManager::GetAllTimers(CFileItemList* results)
{
  EnterCriticalSection(&m_critSection);

  ReceiveAllTimers();

  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
  {
    CFileItemPtr timer(new CFileItem(PVRTimers[i]));
    results->Add(timer);
  }
  
  /* Syncronize Timer Info labels */
  SyncInfo();

  LeaveCriticalSection(&m_critSection);

  return PVRTimers.size();
}

bool CPVRManager::AddTimer(const CFileItem &item)
{
  /* Check if a cPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CPVRManager: AddTimer no TVInfoTag given!");
    return false;
  }

  const cPVRTimerInfoTag* tag = item.GetTVTimerInfoTag();

  try
  {
    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->ClientID()]->AddTimer(*tag);
  
    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    /* Update Timers List */
    ReceiveAllTimers();
    return true;
  }
  catch (PVR_ERROR err)
  {
    if (err == PVR_ERROR_SERVER_ERROR)
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0); /* print info dialog "Server error!" */
    else if (err == PVR_ERROR_NOT_SYNC)
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0); /* print info dialog "Timers not in sync!" */
    else if (err == PVR_ERROR_NOT_SAVED)
      CGUIDialogOK::ShowAndGetInput(18100,18806,18803,0); /* print info dialog "Couldn't delete timer!" */
    else if (err == PVR_ERROR_ALREADY_PRESENT)
      CGUIDialogOK::ShowAndGetInput(18100,18806,0,18814); /* print info dialog */
    else
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0); /* print info dialog "Unknown error!" */
  }
  return false;
}

bool CPVRManager::DeleteTimer(const CFileItem &item, bool force)
{
  /* Check if a cPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CPVRManager: DeleteTimer no TVInfoTag given!");
    return false;
  }

  const cPVRTimerInfoTag* tag = item.GetTVTimerInfoTag();

  try
  {
    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->m_clientID]->DeleteTimer(*tag, force);

    if (err == PVR_ERROR_RECORDING_RUNNING)
    {
      if (CGUIDialogYesNo::ShowAndGetInput(122,0,18162,0))
        err = m_clients[tag->m_clientID]->DeleteTimer(*tag, true);
    }

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    /* Update Timers List */
    ReceiveAllTimers();
    return true;
  }
  catch (PVR_ERROR err)
  {
    if (err == PVR_ERROR_SERVER_ERROR)
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0); // print info dialog "Server error!"
    else if (err == PVR_ERROR_NOT_SYNC)
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0); // print info dialog "Timers not in sync!"
    else if (err == PVR_ERROR_NOT_DELETED)
      CGUIDialogOK::ShowAndGetInput(18100,18802,18803,0); // print info dialog "Couldn't delete timer!"
    else
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0); // print info dialog "Unknown error!"
  }
  return false;
}

bool CPVRManager::RenameTimer(const CFileItem &item, CStdString &newname)
{
  /* Check if a cPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CPVRManager: RenameTimer no TVInfoTag given!");
    return false;
  }

  const cPVRTimerInfoTag* tag = item.GetTVTimerInfoTag();

  try
  {
    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->m_clientID]->RenameTimer(*tag, newname);

    if (err == PVR_ERROR_NOT_IMPLEMENTED)
      err = m_clients[tag->m_clientID]->UpdateTimer(*tag);

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    /* Update Timers List */
    ReceiveAllTimers();
    return true;
  }
  catch (PVR_ERROR err)
  {
    if (err == PVR_ERROR_SERVER_ERROR)
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
    else if (err == PVR_ERROR_NOT_SYNC)
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
    else if (err == PVR_ERROR_NOT_SAVED)
      CGUIDialogOK::ShowAndGetInput(18100,18806,18803,0);
    else
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
  }

  return false;
}

bool CPVRManager::UpdateTimer(const CFileItem &item)
{
  /* Check if a cPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CPVRManager: UpdateTimer no TVInfoTag given!");
    return false;
  }
  
  const cPVRTimerInfoTag* tag = item.GetTVTimerInfoTag();

  try
  {
    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->m_clientID]->UpdateTimer(*tag);
    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    /* Update Timers List */
    ReceiveAllTimers();
    return true;
  }
  catch (PVR_ERROR err)
  {
    if (err == PVR_ERROR_SERVER_ERROR)
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
    else if (err == PVR_ERROR_NOT_SYNC)
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
    else if (err == PVR_ERROR_NOT_SAVED)
      CGUIDialogOK::ShowAndGetInput(18100,18806,18803,0);
    else
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
  }
  return false;
}

void CPVRManager::ReceiveAllTimers()
{
  EnterCriticalSection(&m_critSection);
  
  /* Clear all current present Timers inside list */
  PVRTimers.Clear();

  /* Go thru all clients and receive there timers */
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    /* Load only if the client have timers */
    if (m_clients[(*itr).first]->GetNumTimers() > 0)
    {
      m_clients[(*itr).first]->GetAllTimers(&PVRTimers);
    }
    itr++;
  }
  LeaveCriticalSection(&m_critSection);

  return;
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
  bool ret = m_clients[tag->m_clientID]->TeletextPagePresent(tag->m_iClientNum, Page, subPage);
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
  bool ret = m_clients[tag->m_clientID]->ReadTeletextPage(buf, tag->m_iClientNum, Page, subPage);
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
  if (!m_clients[tag->m_clientID]->OpenLiveStream(tag->m_iClientNum))
  {
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
    LeaveCriticalSection(&m_critSection);
    return false;
  }
  
  SetCurrentPlayingProgram(*m_currentPlayingChannel);

  LeaveCriticalSection(&m_critSection);
  return true;
}

void CPVRManager::CloseLiveStream()
{
  if (!m_currentPlayingChannel)
    return;

  EnterCriticalSection(&m_critSection);

  m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->m_clientID]->CloseLiveStream();
  delete m_currentPlayingChannel;
  m_currentPlayingChannel = NULL;

  LeaveCriticalSection(&m_critSection);
  return;
}

void CPVRManager::PauseLiveStream(bool OnOff)
{

}

int CPVRManager::ReadLiveStream(BYTE* buf, int buf_size)
{
  if (!m_currentPlayingChannel)
    return 0;

  if (m_scanStart)
  {
    if (timeGetTime() - m_scanStart > g_guiSettings.GetInt("pvrplayback.scantime")*1000)
      return 0;
    else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
      m_scanStart = NULL;
  }
  return m_clients[m_currentPlayingChannel->GetTVChannelInfoTag()->m_clientID]->ReadLiveStream(buf, buf_size);
}

__int64 CPVRManager::SeekLiveStream(__int64 pos, int whence)
{
  return 0;
}

int CPVRManager::GetCurrentChannel(bool radio)
{
  if (!m_currentPlayingChannel)
    return 1;

  return m_currentPlayingChannel->GetTVChannelInfoTag()->m_iChannelNum;
}

CFileItem *CPVRManager::GetCurrentChannelItem()
{
  return m_currentPlayingChannel;
}

bool CPVRManager::ChannelSwitch(unsigned int iChannel)
{
  if (!m_currentPlayingChannel)
    return false;

  cPVRChannels *channels;
  if (!m_currentPlayingChannel->GetTVChannelInfoTag()->m_radio)
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  if (iChannel > channels->size()+1)
  {
    CGUIDialogOK::ShowAndGetInput(18100,18105,0,0);
    return false;
  }
  
  EnterCriticalSection(&m_critSection);

  if (!m_clients[channels->at(iChannel-1).m_clientID]->SwitchChannel(channels->at(iChannel-1).m_iClientNum))
  {
    CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
    LeaveCriticalSection(&m_critSection);
    return false;
  }

  delete m_currentPlayingChannel;
  m_currentPlayingChannel = new CFileItem(channels->at(iChannel-1));
  SetCurrentPlayingProgram(*m_currentPlayingChannel);

  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::ChannelUp(unsigned int *newchannel)
{
  if (m_currentPlayingChannel)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->m_radio)
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    int currentTVChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->m_iChannelNum;
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel += 1;

      if (currentTVChannel > channels->size())
        currentTVChannel = 1;

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != channels->at(currentTVChannel-1).m_iGroupID))
        continue;

      if (m_clients[channels->at(currentTVChannel-1).m_clientID]->SwitchChannel(channels->at(currentTVChannel-1).m_iClientNum))
      {
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(channels->at(currentTVChannel-1));
        SetCurrentPlayingProgram(*m_currentPlayingChannel);
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
    if (!m_currentPlayingChannel->GetTVChannelInfoTag()->m_radio)
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    int currentTVChannel = m_currentPlayingChannel->GetTVChannelInfoTag()->m_iChannelNum;
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel -= 1;

      if (currentTVChannel <= 0)
        currentTVChannel = channels->size();

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != channels->at(currentTVChannel-1).m_iGroupID))
        continue;
  
      if (m_clients[channels->at(currentTVChannel-1).m_clientID]->SwitchChannel(channels->at(currentTVChannel-1).m_iClientNum))
      {
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(channels->at(currentTVChannel-1));
        SetCurrentPlayingProgram(*m_currentPlayingChannel);
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
  time_t time_s = NULL;

  if (!m_currentPlayingChannel)
    return 0;

  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
  if (!tag)
    return 0;

  time_s =  tag->m_duration.GetDays()*60*60*24;
  time_s += tag->m_duration.GetHours()*60*60;
  time_s += tag->m_duration.GetMinutes()*60;
  time_s += tag->m_duration.GetSeconds();
  time_s *= 1000;

  return time_s;
}

int CPVRManager::GetStartTime()
{
  if (m_currentPlayingChannel)
  {
    cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetTVChannelInfoTag();
    if (!tag)
      return 0;
    
    CDateTime endtime = tag->m_endTime;

    if (endtime < CDateTime::GetCurrentDateTime())
    {
      SetCurrentPlayingProgram(*m_currentPlayingChannel);

      if (UpdateItem(*m_currentPlayingChannel))
      {
        g_application.CurrentFileItem() = *m_currentPlayingChannel;
        g_infoManager.SetCurrentItem(*m_currentPlayingChannel);
      }
    }

    time_t time_c, time_s;

    CDateTime::GetCurrentDateTime().GetAsTime(time_c);
    tag->m_startTime.GetAsTime(time_s);

    return (time_s - time_c) * 1000;
  }
  return 0;
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

  tag->m_strAlbum         = current->m_strChannel;
  tag->m_strTitle         = current->m_strTitle;
  tag->m_strOriginalTitle = current->m_strTitle;
  tag->m_strPlotOutline   = current->m_strPlotOutline;
  tag->m_strPlot          = current->m_strPlot;
  tag->m_strGenre         = current->m_strGenre;
  tag->m_strShowTitle.Format("%i", current->m_iChannelNum);
  tag->m_strNextTitle     = current->m_strNextTitle;
  tag->m_strPath          = current->m_strFileNameAndPath;
  tag->m_strFileNameAndPath = current->m_strFileNameAndPath;

  item.m_strTitle = current->m_strChannel;
  item.m_dateTime = current->m_startTime;
  item.m_strPath  = current->m_strFileNameAndPath;

  CDateTimeSpan span = current->m_startTime - current->m_endTime;
  StringUtils::SecondsToTimeString(span.GetSeconds() + span.GetMinutes() * 60 + span.GetHours() * 3600,
                                   tag->m_strRuntime,
                                   TIME_FORMAT_GUESS);

  if (current->m_IconPath != "")
  {
    item.SetThumbnailImage(current->m_IconPath);
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

  if (m_client)
  {
    if (m_client->OpenRecordedStream(PVRRecordings[record-1]))
    {
      LeaveCriticalSection(&m_critSection);
      return true;
    }
  }

  LeaveCriticalSection(&m_critSection);
  return false;
}

void CPVRManager::CloseRecordedStream(void)
{
  if (m_client)
  {
    EnterCriticalSection(&m_critSection);
    m_client->CloseRecordedStream();
    LeaveCriticalSection(&m_critSection);
  }
  return;
}

int CPVRManager::ReadRecordedStream(BYTE* buf, int buf_size)
{
  if (m_client)
  {
    EnterCriticalSection(&m_critSection);
    int ret = m_client->ReadRecordedStream(buf, buf_size);
    LeaveCriticalSection(&m_critSection);
    return ret;
  }

  return 0;
}

__int64 CPVRManager::SeekRecordedStream(__int64 pos, int whence)
{
  if (m_client)
  {
    return m_client->SeekRecordedStream(pos, whence);
  }

  return -1;
}

__int64 CPVRManager::LengthRecordedStream(void)
{
  if (m_client)
  {
    return m_client->LengthRecordedStream();
  }

  return -1;
}

bool CPVRManager::IsRecording(unsigned int channel, bool radio)
{
  if (m_client)
  {
    if (!radio)
    {
      return PVRChannelsTV[channel-1].m_isRecording;
    }
    else
    {
      return PVRChannelsRadio[channel-1].m_isRecording;
    }
  }

  return false;
}

bool CPVRManager::RecordChannel(unsigned int channel, bool bOnOff, bool radio)
{
  if (m_client)
  {
    if (!radio)
    {
      if (bOnOff && PVRChannelsTV[channel-1].m_isRecording == false)
      {
        cPVRTimerInfoTag newtimer(true);
        CFileItem *item = new CFileItem(newtimer);

        if (!AddTimer(*item))
        {
          CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
          return true;
        }

        PVRChannelsTV[channel-1].m_isRecording = true;
      }
      else if (PVRChannelsTV[channel-1].m_isRecording == true)
      {
        for (unsigned int i = 0; i < PVRTimers.size(); ++i)
        {
          if ((PVRTimers[i].m_channelNum == PVRChannelsTV[channel-1].m_iChannelNum) &&
              (PVRTimers[i].m_StartTime <= CDateTime::GetCurrentDateTime()) &&
              (PVRTimers[i].m_StopTime >= CDateTime::GetCurrentDateTime()) &&
              (PVRTimers[i].m_Repeat != true) && (PVRTimers[i].m_Active == true))
          {
            DeleteTimer(PVRTimers[i], true);
          }
        }

        PVRChannelsTV[channel-1].m_isRecording = false;
      }
    }
    else
    {
      if (bOnOff && PVRChannelsRadio[channel-1].m_isRecording == false)
      {
        cPVRTimerInfoTag newtimer(true);
        CFileItem *item = new CFileItem(newtimer);

        if (!AddTimer(*item))
        {
          CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
          return true;
        }

        PVRChannelsRadio[channel-1].m_isRecording = true;
      }
      else if (PVRChannelsRadio[channel-1].m_isRecording == true)
      {
        for (unsigned int i = 0; i < PVRTimers.size(); ++i)
        {
          if ((PVRTimers[i].m_channelNum == PVRChannelsRadio[channel-1].m_iChannelNum) &&
              (PVRTimers[i].m_StartTime <= CDateTime::GetCurrentDateTime()) &&
              (PVRTimers[i].m_StopTime >= CDateTime::GetCurrentDateTime()) &&
              (PVRTimers[i].m_Repeat != true) && (PVRTimers[i].m_Active == true))
          {
            DeleteTimer(PVRTimers[i], true);
          }
        }

        PVRChannelsRadio[channel-1].m_isRecording = false;
      }
    }
  }
  return false;
}


/************************************************************/
/** Internal handling **/

void CPVRManager::GetChannels()
{
  cPVRChannels PVRChannelsTV_tmp;
  cPVRChannels PVRChannelsRadio_tmp;

  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_client->GetChannelList(PVRChannelsTV_tmp, false);
  m_client->GetChannelList(PVRChannelsRadio_tmp, true);

  /*
   * First whe look for moved channels on backend (other backend number)
   * and delete no more present channels inside database.
   * Problem:
   * If a channel on client is renamed, it is deleted from Database
   * and later added as new channel and loose his Group Information
   */
  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    bool found = false;

    for (unsigned int j = 0; j < PVRChannelsTV_tmp.size(); j++)
    {
	  if (PVRChannelsTV[i].m_strChannel == PVRChannelsTV_tmp[j].m_strChannel)
	  {
	    if (PVRChannelsTV[i].m_iClientNum != PVRChannelsTV_tmp[j].m_iClientNum)
		{
		  PVRChannelsTV[i].m_iClientNum = PVRChannelsTV_tmp[j].m_iClientNum;
		  m_database.UpdateChannel(m_currentClientID, PVRChannelsTV[i]);
		  CLog::Log(LOGINFO,"PVRManager: Updated TV channel %s", PVRChannelsTV[i].m_strChannel.c_str());
		}

	    found = true;
		PVRChannelsTV_tmp.erase(PVRChannelsTV_tmp.begin()+j);
		break;
	  }
	}

	if (!found)
	{
	  CLog::Log(LOGINFO,"PVRManager: Removing TV channel %s (no more present)", PVRChannelsTV[i].m_strChannel.c_str());
      m_database.RemoveChannel(m_currentClientID, PVRChannelsTV[i]);
	  PVRChannelsTV.erase(PVRChannelsTV.begin()+i);
	  i--;
	}
  }

  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    bool found = false;

    for (unsigned int j = 0; j < PVRChannelsRadio_tmp.size(); j++)
    {
	  if (PVRChannelsRadio[i].m_strChannel == PVRChannelsRadio_tmp[j].m_strChannel)
	  {
	    if (PVRChannelsRadio[i].m_iClientNum != PVRChannelsRadio_tmp[j].m_iClientNum)
		{
		  PVRChannelsRadio[i].m_iClientNum = PVRChannelsRadio_tmp[j].m_iClientNum;
		  m_database.UpdateChannel(m_currentClientID, PVRChannelsRadio[i]);
		  CLog::Log(LOGINFO,"PVRManager: Updated Radio channel %s", PVRChannelsRadio[i].m_strChannel.c_str());
		}

	    found = true;
		PVRChannelsRadio_tmp.erase(PVRChannelsRadio_tmp.begin()+j);
		break;
	  }
	}

	if (!found)
	{
	  CLog::Log(LOGINFO,"PVRManager: Removing Radio channel %s (no more present)", PVRChannelsRadio[i].m_strChannel.c_str());
      m_database.RemoveChannel(m_currentClientID, PVRChannelsRadio[i]);
	  PVRChannelsRadio.erase(PVRChannelsRadio.begin()+i);
	  i--;
	}
  }

  /*
   * Now whe add new channels to frontend
   * All entries now present in the temp lists, are new entries
   */
  for (unsigned int i = 0; i < PVRChannelsTV_tmp.size(); i++)
  {
    PVRChannelsTV_tmp[i].m_strStatus = "livetv";
    PVRChannelsTV_tmp[i].m_iIdChannel = m_database.AddChannel(m_currentClientID, PVRChannelsTV_tmp[i]);
	PVRChannelsTV.push_back(PVRChannelsTV_tmp[i]);
	CLog::Log(LOGINFO,"PVRManager: Added TV channel %s", PVRChannelsTV_tmp[i].m_strChannel.c_str());
  }

  for (unsigned int i = 0; i < PVRChannelsRadio_tmp.size(); i++)
  {
    PVRChannelsRadio_tmp[i].m_strStatus = "livetv";
    PVRChannelsRadio_tmp[i].m_iIdChannel = m_database.AddChannel(m_currentClientID, PVRChannelsRadio_tmp[i]);
	PVRChannelsRadio.push_back(PVRChannelsRadio_tmp[i]);
	CLog::Log(LOGINFO,"PVRManager: Added Radio channel %s", PVRChannelsRadio_tmp[i].m_strChannel.c_str());
  }


  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return;
}

void CPVRManager::SyncInfo()
{
  m_client->GetNumRecordings() > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  m_client->GetNumTimers()     > 0 ? m_hasTimers     = true : m_hasTimers = false;
  m_isRecording = false;

  if (m_hasTimers && m_clientProps.SupportTimers)
  {
    cPVRTimerInfoTag *nextRec = PVRTimers.GetNextActiveTimer();

    m_nextRecordingTitle    = nextRec->m_strTitle;
    m_nextRecordingChannel  = PVRChannelsTV.GetNameForChannel(nextRec->m_channelNum);
    m_nextRecordingDateTime = nextRec->m_StartTime.GetAsLocalizedDateTime(false, false);

    if (nextRec->m_recStatus == true)
    {
      m_isRecording = true;
      CLog::Log(LOGDEBUG, "%s - PVR: next timer is currently recording", __FUNCTION__);
    }
    else
    {
      m_isRecording = false;
    }
  }

  if (m_isRecording && m_clientProps.SupportRecordings)
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
    if (tag->m_EPG.size() > 0)
    {
      CTVEPGInfoTag epgnow(NULL);
      CTVEPGInfoTag epgnext(NULL);

      if (tag->GetEPGNowInfo(&epgnow))
      {
        tag->m_strTitle          = epgnow.m_strTitle;
        tag->m_strOriginalTitle  = epgnow.m_strTitle;
        tag->m_strPlotOutline    = epgnow.m_strPlotOutline;
        tag->m_strPlot           = epgnow.m_strPlot;
        tag->m_strGenre          = epgnow.m_strGenre;
        tag->m_startTime         = epgnow.m_startTime;
        tag->m_endTime           = epgnow.m_endTime;
        tag->m_duration          = epgnow.m_duration;
        if (tag->GetEPGNextInfo(&epgnext))
          tag->m_strNextTitle    = epgnext.m_strTitle;
        else
          tag->m_strNextTitle    = "";

        if (tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
          tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;
        
        CDateTimeSpan span = tag->m_startTime - tag->m_endTime;

        StringUtils::SecondsToTimeString(span.GetSeconds()
                                         + span.GetMinutes() * 60.
                                         + span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);
      }
    }
    else
    {
      tag->m_strTitle          = g_localizeStrings.Get(18074);
      tag->m_strOriginalTitle  = g_localizeStrings.Get(18074);
      tag->m_strPlotOutline    = "";
      tag->m_strPlot           = "";
      tag->m_strGenre          = "";
      tag->m_startTime         = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0);
      tag->m_endTime           = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0);
      tag->m_duration          = CDateTimeSpan(0, 1, 0, 0);
      tag->m_strNextTitle      =  "";
    }

    if (tag->m_radio)
    {
      CMusicInfoTag* musictag = item.GetMusicInfoTag();

      musictag->SetURL(tag->m_strFileNameAndPath);
      musictag->SetTitle(tag->m_strTitle);
      musictag->SetArtist(tag->m_strChannel);
    //    musictag->SetAlbum(tag->m_strBouquet);
      musictag->SetAlbumArtist(tag->m_strChannel);
      musictag->SetGenre(tag->m_strGenre);
      musictag->SetDuration(tag->GetDuration());
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }
    
    tag->m_strAlbum = tag->m_strChannel;
    tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
    tag->m_iEpisode = 0;
    tag->m_strShowTitle.Format("%i", tag->m_iChannelNum);

    item.m_strTitle = tag->m_strChannel;
    item.m_dateTime = tag->m_startTime;
    item.m_strPath  = tag->m_strFileNameAndPath;
  }
}
