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

#include "PVRGUIInfo.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "GUIInfoManager.h"
#include "Util.h"
#include "threads/SingleLock.h"
#include "PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/PVREpgInfoTag.h"

CPVRGUIInfo::CPVRGUIInfo(void)
{
}

CPVRGUIInfo::~CPVRGUIInfo(void)
{
}

void CPVRGUIInfo::ResetProperties(void)
{
  m_NowRecording.clear();
  m_NextRecording               = NULL;
  m_strActiveTimerTitle         = "";
  m_strActiveTimerChannelName   = "";
  m_strActiveTimerTime          = "";
  m_strNextTimerInfo            = "";
  m_strNextRecordingTitle       = "";
  m_strNextRecordingChannelName = "";
  m_strNextRecordingTime        = "";
  m_bIsRecording                = false;
  m_bHasRecordings              = false;
  m_bHasTimers                  = false;
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
  m_playingEpgTag               = NULL;
  CPVRManager::GetClients()->GetQualityData(&m_qualityInfo);
}

void CPVRGUIInfo::Start(void)
{
  ResetProperties();
  Create();
  SetName("XBMC PVR GUI info");
  SetPriority(-1);
}

void CPVRGUIInfo::Stop(void)
{
  StopThread();
}

void CPVRGUIInfo::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("timers"))
    UpdateTimersCache();
}

bool CPVRGUIInfo::AddonInfoToggle(void)
{
  if (m_iAddonInfoToggleStart == 0)
  {
    m_iAddonInfoToggleStart = CTimeUtils::GetTimeMS();
    m_iAddonInfoToggleCurrent = 0;
    return true;
  }

  if (CTimeUtils::GetTimeMS() - m_iAddonInfoToggleStart > INFO_TOGGLE_TIME)
  {
    if (((int) ++m_iAddonInfoToggleCurrent) > m_iActiveClients - 1)
      m_iAddonInfoToggleCurrent = 0;

    return true;
  }

  return false;
}

bool CPVRGUIInfo::TimerInfoToggle(void)
{
  if (m_iTimerInfoToggleStart == 0)
  {
    m_iTimerInfoToggleStart = CTimeUtils::GetTimeMS();
    m_iTimerInfoToggleCurrent = 0;
    return true;
  }

  if (CTimeUtils::GetTimeMS() - m_iTimerInfoToggleStart > INFO_TOGGLE_TIME)
  {
    if (++m_iTimerInfoToggleCurrent > m_NowRecording.size() - 1)
      m_iTimerInfoToggleCurrent = 0;

    return true;
  }

  return false;
}

void CPVRGUIInfo::Process(void)
{
  unsigned int mLoop(0);
  CPVRManager *manager = CPVRManager::Get();

  /* updated on request */
  manager->GetTimers()->AddObserver(this);
  UpdateTimersCache();

  while (!m_bStop)
  {
    UpdateQualityData();
    Sleep(0);

    UpdateMisc();
    Sleep(0);

    UpdatePlayingTag();
    Sleep(0);

    UpdateTimersToggle();
    Sleep(0);

    if (mLoop % 10 == 0)
      UpdateBackendCache();    /* updated every 10 iterations */

    if (++mLoop == 1000)
      mLoop = 0;

    Sleep(1000);
  }
}

void CPVRGUIInfo::UpdateQualityData(void)
{
  CSingleLock lock(m_critSection);

  CPVRManager::GetClients()->GetQualityData(&m_qualityInfo);
}

void CPVRGUIInfo::UpdateMisc(void)
{
  CSingleLock lock(m_critSection);

  m_strPlayingClientName = CPVRManager::GetClients()->GetPlayingClientName();
  m_bHasRecordings = CPVRManager::GetRecordings()->GetNumRecordings() > 0;
}

