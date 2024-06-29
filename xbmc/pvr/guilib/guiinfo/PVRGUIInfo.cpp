/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIInfo.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchFilter.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"

#include <cmath>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace PVR;
using namespace KODI::GUILIB::GUIINFO;
using namespace std::chrono_literals;

CPVRGUIInfo::CPVRGUIInfo() : CThread("PVRGUIInfo")
{
  ResetProperties();
}

void CPVRGUIInfo::ResetProperties()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_anyTimersInfo.ResetProperties();
  m_tvTimersInfo.ResetProperties();
  m_radioTimersInfo.ResetProperties();
  m_timesInfo.Reset();
  m_bHasTVRecordings = false;
  m_bHasRadioRecordings = false;
  m_iCurrentActiveClient = 0;
  m_strPlayingClientName.clear();
  m_strBackendName.clear();
  m_strBackendVersion.clear();
  m_strBackendHost.clear();
  m_strBackendTimers.clear();
  m_strBackendRecordings.clear();
  m_strBackendDeletedRecordings.clear();
  m_strBackendProviders.clear();
  m_strBackendChannelGroups.clear();
  m_strBackendChannels.clear();
  m_iBackendDiskTotal = 0;
  m_iBackendDiskUsed = 0;
  m_bIsPlayingTV = false;
  m_bIsPlayingRadio = false;
  m_bIsPlayingRecording = false;
  m_bIsPlayingEpgTag = false;
  m_bIsPlayingEncryptedStream = false;
  m_bIsRecordingPlayingChannel = false;
  m_bCanRecordPlayingChannel = false;
  m_bIsPlayingActiveRecording = false;
  m_bHasTVChannels = false;
  m_bHasRadioChannels = false;

  ClearQualityInfo(m_qualityInfo);
  ClearDescrambleInfo(m_descrambleInfo);

  m_updateBackendCacheRequested = false;
  m_bRegistered = false;
}

void CPVRGUIInfo::ClearQualityInfo(PVR_SIGNAL_STATUS& qualityInfo)
{
  memset(&qualityInfo, 0, sizeof(qualityInfo));
  strncpy(qualityInfo.strAdapterName, g_localizeStrings.Get(13106).c_str(),
          PVR_ADDON_NAME_STRING_LENGTH - 1);
  strncpy(qualityInfo.strAdapterStatus, g_localizeStrings.Get(13106).c_str(),
          PVR_ADDON_NAME_STRING_LENGTH - 1);
}

void CPVRGUIInfo::ClearDescrambleInfo(PVR_DESCRAMBLE_INFO& descrambleInfo)
{
  descrambleInfo = {};
}

void CPVRGUIInfo::Start()
{
  ResetProperties();
  Create();
  SetPriority(ThreadPriority::BELOW_NORMAL);
}

void CPVRGUIInfo::Stop()
{
  StopThread();

  auto& mgr = CServiceBroker::GetPVRManager();
  auto& channels = mgr.Get<PVR::GUI::Channels>();
  channels.GetChannelNavigator().Unsubscribe(this);
  channels.Events().Unsubscribe(this);
  mgr.Events().Unsubscribe(this);

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
  {
    gui->GetInfoManager().UnregisterInfoProvider(this);
    m_bRegistered = false;
  }
}

void CPVRGUIInfo::Notify(const PVREvent& event)
{
  if (event == PVREvent::Timers || event == PVREvent::TimersInvalidated)
    UpdateTimersCache();
}

void CPVRGUIInfo::Notify(const PVRChannelNumberInputChangedEvent& event)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_channelNumberInput = event.m_input;
}

void CPVRGUIInfo::Notify(const PVRPreviewAndPlayerShowInfoChangedEvent& event)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_previewAndPlayerShowInfo = event.m_previewAndPlayerShowInfo;
}

void CPVRGUIInfo::Process()
{
  auto toggleIntervalMs = std::chrono::milliseconds(
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRInfoToggleInterval);
  XbmcThreads::EndTime<> cacheTimer(toggleIntervalMs);

  auto& mgr = CServiceBroker::GetPVRManager();
  mgr.Events().Subscribe(this, &CPVRGUIInfo::Notify);

  auto& channels = mgr.Get<PVR::GUI::Channels>();
  channels.Events().Subscribe(this, &CPVRGUIInfo::Notify);
  channels.GetChannelNavigator().Subscribe(this, &CPVRGUIInfo::Notify);

  /* updated on request */
  UpdateTimersCache();

  /* update the backend cache once initially */
  m_updateBackendCacheRequested = true;

  while (!g_application.m_bStop && !m_bStop)
  {
    if (!m_bRegistered)
    {
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
      {
        gui->GetInfoManager().RegisterInfoProvider(this);
        m_bRegistered = true;
      }
    }

    if (!m_bStop)
      UpdateQualityData();
    std::this_thread::yield();

    if (!m_bStop)
      UpdateDescrambleData();
    std::this_thread::yield();

    if (!m_bStop)
      UpdateMisc();
    std::this_thread::yield();

    if (!m_bStop)
      UpdateTimeshiftData();
    std::this_thread::yield();

    if (!m_bStop)
      UpdateTimersToggle();
    std::this_thread::yield();

    if (!m_bStop)
      UpdateNextTimer();
    std::this_thread::yield();

    // Update the backend cache every toggleInterval seconds
    if (!m_bStop && cacheTimer.IsTimePast())
    {
      UpdateBackendCache();
      cacheTimer.Set(toggleIntervalMs);
    }

    if (!m_bStop)
      CThread::Sleep(500ms);
  }
}

void CPVRGUIInfo::UpdateQualityData()
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_PVRPLAYBACK_SIGNALQUALITY))
    return;

  const std::shared_ptr<const CPVRPlaybackState> playbackState =
      CServiceBroker::GetPVRManager().PlaybackState();
  if (!playbackState)
    return;

  PVR_SIGNAL_STATUS qualityInfo;
  ClearQualityInfo(qualityInfo);

  const int channelUid = playbackState->GetPlayingChannelUniqueID();
  if (channelUid > 0)
  {
    const std::shared_ptr<const CPVRClient> client =
        CServiceBroker::GetPVRManager().Clients()->GetCreatedClient(
            playbackState->GetPlayingClientID());
    if (client)
      client->SignalQuality(channelUid, qualityInfo);
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_qualityInfo = qualityInfo;
}

void CPVRGUIInfo::UpdateDescrambleData()
{
  const std::shared_ptr<const CPVRPlaybackState> playbackState =
      CServiceBroker::GetPVRManager().PlaybackState();
  if (!playbackState)
    return;

  PVR_DESCRAMBLE_INFO descrambleInfo;
  ClearDescrambleInfo(descrambleInfo);

  const int channelUid = playbackState->GetPlayingChannelUniqueID();
  if (channelUid > 0)
  {
    const std::shared_ptr<const CPVRClient> client =
        CServiceBroker::GetPVRManager().Clients()->GetCreatedClient(
            playbackState->GetPlayingClientID());
    if (client)
      client->GetDescrambleInfo(channelUid, descrambleInfo);
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_descrambleInfo = descrambleInfo;
}

void CPVRGUIInfo::UpdateMisc()
{
  const CPVRManager& mgr = CServiceBroker::GetPVRManager();
  bool bStarted = mgr.IsStarted();
  const std::shared_ptr<const CPVRPlaybackState> state = mgr.PlaybackState();

  /* safe to fetch these unlocked, since they're updated from the same thread as this one */
  const std::string strPlayingClientName = bStarted ? state->GetPlayingClientName() : "";
  const bool bHasTVRecordings = bStarted && mgr.Recordings()->GetNumTVRecordings() > 0;
  const bool bHasRadioRecordings = bStarted && mgr.Recordings()->GetNumRadioRecordings() > 0;
  const bool bIsPlayingTV = bStarted && state->IsPlayingTV();
  const bool bIsPlayingRadio = bStarted && state->IsPlayingRadio();
  const bool bIsPlayingRecording = bStarted && state->IsPlayingRecording();
  const bool bIsPlayingEpgTag = bStarted && state->IsPlayingEpgTag();
  const bool bIsPlayingEncryptedStream = bStarted && state->IsPlayingEncryptedChannel();
  const bool bHasTVChannels = bStarted && mgr.ChannelGroups()->GetGroupAllTV()->HasChannels();
  const bool bHasRadioChannels = bStarted && mgr.ChannelGroups()->GetGroupAllRadio()->HasChannels();
  const bool bCanRecordPlayingChannel = bStarted && state->CanRecordOnPlayingChannel();
  const bool bIsRecordingPlayingChannel = bStarted && state->IsRecordingOnPlayingChannel();
  const bool bIsPlayingActiveRecording = bStarted && state->IsPlayingActiveRecording();
  const std::string strPlayingTVGroup =
      (bStarted && bIsPlayingTV) ? state->GetActiveChannelGroup(false)->GroupName() : "";
  const std::string strPlayingRadioGroup =
      (bStarted && bIsPlayingRadio) ? state->GetActiveChannelGroup(true)->GroupName() : "";

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_strPlayingClientName = strPlayingClientName;
  m_bHasTVRecordings = bHasTVRecordings;
  m_bHasRadioRecordings = bHasRadioRecordings;
  m_bIsPlayingTV = bIsPlayingTV;
  m_bIsPlayingRadio = bIsPlayingRadio;
  m_bIsPlayingRecording = bIsPlayingRecording;
  m_bIsPlayingEpgTag = bIsPlayingEpgTag;
  m_bIsPlayingEncryptedStream = bIsPlayingEncryptedStream;
  m_bHasTVChannels = bHasTVChannels;
  m_bHasRadioChannels = bHasRadioChannels;
  m_strPlayingTVGroup = strPlayingTVGroup;
  m_strPlayingRadioGroup = strPlayingRadioGroup;
  m_bCanRecordPlayingChannel = bCanRecordPlayingChannel;
  m_bIsRecordingPlayingChannel = bIsRecordingPlayingChannel;
  m_bIsPlayingActiveRecording = bIsPlayingActiveRecording;
}

void CPVRGUIInfo::UpdateTimeshiftData()
{
  m_timesInfo.Update();
}

bool CPVRGUIInfo::InitCurrentItem(CFileItem* item)
{
  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::CurrentItem);
  return false;
}

