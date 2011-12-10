/*
 *      Copyright (C) 2005-2011 Team XBMC
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
#include "PVRGUIInfo.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "GUIInfoManager.h"
#include "Util.h"
#include "threads/SingleLock.h"
#include "PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/channels/PVRChannel.h"
#include "epg/EpgInfoTag.h"
#include "settings/AdvancedSettings.h"

using namespace PVR;
using namespace EPG;

CPVRGUIInfo::CPVRGUIInfo(void) :
    CThread("PVR GUI info updater"),
    m_playingEpgTag(NULL)
{
  ResetProperties();
}

CPVRGUIInfo::~CPVRGUIInfo(void)
{
  Stop();
}

void CPVRGUIInfo::ResetProperties(void)
{
  m_strActiveTimerTitle         = "";
  m_strActiveTimerChannelName   = "";
  m_strActiveTimerChannelIcon   = "";
  m_strActiveTimerTime          = "";
  m_strNextTimerInfo            = "";
  m_strNextRecordingTitle       = "";
  m_strNextRecordingChannelName = "";
  m_strNextRecordingChannelIcon = "";
  m_strNextRecordingTime        = "";
  m_iTimerAmount                = 0;
  m_bHasRecordings              = false;
  m_iRecordingTimerAmount       = 0;
  m_iActiveClients              = 0;
  m_strPlayingClientName        = "";
  m_strBackendName              = "";
  m_strBackendVersion           = "";
  m_strBackendHost              = "";
  m_strBackendDiskspace         = "";
  m_strBackendTimers            = "";
  m_strBackendRecordings        = "";
  m_strBackendChannels          = "";
  m_strTotalDiskspace           = "";
  m_iAddonInfoToggleStart       = 0;
  m_iAddonInfoToggleCurrent     = 0;
  m_iTimerInfoToggleStart       = 0;
  m_iTimerInfoToggleCurrent     = 0;
  m_iToggleShowInfo             = 0;
  m_iDuration                   = 0;
  m_bHasNonRecordingTimers      = false;
  m_bIsPlayingTV                = false;
  m_bIsPlayingRadio             = false;
  m_bIsPlayingRecording         = false;
  m_bIsPlayingEncryptedStream   = false;

  if (m_playingEpgTag)
    delete m_playingEpgTag;
  m_playingEpgTag               = NULL;

  strncpy(m_qualityInfo.strAdapterName, g_localizeStrings.Get(13106).c_str(), 1024);
  strncpy(m_qualityInfo.strAdapterStatus, g_localizeStrings.Get(13106).c_str(), 1024);
  m_qualityInfo.iSNR          = 0;
  m_qualityInfo.iSignal       = 0;
  m_qualityInfo.iSNR          = 0;
  m_qualityInfo.iUNC          = 0;
  m_qualityInfo.dVideoBitrate = 0;
  m_qualityInfo.dAudioBitrate = 0;
  m_qualityInfo.dDolbyBitrate = 0;
}

void CPVRGUIInfo::Start(void)
{
  ResetProperties();
  Create();
  SetPriority(-1);
}

void CPVRGUIInfo::Stop(void)
{
  StopThread();
  g_PVRTimers->UnregisterObserver(this);
}

void CPVRGUIInfo::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("timers") || msg.Equals("timers-reset"))
    UpdateTimersCache();
}

void CPVRGUIInfo::ShowPlayerInfo(int iTimeout)
{
  CSingleLock lock(m_critSection);

  if (iTimeout > 0)
    m_iToggleShowInfo = (int) XbmcThreads::SystemClockMillis() + iTimeout * 1000;

  g_infoManager.SetShowInfo(true);
}

void CPVRGUIInfo::ToggleShowInfo(void)
{
  CSingleLock lock(m_critSection);

  if (m_iToggleShowInfo > 0 && m_iToggleShowInfo < (unsigned int) XbmcThreads::SystemClockMillis())
  {
    m_iToggleShowInfo = 0;
    g_infoManager.SetShowInfo(false);
  }
}

bool CPVRGUIInfo::AddonInfoToggle(void)
{
  if (m_iAddonInfoToggleStart == 0)
  {
    m_iAddonInfoToggleStart = XbmcThreads::SystemClockMillis();
    m_iAddonInfoToggleCurrent = 0;
    return true;
  }

  if ((int) (XbmcThreads::SystemClockMillis() - m_iAddonInfoToggleStart) > g_advancedSettings.m_iPVRInfoToggleInterval)
  {
    unsigned int iPrevious = m_iAddonInfoToggleCurrent;
    if (((int) ++m_iAddonInfoToggleCurrent) > m_iActiveClients - 1)
      m_iAddonInfoToggleCurrent = 0;

    return m_iAddonInfoToggleCurrent != iPrevious;
  }

  return false;
}

bool CPVRGUIInfo::TimerInfoToggle(void)
{
  if (m_iTimerInfoToggleStart == 0)
  {
    m_iTimerInfoToggleStart = XbmcThreads::SystemClockMillis();
    m_iTimerInfoToggleCurrent = 0;
    return true;
  }

  if ((int) (XbmcThreads::SystemClockMillis() - m_iTimerInfoToggleStart) > g_advancedSettings.m_iPVRInfoToggleInterval)
  {
    unsigned int iPrevious = m_iTimerInfoToggleCurrent;
    unsigned int iBoundary = m_iRecordingTimerAmount > 0 ? m_iRecordingTimerAmount : m_iTimerAmount;
    if (++m_iTimerInfoToggleCurrent > iBoundary - 1)
      m_iTimerInfoToggleCurrent = 0;

    return m_iTimerInfoToggleCurrent != iPrevious;
  }

  return false;
}

void CPVRGUIInfo::Process(void)
{
  unsigned int mLoop(0);

  /* updated on request */
  g_PVRTimers->RegisterObserver(this);
  UpdateTimersCache();

  while (!g_application.m_bStop && !m_bStop)
  {
    if (!m_bStop)
      ToggleShowInfo();
    Sleep(0);

    if (!m_bStop)
      UpdateQualityData();
    Sleep(0);

    if (!m_bStop)
      UpdateMisc();
    Sleep(0);

    if (!m_bStop)
      UpdatePlayingTag();
    Sleep(0);

    if (!m_bStop)
      UpdateTimersToggle();
    Sleep(0);

    if (!m_bStop)
      UpdateNextTimer();
    Sleep(0);

    if (!m_bStop && mLoop % 10 == 0)
      UpdateBackendCache();    /* updated every 10 iterations */

    if (++mLoop == 1000)
      mLoop = 0;

    if (!m_bStop)
      Sleep(1000);
  }

  if (!m_bStop)
    ResetPlayingTag();
}