const char* CPVRGUIInfo::TranslateCharInfo(DWORD dwInfo) const
{
  CSingleLock lock(m_critSection);

  if      (dwInfo == PVR_NOW_RECORDING_TITLE)     return CharInfoActiveTimerTitle();
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)   return CharInfoActiveTimerChannelName();
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)  return CharInfoActiveTimerDateTime();
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return CharInfoNextTimerTitle();
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return CharInfoNextTimerChannelName();
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return CharInfoNextTimerDateTime();
  else if (dwInfo == PVR_PLAYING_DURATION)        return CharInfoPlayingDuration();
  else if (dwInfo == PVR_PLAYING_TIME)            return CharInfoPlayingTime();
  else if (dwInfo == PVR_NEXT_TIMER)              return CharInfoNextTimer();
  else if (dwInfo == PVR_ACTUAL_STREAM_VIDEO_BR)  return CharInfoVideoBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_AUDIO_BR)  return CharInfoAudioBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_DOLBY_BR)  return CharInfoDolbyBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG)       return CharInfoSignal();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR)       return CharInfoSNR();
  else if (dwInfo == PVR_ACTUAL_STREAM_BER)       return CharInfoBER();
  else if (dwInfo == PVR_ACTUAL_STREAM_UNC)       return CharInfoUNC();
  else if (dwInfo == PVR_ACTUAL_STREAM_CLIENT)    return CharInfoPlayingClientName();
  else if (dwInfo == PVR_ACTUAL_STREAM_DEVICE)    return CharInfoFrontendName();
  else if (dwInfo == PVR_ACTUAL_STREAM_STATUS)    return CharInfoFrontendStatus();
  else if (dwInfo == PVR_ACTUAL_STREAM_CRYPTION)  return CharInfoEncryption();
  else if (dwInfo == PVR_BACKEND_NAME)            return CharInfoBackendName();
  else if (dwInfo == PVR_BACKEND_VERSION)         return CharInfoBackendVersion();
  else if (dwInfo == PVR_BACKEND_HOST)            return CharInfoBackendHost();
  else if (dwInfo == PVR_BACKEND_DISKSPACE)       return CharInfoBackendDiskspace();
  else if (dwInfo == PVR_BACKEND_CHANNELS)        return CharInfoBackendChannels();
  else if (dwInfo == PVR_BACKEND_TIMERS)          return CharInfoBackendTimers();
  else if (dwInfo == PVR_BACKEND_RECORDINGS)      return CharInfoBackendRecordings();
  else if (dwInfo == PVR_BACKEND_NUMBER)          return CharInfoBackendNumber();
  else if (dwInfo == PVR_TOTAL_DISKSPACE)         return CharInfoTotalDiskSpace();
  return "";
}

bool CPVRGUIInfo::TranslateBoolInfo(DWORD dwInfo) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (dwInfo == PVR_IS_RECORDING)
    bReturn = CPVRManager::Get()->IsStarted() && m_bIsRecording;
  else if (dwInfo == PVR_HAS_TIMER)
    bReturn = CPVRManager::Get()->IsStarted() && m_bHasTimers;
  else if (dwInfo == PVR_IS_PLAYING_TV)
    bReturn = CPVRManager::Get()->IsStarted() && CPVRManager::GetClients()->IsPlayingTV();
  else if (dwInfo == PVR_IS_PLAYING_RADIO)
    bReturn = CPVRManager::Get()->IsStarted() && CPVRManager::GetClients()->IsPlayingRadio();
  else if (dwInfo == PVR_IS_PLAYING_RECORDING)
    bReturn = CPVRManager::Get()->IsStarted() && CPVRManager::GetClients()->IsPlayingRecording();
  else if (dwInfo == PVR_ACTUAL_STREAM_ENCRYPTED)
    bReturn = CPVRManager::Get()->IsStarted() && CPVRManager::GetClients()->IsEncrypted();

  return bReturn;
}

int CPVRGUIInfo::TranslateIntInfo(DWORD dwInfo) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  if (dwInfo == PVR_PLAYING_PROGRESS)
    iReturn = (int) ((float) GetStartTime() / GetTotalTime() * 100);
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG_PROGR)
    iReturn = CPVRManager::GetClients()->GetSignalLevel();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR_PROGR)
    iReturn = CPVRManager::GetClients()->GetSNR();

  return iReturn;
}

