/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClient.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/binary-addons/AddonDll.h"
#include "cores/EdlEdit.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "dialogs/GUIDialogKaiToast.h" //! @todo get rid of GUI in core
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRConstants.h" // PVR_CLIENT_INVALID_UID
#include "pvr/PVRDatabase.h"
#include "pvr/PVRDescrambleInfo.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRSignalStatus.h"
#include "pvr/PVRStreamProperties.h"
#include "pvr/addons/PVRClientMenuHooks.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupFactory.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

extern "C"
{
#include <libavcodec/avcodec.h>
}

using namespace ADDON;
using namespace PVR;

namespace
{
class CAddonChannelGroup : public PVR_CHANNEL_GROUP
{
public:
  explicit CAddonChannelGroup(const CPVRChannelGroup& group) : m_groupName(group.ClientGroupName())
  {
    // zero-init base struct members
    PVR_CHANNEL_GROUP* base = static_cast<PVR_CHANNEL_GROUP*>(this);
    *base = {};

    bIsRadio = group.IsRadio();
    strGroupName = m_groupName.c_str();
    iPosition = group.GetClientPosition();
  }
  virtual ~CAddonChannelGroup() = default;

private:
  const std::string m_groupName;
};

class CAddonChannel : public PVR_CHANNEL
{
public:
  explicit CAddonChannel(const CPVRChannel& channel, const std::string& newChannelName = "")
    : m_channelName(newChannelName.empty() ? channel.ClientChannelName() : newChannelName),
      m_mimeType(channel.MimeType()),
      m_iconPath(channel.ClientIconPath())
  {
    // zero-init base struct members
    PVR_CHANNEL* base = static_cast<PVR_CHANNEL*>(this);
    *base = {};

    iUniqueId = channel.UniqueID();
    iChannelNumber = channel.ClientChannelNumber().GetChannelNumber();
    iSubChannelNumber = channel.ClientChannelNumber().GetSubChannelNumber();
    strChannelName = m_channelName.c_str();
    strIconPath = m_iconPath.c_str();
    iEncryptionSystem = channel.EncryptionSystem();
    bIsRadio = channel.IsRadio();
    bIsHidden = channel.IsHidden();
    strMimeType = m_mimeType.c_str();
    iClientProviderUid = channel.ClientProviderUid();
    bHasArchive = channel.HasArchive();
  }
  virtual ~CAddonChannel() = default;

private:
  const std::string m_channelName;
  const std::string m_mimeType;
  const std::string m_iconPath;
};

class CAddonRecording : public PVR_RECORDING
{
public:
  explicit CAddonRecording(const CPVRRecording& recording)
    : m_recordingId(recording.ClientRecordingID()),
      m_title(recording.m_strTitle),
      m_titleExtraInfo(recording.TitleExtraInfo()),
      m_episodeName(recording.m_strShowTitle),
      m_directory(recording.Directory()),
      m_plotOutline(recording.m_strPlotOutline),
      m_plot(recording.m_strPlot),
      m_genreDescription(recording.GetGenresLabel()),
      m_channelName(recording.ChannelName()),
      m_iconPath(recording.ClientIconPath()),
      m_thumbnailPath(recording.ClientThumbnailPath()),
      m_fanartPath(recording.ClientFanartPath()),
      m_firstAired(recording.FirstAired().IsValid() ? recording.FirstAired().GetAsW3CDate() : ""),
      m_providerName(recording.ProviderName()),
      m_parentalRatingCode(recording.GetParentalRatingCode()),
      m_parentalRatingIcon(recording.ClientParentalRatingIconPath()),
      m_parentalRatingSource(recording.GetParentalRatingSource())
  {
    // zero-init base struct members
    PVR_RECORDING* base = static_cast<PVR_RECORDING*>(this);
    *base = {};

    time_t recTime;
    recording.RecordingTimeAsUTC().GetAsTime(recTime);

    strRecordingId = m_recordingId.c_str();
    strTitle = m_title.c_str();
    strTitleExtraInfo = m_titleExtraInfo.c_str();
    strEpisodeName = m_episodeName.c_str();
    iSeriesNumber = recording.m_iSeason;
    iEpisodeNumber = recording.m_iEpisode;
    iEpisodePartNumber = recording.EpisodePart();
    iYear = recording.GetYear();
    strDirectory = m_directory.c_str();
    strPlotOutline = m_plotOutline.c_str();
    strPlot = m_plot.c_str();
    strGenreDescription = m_genreDescription.c_str();
    strChannelName = m_channelName.c_str();
    strIconPath = m_iconPath.c_str();
    strThumbnailPath = m_thumbnailPath.c_str();
    strFanartPath = m_fanartPath.c_str();
    recordingTime =
        recTime -
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
    iDuration = recording.GetDuration();
    iPriority = recording.Priority();
    iLifetime = recording.LifeTime();
    iGenreType = recording.GenreType();
    iGenreSubType = recording.GenreSubType();
    iPlayCount = recording.GetLocalPlayCount();
    iLastPlayedPosition = MathUtils::round_int(recording.GetLocalResumePoint().timeInSeconds);
    bIsDeleted = recording.IsDeleted();
    iEpgEventId = recording.BroadcastUid();
    iChannelUid = recording.ChannelUid();
    channelType =
        recording.IsRadio() ? PVR_RECORDING_CHANNEL_TYPE_RADIO : PVR_RECORDING_CHANNEL_TYPE_TV;
    strFirstAired = m_firstAired.c_str();
    iFlags = recording.Flags();
    sizeInBytes = recording.GetSizeInBytes();
    strProviderName = m_providerName.c_str();
    iClientProviderUid = recording.ClientProviderUid();
    strParentalRatingCode = m_parentalRatingCode.c_str();
    strParentalRatingIcon = m_parentalRatingIcon.c_str();
    strParentalRatingSource = m_parentalRatingSource.c_str();
  }
  virtual ~CAddonRecording() = default;

private:
  const std::string m_recordingId;
  const std::string m_title;
  const std::string m_titleExtraInfo;
  const std::string m_episodeName;
  const std::string m_directory;
  const std::string m_plotOutline;
  const std::string m_plot;
  const std::string m_genreDescription;
  const std::string m_channelName;
  const std::string m_iconPath;
  const std::string m_thumbnailPath;
  const std::string m_fanartPath;
  const std::string m_firstAired;
  const std::string m_providerName;
  const std::string m_parentalRatingCode;
  const std::string m_parentalRatingIcon;
  const std::string m_parentalRatingSource;
};

class CAddonTimer : public PVR_TIMER
{
public:
  explicit CAddonTimer(const CPVRTimerInfoTag& timer)
    : m_title(timer.Title()),
      m_epgSearchString(timer.EpgSearchString()),
      m_directory(timer.Directory()),
      m_summary(timer.Summary()),
      m_seriesLink(timer.SeriesLink())
  {
    // zero-init base struct members
    PVR_TIMER* base = static_cast<PVR_TIMER*>(this);
    *base = {};

    time_t start;
    timer.StartAsUTC().GetAsTime(start);
    time_t end;
    timer.EndAsUTC().GetAsTime(end);
    time_t first;
    timer.FirstDayAsUTC().GetAsTime(first);
    const std::shared_ptr<const CPVREpgInfoTag> epgTag{timer.GetEpgInfoTag()};
    const int timeCorrection{
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection};

    iClientIndex = timer.ClientIndex();
    iParentClientIndex = timer.ParentClientIndex();
    state = timer.State();
    iTimerType = timer.GetTimerType()->GetTypeId();
    iClientChannelUid = timer.ClientChannelUID();
    strTitle = m_title.c_str();
    strEpgSearchString = m_epgSearchString.c_str();
    bFullTextEpgSearch = timer.IsFullTextEpgSearch();
    strDirectory = m_directory.c_str();
    iPriority = timer.Priority();
    iLifetime = timer.Lifetime();
    iMaxRecordings = timer.MaxRecordings();
    iPreventDuplicateEpisodes = timer.PreventDupEpisodesPolicy();
    iRecordingGroup = timer.RecordingGroup();
    iWeekdays = timer.WeekDays();
    startTime = start - timeCorrection;
    endTime = end - timeCorrection;
    bStartAnyTime = timer.IsStartAnyTime();
    bEndAnyTime = timer.IsEndAnyTime();
    firstDay = first - timeCorrection;
    iEpgUid = epgTag ? epgTag->UniqueBroadcastID() : PVR_TIMER_NO_EPG_UID;
    strSummary = m_summary.c_str();
    iMarginStart = timer.MarginStart();
    iMarginEnd = timer.MarginEnd();
    iGenreType = epgTag ? epgTag->GenreType() : 0;
    iGenreSubType = epgTag ? epgTag->GenreSubType() : 0;
    strSeriesLink = m_seriesLink.c_str();

    const auto& props{timer.GetCustomProperties()};
    iCustomPropsSize = static_cast<unsigned int>(props.size());
    if (iCustomPropsSize)
    {
      m_customProps = std::make_unique<PVR_SETTING_KEY_VALUE_PAIR[]>(iCustomPropsSize);
      int idx{0};
      for (const auto& entry : props)
      {
        PVR_SETTING_KEY_VALUE_PAIR& prop{m_customProps[idx]};
        prop.iKey = entry.first;
        prop.eType = entry.second.type;
        prop.iValue = entry.second.value.asInteger32();
        m_customPropStringValues.emplace_back(entry.second.value.asString());
        prop.strValue = m_customPropStringValues.back().c_str();
        ++idx;
      }
      customProps = m_customProps.get();
    }
  }
  virtual ~CAddonTimer() = default;

private:
  const std::string m_title;
  const std::string m_epgSearchString;
  const std::string m_directory;
  const std::string m_summary;
  const std::string m_seriesLink;
  std::vector<std::string> m_customPropStringValues;
  std::unique_ptr<PVR_SETTING_KEY_VALUE_PAIR[]> m_customProps;
};

class CAddonEpgTag : public EPG_TAG
{
public:
  explicit CAddonEpgTag(const CPVREpgInfoTag& tag)
    : m_title(tag.Title()),
      m_titleExtraInfo(tag.TitleExtraInfo()),
      m_plotOutline(tag.PlotOutline()),
      m_plot(tag.Plot()),
      m_originalTitle(tag.OriginalTitle()),
      m_cast(tag.DeTokenize(tag.Cast())),
      m_director(tag.DeTokenize(tag.Directors())),
      m_writer(tag.DeTokenize(tag.Writers())),
      m_IMDBNumber(tag.IMDBNumber()),
      m_episodeName(tag.EpisodeName()),
      m_iconPath(tag.ClientIconPath()),
      m_seriesLink(tag.SeriesLink()),
      m_genreDescription(tag.GenreDescription()),
      m_firstAired(GetFirstAired(tag)),
      m_parentalRatingCode(tag.ParentalRatingCode()),
      m_parentalRatingIcon(tag.ClientParentalRatingIconPath()),
      m_parentalRatingSource(tag.ParentalRatingSource())
  {
    // zero-init base struct members
    EPG_TAG* base = static_cast<EPG_TAG*>(this);
    *base = {};

    time_t t;
    tag.StartAsUTC().GetAsTime(t);
    startTime = t;
    tag.EndAsUTC().GetAsTime(t);
    endTime = t;

    iUniqueBroadcastId = tag.UniqueBroadcastID();
    iUniqueChannelId = tag.UniqueChannelID();
    iParentalRating = tag.ParentalRating();
    iSeriesNumber = tag.SeriesNumber();
    iEpisodeNumber = tag.EpisodeNumber();
    iEpisodePartNumber = tag.EpisodePart();
    iStarRating = tag.StarRating();
    iYear = tag.Year();
    iFlags = tag.Flags();
    iGenreType = tag.GenreType();
    iGenreSubType = tag.GenreSubType();
    strTitle = m_title.c_str();
    strTitleExtraInfo = m_titleExtraInfo.c_str();
    strPlotOutline = m_plotOutline.c_str();
    strPlot = m_plot.c_str();
    strOriginalTitle = m_originalTitle.c_str();
    strCast = m_cast.c_str();
    strDirector = m_director.c_str();
    strWriter = m_writer.c_str();
    strIMDBNumber = m_IMDBNumber.c_str();
    strEpisodeName = m_episodeName.c_str();
    strIconPath = m_iconPath.c_str();
    strSeriesLink = m_seriesLink.c_str();
    strGenreDescription = m_genreDescription.c_str();
    strFirstAired = m_firstAired.c_str();
    strParentalRatingCode = m_parentalRatingCode.c_str();
    strParentalRatingIcon = m_parentalRatingIcon.c_str();
    strParentalRatingSource = m_parentalRatingSource.c_str();
  }

