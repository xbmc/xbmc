/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "Application.h"
#include "settings/GUISettings.h"
#include "Util.h"
#include "guilib/GUIWindowManager.h"
#include "GUIInfoManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/File.h"
#include "StringUtils.h"
#include "utils/TimeUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"

/* GUI Messages includes */
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"

#include "PVRManager.h"
#include "PVRChannelGroupsContainer.h"
#include "epg/PVREpgInfoTag.h"
#include "PVRTimerInfoTag.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;

#define INFO_TOGGLE_TIME 1500

CPVRManager::CPVRManager()
{
  m_bFirstStart              = true;
  m_bLoaded                  = false;
  m_bTriggerChannelsUpdate   = false;
  m_bTriggerRecordingsUpdate = false;
  m_bTriggerTimersUpdate     = false;
}

CPVRManager::~CPVRManager()
{
  Stop();
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

void CPVRManager::Start()
{
  /* first stop and remove any clients */
  Stop();

  /* don't start if Settings->Video->TV->Enable isn't checked */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVRManager - starting up");

  ResetProperties();

  /* discover, load and create chosen client addons */
  if (!LoadClients())
  {
    CLog::Log(LOGERROR, "PVRManager - couldn't load any clients");
  }
  else
  {
    /* create the supervisor thread to do all background activities */
    Create();
    SetName("XBMC PVRManager");
    SetPriority(-15);
    CLog::Log(LOGNOTICE, "PVRManager - started with %u active clients", m_clients.size());
  }
}

void CPVRManager::Stop()
{
  /* nothing to stop if we haven't started */
  if (m_clients.empty())
    return;

  CLog::Log(LOGNOTICE, "PVRManager - stopping");
  if (m_currentPlayingRecording || m_currentPlayingChannel)
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping PVR playback", __FUNCTION__);
    g_application.StopPlaying();
  }

  StopThreads();
  Cleanup();
}

bool CPVRManager::LoadClients()
{
  CAddonMgr::Get().RegisterAddonMgrCallback(ADDON_PVRDLL, this);

  /* get all PVR addons */
  VECADDONS addons;
  if (!CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true))
    return false;

  /* load and initialise the clients */
  if (!m_database.Open())
    return false;

  m_clientsProps.clear();
  for (unsigned iClientPtr = 0; iClientPtr < addons.size(); iClientPtr++)
  {
    const AddonPtr clientAddon = addons.at(iClientPtr);
    if (!clientAddon->Enabled())
      continue;

    /* add this client to the database if it's not in there yet */
    int iClientId = m_database.AddClient(clientAddon->Name(), clientAddon->ID());
    if (iClientId == -1)
    {
      CLog::Log(LOGERROR, "PVRManager - %s - can't add client '%s' to the database",
          __FUNCTION__, clientAddon->Name().c_str());
      continue;
    }

    /* load and initialise the client libraries */
    boost::shared_ptr<CPVRClient> addon = boost::dynamic_pointer_cast<CPVRClient>(clientAddon);
    if (addon && addon->Create(iClientId, this))
    {
      m_clients.insert(std::make_pair(iClientId, addon));

      /* get the client's properties */
      PVR_SERVERPROPS props;
      if (addon->GetProperties(&props) == PVR_ERROR_NO_ERROR)
        m_clientsProps.insert(std::make_pair(iClientId, props));
    }
    else
    {
      CLog::Log(LOGERROR, "PVRManager - %s - can't initialise client '%s'",
          __FUNCTION__, clientAddon->Name().c_str());
    }
  }

  m_database.Close();

  return !m_clients.empty();
}

unsigned int CPVRManager::GetFirstClientID()
{
  CLIENTMAPITR itr = m_clients.begin();
  return m_clients[(*itr).first]->GetID();
}

bool CPVRManager::RequestRestart(AddonPtr addon, bool bDataChanged)
{
  return StopClient(addon, true);
}

bool CPVRManager::RequestRemoval(AddonPtr addon)
{
  return StopClient(addon, false);
}

bool CPVRManager::StopClient(AddonPtr client, bool bRestart)
{
  bool bReturn = false;
  if (!client)
    return bReturn;

  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->ID() == client->ID())
    {
      CLog::Log(LOGINFO, "PVRManager - %s - %s client '%s'",
          __FUNCTION__, bRestart ? "restarting" : "removing", m_clients[(*itr).first]->Name().c_str());

      StopThreads();
      if (bRestart)
        m_clients[(*itr).first]->ReCreate();
      else
        m_clients[(*itr).first]->Destroy();
      StartThreads();

      bReturn = true;
      break;
    }
    itr++;
  }

  return bReturn;
}

void CPVRManager::StartThreads()
{
  if (g_PVREpgContainer)
    g_PVREpgContainer.Create();

  Create();
}

void CPVRManager::StopThreads()
{
  if (g_PVREpgContainer)
    g_PVREpgContainer.StopThread();

  StopThread();
}

void CPVRManager::UpdateWindow(PVRWindow window)
{
  CGUIWindowPVR *pTVWin = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
  if (pTVWin)
    pTVWin->UpdateData(window);
}

void CPVRManager::OnClientMessage(const int iClientId, const PVR_EVENT clientEvent, const char *strMessage)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  CStdString clientName = m_clients[iClientId]->GetBackendName() + ":" + m_clients[iClientId]->GetConnectionString();
  switch (clientEvent)
  {
    case PVR_EVENT_TIMERS_CHANGE:
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - timers changed on client '%d'",
          __FUNCTION__, iClientId);
      m_bTriggerTimersUpdate = true;
    }
    break;

    case PVR_EVENT_RECORDINGS_CHANGE:
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - recording list changed on client '%d'",
          __FUNCTION__, iClientId);
      m_bTriggerRecordingsUpdate = true;
    }
    break;

    case PVR_EVENT_CHANNELS_CHANGE:
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - channel list changed on client '%d'",
          __FUNCTION__, iClientId);
      m_bTriggerChannelsUpdate = true;
    }
    break;

    default:
      CLog::Log(LOGWARNING, "PVRManager - %s - client '%d' sent unknown event '%s'",
          __FUNCTION__, iClientId, strMessage);
      break;
  }
}

