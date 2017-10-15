/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRGUIInfo.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "guiinfo/GUIInfoLabels.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;

CPVRGUIInfo::CPVRGUIInfo(void) :
    CThread("PVRGUIInfo")
{
  ResetProperties();
}

CPVRGUIInfo::~CPVRGUIInfo(void)
{
}

void CPVRGUIInfo::ResetProperties(void)
{
  CSingleLock lock(m_critSection);
  m_anyTimersInfo.ResetProperties();
  m_tvTimersInfo.ResetProperties();
  m_radioTimersInfo.ResetProperties();
  m_bHasTVRecordings            = false;
  m_bHasRadioRecordings         = false;
  m_iCurrentActiveClient        = 0;
  m_strPlayingClientName        .clear();
  m_strBackendName              .clear();
  m_strBackendVersion           .clear();
  m_strBackendHost              .clear();
  m_strBackendTimers            .clear();
  m_strBackendRecordings        .clear();
  m_strBackendDeletedRecordings .clear();
  m_strBackendChannels          .clear();
  m_iBackendDiskTotal           = 0;
  m_iBackendDiskUsed            = 0;
  m_iDuration                   = 0;
  m_bIsPlayingTV                = false;
  m_bIsPlayingRadio             = false;
  m_bIsPlayingRecording         = false;
  m_bIsPlayingEpgTag            = false;
  m_bIsPlayingEncryptedStream   = false;
  m_bHasTVChannels              = false;
  m_bHasRadioChannels           = false;
  m_bIsTimeshifting             = false;
  m_iTimeshiftStartTime         = time_t(0);
  m_iTimeshiftEndTime           = time_t(0);
  m_iTimeshiftPlayTime          = time_t(0);
  m_strTimeshiftStartTime.clear();
  m_strTimeshiftEndTime.clear();
  m_strTimeshiftPlayTime.clear();

  ResetPlayingTag();
  ClearQualityInfo(m_qualityInfo);
  ClearDescrambleInfo(m_descrambleInfo);

  m_updateBackendCacheRequested = false;
}

void CPVRGUIInfo::ClearQualityInfo(PVR_SIGNAL_STATUS &qualityInfo)
{
  memset(&qualityInfo, 0, sizeof(qualityInfo));
  strncpy(qualityInfo.strAdapterName, g_localizeStrings.Get(13106).c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
  strncpy(qualityInfo.strAdapterStatus, g_localizeStrings.Get(13106).c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
}

void CPVRGUIInfo::ClearDescrambleInfo(PVR_DESCRAMBLE_INFO &descrambleInfo)
{
  descrambleInfo = {0};
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
  CServiceBroker::GetPVRManager().UnregisterObserver(this);
}

void CPVRGUIInfo::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageTimers || msg == ObservableMessageTimersReset)
    UpdateTimersCache();
}

void CPVRGUIInfo::Process(void)
{
  unsigned int mLoop(0);
  int toggleInterval = g_advancedSettings.m_iPVRInfoToggleInterval / 1000;

  /* updated on request */
  CServiceBroker::GetPVRManager().RegisterObserver(this);
  UpdateTimersCache();

  /* update the backend cache once initially */
  m_updateBackendCacheRequested = true;

  while (!g_application.m_bStop && !m_bStop)
  {
    if (!m_bStop)
      UpdateQualityData();
    Sleep(0);

    if (!m_bStop)
      UpdateDescrambleData();
    Sleep(0);

    if (!m_bStop)
      UpdateMisc();
    Sleep(0);

    if (!m_bStop)
      UpdateTimeshift();
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

    // Update the backend cache every toggleInterval seconds
    if (!m_bStop && mLoop % toggleInterval == 0)
      UpdateBackendCache();

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
  PVR_SIGNAL_STATUS qualityInfo;
  ClearQualityInfo(qualityInfo);

  PVR_CLIENT client;
  if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_PVRPLAYBACK_SIGNALQUALITY) &&
      CServiceBroker::GetPVRManager().Clients()->GetPlayingClient(client))
  {
    client->SignalQuality(qualityInfo);
  }

  memcpy(&m_qualityInfo, &qualityInfo, sizeof(m_qualityInfo));
}

void CPVRGUIInfo::UpdateDescrambleData(void)
{
  PVR_DESCRAMBLE_INFO descrambleInfo;
  ClearDescrambleInfo(descrambleInfo);

  PVR_CLIENT client;
  if (CServiceBroker::GetPVRManager().Clients()->GetPlayingClient(client) &&
      client->GetDescrambleInfo(descrambleInfo))
    memcpy(&m_descrambleInfo, &descrambleInfo, sizeof(m_descrambleInfo));
}