bool CPVRGUIInfo::GetLabel(std::string& value,
                           const CFileItem* item,
                           int contextWindow,
                           const CGUIInfo& info,
                           std::string* fallback) const
{
  return GetListItemAndPlayerLabel(item, info, value) || GetPVRLabel(item, info, value) ||
         GetRadioRDSLabel(item, info, value);
}

namespace
{
std::string GetAsLocalizedDateString(const CDateTime& datetime, bool bLongDate)
{
  return datetime.IsValid() ? datetime.GetAsLocalizedDate(bLongDate) : "";
}

std::string GetAsLocalizedTimeString(const CDateTime& datetime)
{
  return datetime.IsValid() ? datetime.GetAsLocalizedTime("", false) : "";
}

std::string GetAsLocalizedDateTimeString(const CDateTime& datetime)
{
  return datetime.IsValid() ? datetime.GetAsLocalizedDateTime(false, false) : "";
}

std::string GetEpgTagTitle(const std::shared_ptr<const CPVREpgInfoTag>& epgTag)
{
  if (epgTag)
  {
    if (CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
      return g_localizeStrings.Get(19266); // Parental locked
    else if (!epgTag->Title().empty())
      return epgTag->Title();
  }

  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_EPG_HIDENOINFOAVAILABLE))
    return g_localizeStrings.Get(19055); // no information available

  return {};
}

} // unnamed namespace