void CPVRManager::ResetProperties(void)
{
  m_hasRecordings            = false;
  m_isRecording              = false;
  m_hasTimers                = false;
  m_bTriggerChannelsUpdate   = false;
  m_bTriggerRecordingsUpdate = false;
  m_bTriggerTimersUpdate     = false;
  m_CurrentGroupID           = -1;
  m_currentPlayingChannel    = NULL;
  m_currentPlayingRecording  = NULL;
  m_PreviousChannel[0]       = -1;
  m_PreviousChannel[1]       = -1;
  m_PreviousChannelIndex     = 0;
  m_infoToggleStart          = NULL;
  m_infoToggleCurrent        = 0;
  m_recordingToggleStart     = NULL;
  m_recordingToggleCurrent   = 0;
  m_LastChannel              = 0;
  m_bChannelScanRunning      = false;
}

void CPVRManager::UpdateTimers(void)
{
  CLog::Log(LOGDEBUG, "PVRManager - %s - updating timers", __FUNCTION__);

  g_PVRTimers.Update();
  UpdateRecordingsCache();
  UpdateWindow(PVR_WINDOW_TIMERS);

  m_bTriggerTimersUpdate = false;
}

void CPVRManager::UpdateRecordings(void)
{
  CLog::Log(LOGDEBUG, "PVRManager - %s - updating recordings list", __FUNCTION__);

  PVRRecordings.Update(true);
  UpdateRecordingsCache();
  UpdateWindow(PVR_WINDOW_RECORDINGS);

  m_bTriggerRecordingsUpdate = false;
}

void CPVRManager::UpdateChannels(void)
{
  CLog::Log(LOGDEBUG, "PVRManager - %s - updating channel list", __FUNCTION__);

  g_PVRChannelGroups.Update();
  UpdateRecordingsCache();
  UpdateWindow(PVR_WINDOW_CHANNELS_TV);
  UpdateWindow(PVR_WINDOW_CHANNELS_RADIO);

  m_bTriggerChannelsUpdate = false;
}

bool CPVRManager::ContinueLastChannel()
{
  bool bReturn = false;
  m_bFirstStart = false;

  if (m_database.Open())
  {
    int iLastChannel = m_database.GetLastChannel();
    m_database.Close();

    if (iLastChannel > 0)
    {
      const CPVRChannel *channel = CPVRChannelGroup::GetByChannelIDFromAll(iLastChannel);

      if (channel)
      {
        CLog::Log(LOGNOTICE, "PVRManager - %s - continue playback on channel '%s'",
            __FUNCTION__, channel->ChannelName().c_str());
        bReturn = StartPlayback(channel, (g_guiSettings.GetInt("pvrplayback.startlast") == START_LAST_CHANNEL_MIN));
      }
      else
      {
        CLog::Log(LOGERROR, "PVRManager - %s - cannot continue playback on channel: channel '%d' not found",
            __FUNCTION__, iLastChannel);
      }
    }
  }

  return bReturn;
}

void CPVRManager::Process()
{
  if (!m_bLoaded)
  {
    /* load all channels and groups */
    g_PVRChannelGroups.Load();

    /* start the EPG thread */
    g_PVREpgContainer.Start();

    /* get timers from the backends */
    g_PVRTimers.Load();

    /* get recordings from the backend */
    PVRRecordings.Load();

    /* notify window that all channels and epg are loaded */
    UpdateWindow(PVR_WINDOW_CHANNELS_TV);
    UpdateWindow(PVR_WINDOW_CHANNELS_RADIO);

    m_bLoaded = true;
  }

  /* Continue last watched channel after first startup */
  if (m_bFirstStart && g_guiSettings.GetInt("pvrplayback.startlast") != START_LAST_CHANNEL_OFF)
    ContinueLastChannel();

  /* main loop */
  while (!m_bStop)
  {
    if (m_bTriggerChannelsUpdate)
      UpdateChannels();

    if (m_bTriggerRecordingsUpdate)
      UpdateRecordings();

    if (m_bTriggerTimersUpdate)
      UpdateTimers();

    CSingleLock lock(m_critSection);
    /* Get Signal information of the current playing channel */
    if (m_currentPlayingChannel && g_guiSettings.GetBool("pvrplayback.signalquality") && !m_currentPlayingChannel->GetPVRChannelInfoTag()->IsVirtual())
      m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->SignalQuality(m_qualityInfo);
    lock.Leave();

    Sleep(1000);
  }
}

void CPVRManager::Cleanup(void)
{
  /* stop and clean up the EPG thread */
  g_PVREpgContainer.Stop();

  /* unload the rest */
  PVRRecordings.Unload();
  g_PVRTimers.Unload();
  g_PVRChannelGroups.Unload();
  m_bLoaded = false;

  /* destroy addons */
  for (CLIENTMAPITR itr = m_clients.begin(); itr != m_clients.end(); itr++)
  {
    boost::shared_ptr<CPVRClient> client = m_clients[(*itr).first];
    CLog::Log(LOGDEBUG, "PVRManager - %s - destroying addon '%s' (%s)",
        __FUNCTION__, client->Name().c_str(), client->ID().c_str());

    client->Destroy();
  }

  m_clients.clear();
  m_clientsProps.clear();
}

void CPVRManager::UpdateRecordingsCache(void)
{
  CSingleLock lock(m_critSection);

  m_hasRecordings = PVRRecordings.GetNumRecordings() > 0;
  m_hasTimers = g_PVRTimers.GetNumTimers() > 0;
  m_isRecording = false;
  m_NowRecording.clear();
  m_NextRecording = NULL;

  if (m_hasTimers)
  {
    CDateTime now = CDateTime::GetCurrentDateTime();
    for (unsigned int iTimerPtr = 0; iTimerPtr < g_PVRTimers.size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *timerTag = g_PVRTimers.at(iTimerPtr);
      if (timerTag->Active())
      {
        if (timerTag->Start() <= now && timerTag->Stop() > now)
        {
          m_NowRecording.push_back(timerTag);
          m_isRecording = true;
        }
        if (!m_NextRecording || m_NextRecording->Start() > timerTag->Start())
        {
          m_NextRecording = timerTag;
        }
      }
    }
  }
}