void CPVRGUIInfo::UpdateQualityData(void)
{
  CSingleLock lock(m_critSection);

  g_PVRClients->GetQualityData(&m_qualityInfo);
}

void CPVRGUIInfo::UpdateMisc(void)
{
  CSingleLock lock(m_critSection);
  bool bStarted = g_PVRManager.IsStarted();

  m_strPlayingClientName      = bStarted ? g_PVRClients->GetPlayingClientName() : "";
  m_bHasRecordings            = bStarted && g_PVRRecordings->GetNumRecordings() > 0;
  m_bHasNonRecordingTimers    = bStarted && m_iTimerAmount - m_iRecordingTimerAmount > 0;
  m_bIsPlayingTV              = bStarted && g_PVRClients->IsPlayingTV();
  m_bIsPlayingRadio           = bStarted && g_PVRClients->IsPlayingRadio();
  m_bIsPlayingRecording       = bStarted && g_PVRClients->IsPlayingRecording();
  m_bIsPlayingEncryptedStream = bStarted && g_PVRClients->IsEncrypted();
}

bool CPVRGUIInfo::TranslateCharInfo(DWORD dwInfo, CStdString &strValue) const
{
  bool bReturn(true);
  CSingleLock lock(m_critSection);

  switch(dwInfo)
  {
  case PVR_NOW_RECORDING_TITLE:
    CharInfoActiveTimerTitle(strValue);
    break;
  case PVR_NOW_RECORDING_CHANNEL:
    CharInfoActiveTimerChannelName(strValue);
    break;
  case PVR_NOW_RECORDING_CHAN_ICO:
    CharInfoActiveTimerChannelIcon(strValue);
    break;
  case PVR_NOW_RECORDING_DATETIME:
    CharInfoActiveTimerDateTime(strValue);
    break;
  case PVR_NEXT_RECORDING_TITLE:
    CharInfoNextTimerTitle(strValue);
    break;
  case PVR_NEXT_RECORDING_CHANNEL:
    CharInfoNextTimerChannelName(strValue);
    break;
  case PVR_NEXT_RECORDING_CHAN_ICO:
    CharInfoNextTimerChannelIcon(strValue);
    break;
  case PVR_NEXT_RECORDING_DATETIME:
    CharInfoNextTimerDateTime(strValue);
    break;
  case PVR_PLAYING_DURATION:
    CharInfoPlayingDuration(strValue);
    break;
  case PVR_PLAYING_TIME:
    CharInfoPlayingTime(strValue);
    break;
  case PVR_NEXT_TIMER:
    CharInfoNextTimer(strValue);
    break;
  case PVR_ACTUAL_STREAM_VIDEO_BR:
    CharInfoVideoBR(strValue);
    break;
  case PVR_ACTUAL_STREAM_AUDIO_BR:
    CharInfoAudioBR(strValue);
    break;
  case PVR_ACTUAL_STREAM_DOLBY_BR:
    CharInfoDolbyBR(strValue);
    break;
  case PVR_ACTUAL_STREAM_SIG:
    CharInfoSignal(strValue);
    break;
  case PVR_ACTUAL_STREAM_SNR:
    CharInfoSNR(strValue);
    break;
  case PVR_ACTUAL_STREAM_BER:
    CharInfoBER(strValue);
    break;
  case PVR_ACTUAL_STREAM_UNC:
    CharInfoUNC(strValue);
    break;
  case PVR_ACTUAL_STREAM_CLIENT:
    CharInfoPlayingClientName(strValue);
    break;
  case PVR_ACTUAL_STREAM_DEVICE:
    CharInfoFrontendName(strValue);
    break;
  case PVR_ACTUAL_STREAM_STATUS:
    CharInfoFrontendStatus(strValue);
    break;
  case PVR_ACTUAL_STREAM_CRYPTION:
    CharInfoEncryption(strValue);
    break;
  case PVR_BACKEND_NAME:
    CharInfoBackendName(strValue);
    break;
  case PVR_BACKEND_VERSION:
    CharInfoBackendVersion(strValue);
    break;
  case PVR_BACKEND_HOST:
    CharInfoBackendHost(strValue);
    break;
  case PVR_BACKEND_DISKSPACE:
    CharInfoBackendDiskspace(strValue);
    break;
  case PVR_BACKEND_CHANNELS:
    CharInfoBackendChannels(strValue);
    break;
  case PVR_BACKEND_TIMERS:
    CharInfoBackendTimers(strValue);
    break;
  case PVR_BACKEND_RECORDINGS:
    CharInfoBackendRecordings(strValue);
    break;
  case PVR_BACKEND_NUMBER:
    CharInfoBackendNumber(strValue);
    break;
  case PVR_TOTAL_DISKSPACE:
    CharInfoTotalDiskSpace(strValue);
    break;
  default:
    strValue = "";
    bReturn = false;
    break;
  }

  return bReturn;
}

