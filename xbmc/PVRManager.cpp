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
  m_bConnectionLost       = false;
  m_CurrentTVChannel      = 1;
  m_CurrentRadioChannel   = 1;
  m_CurrentChannelID      = -1;
  m_HiddenChannels        = 0;
  m_CurrentGroupID        = -1;
  m_synchronized     	  = false;
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
  /* If a client is already started, close it first */
  if (m_client)
    delete m_client;

  /* Check if TV is enabled under Settings->Video->TV->Enable */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVR: PVRManager starting");

  /* Discover and load chosen client */
  m_client = LoadClient();
  if (!m_client)
  {
    CLog::Log(LOGERROR, "PVR: couldn't load client");
    return;
  }

  /* now that client have been initialized, we check connectivity */
  if (!ConnectClient())
  {
    CLog::Log(LOGERROR, "PVR: couldn't connect client");
    return;
  }

  /* Add client to TV-Database to identify different backend servers,
   * if client is already added his id is given.
   * Backend name have the following format: "NAME-IP:PORT", as example "VDR-192.168.0.120:2004"
   */
  m_database.Open();
  CStdString ClientName;
  ClientName.Format("%s-%s:%u", m_clientProps.Name, m_clientProps.Hostname, m_clientProps.Port);
  m_currentClientID = m_database.AddClient(ClientName);

  /* Get channels if present from Database, otherwise load it from client
   * and add it to database
   */
  if (m_database.GetNumChannels(m_currentClientID) > 0)
  {
    m_database.GetChannelList(m_currentClientID, &m_channels_tv, false);
    m_database.GetChannelList(m_currentClientID, &m_channels_radio, true);
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
    GetTimers();

  SyncInfo();
  m_database.Close();

  Create();
  SetName("PVRManager Updater");
  SetPriority(-15);

  CLog::Log(LOGNOTICE, "PVR: PVRManager started. Client '%s' loaded with Id: %i", ClientName.c_str(), m_currentClientID);
  return;
}

void CPVRManager::Stop()
{
  CLog::Log(LOGNOTICE, "PVR: PVRManager stoping");
  StopThread();

  if (m_client)
  {
    DisconnectClient();
    delete m_client;
    m_client = NULL;
  }

  m_bConnectionLost       = false;
  m_CurrentTVChannel      = 1;
  m_CurrentRadioChannel   = 1;
  m_CurrentChannelID      = -1;
  m_HiddenChannels        = 0;
  m_CurrentGroupID        = -1;
  m_synchronized     	  = false;

  return;
}

IPVRClient* CPVRManager::LoadClient()
{
  VECADDONS *addons;

  /* call update */
  addons = g_settings.GetAddonsFromType(ADDON_PVRDLL);

  if (addons == NULL || addons->empty())
    return false;

  CPVRClientFactory factory;
  for (unsigned i=0; i<addons->size(); i++)
  {
    CAddon &clientAddon = addons->at(i);

    if (clientAddon.m_disabled) // ignore disabled addons
      continue;

    IPVRClient *client = NULL;
    client = factory.LoadPVRClient((clientAddon.m_strPath + clientAddon.m_strLibName), i, this);
    if (client)
    {
      /* Transmit current unified user settings to the PVR Addon */
      CAddonSettings settings;
      settings.Load(clientAddon.m_strPath);

      TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
      while (setting)
      {
        const char *type = setting->Attribute("type");
        const char *id = setting->Attribute("id");
        const char *value = settings.Get(id).c_str();

        if (type)
        {
          if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0)
          {
            client->SetUserSetting(id, value);
          }
          else if (strcmpi(type, "integer") == 0)
          {
            int tmp = atoi(settings.Get(id));
            client->SetUserSetting(id, (int*) &tmp);
          }
          if (strcmpi(type, "bool") == 0)
          {
            bool tmp = settings.Get(id) == "true" ? true : false;
            client->SetUserSetting(id, (bool*) &tmp);
          }
        }
        setting = setting->NextSiblingElement("setting");
      }

      /* Get Standart client settings and features */
      if (client->GetProperties(&m_clientProps) != PVR_ERROR_NO_ERROR)
      {
        delete client;
        client = NULL;
        continue;
      }

      /* Currently only one backend is supported, break here*/
      return client;
    }
  }

  return NULL;
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
/** Server access */

bool CPVRManager::ConnectClient()
{
  /* signal client to connect to backend */
  PVR_ERROR err = m_client->Connect();
  if (err == PVR_ERROR_NO_ERROR)
    return true;
  else if (err == PVR_ERROR_SERVER_WRONG_VERSION)
    CGUIDialogOK::ShowAndGetInput(18100, 18091, 18106, 18092);
  else
    CGUIDialogOK::ShowAndGetInput(18100, 18091, 0, 18092);

  CLog::Log(LOGERROR, "PVR: Could't connect client to backend server");
  Stop();
  return false;
}

void CPVRManager::DisconnectClient()
{
  /* signal client to disconnect from backend */
  m_client->Disconnect();

  return;
}

bool CPVRManager::IsConnected()
{
  if (m_client)
  {
    if (m_client->IsUp())
    {
      return true;
    }
  }

//
//  if (m_bConnectionLost)
//  {
//    /* If connection was lost, ask user to reconnect to backend */
//    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
//
//    if (pDialog)
//    {
//      pDialog->SetHeading(18090);
//      pDialog->SetLine(0, 18093);
//      pDialog->SetLine(1, "");
//      pDialog->SetLine(2, 18094);
//      pDialog->DoModal();
//
//      if (!pDialog->IsConfirmed())
//      {
//        return false;
//      }
//    }
//  }
//
//  /* Try to reconnect */
//  Stop();
//
//  Start();
//
//  /* If connection is ok, clear lost flag */
//  if (m_client && m_client->IsUp())
//  {
//    m_bConnectionLost = false;
//    return true;
//  }

  return false;
}