void CPVRGUIInfo::UpdateMisc(void)
{
  bool bStarted = CServiceBroker::GetPVRManager().IsStarted();
  /* safe to fetch these unlocked, since they're updated from the same thread as this one */
  std::string strPlayingClientName     = bStarted ? CServiceBroker::GetPVRManager().Clients()->GetPlayingClientName() : "";
  bool       bHasTVRecordings          = bStarted && CServiceBroker::GetPVRManager().Recordings()->GetNumTVRecordings() > 0;
  bool       bHasRadioRecordings       = bStarted && CServiceBroker::GetPVRManager().Recordings()->GetNumRadioRecordings() > 0;
  bool       bIsPlayingTV              = bStarted && CServiceBroker::GetPVRManager().Clients()->IsPlayingTV();
  bool       bIsPlayingRadio           = bStarted && CServiceBroker::GetPVRManager().Clients()->IsPlayingRadio();
  bool       bIsPlayingRecording       = bStarted && CServiceBroker::GetPVRManager().Clients()->IsPlayingRecording();
  bool       bIsPlayingEpgTag          = bStarted && CServiceBroker::GetPVRManager().Clients()->IsPlayingEpgTag();
  bool       bIsPlayingEncryptedStream = bStarted && CServiceBroker::GetPVRManager().Clients()->IsEncrypted();
  bool       bHasTVChannels            = bStarted && CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV()->HasChannels();
  bool       bHasRadioChannels         = bStarted && CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllRadio()->HasChannels();
  std::string strPlayingTVGroup        = (bStarted && bIsPlayingTV) ? CServiceBroker::GetPVRManager().GetPlayingGroup(false)->GroupName() : "";

  CSingleLock lock(m_critSection);
  m_strPlayingClientName      = strPlayingClientName;
  m_bHasTVRecordings          = bHasTVRecordings;
  m_bHasRadioRecordings       = bHasRadioRecordings;
  m_bIsPlayingTV              = bIsPlayingTV;
  m_bIsPlayingRadio           = bIsPlayingRadio;
  m_bIsPlayingRecording       = bIsPlayingRecording;
  m_bIsPlayingEpgTag          = bIsPlayingEpgTag;
  m_bIsPlayingEncryptedStream = bIsPlayingEncryptedStream;
  m_bHasTVChannels            = bHasTVChannels;
  m_bHasRadioChannels         = bHasRadioChannels;
  m_strPlayingTVGroup         = strPlayingTVGroup;
}

void CPVRGUIInfo::UpdateTimeshift(void)
{
  bool bStarted = CServiceBroker::GetPVRManager().IsStarted();

  bool bIsTimeshifting = bStarted && CServiceBroker::GetPVRManager().Clients()->IsTimeshifting();
  CDateTime tmp;
  time_t iTimeshiftStartTime = CServiceBroker::GetPVRManager().Clients()->GetBufferTimeStart();
  tmp.SetFromUTCDateTime(iTimeshiftStartTime);
  std::string strTimeshiftStartTime = tmp.GetAsLocalizedTime("", false);

  time_t iTimeshiftEndTime = CServiceBroker::GetPVRManager().Clients()->GetBufferTimeEnd();
  tmp.SetFromUTCDateTime(iTimeshiftEndTime);
  std::string strTimeshiftEndTime = tmp.GetAsLocalizedTime("", false);

  time_t iTimeshiftPlayTime = CServiceBroker::GetPVRManager().Clients()->GetPlayingTime();
  tmp.SetFromUTCDateTime(iTimeshiftPlayTime);
  std::string strTimeshiftPlayTime = tmp.GetAsLocalizedTime("", true);

  CSingleLock lock(m_critSection);
  m_bIsTimeshifting = bIsTimeshifting;
  m_iTimeshiftStartTime = iTimeshiftStartTime;
  m_iTimeshiftEndTime = iTimeshiftEndTime;
  m_iTimeshiftPlayTime = iTimeshiftPlayTime;
  m_strTimeshiftStartTime = strTimeshiftStartTime;
  m_strTimeshiftEndTime = strTimeshiftEndTime;
  m_strTimeshiftPlayTime = strTimeshiftPlayTime;
}