bool CPVRGUIInfo::TranslateBoolInfo(DWORD dwInfo) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  switch (dwInfo)
  {
  case PVR_IS_RECORDING:
    bReturn = m_iRecordingTimerAmount > 0;
    break;
  case PVR_HAS_TIMER:
    bReturn = m_iTimerAmount > 0;
    break;
  case PVR_HAS_NONRECORDING_TIMER:
    bReturn = m_bHasNonRecordingTimers;
    break;
  case PVR_IS_PLAYING_TV:
    bReturn = m_bIsPlayingTV;
    break;
  case PVR_IS_PLAYING_RADIO:
    bReturn = m_bIsPlayingRadio;
    break;
  case PVR_IS_PLAYING_RECORDING:
    bReturn = m_bIsPlayingRecording;
    break;
  case PVR_ACTUAL_STREAM_ENCRYPTED:
    bReturn = m_bIsPlayingEncryptedStream;
    break;
  default:
    break;
  }

  return bReturn;
}

int CPVRGUIInfo::TranslateIntInfo(DWORD dwInfo) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  if (dwInfo == PVR_PLAYING_PROGRESS)
    iReturn = (int) ((float) GetStartTime() / m_iDuration * 100);
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG_PROGR)
    iReturn = (int) ((float) m_qualityInfo.iSignal / 0xFFFF * 100);
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR_PROGR)
    iReturn = (int) ((float) m_qualityInfo.iSNR / 0xFFFF * 100);

  return iReturn;
}