  virtual ~CAddonEpgTag() = default;

private:
  static std::string GetFirstAired(const CPVREpgInfoTag& tag)
  {
    const CDateTime firstAired{tag.FirstAired()};
    if (firstAired.IsValid())
      return firstAired.GetAsW3CDate();
    return {};
  }

  const std::string m_title;
  const std::string m_titleExtraInfo;
  const std::string m_plotOutline;
  const std::string m_plot;
  const std::string m_originalTitle;
  const std::string m_cast;
  const std::string m_director;
  const std::string m_writer;
  const std::string m_IMDBNumber;
  const std::string m_episodeName;
  const std::string m_iconPath;
  const std::string m_seriesLink;
  const std::string m_genreDescription;
  const std::string m_firstAired;
  const std::string m_parentalRatingCode;
  const std::string m_parentalRatingIcon;
  const std::string m_parentalRatingSource;
};

EDL::Edit ConvertAddonEdl(const PVR_EDL_ENTRY& entry)
{
  EDL::Edit edit;
  edit.start = std::chrono::milliseconds(entry.start);
  edit.end = std::chrono::milliseconds(entry.end);

  switch (entry.type)
  {
    case PVR_EDL_TYPE_CUT:
      edit.action = EDL::Action::CUT;
      break;
    case PVR_EDL_TYPE_MUTE:
      edit.action = EDL::Action::MUTE;
      break;
    case PVR_EDL_TYPE_SCENE:
      edit.action = EDL::Action::SCENE;
      break;
    case PVR_EDL_TYPE_COMBREAK:
      edit.action = EDL::Action::COMM_BREAK;
      break;
    default:
      CLog::LogF(LOGWARNING, "Ignoring entry of unknown EDL type: {}", entry.type);
      break;
  }

  return edit;
}
} // unnamed namespace

namespace PVR
{

#define DEFAULT_INFO_STRING_VALUE "unknown"

CPVRClient::CPVRClient(const ADDON::AddonInfoPtr& addonInfo,
                       ADDON::AddonInstanceId instanceId,
                       int clientId)
  : IAddonInstanceHandler(ADDON_INSTANCE_PVR, addonInfo, instanceId), m_iClientId(clientId)
{
  // Create all interface parts independent to make API changes easier if
  // something is added
  m_ifc.pvr = new AddonInstance_PVR;
  m_ifc.pvr->props = new AddonProperties_PVR();
  m_ifc.pvr->toKodi = new AddonToKodiFuncTable_PVR();
  m_ifc.pvr->toAddon = new KodiToAddonFuncTable_PVR();

  ResetProperties();
}

CPVRClient::~CPVRClient()
{
  Destroy();

  if (m_ifc.pvr)
  {
    delete m_ifc.pvr->props;
    delete m_ifc.pvr->toKodi;
    delete m_ifc.pvr->toAddon;
  }
  delete m_ifc.pvr;
}

void CPVRClient::StopRunningInstance()
{
  // stop the pvr manager and stop and unload the running pvr addon. pvr manager will be restarted on demand.
  CServiceBroker::GetPVRManager().Stop();
  CServiceBroker::GetPVRManager().Clients()->StopClient(m_iClientId, false);
}

void CPVRClient::OnPreInstall()
{
  // note: this method is also called on update; thus stop and unload possibly running instance
  StopRunningInstance();
}

void CPVRClient::OnPreUnInstall()
{
  StopRunningInstance();
}

void CPVRClient::ResetProperties()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  /* initialise members */
  m_strUserPath = CSpecialProtocol::TranslatePath(Profile());
  m_strClientPath = CSpecialProtocol::TranslatePath(Path());
  m_bReadyToUse = false;
  m_bBlockAddonCalls = false;
  m_iAddonCalls = 0;
  m_allAddonCallsFinished.Set();
  m_connectionState = PVR_CONNECTION_STATE_UNKNOWN;
  m_prevConnectionState = PVR_CONNECTION_STATE_UNKNOWN;
  m_ignoreClient = false;
  m_priority.reset();
  m_strBackendVersion = DEFAULT_INFO_STRING_VALUE;
  m_strConnectionString = DEFAULT_INFO_STRING_VALUE;
  m_strBackendName = DEFAULT_INFO_STRING_VALUE;
  m_strBackendHostname.clear();
  m_menuhooks.reset();
  m_timertypes.clear();
  m_clientCapabilities.clear();

  m_ifc.pvr->props->strUserPath = m_strUserPath.c_str();
  m_ifc.pvr->props->strClientPath = m_strClientPath.c_str();
  m_ifc.pvr->props->iEpgMaxPastDays =
      CServiceBroker::GetPVRManager().EpgContainer().GetPastDaysToDisplay();
  m_ifc.pvr->props->iEpgMaxFutureDays =
      CServiceBroker::GetPVRManager().EpgContainer().GetFutureDaysToDisplay();

  m_ifc.pvr->toKodi->kodiInstance = this;
  m_ifc.pvr->toKodi->TransferEpgEntry = cb_transfer_epg_entry;
  m_ifc.pvr->toKodi->TransferChannelEntry = cb_transfer_channel_entry;
  m_ifc.pvr->toKodi->TransferProviderEntry = cb_transfer_provider_entry;
  m_ifc.pvr->toKodi->TransferTimerEntry = cb_transfer_timer_entry;
  m_ifc.pvr->toKodi->TransferRecordingEntry = cb_transfer_recording_entry;
  m_ifc.pvr->toKodi->AddMenuHook = cb_add_menu_hook;
  m_ifc.pvr->toKodi->RecordingNotification = cb_recording_notification;
  m_ifc.pvr->toKodi->TriggerChannelUpdate = cb_trigger_channel_update;
  m_ifc.pvr->toKodi->TriggerProvidersUpdate = cb_trigger_provider_update;
  m_ifc.pvr->toKodi->TriggerChannelGroupsUpdate = cb_trigger_channel_groups_update;
  m_ifc.pvr->toKodi->TriggerTimerUpdate = cb_trigger_timer_update;
  m_ifc.pvr->toKodi->TriggerRecordingUpdate = cb_trigger_recording_update;
  m_ifc.pvr->toKodi->TriggerEpgUpdate = cb_trigger_epg_update;
  m_ifc.pvr->toKodi->FreeDemuxPacket = cb_free_demux_packet;
  m_ifc.pvr->toKodi->AllocateDemuxPacket = cb_allocate_demux_packet;
  m_ifc.pvr->toKodi->TransferChannelGroup = cb_transfer_channel_group;
  m_ifc.pvr->toKodi->TransferChannelGroupMember = cb_transfer_channel_group_member;
  m_ifc.pvr->toKodi->ConnectionStateChange = cb_connection_state_change;
  m_ifc.pvr->toKodi->EpgEventStateChange = cb_epg_event_state_change;
  m_ifc.pvr->toKodi->GetCodecByName = cb_get_codec_by_name;

  // Clear function addresses to have NULL if not set by addon
  memset(m_ifc.pvr->toAddon, 0, sizeof(KodiToAddonFuncTable_PVR));
}

ADDON_STATUS CPVRClient::Create()
{
  ResetProperties();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating PVR add-on instance [{},{},{}]", ID(), InstanceId(),
              m_iClientId);

  const ADDON_STATUS status = CreateInstance();

  if (status == ADDON_STATUS_OK)
  {
    m_bReadyToUse = GetAddonProperties();

    CLog::LogFC(LOGDEBUG, LOGPVR,
                "Created PVR add-on instance {}. readytouse={}, ignoreclient={}, "
                "connectionstate={}",
                GetID(), m_bReadyToUse, IgnoreClient(), GetConnectionState());
  }
  else
  {
    m_bReadyToUse = false;

    CLog::LogF(LOGERROR, "Failed to create PVR add-on instance {}. status={}", GetID(), status);
  }

  return status;
}

void CPVRClient::Destroy()
{
  if (!m_bReadyToUse)
    return;

  m_bReadyToUse = false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Destroying PVR add-on instance {}", GetID());

  m_bBlockAddonCalls = true;
  m_allAddonCallsFinished.Wait();

  DestroyInstance();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Destroyed PVR add-on instance {}", GetID());

  if (m_menuhooks)
    m_menuhooks->Clear();

  ResetProperties();
}

void CPVRClient::Stop()
{
  m_bBlockAddonCalls = true;
  m_priority.reset();
}