bool CPVRGUIInfo::TranslateCharInfo(DWORD dwInfo, std::string &strValue) const
{
  bool bReturn(true);
  CSingleLock lock(m_critSection);

  switch(dwInfo)
  {
  case PVR_NOW_RECORDING_TITLE:
    m_anyTimersInfo.CharInfoActiveTimerTitle(strValue);
    break;
  case PVR_NOW_RECORDING_CHANNEL:
    m_anyTimersInfo.CharInfoActiveTimerChannelName(strValue);
    break;
  case PVR_NOW_RECORDING_CHAN_ICO:
    m_anyTimersInfo.CharInfoActiveTimerChannelIcon(strValue);
    break;
  case PVR_NOW_RECORDING_DATETIME:
    m_anyTimersInfo.CharInfoActiveTimerDateTime(strValue);
    break;
  case PVR_NEXT_RECORDING_TITLE:
    m_anyTimersInfo.CharInfoNextTimerTitle(strValue);
    break;
  case PVR_NEXT_RECORDING_CHANNEL:
    m_anyTimersInfo.CharInfoNextTimerChannelName(strValue);
    break;
  case PVR_NEXT_RECORDING_CHAN_ICO:
    m_anyTimersInfo.CharInfoNextTimerChannelIcon(strValue);
    break;
  case PVR_NEXT_RECORDING_DATETIME:
    m_anyTimersInfo.CharInfoNextTimerDateTime(strValue);
    break;
  case PVR_TV_NOW_RECORDING_TITLE:
    m_tvTimersInfo.CharInfoActiveTimerTitle(strValue);
    break;
  case PVR_TV_NOW_RECORDING_CHANNEL:
    m_tvTimersInfo.CharInfoActiveTimerChannelName(strValue);
    break;
  case PVR_TV_NOW_RECORDING_CHAN_ICO:
    m_tvTimersInfo.CharInfoActiveTimerChannelIcon(strValue);
    break;
  case PVR_TV_NOW_RECORDING_DATETIME:
    m_tvTimersInfo.CharInfoActiveTimerDateTime(strValue);
    break;
  case PVR_TV_NEXT_RECORDING_TITLE:
    m_tvTimersInfo.CharInfoNextTimerTitle(strValue);
    break;
  case PVR_TV_NEXT_RECORDING_CHANNEL:
    m_tvTimersInfo.CharInfoNextTimerChannelName(strValue);
    break;
  case PVR_TV_NEXT_RECORDING_CHAN_ICO:
    m_tvTimersInfo.CharInfoNextTimerChannelIcon(strValue);
    break;
  case PVR_TV_NEXT_RECORDING_DATETIME:
    m_tvTimersInfo.CharInfoNextTimerDateTime(strValue);
    break;
  case PVR_RADIO_NOW_RECORDING_TITLE:
    m_radioTimersInfo.CharInfoActiveTimerTitle(strValue);
    break;
  case PVR_RADIO_NOW_RECORDING_CHANNEL:
    m_radioTimersInfo.CharInfoActiveTimerChannelName(strValue);
    break;
  case PVR_RADIO_NOW_RECORDING_CHAN_ICO:
    m_radioTimersInfo.CharInfoActiveTimerChannelIcon(strValue);
    break;
  case PVR_RADIO_NOW_RECORDING_DATETIME:
    m_radioTimersInfo.CharInfoActiveTimerDateTime(strValue);
    break;
  case PVR_RADIO_NEXT_RECORDING_TITLE:
    m_radioTimersInfo.CharInfoNextTimerTitle(strValue);
    break;
  case PVR_RADIO_NEXT_RECORDING_CHANNEL:
    m_radioTimersInfo.CharInfoNextTimerChannelName(strValue);
    break;
  case PVR_RADIO_NEXT_RECORDING_CHAN_ICO:
    m_radioTimersInfo.CharInfoNextTimerChannelIcon(strValue);
    break;
  case PVR_RADIO_NEXT_RECORDING_DATETIME:
    m_radioTimersInfo.CharInfoNextTimerDateTime(strValue);
    break;
  case PVR_PLAYING_DURATION:
    CharInfoPlayingDuration(strValue);
    break;
  case PVR_PLAYING_TIME:
    CharInfoPlayingTime(strValue);
    break;
  case PVR_NEXT_TIMER:
    m_anyTimersInfo.CharInfoNextTimer(strValue);
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
  case PVR_ACTUAL_STREAM_SERVICE:
    CharInfoService(strValue);
    break;
  case PVR_ACTUAL_STREAM_MUX:
    CharInfoMux(strValue);
    break;
  case PVR_ACTUAL_STREAM_PROVIDER:
    CharInfoProvider(strValue);
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
  case PVR_BACKEND_DELETED_RECORDINGS:
    CharInfoBackendDeletedRecordings(strValue);
    break;
  case PVR_BACKEND_NUMBER:
    CharInfoBackendNumber(strValue);
    break;
  case PVR_TOTAL_DISKSPACE:
    CharInfoTotalDiskSpace(strValue);
    break;
  case PVR_TIMESHIFT_START_TIME:
    CharInfoTimeshiftStartTime(strValue);
    break;
  case PVR_TIMESHIFT_END_TIME:
    CharInfoTimeshiftEndTime(strValue);
    break;
  case PVR_TIMESHIFT_PLAY_TIME:
    CharInfoTimeshiftPlayTime(strValue);
    break;
  default:
    strValue.clear();
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
    bReturn = m_anyTimersInfo.HasRecordingTimers();
    break;
  case PVR_IS_RECORDING_TV:
    bReturn = m_tvTimersInfo.HasRecordingTimers();
    break;
  case PVR_IS_RECORDING_RADIO:
    bReturn = m_radioTimersInfo.HasRecordingTimers();
    break;
  case PVR_HAS_TIMER:
    bReturn = m_anyTimersInfo.HasTimers();
    break;
  case PVR_HAS_TV_TIMER:
    bReturn = m_tvTimersInfo.HasTimers();
    break;
  case PVR_HAS_RADIO_TIMER:
    bReturn = m_radioTimersInfo.HasTimers();
    break;
  case PVR_HAS_TV_CHANNELS:
    bReturn = m_bHasTVChannels;
    break;
  case PVR_HAS_RADIO_CHANNELS:
    bReturn = m_bHasRadioChannels;
    break;
  case PVR_HAS_NONRECORDING_TIMER:
    bReturn = m_anyTimersInfo.HasNonRecordingTimers();
    break;
  case PVR_HAS_NONRECORDING_TV_TIMER:
    bReturn = m_tvTimersInfo.HasNonRecordingTimers();
    break;
  case PVR_HAS_NONRECORDING_RADIO_TIMER:
    bReturn = m_radioTimersInfo.HasNonRecordingTimers();
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
  case PVR_IS_PLAYING_EPGTAG:
    bReturn = m_bIsPlayingEpgTag;
    break;
  case PVR_ACTUAL_STREAM_ENCRYPTED:
    bReturn = m_bIsPlayingEncryptedStream;
    break;
  case PVR_IS_TIMESHIFTING:
    bReturn = m_bIsTimeshifting;
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
  else if (dwInfo == PVR_BACKEND_DISKSPACE_PROGR)
  {
    if (m_iBackendDiskTotal > 0)
      iReturn = static_cast<int>(100 * m_iBackendDiskUsed / m_iBackendDiskTotal);
    else
      iReturn = 0xFF;
  }
  else if (dwInfo == PVR_TIMESHIFT_PROGRESS)
  {
    iReturn = static_cast<int>(static_cast<float>(m_iTimeshiftPlayTime - m_iTimeshiftStartTime) /
                               (m_iTimeshiftEndTime - m_iTimeshiftStartTime) * 100);
  }

  return iReturn;
}

#define GET_CURRENT_VIDEO_LABEL_WITH_CONDITION(value_getter_function, condition)  \
  if (channel)                        \
    epgTag = channel->GetEPGNow();    \
                                      \
  if (epgTag && condition)            \
  {                                   \
    strValue = value_getter_function; \
    return true;                      \
  }                                   \
  return false;                       \

#define GET_CURRENT_VIDEO_LABEL(value_getter_function)  \
  GET_CURRENT_VIDEO_LABEL_WITH_CONDITION(value_getter_function, true)

#define GET_CURRENT_VIDEO_LABEL_OR_DEFAULT(value_getter_function, default_value_getter_function) \
  if (channel)                                \
    epgTag = channel->GetEPGNow();            \
                                              \
  if (epgTag)                                 \
    strValue = value_getter_function;         \
  else                                        \
    strValue = default_value_getter_function; \
                                              \
  return true;                                \

#define GET_NEXT_VIDEO_LABEL_WITH_CONDITION(value_getter_function, condition)  \
  if (channel)                        \
    epgTag = channel->GetEPGNext();   \
                                      \
  if (epgTag && condition)            \
  {                                   \
    strValue = value_getter_function; \
    return true;                      \
  }                                   \
  return false;                       \

#define GET_NEXT_VIDEO_LABEL(value_getter_function)  \
  GET_NEXT_VIDEO_LABEL_WITH_CONDITION(value_getter_function, true)

#define GET_NEXT_VIDEO_LABEL_OR_DEFAULT(value_getter_function, default_value_getter_function) \
  if (channel)                                \
    epgTag = channel->GetEPGNext();           \
                                              \
  if (epgTag)                                 \
    strValue = value_getter_function;         \
  else                                        \
    strValue = default_value_getter_function; \
                                              \
  return true;                                \

bool CPVRGUIInfo::GetVideoLabel(const CFileItem &item, int iLabel, std::string &strValue) const
{
  const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
  if (recording)
  {
    // Note: CPVRRecoding is derived from CVideoInfoTag. All base class properties will be handled
    //       by CGUIInfoManager. Only properties introduced by CPVRRecording need to be handled here.
    switch (iLabel)
    {
      // 'Now playing' infos
      case VIDEOPLAYER_STARTTIME:
      {
        strValue = recording->RecordingTimeAsLocalTime().GetAsLocalizedTime("", false);
        return true;
      }
      case VIDEOPLAYER_ENDTIME:
      {
        strValue = recording->EndTimeAsLocalTime().GetAsLocalizedTime("", false);
        return true;
      }
      case VIDEOPLAYER_EPISODENAME:
      {
        strValue = recording->EpisodeName();
        return true;
      }

      // General channel infos
      case VIDEOPLAYER_CHANNEL_NAME:
      {
        strValue = recording->m_strChannelName;
        return true;
      }
      case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
      {
        const CPVRChannelPtr channel = recording->Channel();
        if (channel)
        {
          strValue = StringUtils::Format("%i", channel->SubChannelNumber());
          return true;
        }
        break;
      }
      case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
      {
        const CPVRChannelPtr channel = recording->Channel();
        if (channel)
        {
          strValue = channel->FormattedChannelNumber();
          return true;
        }
        break;
      }
      case VIDEOPLAYER_CHANNEL_GROUP:
      {
        if (!recording->IsRadio())
        {
          strValue = CServiceBroker::GetPVRManager().GetPlayingTVGroupName();
          return true;
        }
        break;
      }
    }
    return false;
  }

  CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
  CPVREpgInfoTagPtr epgTag(item.GetEPGInfoTag());

  if (channel || epgTag)
  {
    switch (iLabel)
    {
      // 'Now playing' infos
      case VIDEOPLAYER_TITLE:
      {
        GET_CURRENT_VIDEO_LABEL_OR_DEFAULT(epgTag->Title(),
                                           CServiceBroker::GetSettings().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE)
                                           ? ""
                                           : g_localizeStrings.Get(19055)); // no information available
      }
      case VIDEOPLAYER_GENRE:
      {
        GET_CURRENT_VIDEO_LABEL(StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator));
      }
      case VIDEOPLAYER_PLOT:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->Plot());
      }
      case VIDEOPLAYER_PLOT_OUTLINE:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->PlotOutline());
      }
      case VIDEOPLAYER_STARTTIME:
      {
        GET_CURRENT_VIDEO_LABEL_OR_DEFAULT(epgTag->StartAsLocalTime().GetAsLocalizedTime("", false),
                                           CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false)); // @todo: really current time?
      }
      case VIDEOPLAYER_ENDTIME:
      {
        GET_CURRENT_VIDEO_LABEL_OR_DEFAULT(epgTag->EndAsLocalTime().GetAsLocalizedTime("", false),
                                           CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false));  // @todo: really current time?
      }
      case VIDEOPLAYER_IMDBNUMBER:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->IMDBNumber());
      }
      case VIDEOPLAYER_ORIGINALTITLE:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->OriginalTitle());
      }
      case VIDEOPLAYER_YEAR:
      {
        GET_CURRENT_VIDEO_LABEL_WITH_CONDITION(StringUtils::Format("%i", epgTag->Year()), epgTag->Year() > 0);
      }
      case VIDEOPLAYER_EPISODE:
      {
        if (channel)
          epgTag = channel->GetEPGNow();

        if (epgTag && epgTag->EpisodeNumber() > 0)
        {
          if (epgTag->SeriesNumber() == 0) // prefix episode with 'S'
            strValue = StringUtils::Format("S%i", epgTag->EpisodeNumber());
          else
            strValue = StringUtils::Format("%i", epgTag->EpisodeNumber());

          return true;
        }
        break;
      }
      case VIDEOPLAYER_SEASON:
      {
        GET_CURRENT_VIDEO_LABEL_WITH_CONDITION(StringUtils::Format("%i", epgTag->SeriesNumber()), epgTag->SeriesNumber() > 0);
      }
      case VIDEOPLAYER_EPISODENAME:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->EpisodeName());
      }
      case VIDEOPLAYER_CAST:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->Cast());
      }
      case VIDEOPLAYER_DIRECTOR:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->Director());
      }
      case VIDEOPLAYER_WRITER:
      {
        GET_CURRENT_VIDEO_LABEL(epgTag->Writer());
      }
      case VIDEOPLAYER_PARENTAL_RATING:
      {
        GET_CURRENT_VIDEO_LABEL_WITH_CONDITION(StringUtils::Format("%i", epgTag->ParentalRating()), epgTag->ParentalRating() > 0);
      }

      // 'Next playing' infos
      case VIDEOPLAYER_NEXT_TITLE:
      {
        GET_NEXT_VIDEO_LABEL_OR_DEFAULT(epgTag->Title(),
                                        CServiceBroker::GetSettings().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE)
                                        ? ""
                                        : g_localizeStrings.Get(19055)); // no information available
      }
      case VIDEOPLAYER_NEXT_GENRE:
      {
        GET_NEXT_VIDEO_LABEL(StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator));
      }
      case VIDEOPLAYER_NEXT_PLOT:
      {
        GET_NEXT_VIDEO_LABEL(epgTag->Plot());
      }
      case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
      {
        GET_NEXT_VIDEO_LABEL(epgTag->PlotOutline());
      }
      case VIDEOPLAYER_NEXT_STARTTIME:
      {
        GET_NEXT_VIDEO_LABEL(epgTag->StartAsLocalTime().GetAsLocalizedTime("", false));
      }
      case VIDEOPLAYER_NEXT_ENDTIME:
      {
        GET_NEXT_VIDEO_LABEL(epgTag->EndAsLocalTime().GetAsLocalizedTime("", false));
      }
      case VIDEOPLAYER_NEXT_DURATION:
      {
        GET_NEXT_VIDEO_LABEL_WITH_CONDITION(StringUtils::SecondsToTimeString(epgTag->GetDuration()), epgTag->GetDuration() > 0);
      }

      // General channel infos
      case VIDEOPLAYER_CHANNEL_NAME:
      {
        if (!channel && epgTag)
          channel = epgTag->Channel();

        if (channel)
        {
          strValue = channel->ChannelName();
          return true;
        }
        break;
      }
      case VIDEOPLAYER_CHANNEL_NUMBER:
      {
        if (!channel && epgTag)
          channel = epgTag->Channel();

        if (channel)
        {
          strValue = StringUtils::Format("%i", channel->ChannelNumber());
          return true;
        }
        break;
      }
      case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
      {
        if (!channel && epgTag)
          channel = epgTag->Channel();

        if (channel)
        {
          strValue = StringUtils::Format("%i", channel->SubChannelNumber());
          return true;
        }
        break;
      }
      case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
      {
        if (!channel && epgTag)
          channel = epgTag->Channel();

        if (channel)
        {
          strValue = channel->FormattedChannelNumber();
          return true;
        }
        break;
      }
      case VIDEOPLAYER_CHANNEL_GROUP:
      {
        if (!channel && epgTag)
          channel = epgTag->Channel();
        
        if (channel && !channel->IsRadio())
        {
          strValue = CServiceBroker::GetPVRManager().GetPlayingTVGroupName();
          return true;
        }
        break;
      }
    }
  }

  return false;
}