bool CPVRGUIInfo::GetListItemAndPlayerLabel(const CFileItem* item,
                                            const CGUIInfo& info,
                                            std::string& strValue) const
{
  const std::shared_ptr<const CPVRTimerInfoTag> timer = item->GetPVRTimerInfoTag();
  if (timer)
  {
    switch (info.m_info)
    {
      case LISTITEM_DATE:
        strValue = timer->Summary();
        return true;
      case LISTITEM_STARTDATE:
        strValue = GetAsLocalizedDateString(timer->StartAsLocalTime(), true);
        return true;
      case LISTITEM_STARTTIME:
        strValue = GetAsLocalizedTimeString(timer->StartAsLocalTime());
        return true;
      case LISTITEM_ENDDATE:
        strValue = GetAsLocalizedDateString(timer->EndAsLocalTime(), true);
        return true;
      case LISTITEM_ENDTIME:
        strValue = GetAsLocalizedTimeString(timer->EndAsLocalTime());
        return true;
      case LISTITEM_DURATION:
        if (timer->GetDuration() > 0)
        {
          strValue = StringUtils::SecondsToTimeString(timer->GetDuration(),
                                                      static_cast<TIME_FORMAT>(info.GetData4()));
          return true;
        }
        return false;
      case LISTITEM_TITLE:
        strValue = timer->Title();
        return true;
      case LISTITEM_COMMENT:
        strValue =
            timer->GetStatus(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() ==
                             WINDOW_RADIO_TIMER_RULES);
        return true;
      case LISTITEM_TIMERTYPE:
        strValue = timer->GetTypeAsString();
        return true;
      case LISTITEM_CHANNEL_NAME:
        strValue = timer->ChannelName();
        return true;
      case LISTITEM_CHANNEL_LOGO:
        strValue = timer->ChannelIcon();
        return true;
      case LISTITEM_PVR_CLIENT_NAME:
        strValue = CServiceBroker::GetPVRManager().GetClient(timer->ClientID())->GetClientName();
        return true;
      case LISTITEM_PVR_INSTANCE_NAME:
        strValue = CServiceBroker::GetPVRManager().GetClient(timer->ClientID())->GetInstanceName();
        return true;
      case LISTITEM_EPG_EVENT_TITLE:
      case LISTITEM_EPG_EVENT_ICON:
      case LISTITEM_GENRE:
      case LISTITEM_PLOT:
      case LISTITEM_PLOT_OUTLINE:
      case LISTITEM_ORIGINALTITLE:
      case LISTITEM_YEAR:
      case LISTITEM_SEASON:
      case LISTITEM_EPISODE:
      case LISTITEM_EPISODENAME:
      case LISTITEM_DIRECTOR:
      case LISTITEM_CHANNEL_NUMBER:
      case LISTITEM_PREMIERED:
        break; // obtain value from channel/epg
      default:
        return false;
    }
  }

  const std::shared_ptr<const CPVRRecording> recording(item->GetPVRRecordingInfoTag());
  if (recording)
  {
    // Note: CPVRRecoding is derived from CVideoInfoTag. All base class properties will be handled
    //       by CVideoGUIInfoProvider. Only properties introduced by CPVRRecording need to be handled here.
    switch (info.m_info)
    {
      case LISTITEM_DATE:
        strValue = GetAsLocalizedDateTimeString(recording->RecordingTimeAsLocalTime());
        return true;
      case LISTITEM_STARTDATE:
        strValue = GetAsLocalizedDateString(recording->RecordingTimeAsLocalTime(), true);
        return true;
      case VIDEOPLAYER_STARTTIME:
      case LISTITEM_STARTTIME:
        strValue = GetAsLocalizedTimeString(recording->RecordingTimeAsLocalTime());
        return true;
      case LISTITEM_ENDDATE:
        strValue = GetAsLocalizedDateString(recording->EndTimeAsLocalTime(), true);
        return true;
      case VIDEOPLAYER_ENDTIME:
      case LISTITEM_ENDTIME:
        strValue = GetAsLocalizedTimeString(recording->EndTimeAsLocalTime());
        return true;
      case LISTITEM_EXPIRATION_DATE:
        if (recording->HasExpirationTime())
        {
          strValue = GetAsLocalizedDateString(recording->ExpirationTimeAsLocalTime(), false);
          return true;
        }
        break;
      case LISTITEM_EXPIRATION_TIME:
        if (recording->HasExpirationTime())
        {
          strValue = GetAsLocalizedTimeString(recording->ExpirationTimeAsLocalTime());
          return true;
        }
        break;
      case VIDEOPLAYER_EPISODENAME:
      case LISTITEM_EPISODENAME:
        strValue = recording->EpisodeName();
        // fixup multiline episode name strings (which do not fit in any way in our GUI)
        StringUtils::Replace(strValue, "\n", ", ");
        return true;
      case VIDEOPLAYER_CHANNEL_NAME:
      case LISTITEM_CHANNEL_NAME:
        strValue = recording->ChannelName();
        if (strValue.empty())
        {
          if (recording->ProviderName().empty())
          {
            const auto& provider = recording->GetProvider();
            strValue = provider->GetName();
          }
          else
          {
            strValue = recording->ProviderName();
          }
        }
        return true;
      case VIDEOPLAYER_CHANNEL_NUMBER:
      case LISTITEM_CHANNEL_NUMBER:
      {
        const std::shared_ptr<const CPVRChannelGroupMember> groupMember =
            CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(*item);
        if (groupMember)
        {
          strValue = groupMember->ChannelNumber().FormattedChannelNumber();
          return true;
        }
        break;
      }
      case VIDEOPLAYER_CHANNEL_LOGO:
      case LISTITEM_CHANNEL_LOGO:
      {
        const std::shared_ptr<const CPVRChannelGroupMember> groupMember =
            CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(*item);
        if (groupMember)
        {
          strValue = groupMember->Channel()->IconPath();
          return true;
        }
        break;
      }
      case LISTITEM_ICON:
        if (recording->ClientIconPath().empty() && recording->ClientThumbnailPath().empty() &&
            // Only use a fallback if there is more than a single provider available
            // Note: an add-on itself is a provider, hence we don't use > 0
            CServiceBroker::GetPVRManager().Providers()->GetNumProviders() > 1 &&
            !recording->Channel())
        {
          auto provider = recording->GetProvider();
          if (!provider->GetIconPath().empty())
          {
            strValue = provider->GetIconPath();
            return true;
          }
        }
        return false;
      case VIDEOPLAYER_CHANNEL_GROUP:
      {
        std::unique_lock<CCriticalSection> lock(m_critSection);
        strValue = recording->IsRadio() ? m_strPlayingRadioGroup : m_strPlayingTVGroup;
        return true;
      }
      case VIDEOPLAYER_PREMIERED:
      case LISTITEM_PREMIERED:
        if (recording->FirstAired().IsValid())
        {
          strValue = recording->FirstAired().GetAsLocalizedDate();
          return true;
        }
        else if (recording->HasYear())
        {
          strValue = std::to_string(recording->GetYear());
          return true;
        }
        return false;
      case LISTITEM_SIZE:
        if (recording->GetSizeInBytes() > 0)
        {
          strValue = StringUtils::SizeToString(recording->GetSizeInBytes());
          return true;
        }
        return false;
      case LISTITEM_PVR_CLIENT_NAME:
        strValue =
            CServiceBroker::GetPVRManager().GetClient(recording->ClientID())->GetClientName();
        return true;
      case LISTITEM_PVR_INSTANCE_NAME:
        strValue =
            CServiceBroker::GetPVRManager().GetClient(recording->ClientID())->GetInstanceName();
        return true;
    }
    return false;
  }

  const std::shared_ptr<const CPVREpgSearchFilter> filter = item->GetEPGSearchFilter();
  if (filter)
  {
    switch (info.m_info)
    {
      case LISTITEM_DATE:
      {
        CDateTime lastExecLocal;
        lastExecLocal.SetFromUTCDateTime(filter->GetLastExecutedDateTime());
        strValue = GetAsLocalizedDateTimeString(lastExecLocal);
        if (strValue.empty())
          strValue = g_localizeStrings.Get(10006); // "N/A"
        return true;
      }
    }
    return false;
  }

  if (item->IsPVRChannelGroup())
  {
    switch (info.m_info)
    {
      case LISTITEM_PVR_GROUP_ORIGIN:
      {
        const std::shared_ptr<CPVRChannelGroup> group{
            CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupByPath(item->GetPath())};
        if (group)
        {
          const CPVRChannelGroup::Origin origin{group->GetOrigin()};
          switch (origin)
          {
            case CPVRChannelGroup::Origin::CLIENT:
              strValue = g_localizeStrings.Get(856); // Client
              return true;
            case CPVRChannelGroup::Origin::SYSTEM:
              strValue = g_localizeStrings.Get(857); // System
              return true;
            case CPVRChannelGroup::Origin::USER:
              strValue = g_localizeStrings.Get(858); // User
              return true;
          }
        }
        break;
      }
    }
    return false;
  }

  std::shared_ptr<const CPVREpgInfoTag> epgTag;
  std::shared_ptr<const CPVRChannel> channel;
  if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
  {
    CPVRItem pvrItem(item);
    channel = pvrItem.GetChannel();

    switch (info.m_info)
    {
      case VIDEOPLAYER_NEXT_TITLE:
      case VIDEOPLAYER_NEXT_GENRE:
      case VIDEOPLAYER_NEXT_PLOT:
      case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
      case VIDEOPLAYER_NEXT_STARTTIME:
      case VIDEOPLAYER_NEXT_ENDTIME:
      case VIDEOPLAYER_NEXT_DURATION:
      case LISTITEM_NEXT_TITLE:
      case LISTITEM_NEXT_GENRE:
      case LISTITEM_NEXT_PLOT:
      case LISTITEM_NEXT_PLOT_OUTLINE:
      case LISTITEM_NEXT_STARTDATE:
      case LISTITEM_NEXT_STARTTIME:
      case LISTITEM_NEXT_ENDDATE:
      case LISTITEM_NEXT_ENDTIME:
      case LISTITEM_NEXT_DURATION:
        // next playing event
        epgTag = pvrItem.GetNextEpgInfoTag();
        break;
      default:
        // now playing event
        epgTag = pvrItem.GetEpgInfoTag();
        break;
    }

    switch (info.m_info)
    {
      // special handling for channels without epg or with radio rds data
      case PLAYER_TITLE:
      case LISTITEM_TITLE:
      case VIDEOPLAYER_NEXT_TITLE:
      case LISTITEM_NEXT_TITLE:
      case LISTITEM_EPG_EVENT_TITLE:
        // Note: in difference to LISTITEM_TITLE, LISTITEM_EPG_EVENT_TITLE returns the title
        // associated with the epg event of a timer, if any, and not the title of the timer.
        strValue = GetEpgTagTitle(epgTag);
        return true;
    }
  }

  if (epgTag)
  {
    switch (info.m_info)
    {
      case VIDEOPLAYER_GENRE:
      case LISTITEM_GENRE:
      case VIDEOPLAYER_NEXT_GENRE:
      case LISTITEM_NEXT_GENRE:
        strValue = epgTag->GetGenresLabel();
        return true;
      case VIDEOPLAYER_PLOT:
      case LISTITEM_PLOT:
      case VIDEOPLAYER_NEXT_PLOT:
      case LISTITEM_NEXT_PLOT:
        if (!CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
          strValue = epgTag->Plot();
        return true;
      case VIDEOPLAYER_PLOT_OUTLINE:
      case LISTITEM_PLOT_OUTLINE:
      case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
      case LISTITEM_NEXT_PLOT_OUTLINE:
        if (!CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
          strValue = epgTag->PlotOutline();
        return true;
      case LISTITEM_DATE:
        strValue = GetAsLocalizedDateTimeString(epgTag->StartAsLocalTime());
        return true;
      case LISTITEM_STARTDATE:
      case LISTITEM_NEXT_STARTDATE:
        strValue = GetAsLocalizedDateString(epgTag->StartAsLocalTime(), true);
        return true;
      case VIDEOPLAYER_STARTTIME:
      case VIDEOPLAYER_NEXT_STARTTIME:
      case LISTITEM_STARTTIME:
      case LISTITEM_NEXT_STARTTIME:
        strValue = GetAsLocalizedTimeString(epgTag->StartAsLocalTime());
        return true;
      case LISTITEM_ENDDATE:
      case LISTITEM_NEXT_ENDDATE:
        strValue = GetAsLocalizedDateString(epgTag->EndAsLocalTime(), true);
        return true;
      case VIDEOPLAYER_ENDTIME:
      case VIDEOPLAYER_NEXT_ENDTIME:
      case LISTITEM_ENDTIME:
      case LISTITEM_NEXT_ENDTIME:
        strValue = GetAsLocalizedTimeString(epgTag->EndAsLocalTime());
        return true;
      // note: for some reason, there is no VIDEOPLAYER_DURATION
      case LISTITEM_DURATION:
      case VIDEOPLAYER_NEXT_DURATION:
      case LISTITEM_NEXT_DURATION:
        if (epgTag->GetDuration() > 0)
        {
          strValue = StringUtils::SecondsToTimeString(epgTag->GetDuration(),
                                                      static_cast<TIME_FORMAT>(info.GetData4()));
          return true;
        }
        return false;
      case VIDEOPLAYER_IMDBNUMBER:
      case LISTITEM_IMDBNUMBER:
        strValue = epgTag->IMDBNumber();
        return true;
      case VIDEOPLAYER_ORIGINALTITLE:
      case LISTITEM_ORIGINALTITLE:
        if (!CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
          strValue = epgTag->OriginalTitle();
        return true;
      case VIDEOPLAYER_YEAR:
      case LISTITEM_YEAR:
        if (epgTag->Year() > 0)
        {
          strValue = std::to_string(epgTag->Year());
          return true;
        }
        return false;
      case VIDEOPLAYER_SEASON:
      case LISTITEM_SEASON:
        if (epgTag->SeriesNumber() >= 0)
        {
          strValue = std::to_string(epgTag->SeriesNumber());
          return true;
        }
        return false;
      case VIDEOPLAYER_EPISODE:
      case LISTITEM_EPISODE:
        if (epgTag->EpisodeNumber() >= 0)
        {
          strValue = std::to_string(epgTag->EpisodeNumber());
          return true;
        }
        return false;
      case VIDEOPLAYER_EPISODENAME:
      case LISTITEM_EPISODENAME:
        if (!CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
        {
          strValue = epgTag->EpisodeName();
          // fixup multiline episode name strings (which do not fit in any way in our GUI)
          StringUtils::Replace(strValue, "\n", ", ");
        }
        return true;
      case VIDEOPLAYER_CAST:
      case LISTITEM_CAST:
        strValue = epgTag->GetCastLabel();
        return true;
      case VIDEOPLAYER_DIRECTOR:
      case LISTITEM_DIRECTOR:
        strValue = epgTag->GetDirectorsLabel();
        return true;
      case VIDEOPLAYER_WRITER:
      case LISTITEM_WRITER:
        strValue = epgTag->GetWritersLabel();
        return true;
      case LISTITEM_EPG_EVENT_ICON:
        strValue = epgTag->IconPath();
        return true;
      case VIDEOPLAYER_PARENTAL_RATING:
      case LISTITEM_PARENTAL_RATING:
        if (epgTag->ParentalRating() > 0)
        {
          strValue = std::to_string(epgTag->ParentalRating());
          return true;
        }
        return false;
      case LISTITEM_PARENTAL_RATING_CODE:
        strValue = epgTag->ParentalRatingCode();
        return true;
      case LISTITEM_PVR_CLIENT_NAME:
        strValue = CServiceBroker::GetPVRManager().GetClient(epgTag->ClientID())->GetClientName();
        return true;
      case LISTITEM_PVR_INSTANCE_NAME:
        strValue = CServiceBroker::GetPVRManager().GetClient(epgTag->ClientID())->GetInstanceName();
        return true;
      case VIDEOPLAYER_PREMIERED:
      case LISTITEM_PREMIERED:
        if (epgTag->FirstAired().IsValid())
        {
          strValue = epgTag->FirstAired().GetAsLocalizedDate();
          return true;
        }
        else if (epgTag->Year() > 0)
        {
          strValue = std::to_string(epgTag->Year());
          return true;
        }
        return false;
      case VIDEOPLAYER_RATING:
      case LISTITEM_RATING:
      {
        int iStarRating = epgTag->StarRating();
        if (iStarRating > 0)
        {
          strValue = StringUtils::FormatNumber(iStarRating);
          return true;
        }
        return false;
      }
    }
  }

  if (channel)
  {
    switch (info.m_info)
    {
      case MUSICPLAYER_CHANNEL_NAME:
      {
        const std::shared_ptr<const CPVRRadioRDSInfoTag> rdsTag = channel->GetRadioRDSInfoTag();
        if (rdsTag)
        {
          strValue = rdsTag->GetProgStation();
          if (!strValue.empty())
            return true;
        }
        [[fallthrough]];
      }
      case VIDEOPLAYER_CHANNEL_NAME:
      case LISTITEM_CHANNEL_NAME:
        strValue = channel->ChannelName();
        return true;
      case MUSICPLAYER_CHANNEL_LOGO:
      case VIDEOPLAYER_CHANNEL_LOGO:
      case LISTITEM_CHANNEL_LOGO:
        strValue = channel->IconPath();
        return true;
      case MUSICPLAYER_CHANNEL_NUMBER:
      case VIDEOPLAYER_CHANNEL_NUMBER:
      case LISTITEM_CHANNEL_NUMBER:
      {
        auto groupMember = item->GetPVRChannelGroupMemberInfoTag();
        if (!groupMember)
          groupMember =
              CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(
                  *item);
        if (groupMember)
        {
          strValue = groupMember->ChannelNumber().FormattedChannelNumber();
          return true;
        }
        break;
      }
      case MUSICPLAYER_CHANNEL_GROUP:
      case VIDEOPLAYER_CHANNEL_GROUP:
      {
        std::unique_lock<CCriticalSection> lock(m_critSection);
        strValue = channel->IsRadio() ? m_strPlayingRadioGroup : m_strPlayingTVGroup;
        return true;
      }
      case LISTITEM_PVR_CLIENT_NAME:
        strValue = CServiceBroker::GetPVRManager().GetClient(channel->ClientID())->GetClientName();
        return true;
      case LISTITEM_PVR_INSTANCE_NAME:
        strValue =
            CServiceBroker::GetPVRManager().GetClient(channel->ClientID())->GetInstanceName();
        return true;
      case LISTITEM_DATE_ADDED:
        if (channel->DateTimeAdded().IsValid())
        {
          strValue = channel->DateTimeAdded().GetAsLocalizedDate();
          return true;
        }
        break;
    }
  }

  return false;
}

bool CPVRGUIInfo::GetPVRLabel(const CFileItem* item,
                              const CGUIInfo& info,
                              std::string& strValue) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  switch (info.m_info)
  {
    case PVR_EPG_EVENT_ICON:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      if (epgTag)
      {
        strValue = epgTag->IconPath();
      }
      return true;
    }
    case PVR_EPG_EVENT_DURATION:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      strValue = m_timesInfo.GetEpgEventDuration(epgTag, static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PVR_EPG_EVENT_ELAPSED_TIME:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      strValue =
          m_timesInfo.GetEpgEventElapsedTime(epgTag, static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PVR_EPG_EVENT_REMAINING_TIME:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      strValue =
          m_timesInfo.GetEpgEventRemainingTime(epgTag, static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PVR_EPG_EVENT_FINISH_TIME:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      strValue =
          m_timesInfo.GetEpgEventFinishTime(epgTag, static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PVR_TIMESHIFT_START_TIME:
      strValue = m_timesInfo.GetTimeshiftStartTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_TIMESHIFT_END_TIME:
      strValue = m_timesInfo.GetTimeshiftEndTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_TIMESHIFT_PLAY_TIME:
      strValue = m_timesInfo.GetTimeshiftPlayTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_TIMESHIFT_OFFSET:
      strValue = m_timesInfo.GetTimeshiftOffset(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_TIMESHIFT_PROGRESS_DURATION:
      strValue =
          m_timesInfo.GetTimeshiftProgressDuration(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_TIMESHIFT_PROGRESS_START_TIME:
      strValue =
          m_timesInfo.GetTimeshiftProgressStartTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_TIMESHIFT_PROGRESS_END_TIME:
      strValue = m_timesInfo.GetTimeshiftProgressEndTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PVR_EPG_EVENT_SEEK_TIME:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      strValue = m_timesInfo.GetEpgEventSeekTime(appPlayer->GetSeekHandler().GetSeekSize(),
                                                 static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PVR_NOW_RECORDING_TITLE:
      strValue = m_anyTimersInfo.GetActiveTimerTitle();
      return true;
    case PVR_NOW_RECORDING_CHANNEL:
      strValue = m_anyTimersInfo.GetActiveTimerChannelName();
      return true;
    case PVR_NOW_RECORDING_CHAN_ICO:
      strValue = m_anyTimersInfo.GetActiveTimerChannelIcon();
      return true;
    case PVR_NOW_RECORDING_DATETIME:
      strValue = m_anyTimersInfo.GetActiveTimerDateTime();
      return true;
    case PVR_NEXT_RECORDING_TITLE:
      strValue = m_anyTimersInfo.GetNextTimerTitle();
      return true;
    case PVR_NEXT_RECORDING_CHANNEL:
      strValue = m_anyTimersInfo.GetNextTimerChannelName();
      return true;
    case PVR_NEXT_RECORDING_CHAN_ICO:
      strValue = m_anyTimersInfo.GetNextTimerChannelIcon();
      return true;
    case PVR_NEXT_RECORDING_DATETIME:
      strValue = m_anyTimersInfo.GetNextTimerDateTime();
      return true;
    case PVR_TV_NOW_RECORDING_TITLE:
      strValue = m_tvTimersInfo.GetActiveTimerTitle();
      return true;
    case PVR_TV_NOW_RECORDING_CHANNEL:
      strValue = m_tvTimersInfo.GetActiveTimerChannelName();
      return true;
    case PVR_TV_NOW_RECORDING_CHAN_ICO:
      strValue = m_tvTimersInfo.GetActiveTimerChannelIcon();
      return true;
    case PVR_TV_NOW_RECORDING_DATETIME:
      strValue = m_tvTimersInfo.GetActiveTimerDateTime();
      return true;
    case PVR_TV_NEXT_RECORDING_TITLE:
      strValue = m_tvTimersInfo.GetNextTimerTitle();
      return true;
    case PVR_TV_NEXT_RECORDING_CHANNEL:
      strValue = m_tvTimersInfo.GetNextTimerChannelName();
      return true;
    case PVR_TV_NEXT_RECORDING_CHAN_ICO:
      strValue = m_tvTimersInfo.GetNextTimerChannelIcon();
      return true;
    case PVR_TV_NEXT_RECORDING_DATETIME:
      strValue = m_tvTimersInfo.GetNextTimerDateTime();
      return true;
    case PVR_RADIO_NOW_RECORDING_TITLE:
      strValue = m_radioTimersInfo.GetActiveTimerTitle();
      return true;
    case PVR_RADIO_NOW_RECORDING_CHANNEL:
      strValue = m_radioTimersInfo.GetActiveTimerChannelName();
      return true;
    case PVR_RADIO_NOW_RECORDING_CHAN_ICO:
      strValue = m_radioTimersInfo.GetActiveTimerChannelIcon();
      return true;
    case PVR_RADIO_NOW_RECORDING_DATETIME:
      strValue = m_radioTimersInfo.GetActiveTimerDateTime();
      return true;
    case PVR_RADIO_NEXT_RECORDING_TITLE:
      strValue = m_radioTimersInfo.GetNextTimerTitle();
      return true;
    case PVR_RADIO_NEXT_RECORDING_CHANNEL:
      strValue = m_radioTimersInfo.GetNextTimerChannelName();
      return true;
    case PVR_RADIO_NEXT_RECORDING_CHAN_ICO:
      strValue = m_radioTimersInfo.GetNextTimerChannelIcon();
      return true;
    case PVR_RADIO_NEXT_RECORDING_DATETIME:
      strValue = m_radioTimersInfo.GetNextTimerDateTime();
      return true;
    case PVR_NEXT_TIMER:
      strValue = m_anyTimersInfo.GetNextTimer();
      return true;
    case PVR_ACTUAL_STREAM_SIG:
      CharInfoSignal(strValue);
      return true;
    case PVR_ACTUAL_STREAM_SNR:
      CharInfoSNR(strValue);
      return true;
    case PVR_ACTUAL_STREAM_BER:
      CharInfoBER(strValue);
      return true;
    case PVR_ACTUAL_STREAM_UNC:
      CharInfoUNC(strValue);
      return true;
    case PVR_ACTUAL_STREAM_CLIENT:
      CharInfoPlayingClientName(strValue);
      return true;
    case PVR_ACTUAL_STREAM_DEVICE:
      CharInfoFrontendName(strValue);
      return true;
    case PVR_ACTUAL_STREAM_STATUS:
      CharInfoFrontendStatus(strValue);
      return true;
    case PVR_ACTUAL_STREAM_CRYPTION:
      CharInfoEncryption(strValue);
      return true;
    case PVR_ACTUAL_STREAM_SERVICE:
      CharInfoService(strValue);
      return true;
    case PVR_ACTUAL_STREAM_MUX:
      CharInfoMux(strValue);
      return true;
    case PVR_ACTUAL_STREAM_PROVIDER:
      CharInfoProvider(strValue);
      return true;
    case PVR_BACKEND_NAME:
      CharInfoBackendName(strValue);
      return true;
    case PVR_BACKEND_VERSION:
      CharInfoBackendVersion(strValue);
      return true;
    case PVR_BACKEND_HOST:
      CharInfoBackendHost(strValue);
      return true;
    case PVR_BACKEND_DISKSPACE:
      CharInfoBackendDiskspace(strValue);
      return true;
    case PVR_BACKEND_PROVIDERS:
      CharInfoBackendProviders(strValue);
      return true;
    case PVR_BACKEND_CHANNEL_GROUPS:
      CharInfoBackendChannelGroups(strValue);
      return true;
    case PVR_BACKEND_CHANNELS:
      CharInfoBackendChannels(strValue);
      return true;
    case PVR_BACKEND_TIMERS:
      CharInfoBackendTimers(strValue);
      return true;
    case PVR_BACKEND_RECORDINGS:
      CharInfoBackendRecordings(strValue);
      return true;
    case PVR_BACKEND_DELETED_RECORDINGS:
      CharInfoBackendDeletedRecordings(strValue);
      return true;
    case PVR_BACKEND_NUMBER:
      CharInfoBackendNumber(strValue);
      return true;
    case PVR_TOTAL_DISKSPACE:
      CharInfoTotalDiskSpace(strValue);
      return true;
    case PVR_CHANNEL_NUMBER_INPUT:
      strValue = m_channelNumberInput;
      return true;
  }

  return false;
}

bool CPVRGUIInfo::GetRadioRDSLabel(const CFileItem* item,
                                   const CGUIInfo& info,
                                   std::string& strValue) const
{
  if (!item->HasPVRChannelInfoTag())
    return false;

  const std::shared_ptr<const CPVRRadioRDSInfoTag> tag =
      item->GetPVRChannelInfoTag()->GetRadioRDSInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      case RDS_CHANNEL_COUNTRY:
        strValue = tag->GetCountry();
        return true;
      case RDS_TITLE:
        strValue = tag->GetTitle();
        return true;
      case RDS_ARTIST:
        strValue = tag->GetArtist();
        return true;
      case RDS_BAND:
        strValue = tag->GetBand();
        return true;
      case RDS_COMPOSER:
        strValue = tag->GetComposer();
        return true;
      case RDS_CONDUCTOR:
        strValue = tag->GetConductor();
        return true;
      case RDS_ALBUM:
        strValue = tag->GetAlbum();
        return true;
      case RDS_ALBUM_TRACKNUMBER:
        if (tag->GetAlbumTrackNumber() > 0)
        {
          strValue = std::to_string(tag->GetAlbumTrackNumber());
          return true;
        }
        break;
      case RDS_GET_RADIO_STYLE:
        strValue = tag->GetRadioStyle();
        return true;
      case RDS_COMMENT:
        strValue = tag->GetComment();
        return true;
      case RDS_INFO_NEWS:
        strValue = tag->GetInfoNews();
        return true;
      case RDS_INFO_NEWS_LOCAL:
        strValue = tag->GetInfoNewsLocal();
        return true;
      case RDS_INFO_STOCK:
        strValue = tag->GetInfoStock();
        return true;
      case RDS_INFO_STOCK_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoStock().size()));
        return true;
      case RDS_INFO_SPORT:
        strValue = tag->GetInfoSport();
        return true;
      case RDS_INFO_SPORT_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoSport().size()));
        return true;
      case RDS_INFO_LOTTERY:
        strValue = tag->GetInfoLottery();
        return true;
      case RDS_INFO_LOTTERY_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoLottery().size()));
        return true;
      case RDS_INFO_WEATHER:
        strValue = tag->GetInfoWeather();
        return true;
      case RDS_INFO_WEATHER_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoWeather().size()));
        return true;
      case RDS_INFO_HOROSCOPE:
        strValue = tag->GetInfoHoroscope();
        return true;
      case RDS_INFO_HOROSCOPE_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoHoroscope().size()));
        return true;
      case RDS_INFO_CINEMA:
        strValue = tag->GetInfoCinema();
        return true;
      case RDS_INFO_CINEMA_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoCinema().size()));
        return true;
      case RDS_INFO_OTHER:
        strValue = tag->GetInfoOther();
        return true;
      case RDS_INFO_OTHER_SIZE:
        strValue = std::to_string(static_cast<int>(tag->GetInfoOther().size()));
        return true;
      case RDS_PROG_HOST:
        strValue = tag->GetProgHost();
        return true;
      case RDS_PROG_EDIT_STAFF:
        strValue = tag->GetEditorialStaff();
        return true;
      case RDS_PROG_HOMEPAGE:
        strValue = tag->GetProgWebsite();
        return true;
      case RDS_PROG_STYLE:
        strValue = tag->GetProgStyle();
        return true;
      case RDS_PHONE_HOTLINE:
        strValue = tag->GetPhoneHotline();
        return true;
      case RDS_PHONE_STUDIO:
        strValue = tag->GetPhoneStudio();
        return true;
      case RDS_SMS_STUDIO:
        strValue = tag->GetSMSStudio();
        return true;
      case RDS_EMAIL_HOTLINE:
        strValue = tag->GetEMailHotline();
        return true;
      case RDS_EMAIL_STUDIO:
        strValue = tag->GetEMailStudio();
        return true;
      case RDS_PROG_STATION:
        strValue = tag->GetProgStation();
        return true;
      case RDS_PROG_NOW:
        strValue = tag->GetProgNow();
        return true;
      case RDS_PROG_NEXT:
        strValue = tag->GetProgNext();
        return true;
      case RDS_AUDIO_LANG:
        strValue = tag->GetLanguage();
        return true;
      case RDS_GET_RADIOTEXT_LINE:
        strValue = tag->GetRadioText(info.GetData1());
        return true;
    }
  }
  return false;
}