const char *CPVRGUIInfo::CharInfoActiveTimerTitle(void) const
{
  static CStdString strReturn = m_strActiveTimerTitle;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoActiveTimerChannelName(void) const
{
  static CStdString strReturn = m_strActiveTimerChannelName;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoActiveTimerDateTime(void) const
{
  static CStdString strReturn = m_strActiveTimerTime;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoNextTimerTitle(void) const
{
  static CStdString strReturn = m_strNextRecordingTitle;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoNextTimerChannelName(void) const
{
  static CStdString strReturn = m_strNextRecordingChannelName;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoNextTimerDateTime(void) const
{
  static CStdString strReturn = m_strNextRecordingTime;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoPlayingDuration(void) const
{
  static CStdString strReturn = StringUtils::SecondsToTimeString(GetTotalTime() / 1000, TIME_FORMAT_GUESS);
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoPlayingTime(void) const
{
  static CStdString strReturn = StringUtils::SecondsToTimeString(GetStartTime()/1000, TIME_FORMAT_GUESS);
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoNextTimer(void) const
{
  static CStdString strReturn = m_strNextTimerInfo;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendNumber(void) const
{
  static CStdString backendClients;
  if (m_iActiveClients > 0)
    backendClients.Format("%u %s %u", m_iAddonInfoToggleCurrent+1, g_localizeStrings.Get(20163), m_iActiveClients);
  else
    backendClients = g_localizeStrings.Get(14023);

  return backendClients.c_str();
}

const char *CPVRGUIInfo::CharInfoTotalDiskSpace(void) const
{
  static CStdString strReturn = m_strTotalDiskspace;
  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoVideoBR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.dVideoBitrate > 0)
    strReturn.Format("%.2f Mbit/s", m_qualityInfo.dVideoBitrate);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoAudioBR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.dAudioBitrate > 0)
    strReturn.Format("%.0f kbit/s", m_qualityInfo.dAudioBitrate);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoDolbyBR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.dDolbyBitrate > 0)
    strReturn.Format("%.0f kbit/s", m_qualityInfo.dDolbyBitrate);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoSignal(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.iSignal > 0)
    strReturn.Format("%d %%", m_qualityInfo.iSignal / 655);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoSNR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.iSNR > 0)
    strReturn.Format("%d %%", m_qualityInfo.iSNR / 655);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBER(void) const
{
  static CStdString strReturn = "";
  strReturn.Format("%08X", m_qualityInfo.iBER);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoUNC(void) const
{
  static CStdString strReturn = "";
  strReturn.Format("%08X", m_qualityInfo.iUNC);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoFrontendName(void) const
{
  static CStdString strReturn = m_qualityInfo.strAdapterName;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoFrontendStatus(void) const
{
  static CStdString strReturn = m_qualityInfo.strAdapterStatus;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendName(void) const
{
  static CStdString strReturn = m_strBackendName;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendVersion(void) const
{
  static CStdString strReturn = m_strBackendVersion;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendHost(void) const
{
  static CStdString strReturn = m_strBackendHost;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendDiskspace(void) const
{
  static CStdString strReturn = m_strBackendDiskspace;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendChannels(void) const
{
  static CStdString strReturn = m_strBackendChannels;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendTimers(void) const
{
  static CStdString strReturn = m_strBackendTimers;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoBackendRecordings(void) const
{
  static CStdString strReturn = m_strBackendRecordings;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}

const char *CPVRGUIInfo::CharInfoPlayingClientName(void) const
{
  static CStdString strReturn = m_strPlayingClientName;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn.c_str();
}


const char *CPVRGUIInfo::CharInfoEncryption(void) const
{
  static CStdString strReturn = "";

  CPVRChannel channel;
  if (CPVRManager::GetClients()->GetPlayingChannel(&channel))
    strReturn = channel.EncryptionName();

  return strReturn.c_str();
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

  CPVRClients *clients = CPVRManager::GetClients();
  std::map<long, CStdString> activeClients;
  clients->GetClients(&activeClients);

  m_iActiveClients = activeClients.size();
  if (m_iActiveClients > 0)
  {
    std::map<long, CStdString>::iterator activeClient = activeClients.begin();
    for (unsigned int i = 0; i < m_iAddonInfoToggleCurrent; i++)
      activeClient++;

    long long kBTotal = 0;
    long long kBUsed  = 0;
    CLIENTMAPITR current = clients->m_clientMap.find(activeClient->first);
    if (current == clients->m_clientMap.end() ||
        !current->second->ReadyToUse())
      return;

    if (current->second->GetDriveSpace(&kBTotal, &kBUsed) == PVR_ERROR_NO_ERROR)
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

    int NumChannels = current->second->GetChannelsAmount();
    if (NumChannels >= 0)
      m_strBackendChannels.Format("%i", NumChannels);
    else
      m_strBackendChannels = g_localizeStrings.Get(161);

    int NumTimers = current->second->GetTimersAmount();
    if (NumTimers >= 0)
      m_strBackendTimers.Format("%i", NumTimers);
    else
      m_strBackendTimers = g_localizeStrings.Get(161);

    int NumRecordings = current->second->GetRecordingsAmount();
    if (NumRecordings >= 0)
      m_strBackendRecordings.Format("%i", NumRecordings);
    else
      m_strBackendRecordings = g_localizeStrings.Get(161);

    m_strBackendName         = current->second->GetBackendName();
    m_strBackendVersion      = current->second->GetBackendVersion();
    m_strBackendHost         = current->second->GetConnectionString();
  }
}

void CPVRGUIInfo::UpdateTimersCache(void)
{
  CSingleLock lock(m_critSection);

  /* reset values */
  m_strActiveTimerTitle         = "";
  m_strActiveTimerChannelName   = "";
  m_strActiveTimerTime          = "";
  m_strNextRecordingTitle       = "";
  m_strNextRecordingChannelName = "";
  m_strNextRecordingTime        = "";
  m_strNextTimerInfo            = "";
  m_bIsRecording = false;
  m_NowRecording.clear();
  m_NextRecording = NULL;

  CPVRTimers *timers = CPVRManager::GetTimers();

  /* fill values */
  timers->GetActiveTimers(&m_NowRecording);
  m_bHasTimers = m_NowRecording.size() > 0;
  if (m_bHasTimers)
  {
    /* set the active timer info locally if we're recording right now */
    m_bIsRecording = timers->IsRecording();
    if (m_bIsRecording)
    {
      const CPVRTimerInfoTag *tag = m_NowRecording.at(0);
      m_strActiveTimerTitle.Format("%s",       tag->m_strTitle);
      m_strActiveTimerChannelName.Format("%s", tag->ChannelName());
      m_strActiveTimerTime.Format("%s",        tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
    }

    /* set the next timer info locally if there is a next timer */
    m_NextRecording = timers->GetNextActiveTimer();
    if (m_NextRecording != NULL)
    {
      m_strNextRecordingTitle.Format("%s",       m_NextRecording->m_strTitle);
      m_strNextRecordingChannelName.Format("%s", m_NextRecording->ChannelName());
      m_strNextRecordingTime.Format("%s",        m_NextRecording->StartAsLocalTime().GetAsLocalizedDateTime(false, false));

      m_strNextTimerInfo.Format("%s %s %s %s",
          g_localizeStrings.Get(19106),
          m_NextRecording->StartAsLocalTime().GetAsLocalizedDate(true),
          g_localizeStrings.Get(19107),
          m_NextRecording->StartAsLocalTime().GetAsLocalizedTime("HH:mm", false));
    }
  }
}

void CPVRGUIInfo::UpdateTimersToggle(void)
{
  CSingleLock lock(m_critSection);

  if (!TimerInfoToggle())
    return;

  if (m_NowRecording.size() > 0 && m_iTimerInfoToggleCurrent < m_NowRecording.size())
  {
    const CPVRTimerInfoTag *tag = m_NowRecording.at(m_iTimerInfoToggleCurrent);
    m_strActiveTimerTitle       = tag->m_strTitle;
    m_strActiveTimerChannelName = tag->ChannelName();
    m_strActiveTimerTime        = tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false);
  }
  else
  {
    m_strActiveTimerTitle       = "";
    m_strActiveTimerChannelName = "";
    m_strActiveTimerTime        = "";
  }
}

int CPVRGUIInfo::GetTotalTime(void) const
{
  CSingleLock lock(m_critSection);

  return m_playingEpgTag ? m_playingEpgTag->GetDuration() * 1000 : 0;
}

int CPVRGUIInfo::GetStartTime(void) const
{
  CSingleLock lock(m_critSection);

  if (m_playingEpgTag)
  {
    /* Calculate here the position we have of the running live TV event.
     * "position in ms" = ("current local time" - "event start local time") * 1000
     */
    CDateTimeSpan time = CDateTime::GetCurrentDateTime() - m_playingEpgTag->StartAsLocalTime();
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

void CPVRGUIInfo::UpdatePlayingTag(void)
{
  CSingleLock lock(m_critSection);

  if (m_playingEpgTag && m_playingEpgTag->IsActive())
    return;

  CPVRChannel currentChannel;
  if (CPVRManager::Get()->GetCurrentChannel(&currentChannel))
  {
    m_playingEpgTag = currentChannel.GetEPGNow();
    CPVRManager::Get()->UpdateCurrentFile();
  }
}

bool CPVRGUIInfo::IsRecording(void) const
{
  CSingleLock lock(m_critSection);

  return m_bIsRecording;
}

bool CPVRGUIInfo::HasTimers(void) const
{
  CSingleLock lock(m_critSection);

  return m_bHasTimers;
}

const CPVREpgInfoTag *CPVRGUIInfo::GetPlayingTag(void) const
{
  CSingleLock lock(m_critSection);

  return m_playingEpgTag;
}