void CPVRGUIInfo::CharInfoPlayingDuration(std::string &strValue) const
{
  strValue = StringUtils::SecondsToTimeString(m_iDuration / 1000, TIME_FORMAT_GUESS).c_str();
}

void CPVRGUIInfo::CharInfoTimeshiftStartTime(std::string &strValue) const
{
  strValue = m_strTimeshiftStartTime;
}

void CPVRGUIInfo::CharInfoTimeshiftEndTime(std::string &strValue) const
{
  strValue = m_strTimeshiftEndTime;
}

void CPVRGUIInfo::CharInfoTimeshiftPlayTime(std::string &strValue) const
{
  strValue = m_strTimeshiftPlayTime;
}

void CPVRGUIInfo::CharInfoPlayingTime(std::string &strValue) const
{
  strValue = StringUtils::SecondsToTimeString(GetStartTime()/1000, TIME_FORMAT_GUESS).c_str();
}

void CPVRGUIInfo::CharInfoBackendNumber(std::string &strValue) const
{
  size_t numBackends = m_backendProperties.size();

  if (numBackends > 0)
    strValue = StringUtils::Format("{0} {1} {2}", m_iCurrentActiveClient + 1, g_localizeStrings.Get(20163).c_str(), numBackends);
  else
    strValue = g_localizeStrings.Get(14023);
}