void CPVRClient::Continue()
{
  m_bBlockAddonCalls = false;
}

void CPVRClient::ReCreate()
{
  Destroy();
  Create();
}

bool CPVRClient::ReadyToUse() const
{
  return m_bReadyToUse;
}

PVR_CONNECTION_STATE CPVRClient::GetConnectionState() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_connectionState;
}

void CPVRClient::SetConnectionState(PVR_CONNECTION_STATE state)
{
  if (state == PVR_CONNECTION_STATE_CONNECTED)
  {
    // update properties - some will only be available after add-on is connected to backend
    if (!GetAddonProperties())
      CLog::LogF(LOGERROR, "Error reading PVR client properties");
  }
  else
  {
    if (!GetAddonNameStringProperties())
      CLog::LogF(LOGERROR, "Cannot read PVR client name string properties");
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_prevConnectionState = m_connectionState;
  m_connectionState = state;

  if (m_connectionState == PVR_CONNECTION_STATE_CONNECTED)
    m_ignoreClient = false;
  else if (m_connectionState == PVR_CONNECTION_STATE_CONNECTING &&
           m_prevConnectionState == PVR_CONNECTION_STATE_UNKNOWN)
    m_ignoreClient = true; // ignore until connected
}

PVR_CONNECTION_STATE CPVRClient::GetPreviousConnectionState() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_prevConnectionState;
}

bool CPVRClient::IgnoreClient() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_ignoreClient;
}

bool CPVRClient::IsEnabled() const
{
  if (InstanceId() == ADDON_SINGLETON_INSTANCE_ID)
  {
    return !CServiceBroker::GetAddonMgr().IsAddonDisabled(ID());
  }
  else
  {
    bool instanceEnabled{false};
    Addon()->ReloadSettings(InstanceId());
    Addon()->GetSettingBool(ADDON_SETTING_INSTANCE_ENABLED_VALUE, instanceEnabled, InstanceId());
    return instanceEnabled;
  }
}

int CPVRClient::GetID() const
{
  return m_iClientId;
}

bool CPVRClient::GetAddonProperties()
{
  if (!GetAddonNameStringProperties())
    return false;

  PVR_ADDON_CAPABILITIES addonCapabilities = {};

  /* get the capabilities */
  PVR_ERROR retVal = DoAddonCall(
      __func__,
      [&addonCapabilities](const AddonInstance* addon)
      { return addon->toAddon->GetCapabilities(addon, &addonCapabilities); },
      true, false);

  if (retVal == PVR_ERROR_NO_ERROR)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_clientCapabilities = addonCapabilities;
  }

  /* free the resources of the capabilities instance */
  DoAddonCall(
      __func__,
      [&addonCapabilities](const AddonInstance* addon)
      { return addon->toAddon->FreeCapabilities(addon, &addonCapabilities); },
      true, false);

  return (retVal == PVR_ERROR_NO_ERROR);
}

bool CPVRClient::GetAddonNameStringProperties()
{
  std::string backendName;
  std::string connectionString;
  std::string backendVersion;
  std::string backendHostname;

  /* get the name of the backend */
  PVR_ERROR retVal = DoAddonCall(
      __func__,
      [&backendName](const AddonInstance* addon)
      {
        char* strBackendName{nullptr};
        const PVR_ERROR error{addon->toAddon->GetBackendName(addon, &strBackendName)};
        if (error == PVR_ERROR_NO_ERROR && strBackendName != nullptr)
          backendName = strBackendName;
        addon->toAddon->FreeString(addon, strBackendName);
        return error;
      },
      true, false);

  if (retVal != PVR_ERROR_NO_ERROR)
    return false;

  /* get the connection string */
  retVal = DoAddonCall(
      __func__,
      [&connectionString](const AddonInstance* addon)
      {
        char* strConnectionString{nullptr};
        const PVR_ERROR error{addon->toAddon->GetConnectionString(addon, &strConnectionString)};
        if (error == PVR_ERROR_NO_ERROR && strConnectionString != nullptr)
          connectionString = strConnectionString;
        addon->toAddon->FreeString(addon, strConnectionString);
        return error;
      },
      true, false);

  if (retVal != PVR_ERROR_NO_ERROR && retVal != PVR_ERROR_NOT_IMPLEMENTED)
    return false;

  /* backend version number */
  retVal = DoAddonCall(
      __func__,
      [&backendVersion](const AddonInstance* addon)
      {
        char* strBackendVersion{nullptr};
        const PVR_ERROR error{addon->toAddon->GetBackendVersion(addon, &strBackendVersion)};
        if (error == PVR_ERROR_NO_ERROR && strBackendVersion != nullptr)
          backendVersion = strBackendVersion;
        addon->toAddon->FreeString(addon, strBackendVersion);
        return error;
      },
      true, false);

  if (retVal != PVR_ERROR_NO_ERROR)
    return false;

  /* backend hostname */
  retVal = DoAddonCall(
      __func__,
      [&backendHostname](const AddonInstance* addon)
      {
        char* strBackendHostname{nullptr};
        const PVR_ERROR error{addon->toAddon->GetBackendHostname(addon, &strBackendHostname)};
        if (error == PVR_ERROR_NO_ERROR && strBackendHostname != nullptr)
          backendHostname = strBackendHostname;
        addon->toAddon->FreeString(addon, strBackendHostname);
        return error;
      },
      true, false);

  if (retVal != PVR_ERROR_NO_ERROR && retVal != PVR_ERROR_NOT_IMPLEMENTED)
    return false;

  /* update the members */
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_strBackendName = backendName;
  m_strConnectionString = connectionString;
  m_strBackendVersion = backendVersion;
  m_strBackendHostname = backendHostname;

  return true;
}

const std::string& CPVRClient::GetBackendName() const
{
  return m_strBackendName;
}

const std::string& CPVRClient::GetBackendVersion() const
{
  return m_strBackendVersion;
}

const std::string& CPVRClient::GetBackendHostname() const
{
  return m_strBackendHostname;
}

const std::string& CPVRClient::GetConnectionString() const
{
  return m_strConnectionString;
}

std::string CPVRClient::GetClientName() const
{
  return Name();
}

std::string CPVRClient::GetInstanceName() const
{
  std::string instanceName;
  if (Addon()->SupportsInstanceSettings())
    Addon()->GetSettingString(ADDON_SETTING_INSTANCE_NAME_VALUE, instanceName, InstanceId());

  return instanceName;
}

std::string CPVRClient::GetFullClientName() const
{
  if (Addon()->SupportsInstanceSettings())
  {
    std::string instanceName;
    Addon()->GetSettingString(ADDON_SETTING_INSTANCE_NAME_VALUE, instanceName, InstanceId());
    if (!instanceName.empty())
      return StringUtils::Format("{} ({})", Name(), instanceName);
  }
  return Name();
}

PVR_ERROR CPVRClient::GetDriveSpace(uint64_t& iTotal, uint64_t& iUsed) const
{
  /* default to 0 in case of error */
  iTotal = 0;
  iUsed = 0;

  return DoAddonCall(__func__,
                     [&iTotal, &iUsed](const AddonInstance* addon)
                     {
                       uint64_t iTotalSpace = 0;
                       uint64_t iUsedSpace = 0;
                       PVR_ERROR error =
                           addon->toAddon->GetDriveSpace(addon, &iTotalSpace, &iUsedSpace);
                       if (error == PVR_ERROR_NO_ERROR)
                       {
                         iTotal = iTotalSpace;
                         iUsed = iUsedSpace;
                       }
                       return error;
                     });
}

PVR_ERROR CPVRClient::StartChannelScan()
{
  return DoAddonCall(
      __func__,
      [](const AddonInstance* addon) { return addon->toAddon->OpenDialogChannelScan(addon); },
      m_clientCapabilities.SupportsChannelScan());
}