const char *CPVRManager::CharInfoNowRecordingTitle(void)
{
  if (m_recordingToggleStart == 0)
  {
    UpdateRecordingsCache();
    m_recordingToggleStart = CTimeUtils::GetTimeMS();
    m_recordingToggleCurrent = 0;
  }
  else
  {
    if (CTimeUtils::GetTimeMS() - m_recordingToggleStart > INFO_TOGGLE_TIME)
    {
      UpdateRecordingsCache();
      if (m_NowRecording.size() > 0)
      {
        m_recordingToggleCurrent++;
        if (m_recordingToggleCurrent > m_NowRecording.size()-1)
          m_recordingToggleCurrent = 0;

        m_recordingToggleStart = CTimeUtils::GetTimeMS();
      }
    }
  }

  return (m_NowRecording.size() >= m_recordingToggleCurrent + 1) ?
    m_NowRecording[m_recordingToggleCurrent]->Title() :
    "";
}

const char *CPVRManager::CharInfoNowRecordingChannel(void)
{
  static CStdString strReturn = "";

  if (m_NowRecording.size() > 0)
  {
    CPVRTimerInfoTag * timerTag = m_NowRecording[m_recordingToggleCurrent];
    strReturn = timerTag ? timerTag->ChannelName() : "";
  }

  return strReturn;
}

const char *CPVRManager::CharInfoNowRecordingDateTime(void)
{
  static CStdString strReturn = "";

  if (m_NowRecording.size() > 0)
  {
    CPVRTimerInfoTag *timerTag = m_NowRecording[m_recordingToggleCurrent];
    strReturn = timerTag ? timerTag->Start().GetAsLocalizedDateTime(false, false) : "";
  }

  return strReturn;
}

const char *CPVRManager::CharInfoBackendNumber(void)
{
  if (m_infoToggleStart == 0)
  {
    m_infoToggleStart = CTimeUtils::GetTimeMS();
    m_infoToggleCurrent = 0;
  }
  else
  {
    if (CTimeUtils::GetTimeMS() - m_infoToggleStart > INFO_TOGGLE_TIME)
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
          m_backendDiskspace.Format("%s %.1f GByte - %s: %.1f GByte", g_localizeStrings.Get(20161), (float) kBTotal / 1024, g_localizeStrings.Get(20162), (float) kBUsed / 1024);
        }
        else
        {
          m_backendDiskspace = g_localizeStrings.Get(19055);
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
      m_infoToggleStart = CTimeUtils::GetTimeMS();
    }
  }

  static CStdString backendClients;
  if (m_clients.size() > 0)
    backendClients.Format("%u %s %u", m_infoToggleCurrent+1, g_localizeStrings.Get(20163), m_clients.size());
  else
    backendClients = g_localizeStrings.Get(14023);

  return backendClients;
}

const char *CPVRManager::CharInfoTotalDiskSpace(void)
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
  m_totalDiskspace.Format("%s %0.1f GByte - %s: %0.1f GByte", g_localizeStrings.Get(20161), (float) kBTotal / 1024, g_localizeStrings.Get(20162), (float) kBUsed / 1024);
  return m_totalDiskspace;
}

const char *CPVRManager::CharInfoNextTimer(void)
{
  static CStdString strReturn = "";
  CPVRTimerInfoTag *next = g_PVRTimers.GetNextActiveTimer();
  if (next != NULL)
  {
    m_nextTimer.Format("%s %s %s %s", g_localizeStrings.Get(19106),
        next->Start().GetAsLocalizedDate(true),
        g_localizeStrings.Get(19107),
        next->Start().GetAsLocalizedTime("HH:mm", false));
    strReturn = m_nextTimer;
  }

  return strReturn;
}

const char *CPVRManager::CharInfoPlayingDuration(void)
{
  m_playingDuration = StringUtils::SecondsToTimeString(GetTotalTime()/1000, TIME_FORMAT_GUESS);
  return m_playingDuration.c_str();
}

const char *CPVRManager::CharInfoPlayingTime(void)
{
  m_playingTime = StringUtils::SecondsToTimeString(GetStartTime()/1000, TIME_FORMAT_GUESS);
  return m_playingTime.c_str();
}

const char *CPVRManager::CharInfoVideoBR(void)
{
  static CStdString strReturn = "";
  if (m_qualityInfo.video_bitrate > 0)
    strReturn.Format("%.2f Mbit/s", m_qualityInfo.video_bitrate);
  return strReturn;
}

const char *CPVRManager::CharInfoAudioBR(void)
{
  static CStdString strReturn = "";
  if (m_qualityInfo.audio_bitrate > 0)
    strReturn.Format("%.0f kbit/s", m_qualityInfo.audio_bitrate);
  return strReturn;
}

const char *CPVRManager::CharInfoDolbyBR(void)
{
  static CStdString strReturn = "";
  if (m_qualityInfo.dolby_bitrate > 0)
    strReturn.Format("%.0f kbit/s", m_qualityInfo.dolby_bitrate);
  return strReturn;
}

const char *CPVRManager::CharInfoSignal(void)
{
  static CStdString strReturn = "";
  if (m_qualityInfo.signal > 0)
    strReturn.Format("%d %%", m_qualityInfo.signal / 655);
  return strReturn;
}

const char *CPVRManager::CharInfoSNR(void)
{
  static CStdString strReturn = "";
  if (m_qualityInfo.snr > 0)
    strReturn.Format("%d %%", m_qualityInfo.snr / 655);
  return strReturn;
}

const char *CPVRManager::CharInfoBER(void)
{
  static CStdString strReturn;
  strReturn.Format("%08X", m_qualityInfo.ber);
  return strReturn;
}

const char *CPVRManager::CharInfoUNC(void)
{
  static CStdString strReturn;
  strReturn.Format("%08X", m_qualityInfo.unc);
  return strReturn;
}