bool CPVRGUIInfo::GetFallbackLabel(std::string& value,
                                   const CFileItem* item,
                                   int contextWindow,
                                   const CGUIInfo& info,
                                   std::string* fallback)
{
  if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // VIDEOPLAYER_*, MUSICPLAYER_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case VIDEOPLAYER_TITLE:
      case MUSICPLAYER_TITLE:
        value = GetEpgTagTitle(CPVRItem(item).GetEpgInfoTag());
        return !value.empty();
      default:
        break;
    }
  }
  return false;
}

bool CPVRGUIInfo::GetInt(int& value,
                         const CGUIListItem* item,
                         int contextWindow,
                         const CGUIInfo& info) const
{
  if (!item->IsFileItem())
    return false;

  const CFileItem* fitem = static_cast<const CFileItem*>(item);
  return GetListItemAndPlayerInt(fitem, info, value) || GetPVRInt(fitem, info, value);
}

bool CPVRGUIInfo::GetListItemAndPlayerInt(const CFileItem* item,
                                          const CGUIInfo& info,
                                          int& iValue) const
{
  switch (info.m_info)
  {
    case LISTITEM_PROGRESS:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgTag = CPVRItem(item).GetEpgInfoTag();
        if (epgTag)
          iValue = static_cast<int>(epgTag->ProgressPercentage());
      }
      return true;
  }
  return false;
}