PVR_ERROR CPVRClient::OpenDialogChannelAdd(const std::shared_ptr<const CPVRChannel>& channel)
{
  return DoAddonCall(
      __func__,
      [channel](const AddonInstance* addon)
      {
        const CAddonChannel addonChannel{*channel};
        return addon->toAddon->OpenDialogChannelAdd(addon, &addonChannel);
      },
      m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::OpenDialogChannelSettings(const std::shared_ptr<const CPVRChannel>& channel)
{
  return DoAddonCall(
      __func__,
      [channel](const AddonInstance* addon)
      {
        const CAddonChannel addonChannel{*channel};
        return addon->toAddon->OpenDialogChannelSettings(addon, &addonChannel);
      },
      m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::DeleteChannel(const std::shared_ptr<const CPVRChannel>& channel)
{
  return DoAddonCall(
      __func__,
      [channel](const AddonInstance* addon)
      {
        const CAddonChannel addonChannel{*channel};
        return addon->toAddon->DeleteChannel(addon, &addonChannel);
      },
      m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::RenameChannel(const std::shared_ptr<const CPVRChannel>& channel)
{
  return DoAddonCall(
      __func__,
      [channel](const AddonInstance* addon)
      {
        const CAddonChannel addonChannel{*channel, channel->ChannelName()};
        return addon->toAddon->RenameChannel(addon, &addonChannel);
      },
      m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::GetEPGForChannel(int iChannelUid,
                                       CPVREpg* epg,
                                       time_t start,
                                       time_t end) const
{
  return DoAddonCall(
      __func__,
      [this, iChannelUid, epg, start, end](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = epg;

        int iPVRTimeCorrection =
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;

        return addon->toAddon->GetEPGForChannel(addon, &handle, iChannelUid,
                                                start ? start - iPVRTimeCorrection : 0,
                                                end ? end - iPVRTimeCorrection : 0);
      },
      m_clientCapabilities.SupportsEPG());
}

PVR_ERROR CPVRClient::SetEPGMaxPastDays(int iPastDays)
{
  return DoAddonCall(
      __func__,
      [iPastDays](const AddonInstance* addon)
      { return addon->toAddon->SetEPGMaxPastDays(addon, iPastDays); },
      m_clientCapabilities.SupportsEPG());
}

PVR_ERROR CPVRClient::SetEPGMaxFutureDays(int iFutureDays)
{
  return DoAddonCall(
      __func__,
      [iFutureDays](const AddonInstance* addon)
      { return addon->toAddon->SetEPGMaxFutureDays(addon, iFutureDays); },
      m_clientCapabilities.SupportsEPG());
}

PVR_ERROR CPVRClient::IsRecordable(const std::shared_ptr<const CPVREpgInfoTag>& tag,
                                   bool& bIsRecordable) const
{
  return DoAddonCall(
      __func__,
      [tag, &bIsRecordable](const AddonInstance* addon)
      {
        CAddonEpgTag addonTag(*tag);
        return addon->toAddon->IsEPGTagRecordable(addon, &addonTag, &bIsRecordable);
      },
      m_clientCapabilities.SupportsRecordings() && m_clientCapabilities.SupportsEPG());
}

PVR_ERROR CPVRClient::IsPlayable(const std::shared_ptr<const CPVREpgInfoTag>& tag,
                                 bool& bIsPlayable) const
{
  return DoAddonCall(
      __func__,
      [tag, &bIsPlayable](const AddonInstance* addon)
      {
        CAddonEpgTag addonTag(*tag);
        return addon->toAddon->IsEPGTagPlayable(addon, &addonTag, &bIsPlayable);
      },
      m_clientCapabilities.SupportsEPG());
}

void CPVRClient::WriteStreamProperties(PVR_NAMED_VALUE** properties,
                                       unsigned int iPropertyCount,
                                       CPVRStreamProperties& props)
{
  for (unsigned int i = 0; i < iPropertyCount; ++i)
  {
    const PVR_NAMED_VALUE* prop{properties[i]};
    props.emplace_back(std::make_pair(prop->strName, prop->strValue));
  }
}

PVR_ERROR CPVRClient::GetEpgTagStreamProperties(const std::shared_ptr<const CPVREpgInfoTag>& tag,
                                                CPVRStreamProperties& props) const
{
  return DoAddonCall(__func__,
                     [&tag, &props](const AddonInstance* addon)
                     {
                       CAddonEpgTag addonTag(*tag);

                       PVR_NAMED_VALUE** property_array{nullptr};
                       unsigned int size{0};
                       const PVR_ERROR error{addon->toAddon->GetEPGTagStreamProperties(
                           addon, &addonTag, &property_array, &size)};
                       if (error == PVR_ERROR_NO_ERROR)
                         WriteStreamProperties(property_array, size, props);

                       addon->toAddon->FreeProperties(addon, property_array, size);
                       return error;
                     });
}

PVR_ERROR CPVRClient::GetEpgTagEdl(const std::shared_ptr<const CPVREpgInfoTag>& epgTag,
                                   std::vector<EDL::Edit>& edls) const
{
  edls.clear();
  return DoAddonCall(
      __func__,
      [&epgTag, &edls](const AddonInstance* addon)
      {
        CAddonEpgTag addonTag(*epgTag);

        PVR_EDL_ENTRY** edl_array{nullptr};
        unsigned int size{0};
        const PVR_ERROR error{addon->toAddon->GetEPGTagEdl(addon, &addonTag, &edl_array, &size)};
        if (error == PVR_ERROR_NO_ERROR)
        {
          edls.reserve(size);
          for (unsigned int i = 0; i < size; ++i)
            edls.emplace_back(ConvertAddonEdl(*edl_array[i]));
        }
        addon->toAddon->FreeEdlEntries(addon, edl_array, size);
        return error;
      },
      m_clientCapabilities.SupportsEpgTagEdl());
}

PVR_ERROR CPVRClient::GetChannelGroupsAmount(int& iGroups) const
{
  iGroups = -1;
  return DoAddonCall(
      __func__,
      [&iGroups](const AddonInstance* addon)
      { return addon->toAddon->GetChannelGroupsAmount(addon, &iGroups); },
      m_clientCapabilities.SupportsChannelGroups());
}

PVR_ERROR CPVRClient::GetChannelGroups(CPVRChannelGroups* groups) const
{
  const bool radio{groups->IsRadio()};
  return DoAddonCall(
      __func__,
      [this, groups](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = groups;
        return addon->toAddon->GetChannelGroups(addon, &handle, groups->IsRadio());
      },
      m_clientCapabilities.SupportsChannelGroups() &&
          ((radio && m_clientCapabilities.SupportsRadio()) ||
           (!radio && m_clientCapabilities.SupportsTV())));
}

PVR_ERROR CPVRClient::GetChannelGroupMembers(
    CPVRChannelGroup* group,
    std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers) const
{
  const bool radio{group->IsRadio()};
  return DoAddonCall(
      __func__,
      [this, group, &groupMembers](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = &groupMembers;

        const CAddonChannelGroup addonGroup{*group};
        return addon->toAddon->GetChannelGroupMembers(addon, &handle, &addonGroup);
      },
      m_clientCapabilities.SupportsChannelGroups() &&
          ((radio && m_clientCapabilities.SupportsRadio()) ||
           (!radio && m_clientCapabilities.SupportsTV())));
}

PVR_ERROR CPVRClient::GetProvidersAmount(int& iProviders) const
{
  iProviders = -1;
  return DoAddonCall(__func__, [&iProviders](const AddonInstance* addon)
                     { return addon->toAddon->GetProvidersAmount(addon, &iProviders); });
}

PVR_ERROR CPVRClient::GetProviders(CPVRProvidersContainer& providers) const
{
  return DoAddonCall(
      __func__,
      [this, &providers](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = &providers;
        return addon->toAddon->GetProviders(addon, &handle);
      },
      m_clientCapabilities.SupportsProviders());
}

PVR_ERROR CPVRClient::GetChannelsAmount(int& iChannels) const
{
  iChannels = -1;
  return DoAddonCall(__func__, [&iChannels](const AddonInstance* addon)
                     { return addon->toAddon->GetChannelsAmount(addon, &iChannels); });
}

PVR_ERROR CPVRClient::GetChannels(bool radio,
                                  std::vector<std::shared_ptr<CPVRChannel>>& channels) const
{
  return DoAddonCall(
      __func__,
      [this, radio, &channels](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = &channels;
        const PVR_ERROR error{addon->toAddon->GetChannels(addon, &handle, radio)};

        if (error == PVR_ERROR_NO_ERROR)
        {
          const CDateTime& dateTime{GetDateTimeFirstChannelsAdded()};
          if (!dateTime.IsValid())
          {
            // Remember when first channels were added for this client.
            const_cast<CPVRClient*>(this)->SetDateTimeFirstChannelsAdded(
                CDateTime::GetUTCDateTime());
          }
        }

        return error;
      },
      (radio && m_clientCapabilities.SupportsRadio()) ||
          (!radio && m_clientCapabilities.SupportsTV()));
}

PVR_ERROR CPVRClient::GetRecordingsAmount(bool deleted, int& iRecordings) const
{
  iRecordings = -1;
  return DoAddonCall(
      __func__,
      [deleted, &iRecordings](const AddonInstance* addon)
      { return addon->toAddon->GetRecordingsAmount(addon, deleted, &iRecordings); },
      m_clientCapabilities.SupportsRecordings() &&
          (!deleted || m_clientCapabilities.SupportsRecordingsUndelete()));
}

PVR_ERROR CPVRClient::GetRecordings(CPVRRecordings* results, bool deleted) const
{
  return DoAddonCall(
      __func__,
      [this, results, deleted](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = results;
        return addon->toAddon->GetRecordings(addon, &handle, deleted);
      },
      m_clientCapabilities.SupportsRecordings() &&
          (!deleted || m_clientCapabilities.SupportsRecordingsUndelete()));
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecording& recording)
{
  return DoAddonCall(
      __func__,
      [&recording](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->DeleteRecording(addon, &tag);
      },
      m_clientCapabilities.SupportsRecordings() && m_clientCapabilities.SupportsRecordingsDelete());
}

PVR_ERROR CPVRClient::UndeleteRecording(const CPVRRecording& recording)
{
  return DoAddonCall(
      __func__,
      [&recording](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->UndeleteRecording(addon, &tag);
      },
      m_clientCapabilities.SupportsRecordingsUndelete());
}

PVR_ERROR CPVRClient::DeleteAllRecordingsFromTrash()
{
  return DoAddonCall(
      __func__,
      [](const AddonInstance* addon)
      { return addon->toAddon->DeleteAllRecordingsFromTrash(addon); },
      m_clientCapabilities.SupportsRecordingsUndelete());
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording& recording)
{
  return DoAddonCall(
      __func__,
      [&recording](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->RenameRecording(addon, &tag);
      },
      m_clientCapabilities.SupportsRecordings());
}

PVR_ERROR CPVRClient::SetRecordingLifetime(const CPVRRecording& recording)
{
  return DoAddonCall(
      __func__,
      [&recording](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->SetRecordingLifetime(addon, &tag);
      },
      m_clientCapabilities.SupportsRecordingsLifetimeChange());
}

PVR_ERROR CPVRClient::SetRecordingPlayCount(const CPVRRecording& recording, int count)
{
  return DoAddonCall(
      __func__,
      [&recording, count](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->SetRecordingPlayCount(addon, &tag, count);
      },
      m_clientCapabilities.SupportsRecordingsPlayCount());
}

PVR_ERROR CPVRClient::SetRecordingLastPlayedPosition(const CPVRRecording& recording,
                                                     int lastplayedposition)
{
  return DoAddonCall(
      __func__,
      [&recording, lastplayedposition](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->SetRecordingLastPlayedPosition(addon, &tag, lastplayedposition);
      },
      m_clientCapabilities.SupportsRecordingsLastPlayedPosition());
}

PVR_ERROR CPVRClient::GetRecordingLastPlayedPosition(const CPVRRecording& recording,
                                                     int& iPosition) const
{
  iPosition = -1;
  return DoAddonCall(
      __func__,
      [&recording, &iPosition](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->GetRecordingLastPlayedPosition(addon, &tag, &iPosition);
      },
      m_clientCapabilities.SupportsRecordingsLastPlayedPosition());
}

PVR_ERROR CPVRClient::GetRecordingEdl(const CPVRRecording& recording,
                                      std::vector<EDL::Edit>& edls) const
{
  edls.clear();
  return DoAddonCall(
      __func__,
      [&recording, &edls](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};

        PVR_EDL_ENTRY** edl_array{nullptr};
        unsigned int size{0};
        const PVR_ERROR error{addon->toAddon->GetRecordingEdl(addon, &tag, &edl_array, &size)};
        if (error == PVR_ERROR_NO_ERROR)
        {
          edls.reserve(size);
          for (unsigned int i = 0; i < size; ++i)
            edls.emplace_back(ConvertAddonEdl(*edl_array[i]));
        }
        addon->toAddon->FreeEdlEntries(addon, edl_array, size);
        return error;
      },
      m_clientCapabilities.SupportsRecordingsEdl());
}

PVR_ERROR CPVRClient::GetRecordingSize(const CPVRRecording& recording, int64_t& sizeInBytes) const
{
  return DoAddonCall(
      __func__,
      [&recording, &sizeInBytes](const AddonInstance* addon)
      {
        const CAddonRecording tag{recording};
        return addon->toAddon->GetRecordingSize(addon, &tag, &sizeInBytes);
      },
      m_clientCapabilities.SupportsRecordingsSize());
}

PVR_ERROR CPVRClient::GetTimersAmount(int& iTimers) const
{
  iTimers = -1;
  return DoAddonCall(
      __func__,
      [&iTimers](const AddonInstance* addon)
      { return addon->toAddon->GetTimersAmount(addon, &iTimers); },
      m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::GetTimers(CPVRTimersContainer* results) const
{
  return DoAddonCall(
      __func__,
      [this, results](const AddonInstance* addon)
      {
        PVR_HANDLE_STRUCT handle = {};
        handle.callerAddress = this;
        handle.dataAddress = results;
        return addon->toAddon->GetTimers(addon, &handle);
      },
      m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag& timer)
{
  return DoAddonCall(
      __func__,
      [&timer](const AddonInstance* addon)
      {
        const CAddonTimer tag{timer};
        return addon->toAddon->AddTimer(addon, &tag);
      },
      m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag& timer, bool bForce /* = false */)
{
  return DoAddonCall(
      __func__,
      [&timer, bForce](const AddonInstance* addon)
      {
        const CAddonTimer tag{timer};
        return addon->toAddon->DeleteTimer(addon, &tag, bForce);
      },
      m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag& timer)
{
  return DoAddonCall(
      __func__,
      [&timer](const AddonInstance* addon)
      {
        const CAddonTimer tag{timer};
        return addon->toAddon->UpdateTimer(addon, &tag);
      },
      m_clientCapabilities.SupportsTimers());
}

const std::vector<std::shared_ptr<CPVRTimerType>>& CPVRClient::GetTimerTypes() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_timertypes;
}

PVR_ERROR CPVRClient::UpdateTimerTypes()
{
  std::vector<std::shared_ptr<CPVRTimerType>> timerTypes;

  PVR_ERROR retVal = DoAddonCall(
      __func__,
      [this, &timerTypes](const AddonInstance* addon)
      {
        PVR_TIMER_TYPE** types_array{nullptr};
        unsigned int size{0};
        PVR_ERROR retval{addon->toAddon->GetTimerTypes(addon, &types_array, &size)};

        const bool array_owner{retval == PVR_ERROR_NOT_IMPLEMENTED};
        if (array_owner)
        {
          // begin compat section
          CLog::LogF(LOGWARNING,
                     "Add-on {} does not support timer types. It will work, but not benefit from "
                     "the timer features introduced with PVR Addon API 2.0.0",
                     Name());

          // Create standard timer types (mostly) matching the timer functionality available in Isengard.
          // This is for migration only and does not make changes to the addons obsolete. Addons should
          // work and benefit from some UI changes (e.g. some of the timer settings dialog enhancements),
          // but all old problems/bugs due to static attributes and values will remain the same as in
          // Isengard. Also, new features (like epg search) are not available to addons automatically.
          // This code can be removed once all addons actually support the respective PVR Addon API version.

          size = 2;
          if (m_clientCapabilities.SupportsEPG())
            size++;

          types_array = new PVR_TIMER_TYPE*[size];

          // manual one time
          types_array[0] = new PVR_TIMER_TYPE{};
          types_array[0]->iId = 1;
          types_array[0]->iAttributes =
              PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE |
              PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_TIME |
              PVR_TIMER_TYPE_SUPPORTS_END_TIME | PVR_TIMER_TYPE_SUPPORTS_PRIORITY |
              PVR_TIMER_TYPE_SUPPORTS_LIFETIME | PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS;

          // manual timer rule
          types_array[1] = new PVR_TIMER_TYPE{};
          types_array[1]->iId = 2;
          types_array[1]->iAttributes =
              PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_IS_REPEATING |
              PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE | PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
              PVR_TIMER_TYPE_SUPPORTS_START_TIME | PVR_TIMER_TYPE_SUPPORTS_END_TIME |
              PVR_TIMER_TYPE_SUPPORTS_PRIORITY | PVR_TIMER_TYPE_SUPPORTS_LIFETIME |
              PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY | PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS |
              PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS;

          if (m_clientCapabilities.SupportsEPG())
          {
            // One-shot epg-based
            types_array[2] = new PVR_TIMER_TYPE{};
            types_array[2]->iId = 3;
            types_array[2]->iAttributes =
                PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE | PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE |
                PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                PVR_TIMER_TYPE_SUPPORTS_END_TIME | PVR_TIMER_TYPE_SUPPORTS_PRIORITY |
                PVR_TIMER_TYPE_SUPPORTS_LIFETIME | PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS;
          }

          retval = PVR_ERROR_NO_ERROR;
          // end compat section
        }

        if (retval == PVR_ERROR_NO_ERROR)
        {
          timerTypes.reserve(size);
          for (unsigned int i = 0; i < size; ++i)
          {
            if (types_array[i]->iId == PVR_TIMER_TYPE_NONE)
            {
              CLog::LogF(LOGERROR, "Invalid timer type supplied by add-on {}.", GetID());
              continue;
            }
            timerTypes.emplace_back(
                std::make_shared<CPVRTimerType>(*(types_array[i]), m_iClientId));
          }
        }

        /* free the resources of the timer types array */
        if (array_owner)
        {
          // begin compat section
          for (unsigned int i = 0; i < size; ++i)
            delete types_array[i];

          delete[] types_array;
          // end compat section
        }
        else
        {
          addon->toAddon->FreeTimerTypes(addon, types_array, size);
        }
        types_array = nullptr;

        return retval;
      },
      m_clientCapabilities.SupportsTimers(), false);

  if (retVal == PVR_ERROR_NO_ERROR)
  {
    std::vector<std::shared_ptr<CPVRTimerType>> newTimerTypes;
    newTimerTypes.reserve(timerTypes.size());

    std::unique_lock<CCriticalSection> lock(m_critSection);

    for (const auto& type : timerTypes)
    {
      const auto it = std::find_if(m_timertypes.cbegin(), m_timertypes.cend(),
                                   [&type](const std::shared_ptr<const CPVRTimerType>& entry)
                                   { return entry->GetTypeId() == type->GetTypeId(); });
      if (it == m_timertypes.cend())
      {
        newTimerTypes.emplace_back(type);
      }
      else
      {
        (*it)->Update(*type);
        newTimerTypes.emplace_back(*it);
      }
    }

    m_timertypes = newTimerTypes;
  }
  return retVal;
}

PVR_ERROR CPVRClient::GetStreamReadChunkSize(int& iChunkSize) const
{
  return DoAddonCall(
      __func__,
      [&iChunkSize](const AddonInstance* addon)
      { return addon->toAddon->GetStreamReadChunkSize(addon, &iChunkSize); },
      m_clientCapabilities.SupportsRecordings() || m_clientCapabilities.HandlesInputStream());
}

PVR_ERROR CPVRClient::ReadLiveStream(void* lpBuf, int64_t uiBufSize, int& iRead)
{
  iRead = -1;
  return DoAddonCall(__func__,
                     [&lpBuf, uiBufSize, &iRead](const AddonInstance* addon)
                     {
                       iRead = addon->toAddon->ReadLiveStream(
                           addon, static_cast<unsigned char*>(lpBuf), static_cast<int>(uiBufSize));
                       return (iRead == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::ReadRecordedStream(int64_t streamId,
                                         void* lpBuf,
                                         int64_t uiBufSize,
                                         int& iRead)
{
  iRead = -1;
  return DoAddonCall(__func__,
                     [streamId, &lpBuf, uiBufSize, &iRead](const AddonInstance* addon)
                     {
                       iRead = addon->toAddon->ReadRecordedStream(
                           addon, streamId, static_cast<unsigned char*>(lpBuf),
                           static_cast<int>(uiBufSize));
                       return (iRead == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::SeekLiveStream(int64_t iFilePosition, int iWhence, int64_t& iPosition)
{
  iPosition = -1;
  return DoAddonCall(__func__,
                     [iFilePosition, iWhence, &iPosition](const AddonInstance* addon)
                     {
                       iPosition = addon->toAddon->SeekLiveStream(addon, iFilePosition, iWhence);
                       return (iPosition == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::SeekRecordedStream(int64_t streamId,
                                         int64_t iFilePosition,
                                         int iWhence,
                                         int64_t& iPosition)
{
  iPosition = -1;
  return DoAddonCall(__func__,
                     [streamId, iFilePosition, iWhence, &iPosition](const AddonInstance* addon)
                     {
                       iPosition = addon->toAddon->SeekRecordedStream(addon, streamId,
                                                                      iFilePosition, iWhence);
                       return (iPosition == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::SeekTime(double time, bool backwards, double* startpts)
{
  return DoAddonCall(__func__,
                     [time, backwards, &startpts](const AddonInstance* addon)
                     {
                       return addon->toAddon->SeekTime(addon, time, backwards, startpts)
                                  ? PVR_ERROR_NO_ERROR
                                  : PVR_ERROR_NOT_IMPLEMENTED;
                     });
}

PVR_ERROR CPVRClient::GetLiveStreamLength(int64_t& iLength) const
{
  iLength = -1;
  return DoAddonCall(__func__,
                     [&iLength](const AddonInstance* addon)
                     {
                       iLength = addon->toAddon->LengthLiveStream(addon);
                       return (iLength == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::GetRecordedStreamLength(int64_t streamId, int64_t& iLength) const
{
  iLength = -1;
  return DoAddonCall(__func__,
                     [streamId, &iLength](const AddonInstance* addon)
                     {
                       iLength = addon->toAddon->LengthRecordedStream(addon, streamId);
                       return (iLength == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::SignalQuality(int channelUid, CPVRSignalStatus& qualityinfo) const
{
  return DoAddonCall(__func__,
                     [channelUid, &qualityinfo](const AddonInstance* addon)
                     {
                       PVR_SIGNAL_STATUS info{};
                       const PVR_ERROR error{
                           addon->toAddon->GetSignalStatus(addon, channelUid, &info)};
                       if (error == PVR_ERROR_NO_ERROR)
                         qualityinfo = CPVRSignalStatus{info};

                       addon->toAddon->FreeSignalStatus(addon, &info);
                       return error;
                     });
}

PVR_ERROR CPVRClient::GetDescrambleInfo(int channelUid, CPVRDescrambleInfo& descrambleinfo) const
{
  return DoAddonCall(
      __func__,
      [channelUid, &descrambleinfo](const AddonInstance* addon)
      {
        PVR_DESCRAMBLE_INFO info{};
        const PVR_ERROR error{addon->toAddon->GetDescrambleInfo(addon, channelUid, &info)};
        if (error == PVR_ERROR_NO_ERROR)
          descrambleinfo = CPVRDescrambleInfo{info};

        addon->toAddon->FreeDescrambleInfo(addon, &info);
        return error;
      },
      m_clientCapabilities.SupportsDescrambleInfo());
}

PVR_ERROR CPVRClient::GetChannelStreamProperties(const std::shared_ptr<const CPVRChannel>& channel,
                                                 PVR_SOURCE source,
                                                 CPVRStreamProperties& props) const
{
  return DoAddonCall(
      __func__,
      [this, &channel, source, &props](const AddonInstance* addon)
      {
        if (!CanPlayChannel(channel))
          return PVR_ERROR_NO_ERROR; // no error, but no need to obtain the values from the addon

        const CAddonChannel addonChannel{*channel};

        PVR_NAMED_VALUE** property_array{nullptr};
        unsigned int size{0};
        const PVR_ERROR error{addon->toAddon->GetChannelStreamProperties(
            addon, &addonChannel, source, &property_array, &size)};
        if (error == PVR_ERROR_NO_ERROR)
          WriteStreamProperties(property_array, size, props);

        addon->toAddon->FreeProperties(addon, property_array, size);
        return error;
      });
}

PVR_ERROR CPVRClient::GetRecordingStreamProperties(
    const std::shared_ptr<const CPVRRecording>& recording, CPVRStreamProperties& props) const
{
  return DoAddonCall(
      __func__,
      [this, &recording, &props](const AddonInstance* addon)
      {
        if (!m_clientCapabilities.SupportsRecordings())
          return PVR_ERROR_NO_ERROR; // no error, but no need to obtain the values from the addon

        const CAddonRecording addonRecording(*recording);

        PVR_NAMED_VALUE** property_array{nullptr};
        unsigned int size{0};
        const PVR_ERROR error{addon->toAddon->GetRecordingStreamProperties(addon, &addonRecording,
                                                                           &property_array, &size)};
        if (error == PVR_ERROR_NO_ERROR)
          WriteStreamProperties(property_array, size, props);

        addon->toAddon->FreeProperties(addon, property_array, size);
        return error;
      });
}

PVR_ERROR CPVRClient::GetStreamProperties(PVR_STREAM_PROPERTIES* props) const
{
  return DoAddonCall(__func__, [&props](const AddonInstance* addon)
                     { return addon->toAddon->GetStreamProperties(addon, props); });
}

PVR_ERROR CPVRClient::StreamClosed() const
{
  return DoAddonCall(__func__, [](const AddonInstance* addon)
                     { return addon->toAddon->StreamClosed(addon); });
}

PVR_ERROR CPVRClient::DemuxReset()
{
  return DoAddonCall(
      __func__,
      [](const AddonInstance* addon)
      {
        addon->toAddon->DemuxReset(addon);
        return PVR_ERROR_NO_ERROR;
      },
      m_clientCapabilities.HandlesDemuxing());
}

PVR_ERROR CPVRClient::DemuxAbort()
{
  return DoAddonCall(
      __func__,
      [](const AddonInstance* addon)
      {
        addon->toAddon->DemuxAbort(addon);
        return PVR_ERROR_NO_ERROR;
      },
      m_clientCapabilities.HandlesDemuxing());
}

PVR_ERROR CPVRClient::DemuxFlush()
{
  return DoAddonCall(
      __func__,
      [](const AddonInstance* addon)
      {
        addon->toAddon->DemuxFlush(addon);
        return PVR_ERROR_NO_ERROR;
      },
      m_clientCapabilities.HandlesDemuxing());
}

PVR_ERROR CPVRClient::DemuxRead(DemuxPacket*& packet)
{
  return DoAddonCall(
      __func__,
      [&packet](const AddonInstance* addon)
      {
        packet = static_cast<DemuxPacket*>(addon->toAddon->DemuxRead(addon));
        return packet ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_IMPLEMENTED;
      },
      m_clientCapabilities.HandlesDemuxing());
}

const char* CPVRClient::ToString(const PVR_ERROR error)
{
  switch (error)
  {
    case PVR_ERROR_NO_ERROR:
      return "no error";
    case PVR_ERROR_NOT_IMPLEMENTED:
      return "not implemented";
    case PVR_ERROR_SERVER_ERROR:
      return "server error";
    case PVR_ERROR_SERVER_TIMEOUT:
      return "server timeout";
    case PVR_ERROR_RECORDING_RUNNING:
      return "recording already running";
    case PVR_ERROR_ALREADY_PRESENT:
      return "already present";
    case PVR_ERROR_REJECTED:
      return "rejected by the backend";
    case PVR_ERROR_INVALID_PARAMETERS:
      return "invalid parameters for this method";
    case PVR_ERROR_FAILED:
      return "the command failed";
    case PVR_ERROR_UNKNOWN:
    default:
      return "unknown error";
  }
}

PVR_ERROR CPVRClient::DoAddonCall(const char* strFunctionName,
                                  const std::function<PVR_ERROR(const AddonInstance*)>& function,
                                  bool bIsImplemented /* = true */,
                                  bool bCheckReadyToUse /* = true */) const
{
  // Check preconditions.
  if (!bIsImplemented)
    return PVR_ERROR_NOT_IMPLEMENTED;

  if (m_bBlockAddonCalls)
  {
    CLog::Log(LOGWARNING, "{}: Blocking call to add-on {}.", strFunctionName, GetID());
    return PVR_ERROR_SERVER_ERROR;
  }

  if (bCheckReadyToUse && IgnoreClient())
  {
    CLog::Log(LOGWARNING, "{}: Blocking call to add-on {}. Add-on not (yet) connected.",
              strFunctionName, GetID());
    return PVR_ERROR_SERVER_ERROR;
  }

  if (bCheckReadyToUse && !ReadyToUse())
  {
    CLog::Log(LOGWARNING, "{}: Blocking call to add-on {}. Add-on not ready to use.",
              strFunctionName, GetID());
    return PVR_ERROR_SERVER_ERROR;
  }

  // Call.
  m_allAddonCallsFinished.Reset();
  m_iAddonCalls++;

  //  CLog::LogFC(LOGDEBUG, LOGPVR, "Calling add-on function '{}' on client {}.", strFunctionName,
  //              GetID());

  const PVR_ERROR error = function(m_ifc.pvr);

  //  CLog::LogFC(LOGDEBUG, LOGPVR, "Called add-on function '{}' on client {}. return={}",
  //              strFunctionName, GetID(), error);

  m_iAddonCalls--;
  if (m_iAddonCalls == 0)
    m_allAddonCallsFinished.Set();

  // Log error, if any.
  if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
    CLog::Log(LOGERROR, "{}: Add-on {} returned an error: {}", strFunctionName, GetID(),
              ToString(error));

  return error;
}

bool CPVRClient::CanPlayChannel(const std::shared_ptr<const CPVRChannel>& channel) const
{
  return (m_bReadyToUse && ((m_clientCapabilities.SupportsTV() && !channel->IsRadio()) ||
                            (m_clientCapabilities.SupportsRadio() && channel->IsRadio())));
}

PVR_ERROR CPVRClient::OpenLiveStream(const std::shared_ptr<const CPVRChannel>& channel)
{
  if (!channel)
    return PVR_ERROR_INVALID_PARAMETERS;

  return DoAddonCall(__func__,
                     [this, channel](const AddonInstance* addon)
                     {
                       CloseLiveStream();

                       if (!CanPlayChannel(channel))
                       {
                         CLog::LogFC(LOGDEBUG, LOGPVR, "Add-on {} can not play channel '{}'",
                                     GetID(), channel->ChannelName());
                         return PVR_ERROR_SERVER_ERROR;
                       }
                       else
                       {
                         CLog::LogFC(LOGDEBUG, LOGPVR, "Opening live stream for channel '{}'",
                                     channel->ChannelName());
                         const CAddonChannel addonChannel{*channel};
                         return addon->toAddon->OpenLiveStream(addon, &addonChannel)
                                    ? PVR_ERROR_NO_ERROR
                                    : PVR_ERROR_NOT_IMPLEMENTED;
                       }
                     });
}

PVR_ERROR CPVRClient::OpenRecordedStream(const std::shared_ptr<const CPVRRecording>& recording,
                                         int64_t& streamId)
{
  if (!recording)
    return PVR_ERROR_INVALID_PARAMETERS;

  return DoAddonCall(
      __func__,
      [this, recording, &streamId](const AddonInstance* addon)
      {
        if (!m_clientCapabilities.SupportsMultipleRecordedStreams())
          CloseRecordedStream(streamId);

        const CAddonRecording tag(*recording);
        CLog::LogFC(LOGDEBUG, LOGPVR, "Opening stream for recording '{}'", recording->m_strTitle);
        return addon->toAddon->OpenRecordedStream(addon, &tag, &streamId)
                   ? PVR_ERROR_NO_ERROR
                   : PVR_ERROR_NOT_IMPLEMENTED;
      },
      m_clientCapabilities.SupportsRecordings());
}

PVR_ERROR CPVRClient::CloseLiveStream()
{
  return DoAddonCall(__func__,
                     [](const AddonInstance* addon)
                     {
                       addon->toAddon->CloseLiveStream(addon);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::CloseRecordedStream(int64_t streamId)
{
  return DoAddonCall(__func__,
                     [streamId](const AddonInstance* addon)
                     {
                       addon->toAddon->CloseRecordedStream(addon, streamId);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::IsRecordedStreamRealTime(int64_t streamId, bool& isRealTime) const
{
  if (m_clientCapabilities.SupportsMultipleRecordedStreams())
  {
    return DoAddonCall(
        __func__, [streamId, &isRealTime](const AddonInstance* addon)
        { return addon->toAddon->IsRecordedStreamRealTime(addon, streamId, &isRealTime); });
  }
  else
  {
    return IsRealTimeStream(isRealTime);
  }
}

PVR_ERROR CPVRClient::PauseRecordedStream(int64_t streamId, bool paused)
{
  if (m_clientCapabilities.SupportsMultipleRecordedStreams())
  {
    return DoAddonCall(__func__, [streamId, paused](const AddonInstance* addon)
                       { return addon->toAddon->PauseRecordedStream(addon, streamId, paused); });
  }
  else
  {
    return PauseStream(paused);
  }
}

PVR_ERROR CPVRClient::GetRecordedStreamTimes(int64_t streamId, PVR_STREAM_TIMES* times) const
{
  if (m_clientCapabilities.SupportsMultipleRecordedStreams())
  {
    return DoAddonCall(__func__, [streamId, &times](const AddonInstance* addon)
                       { return addon->toAddon->GetRecordedStreamTimes(addon, streamId, times); });
  }
  else
  {
    return GetStreamTimes(times);
  }
}

PVR_ERROR CPVRClient::PauseStream(bool bPaused)
{
  return DoAddonCall(__func__,
                     [bPaused](const AddonInstance* addon)
                     {
                       addon->toAddon->PauseStream(addon, bPaused);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::SetSpeed(int speed)
{
  return DoAddonCall(__func__,
                     [speed](const AddonInstance* addon)
                     {
                       addon->toAddon->SetSpeed(addon, speed);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::FillBuffer(bool mode)
{
  return DoAddonCall(__func__,
                     [mode](const AddonInstance* addon)
                     {
                       addon->toAddon->FillBuffer(addon, mode);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::CanPauseStream(bool& bCanPause) const
{
  bCanPause = false;
  return DoAddonCall(__func__,
                     [&bCanPause](const AddonInstance* addon)
                     {
                       bCanPause = addon->toAddon->CanPauseStream(addon);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::CanSeekStream(bool& bCanSeek) const
{
  bCanSeek = false;
  return DoAddonCall(__func__,
                     [&bCanSeek](const AddonInstance* addon)
                     {
                       bCanSeek = addon->toAddon->CanSeekStream(addon);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::GetStreamTimes(PVR_STREAM_TIMES* times) const
{
  return DoAddonCall(__func__, [&times](const AddonInstance* addon)
                     { return addon->toAddon->GetStreamTimes(addon, times); });
}

PVR_ERROR CPVRClient::IsRealTimeStream(bool& bRealTime) const
{
  bRealTime = false;
  return DoAddonCall(__func__,
                     [&bRealTime](const AddonInstance* addon)
                     {
                       bRealTime = addon->toAddon->IsRealTimeStream(addon);
                       return PVR_ERROR_NO_ERROR;
                     });
}

PVR_ERROR CPVRClient::OnSystemSleep()
{
  return DoAddonCall(__func__, [](const AddonInstance* addon)
                     { return addon->toAddon->OnSystemSleep(addon); });
}

PVR_ERROR CPVRClient::OnSystemWake()
{
  return DoAddonCall(__func__, [](const AddonInstance* addon)
                     { return addon->toAddon->OnSystemWake(addon); });
}

PVR_ERROR CPVRClient::OnPowerSavingActivated()
{
  return DoAddonCall(__func__, [](const AddonInstance* addon)
                     { return addon->toAddon->OnPowerSavingActivated(addon); });
}

PVR_ERROR CPVRClient::OnPowerSavingDeactivated()
{
  return DoAddonCall(__func__, [](const AddonInstance* addon)
                     { return addon->toAddon->OnPowerSavingDeactivated(addon); });
}

std::shared_ptr<CPVRClientMenuHooks> CPVRClient::GetMenuHooks() const
{
  if (!m_menuhooks)
    m_menuhooks = std::make_shared<CPVRClientMenuHooks>(ID());

  return m_menuhooks;
}

PVR_ERROR CPVRClient::CallEpgTagMenuHook(const CPVRClientMenuHook& hook,
                                         const std::shared_ptr<const CPVREpgInfoTag>& tag)
{
  return DoAddonCall(__func__,
                     [&hook, &tag](const AddonInstance* addon)
                     {
                       CAddonEpgTag addonTag(*tag);

                       PVR_MENUHOOK menuHook;
                       menuHook.category = PVR_MENUHOOK_EPG;
                       menuHook.iHookId = hook.GetId();
                       menuHook.iLocalizedStringId = hook.GetLabelId();

                       return addon->toAddon->CallEPGMenuHook(addon, &menuHook, &addonTag);
                     });
}

PVR_ERROR CPVRClient::CallChannelMenuHook(const CPVRClientMenuHook& hook,
                                          const std::shared_ptr<const CPVRChannel>& channel)
{
  return DoAddonCall(__func__,
                     [&hook, &channel](const AddonInstance* addon)
                     {
                       const CAddonChannel addonChannel{*channel};

                       PVR_MENUHOOK menuHook;
                       menuHook.category = PVR_MENUHOOK_CHANNEL;
                       menuHook.iHookId = hook.GetId();
                       menuHook.iLocalizedStringId = hook.GetLabelId();

                       return addon->toAddon->CallChannelMenuHook(addon, &menuHook, &addonChannel);
                     });
}

PVR_ERROR CPVRClient::CallRecordingMenuHook(const CPVRClientMenuHook& hook,
                                            const std::shared_ptr<const CPVRRecording>& recording,
                                            bool bDeleted)
{
  return DoAddonCall(__func__,
                     [&hook, &recording, &bDeleted](const AddonInstance* addon)
                     {
                       const CAddonRecording tag(*recording);

                       PVR_MENUHOOK menuHook;
                       menuHook.category =
                           bDeleted ? PVR_MENUHOOK_DELETED_RECORDING : PVR_MENUHOOK_RECORDING;
                       menuHook.iHookId = hook.GetId();
                       menuHook.iLocalizedStringId = hook.GetLabelId();

                       return addon->toAddon->CallRecordingMenuHook(addon, &menuHook, &tag);
                     });
}

PVR_ERROR CPVRClient::CallTimerMenuHook(const CPVRClientMenuHook& hook,
                                        const std::shared_ptr<const CPVRTimerInfoTag>& timer)
{
  return DoAddonCall(__func__,
                     [&hook, &timer](const AddonInstance* addon)
                     {
                       const CAddonTimer tag(*timer);

                       PVR_MENUHOOK menuHook;
                       menuHook.category = PVR_MENUHOOK_TIMER;
                       menuHook.iHookId = hook.GetId();
                       menuHook.iLocalizedStringId = hook.GetLabelId();

                       return addon->toAddon->CallTimerMenuHook(addon, &menuHook, &tag);
                     });
}

PVR_ERROR CPVRClient::CallSettingsMenuHook(const CPVRClientMenuHook& hook)
{
  return DoAddonCall(__func__,
                     [&hook](const AddonInstance* addon)
                     {
                       PVR_MENUHOOK menuHook;
                       menuHook.category = PVR_MENUHOOK_SETTING;
                       menuHook.iHookId = hook.GetId();
                       menuHook.iLocalizedStringId = hook.GetLabelId();

                       return addon->toAddon->CallSettingsMenuHook(addon, &menuHook);
                     });
}

void CPVRClient::SetPriority(int iPriority)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_priority != iPriority)
  {
    m_priority = iPriority;
    if (m_iClientId != PVR_CLIENT_INVALID_UID)
    {
      CServiceBroker::GetPVRManager().GetTVDatabase()->Persist(*this);
    }
    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ClientsPrioritiesInvalidated);
  }
}

int CPVRClient::GetPriority() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_priority.has_value() && m_iClientId != PVR_CLIENT_INVALID_UID)
  {
    m_priority = CServiceBroker::GetPVRManager().GetTVDatabase()->GetPriority(*this);
  }
  return *m_priority;
}

const CDateTime& CPVRClient::GetDateTimeFirstChannelsAdded() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_firstChannelsAdded.has_value() && m_iClientId != PVR_CLIENT_INVALID_UID)
  {
    m_firstChannelsAdded =
        CServiceBroker::GetPVRManager().GetTVDatabase()->GetDateTimeFirstChannelsAdded(*this);
  }
  return *m_firstChannelsAdded;
}

void CPVRClient::SetDateTimeFirstChannelsAdded(const CDateTime& dateTime)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_firstChannelsAdded != dateTime)
  {
    m_firstChannelsAdded = dateTime;
    if (m_iClientId != PVR_CLIENT_INVALID_UID)
    {
      CServiceBroker::GetPVRManager().GetTVDatabase()->Persist(*this);
    }
  }
}

void CPVRClient::HandleAddonCallback(const char* strFunctionName,
                                     void* kodiInstance,
                                     const std::function<void(CPVRClient* client)>& function,
                                     bool bForceCall /* = false */)
{
  // Check preconditions.
  CPVRClient* client = static_cast<CPVRClient*>(kodiInstance);
  if (!client)
  {
    CLog::Log(LOGERROR, "{}: No instance pointer given!", strFunctionName);
    return;
  }

  if (!bForceCall && client->m_bBlockAddonCalls && client->m_iAddonCalls == 0)
  {
    CLog::Log(LOGWARNING, LOGPVR, "{}: Ignoring callback from PVR client '{}'", strFunctionName,
              client->ID());
    return;
  }

  // Call.
  function(client);
}

void CPVRClient::cb_transfer_channel_group(void* kodiInstance,
                                           const PVR_HANDLE handle,
                                           const PVR_CHANNEL_GROUP* group)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!handle || !group)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        if (strlen(group->strGroupName) == 0)
                        {
                          CLog::LogF(LOGERROR, "Empty group name");
                          return;
                        }

                        // transfer this entry to the groups container
                        CPVRChannelGroups* kodiGroups =
                            static_cast<CPVRChannelGroups*>(handle->dataAddress);
                        const auto transferGroup = kodiGroups->GetGroupFactory()->CreateClientGroup(
                            *group, client->GetID(), kodiGroups->GetGroupAll());
                        kodiGroups->UpdateFromClient(transferGroup);
                      });
}

void CPVRClient::cb_transfer_channel_group_member(void* kodiInstance,
                                                  const PVR_HANDLE handle,
                                                  const PVR_CHANNEL_GROUP_MEMBER* member)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!handle || !member)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        const std::shared_ptr<CPVRChannel> channel =
                            CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(
                                member->iChannelUniqueId, client->GetID());
                        if (!channel)
                        {
                          CLog::LogF(LOGERROR, "Cannot find group '{}' or channel '{}'",
                                     member->strGroupName, member->iChannelUniqueId);
                        }
                        else
                        {
                          auto* groupMembers =
                              static_cast<std::vector<std::shared_ptr<CPVRChannelGroupMember>>*>(
                                  handle->dataAddress);
                          groupMembers->emplace_back(std::make_shared<CPVRChannelGroupMember>(
                              member->strGroupName, client->GetID(), member->iOrder, channel));
                        }
                      });
}

void CPVRClient::cb_transfer_epg_entry(void* kodiInstance,
                                       const PVR_HANDLE handle,
                                       const EPG_TAG* epgentry)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!handle || !epgentry)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        // transfer this entry to the epg
                        CPVREpg* epg = static_cast<CPVREpg*>(handle->dataAddress);
                        epg->UpdateEntry(epgentry, client->GetID());
                      });
}

void CPVRClient::cb_transfer_provider_entry(void* kodiInstance,
                                            const PVR_HANDLE handle,
                                            const PVR_PROVIDER* provider)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRClient* client = static_cast<CPVRClient*>(kodiInstance);
  CPVRProvidersContainer* kodiProviders = static_cast<CPVRProvidersContainer*>(handle->dataAddress);
  if (!provider || !client || !kodiProviders)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  /* transfer this entry to the internal channels group */
  std::shared_ptr<CPVRProvider> transferProvider(
      std::make_shared<CPVRProvider>(*provider, client->GetID()));
  kodiProviders->UpdateFromClient(transferProvider);
}

void CPVRClient::cb_transfer_channel_entry(void* kodiInstance,
                                           const PVR_HANDLE handle,
                                           const PVR_CHANNEL* channel)
{
  HandleAddonCallback(
      __func__, kodiInstance,
      [&](CPVRClient* client)
      {
        if (!handle || !channel)
        {
          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
          return;
        }

        auto* channels =
            static_cast<std::vector<std::shared_ptr<CPVRChannel>>*>(handle->dataAddress);
        channels->emplace_back(std::make_shared<CPVRChannel>(*channel, client->GetID()));
      });
}

void CPVRClient::cb_transfer_recording_entry(void* kodiInstance,
                                             const PVR_HANDLE handle,
                                             const PVR_RECORDING* recording)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!handle || !recording)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        // transfer this entry to the recordings container
                        const std::shared_ptr<CPVRRecording> transferRecording =
                            std::make_shared<CPVRRecording>(*recording, client->GetID());
                        CPVRRecordings* recordings =
                            static_cast<CPVRRecordings*>(handle->dataAddress);
                        recordings->UpdateFromClient(transferRecording, *client);
                      });
}

void CPVRClient::cb_transfer_timer_entry(void* kodiInstance,
                                         const PVR_HANDLE handle,
                                         const PVR_TIMER* timer)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!handle || !timer)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        // Note: channel can be nullptr here, for instance for epg-based timer rules
                        //       ("record on any channel" condition)
                        const std::shared_ptr<CPVRChannel> channel =
                            CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(
                                timer->iClientChannelUid, client->GetID());

                        // transfer this entry to the timers container
                        const std::shared_ptr<CPVRTimerInfoTag> transferTimer =
                            std::make_shared<CPVRTimerInfoTag>(*timer, channel, client->GetID());
                        CPVRTimersContainer* timers =
                            static_cast<CPVRTimersContainer*>(handle->dataAddress);
                        timers->UpdateFromClient(transferTimer);
                      });
}

void CPVRClient::cb_add_menu_hook(void* kodiInstance, const PVR_MENUHOOK* hook)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!hook)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        client->GetMenuHooks()->AddHook(*hook);
                      });
}

void CPVRClient::cb_recording_notification(void* kodiInstance,
                                           const char* strName,
                                           const char* strFileName,
                                           bool bOnOff)
{
  HandleAddonCallback(
      __func__, kodiInstance,
      [&](CPVRClient* client)
      {
        if (!strFileName)
        {
          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
          return;
        }

        const std::string strLine1 = StringUtils::Format(
            g_localizeStrings.Get(bOnOff ? 19197 : 19198), client->GetFullClientName());
        std::string strLine2;
        if (strName)
          strLine2 = strName;
        else
          strLine2 = strFileName;

        // display a notification for 5 seconds
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strLine1, strLine2, 5000,
                                              false);
        auto eventLog = CServiceBroker::GetEventLog();
        if (eventLog)
          eventLog->Add(EventPtr(new CNotificationEvent(client->GetFullClientName(), strLine1,
                                                        client->Icon(), strLine2)));

        CLog::LogFC(LOGDEBUG, LOGPVR, "Recording {} on client {}. name='{}' filename='{}'",
                    bOnOff ? "started" : "finished", client->GetID(), strName, strFileName);
      });
}

void CPVRClient::cb_trigger_channel_update(void* kodiInstance)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        // update channels in the next iteration of the pvrmanager's main loop
                        CServiceBroker::GetPVRManager().TriggerChannelsUpdate(client->GetID());
                      });
}

void CPVRClient::cb_trigger_provider_update(void* kodiInstance)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        /* update the providers table in the next iteration of the pvrmanager's main loop */
                        CServiceBroker::GetPVRManager().TriggerProvidersUpdate(client->GetID());
                      });
}

void CPVRClient::cb_trigger_timer_update(void* kodiInstance)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        // update timers in the next iteration of the pvrmanager's main loop
                        CServiceBroker::GetPVRManager().TriggerTimersUpdate(client->GetID());
                      });
}

void CPVRClient::cb_trigger_recording_update(void* kodiInstance)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        // update recordings in the next iteration of the pvrmanager's main loop
                        CServiceBroker::GetPVRManager().TriggerRecordingsUpdate(client->GetID());
                      });
}

void CPVRClient::cb_trigger_channel_groups_update(void* kodiInstance)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        // update all channel groups in the next iteration of the pvrmanager's main loop
                        CServiceBroker::GetPVRManager().TriggerChannelGroupsUpdate(client->GetID());
                      });
}

void CPVRClient::cb_trigger_epg_update(void* kodiInstance, unsigned int iChannelUid)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client) {
                        CServiceBroker::GetPVRManager().EpgContainer().UpdateRequest(
                            client->GetID(), iChannelUid);
                      });
}

void CPVRClient::cb_free_demux_packet(void* kodiInstance, DEMUX_PACKET* pPacket)
{
  HandleAddonCallback(
      __func__, kodiInstance,
      [&](CPVRClient* client)
      { CDVDDemuxUtils::FreeDemuxPacket(static_cast<DemuxPacket*>(pPacket)); },
      true);
}

DEMUX_PACKET* CPVRClient::cb_allocate_demux_packet(void* kodiInstance, int iDataSize)
{
  DEMUX_PACKET* result = nullptr;

  HandleAddonCallback(
      __func__, kodiInstance,
      [&](CPVRClient* client) { result = CDVDDemuxUtils::AllocateDemuxPacket(iDataSize); }, true);

  return result;
}

void CPVRClient::cb_connection_state_change(void* kodiInstance,
                                            const char* strConnectionString,
                                            PVR_CONNECTION_STATE newState,
                                            const char* strMessage)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!strConnectionString)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        const PVR_CONNECTION_STATE prevState(client->GetConnectionState());
                        if (prevState == newState)
                          return;

                        CLog::LogFC(LOGDEBUG, LOGPVR,
                                    "Connection state for client {} changed from {} to {}",
                                    client->GetID(), prevState, newState);

                        client->SetConnectionState(newState);

                        std::string msg;
                        if (strMessage)
                          msg = strMessage;

                        CServiceBroker::GetPVRManager().ConnectionStateChange(
                            client, std::string(strConnectionString), newState, msg);
                      });
}