const char *CPVRManager::CharInfoFrontendName(void)
{
  static CStdString strReturn = m_qualityInfo.frontend_name;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRManager::CharInfoFrontendStatus(void)
{
  static CStdString strReturn = m_qualityInfo.frontend_status;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRManager::CharInfoEncryption(void)
{
  static CStdString strReturn = "";

  if (m_currentPlayingChannel)
    strReturn = m_currentPlayingChannel->GetPVRChannelInfoTag()->EncryptionName();

  return strReturn;
}

const char* CPVRManager::TranslateCharInfo(DWORD dwInfo)
{
  if      (dwInfo == PVR_NOW_RECORDING_TITLE)     return CharInfoNowRecordingTitle();
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)   return CharInfoNowRecordingChannel();
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)  return CharInfoNowRecordingDateTime();
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return m_NextRecording ? m_NextRecording->Title() : "";
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return m_NextRecording ? m_NextRecording->ChannelName() : "";
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_NextRecording ? m_NextRecording->Start().GetAsLocalizedDateTime(false, false) : "";
  else if (dwInfo == PVR_BACKEND_NAME)            return m_backendName;
  else if (dwInfo == PVR_BACKEND_VERSION)         return m_backendVersion;
  else if (dwInfo == PVR_BACKEND_HOST)            return m_backendHost;
  else if (dwInfo == PVR_BACKEND_DISKSPACE)       return m_backendDiskspace;
  else if (dwInfo == PVR_BACKEND_CHANNELS)        return m_backendChannels;
  else if (dwInfo == PVR_BACKEND_TIMERS)          return m_backendTimers;
  else if (dwInfo == PVR_BACKEND_RECORDINGS)      return m_backendRecordings;
  else if (dwInfo == PVR_BACKEND_NUMBER)          return CharInfoBackendNumber();
  else if (dwInfo == PVR_TOTAL_DISKSPACE)         return CharInfoTotalDiskSpace();
  else if (dwInfo == PVR_NEXT_TIMER)              return CharInfoNextTimer();
  else if (dwInfo == PVR_PLAYING_DURATION)        return CharInfoPlayingDuration();
  else if (dwInfo == PVR_PLAYING_TIME)            return CharInfoPlayingTime();
  else if (dwInfo == PVR_ACTUAL_STREAM_VIDEO_BR)  return CharInfoVideoBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_AUDIO_BR)  return CharInfoAudioBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_DOLBY_BR)  return CharInfoDolbyBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG)       return CharInfoSignal();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR)       return CharInfoSNR();
  else if (dwInfo == PVR_ACTUAL_STREAM_BER)       return CharInfoBER();
  else if (dwInfo == PVR_ACTUAL_STREAM_UNC)       return CharInfoUNC();
  else if (dwInfo == PVR_ACTUAL_STREAM_CLIENT)    return m_playingClientName;
  else if (dwInfo == PVR_ACTUAL_STREAM_DEVICE)    return CharInfoFrontendName();
  else if (dwInfo == PVR_ACTUAL_STREAM_STATUS)    return CharInfoFrontendStatus();
  else if (dwInfo == PVR_ACTUAL_STREAM_CRYPTION)  return CharInfoEncryption();
  return "";
}

int CPVRManager::TranslateIntInfo(DWORD dwInfo)
{
  int iReturn = 0;

  if (dwInfo == PVR_PLAYING_PROGRESS)
    iReturn = (float) GetStartTime() / GetTotalTime() * 100;
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG_PROGR)
    iReturn = (float) m_qualityInfo.signal / 0xFFFF * 100;
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR_PROGR)
    iReturn = (float) m_qualityInfo.snr / 0xFFFF * 100;

  return iReturn;
}

bool CPVRManager::TranslateBoolInfo(DWORD dwInfo)
{
  bool bReturn = false;

  if (dwInfo == PVR_IS_RECORDING)
    bReturn = m_isRecording;
  else if (dwInfo == PVR_HAS_TIMER)
    bReturn = m_hasTimers;
  else if (dwInfo == PVR_IS_PLAYING_TV)
    bReturn = IsPlayingTV();
  else if (dwInfo == PVR_IS_PLAYING_RADIO)
    bReturn = IsPlayingRadio();
  else if (dwInfo == PVR_IS_PLAYING_RECORDING)
    bReturn = IsPlayingRecording();
  else if (dwInfo == PVR_ACTUAL_STREAM_ENCRYPTED)
    bReturn = (m_currentPlayingChannel && m_currentPlayingChannel->GetPVRChannelInfoTag()->IsEncrypted());

  return bReturn;
}

void CPVRManager::StartChannelScan(void)
{
  std::vector<long> clients;
  int scanningClientID = -1;
  m_bChannelScanRunning = true;

  /* get clients that support channel scanning */
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->ReadyToUse() && GetClientProperties(m_clients[(*itr).first]->GetID())->SupportChannelScan)
      clients.push_back(m_clients[(*itr).first]->GetID());

    itr++;
  }

  /* multiple clients found */
  if (clients.size() > 1)
  {
    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19119);

    for (unsigned int i = 0; i < clients.size(); i++)
      pDialog->Add(m_clients[clients[i]]->GetBackendName() + ":" + m_clients[clients[i]]->GetConnectionString());

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
      scanningClientID = clients[selection];
  }
  /* one client found */
  else if (clients.size() == 1)
  {
    scanningClientID = clients[0];
  }
  /* no clients found */
  else if (scanningClientID < 0)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19192,0);
    return;
  }

  /* start the channel scan */
  CLog::Log(LOGNOTICE,"PVRManager - %s - starting to scan for channels on client %s:%s",
      __FUNCTION__, m_clients[scanningClientID]->GetBackendName().c_str(), m_clients[scanningClientID]->GetConnectionString().c_str());
  long perfCnt = CTimeUtils::GetTimeMS();

  /* stop the supervisor thread */
  StopThreads();

  /* do the scan */
  if (m_clients[scanningClientID]->StartChannelScan() != PVR_ERROR_NO_ERROR)
    /* an error occured */
    CGUIDialogOK::ShowAndGetInput(19111,0,19193,0);

  /* restart the supervisor thread */
  StartThreads();

  CLog::Log(LOGNOTICE, "PVRManager - %s - channel scan finished after %li.%li seconds",
      __FUNCTION__, (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);
  m_bChannelScanRunning = false;
}

