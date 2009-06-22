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
bool CPVRManager::m_isPlayingTV         = false;
bool CPVRManager::m_isPlayingRadio      = false;
bool CPVRManager::m_isPlayingRecording  = false;
bool CPVRManager::m_hasRecordings       = false;
bool CPVRManager::m_isRecording         = false;
bool CPVRManager::m_hasTimers           = false;


/************************************************************/
/** Class handling */

CPVRManager::CPVRManager()
{
  /* Initialize Member variables */
  m_CurrentTVChannel      = 1;
  m_CurrentRadioChannel   = 1;
  m_CurrentChannelID      = -1;
  m_HiddenChannels        = 0;
  m_CurrentGroupID        = -1;
  m_synchronized     	    = false;
  m_client                = NULL;

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
  CAddon::RegisterAddonCallback(ADDON_PVRDLL, this);
  if (!LoadClients())
  {
    CLog::Log(LOGERROR, "PVR: couldn't load clients");
    return;
  }

  /* Get channels if present from Database, otherwise load it from client
   * and add it to database
   */
  if (m_database.GetNumChannels(m_currentClientID) > 0)
  {
    m_database.GetChannelList(m_currentClientID, m_channels_tv, false);
    m_database.GetChannelList(m_currentClientID, m_channels_radio, true);
    m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);
  }
  else
  {
    CLog::Log(LOGNOTICE, "PVR: TV Database holds no channels, reading channels from client");

    m_client->GetChannelList(m_channels_tv, false);
    m_client->GetChannelList(m_channels_radio, true);

    /* Fill Channels to Database */
    for (unsigned int i = 0; i < m_channels_tv.size(); i++)
    {
      m_channels_tv[i].m_strStatus = "livetv";
      m_channels_tv[i].m_iIdChannel = m_database.AddChannel(m_currentClientID, m_channels_tv[i]);
    }

    for (unsigned int i = 0; i < m_channels_radio.size(); i++)
    {
      m_channels_radio[i].m_strStatus = "livetv";
      m_channels_radio[i].m_iIdChannel = m_database.AddChannel(m_currentClientID, m_channels_radio[i]);
    }

    m_database.Compress(true);
  }

  /* Get Channelgroups */
  m_database.GetGroupList(m_currentClientID, &m_channel_group);

  /* Get Recordings from Backend */
  if (m_clientProps.SupportRecordings)
    GetRecordings();

  /* Get Timers from Backend */
  if (m_clientProps.SupportTimers)
    ReceiveAllTimers();

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

  m_CurrentTVChannel      = 1;
  m_CurrentRadioChannel   = 1;
  m_CurrentChannelID      = -1;
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
  addons = g_settings.GetAddonsFromType(ADDON_PVRDLL);

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
  GetChannels();
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

    for (unsigned int i = 0; i < m_channels_tv.size(); i++)
    {
      EnterCriticalSection(&m_critSection);
      m_client->GetEPGForChannel(m_channels_tv[i].m_iClientNum, m_channels_tv[i].m_EPG, start, end);
      LeaveCriticalSection(&m_critSection);
    }

    for (unsigned int i = 0; i < m_channels_radio.size(); i++)
    {
      EnterCriticalSection(&m_critSection);
      m_client->GetEPGForChannel(m_channels_radio[i].m_iClientNum, m_channels_radio[i].m_EPG, start, end);
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

        if (m_channels_tv.size() > 0)
        {
          time_t end;
          CDateTime::GetCurrentDateTime().GetAsTime(end);
		  end += (time_t)g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

          for (unsigned int i = 0; i < m_channels_tv.size(); ++i)
          {
		    CTVEPGInfoTag epgentry;
			time_t lastEntry = NULL;

			m_channels_tv[i].GetEPGLastEntry(&epgentry);
			if (epgentry.m_endTime.IsValid())
			{
			  epgentry.m_endTime.GetAsTime(lastEntry);
              m_channels_tv[i].CleanupEPG();
			}
		    EnterCriticalSection(&m_critSection);
		    m_client->GetEPGForChannel(m_channels_tv[i].m_iClientNum, m_channels_tv[i].m_EPG, lastEntry, end);
		    LeaveCriticalSection(&m_critSection);
          }
        }
      }
      if (m_clientProps.SupportRadio)
      {
        if (lastRadioUpdate+CDateTimeSpan(0, g_guiSettings.GetInt("pvrepg.epgupdate") / 60, g_guiSettings.GetInt("pvrepg.epgupdate") % 60+5, 0) < CDateTime::GetCurrentDateTime())
        {
          lastRadioUpdate = CDateTime::GetCurrentDateTime();

          if (m_channels_radio.size() > 0)
          {
            time_t end;
            CDateTime::GetCurrentDateTime().GetAsTime(end);
		    end += (time_t)g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

            for (unsigned int i = 0; i < m_channels_radio.size(); ++i)
            {
		      CTVEPGInfoTag epgentry;
			  time_t lastEntry = NULL;

			  m_channels_radio[i].GetEPGLastEntry(&epgentry);
			  if (epgentry.m_endTime.IsValid())
			  {
			    epgentry.m_endTime.GetAsTime(lastEntry);
                m_channels_radio[i].CleanupEPG();
			  }
		      EnterCriticalSection(&m_critSection);
		      m_client->GetEPGForChannel(m_channels_radio[i].m_iClientNum, m_channels_radio[i].m_EPG, lastEntry, end);
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
    CDateTime next = NextTimerDate();
    m_nextTimer.Format("%s %s %s %s", g_localizeStrings.Get(18190)
                       , next.GetAsLocalizedDate(true)
                       , g_localizeStrings.Get(18191)
                       , next.GetAsLocalizedTime("HH:mm", false));
    return m_nextTimer;
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
	    GetRecordings();
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
    if (CPVRManager::GetInstance()->IsPlayingTV())
    {
      return m_channels_tv[m_CurrentTVChannel-1].m_bTeletext;
    }
    else if (CPVRManager::GetInstance()->IsPlayingRadio())
    {
      return m_channels_tv[m_CurrentRadioChannel-1].m_bTeletext;
    }
  }
  return false;
}

bool CPVRManager::SupportDirector()
{
  return m_clientProps.SupportDirector;
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
        ok = m_channels_tv[number-1].GetEPGNowInfo(now.GetTVEPGInfoTag());

      if (next.IsTVEPG() && ok)
        ok = m_channels_tv[number-1].GetEPGNowInfo(next.GetTVEPGInfoTag());
    }
    else
    {
      if (now.IsTVEPG())
        ok = m_channels_radio[number-1].GetEPGNowInfo(now.GetTVEPGInfoTag());

      if (next.IsTVEPG() && ok)
        ok = m_channels_radio[number-1].GetEPGNowInfo(next.GetTVEPGInfoTag());
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
    for (unsigned int channel = 0; channel < m_channels_tv.size(); channel++)
    {
      if (m_channels_tv[channel].m_hide)
        continue;

      for (unsigned int i = 0; i < m_channels_tv[channel].m_EPG.size(); i++)
      {
        CTVEPGInfoTag epgentry(NULL);

        epgentry.m_strChannel        = m_channels_tv[channel].m_strChannel;
        epgentry.m_strTitle          = m_channels_tv[channel].m_EPG[i].m_strTitle;
        epgentry.m_strPlotOutline    = m_channels_tv[channel].m_EPG[i].m_strPlotOutline;
        epgentry.m_strPlot           = m_channels_tv[channel].m_EPG[i].m_strPlot;
        epgentry.m_GenreType         = m_channels_tv[channel].m_EPG[i].m_GenreType;
        epgentry.m_GenreSubType      = m_channels_tv[channel].m_EPG[i].m_GenreSubType;
        epgentry.m_strGenre          = m_channels_tv[channel].m_EPG[i].m_strGenre;
        epgentry.m_startTime         = m_channels_tv[channel].m_EPG[i].m_startTime;
        epgentry.m_endTime           = m_channels_tv[channel].m_EPG[i].m_endTime;
        epgentry.m_duration          = m_channels_tv[channel].m_EPG[i].m_duration;
        epgentry.m_channelNum        = m_channels_tv[channel].m_iChannelNum;
        epgentry.m_idChannel         = m_channels_tv[channel].m_iIdChannel;
        epgentry.m_isRadio           = m_channels_tv[channel].m_radio;
        epgentry.m_IconPath          = m_channels_tv[channel].m_IconPath;

        CFileItemPtr channel(new CFileItem(epgentry));
        results->Add(channel);
        cnt++;
      }
    }
  }
  else
  {
    for (unsigned int channel = 0; channel < m_channels_radio.size(); channel++)
    {
      if (m_channels_radio[channel].m_hide)
        continue;

      for (unsigned int i = 0; i < m_channels_radio[channel].m_EPG.size(); i++)
      {
        CTVEPGInfoTag epgentry(NULL);

        epgentry.m_strChannel        = m_channels_radio[channel].m_strChannel;
        epgentry.m_strTitle          = m_channels_radio[channel].m_EPG[i].m_strTitle;
        epgentry.m_strPlotOutline    = m_channels_radio[channel].m_EPG[i].m_strPlotOutline;
        epgentry.m_strPlot           = m_channels_radio[channel].m_EPG[i].m_strPlot;
        epgentry.m_GenreType         = m_channels_radio[channel].m_EPG[i].m_GenreType;
        epgentry.m_GenreSubType      = m_channels_radio[channel].m_EPG[i].m_GenreSubType;
        epgentry.m_strGenre          = m_channels_radio[channel].m_EPG[i].m_strGenre;
        epgentry.m_startTime         = m_channels_radio[channel].m_EPG[i].m_startTime;
        epgentry.m_endTime           = m_channels_radio[channel].m_EPG[i].m_endTime;
        epgentry.m_duration          = m_channels_radio[channel].m_EPG[i].m_duration;
        epgentry.m_channelNum        = m_channels_radio[channel].m_iChannelNum;
        epgentry.m_idChannel         = m_channels_radio[channel].m_iIdChannel;

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
    for (unsigned int i = 0; i < m_channels_tv.size(); i++)
    {
      CTVEPGInfoTag epgnow(NULL);

      if (m_channels_tv[i].m_hide)
        continue;

      m_channels_tv[i].GetEPGNowInfo(&epgnow);

      CFileItemPtr channel(new CFileItem(epgnow));
      channel->SetLabel2(epgnow.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = m_channels_tv[i].m_strChannel;
      channel->SetThumbnailImage(m_channels_tv[i].m_IconPath);
      results->Add(channel);
      cnt++;
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_channels_radio.size(); i++)
    {
      CTVEPGInfoTag epgnow(NULL);

      if (m_channels_radio[i].m_hide)
        continue;

      m_channels_radio[i].GetEPGNowInfo(&epgnow);

      CFileItemPtr channel(new CFileItem(epgnow));
      channel->SetLabel2(epgnow.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = m_channels_radio[i].m_strChannel;
      channel->SetThumbnailImage(m_channels_radio[i].m_IconPath);
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
    for (unsigned int i = 0; i < m_channels_tv.size(); i++)
    {
      CTVEPGInfoTag epgnext(NULL);

      if (m_channels_tv[i].m_hide)
        continue;

      m_channels_tv[i].GetEPGNextInfo(&epgnext);

      CFileItemPtr channel(new CFileItem(epgnext));
      channel->SetLabel2(epgnext.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = m_channels_tv[i].m_strChannel;
      channel->SetThumbnailImage(m_channels_tv[i].m_IconPath);
      results->Add(channel);
      cnt++;
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_channels_radio.size(); i++)
    {
      CTVEPGInfoTag epgnext(NULL);

      if (m_channels_radio[i].m_hide)
        continue;

      m_channels_radio[i].GetEPGNextInfo(&epgnext);

      CFileItemPtr channel(new CFileItem(epgnext));
      channel->SetLabel2(epgnext.m_startTime.GetAsLocalizedTime("", false));
      channel->m_strPath = m_channels_radio[i].m_strChannel;
      channel->SetThumbnailImage(m_channels_radio[i].m_IconPath);
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
    for (unsigned int i = 0; i < m_channels_tv[number-1].m_EPG.size(); i++)
    {
      CTVEPGInfoTag epgentry(NULL);

      epgentry.m_strChannel        = m_channels_tv[number-1].m_strChannel;
      epgentry.m_strTitle          = m_channels_tv[number-1].m_EPG[i].m_strTitle;
      epgentry.m_strPlotOutline    = m_channels_tv[number-1].m_EPG[i].m_strPlotOutline;
      epgentry.m_strPlot           = m_channels_tv[number-1].m_EPG[i].m_strPlot;
      epgentry.m_GenreType         = m_channels_tv[number-1].m_EPG[i].m_GenreType;
      epgentry.m_GenreSubType      = m_channels_tv[number-1].m_EPG[i].m_GenreSubType;
      epgentry.m_strGenre          = m_channels_tv[number-1].m_EPG[i].m_strGenre;
      epgentry.m_startTime         = m_channels_tv[number-1].m_EPG[i].m_startTime;
      epgentry.m_endTime           = m_channels_tv[number-1].m_EPG[i].m_endTime;
      epgentry.m_duration          = m_channels_tv[number-1].m_EPG[i].m_duration;
      epgentry.m_channelNum        = m_channels_tv[number-1].m_iChannelNum;
      epgentry.m_idChannel         = m_channels_tv[number-1].m_iIdChannel;
      epgentry.m_isRadio           = m_channels_tv[number-1].m_radio;

      CFileItemPtr channel(new CFileItem(epgentry));
      channel->SetLabel2(epgentry.m_startTime.GetAsLocalizedDateTime(false, false));
      results->Add(channel);
      cnt++;
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_channels_radio[number-1].m_EPG.size(); i++)
    {
      CTVEPGInfoTag epgentry(NULL);

      epgentry.m_strChannel        = m_channels_radio[number-1].m_strChannel;
      epgentry.m_strTitle          = m_channels_radio[number-1].m_EPG[i].m_strTitle;
      epgentry.m_strPlotOutline    = m_channels_radio[number-1].m_EPG[i].m_strPlotOutline;
      epgentry.m_strPlot           = m_channels_radio[number-1].m_EPG[i].m_strPlot;
      epgentry.m_GenreType         = m_channels_radio[number-1].m_EPG[i].m_GenreType;
      epgentry.m_GenreSubType      = m_channels_radio[number-1].m_EPG[i].m_GenreSubType;
      epgentry.m_strGenre          = m_channels_radio[number-1].m_EPG[i].m_strGenre;
      epgentry.m_startTime         = m_channels_radio[number-1].m_EPG[i].m_startTime;
      epgentry.m_endTime           = m_channels_radio[number-1].m_EPG[i].m_endTime;
      epgentry.m_duration          = m_channels_radio[number-1].m_EPG[i].m_duration;
      epgentry.m_channelNum        = m_channels_radio[number-1].m_iChannelNum;
      epgentry.m_idChannel         = m_channels_radio[number-1].m_iIdChannel;

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

int CPVRManager::GetNumChannels()
{
  return m_channels_tv.size() + m_channels_radio.size();
}

int CPVRManager::GetNumHiddenChannels()
{
  return m_HiddenChannels;
}

int CPVRManager::GetTVChannels(CFileItemList* results, int group_id, bool hidden)
{
  EnterCriticalSection(&m_critSection);

  int cnt = 0;

  for (unsigned int i = 0; i < m_channels_tv.size(); i++)
  {
    if (m_channels_tv[i].m_hide != hidden)
      continue;

    if ((group_id != -1) && (m_channels_tv[i].m_iGroupID != group_id))
      continue;

    CTVEPGInfoTag epgnow(NULL);
    if (m_channels_tv[i].GetEPGNowInfo(&epgnow))
    {
      m_channels_tv[i].m_strTitle          = epgnow.m_strTitle;
      m_channels_tv[i].m_strOriginalTitle  = epgnow.m_strTitle;
      m_channels_tv[i].m_strPlotOutline    = epgnow.m_strPlotOutline;
      m_channels_tv[i].m_strPlot           = epgnow.m_strPlot;
      m_channels_tv[i].m_strGenre          = epgnow.m_strGenre;
      m_channels_tv[i].m_startTime         = epgnow.m_startTime;
      m_channels_tv[i].m_endTime           = epgnow.m_endTime;
      m_channels_tv[i].m_duration          = epgnow.m_duration;

      if (m_channels_tv[i].m_strPlot.Left(m_channels_tv[i].m_strPlotOutline.length()) != m_channels_tv[i].m_strPlotOutline && !m_channels_tv[i].m_strPlotOutline.IsEmpty())
        m_channels_tv[i].m_strPlot = m_channels_tv[i].m_strPlotOutline + '\n' + m_channels_tv[i].m_strPlot;

      CDateTimeSpan span = m_channels_tv[i].m_startTime - m_channels_tv[i].m_endTime;

      StringUtils::SecondsToTimeString(span.GetSeconds()
                                       + span.GetMinutes() * 60.
                                       + span.GetHours() * 3600, m_channels_tv[i].m_strRuntime, TIME_FORMAT_GUESS);
    }
    else
    {
      m_channels_tv[i].m_strTitle          = g_localizeStrings.Get(18074);
      m_channels_tv[i].m_strOriginalTitle  = g_localizeStrings.Get(18074);
      m_channels_tv[i].m_strPlotOutline    = "";
      m_channels_tv[i].m_strPlot           = "";
      m_channels_tv[i].m_strGenre          = "";
      m_channels_tv[i].m_startTime         = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0);
      m_channels_tv[i].m_endTime           = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0);
      m_channels_tv[i].m_duration          = CDateTimeSpan(0, 1, 0, 0);
    }

    m_channels_tv[i].m_strAlbum = m_channels_tv[i].m_strChannel;

    m_channels_tv[i].m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
    m_channels_tv[i].m_iEpisode = 0;
    m_channels_tv[i].m_strShowTitle.Format("%i", m_channels_tv[i].m_iChannelNum);

    CFileItemPtr channel(new CFileItem(m_channels_tv[i]));
    
    channel->SetProperty("start_time", m_channels_tv[i].m_startTime.GetAsLocalizedTime("", false));
    channel->SetProperty("end_time", m_channels_tv[i].m_endTime.GetAsLocalizedTime("", false));


    results->Add(channel);
    cnt++;
  }

  LeaveCriticalSection(&m_critSection);

  return cnt;
}

int CPVRManager::GetRadioChannels(CFileItemList* results, int group_id, bool hidden)
{
  EnterCriticalSection(&m_critSection);

  int cnt = 0;

  for (unsigned int i = 0; i < m_channels_radio.size(); i++)
  {
    if (m_channels_radio[i].m_hide != hidden)
      continue;

    if ((group_id != -1) && (m_channels_radio[i].m_iGroupID != group_id))
      continue;

    CTVEPGInfoTag epgnow(NULL);
    if (m_channels_radio[i].GetEPGNowInfo(&epgnow))
    {
      m_channels_radio[i].m_strTitle          = epgnow.m_strTitle;
      m_channels_radio[i].m_strOriginalTitle  = epgnow.m_strTitle;
      m_channels_radio[i].m_strPlotOutline    = epgnow.m_strPlotOutline;
      m_channels_radio[i].m_strPlot           = epgnow.m_strPlot;
      m_channels_radio[i].m_strGenre          = epgnow.m_strGenre;
      m_channels_radio[i].m_startTime         = epgnow.m_startTime;
      m_channels_radio[i].m_endTime           = epgnow.m_endTime;
      m_channels_radio[i].m_duration          = epgnow.m_duration;

      if (m_channels_radio[i].m_strPlot.Left(m_channels_radio[i].m_strPlotOutline.length()) != m_channels_radio[i].m_strPlotOutline && !m_channels_radio[i].m_strPlotOutline.IsEmpty())
        m_channels_radio[i].m_strPlot = m_channels_radio[i].m_strPlotOutline + '\n' + m_channels_radio[i].m_strPlot;

      CDateTimeSpan span = m_channels_radio[i].m_startTime - m_channels_radio[i].m_endTime;

      StringUtils::SecondsToTimeString(span.GetSeconds()
                                       + span.GetMinutes() * 60.
                                       + span.GetHours() * 3600, m_channels_radio[i].m_strRuntime, TIME_FORMAT_GUESS);
    }
    else
    {
      m_channels_radio[i].m_strTitle          = g_localizeStrings.Get(18074);
      m_channels_radio[i].m_strOriginalTitle  = g_localizeStrings.Get(18074);
      m_channels_radio[i].m_strPlotOutline    = "";
      m_channels_radio[i].m_strPlot           = "";
      m_channels_radio[i].m_strGenre          = "";
      m_channels_radio[i].m_startTime         = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0);
      m_channels_radio[i].m_endTime           = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0);
      m_channels_radio[i].m_duration          = CDateTimeSpan(0, 1, 0, 0);
    }

    m_channels_radio[i].m_strAlbum = m_channels_radio[i].m_strChannel;

    m_channels_radio[i].m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
    m_channels_radio[i].m_iEpisode = 0;
    m_channels_radio[i].m_strShowTitle.Format("%i", m_channels_radio[i].m_iChannelNum);

    CFileItemPtr channel(new CFileItem(m_channels_radio[i]));

    SetMusicInfoTag(*channel, i+1);
    results->Add(channel);
    cnt++;
  }

  LeaveCriticalSection(&m_critSection);

  return cnt;
}

void CPVRManager::MoveChannel(unsigned int oldindex, unsigned int newindex, bool radio)
{
  VECCHANNELS m_channels_temp;

  if ((newindex == oldindex) || (newindex == 0))
    return;

  EnterCriticalSection(&m_critSection);

  m_database.Open();

  if (!radio)
  {
    int CurrentChannelID = m_channels_tv[m_CurrentTVChannel].m_iIdChannel;
    int CurrentClientChannel = GetClientChannelNumber(CurrentChannelID, false);

    m_channels_temp.push_back(m_channels_tv[oldindex-1]);
    m_channels_tv.erase(m_channels_tv.begin()+oldindex-1);
    if (newindex < m_channels_tv.size())
      m_channels_tv.insert(m_channels_tv.begin()+newindex-1, m_channels_temp[0]);
    else
      m_channels_tv.push_back(m_channels_temp[0]);

    for (unsigned int i = 0; i < m_channels_tv.size(); i++)
    {
      if (m_channels_tv[i].m_iChannelNum != i+1)
      {
        m_channels_tv[i].m_iChannelNum = i+1;
        m_channels_tv[i].m_strFileNameAndPath.Format("tv://%i", m_channels_tv[i].m_iChannelNum);
        m_database.UpdateChannel(m_currentClientID, m_channels_tv[i]);
      }
    }

    CLog::Log(LOGNOTICE, "PVR: TV Channel %d moved to %d", oldindex, newindex);

    if (m_isPlayingTV && m_channels_tv[m_CurrentTVChannel].m_iIdChannel != CurrentChannelID)
    {
      /* Perform Channel switch with new number, if played channelnumber is modified */
      GetFrontendChannelNumber(CurrentClientChannel, m_currentClientID, &m_CurrentTVChannel, NULL);
      CFileItemPtr channel(new CFileItem(m_channels_tv[m_CurrentTVChannel]));
      g_application.PlayFile(*channel);
    }
  }
  else
  {
    int CurrentChannelID = m_channels_radio[m_CurrentRadioChannel].m_iIdChannel;
    int CurrentClientChannel = GetClientChannelNumber(CurrentChannelID, false);

    m_channels_temp.push_back(m_channels_radio[oldindex-1]);
    m_channels_radio.erase(m_channels_radio.begin()+oldindex-1);
    if (newindex < m_channels_radio.size())
      m_channels_radio.insert(m_channels_radio.begin()+newindex-1, m_channels_temp[0]);
    else
      m_channels_radio.push_back(m_channels_temp[0]);

    for (unsigned int i = 0; i < m_channels_radio.size(); i++)
    {
      if (m_channels_radio[i].m_iChannelNum != i+1)
      {
        m_channels_radio[i].m_iChannelNum = i+1;
        m_channels_radio[i].m_strFileNameAndPath.Format("radio://%i", m_channels_radio[i].m_iChannelNum);
        m_database.UpdateChannel(m_currentClientID, m_channels_radio[i]);
      }
    }

    CLog::Log(LOGNOTICE, "PVR: TV Channel %d moved to %d", oldindex, newindex);

    if (m_isPlayingTV && m_channels_radio[m_CurrentRadioChannel].m_iIdChannel != CurrentChannelID)
    {
      /* Perform Channel switch with new number, if played channelnumber is modified */
      GetFrontendChannelNumber(CurrentClientChannel, m_currentClientID, &m_CurrentRadioChannel, NULL);
      CFileItemPtr channel(new CFileItem(m_channels_radio[m_CurrentRadioChannel]));
      g_application.PlayFile(*channel);
    }
  }
  m_database.Close();

  /* Synchronize channel numbers inside timers */
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    GetFrontendChannelNumber(m_timers[i].m_clientNum, m_currentClientID, &m_timers[i].m_channelNum, &m_timers[i].m_Radio);
  }

  LeaveCriticalSection(&m_critSection);

  return;
}

void CPVRManager::HideChannel(unsigned int number, bool radio)
{
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    if ((m_timers[i].m_channelNum == number) && (m_timers[i].m_Radio == radio))
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (!pDialog)
        return;

      pDialog->SetHeading(18090);
      pDialog->SetLine(0, 18095);
      pDialog->SetLine(1, "");
      pDialog->SetLine(2, 18096);
      pDialog->DoModal();

      if (!pDialog->IsConfirmed())
        return;

      DeleteTimer(m_timers[i].m_Index, true);
    }
  }

  if (!radio)
  {
    if (m_isPlayingTV && m_CurrentTVChannel == number)
    {
      CGUIDialogOK::ShowAndGetInput(18090,18097,0,18098);
      return;
    }

    if (m_channels_tv[number-1].m_hide)
    {
      EnterCriticalSection(&m_critSection);
      m_channels_tv[number-1].m_hide = false;
      m_database.Open();
      m_database.UpdateChannel(m_currentClientID, m_channels_tv[number-1]);
      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);
      m_database.Close();
      LeaveCriticalSection(&m_critSection);
    }
    else
    {
      EnterCriticalSection(&m_critSection);
      m_channels_tv[number-1].m_hide = true;
      m_channels_tv[number-1].m_EPG.erase(m_channels_tv[number-1].m_EPG.begin(), m_channels_tv[number-1].m_EPG.end());
      m_database.Open();
      m_database.UpdateChannel(m_currentClientID, m_channels_tv[number-1]);
      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);
      m_database.Close();
      LeaveCriticalSection(&m_critSection);
      MoveChannel(number, m_channels_tv.size(), false);
    }
  }
  else
  {
    if (m_isPlayingRadio && m_CurrentRadioChannel == number)
    {
      CGUIDialogOK::ShowAndGetInput(18090,18097,0,18098);
      return;
    }

    if (m_channels_radio[number-1].m_hide)
    {
      EnterCriticalSection(&m_critSection);
      m_channels_radio[number-1].m_hide = false;
      m_channels_radio[number-1].m_EPG.erase(m_channels_radio[number-1].m_EPG.begin(), m_channels_radio[number-1].m_EPG.end());
      m_database.Open();
      m_database.UpdateChannel(m_currentClientID, m_channels_radio[number-1]);
      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);
      m_database.Close();
      LeaveCriticalSection(&m_critSection);
    }
    else
    {
      EnterCriticalSection(&m_critSection);
      m_channels_radio[number-1].m_hide = true;
      m_channels_radio[number-1].m_EPG.erase(m_channels_radio[number-1].m_EPG.begin(), m_channels_tv[number-1].m_EPG.end());
      m_database.Open();
      m_database.UpdateChannel(m_currentClientID, m_channels_radio[number-1]);
      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);
      m_database.Close();
      LeaveCriticalSection(&m_critSection);
      MoveChannel(number, m_channels_radio.size(), true);
    }
  }
}