void CPVRGUIInfo::CharInfoActiveTimerTitle(CStdString &strValue) const
{
  strValue.Format("%s", m_strActiveTimerTitle);
}

void CPVRGUIInfo::CharInfoActiveTimerChannelName(CStdString &strValue) const
{
  strValue.Format("%s", m_strActiveTimerChannelName);
}

void CPVRGUIInfo::CharInfoActiveTimerChannelIcon(CStdString &strValue) const
{
  strValue.Format("%s", m_strActiveTimerChannelIcon);
}

void CPVRGUIInfo::CharInfoActiveTimerDateTime(CStdString &strValue) const
{
  strValue.Format("%s", m_strActiveTimerTime);
}

void CPVRGUIInfo::CharInfoNextTimerTitle(CStdString &strValue) const
{
  strValue.Format("%s", m_strNextRecordingTitle);
}

void CPVRGUIInfo::CharInfoNextTimerChannelName(CStdString &strValue) const
{
  strValue.Format("%s", m_strNextRecordingChannelName);
}

void CPVRGUIInfo::CharInfoNextTimerChannelIcon(CStdString &strValue) const
{
  strValue.Format("%s", m_strNextRecordingChannelIcon);
}

void CPVRGUIInfo::CharInfoNextTimerDateTime(CStdString &strValue) const
{
  strValue.Format("%s", m_strNextRecordingTime);
}

void CPVRGUIInfo::CharInfoPlayingDuration(CStdString &strValue) const
{
  strValue.Format("%s", StringUtils::SecondsToTimeString(m_iDuration / 1000, TIME_FORMAT_GUESS));
}

void CPVRGUIInfo::CharInfoPlayingTime(CStdString &strValue) const
{
  strValue.Format("%s", StringUtils::SecondsToTimeString(GetStartTime()/1000, TIME_FORMAT_GUESS));
}

void CPVRGUIInfo::CharInfoNextTimer(CStdString &strValue) const
{
  strValue.Format("%s", m_strNextTimerInfo);
}

void CPVRGUIInfo::CharInfoBackendNumber(CStdString &strValue) const
{
  if (m_iActiveClients > 0)
    strValue.Format("%u %s %u", m_iAddonInfoToggleCurrent+1, g_localizeStrings.Get(20163), m_iActiveClients);
  else
    strValue.Format("%s", g_localizeStrings.Get(14023));
}

void CPVRGUIInfo::CharInfoTotalDiskSpace(CStdString &strValue) const
{
  strValue.Format("%s", m_strTotalDiskspace);
}

void CPVRGUIInfo::CharInfoVideoBR(CStdString &strValue) const
{
  strValue.Format("%.2f Mbit/s", m_qualityInfo.dVideoBitrate);
}

void CPVRGUIInfo::CharInfoAudioBR(CStdString &strValue) const
{
  strValue.Format("%.0f kbit/s", m_qualityInfo.dAudioBitrate);
}

void CPVRGUIInfo::CharInfoDolbyBR(CStdString &strValue) const
{
  strValue.Format("%.0f kbit/s", m_qualityInfo.dDolbyBitrate);
}

void CPVRGUIInfo::CharInfoSignal(CStdString &strValue) const
{
  strValue.Format("%d %%", m_qualityInfo.iSignal / 655);
}

void CPVRGUIInfo::CharInfoSNR(CStdString &strValue) const
{
  strValue.Format("%d %%", m_qualityInfo.iSNR / 655);
}

void CPVRGUIInfo::CharInfoBER(CStdString &strValue) const
{
  strValue.Format("%08X", m_qualityInfo.iBER);
}

void CPVRGUIInfo::CharInfoUNC(CStdString &strValue) const
{
  strValue.Format("%08X", m_qualityInfo.iUNC);
}