void CPVRManager::ResetDatabase(bool bShowProgress /* = true */)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the PVR database", __FUNCTION__);

  CGUIDialogProgress* pDlgProgress = NULL;

  if (bShowProgress)
  {
    pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pDlgProgress->SetLine(0, "");
    pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
    pDlgProgress->SetLine(2, "");
    pDlgProgress->StartModal();
    pDlgProgress->Progress();
  }

  if (m_currentPlayingRecording || m_currentPlayingChannel)
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping playback", __FUNCTION__);
    g_application.StopPlaying();
  }

  if (bShowProgress)
    pDlgProgress->SetPercentage(10);

  /* stop the thread */
  Stop();
  if (bShowProgress)
    pDlgProgress->SetPercentage(20);

  if (m_database.Open())
  {
    /* clean the EPG database */
    g_PVREpgContainer.Clear(true);
    if (bShowProgress)
      pDlgProgress->SetPercentage(30);

    /* delete all TV channel groups */
    m_database.DeleteChannelGroups(false);
    if (bShowProgress)
      pDlgProgress->SetPercentage(50);

    /* delete all radio channel groups */
    m_database.DeleteChannelGroups(true);
    if (bShowProgress)
      pDlgProgress->SetPercentage(60);

    /* delete all channels */
    m_database.DeleteChannels();
    if (bShowProgress)
      pDlgProgress->SetPercentage(70);

    /* delete all channel settings */
    m_database.DeleteChannelSettings();
    if (bShowProgress)
      pDlgProgress->SetPercentage(80);

    /* delete all client information */
    m_database.DeleteClients();
    if (bShowProgress)
      pDlgProgress->SetPercentage(90);

    m_database.Close();
  }

  CLog::Log(LOGNOTICE,"PVRManager - %s - PVR database cleared. restarting the PVRManager", __FUNCTION__);

  Start();

  if (bShowProgress)
  {
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
  }
}

void CPVRManager::ResetEPG(void)
{
  StopThreads();
  g_PVREpgContainer.Reset();
  StartThreads();
}

bool CPVRManager::IsPlayingTV(void)
{
  return m_currentPlayingChannel ?
      !m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio() :
      false;
}

bool CPVRManager::IsPlayingRadio(void)
{
  return m_currentPlayingChannel ?
      m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio() :
      false;
}

bool CPVRManager::IsPlayingRecording(void)
{
  return m_currentPlayingRecording;
}

bool CPVRManager::IsPlaying(void)
{
  return (m_currentPlayingChannel || m_currentPlayingRecording);
}

PVR_SERVERPROPS *CPVRManager::GetCurrentClientProperties(void)
{
  PVR_SERVERPROPS * props = NULL;

  if (m_currentPlayingChannel)
    props = &m_clientsProps[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()];
  else if (m_currentPlayingRecording)
    props = &m_clientsProps[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()];

  return props;
}

int CPVRManager::GetCurrentPlayingClientID(void)
{
  int iReturn = -1;

  if (m_currentPlayingChannel)
    iReturn = m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID();
  else if (m_currentPlayingRecording)
    iReturn = m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID();

  return iReturn;
}

PVR_STREAMPROPS *CPVRManager::GetCurrentStreamProperties(void)
{
  PVR_STREAMPROPS *props = NULL;

  if (m_currentPlayingChannel)
  {
    int cid = m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID();
    m_clients[cid]->GetStreamProperties(&m_streamProps[cid]);

    props = &m_streamProps[cid];
  }

  return props;
}

CFileItem *CPVRManager::GetCurrentPlayingItem(void)
{
  CFileItem *item = NULL;

  if (m_currentPlayingChannel)
    item = m_currentPlayingChannel;
  else if (m_currentPlayingRecording)
    item = m_currentPlayingRecording;

  return item;
}

CStdString CPVRManager::GetCurrentInputFormat(void)
{
  if (m_currentPlayingChannel)
    return m_currentPlayingChannel->GetPVRChannelInfoTag()->InputFormat();

  return "";
}

bool CPVRManager::GetCurrentChannel(const CPVRChannel *channel)
{
  bool bReturn = false;

  if (m_currentPlayingChannel)
  {
    channel = m_currentPlayingChannel->GetPVRChannelInfoTag();
    CLog::Log(LOGDEBUG,"PVRManager - %s - current channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
    bReturn = true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"PVRManager - %s - no current channel set", __FUNCTION__);
    channel = NULL;
  }

  return bReturn;
}

bool CPVRManager::HasActiveClients(void)
{
  bool bReturn = false;

  if (!m_clients.empty())
  {
    CLIENTMAPITR itr = m_clients.begin();
    while (itr != m_clients.end())
    {
      if (m_clients[(*itr).first]->ReadyToUse())
      {
        bReturn = true;
        break;
      }
      itr++;
    }
  }

  return bReturn;
}

bool CPVRManager::HasMenuHooks(int iClientID)
{
  if (iClientID < 0)
    iClientID = GetCurrentPlayingClientID();

  return (iClientID < 0) ? false : m_clients[iClientID]->HaveMenuHooks();
}

void CPVRManager::ProcessMenuHooks(int iClientID)
{
  if (m_clients[iClientID]->HaveMenuHooks())
  {
    PVR_MENUHOOKS *hooks = m_clients[iClientID]->GetMenuHooks();
    std::vector<long> hookIDs;

    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19196);

    for (unsigned int i = 0; i < hooks->size(); i++)
    {
      pDialog->Add(m_clients[iClientID]->GetString(hooks->at(i).string_id));
    }

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
    {
      m_clients[iClientID]->CallMenuHook(hooks->at(selection));
    }
  }
}

int CPVRManager::GetPreviousChannel(void)
{
  //XXX this must be the craziest way to store the last channel
  int iReturn = -1;

  if (m_currentPlayingChannel)
  {
    int iLastChannel = m_currentPlayingChannel->GetPVRChannelInfoTag()->ChannelNumber();

    if ((m_PreviousChannel[m_PreviousChannelIndex ^ 1] == iLastChannel || iLastChannel != m_PreviousChannel[0]) &&
        iLastChannel != m_PreviousChannel[1])
      m_PreviousChannelIndex ^= 1;

    iReturn = m_PreviousChannel[m_PreviousChannelIndex ^= 1];
  }

  return iReturn;
}

bool CPVRManager::CanRecordInstantly(void)
{
  bool bReturn = false;

  if (m_currentPlayingChannel)
  {
    const CPVRChannel* tag = m_currentPlayingChannel->GetPVRChannelInfoTag();
    bReturn = tag ? (m_clientsProps[tag->ClientID()].SupportTimers) : false;
  }

  return bReturn;
}