bool CPVRGUIInfo::GetPVRInt(const CFileItem* item, const CGUIInfo& info, int& iValue) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  switch (info.m_info)
  {
    case PVR_EPG_EVENT_DURATION:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      iValue = m_timesInfo.GetEpgEventDuration(epgTag);
      return true;
    }
    case PVR_EPG_EVENT_PROGRESS:
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag =
          (item->IsPVRChannel() || item->IsEPG()) ? CPVRItem(item).GetEpgInfoTag() : nullptr;
      iValue = m_timesInfo.GetEpgEventProgress(epgTag);
      return true;
    }
    case PVR_TIMESHIFT_PROGRESS:
      iValue = m_timesInfo.GetTimeshiftProgress();
      return true;
    case PVR_TIMESHIFT_PROGRESS_DURATION:
      iValue = m_timesInfo.GetTimeshiftProgressDuration();
      return true;
    case PVR_TIMESHIFT_PROGRESS_PLAY_POS:
      iValue = m_timesInfo.GetTimeshiftProgressPlayPosition();
      return true;
    case PVR_TIMESHIFT_PROGRESS_EPG_START:
      iValue = m_timesInfo.GetTimeshiftProgressEpgStart();
      return true;
    case PVR_TIMESHIFT_PROGRESS_EPG_END:
      iValue = m_timesInfo.GetTimeshiftProgressEpgEnd();
      return true;
    case PVR_TIMESHIFT_PROGRESS_BUFFER_START:
      iValue = m_timesInfo.GetTimeshiftProgressBufferStart();
      return true;
    case PVR_TIMESHIFT_PROGRESS_BUFFER_END:
      iValue = m_timesInfo.GetTimeshiftProgressBufferEnd();
      return true;
    case PVR_TIMESHIFT_SEEKBAR:
      iValue = GetTimeShiftSeekPercent();
      return true;
    case PVR_ACTUAL_STREAM_SIG_PROGR:
      iValue = std::lrintf(static_cast<float>(m_qualityInfo.iSignal) / 0xFFFF * 100);
      return true;
    case PVR_ACTUAL_STREAM_SNR_PROGR:
      iValue = std::lrintf(static_cast<float>(m_qualityInfo.iSNR) / 0xFFFF * 100);
      return true;
    case PVR_BACKEND_DISKSPACE_PROGR:
      if (m_iBackendDiskTotal > 0)
        iValue = std::lrintf(static_cast<float>(m_iBackendDiskUsed) / m_iBackendDiskTotal * 100);
      else
        iValue = 0xFF;
      return true;
    case PVR_CLIENT_COUNT:
      iValue = CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount();
      return true;
  }
  return false;
}