void CPVRManager::SetChannelIcon(unsigned int number, CStdString icon, bool radio)
{
  EnterCriticalSection(&m_critSection);

  if (!radio)
  {
    if (m_channels_tv[number-1].m_IconPath != icon)
    {
      m_database.Open();
      m_channels_tv[number-1].m_IconPath = icon;
      m_database.UpdateChannel(m_currentClientID, m_channels_tv[number-1]);
      m_database.Close();
    }
  }
  else
  {
    if (m_channels_radio[number-1].m_IconPath != icon)
    {
      m_database.Open();
      m_channels_radio[number-1].m_IconPath = icon;
      m_database.UpdateChannel(m_currentClientID, m_channels_radio[number-1]);
      m_database.Close();
    }
  }

  LeaveCriticalSection(&m_critSection);
}

CStdString CPVRManager::GetChannelIcon(unsigned int number, bool radio)
{
  if (!radio)
    return m_channels_tv[number-1].m_IconPath;
  else
    return m_channels_radio[number-1].m_IconPath;
}

CStdString CPVRManager::GetNameForChannel(unsigned int number, bool radio)
{
  if (m_client)
  {
    if (!radio)
    {
      if (((int) number <= m_channels_tv.size()+1) && (number != 0))
      {
        if (m_channels_tv[number-1].m_strChannel != NULL)
          return m_channels_tv[number-1].m_strChannel;
        else
          return g_localizeStrings.Get(13205);
      }
    }
    else
    {
      if (((int) number <= m_channels_radio.size()+1) && (number != 0))
      {
        if (m_channels_radio[number-1].m_strChannel != NULL)
          return m_channels_radio[number-1].m_strChannel;
        else
          return g_localizeStrings.Get(13205);
      }
    }
  }

  return "";
}