bool CPVRManager::IsRecordingOnPlayingChannel(void)
{
  bool bReturn = false;

  if (m_currentPlayingChannel)
  {
    const CPVRChannel* tag = m_currentPlayingChannel->GetPVRChannelInfoTag();
    bReturn = tag ? tag->IsRecording() : false;
  }

  return bReturn;
}

bool CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  bool bReturn = false;

  if (!m_currentPlayingChannel)
    return bReturn;

  CPVRChannel *channel = (CPVRChannel *) m_currentPlayingChannel->GetPVRChannelInfoTag();
  if (m_clientsProps[channel->ClientID()].SupportTimers)
  {
    /* timers are supported on this channel */
    if (bOnOff && !channel->IsRecording())
    {
      CPVRTimerInfoTag *newTimer = g_PVRTimers.InstantTimer(channel);
      if (!newTimer)
        CGUIDialogOK::ShowAndGetInput(19033,0,19164,0);
      else
        bReturn = true;
    }
    else if (!bOnOff && channel->IsRecording())
    {
      /* delete active timers */
      bReturn = g_PVRTimers.DeleteTimersOnChannel(*channel, false, true);
    }
  }

  return bReturn;
}

void CPVRManager::SaveCurrentChannelSettings()
{
  if (m_currentPlayingChannel &&
      /* only save settings if they differ from the default settings */
      g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings &&
      m_database.Open())
  {
    m_database.PersistChannelSettings(*m_currentPlayingChannel->GetPVRChannelInfoTag(), g_settings.m_currentVideoSettings);
    m_database.Close();
  }
  else if (m_currentPlayingChannel &&
      /* delete record which might differ from the default settings */
      !(g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings) &&
      m_database.Open())
  {
    m_database.DeleteChannelSettings(*m_currentPlayingChannel->GetPVRChannelInfoTag());
    m_database.Close();
  }
}

void CPVRManager::LoadCurrentChannelSettings()
{
  if (m_currentPlayingChannel && g_application.m_pPlayer)
  {
    CVideoSettings loadedChannelSettings;

    /* set the default settings first */
    g_settings.m_currentVideoSettings = g_settings.m_defaultVideoSettings;

    if (m_database.Open() &&
        m_database.GetChannelSettings(*m_currentPlayingChannel->GetPVRChannelInfoTag(), loadedChannelSettings))
    {
      g_settings.m_currentVideoSettings.m_Brightness          = loadedChannelSettings.m_Brightness;
      g_settings.m_currentVideoSettings.m_Contrast            = loadedChannelSettings.m_Contrast;
      g_settings.m_currentVideoSettings.m_Gamma               = loadedChannelSettings.m_Gamma;
      g_settings.m_currentVideoSettings.m_Crop                = loadedChannelSettings.m_Crop;
      g_settings.m_currentVideoSettings.m_CropLeft            = loadedChannelSettings.m_CropLeft;
      g_settings.m_currentVideoSettings.m_CropRight           = loadedChannelSettings.m_CropRight;
      g_settings.m_currentVideoSettings.m_CropTop             = loadedChannelSettings.m_CropTop;
      g_settings.m_currentVideoSettings.m_CropBottom          = loadedChannelSettings.m_CropBottom;
      g_settings.m_currentVideoSettings.m_CustomPixelRatio    = loadedChannelSettings.m_CustomPixelRatio;
      g_settings.m_currentVideoSettings.m_CustomZoomAmount    = loadedChannelSettings.m_CustomZoomAmount;
      g_settings.m_currentVideoSettings.m_NoiseReduction      = loadedChannelSettings.m_NoiseReduction;
      g_settings.m_currentVideoSettings.m_Sharpness           = loadedChannelSettings.m_Sharpness;
      g_settings.m_currentVideoSettings.m_InterlaceMethod     = loadedChannelSettings.m_InterlaceMethod;
      g_settings.m_currentVideoSettings.m_OutputToAllSpeakers = loadedChannelSettings.m_OutputToAllSpeakers;
      g_settings.m_currentVideoSettings.m_AudioDelay          = loadedChannelSettings.m_AudioDelay;
      g_settings.m_currentVideoSettings.m_AudioStream         = loadedChannelSettings.m_AudioStream;
      g_settings.m_currentVideoSettings.m_SubtitleOn          = loadedChannelSettings.m_SubtitleOn;
      g_settings.m_currentVideoSettings.m_SubtitleDelay       = loadedChannelSettings.m_SubtitleDelay;

      /* only change the view mode if it's different */
      if (g_settings.m_currentVideoSettings.m_ViewMode != loadedChannelSettings.m_ViewMode)
      {
        g_settings.m_currentVideoSettings.m_ViewMode = loadedChannelSettings.m_ViewMode;

        g_renderManager.SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
        g_settings.m_currentVideoSettings.m_CustomZoomAmount = g_settings.m_fZoomAmount;
        g_settings.m_currentVideoSettings.m_CustomPixelRatio = g_settings.m_fPixelRatio;
      }

      /* only change the subtitle strea, if it's different */
      if (g_settings.m_currentVideoSettings.m_SubtitleStream != loadedChannelSettings.m_SubtitleStream)
      {
        g_settings.m_currentVideoSettings.m_SubtitleStream = loadedChannelSettings.m_SubtitleStream;

        g_application.m_pPlayer->SetSubtitle(g_settings.m_currentVideoSettings.m_SubtitleStream);
      }

      /* only change the audio stream if it's different */
      if (g_application.m_pPlayer->GetAudioStream() != g_settings.m_currentVideoSettings.m_AudioStream)
        g_application.m_pPlayer->SetAudioStream(g_settings.m_currentVideoSettings.m_AudioStream);

      g_application.m_pPlayer->SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
      g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));
      g_application.m_pPlayer->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
      g_application.m_pPlayer->SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);

      m_database.Close();
    }
  }
}

void CPVRManager::SetPlayingGroup(int GroupId)
{
  m_CurrentGroupID = GroupId;
}

void CPVRManager::ResetQualityData()
{
  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
  {
    strncpy(m_qualityInfo.frontend_name, g_localizeStrings.Get(13205).c_str(), 1024);
    strncpy(m_qualityInfo.frontend_status, g_localizeStrings.Get(13205).c_str(), 1024);
  }
  else
  {
    strncpy(m_qualityInfo.frontend_name, g_localizeStrings.Get(13106).c_str(), 1024);
    strncpy(m_qualityInfo.frontend_status, g_localizeStrings.Get(13106).c_str(), 1024);
  }
  m_qualityInfo.snr           = 0;
  m_qualityInfo.signal        = 0;
  m_qualityInfo.ber           = 0;
  m_qualityInfo.unc           = 0;
  m_qualityInfo.video_bitrate = 0;
  m_qualityInfo.audio_bitrate = 0;
  m_qualityInfo.dolby_bitrate = 0;
}