void CPVRGUIInfo::CharInfoFrontendName(CStdString &strValue) const
{
  if (!strcmp(m_qualityInfo.strAdapterName, ""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_qualityInfo.strAdapterName);
}

void CPVRGUIInfo::CharInfoFrontendStatus(CStdString &strValue) const
{
  if (!strcmp(m_qualityInfo.strAdapterStatus, ""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_qualityInfo.strAdapterStatus);
}

void CPVRGUIInfo::CharInfoBackendName(CStdString &strValue) const
{
  if (m_strBackendName.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strBackendName);
}

void CPVRGUIInfo::CharInfoBackendVersion(CStdString &strValue) const
{
  if (m_strBackendVersion.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s",  m_strBackendVersion);
}

void CPVRGUIInfo::CharInfoBackendHost(CStdString &strValue) const
{
  if (m_strBackendHost.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strBackendHost);
}

void CPVRGUIInfo::CharInfoBackendDiskspace(CStdString &strValue) const
{
  if (m_strBackendDiskspace.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strBackendDiskspace);
}

void CPVRGUIInfo::CharInfoBackendChannels(CStdString &strValue) const
{
  if (m_strBackendChannels.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strBackendChannels);
}

void CPVRGUIInfo::CharInfoBackendTimers(CStdString &strValue) const
{
  if (m_strBackendTimers.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strBackendTimers);
}

void CPVRGUIInfo::CharInfoBackendRecordings(CStdString &strValue) const
{
  if (m_strBackendRecordings.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strBackendRecordings);
}

void CPVRGUIInfo::CharInfoPlayingClientName(CStdString &strValue) const
{
  if (m_strPlayingClientName.Equals(""))
    strValue.Format("%s", g_localizeStrings.Get(13205));
  else
    strValue.Format("%s", m_strPlayingClientName);
}

void CPVRGUIInfo::CharInfoEncryption(CStdString &strValue) const
{
  CPVRChannel channel;
  if (g_PVRClients->GetPlayingChannel(&channel))
    strValue.Format("%s", channel.EncryptionName());
  else
    strValue = "";
}

void CPVRGUIInfo::UpdateBackendCache(void)
{
  CSingleLock lock(m_critSection);

  if (!AddonInfoToggle())
    return;

  m_strBackendName         = "";
  m_strBackendVersion      = "";
  m_strBackendHost         = "";
  m_strBackendDiskspace    = "";
  m_strBackendTimers       = "";
  m_strBackendRecordings   = "";
  m_strBackendChannels     = "";

  CPVRClients *clients = g_PVRClients;
  CLIENTMAP activeClients;
  m_iActiveClients = clients->GetConnectedClients(&activeClients);
  if (m_iActiveClients > 0)
  {
    CLIENTMAPITR activeClient = activeClients.begin();
    for (unsigned int i = 0; i < m_iAddonInfoToggleCurrent; i++)
      activeClient++;

    long long kBTotal = 0;
    long long kBUsed  = 0;

    if (activeClient->second->GetDriveSpace(&kBTotal, &kBUsed) == PVR_ERROR_NO_ERROR)
    {
      kBTotal /= 1024; // Convert to MBytes
      kBUsed /= 1024;  // Convert to MBytes
      m_strBackendDiskspace.Format("%s %.1f GByte - %s: %.1f GByte",
          g_localizeStrings.Get(20161), (float) kBTotal / 1024, g_localizeStrings.Get(20162), (float) kBUsed / 1024);
    }
    else
    {
      m_strBackendDiskspace = g_localizeStrings.Get(19055);
    }

    int NumChannels = activeClient->second->GetChannelsAmount();
    if (NumChannels >= 0)
      m_strBackendChannels.Format("%i", NumChannels);
    else
      m_strBackendChannels = g_localizeStrings.Get(161);

    int NumTimers = activeClient->second->GetTimersAmount();
    if (NumTimers >= 0)
      m_strBackendTimers.Format("%i", NumTimers);
    else
      m_strBackendTimers = g_localizeStrings.Get(161);

    int NumRecordings = activeClient->second->GetRecordingsAmount();
    if (NumRecordings >= 0)
      m_strBackendRecordings.Format("%i", NumRecordings);
    else
      m_strBackendRecordings = g_localizeStrings.Get(161);

    m_strBackendName         = activeClient->second->GetBackendName();
    m_strBackendVersion      = activeClient->second->GetBackendVersion();
    m_strBackendHost         = activeClient->second->GetConnectionString();
  }
}

void CPVRGUIInfo::UpdateTimersCache(void)
{
  CSingleLock lock(m_critSection);

  m_iTimerAmount          = g_PVRTimers->GetNumActiveTimers();
  m_iRecordingTimerAmount = g_PVRTimers->GetNumActiveRecordings();
  m_iTimerInfoToggleStart = 0;

  UpdateTimersToggle();
}

void CPVRGUIInfo::UpdateNextTimer(void)
{
  CPVRTimerInfoTag tag;
  if (g_PVRTimers->GetNextActiveTimer(&tag))
  {
    CSingleLock lock(m_critSection);
    m_strNextRecordingTitle.Format("%s",       tag.m_strTitle);
    m_strNextRecordingChannelName.Format("%s", tag.ChannelName());
    m_strNextRecordingChannelIcon.Format("%s", tag.ChannelIcon());
    m_strNextRecordingTime.Format("%s",        tag.StartAsLocalTime().GetAsLocalizedDateTime(false, false));

    m_strNextTimerInfo.Format("%s %s %s %s",
        g_localizeStrings.Get(19106),
        tag.StartAsLocalTime().GetAsLocalizedDate(true),
        g_localizeStrings.Get(19107),
        tag.StartAsLocalTime().GetAsLocalizedTime("HH:mm", false));
  }
  else
  {
    m_strNextRecordingTitle       = "";
    m_strNextRecordingChannelName = "";
    m_strNextRecordingChannelIcon = "";
    m_strNextRecordingTime        = "";
    m_strNextTimerInfo            = "";
  }
}

void CPVRGUIInfo::UpdateTimersToggle(void)
{
  CSingleLock lock(m_critSection);

  if (!TimerInfoToggle())
    return;

  m_strActiveTimerTitle         = "";
  m_strActiveTimerChannelName   = "";
  m_strActiveTimerChannelIcon   = "";
  m_strActiveTimerTime          = "";

  unsigned int iBoundary = m_iRecordingTimerAmount > 0 ? m_iRecordingTimerAmount : m_iTimerAmount;
  if (m_iTimerInfoToggleCurrent < iBoundary)
  {
    CPVRTimerInfoTag tag;
    if (g_PVRTimers->GetTimerByIndex(m_iTimerInfoToggleCurrent, &tag))
    {
      m_strActiveTimerTitle.Format("%s",       tag.m_strTitle);
      m_strActiveTimerChannelName.Format("%s", tag.ChannelName());
      m_strActiveTimerChannelIcon.Format("%s", tag.ChannelIcon());
      m_strActiveTimerTime.Format("%s",        tag.StartAsLocalTime().GetAsLocalizedDateTime(false, false));
    }
  }
}

int CPVRGUIInfo::GetDuration(void) const
{
  CSingleLock lock(m_critSection);
  return m_iDuration;
}

int CPVRGUIInfo::GetStartTime(void) const
{
  CSingleLock lock(m_critSection);

  if (m_playingEpgTag)
  {
    /* Calculate here the position we have of the running live TV event.
     * "position in ms" = ("current local time" - "event start local time") * 1000
     */
    CDateTime current = CDateTime::GetCurrentDateTime();
    CDateTime start = m_playingEpgTag->StartAsLocalTime();
    CDateTimeSpan time = current > start ? current - start : CDateTimeSpan(0, 0, 0, 0);
    return (time.GetDays()   * 60 * 60 * 24
         + time.GetHours()   * 60 * 60
         + time.GetMinutes() * 60
         + time.GetSeconds()) * 1000;
  }
  else
  {
    return 0;
  }
}

void CPVRGUIInfo::ResetPlayingTag(void)
{
  CSingleLock lock(m_critSection);

  if (m_playingEpgTag)
    delete m_playingEpgTag;
  m_playingEpgTag = NULL;
}

void CPVRGUIInfo::UpdatePlayingTag(void)
{
  CSingleLock lock(m_critSection);

  CPVRChannel currentChannel;
  CPVRRecording recording;
  if (g_PVRManager.GetCurrentChannel(&currentChannel))
  {
    if (!m_playingEpgTag || !m_playingEpgTag->IsActive() ||
        !m_playingEpgTag->ChannelTag() ||
        (*m_playingEpgTag->ChannelTag() != currentChannel))
    {
      if (m_playingEpgTag)
      {
        delete m_playingEpgTag;
        m_playingEpgTag = NULL;
      }

      const CEpgInfoTag *newTag = currentChannel.GetEPGNow();
      if (newTag)
        m_playingEpgTag = new CEpgInfoTag(*newTag);

      m_iDuration = m_playingEpgTag ? m_playingEpgTag->GetDuration() * 1000 : 0;
      g_PVRManager.UpdateCurrentFile();
    }
  }
  else if (g_PVRClients->GetPlayingRecording(&recording))
  {
    if (m_playingEpgTag)
      delete m_playingEpgTag;
    m_playingEpgTag = NULL;
    m_iDuration = recording.GetDuration() * 1000;
  }
}