bool CPVRManager::GetFrontendChannelNumber(unsigned int client_no, unsigned int client_id, int *frontend_no, bool *isRadio)
{
  for (unsigned int i = 0; i < m_channels_tv.size(); i++)
  {
    if (m_channels_tv[i].m_iClientNum == client_no && m_channels_tv[i].m_clientID == client_id)
    {
      if (frontend_no != NULL)
        *frontend_no = m_channels_tv[i].m_iChannelNum;

      if (isRadio != NULL)
        *isRadio = false;

      return true;
    }
  }

  for (unsigned int i = 0; i < m_channels_radio.size(); i++)
  {
    if (m_channels_radio[i].m_iClientNum == client_no)
    {
      if (frontend_no != NULL)
        *frontend_no = m_channels_radio[i].m_iChannelNum;

      if (isRadio != NULL)
        *isRadio = true;

      return true;
    }
  }

  return false;
}

int CPVRManager::GetClientChannelNumber(unsigned int frontend_no, bool radio)
{
  if (m_client)
  {
    if (!radio)
    {
      if (((int) frontend_no <= m_channels_tv.size()+1) && (frontend_no != 0))
        return m_channels_tv[frontend_no-1].m_iClientNum;
    }
    else
    {
      if (((int) frontend_no <= m_channels_radio.size()+1) && (frontend_no != 0))
        return m_channels_radio[frontend_no-1].m_iClientNum;
    }
  }

  return -1;
}