bool CPVRGUIInfo::GetBool(bool& value,
                          const CGUIListItem* item,
                          int contextWindow,
                          const CGUIInfo& info) const
{
  if (!item->IsFileItem())
    return false;

  const CFileItem* fitem = static_cast<const CFileItem*>(item);
  return GetListItemAndPlayerBool(fitem, info, value) || GetPVRBool(fitem, info, value) ||
         GetRadioRDSBool(fitem, info, value);
}

bool CPVRGUIInfo::GetListItemAndPlayerBool(const CFileItem* item,
                                           const CGUIInfo& info,
                                           bool& bValue) const
{
  switch (info.m_info)
  {
    case LISTITEM_HASARCHIVE:
      if (item->IsPVRChannel())
      {
        bValue = item->GetPVRChannelInfoTag()->HasArchive();
        return true;
      }
      break;
    case LISTITEM_ISPLAYABLE:
      if (item->IsEPG())
      {
        bValue = item->GetEPGInfoTag()->IsPlayable();
        return true;
      }
      break;
    case LISTITEM_ISRECORDING:
      if (item->IsPVRChannel())
      {
        bValue = CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(
            *item->GetPVRChannelInfoTag());
        return true;
      }
      else if (item->IsEPG() || item->IsPVRTimer())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = timer->IsRecording();
        return true;
      }
      else if (item->IsPVRRecording())
      {
        bValue = item->GetPVRRecordingInfoTag()->IsInProgress();
        return true;
      }
      break;
    case LISTITEM_INPROGRESS:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgTag = CPVRItem(item).GetEpgInfoTag();
        if (epgTag)
          bValue = epgTag->IsActive();
        return true;
      }
      break;
    case LISTITEM_HASTIMER:
      if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = true;
        return true;
      }
      break;
    case LISTITEM_HASTIMERSCHEDULE:
      if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = timer->HasParent();
        return true;
      }
      break;
    case LISTITEM_HASREMINDER:
      if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = timer->IsReminder();
        return true;
      }
      break;
    case LISTITEM_HASREMINDERRULE:
      if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = timer->IsReminder() && timer->HasParent();
        return true;
      }
      break;
    case LISTITEM_TIMERISACTIVE:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = timer->IsActive();
        break;
      }
      break;
    case LISTITEM_TIMERHASCONFLICT:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = timer->HasConflict();
        return true;
      }
      break;
    case LISTITEM_TIMERHASERROR:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVRTimerInfoTag> timer = CPVRItem(item).GetTimerInfoTag();
        if (timer)
          bValue = (timer->IsBroken() && !timer->HasConflict());
        return true;
      }
      break;
    case LISTITEM_HASRECORDING:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgTag = CPVRItem(item).GetEpgInfoTag();
        if (epgTag)
          bValue = !!CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(epgTag);
        return true;
      }
      break;
    case LISTITEM_HAS_EPG:
      if (item->IsPVRChannel() || item->IsEPG() || item->IsPVRTimer())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgTag = CPVRItem(item).GetEpgInfoTag();
        bValue = (epgTag != nullptr);
        return true;
      }
      break;
    case LISTITEM_ISENCRYPTED:
      if (item->IsPVRChannel() || item->IsEPG())
      {
        const std::shared_ptr<const CPVRChannel> channel = CPVRItem(item).GetChannel();
        if (channel)
          bValue = channel->IsEncrypted();
        return true;
      }
      break;
    case LISTITEM_IS_NEW:
      if (item->IsEPG())
      {
        if (item->GetEPGInfoTag())
        {
          bValue = item->GetEPGInfoTag()->IsNew();
          return true;
        }
      }
      else if (item->IsPVRRecording())
      {
        bValue = item->GetPVRRecordingInfoTag()->IsNew();
        return true;
      }
      else if (item->IsPVRTimer() && item->GetPVRTimerInfoTag()->GetEpgInfoTag())
      {
        bValue = item->GetPVRTimerInfoTag()->GetEpgInfoTag()->IsNew();
        return true;
      }
      else if (item->IsPVRChannel())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgNow =
            item->GetPVRChannelInfoTag()->GetEPGNow();
        bValue = epgNow ? epgNow->IsNew() : false;
        return true;
      }
      break;
    case LISTITEM_IS_PREMIERE:
      if (item->IsEPG())
      {
        bValue = item->GetEPGInfoTag()->IsPremiere();
        return true;
      }
      else if (item->IsPVRRecording())
      {
        bValue = item->GetPVRRecordingInfoTag()->IsPremiere();
        return true;
      }
      else if (item->IsPVRTimer() && item->GetPVRTimerInfoTag()->GetEpgInfoTag())
      {
        bValue = item->GetPVRTimerInfoTag()->GetEpgInfoTag()->IsPremiere();
        return true;
      }
      else if (item->IsPVRChannel())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgNow =
            item->GetPVRChannelInfoTag()->GetEPGNow();
        bValue = epgNow ? epgNow->IsPremiere() : false;
        return true;
      }
      break;
    case LISTITEM_IS_FINALE:
      if (item->IsEPG())
      {
        bValue = item->GetEPGInfoTag()->IsFinale();
        return true;
      }
      else if (item->IsPVRRecording())
      {
        bValue = item->GetPVRRecordingInfoTag()->IsFinale();
        return true;
      }
      else if (item->IsPVRTimer() && item->GetPVRTimerInfoTag()->GetEpgInfoTag())
      {
        bValue = item->GetPVRTimerInfoTag()->GetEpgInfoTag()->IsFinale();
        return true;
      }
      else if (item->IsPVRChannel())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgNow =
            item->GetPVRChannelInfoTag()->GetEPGNow();
        bValue = epgNow ? epgNow->IsFinale() : false;
        return true;
      }
      break;
    case LISTITEM_IS_LIVE:
      if (item->IsEPG())
      {
        bValue = item->GetEPGInfoTag()->IsLive();
        return true;
      }
      else if (item->IsPVRRecording())
      {
        bValue = item->GetPVRRecordingInfoTag()->IsLive();
        return true;
      }
      else if (item->IsPVRTimer() && item->GetPVRTimerInfoTag()->GetEpgInfoTag())
      {
        bValue = item->GetPVRTimerInfoTag()->GetEpgInfoTag()->IsLive();
        return true;
      }
      else if (item->IsPVRChannel())
      {
        const std::shared_ptr<const CPVREpgInfoTag> epgNow =
            item->GetPVRChannelInfoTag()->GetEPGNow();
        bValue = epgNow ? epgNow->IsLive() : false;
        return true;
      }
      break;
    case LISTITEM_ISPLAYING:
      if (item->IsPVRChannel())
      {
        const std::shared_ptr<const CPVRChannel> playingChannel{
            CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel()};
        if (playingChannel)
        {
          const std::shared_ptr<const CPVRChannel> channel{item->GetPVRChannelInfoTag()};
          bValue = (channel->StorageId() == playingChannel->StorageId());
          return true;
        }
      }
      break;
    case MUSICPLAYER_CONTENT:
    case VIDEOPLAYER_CONTENT:
      if (item->IsPVRChannel())
      {
        bValue = StringUtils::EqualsNoCase(info.GetData3(), "livetv");
        return bValue; // if no match for this provider, other providers shall be asked.
      }
      break;
    case VIDEOPLAYER_HAS_INFO:
      if (item->IsPVRChannel())
      {
        bValue = !item->GetPVRChannelInfoTag()->ChannelName().empty();
        return true;
      }
      break;
    case VIDEOPLAYER_HAS_EPG:
      if (item->IsPVRChannel())
      {
        bValue = (item->GetPVRChannelInfoTag()->GetEPGNow() != nullptr);
        return true;
      }
      break;
    case VIDEOPLAYER_CAN_RESUME_LIVE_TV:
      if (item->IsPVRRecording())
      {
        const std::shared_ptr<const CPVRRecording> recording = item->GetPVRRecordingInfoTag();
        const std::shared_ptr<const CPVREpg> epg =
            recording->Channel() ? recording->Channel()->GetEPG() : nullptr;
        const std::shared_ptr<const CPVREpgInfoTag> epgTag =
            CServiceBroker::GetPVRManager().EpgContainer().GetTagById(epg,
                                                                      recording->BroadcastUid());
        bValue = (epgTag && epgTag->IsActive());
        return true;
      }
      break;
    case PLAYER_IS_CHANNEL_PREVIEW_ACTIVE:
      if (item->IsPVRChannel())
      {
        if (m_previewAndPlayerShowInfo)
        {
          bValue = true;
        }
        else
        {
          bValue = !m_videoInfo.valid;
          if (bValue && item->GetPVRChannelInfoTag()->IsRadio())
            bValue = !m_audioInfo.valid;
        }
        return true;
      }
      break;
  }
  return false;
}