CURL CPVRManager::GetConnString()
{
  CURL connString;

  /* set client connection string, for password and username
   * only default values are given
   */
  connString.SetHostName(m_clientProps.Hostname);
  connString.SetUserName(m_clientProps.DefaultUser);
  connString.SetPassword(m_clientProps.DefaultPassword);
  connString.SetPort(m_clientProps.Port);

  return connString;
}


/************************************************************/
/** Event handling */

const char* CPVRManager::TranslateInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_NOW_RECORDING_TITLE)     return m_nowRecordingTitle;
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)   return m_nowRecordingChannel;
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)  return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return m_nextRecordingChannel;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;

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
	    GetTimers();
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
/** General handling */

CStdString CPVRManager::GetBackendName()
{
  if (m_client)
    return m_client->GetBackendName();

  return "";
}

CStdString CPVRManager::GetBackendVersion()
{
  if (m_client)
    return m_client->GetBackendVersion();

  return "";
}

bool CPVRManager::GetDriveSpace(long long *total, long long *used, int *percent)
{
  if (m_client && m_clientProps.SupportRecordings)
  {
    if (m_client->GetDriveSpace(total, used) == PVR_ERROR_NO_ERROR)
    {
      *percent = (int)(((float) * used / (float) * total) * 100);
      return true;
    }
  }

  *total = 1024;
  *used = 1024;
  *percent = 100;
  return false;
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
      GetFrontendChannelNumber(CurrentClientChannel, &m_CurrentTVChannel, NULL);
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
      GetFrontendChannelNumber(CurrentClientChannel, &m_CurrentRadioChannel, NULL);
      CFileItemPtr channel(new CFileItem(m_channels_radio[m_CurrentRadioChannel]));
      g_application.PlayFile(*channel);
    }
  }
  m_database.Close();

  /* Synchronize channel numbers inside timers */
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    GetFrontendChannelNumber(m_timers[i].m_clientNum, &m_timers[i].m_channelNum, &m_timers[i].m_Radio);
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
      m_database.RemoveEPGEntries(m_currentClientID, m_channels_tv[number-1].m_iIdChannel, NULL, NULL);
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
      m_database.RemoveEPGEntries(m_currentClientID, m_channels_radio[number-1].m_iIdChannel, NULL, NULL);
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

bool CPVRManager::GetFrontendChannelNumber(unsigned int client_no, int *frontend_no, bool *isRadio)
{
  for (unsigned int i = 0; i < m_channels_tv.size(); i++)
  {
    if (m_channels_tv[i].m_iClientNum == client_no)
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
      LostConnection();
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
      LostConnection();
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

  GetTimers();

  for (unsigned int i = 0; i < m_timers.size(); ++i)
  {
    CFileItemPtr timer(new CFileItem(m_timers[i]));
    results->Add(timer);
  }

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
      LostConnection();
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
  if (m_client && m_clientProps.SupportTimers)
  {
    /* Check if a CTVTimerInfoTag is inside file item */
    if (!item.IsTVTimer())
    {
      CLog::Log(LOGERROR, "CPVRManager: UpdateTimer no TVInfoTag given!");
      return false;
    }

    /* and write it to the backend */
    PVR_ERROR err = m_client->UpdateTimer(*item.GetTVTimerInfoTag());

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
    return m_client->ReadRecordedStream(buf, buf_size);
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

void CPVRManager::GetTimers()
{
  if (m_client)
  {
    EnterCriticalSection(&m_critSection);
    m_timers.erase(m_timers.begin(), m_timers.end());

    if (m_client->GetNumTimers() > 0)
    {
      m_client->GetAllTimers(&m_timers);

      for (unsigned int i = 0; i < m_timers.size(); i++)
      {
        GetFrontendChannelNumber(m_timers[i].m_clientNum, &m_timers[i].m_channelNum, &m_timers[i].m_Radio);
        m_timers[i].m_strChannel = GetNameForChannel(m_timers[i].m_channelNum, m_timers[i].m_Radio);
      }
    }

    LeaveCriticalSection(&m_critSection);
  }

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
    CDateTime nextRec;

    for (unsigned int i = 0; i < m_timers.size(); ++i)
    {
      if (nextRec > m_timers[i].m_StartTime)
      {
        nextRec = m_timers[i].m_StartTime;

        m_nextRecordingTitle    = m_timers[i].m_strTitle;
        m_nextRecordingChannel  = GetNameForChannel(m_timers[i].m_channelNum);
        m_nextRecordingDateTime = nextRec.GetAsLocalizedDateTime(false, false);

        if (m_timers[i].m_recStatus == true)
        {
          m_isRecording = true;
        }
        else
        {
          m_isRecording = false;
        }
      }
    }
  }

  if (m_isRecording && m_clientProps.SupportRecordings)
  {
    m_nowRecordingTitle = m_nextRecordingTitle;
    m_nowRecordingDateTime = m_nextRecordingDateTime;
    m_nowRecordingChannel = m_nextRecordingChannel;
  }
  else
  {
    m_nowRecordingTitle.clear();
    m_nowRecordingDateTime.clear();
    m_nowRecordingChannel.clear();
  }
}

void CPVRManager::LostConnection()
{
  /* Set lost flag */
  m_bConnectionLost = true;

  /* And inform the user about the lost connection */
  CGUIDialogOK::ShowAndGetInput(18090,0,18093,0);
  return;
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