int CPVRManager::GetChannelID(unsigned int frontend_no, bool radio)
{
  if (m_client)
  {
    if (!radio)
    {
      if (((int) frontend_no <= m_channels_tv.size()+1) && (frontend_no != 0))
        return m_channels_tv[frontend_no-1].m_iIdChannel;
    }
    else
    {
      if (((int) frontend_no <= m_channels_radio.size()+1) && (frontend_no != 0))
        return m_channels_radio[frontend_no-1].m_iIdChannel;
    }
  }

  return -1;
}

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

  for (unsigned int i = 0; i < m_channels_tv.size(); i++)
  {
    if (m_channels_tv[i].m_iGroupID == GroupId)
    {
      m_channels_tv[i].m_iGroupID = 0;
      m_database.UpdateChannel(m_currentClientID, m_channels_tv[i]);
    }
  }
  for (unsigned int i = 0; i < m_channels_radio.size(); i++)
  {
    if (m_channels_radio[i].m_iGroupID == GroupId)
    {
      m_channels_radio[i].m_iGroupID = 0;
      m_database.UpdateChannel(m_currentClientID, m_channels_radio[i]);
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
      if (((int) number <= m_channels_tv.size()+1) && (number != 0))
      {
        EnterCriticalSection(&m_critSection);
        m_database.Open();
        m_channels_tv[number-1].m_iGroupID = GroupId;
        m_database.UpdateChannel(m_currentClientID, m_channels_tv[number-1]);
        m_database.Close();
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    else
    {
      if (((int) number <= m_channels_radio.size()+1) && (number != 0))
      {
        EnterCriticalSection(&m_critSection);
        m_database.Open();
        m_channels_radio[number-1].m_iGroupID = GroupId;
        m_database.UpdateChannel(m_currentClientID, m_channels_radio[number-1]);
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
    for (unsigned int i = 0; i < m_channels_tv.size(); i++)
    {
      if (m_channels_tv[i].m_iGroupID == GroupId)
        return i+1;
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_channels_radio.size(); i++)
    {
      if (m_channels_radio[i].m_iGroupID == GroupId)
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

int CPVRManager::GetNumRecordings()
{
  return m_recordings.size();
}

int CPVRManager::GetAllRecordings(CFileItemList* results)
{
  EnterCriticalSection(&m_critSection);
  GetRecordings();

  for (unsigned int i = 0; i < m_recordings.size(); ++i)
  {
    if ((m_recordings[i].m_startTime < CDateTime::GetCurrentDateTime()) &&
        (m_recordings[i].m_endTime > CDateTime::GetCurrentDateTime()))
    {
      for (unsigned int j = 0; j < m_timers.size(); ++j)
      {
        if ((m_timers[j].m_strChannel == m_recordings[i].m_strChannel)  &&
            (m_timers[j].m_StartTime  <= CDateTime::GetCurrentDateTime()) &&
            (m_timers[j].m_StopTime   >= CDateTime::GetCurrentDateTime()) &&
            (m_timers[j].m_Repeat != true) && (m_timers[j].m_Active == true))
        {
          m_recordings[i].m_Summary.Format("%s", g_localizeStrings.Get(18069));
        }
      }
    }

    CFileItemPtr record(new CFileItem(m_recordings[i]));
    results->Add(record);
  }

  LeaveCriticalSection(&m_critSection);

  return m_recordings.size();
}

bool CPVRManager::DeleteRecording(unsigned int index)
{
  if (m_client && m_clientProps.SupportRecordings)
  {
    PVR_ERROR err = m_client->DeleteRecording(m_recordings[index-1]);

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
    }
    else if (err == PVR_ERROR_NOT_SYNC)
    {
      /* print info dialog "Recordings not in sync!" */
      CGUIDialogOK::ShowAndGetInput(18100,18810,18803,0);
    }
    else if (err == PVR_ERROR_NOT_DELETED)
    {
      /* print info dialog "Couldn't delete recording!" */
      CGUIDialogOK::ShowAndGetInput(18100,18811,18803,0);
    }
    else
    {
      /* print info dialog "Unknown error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
    }
  }

  return false;
}

bool CPVRManager::RenameRecording(unsigned int index, CStdString &newname)
{
  if (m_client && m_clientProps.SupportRecordings)
  {
    PVR_ERROR err = m_client->RenameRecording(m_recordings[index-1], newname);

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
    }
    else if (err == PVR_ERROR_NOT_SYNC)
    {
      /* print info dialog "Recordings not in sync!" */
      CGUIDialogOK::ShowAndGetInput(18100,18810,18803,0);
    }
    else if (err == PVR_ERROR_NOT_SAVED)
    {
      /* print info dialog "Couldn't delete recording!" */
      CGUIDialogOK::ShowAndGetInput(18100,18811,18803,0);
    }
    else
    {
      /* print info dialog "Unknown error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
    }
  }

  return false;
}


/************************************************************/
/** Timer handling **/

int CPVRManager::GetNumTimers()
{
  return m_timers.size();
}

int CPVRManager::GetAllTimers(CFileItemList* results)
{
  EnterCriticalSection(&m_critSection);

  ReceiveAllTimers();

  for (unsigned int i = 0; i < m_timers.size(); ++i)
  {
    CFileItemPtr timer(new CFileItem(m_timers[i]));
    results->Add(timer);
  }
  
  /* Syncronize Timer Info labels */
  SyncInfo();

  LeaveCriticalSection(&m_critSection);

  return m_timers.size();
}

bool CPVRManager::AddTimer(const CFileItem &item)
{
  if (m_client && m_clientProps.SupportTimers)
  {
    /* Check if a CTVTimerInfoTag is inside file item */
    if (!item.IsTVTimer())
    {
      CLog::Log(LOGERROR, "CPVRManager: UpdateTimer no TVInfoTag given!");
      return false;
    }

    PVR_ERROR err = m_client->AddTimer(*item.GetTVTimerInfoTag());

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
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
  else
  {
    return false;
  }
}

bool CPVRManager::DeleteTimer(unsigned int index, bool force)
{
  if (m_client && m_clientProps.SupportTimers)
  {
    PVR_ERROR err = PVR_ERROR_NO_ERROR;

    if (force)
      err = m_client->DeleteTimer(m_timers[index-1], true);
    else
      err = m_client->DeleteTimer(m_timers[index-1]);

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

        err = m_client->DeleteTimer(m_timers[index-1], true);
      }
    }

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      // print info dialog "Server error!"
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
      return false;
    }
    else if (err == PVR_ERROR_NOT_SYNC)
    {
      // print info dialog "Timers not in sync!"
      CGUIDialogOK::ShowAndGetInput(18100,18800,18803,0);
      return false;
    }
    else if (err == PVR_ERROR_NOT_DELETED)
    {
      // print info dialog "Couldn't delete timer!"
      CGUIDialogOK::ShowAndGetInput(18100,18802,18803,0);
      return false;
    }
    else
    {
      // print info dialog "Unknown error!"
      CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0);
      return false;
    }
  }
  else
  {
    return false;
  }
}

bool CPVRManager::RenameTimer(unsigned int index, CStdString &newname)
{
  if (m_client && m_clientProps.SupportTimers)
  {
    PVR_ERROR err = m_client->RenameTimer(m_timers[index-1], newname);

    if (err == PVR_ERROR_NOT_IMPLEMENTED)
    {
      m_timers[index-1].m_strTitle = newname;
      err = m_client->UpdateTimer(m_timers[index-1]);
    }

    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
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
  if (m_client && m_clientProps.SupportTimers)
  {
    /* Check if a CTVTimerInfoTag is inside file item */
    if (!item.IsTVTimer())
    {
      CLog::Log(LOGERROR, "CPVRManager: UpdateTimer no TVInfoTag given!");
      return false;
    }
    
    const CTVTimerInfoTag* tag = item.GetTVTimerInfoTag();

    /* and write it to the backend */
    PVR_ERROR err = m_clients[tag->m_clientID]->UpdateTimer(tag);

    /* Check for errors and inform the user */
    if (err == PVR_ERROR_NO_ERROR)
    {
      return true;
    }
    else if (err == PVR_ERROR_SERVER_ERROR)
    {
      /* print info dialog "Server error!" */
      CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0);
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
  }

  return NULL;
}

void CPVRManager::ReceiveAllTimers()
{
  EnterCriticalSection(&m_critSection);
  
  /* Clear all current present Timers inside list */
  m_timers.erase(m_timers.begin(), m_timers.end());

  /* Go thru all clients and receive there timers */
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    /* Load only if the client have timers */
    if (m_clients[(*itr).first]->GetNumTimers() > 0)
    {
      m_clients[(*itr).first]->GetAllTimers(&m_timers);
    }
    itr++;
  }

  /* Set the XBMC Channel number and Channel Name for the timers */
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    GetFrontendChannelNumber(m_timers[i].m_clientNum, m_timers[i].m_clientID, &m_timers[i].m_channelNum, &m_timers[i].m_Radio);
    m_timers[i].m_strChannel = GetNameForChannel(m_timers[i].m_channelNum, m_timers[i].m_Radio);
  }

  LeaveCriticalSection(&m_critSection);

  return;
}


/************************************************************/
/** Live stream handling **/

bool CPVRManager::OpenLiveStream(unsigned int channel, bool radio)
{
  bool ret = false;
  m_scanStart = timeGetTime();

  EnterCriticalSection(&m_critSection);

  if (m_client)
  {
    if (!radio)
    {
      LoadVideoSettings(m_channels_tv[channel-1].m_iIdChannel, false);

      if (m_client->OpenLiveStream(GetClientChannelNumber(channel, radio)))
      {
        m_CurrentTVChannel      = channel;
        m_CurrentChannelID      = m_channels_tv[channel-1].m_iIdChannel;
        m_isPlayingTV           = true;
        m_isPlayingRadio        = false;
        m_isPlayingRecording    = false;
        SetCurrentPlayingProgram();
        ret = true;
      }
    }
    else
    {
      LoadVideoSettings(m_channels_radio[channel-1].m_iIdChannel, false);

      if (m_client->OpenLiveStream(GetClientChannelNumber(channel, radio)))
      {
        m_CurrentRadioChannel   = channel;
        m_CurrentChannelID      = m_channels_radio[channel-1].m_iIdChannel;
        m_isPlayingTV           = false;
        m_isPlayingRadio        = true;
        m_isPlayingRecording    = false;
        SetCurrentPlayingProgram();
        ret = true;
      }
    }
  }

  LeaveCriticalSection(&m_critSection);

  return ret;
}

void CPVRManager::CloseLiveStream()
{

  if (m_client)
  {
    EnterCriticalSection(&m_critSection);

    SaveVideoSettings(m_CurrentChannelID);
    m_client->CloseLiveStream();
    m_CurrentTVChannel      = 1;
    m_CurrentRadioChannel   = 1;
    m_CurrentChannelID      = -1;
    m_isPlayingTV           = false;
    m_isPlayingRadio        = false;
    m_isPlayingRecording    = false;

    LeaveCriticalSection(&m_critSection);
  }
  return;
}

void CPVRManager::PauseLiveStream(bool OnOff)
{

}

int CPVRManager::ReadLiveStream(BYTE* buf, int buf_size)
{
  if (m_client)
  {
    if (m_scanStart)
    {
      if (timeGetTime() - m_scanStart > g_guiSettings.GetInt("pvrmenu.scantime")*1000)
        return 0;
      else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
        m_scanStart = NULL;
    }

    return m_client->ReadLiveStream(buf, buf_size);
  }

  return 0;
}

__int64 CPVRManager::SeekLiveStream(__int64 pos, int whence)
{

  return 0;
}

int CPVRManager::GetCurrentChannel(bool radio)
{
  if (!radio)
    return m_CurrentTVChannel;
  else
    return m_CurrentRadioChannel;
}

bool CPVRManager::ChannelSwitch(unsigned int iChannel)
{
  if (m_client)
  {
    if (m_isPlayingTV)
    {
      if (iChannel > m_channels_tv.size()+1)
      {
        CGUIDialogOK::ShowAndGetInput(18100,18105,0,0);
        return false;
      }

      EnterCriticalSection(&m_critSection);

      SaveVideoSettings(m_CurrentChannelID);
      LoadVideoSettings(m_channels_tv[iChannel-1].m_iIdChannel);
      if (!m_client->SwitchChannel(GetClientChannelNumber(iChannel, false)))
      {
        CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
        LeaveCriticalSection(&m_critSection);
        return false;
      }
      m_CurrentTVChannel  = iChannel;
      m_CurrentChannelID  = m_channels_tv[iChannel-1].m_iIdChannel;
      m_CurrentGroupID    = m_channels_tv[iChannel-1].m_iGroupID;
      SetCurrentPlayingProgram();

      LeaveCriticalSection(&m_critSection);
    }
    else if (m_isPlayingRadio)
    {
      if (iChannel > m_channels_radio.size()+1)
      {
        CGUIDialogOK::ShowAndGetInput(18100,18105,0,0);
        return false;
      }

      EnterCriticalSection(&m_critSection);

      SaveVideoSettings(m_CurrentChannelID);
      LoadVideoSettings(m_channels_radio[iChannel-1].m_iIdChannel);
      if (!m_client->SwitchChannel(GetClientChannelNumber(iChannel, true)))
      {
        CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
        LeaveCriticalSection(&m_critSection);
        return false;
      }
      m_CurrentRadioChannel = iChannel;
      m_CurrentChannelID    = m_channels_radio[iChannel-1].m_iIdChannel;
      m_CurrentGroupID      = m_channels_radio[iChannel-1].m_iGroupID;
      SetCurrentPlayingProgram();

      LeaveCriticalSection(&m_critSection);
    }

    return true;
  }

  return false;
}

bool CPVRManager::ChannelUp(unsigned int *newchannel)
{
  if (m_client)
  {
    if (m_isPlayingTV)
    {
      EnterCriticalSection(&m_critSection);
      SaveVideoSettings(m_CurrentChannelID);

      for (unsigned int i = 1; i < m_channels_tv.size(); i++)
      {
        m_CurrentTVChannel += 1;

        if (m_CurrentTVChannel > m_channels_tv.size())
          m_CurrentTVChannel = 1;

        if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != m_channels_tv[m_CurrentTVChannel-1].m_iGroupID))
          continue;

        m_CurrentChannelID = m_channels_tv[m_CurrentTVChannel-1].m_iIdChannel;
        LoadVideoSettings(m_CurrentChannelID);

        if (m_client->SwitchChannel(GetClientChannelNumber(m_CurrentTVChannel, false)))
        {
          SetCurrentPlayingProgram();
          *newchannel = m_CurrentTVChannel;
          LeaveCriticalSection(&m_critSection);
          return true;
        }
      }

      LeaveCriticalSection(&m_critSection);
    }
    else if (m_isPlayingRadio)
    {
      EnterCriticalSection(&m_critSection);
      SaveVideoSettings(m_CurrentChannelID);

      for (unsigned int i = 1; i < m_channels_radio.size(); i++)
      {
        m_CurrentRadioChannel += 1;

        if (m_CurrentRadioChannel > m_channels_radio.size())
          m_CurrentRadioChannel = 1;

        if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != m_channels_radio[m_CurrentRadioChannel-1].m_iGroupID))
          continue;

        m_CurrentChannelID = m_channels_radio[m_CurrentRadioChannel-1].m_iIdChannel;
        LoadVideoSettings(m_CurrentChannelID);

        if (m_client->SwitchChannel(GetClientChannelNumber(m_CurrentRadioChannel, true)))
        {
          SetCurrentPlayingProgram();
          *newchannel = m_CurrentRadioChannel;
          LeaveCriticalSection(&m_critSection);
          return true;
        }
      }

      LeaveCriticalSection(&m_critSection);
    }

    CGUIDialogOK::ShowAndGetInput(18100,18103,0,0);
  }

  return false;
}