bool CPVRGUIInfo::GetPVRBool(const CFileItem* item, const CGUIInfo& info, bool& bValue) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  switch (info.m_info)
  {
    case PVR_IS_RECORDING:
      bValue = m_anyTimersInfo.HasRecordingTimers();
      return true;
    case PVR_IS_RECORDING_TV:
      bValue = m_tvTimersInfo.HasRecordingTimers();
      return true;
    case PVR_IS_RECORDING_RADIO:
      bValue = m_radioTimersInfo.HasRecordingTimers();
      return true;
    case PVR_HAS_TIMER:
      bValue = m_anyTimersInfo.HasTimers();
      return true;
    case PVR_HAS_TV_TIMER:
      bValue = m_tvTimersInfo.HasTimers();
      return true;
    case PVR_HAS_RADIO_TIMER:
      bValue = m_radioTimersInfo.HasTimers();
      return true;
    case PVR_HAS_TV_CHANNELS:
      bValue = m_bHasTVChannels;
      return true;
    case PVR_HAS_RADIO_CHANNELS:
      bValue = m_bHasRadioChannels;
      return true;
    case PVR_HAS_NONRECORDING_TIMER:
      bValue = m_anyTimersInfo.HasNonRecordingTimers();
      return true;
    case PVR_HAS_NONRECORDING_TV_TIMER:
      bValue = m_tvTimersInfo.HasNonRecordingTimers();
      return true;
    case PVR_HAS_NONRECORDING_RADIO_TIMER:
      bValue = m_radioTimersInfo.HasNonRecordingTimers();
      return true;
    case PVR_IS_PLAYING_TV:
      bValue = m_bIsPlayingTV;
      return true;
    case PVR_IS_PLAYING_RADIO:
      bValue = m_bIsPlayingRadio;
      return true;
    case PVR_IS_PLAYING_RECORDING:
      bValue = m_bIsPlayingRecording;
      return true;
    case PVR_IS_PLAYING_EPGTAG:
      bValue = m_bIsPlayingEpgTag;
      return true;
    case PVR_ACTUAL_STREAM_ENCRYPTED:
      bValue = m_bIsPlayingEncryptedStream;
      return true;
    case PVR_IS_TIMESHIFTING:
      bValue = m_timesInfo.IsTimeshifting();
      return true;
    case PVR_CAN_RECORD_PLAYING_CHANNEL:
      bValue = m_bCanRecordPlayingChannel;
      return true;
    case PVR_IS_RECORDING_PLAYING_CHANNEL:
      bValue = m_bIsRecordingPlayingChannel;
      return true;
    case PVR_IS_PLAYING_ACTIVE_RECORDING:
      bValue = m_bIsPlayingActiveRecording;
      return true;
  }
  return false;
}