int CPVRManager::GetPlayingGroup()
{
  return m_CurrentGroupID;
}

void CPVRManager::TriggerRecordingsUpdate()
{
  m_bTriggerRecordingsUpdate = true;
}

void CPVRManager::TriggerTimersUpdate()
{
  m_bTriggerTimersUpdate = true;
}

void CPVRManager::TriggerChannelsUpdate()
{
  m_bTriggerChannelsUpdate = true;
}

bool CPVRManager::OpenLiveStream(const CPVRChannel* tag)
{
  if (tag == NULL)
    return false;

  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening live stream on channel '%s'",
      __FUNCTION__, tag->ChannelName().c_str());

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new channel information */
  m_currentPlayingChannel   = new CFileItem(*tag);
  m_currentPlayingRecording = NULL;
  m_scanStart               = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
  ResetQualityData();

  /* Open the stream on the Client */
  if (tag->StreamURL().IsEmpty())
  {
    if (!m_clientsProps[tag->ClientID()].HandleInputStream ||
        !m_clients[tag->ClientID()]->OpenLiveStream(*tag))
    {
      delete m_currentPlayingChannel;
      m_currentPlayingChannel = NULL;
      return false;
    }
  }

  /* Load now the new channel settings from Database */
  LoadCurrentChannelSettings();
  return true;
}

bool CPVRManager::OpenRecordedStream(const CPVRRecordingInfoTag* tag)
{
  if (tag == NULL)
    return false;

  CSingleLock lock(m_critSection);

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new recording information */
  m_currentPlayingRecording = new CFileItem(*tag);
  m_currentPlayingChannel   = NULL;
  m_scanStart               = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
  m_playingClientName       = m_clients[tag->ClientID()]->GetBackendName() + ":" + m_clients[tag->ClientID()]->GetConnectionString();

  /* Open the recording stream on the Client */
  return m_clients[tag->ClientID()]->OpenRecordedStream(*tag);
}

CStdString CPVRManager::GetLiveStreamURL(const CPVRChannel *channel)
{
  CStdString stream_url;

  CSingleLock lock(m_critSection);

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new channel information */
  m_currentPlayingChannel   = new CFileItem(*channel);
  m_currentPlayingRecording = NULL;
  m_scanStart               = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
  ResetQualityData();

  /* Retrieve the dynamily generated stream URL from the Client */
  stream_url = m_clients[channel->ClientID()]->GetLiveStreamURL(*channel);
  if (stream_url.IsEmpty())
  {
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
    return "";
  }

  return stream_url;
}

void CPVRManager::CloseStream()
{
  CSingleLock lock(m_critSection);

  if (m_currentPlayingChannel)
  {
    m_playingClientName = "";

    /* Save channel number in database */
    m_database.Open();
    m_database.PersistLastChannel(*m_currentPlayingChannel->GetPVRChannelInfoTag());
    m_database.Close();

    /* Store current settings inside Database */
    SaveCurrentChannelSettings();

    /* Set quality data to undefined defaults */
    ResetQualityData();

    /* Close the Client connection */
    if ((m_currentPlayingChannel->GetPVRChannelInfoTag()->StreamURL().IsEmpty()) || (m_currentPlayingChannel->GetPVRChannelInfoTag()->StreamURL().compare(0,13, "pvr://stream/") == 0))
      m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->CloseLiveStream();
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
  }
  else if (m_currentPlayingRecording)
  {
    /* Close the Client connection */
    m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->CloseRecordedStream();
    delete m_currentPlayingRecording;
    m_currentPlayingRecording = NULL;
  }
}

int CPVRManager::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);

  int bytesRead = 0;

  /* Check stream for available video or audio data, if after the scantime no stream
     is present playback is canceled and returns to the window */
  if (m_scanStart)
  {
    if (CTimeUtils::GetTimeMS() - m_scanStart > (unsigned int) g_guiSettings.GetInt("pvrplayback.scantime")*1000)
    {
      CLog::Log(LOGERROR,"PVRManager - %s - no video or audio data available after %i seconds, playback stopped",
          __FUNCTION__, g_guiSettings.GetInt("pvrplayback.scantime"));
      return 0;
    }
    else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
      m_scanStart = NULL;
  }

  if (m_currentPlayingChannel)
    bytesRead = m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->ReadLiveStream(lpBuf, uiBufSize);
  else if (m_currentPlayingRecording)
    bytesRead = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->ReadRecordedStream(lpBuf, uiBufSize);

  return bytesRead;
}

void CPVRManager::DemuxReset()
{
  CSingleLock lock(m_critSection);
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->DemuxReset();
}

void CPVRManager::DemuxAbort()
{
  CSingleLock lock(m_critSection);
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->DemuxAbort();
}

void CPVRManager::DemuxFlush()
{
  CSingleLock lock(m_critSection);
  if (m_currentPlayingChannel)
    m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->DemuxFlush();
}

DemuxPacket* CPVRManager::ReadDemuxStream()
{
  DemuxPacket* packet = NULL;

  CSingleLock lock(m_critSection);
  if (m_currentPlayingChannel)
    packet = m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->DemuxRead();

  return packet;
}

int64_t CPVRManager::LengthStream(void)
{
  int64_t streamLength = 0;

  CSingleLock lock(m_critSection);

  if (m_currentPlayingChannel)
    streamLength = 0;
  else if (m_currentPlayingRecording)
    streamLength = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->LengthRecordedStream();

  return streamLength;
}

int64_t CPVRManager::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  int64_t streamNewPos = 0;

  CSingleLock lock(m_critSection);

  if (m_currentPlayingChannel)
    streamNewPos = 0;
  else if (m_currentPlayingRecording)
    streamNewPos = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->SeekRecordedStream(iFilePosition, iWhence);

  return streamNewPos;
}