void CPVRClient::cb_epg_event_state_change(void* kodiInstance,
                                           EPG_TAG* tag,
                                           EPG_EVENT_STATE newState)
{
  HandleAddonCallback(__func__, kodiInstance,
                      [&](CPVRClient* client)
                      {
                        if (!tag)
                        {
                          CLog::LogF(LOGERROR, "Invalid callback parameter(s)");
                          return;
                        }

                        // Note: channel data and epg id may not yet be available. Tag will be fully initialized later.
                        const std::shared_ptr<CPVREpgInfoTag> epgTag =
                            std::make_shared<CPVREpgInfoTag>(*tag, client->GetID(), nullptr, -1);
                        CServiceBroker::GetPVRManager().EpgContainer().UpdateFromClient(epgTag,
                                                                                        newState);
                      });
}

class CCodecIds
{
public:
  virtual ~CCodecIds() = default;

  static CCodecIds& GetInstance()
  {
    static CCodecIds _instance;
    return _instance;
  }

  PVR_CODEC GetCodecByName(const char* strCodecName)
  {
    PVR_CODEC retVal = PVR_INVALID_CODEC;
    if (strlen(strCodecName) == 0)
      return retVal;

    std::string strUpperCodecName = strCodecName;
    StringUtils::ToUpper(strUpperCodecName);

    std::map<std::string, PVR_CODEC>::const_iterator it = m_lookup.find(strUpperCodecName);
    if (it != m_lookup.end())
      retVal = it->second;

    return retVal;
  }

private:
  CCodecIds()
  {
    // get ids and names
    const AVCodec* codec = nullptr;
    void* i = nullptr;
    PVR_CODEC tmp;
    while ((codec = av_codec_iterate(&i)))
    {
      if (av_codec_is_decoder(codec))
      {
        tmp.codec_type = static_cast<PVR_CODEC_TYPE>(codec->type);
        tmp.codec_id = codec->id;

        std::string strUpperCodecName = codec->name;
        StringUtils::ToUpper(strUpperCodecName);

        m_lookup.insert(std::make_pair(strUpperCodecName, tmp));
      }
    }

    // teletext is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = PVR_CODEC_TYPE_SUBTITLE;
    tmp.codec_id = AV_CODEC_ID_DVB_TELETEXT;
    m_lookup.insert(std::make_pair("TELETEXT", tmp));

    // rds is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = PVR_CODEC_TYPE_RDS;
    tmp.codec_id = AV_CODEC_ID_NONE;
    m_lookup.insert(std::make_pair("RDS", tmp));

    // ID3 is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = PVR_CODEC_TYPE_ID3;
    tmp.codec_id = AV_CODEC_ID_NONE;
    m_lookup.insert({"ID3", tmp});
  }

  std::map<std::string, PVR_CODEC> m_lookup;
};

PVR_CODEC CPVRClient::cb_get_codec_by_name(const void* kodiInstance, const char* strCodecName)
{
  PVR_CODEC result = PVR_INVALID_CODEC;

  HandleAddonCallback(
      __func__, const_cast<void*>(kodiInstance),
      [&](CPVRClient* client) { result = CCodecIds::GetInstance().GetCodecByName(strCodecName); },
      true);

  return result;
}

} // namespace PVR