void CPVRGUIInfo::CharInfoTotalDiskSpace(std::string &strValue) const
{
  strValue = StringUtils::SizeToString(m_iBackendDiskTotal).c_str();
}

void CPVRGUIInfo::CharInfoSignal(std::string &strValue) const
{
  strValue = StringUtils::Format("%d %%", m_qualityInfo.iSignal / 655);
}

void CPVRGUIInfo::CharInfoSNR(std::string &strValue) const
{
  strValue = StringUtils::Format("%d %%", m_qualityInfo.iSNR / 655);
}

void CPVRGUIInfo::CharInfoBER(std::string &strValue) const
{
  strValue = StringUtils::Format("%08lX", m_qualityInfo.iBER);
}

void CPVRGUIInfo::CharInfoUNC(std::string &strValue) const
{
  strValue = StringUtils::Format("%08lX", m_qualityInfo.iUNC);
}

void CPVRGUIInfo::CharInfoFrontendName(std::string &strValue) const
{
  if (!strlen(m_qualityInfo.strAdapterName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strAdapterName;
}

void CPVRGUIInfo::CharInfoFrontendStatus(std::string &strValue) const
{
  if (!strlen(m_qualityInfo.strAdapterStatus))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strAdapterStatus;
}

void CPVRGUIInfo::CharInfoBackendName(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendName;
}

void CPVRGUIInfo::CharInfoBackendVersion(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendVersion;
}

void CPVRGUIInfo::CharInfoBackendHost(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendHost;
}

void CPVRGUIInfo::CharInfoBackendDiskspace(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;

  auto diskTotal = m_iBackendDiskTotal;
  auto diskUsed = m_iBackendDiskUsed;

  if (diskTotal > 0)
  {
    strValue = StringUtils::Format(g_localizeStrings.Get(802).c_str(),
        StringUtils::SizeToString(diskTotal - diskUsed).c_str(),
        StringUtils::SizeToString(diskTotal).c_str());
  }
  else
    strValue = g_localizeStrings.Get(13205);
}

void CPVRGUIInfo::CharInfoBackendChannels(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendChannels;
}

void CPVRGUIInfo::CharInfoBackendTimers(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendTimers;
}

void CPVRGUIInfo::CharInfoBackendRecordings(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendRecordings;
}

void CPVRGUIInfo::CharInfoBackendDeletedRecordings(std::string &strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendDeletedRecordings;
}

void CPVRGUIInfo::CharInfoPlayingClientName(std::string &strValue) const
{
  if (m_strPlayingClientName.empty())
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_strPlayingClientName;
}

void CPVRGUIInfo::CharInfoEncryption(std::string &strValue) const
{
  if (m_descrambleInfo.iCaid != PVR_DESCRAMBLE_INFO_NOT_AVAILABLE)
  {
    // prefer dynamically updated info, if available
    strValue = CPVRChannel::GetEncryptionName(m_descrambleInfo.iCaid);
    return;
  }
  else
  {
    const CPVRChannelPtr channel(CServiceBroker::GetPVRManager().Clients()->GetPlayingChannel());
    if (channel)
    {
      strValue = channel->EncryptionName();
      return;
    }
  }

  strValue.clear();
}

void CPVRGUIInfo::CharInfoService(std::string &strValue) const
{
  if (!strlen(m_qualityInfo.strServiceName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strServiceName;
}

void CPVRGUIInfo::CharInfoMux(std::string &strValue) const
{
  if (!strlen(m_qualityInfo.strMuxName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strMuxName;
}

void CPVRGUIInfo::CharInfoProvider(std::string &strValue) const
{
  if (!strlen(m_qualityInfo.strProviderName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strProviderName;
}

void CPVRGUIInfo::UpdateBackendCache(void)
{
  CSingleLock lock(m_critSection);

  // Update the backend information for all backends if
  // an update has been requested
  if (m_iCurrentActiveClient == 0 && m_updateBackendCacheRequested)
  {
    std::vector<SBackend> backendProperties;
    {
      CSingleExit exit(m_critSection);
      backendProperties = CServiceBroker::GetPVRManager().Clients()->GetBackendProperties();
    }

    m_backendProperties = backendProperties;
    m_updateBackendCacheRequested = false;
  }

  // Store some defaults
  m_strBackendName = g_localizeStrings.Get(13205);
  m_strBackendVersion = g_localizeStrings.Get(13205);
  m_strBackendHost = g_localizeStrings.Get(13205);
  m_strBackendChannels = g_localizeStrings.Get(13205);
  m_strBackendTimers = g_localizeStrings.Get(13205);
  m_strBackendRecordings = g_localizeStrings.Get(13205);
  m_strBackendDeletedRecordings = g_localizeStrings.Get(13205);
  m_iBackendDiskTotal = 0;
  m_iBackendDiskUsed = 0;

  // Update with values from the current client when we have at least one
  if (!m_backendProperties.empty())
  {
    const auto &backend = m_backendProperties[m_iCurrentActiveClient];

    m_strBackendName = backend.name;
    m_strBackendVersion = backend.version;
    m_strBackendHost = backend.host;

    if (backend.numChannels >= 0)
      m_strBackendChannels = StringUtils::Format("%i", backend.numChannels);

    if (backend.numTimers >= 0)
      m_strBackendTimers = StringUtils::Format("%i", backend.numTimers);

    if (backend.numRecordings >= 0)
      m_strBackendRecordings = StringUtils::Format("%i", backend.numRecordings);

    if (backend.numDeletedRecordings >= 0)
      m_strBackendDeletedRecordings = StringUtils::Format("%i", backend.numDeletedRecordings);

    m_iBackendDiskTotal = backend.diskTotal;
    m_iBackendDiskUsed = backend.diskUsed;
  }

  // Update the current active client, eventually wrapping around
  if (++m_iCurrentActiveClient >= m_backendProperties.size())
    m_iCurrentActiveClient = 0;
}

void CPVRGUIInfo::UpdateTimersCache(void)
{
  m_anyTimersInfo.UpdateTimersCache();
  m_tvTimersInfo.UpdateTimersCache();
  m_radioTimersInfo.UpdateTimersCache();
}

void CPVRGUIInfo::UpdateTimersToggle(void)
{
  m_anyTimersInfo.UpdateTimersToggle();
  m_tvTimersInfo.UpdateTimersToggle();
  m_radioTimersInfo.UpdateTimersToggle();
}

void CPVRGUIInfo::UpdateNextTimer(void)
{
  m_anyTimersInfo.UpdateNextTimer();
  m_tvTimersInfo.UpdateNextTimer();
  m_radioTimersInfo.UpdateNextTimer();
}

int CPVRGUIInfo::GetDuration(void) const
{
  CSingleLock lock(m_critSection);
  return m_iDuration;
}

int CPVRGUIInfo::GetStartTime(void) const
{
  CSingleLock lock(m_critSection);
  if (m_playingEpgTag || m_iTimeshiftStartTime)
  {
    /* Calculate here the position we have of the running live TV event.
     * "position in ms" = ("current UTC" - "event start UTC") * 1000
     */
    CDateTime current(m_iTimeshiftPlayTime);
    CDateTime start = m_playingEpgTag ? CDateTime(m_playingEpgTag->StartAsUTC())
                                      : CDateTime(m_iTimeshiftStartTime);
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
  m_playingEpgTag.reset();
  m_iDuration = 0;
}

CPVREpgInfoTagPtr CPVRGUIInfo::GetPlayingTag() const
{
  CSingleLock lock(m_critSection);
  return m_playingEpgTag;
}

void CPVRGUIInfo::UpdatePlayingTag(void)
{
  const CPVRChannelPtr currentChannel(CServiceBroker::GetPVRManager().GetCurrentChannel());
  const CPVREpgInfoTagPtr currentTag(CServiceBroker::GetPVRManager().Clients()->GetPlayingEpgTag());
  if (currentChannel || currentTag)
  {
    CPVREpgInfoTagPtr epgTag(GetPlayingTag());
    CPVRChannelPtr channel;
    if (epgTag)
      channel = epgTag->Channel();

    if (!epgTag || !epgTag->IsActive() ||
        !channel || !currentChannel || *channel != *currentChannel)
    {
      CSingleLock lock(m_critSection);
      ResetPlayingTag();
      const CPVREpgInfoTagPtr newTag(currentTag ? currentTag : currentChannel->GetEPGNow());
      if (newTag)
      {
        m_playingEpgTag = newTag;
        m_iDuration = m_playingEpgTag->GetDuration() * 1000;
      }
      else if (m_iTimeshiftEndTime > m_iTimeshiftStartTime)
      {
        m_iDuration = (m_iTimeshiftEndTime - m_iTimeshiftStartTime) * 1000;
      }
    }
  }
  else
  {
    const CPVRRecordingPtr recording(CServiceBroker::GetPVRManager().Clients()->GetPlayingRecording());
    if (recording)
    {
      ResetPlayingTag();
      m_iDuration = recording->GetDuration() * 1000;
    }
  }
}

std::string CPVRGUIInfo::GetPlayingTVGroup()
{
  return m_strPlayingTVGroup;
}

CPVRGUIInfo::TimerInfo::TimerInfo(void)
{
  ResetProperties();
}

void CPVRGUIInfo::TimerInfo::ResetProperties(void)
{
  CSingleLock lock(m_critSection);
  m_strActiveTimerTitle         .clear();
  m_strActiveTimerChannelName   .clear();
  m_strActiveTimerChannelIcon   .clear();
  m_strActiveTimerTime          .clear();
  m_strNextTimerInfo            .clear();
  m_strNextRecordingTitle       .clear();
  m_strNextRecordingChannelName .clear();
  m_strNextRecordingChannelIcon .clear();
  m_strNextRecordingTime        .clear();
  m_iTimerAmount                = 0;
  m_iRecordingTimerAmount       = 0;
  m_iTimerInfoToggleStart       = 0;
  m_iTimerInfoToggleCurrent     = 0;
}

bool CPVRGUIInfo::TimerInfo::TimerInfoToggle()
{
  CSingleLock lock(m_critSection);
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

void CPVRGUIInfo::TimerInfo::UpdateTimersToggle()
{
  if (!TimerInfoToggle())
    return;

  std::string strActiveTimerTitle;
  std::string strActiveTimerChannelName;
  std::string strActiveTimerChannelIcon;
  std::string strActiveTimerTime;

  /* safe to fetch these unlocked, since they're updated from the same thread as this one */
  if (m_iRecordingTimerAmount > 0)
  {
    std::vector<CFileItemPtr> activeTags = GetActiveRecordings();
    if (m_iTimerInfoToggleCurrent < activeTags.size() && activeTags.at(m_iTimerInfoToggleCurrent)->HasPVRTimerInfoTag())
    {
      CPVRTimerInfoTagPtr tag = activeTags.at(m_iTimerInfoToggleCurrent)->GetPVRTimerInfoTag();
      strActiveTimerTitle = StringUtils::Format("%s",       tag->Title().c_str());
      strActiveTimerChannelName = StringUtils::Format("%s", tag->ChannelName().c_str());
      strActiveTimerChannelIcon = StringUtils::Format("%s", tag->ChannelIcon().c_str());
      strActiveTimerTime = StringUtils::Format("%s",        tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false).c_str());
    }
  }

  CSingleLock lock(m_critSection);
  m_strActiveTimerTitle       = strActiveTimerTitle;
  m_strActiveTimerChannelName = strActiveTimerChannelName;
  m_strActiveTimerChannelIcon = strActiveTimerChannelIcon;
  m_strActiveTimerTime        = strActiveTimerTime;
}

void CPVRGUIInfo::TimerInfo::UpdateTimersCache(void)
{
  int iTimerAmount          = AmountActiveTimers();
  int iRecordingTimerAmount = AmountActiveRecordings();

  {
    CSingleLock lock(m_critSection);
    m_iTimerAmount          = iTimerAmount;
    m_iRecordingTimerAmount = iRecordingTimerAmount;
    m_iTimerInfoToggleStart = 0;
  }

  UpdateTimersToggle();
}

void CPVRGUIInfo::TimerInfo::UpdateNextTimer()
{
  std::string strNextRecordingTitle;
  std::string strNextRecordingChannelName;
  std::string strNextRecordingChannelIcon;
  std::string strNextRecordingTime;
  std::string strNextTimerInfo;

  CFileItemPtr tag = GetNextActiveTimer();
  if (tag && tag->HasPVRTimerInfoTag())
  {
    CPVRTimerInfoTagPtr timer = tag->GetPVRTimerInfoTag();
    strNextRecordingTitle = StringUtils::Format("%s",       timer->Title().c_str());
    strNextRecordingChannelName = StringUtils::Format("%s", timer->ChannelName().c_str());
    strNextRecordingChannelIcon = StringUtils::Format("%s", timer->ChannelIcon().c_str());
    strNextRecordingTime = StringUtils::Format("%s",        timer->StartAsLocalTime().GetAsLocalizedDateTime(false, false).c_str());

    strNextTimerInfo = StringUtils::Format("%s %s %s %s",
        g_localizeStrings.Get(19106).c_str(),
        timer->StartAsLocalTime().GetAsLocalizedDate(true).c_str(),
        g_localizeStrings.Get(19107).c_str(),
        timer->StartAsLocalTime().GetAsLocalizedTime("HH:mm", false).c_str());
  }

  CSingleLock lock(m_critSection);
  m_strNextRecordingTitle       = strNextRecordingTitle;
  m_strNextRecordingChannelName = strNextRecordingChannelName;
  m_strNextRecordingChannelIcon = strNextRecordingChannelIcon;
  m_strNextRecordingTime        = strNextRecordingTime;
  m_strNextTimerInfo            = strNextTimerInfo;
}

int CPVRGUIInfo::AnyTimerInfo::AmountActiveTimers()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveTimers();
}

int CPVRGUIInfo::AnyTimerInfo::AmountActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveRecordings();
}

std::vector<CFileItemPtr> CPVRGUIInfo::AnyTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();
}

CFileItemPtr CPVRGUIInfo::AnyTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveTimer();
}

int CPVRGUIInfo::TVTimerInfo::AmountActiveTimers()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveTVTimers();
}

int CPVRGUIInfo::TVTimerInfo::AmountActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveTVRecordings();
}

std::vector<CFileItemPtr> CPVRGUIInfo::TVTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveTVRecordings();
}

CFileItemPtr CPVRGUIInfo::TVTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveTVTimer();
}

int CPVRGUIInfo::RadioTimerInfo::AmountActiveTimers()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveRadioTimers();
}

int CPVRGUIInfo::RadioTimerInfo::AmountActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveRadioRecordings();
}

std::vector<CFileItemPtr> CPVRGUIInfo::RadioTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveRadioRecordings();
}

CFileItemPtr CPVRGUIInfo::RadioTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveRadioTimer();
}