bool CPVRManager::ChannelDown(unsigned int *newchannel)
{
  if (m_client)
  {
    if (m_isPlayingTV)
    {
      EnterCriticalSection(&m_critSection);
      SaveVideoSettings(m_CurrentChannelID);

      for (unsigned int i = 1; i < m_channels_tv.size(); i++)
      {
        m_CurrentTVChannel -= 1;

        if (m_CurrentTVChannel <= 0)
          m_CurrentTVChannel = m_channels_tv.size();

        if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != m_channels_tv[m_CurrentTVChannel-1].m_iGroupID))
          continue;

        m_CurrentChannelID = m_channels_tv[m_CurrentTVChannel-1].m_iIdChannel;
        LoadVideoSettings(m_CurrentChannelID);

        if (m_client->SwitchChannel(GetClientChannelNumber(m_CurrentTVChannel, false)))
        {
          SetCurrentPlayingProgram();
          *newchannel = m_CurrentTVChannel;
          LeaveCriticalSection(&m_critSection);
          return true;
        }
      }

      LeaveCriticalSection(&m_critSection);
    }
    else if (m_isPlayingRadio)
    {
      EnterCriticalSection(&m_critSection);
      SaveVideoSettings(m_CurrentChannelID);

      for (unsigned int i = 1; i < m_channels_radio.size(); i++)
      {
        m_CurrentRadioChannel -= 1;

        if (m_CurrentRadioChannel <= 0)
          m_CurrentRadioChannel = m_channels_radio.size();

        if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != m_channels_radio[m_CurrentRadioChannel-1].m_iGroupID))
          continue;

        m_CurrentChannelID = m_channels_radio[m_CurrentRadioChannel-1].m_iIdChannel;
        LoadVideoSettings(m_CurrentChannelID);

        if (m_client->SwitchChannel(GetClientChannelNumber(m_CurrentRadioChannel, true)))
        {
          SetCurrentPlayingProgram();
          *newchannel = m_CurrentRadioChannel;
          LeaveCriticalSection(&m_critSection);
          return true;
        }
      }

      LeaveCriticalSection(&m_critSection);
    }

    CGUIDialogOK::ShowAndGetInput(18100,18103,0,0);
  }

  return false;
}

int CPVRManager::GetTotalTime()
{
  if (m_client)
  {
    time_t time_s;

    if (m_isPlayingTV)
    {
      time_s =  m_channels_tv[m_CurrentTVChannel-1].m_duration.GetDays()*60*60*24;
      time_s += m_channels_tv[m_CurrentTVChannel-1].m_duration.GetHours()*60*60;
      time_s += m_channels_tv[m_CurrentTVChannel-1].m_duration.GetMinutes()*60;
      time_s += m_channels_tv[m_CurrentTVChannel-1].m_duration.GetSeconds();
    }
    else if (m_isPlayingRadio)
    {
      time_s =  m_channels_radio[m_CurrentRadioChannel-1].m_duration.GetDays()*60*60*24;
      time_s += m_channels_radio[m_CurrentRadioChannel-1].m_duration.GetHours()*60*60;
      time_s += m_channels_radio[m_CurrentRadioChannel-1].m_duration.GetMinutes()*60;
      time_s += m_channels_radio[m_CurrentRadioChannel-1].m_duration.GetSeconds();
    }

    time_s *= 1000;

    return time_s;
  }

  return -1;
}