int64_t CPVRManager::GetStreamPosition()
{
  int64_t streamPos = 0;

  CSingleLock lock(m_critSection);

  if (m_currentPlayingChannel)
    streamPos = 0;
  else if (m_currentPlayingRecording)
    streamPos = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->PositionRecordedStream();

  return streamPos;
}

bool CPVRManager::UpdateItem(CFileItem& item)
{
  /* Don't update if a recording is played */
  if (item.IsPVRRecording())
    return false;

  if (!item.IsPVRChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager - %s - no channel tag provided", __FUNCTION__);
    return false;
  }

  g_application.CurrentFileItem() = *m_currentPlayingChannel;
  g_infoManager.SetCurrentItem(*m_currentPlayingChannel);

  CPVRChannel* channelTag = item.GetPVRChannelInfoTag();
  const CPVREpgInfoTag* epgTagNow = channelTag->GetEPGNow();

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(epgTagNow->Title());
      musictag->SetGenre(epgTagNow->Genre());
      musictag->SetDuration(epgTagNow->GetDuration());
      musictag->SetURL(channelTag->Path());
      musictag->SetArtist(channelTag->ChannelName());
      musictag->SetAlbumArtist(channelTag->ChannelName());
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }
  }

  CPVRChannel* tagPrev = item.GetPVRChannelInfoTag();
  if (tagPrev && tagPrev->ChannelNumber() != m_LastChannel)
  {
    m_LastChannel         = tagPrev->ChannelNumber();
    m_LastChannelChanged  = CTimeUtils::GetTimeMS();
    if (channelTag->ClientID() == 999)
      m_playingClientName = g_localizeStrings.Get(19209);
    else if (!channelTag->IsVirtual())
      m_playingClientName = m_clients[channelTag->ClientID()]->GetBackendName() + ":" + m_clients[channelTag->ClientID()]->GetConnectionString();
    else
      m_playingClientName = g_localizeStrings.Get(13205);
  }
  if (CTimeUtils::GetTimeMS() - m_LastChannelChanged >= (unsigned int) g_guiSettings.GetInt("pvrplayback.channelentrytimeout") && m_LastChannel != m_PreviousChannel[m_PreviousChannelIndex])
     m_PreviousChannel[m_PreviousChannelIndex ^= 1] = m_LastChannel;

  return false;
}

bool CPVRManager::ChannelSwitch(unsigned int iChannel)
{
  return PerformChannelSwitch(m_currentPlayingChannel->GetPVRChannelInfoTag(), false);
}

bool CPVRManager::ChannelUp(unsigned int *iNewChannelNumber, bool bPreview /* = false*/)
{
  return ChannelUpDown(iNewChannelNumber, bPreview, true);
}

bool CPVRManager::ChannelDown(unsigned int *iNewChannelNumber, bool bPreview /* = false*/)
{
  return ChannelUpDown(iNewChannelNumber, bPreview, false);
}

bool CPVRManager::ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp)
{
  bool bReturn = false;

  if (m_currentPlayingChannel)
  {
    const CPVRChannel *currentChannel = m_currentPlayingChannel->GetPVRChannelInfoTag();
    const CPVRChannelGroup *group = g_PVRChannelGroups.GetById(currentChannel->IsRadio(), currentChannel->GroupID());
    if (group)
    {
      const CPVRChannel *newChannel = bUp ? group->GetByChannelUp(currentChannel) : group->GetByChannelDown(currentChannel);
      if (PerformChannelSwitch(newChannel, bPreview))
      {
        *iNewChannelNumber = newChannel->ChannelNumber();
        bReturn = true;
      }
    }
  }

  return bReturn;
}

bool CPVRManager::StartPlayback(const CPVRChannel *channel, bool bPreview /* = false */)
{
  bool bReturn = false;
  g_settings.m_bStartVideoWindowed = bPreview;

  if (g_application.PlayFile(CFileItem(*channel)))
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - started playback on channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
    bReturn = true;
  }
  else
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to start playback on channel '%s'",
        __FUNCTION__, channel ? channel->ChannelName().c_str() : "NULL");
  }

  return bReturn;
}

bool CPVRManager::PerformChannelSwitch(const CPVRChannel *channel, bool bPreview)
{
  CSingleLock lock(m_critSection);

  if (!channel || channel->StreamURL().IsEmpty() || !m_clients[channel->ClientID()]->SwitchChannel(*channel))
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to switch to channel '%s'",
        __FUNCTION__, channel ? channel->ChannelName().c_str() : "NULL");
    CGUIDialogOK::ShowAndGetInput(19033,0,19136,0);
    return false;
  }

  if (!bPreview)
  {
    m_scanStart = CTimeUtils::GetTimeMS();
    ResetQualityData();
  }

  SaveCurrentChannelSettings();

  delete m_currentPlayingChannel;
  m_currentPlayingChannel = new CFileItem(*channel);

  LoadCurrentChannelSettings();

  CLog::Log(LOGNOTICE, "PVRManager - %s - switched to channel '%s'",
      __FUNCTION__, channel->ChannelName().c_str());

  return true;
}

const CPVREpgInfoTag *CPVRManager::GetPlayingTag(void)
{
  const CPVREpgInfoTag *tag = NULL;

  if (m_currentPlayingChannel)
  {
    tag = m_currentPlayingChannel->GetPVRChannelInfoTag()->GetEPGNow();
    if (tag && !tag->IsActive())
    {
      CSingleLock lock(m_critSection);
      UpdateItem(*m_currentPlayingChannel);
    }
  }

  return tag;
}

int CPVRManager::GetTotalTime()
{
  const CPVREpgInfoTag *tag = GetPlayingTag();

  return tag ? tag->GetDuration() * 1000 : 0;
}

int CPVRManager::GetStartTime()
{
  const CPVREpgInfoTag *tag = GetPlayingTag();

  if (tag)
  {
    /* Calculate here the position we have of the running live TV event.
     * "position in ms" = ("current local time" - "event start local time") * 1000
     */
    CDateTimeSpan time = CDateTime::GetCurrentDateTime() - tag->Start();
    return time.GetDays()    * 1000 * 60 * 60 * 24
         + time.GetHours()   * 1000 * 60 * 60
         + time.GetMinutes() * 1000 * 60
         + time.GetSeconds() * 1000;
  }
  else
  {
    return 0;
  }
}

CPVRManager g_PVRManager;