bool CPVRGUIInfo::GetRadioRDSBool(const CFileItem* item, const CGUIInfo& info, bool& bValue) const
{
  if (!item->HasPVRChannelInfoTag())
    return false;

  const std::shared_ptr<const CPVRRadioRDSInfoTag> tag =
      item->GetPVRChannelInfoTag()->GetRadioRDSInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      case RDS_HAS_RADIOTEXT:
        bValue = tag->IsPlayingRadioText();
        return true;
      case RDS_HAS_RADIOTEXT_PLUS:
        bValue = tag->IsPlayingRadioTextPlus();
        return true;
      case RDS_HAS_HOTLINE_DATA:
        bValue = (!tag->GetEMailHotline().empty() || !tag->GetPhoneHotline().empty());
        return true;
      case RDS_HAS_STUDIO_DATA:
        bValue = (!tag->GetEMailStudio().empty() || !tag->GetSMSStudio().empty() ||
                  !tag->GetPhoneStudio().empty());
        return true;
    }
  }

  switch (info.m_info)
  {
    case RDS_HAS_RDS:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      bValue = appPlayer->IsPlayingRDS();
      return true;
    }
  }

  return false;
}

void CPVRGUIInfo::CharInfoBackendNumber(std::string& strValue) const
{
  size_t numBackends = m_backendProperties.size();

  if (numBackends > 0)
    strValue = StringUtils::Format("{0} {1} {2}", m_iCurrentActiveClient + 1,
                                   g_localizeStrings.Get(20163), numBackends);
  else
    strValue = g_localizeStrings.Get(14023);
}

void CPVRGUIInfo::CharInfoTotalDiskSpace(std::string& strValue) const
{
  strValue = StringUtils::SizeToString(m_iBackendDiskTotal).c_str();
}

void CPVRGUIInfo::CharInfoSignal(std::string& strValue) const
{
  strValue = StringUtils::Format("{} %", m_qualityInfo.iSignal / 655);
}

void CPVRGUIInfo::CharInfoSNR(std::string& strValue) const
{
  strValue = StringUtils::Format("{} %", m_qualityInfo.iSNR / 655);
}

void CPVRGUIInfo::CharInfoBER(std::string& strValue) const
{
  strValue = StringUtils::Format("{:08X}", m_qualityInfo.iBER);
}

void CPVRGUIInfo::CharInfoUNC(std::string& strValue) const
{
  strValue = StringUtils::Format("{:08X}", m_qualityInfo.iUNC);
}

void CPVRGUIInfo::CharInfoFrontendName(std::string& strValue) const
{
  if (!strlen(m_qualityInfo.strAdapterName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strAdapterName;
}

void CPVRGUIInfo::CharInfoFrontendStatus(std::string& strValue) const
{
  if (!strlen(m_qualityInfo.strAdapterStatus))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strAdapterStatus;
}

void CPVRGUIInfo::CharInfoBackendName(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendName;
}

void CPVRGUIInfo::CharInfoBackendVersion(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendVersion;
}

void CPVRGUIInfo::CharInfoBackendHost(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendHost;
}

void CPVRGUIInfo::CharInfoBackendDiskspace(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;

  auto diskTotal = m_iBackendDiskTotal;
  auto diskUsed = m_iBackendDiskUsed;

  if (diskTotal > 0)
  {
    strValue = StringUtils::Format(g_localizeStrings.Get(802),
                                   StringUtils::SizeToString(diskTotal - diskUsed),
                                   StringUtils::SizeToString(diskTotal));
  }
  else
    strValue = g_localizeStrings.Get(13205);
}

void CPVRGUIInfo::CharInfoBackendProviders(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendProviders;
}

void CPVRGUIInfo::CharInfoBackendChannelGroups(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendChannelGroups;
}

void CPVRGUIInfo::CharInfoBackendChannels(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendChannels;
}

void CPVRGUIInfo::CharInfoBackendTimers(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendTimers;
}

void CPVRGUIInfo::CharInfoBackendRecordings(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendRecordings;
}

void CPVRGUIInfo::CharInfoBackendDeletedRecordings(std::string& strValue) const
{
  m_updateBackendCacheRequested = true;
  strValue = m_strBackendDeletedRecordings;
}

void CPVRGUIInfo::CharInfoPlayingClientName(std::string& strValue) const
{
  if (m_strPlayingClientName.empty())
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_strPlayingClientName;
}

void CPVRGUIInfo::CharInfoEncryption(std::string& strValue) const
{
  if (m_descrambleInfo.iCaid != PVR_DESCRAMBLE_INFO_NOT_AVAILABLE)
  {
    // prefer dynamically updated info, if available
    strValue = CPVRChannel::GetEncryptionName(m_descrambleInfo.iCaid);
    return;
  }
  else
  {
    const std::shared_ptr<const CPVRChannel> channel =
        CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (channel)
    {
      strValue = channel->EncryptionName();
      return;
    }
  }

  strValue.clear();
}

void CPVRGUIInfo::CharInfoService(std::string& strValue) const
{
  if (!strlen(m_qualityInfo.strServiceName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strServiceName;
}

void CPVRGUIInfo::CharInfoMux(std::string& strValue) const
{
  if (!strlen(m_qualityInfo.strMuxName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strMuxName;
}

void CPVRGUIInfo::CharInfoProvider(std::string& strValue) const
{
  if (!strlen(m_qualityInfo.strProviderName))
    strValue = g_localizeStrings.Get(13205);
  else
    strValue = m_qualityInfo.strProviderName;
}

void CPVRGUIInfo::UpdateBackendCache()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

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
  m_strBackendProviders = g_localizeStrings.Get(13205);
  m_strBackendChannelGroups = g_localizeStrings.Get(13205);
  m_strBackendChannels = g_localizeStrings.Get(13205);
  m_strBackendTimers = g_localizeStrings.Get(13205);
  m_strBackendRecordings = g_localizeStrings.Get(13205);
  m_strBackendDeletedRecordings = g_localizeStrings.Get(13205);
  m_iBackendDiskTotal = 0;
  m_iBackendDiskUsed = 0;

  // Update with values from the current client when we have at least one
  if (!m_backendProperties.empty())
  {
    const auto& backend = m_backendProperties[m_iCurrentActiveClient];

    m_strBackendName = backend.name;
    m_strBackendVersion = backend.version;
    m_strBackendHost = backend.host;

    // We always display one extra as the add-on itself counts as a provider
    if (backend.numProviders >= 0)
      m_strBackendProviders = std::to_string(backend.numProviders + 1);

    if (backend.numChannelGroups >= 0)
      m_strBackendChannelGroups = std::to_string(backend.numChannelGroups);

    if (backend.numChannels >= 0)
      m_strBackendChannels = std::to_string(backend.numChannels);

    if (backend.numTimers >= 0)
      m_strBackendTimers = std::to_string(backend.numTimers);

    if (backend.numRecordings >= 0)
      m_strBackendRecordings = std::to_string(backend.numRecordings);

    if (backend.numDeletedRecordings >= 0)
      m_strBackendDeletedRecordings = std::to_string(backend.numDeletedRecordings);

    m_iBackendDiskTotal = backend.diskTotal;
    m_iBackendDiskUsed = backend.diskUsed;
  }

  // Update the current active client, eventually wrapping around
  if (++m_iCurrentActiveClient >= m_backendProperties.size())
    m_iCurrentActiveClient = 0;
}

void CPVRGUIInfo::UpdateTimersCache()
{
  m_anyTimersInfo.UpdateTimersCache();
  m_tvTimersInfo.UpdateTimersCache();
  m_radioTimersInfo.UpdateTimersCache();
}

void CPVRGUIInfo::UpdateTimersToggle()
{
  m_anyTimersInfo.UpdateTimersToggle();
  m_tvTimersInfo.UpdateTimersToggle();
  m_radioTimersInfo.UpdateTimersToggle();
}

void CPVRGUIInfo::UpdateNextTimer()
{
  m_anyTimersInfo.UpdateNextTimer();
  m_tvTimersInfo.UpdateNextTimer();
  m_radioTimersInfo.UpdateNextTimer();
}

int CPVRGUIInfo::GetTimeShiftSeekPercent() const
{
  int progress = m_timesInfo.GetTimeshiftProgressPlayPosition();

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  int seekSize = appPlayer->GetSeekHandler().GetSeekSize();
  if (seekSize != 0)
  {
    int total = m_timesInfo.GetTimeshiftProgressDuration();

    float totalTime = static_cast<float>(total);
    if (totalTime == 0.0f)
      return 0;

    float percentPerSecond = 100.0f / totalTime;
    float percent = progress + percentPerSecond * seekSize;
    percent = std::max(0.0f, std::min(percent, 100.0f));
    return std::lrintf(percent);
  }
  return progress;
}