int CPVRManager::GetStartTime()
{
  if (m_client)
  {
    CTVEPGInfoTag epgnow;
    time_t time_c, time_s;
    CDateTime endtime;

    if (m_isPlayingTV)
    {
      endtime = m_channels_tv[m_CurrentTVChannel-1].m_endTime;
    }
    else if (m_isPlayingRadio)
    {
      endtime = m_channels_radio[m_CurrentRadioChannel-1].m_endTime;
    }

    if (endtime < CDateTime::GetCurrentDateTime())
    {
      SetCurrentPlayingProgram();

      CFileItem item(g_application.CurrentFileItem());

      if (UpdateItem(item))
      {
        g_application.CurrentFileItem() = item;
        g_infoManager.SetCurrentItem(item);
      }
    }

    CDateTime::GetCurrentDateTime().GetAsTime(time_c);

    if (m_isPlayingTV)
    {
      m_channels_tv[m_CurrentTVChannel-1].m_startTime.GetAsTime(time_s);
    }
    else if (m_isPlayingRadio)
    {
      m_channels_radio[m_CurrentRadioChannel-1].m_startTime.GetAsTime(time_s);
    }

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

  CTVChannelInfoTag* tag = item.GetTVChannelInfoTag();

  if (m_isPlayingTV)
  {
    CTVEPGInfoTag epgnext(NULL);
    m_channels_tv[m_CurrentTVChannel-1].GetEPGNextInfo(&epgnext);

    tag->m_strAlbum         = m_channels_tv[m_CurrentTVChannel-1].m_strChannel;
    tag->m_strTitle         = m_channels_tv[m_CurrentTVChannel-1].m_strTitle;
    tag->m_strOriginalTitle = m_channels_tv[m_CurrentTVChannel-1].m_strTitle;
    tag->m_strPlotOutline   = m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline;
    tag->m_strPlot          = m_channels_tv[m_CurrentTVChannel-1].m_strPlot;
    tag->m_strGenre         = m_channels_tv[m_CurrentTVChannel-1].m_strGenre;
    tag->m_strShowTitle.Format("%i", m_channels_tv[m_CurrentTVChannel-1].m_iChannelNum);
    tag->m_strNextTitle     = epgnext.m_strTitle;
    tag->m_strPath          = m_channels_tv[m_CurrentTVChannel-1].m_strFileNameAndPath;
    tag->m_strFileNameAndPath = m_channels_tv[m_CurrentTVChannel-1].m_strFileNameAndPath;

    item.m_strTitle = m_channels_tv[m_CurrentTVChannel-1].m_strChannel;
    item.m_dateTime = m_channels_tv[m_CurrentTVChannel-1].m_startTime;

    CDateTimeSpan span = m_channels_tv[m_CurrentTVChannel-1].m_startTime - m_channels_tv[m_CurrentTVChannel-1].m_endTime;
    StringUtils::SecondsToTimeString(span.GetSeconds() + span.GetMinutes() * 60 + span.GetHours() * 3600,
                                     tag->m_strRuntime,
                                     TIME_FORMAT_GUESS);

    if (m_channels_tv[m_CurrentTVChannel-1].m_IconPath != "")
    {
      item.SetThumbnailImage(m_channels_tv[m_CurrentTVChannel-1].m_IconPath);
    }
    else
    {
      item.SetThumbnailImage("");
      item.FillInDefaultIcon();
    }
  }
  else if (m_isPlayingRadio)
  {
    CTVEPGInfoTag epgnext(NULL);
    m_channels_tv[m_CurrentRadioChannel-1].GetEPGNextInfo(&epgnext);

    tag->m_strAlbum         = m_channels_radio[m_CurrentRadioChannel-1].m_strChannel;
    tag->m_strTitle         = m_channels_radio[m_CurrentRadioChannel-1].m_strTitle;
    tag->m_strOriginalTitle = m_channels_radio[m_CurrentRadioChannel-1].m_strTitle;
    tag->m_strPlotOutline   = m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline;
    tag->m_strPlot          = m_channels_radio[m_CurrentRadioChannel-1].m_strPlot;
    tag->m_strGenre         = m_channels_radio[m_CurrentRadioChannel-1].m_strGenre;
    tag->m_strShowTitle.Format("%i", m_channels_radio[m_CurrentRadioChannel-1].m_iChannelNum);
    tag->m_strNextTitle     = epgnext.m_strTitle;
    tag->m_strPath          = m_channels_radio[m_CurrentRadioChannel-1].m_strFileNameAndPath;
    tag->m_strFileNameAndPath = m_channels_radio[m_CurrentRadioChannel-1].m_strFileNameAndPath;

    item.m_strTitle = m_channels_radio[m_CurrentRadioChannel-1].m_strChannel;
    item.m_dateTime = m_channels_radio[m_CurrentRadioChannel-1].m_startTime;

    CDateTimeSpan span = m_channels_radio[m_CurrentRadioChannel-1].m_startTime - m_channels_radio[m_CurrentRadioChannel-1].m_endTime;
    StringUtils::SecondsToTimeString(span.GetSeconds() + span.GetMinutes() * 60 + span.GetHours() * 3600,
                                     tag->m_strRuntime,
                                     TIME_FORMAT_GUESS);

    if (m_channels_radio[m_CurrentRadioChannel-1].m_IconPath != "")
    {
      item.SetThumbnailImage(m_channels_radio[m_CurrentRadioChannel-1].m_IconPath);
    }
    else
    {
      item.SetThumbnailImage("");
      item.FillInDefaultIcon();
    }
  }

  if ((tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline) && (!tag->m_strPlotOutline.IsEmpty()))
  {
    tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;
  }

  tag->m_iSeason      = 0; /* set this so xbmc knows it's a tv show */

  tag->m_iEpisode     = 0;
  tag->m_strStatus    = "livetv";

  g_infoManager.SetCurrentItem(item);
  return true;
}

void CPVRManager::SetPlayingGroup(int GroupId)
{
  m_CurrentGroupID = GroupId;
}

int CPVRManager::GetPlayingGroup() {
  return m_CurrentGroupID;
}


/************************************************************/
/** Recorded stream handling **/

bool CPVRManager::OpenRecordedStream(unsigned int record)
{
  EnterCriticalSection(&m_critSection);

  if (m_client)
  {
    if (m_client->OpenRecordedStream(m_recordings[record-1]))
    {
      m_isPlayingRecording    = true;
      m_isPlayingTV           = false;
      m_isPlayingRadio        = false;
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
    m_isPlayingRecording    = false;
    m_isPlayingTV           = false;
    m_isPlayingRadio        = false;
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
      return m_channels_tv[channel-1].m_isRecording;
    }
    else
    {
      return m_channels_radio[channel-1].m_isRecording;
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
      if (bOnOff && m_channels_tv[channel-1].m_isRecording == false)
      {
        CTVTimerInfoTag newtimer(true);
        CFileItem *item = new CFileItem(newtimer);

        if (!AddTimer(*item))
        {
          CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
          return true;
        }

        m_channels_tv[channel-1].m_isRecording = true;
      }
      else if (m_channels_tv[channel-1].m_isRecording == true)
      {
        for (unsigned int i = 0; i < m_timers.size(); ++i)
        {
          if ((m_timers[i].m_strChannel == m_channels_tv[channel-1].m_strChannel) &&
              (m_timers[i].m_StartTime  <= CDateTime::GetCurrentDateTime()) &&
              (m_timers[i].m_StopTime   >= CDateTime::GetCurrentDateTime()) &&
              (m_timers[i].m_Repeat != true) && (m_timers[i].m_Active == true))
          {
            DeleteTimer(m_timers[i].m_Index, true);
          }
        }

        m_channels_tv[channel-1].m_isRecording = false;
      }
    }
    else
    {
      if (bOnOff && m_channels_radio[channel-1].m_isRecording == false)
      {
        CTVTimerInfoTag newtimer(true);
        CFileItem *item = new CFileItem(newtimer);

        if (!AddTimer(*item))
        {
          CGUIDialogOK::ShowAndGetInput(18100,0,18053,0);
          return true;
        }

        m_channels_radio[channel-1].m_isRecording = true;
      }
      else if (m_channels_radio[channel-1].m_isRecording == true)
      {
        for (unsigned int i = 0; i < m_timers.size(); ++i)
        {
          if ((m_timers[i].m_strChannel == m_channels_tv[channel-1].m_strChannel) &&
              (m_timers[i].m_StartTime  <= CDateTime::GetCurrentDateTime()) &&
              (m_timers[i].m_StopTime   >= CDateTime::GetCurrentDateTime()) &&
              (m_timers[i].m_Repeat != true) && (m_timers[i].m_Active == true))
          {
            DeleteTimer(m_timers[i].m_Index, true);
          }
        }

        m_channels_radio[channel-1].m_isRecording = false;
      }
    }
  }
  return false;
}


/************************************************************/
/** Internal handling **/

void CPVRManager::GetChannels()
{
  VECCHANNELS m_channels_tv_tmp;
  VECCHANNELS m_channels_radio_tmp;

  EnterCriticalSection(&m_critSection);
  m_database.Open();

  m_client->GetChannelList(m_channels_tv_tmp, false);
  m_client->GetChannelList(m_channels_radio_tmp, true);

  /*
   * First whe look for moved channels on backend (other backend number)
   * and delete no more present channels inside database.
   * Problem:
   * If a channel on client is renamed, it is deleted from Database
   * and later added as new channel and loose his Group Information
   */
  for (unsigned int i = 0; i < m_channels_tv.size(); i++)
  {
    bool found = false;

    for (unsigned int j = 0; j < m_channels_tv_tmp.size(); j++)
    {
	  if (m_channels_tv[i].m_strChannel == m_channels_tv_tmp[j].m_strChannel)
	  {
	    if (m_channels_tv[i].m_iClientNum != m_channels_tv_tmp[j].m_iClientNum)
		{
		  m_channels_tv[i].m_iClientNum = m_channels_tv_tmp[j].m_iClientNum;
		  m_database.UpdateChannel(m_currentClientID, m_channels_tv[i]);
		  CLog::Log(LOGINFO,"PVRManager: Updated TV channel %s", m_channels_tv[i].m_strChannel.c_str());
		}

	    found = true;
		m_channels_tv_tmp.erase(m_channels_tv_tmp.begin()+j);
		break;
	  }
	}

	if (!found)
	{
	  CLog::Log(LOGINFO,"PVRManager: Removing TV channel %s (no more present)", m_channels_tv[i].m_strChannel.c_str());
      m_database.RemoveChannel(m_currentClientID, m_channels_tv[i]);
	  m_channels_tv.erase(m_channels_tv.begin()+i);
	  i--;
	}
  }

  for (unsigned int i = 0; i < m_channels_radio.size(); i++)
  {
    bool found = false;

    for (unsigned int j = 0; j < m_channels_radio_tmp.size(); j++)
    {
	  if (m_channels_radio[i].m_strChannel == m_channels_radio_tmp[j].m_strChannel)
	  {
	    if (m_channels_radio[i].m_iClientNum != m_channels_radio_tmp[j].m_iClientNum)
		{
		  m_channels_radio[i].m_iClientNum = m_channels_radio_tmp[j].m_iClientNum;
		  m_database.UpdateChannel(m_currentClientID, m_channels_radio[i]);
		  CLog::Log(LOGINFO,"PVRManager: Updated Radio channel %s", m_channels_radio[i].m_strChannel.c_str());
		}

	    found = true;
		m_channels_radio_tmp.erase(m_channels_radio_tmp.begin()+j);
		break;
	  }
	}

	if (!found)
	{
	  CLog::Log(LOGINFO,"PVRManager: Removing Radio channel %s (no more present)", m_channels_radio[i].m_strChannel.c_str());
      m_database.RemoveChannel(m_currentClientID, m_channels_radio[i]);
	  m_channels_radio.erase(m_channels_radio.begin()+i);
	  i--;
	}
  }

  /*
   * Now whe add new channels to frontend
   * All entries now present in the temp lists, are new entries
   */
  for (unsigned int i = 0; i < m_channels_tv_tmp.size(); i++)
  {
    m_channels_tv_tmp[i].m_strStatus = "livetv";
    m_channels_tv_tmp[i].m_iIdChannel = m_database.AddChannel(m_currentClientID, m_channels_tv_tmp[i]);
	m_channels_tv.push_back(m_channels_tv_tmp[i]);
	CLog::Log(LOGINFO,"PVRManager: Added TV channel %s", m_channels_tv_tmp[i].m_strChannel.c_str());
  }

  for (unsigned int i = 0; i < m_channels_radio_tmp.size(); i++)
  {
    m_channels_radio_tmp[i].m_strStatus = "livetv";
    m_channels_radio_tmp[i].m_iIdChannel = m_database.AddChannel(m_currentClientID, m_channels_radio_tmp[i]);
	m_channels_radio.push_back(m_channels_radio_tmp[i]);
	CLog::Log(LOGINFO,"PVRManager: Added Radio channel %s", m_channels_radio_tmp[i].m_strChannel.c_str());
  }


  m_database.Close();
  LeaveCriticalSection(&m_critSection);
  return;
}

void CPVRManager::GetRecordings()
{
  if (m_client)
  {
    EnterCriticalSection(&m_critSection);
    m_recordings.erase(m_recordings.begin(), m_recordings.end());

    if (m_client->GetNumRecordings() > 0)
    {
      m_client->GetAllRecordings(&m_recordings);
    }

    LeaveCriticalSection(&m_critSection);
  }

  return;
}

void CPVRManager::SyncInfo()
{
  m_client->GetNumRecordings() > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  m_client->GetNumTimers()     > 0 ? m_hasTimers     = true : m_hasTimers = false;
  m_isRecording = false;

  if (m_hasTimers && m_clientProps.SupportTimers)
  {
    CDateTime nextRec = CDateTime(2030, 11, 30, 0, 0, 0); //future...

    for (unsigned int i = 0; i < m_timers.size(); ++i)
    {
      CLog::Log(LOGDEBUG, "%s - PVR: nextRec is '%s' current timer starts at %s ", __FUNCTION__,
                nextRec.GetAsLocalizedDateTime(false, false).c_str(),
                m_timers[i].m_StartTime.GetAsLocalizedDateTime(false, false).c_str());

      if (nextRec > m_timers[i].m_StartTime)
      {
        CLog::Log(LOGDEBUG, "%s - PVR: found earlier timer: timer %i GOOD!", __FUNCTION__, i);
        nextRec = m_timers[i].m_StartTime;

        m_nextRecordingTitle    = m_timers[i].m_strTitle;
        m_nextRecordingChannel  = GetNameForChannel(m_timers[i].m_channelNum);
        m_nextRecordingDateTime = nextRec.GetAsLocalizedDateTime(false, false);

        if (m_timers[i].m_recStatus == true)
        {
          m_isRecording = true;
          CLog::Log(LOGDEBUG, "%s - PVR: next timer is currently recording", __FUNCTION__);
        }
        else
        {
          m_isRecording = false;
        }
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s - PVR: trying to find next timer: timer %i is starting later than others.", __FUNCTION__, i);
      }
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

CTVChannelInfoTag *CPVRManager::GetChannelByNumber(int Number, bool radio, int SkipGap)
{
  CTVChannelInfoTag *previous = NULL;

  if (!radio)
  {
    for (unsigned int channel = 0; channel < m_channels_tv.size(); channel++)
    {
      if (m_channels_tv[channel].m_radio != radio)
        continue;

      if (m_channels_tv[channel].m_iChannelNum == Number)
      {
        return &m_channels_tv[channel];
      }
      else if (SkipGap && m_channels_tv[channel].m_iChannelNum > Number)
      {
        return SkipGap > 0 ? &m_channels_tv[channel] : previous;
      }

      previous = &m_channels_tv[channel];
    }
  }
  else
  {
    for (unsigned int channel = 0; channel < m_channels_radio.size(); channel++)
    {
      if (m_channels_radio[channel].m_radio != radio)
        continue;

      if (m_channels_radio[channel].m_iChannelNum == Number)
      {
        return &m_channels_radio[channel];
      }
      else if (SkipGap && m_channels_radio[channel].m_iChannelNum > Number)
      {
        return SkipGap > 0 ? &m_channels_radio[channel] : previous;
      }

      previous = &m_channels_radio[channel];
    }
  }

  return NULL;
}

CTVChannelInfoTag *CPVRManager::GetChannelByID(int Id, bool radio, int SkipGap)
{
  CTVChannelInfoTag *previous = NULL;

  if (!radio)
  {
    for (unsigned int channel = 0; channel < m_channels_tv.size(); channel++)
    {

      if (m_channels_tv[channel].m_radio != radio)
        continue;

      if (m_channels_tv[channel].m_iIdChannel == Id)
      {
        return &m_channels_tv[channel];
      }
      else if (SkipGap && m_channels_tv[channel].m_iIdChannel > Id)
      {
        return SkipGap > 0 ? &m_channels_tv[channel] : previous;
      }

      previous = &m_channels_tv[channel];
    }
  }
  else
  {
    for (unsigned int channel = 0; channel < m_channels_radio.size(); channel++)
    {

      if (m_channels_radio[channel].m_radio != radio)
        continue;

      if (m_channels_radio[channel].m_iIdChannel == Id)
      {
        return &m_channels_radio[channel];
      }
      else if (SkipGap && m_channels_radio[channel].m_iIdChannel > Id)
      {
        return SkipGap > 0 ? &m_channels_radio[channel] : previous;
      }

      previous = &m_channels_radio[channel];
    }
  }

  return NULL;
}

void CPVRManager::SetMusicInfoTag(CFileItem& item, unsigned int channel)
{
  int duration;
  channel--;
  duration =  m_channels_radio[channel].m_duration.GetDays()*60*60*24;
  duration += m_channels_radio[channel].m_duration.GetHours()*60*60;
  duration += m_channels_radio[channel].m_duration.GetMinutes()*60;
  duration += m_channels_radio[channel].m_duration.GetSeconds();

  item.GetMusicInfoTag()->SetURL(m_channels_radio[channel].m_strFileNameAndPath);
  item.GetMusicInfoTag()->SetTitle(m_channels_radio[channel].m_strTitle);
  item.GetMusicInfoTag()->SetArtist(m_channels_radio[channel].m_strChannel);
//    item.GetMusicInfoTag()->SetAlbum(m_channels_radio[channel].m_strBouquet);
  item.GetMusicInfoTag()->SetAlbumArtist(m_channels_radio[channel].m_strChannel);
  item.GetMusicInfoTag()->SetGenre(m_channels_radio[channel].m_strGenre);
  item.GetMusicInfoTag()->SetDuration(duration);
  item.GetMusicInfoTag()->SetLoaded(true);
  item.GetMusicInfoTag()->SetComment("");
  item.GetMusicInfoTag()->SetLyrics("");
  return;
}

void CPVRManager::SetCurrentPlayingProgram()
{
  if (m_isPlayingTV)
  {
    if (m_channels_tv[m_CurrentTVChannel-1].m_EPG.size() > 0 && m_clientProps.SupportEPG)
    {
      CTVEPGInfoTag epgnow(NULL);
      CTVEPGInfoTag epgnext(NULL);
      m_channels_tv[m_CurrentTVChannel-1].GetEPGNextInfo(&epgnext);

      if (m_channels_tv[m_CurrentTVChannel-1].GetEPGNowInfo(&epgnow))
      {
        m_channels_tv[m_CurrentTVChannel-1].m_strTitle          = epgnow.m_strTitle;
        m_channels_tv[m_CurrentTVChannel-1].m_strOriginalTitle  = epgnow.m_strTitle;
        m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline    = epgnow.m_strPlotOutline;
        m_channels_tv[m_CurrentTVChannel-1].m_strPlot           = epgnow.m_strPlot;
        m_channels_tv[m_CurrentTVChannel-1].m_strGenre          = epgnow.m_strGenre;
        m_channels_tv[m_CurrentTVChannel-1].m_startTime         = epgnow.m_startTime;
        m_channels_tv[m_CurrentTVChannel-1].m_endTime           = epgnow.m_endTime;
        m_channels_tv[m_CurrentTVChannel-1].m_duration          = epgnow.m_duration;
        m_channels_tv[m_CurrentTVChannel-1].m_strNextTitle      = epgnext.m_strTitle;

        if (m_channels_tv[m_CurrentTVChannel-1].m_strPlot.Left(m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline.length()) != m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline && !m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline.IsEmpty())
          m_channels_tv[m_CurrentTVChannel-1].m_strPlot = m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline + '\n' + m_channels_tv[m_CurrentTVChannel-1].m_strPlot;

        CDateTimeSpan span = m_channels_tv[m_CurrentTVChannel-1].m_startTime - m_channels_tv[m_CurrentTVChannel-1].m_endTime;

        StringUtils::SecondsToTimeString(span.GetSeconds()
                                         + span.GetMinutes() * 60.
                                         + span.GetHours() * 3600, m_channels_tv[m_CurrentTVChannel-1].m_strRuntime, TIME_FORMAT_GUESS);
      }
      else
      {
        m_channels_tv[m_CurrentTVChannel-1].m_strTitle          = g_localizeStrings.Get(18074);
        m_channels_tv[m_CurrentTVChannel-1].m_strOriginalTitle  = g_localizeStrings.Get(18074);
        m_channels_tv[m_CurrentTVChannel-1].m_strPlotOutline    = "";
        m_channels_tv[m_CurrentTVChannel-1].m_strPlot           = "";
        m_channels_tv[m_CurrentTVChannel-1].m_strGenre          = "";
        m_channels_tv[m_CurrentTVChannel-1].m_startTime         = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0);
        m_channels_tv[m_CurrentTVChannel-1].m_endTime           = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0);
        m_channels_tv[m_CurrentTVChannel-1].m_duration          = CDateTimeSpan(0, 1, 0, 0);
      }

      m_channels_tv[m_CurrentTVChannel-1].m_strAlbum = m_channels_tv[m_CurrentTVChannel-1].m_strChannel;

      m_channels_tv[m_CurrentTVChannel-1].m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
      m_channels_tv[m_CurrentTVChannel-1].m_iEpisode = 0;
      m_channels_tv[m_CurrentTVChannel-1].m_strShowTitle.Format("%i", m_channels_tv[m_CurrentTVChannel-1].m_iChannelNum);
    }
  }
  else if (m_isPlayingRadio)
  {
    if (m_channels_radio[m_CurrentRadioChannel-1].m_EPG.size() > 0 && m_clientProps.SupportEPG)
    {
      CTVEPGInfoTag epgnow(NULL);
      CTVEPGInfoTag epgnext(NULL);
      m_channels_tv[m_CurrentRadioChannel-1].GetEPGNextInfo(&epgnext);

      if (m_channels_radio[m_CurrentRadioChannel-1].GetEPGNowInfo(&epgnow))
      {
        m_channels_radio[m_CurrentRadioChannel-1].m_strTitle          = epgnow.m_strTitle;
        m_channels_radio[m_CurrentRadioChannel-1].m_strOriginalTitle  = epgnow.m_strTitle;
        m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline    = epgnow.m_strPlotOutline;
        m_channels_radio[m_CurrentRadioChannel-1].m_strPlot           = epgnow.m_strPlot;
        m_channels_radio[m_CurrentRadioChannel-1].m_strGenre          = epgnow.m_strGenre;
        m_channels_radio[m_CurrentRadioChannel-1].m_startTime         = epgnow.m_startTime;
        m_channels_radio[m_CurrentRadioChannel-1].m_endTime           = epgnow.m_endTime;
        m_channels_radio[m_CurrentRadioChannel-1].m_duration          = epgnow.m_duration;
        m_channels_radio[m_CurrentRadioChannel-1].m_strNextTitle      = epgnext.m_strTitle;

        if (m_channels_radio[m_CurrentRadioChannel-1].m_strPlot.Left(m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline.length()) != m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline && !m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline.IsEmpty())
          m_channels_radio[m_CurrentRadioChannel-1].m_strPlot = m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline + '\n' + m_channels_radio[m_CurrentRadioChannel-1].m_strPlot;

        CDateTimeSpan span = m_channels_radio[m_CurrentRadioChannel-1].m_startTime - m_channels_radio[m_CurrentRadioChannel-1].m_endTime;

        StringUtils::SecondsToTimeString(span.GetSeconds()
                                         + span.GetMinutes() * 60.
                                         + span.GetHours() * 3600, m_channels_radio[m_CurrentRadioChannel-1].m_strRuntime, TIME_FORMAT_GUESS);
      }
      else
      {
        m_channels_radio[m_CurrentRadioChannel-1].m_strTitle          = g_localizeStrings.Get(18074);
        m_channels_radio[m_CurrentRadioChannel-1].m_strOriginalTitle  = g_localizeStrings.Get(18074);
        m_channels_radio[m_CurrentRadioChannel-1].m_strPlotOutline    = "";
        m_channels_radio[m_CurrentRadioChannel-1].m_strPlot           = "";
        m_channels_radio[m_CurrentRadioChannel-1].m_strGenre          = "";
        m_channels_radio[m_CurrentRadioChannel-1].m_startTime         = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 0, 0, 0)-CDateTimeSpan(0, 1, 0, 0);
        m_channels_radio[m_CurrentRadioChannel-1].m_endTime           = CDateTime::GetCurrentDateTime()+CDateTimeSpan(0, 23, 0, 0);
        m_channels_radio[m_CurrentRadioChannel-1].m_duration          = CDateTimeSpan(0, 1, 0, 0);
      }

      m_channels_radio[m_CurrentRadioChannel-1].m_strAlbum = m_channels_radio[m_CurrentRadioChannel-1].m_strChannel;

      m_channels_radio[m_CurrentRadioChannel-1].m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
      m_channels_radio[m_CurrentRadioChannel-1].m_iEpisode = 0;
      m_channels_radio[m_CurrentRadioChannel-1].m_strShowTitle.Format("%i", m_channels_radio[m_CurrentRadioChannel-1].m_iChannelNum);
    }
  }
}

void CPVRManager::SaveVideoSettings(unsigned int channel_id)
{
  if (g_stSettings.m_currentVideoSettings != g_stSettings.m_defaultVideoSettings)
  {
    CTVDatabase dbs;
    dbs.Open();
    dbs.SetChannelSettings(m_currentClientID, channel_id, g_stSettings.m_currentVideoSettings);
    dbs.Close();
  }
}

void CPVRManager::LoadVideoSettings(unsigned int channel_id, bool update)
{
  CTVDatabase dbs;
  dbs.Open();

  if (update)
  {
    CVideoSettings m_savedVideoSettings;

    if (dbs.GetChannelSettings(m_currentClientID, channel_id, m_savedVideoSettings))
    {

      if (m_savedVideoSettings.m_AudioDelay != g_stSettings.m_currentVideoSettings.m_AudioDelay)
      {
        g_stSettings.m_currentVideoSettings.m_AudioDelay = m_savedVideoSettings.m_AudioDelay;

        if (g_application.m_pPlayer)
          g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
      }

      if (m_savedVideoSettings.m_AudioStream != g_stSettings.m_currentVideoSettings.m_AudioStream)
      {
        g_stSettings.m_currentVideoSettings.m_AudioStream = m_savedVideoSettings.m_AudioStream;

        // only change the audio stream if a different one has been asked for
        if (g_application.m_pPlayer->GetAudioStream() != g_stSettings.m_currentVideoSettings.m_AudioStream)
        {
          g_application.m_pPlayer->SetAudioStream(g_stSettings.m_currentVideoSettings.m_AudioStream);    // Set the audio stream to the one selected
        }
      }

      if (m_savedVideoSettings.m_Brightness != g_stSettings.m_currentVideoSettings.m_Brightness ||
          m_savedVideoSettings.m_Contrast != g_stSettings.m_currentVideoSettings.m_Contrast ||
          m_savedVideoSettings.m_Gamma != g_stSettings.m_currentVideoSettings.m_Gamma)
      {

        g_stSettings.m_currentVideoSettings.m_AudioStream = m_savedVideoSettings.m_AudioStream;
        g_stSettings.m_currentVideoSettings.m_Contrast = m_savedVideoSettings.m_Contrast;
        g_stSettings.m_currentVideoSettings.m_Gamma = m_savedVideoSettings.m_Gamma;

        CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
      }

      if (m_savedVideoSettings.m_NonInterleaved != g_stSettings.m_currentVideoSettings.m_NonInterleaved)
      {
        g_stSettings.m_currentVideoSettings.m_NonInterleaved = m_savedVideoSettings.m_NonInterleaved;
      }

      if (m_savedVideoSettings.m_NoCache != g_stSettings.m_currentVideoSettings.m_NoCache)
      {
        g_stSettings.m_currentVideoSettings.m_NoCache = m_savedVideoSettings.m_NoCache;
      }

      if (m_savedVideoSettings.m_Crop != g_stSettings.m_currentVideoSettings.m_Crop)
      {
        g_stSettings.m_currentVideoSettings.m_Crop = m_savedVideoSettings.m_Crop;
        g_renderManager.AutoCrop(g_stSettings.m_currentVideoSettings.m_Crop);
      }

      if (m_savedVideoSettings.m_CropLeft != g_stSettings.m_currentVideoSettings.m_CropLeft)
      {
        g_stSettings.m_currentVideoSettings.m_CropLeft = m_savedVideoSettings.m_CropLeft;
      }

      if (m_savedVideoSettings.m_CropRight != g_stSettings.m_currentVideoSettings.m_CropRight)
      {
        g_stSettings.m_currentVideoSettings.m_CropRight = m_savedVideoSettings.m_CropRight;
      }

      if (m_savedVideoSettings.m_CropTop != g_stSettings.m_currentVideoSettings.m_CropTop)
      {
        g_stSettings.m_currentVideoSettings.m_CropTop = m_savedVideoSettings.m_CropTop;
      }

      if (m_savedVideoSettings.m_CropBottom != g_stSettings.m_currentVideoSettings.m_CropBottom)
      {
        g_stSettings.m_currentVideoSettings.m_CropBottom = m_savedVideoSettings.m_CropBottom;
      }

      if (m_savedVideoSettings.m_InterlaceMethod != g_stSettings.m_currentVideoSettings.m_InterlaceMethod)
      {
        g_stSettings.m_currentVideoSettings.m_InterlaceMethod = m_savedVideoSettings.m_InterlaceMethod;
      }

      if (m_savedVideoSettings.m_VolumeAmplification != g_stSettings.m_currentVideoSettings.m_VolumeAmplification)
      {
        g_stSettings.m_currentVideoSettings.m_VolumeAmplification = m_savedVideoSettings.m_VolumeAmplification;

        if (g_application.m_pPlayer)
          g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_stSettings.m_currentVideoSettings.m_VolumeAmplification * 100));
      }

      if (m_savedVideoSettings.m_OutputToAllSpeakers != g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers)
      {
        g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers = m_savedVideoSettings.m_OutputToAllSpeakers;
      }

      if (m_savedVideoSettings.m_CustomZoomAmount != g_stSettings.m_currentVideoSettings.m_CustomZoomAmount)
      {
        g_stSettings.m_currentVideoSettings.m_CustomZoomAmount = m_savedVideoSettings.m_CustomZoomAmount;
      }

      if (m_savedVideoSettings.m_CustomPixelRatio != g_stSettings.m_currentVideoSettings.m_CustomPixelRatio)
      {
        g_stSettings.m_currentVideoSettings.m_CustomPixelRatio = m_savedVideoSettings.m_CustomPixelRatio;
      }

      if (m_savedVideoSettings.m_ViewMode != g_stSettings.m_currentVideoSettings.m_ViewMode)
      {
        g_stSettings.m_currentVideoSettings.m_ViewMode = m_savedVideoSettings.m_ViewMode;

        g_renderManager.SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
        g_stSettings.m_currentVideoSettings.m_CustomZoomAmount = g_stSettings.m_fZoomAmount;
        g_stSettings.m_currentVideoSettings.m_CustomPixelRatio = g_stSettings.m_fPixelRatio;
      }

      if (m_savedVideoSettings.m_SubtitleDelay != g_stSettings.m_currentVideoSettings.m_SubtitleDelay)
      {
        g_stSettings.m_currentVideoSettings.m_SubtitleDelay = m_savedVideoSettings.m_SubtitleDelay;

        g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
      }

      if (m_savedVideoSettings.m_SubtitleOn != g_stSettings.m_currentVideoSettings.m_SubtitleOn)
      {
        g_stSettings.m_currentVideoSettings.m_SubtitleOn = m_savedVideoSettings.m_SubtitleOn;

        g_application.m_pPlayer->SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
      }

      if (m_savedVideoSettings.m_SubtitleStream != g_stSettings.m_currentVideoSettings.m_SubtitleStream)
      {
        g_stSettings.m_currentVideoSettings.m_SubtitleStream = m_savedVideoSettings.m_SubtitleStream;

        g_application.m_pPlayer->SetSubtitle(g_stSettings.m_currentVideoSettings.m_SubtitleStream);
      }
    }
  }
  else
  {
    dbs.GetChannelSettings(m_currentClientID, channel_id, g_stSettings.m_currentVideoSettings);
  }

  dbs.Close();
}
