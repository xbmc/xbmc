/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClient.h"

#include <algorithm>
#include <cmath>
#include <memory>

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "ServiceBroker.h"
#include "addons/PVRClientMenuHooks.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "pvr/timers/PVRTimers.h"

using namespace ADDON;

namespace PVR
{

#define DEFAULT_INFO_STRING_VALUE "unknown"

CPVRClient::CPVRClient(CAddonInfo addonInfo)
  : CAddonDll(std::move(addonInfo))
{
  ResetProperties();
}

CPVRClient::~CPVRClient(void)
{
  Destroy();
}

void CPVRClient::StopRunningInstance()
{
  const ADDON::AddonPtr addon(GetRunningInstance());
  if (addon)
  {
    // stop the pvr manager and stop and unload the running pvr addon. pvr manager will be restarted on demand.
    CServiceBroker::GetPVRManager().Stop();
    CServiceBroker::GetPVRManager().Clients()->StopClient(addon, false);
  }
}

void CPVRClient::OnPreInstall()
{
  // note: this method is also called on update; thus stop and unload possibly running instance
  StopRunningInstance();
  CAddon::OnPreInstall();
}

void CPVRClient::OnPreUnInstall()
{
  StopRunningInstance();
  CAddon::OnPreUnInstall();
}

ADDON::AddonPtr CPVRClient::GetRunningInstance() const
{
  ADDON::AddonPtr addon;
  CServiceBroker::GetPVRManager().Clients()->GetClient(ID(), addon);
  return addon;
}

void CPVRClient::ResetProperties(int iClientId /* = PVR_INVALID_CLIENT_ID */)
{
  CSingleLock lock(m_critSection);

  /* initialise members */
  m_strUserPath           = CSpecialProtocol::TranslatePath(Profile());
  m_strClientPath         = CSpecialProtocol::TranslatePath(Path());
  m_bReadyToUse           = false;
  m_bBlockAddonCalls      = false;
  m_connectionState       = PVR_CONNECTION_STATE_UNKNOWN;
  m_prevConnectionState   = PVR_CONNECTION_STATE_UNKNOWN;
  m_ignoreClient          = false;
  m_iClientId             = iClientId;
  m_iPriority             = 0;
  m_bPriorityFetched      = false;
  m_strBackendVersion     = DEFAULT_INFO_STRING_VALUE;
  m_strConnectionString   = DEFAULT_INFO_STRING_VALUE;
  m_strFriendlyName       = DEFAULT_INFO_STRING_VALUE;
  m_strBackendName        = DEFAULT_INFO_STRING_VALUE;
  m_strBackendHostname.clear();
  m_menuhooks.reset();
  m_timertypes.clear();
  m_clientCapabilities.clear();

  m_struct = {{0}};
  m_struct.props.strUserPath = m_strUserPath.c_str();
  m_struct.props.strClientPath = m_strClientPath.c_str();
  m_struct.props.iEpgMaxDays = CServiceBroker::GetPVRManager().EpgContainer().GetFutureDaysToDisplay();

  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.TransferEpgEntry = cb_transfer_epg_entry;
  m_struct.toKodi.TransferChannelEntry = cb_transfer_channel_entry;
  m_struct.toKodi.TransferTimerEntry = cb_transfer_timer_entry;
  m_struct.toKodi.TransferRecordingEntry = cb_transfer_recording_entry;
  m_struct.toKodi.AddMenuHook = cb_add_menu_hook;
  m_struct.toKodi.Recording = cb_recording;
  m_struct.toKodi.TriggerChannelUpdate = cb_trigger_channel_update;
  m_struct.toKodi.TriggerChannelGroupsUpdate = cb_trigger_channel_groups_update;
  m_struct.toKodi.TriggerTimerUpdate = cb_trigger_timer_update;
  m_struct.toKodi.TriggerRecordingUpdate = cb_trigger_recording_update;
  m_struct.toKodi.TriggerEpgUpdate = cb_trigger_epg_update;
  m_struct.toKodi.FreeDemuxPacket = cb_free_demux_packet;
  m_struct.toKodi.AllocateDemuxPacket = cb_allocate_demux_packet;
  m_struct.toKodi.TransferChannelGroup = cb_transfer_channel_group;
  m_struct.toKodi.TransferChannelGroupMember = cb_transfer_channel_group_member;
  m_struct.toKodi.ConnectionStateChange = cb_connection_state_change;
  m_struct.toKodi.EpgEventStateChange = cb_epg_event_state_change;
  m_struct.toKodi.GetCodecByName = cb_get_codec_by_name;
}

ADDON_STATUS CPVRClient::Create(int iClientId)
{
  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
  if (iClientId <= PVR_INVALID_CLIENT_ID)
    return status;

  /* reset all properties to defaults */
  ResetProperties(iClientId);

  /* initialise the add-on */
  bool bReadyToUse(false);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating PVR add-on instance '%s'", Name().c_str());
  if ((status = CAddonDll::Create(ADDON_INSTANCE_PVR, &m_struct, &m_struct.props)) == ADDON_STATUS_OK)
    bReadyToUse = GetAddonProperties();

  m_bReadyToUse = bReadyToUse;
  return status;
}

bool CPVRClient::DllLoaded(void) const
{
  return CAddonDll::DllLoaded();
}

void CPVRClient::Destroy(void)
{
  if (!m_bReadyToUse)
    return;

  m_bReadyToUse = false;

  /* reset 'ready to use' to false */
  CLog::LogFC(LOGDEBUG, LOGPVR, "Destroying PVR add-on instance '%s'", GetFriendlyName().c_str());

  /* destroy the add-on */
  CAddonDll::Destroy();

  if (m_menuhooks)
    m_menuhooks->Clear();

  /* reset all properties to defaults */
  ResetProperties();
}

void CPVRClient::Stop()
{
  m_bBlockAddonCalls = true;
}

void CPVRClient::Continue()
{
  m_bBlockAddonCalls = false;
}

void CPVRClient::ReCreate(void)
{
  int iClientID(m_iClientId);
  Destroy();

  /* recreate the instance */
  Create(iClientID);
}

bool CPVRClient::ReadyToUse(void) const
{
  return m_bReadyToUse;
}

PVR_CONNECTION_STATE CPVRClient::GetConnectionState(void) const
{
  CSingleLock lock(m_critSection);
  return m_connectionState;
}

void CPVRClient::SetConnectionState(PVR_CONNECTION_STATE state)
{
  CSingleLock lock(m_critSection);

  m_prevConnectionState = m_connectionState;
  m_connectionState = state;

  if (m_connectionState == PVR_CONNECTION_STATE_CONNECTED)
    m_ignoreClient = false;
  else if (m_connectionState == PVR_CONNECTION_STATE_CONNECTING &&
           m_prevConnectionState == PVR_CONNECTION_STATE_UNKNOWN)
    m_ignoreClient = true;
}

PVR_CONNECTION_STATE CPVRClient::GetPreviousConnectionState(void) const
{
  CSingleLock lock(m_critSection);
  return m_prevConnectionState;
}

bool CPVRClient::IgnoreClient(void) const
{
  CSingleLock lock(m_critSection);
  return m_ignoreClient;
}

int CPVRClient::GetID(void) const
{
  return m_iClientId;
}

/*!
 * @brief Copy over group info from xbmcGroup to addonGroup.
 * @param xbmcGroup The group on XBMC's side.
 * @param addonGroup The group on the addon's side.
 */
void CPVRClient::WriteClientGroupInfo(const CPVRChannelGroup &xbmcGroup, PVR_CHANNEL_GROUP &addonGroup)
{
  addonGroup = {{0}};
  addonGroup.bIsRadio = xbmcGroup.IsRadio();
  strncpy(addonGroup.strGroupName, xbmcGroup.GroupName().c_str(), sizeof(addonGroup.strGroupName) - 1);
}

/*!
 * @brief Copy over recording info from xbmcRecording to addonRecording.
 * @param xbmcRecording The recording on XBMC's side.
 * @param addonRecording The recording on the addon's side.
 */
void CPVRClient::WriteClientRecordingInfo(const CPVRRecording &xbmcRecording, PVR_RECORDING &addonRecording)
{
  time_t recTime;
  xbmcRecording.RecordingTimeAsUTC().GetAsTime(recTime);

  addonRecording = {{0}};
  strncpy(addonRecording.strRecordingId, xbmcRecording.m_strRecordingId.c_str(), sizeof(addonRecording.strRecordingId) - 1);
  strncpy(addonRecording.strTitle, xbmcRecording.m_strTitle.c_str(), sizeof(addonRecording.strTitle) - 1);
  strncpy(addonRecording.strEpisodeName, xbmcRecording.m_strShowTitle.c_str(), sizeof(addonRecording.strEpisodeName) - 1);
  addonRecording.iSeriesNumber = xbmcRecording.m_iSeason;
  addonRecording.iEpisodeNumber = xbmcRecording.m_iEpisode;
  addonRecording.iYear = xbmcRecording.GetYear();
  strncpy(addonRecording.strDirectory, xbmcRecording.m_strDirectory.c_str(), sizeof(addonRecording.strDirectory) - 1);
  strncpy(addonRecording.strPlotOutline, xbmcRecording.m_strPlotOutline.c_str(), sizeof(addonRecording.strPlotOutline) - 1);
  strncpy(addonRecording.strPlot, xbmcRecording.m_strPlot.c_str(), sizeof(addonRecording.strPlot) - 1);
  strncpy(addonRecording.strGenreDescription, xbmcRecording.GetGenresLabel().c_str(), sizeof(addonRecording.strGenreDescription) - 1);
  strncpy(addonRecording.strChannelName, xbmcRecording.m_strChannelName.c_str(), sizeof(addonRecording.strChannelName) - 1);
  strncpy(addonRecording.strIconPath, xbmcRecording.m_strIconPath.c_str(), sizeof(addonRecording.strIconPath) - 1);
  strncpy(addonRecording.strThumbnailPath, xbmcRecording.m_strThumbnailPath.c_str(), sizeof(addonRecording.strThumbnailPath) - 1);
  strncpy(addonRecording.strFanartPath, xbmcRecording.m_strFanartPath.c_str(), sizeof(addonRecording.strFanartPath) - 1);
  addonRecording.recordingTime = recTime - CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
  addonRecording.iDuration = xbmcRecording.GetDuration();
  addonRecording.iPriority = xbmcRecording.m_iPriority;
  addonRecording.iLifetime = xbmcRecording.m_iLifetime;
  addonRecording.iGenreType = xbmcRecording.GenreType();
  addonRecording.iGenreSubType = xbmcRecording.GenreSubType();
  addonRecording.iPlayCount = xbmcRecording.GetLocalPlayCount();
  addonRecording.iLastPlayedPosition = lrint(xbmcRecording.GetLocalResumePoint().timeInSeconds);
  addonRecording.bIsDeleted = xbmcRecording.IsDeleted();
  addonRecording.iChannelUid = xbmcRecording.ChannelUid();
  addonRecording.channelType = xbmcRecording.IsRadio() ? PVR_RECORDING_CHANNEL_TYPE_RADIO : PVR_RECORDING_CHANNEL_TYPE_TV;
}

/*!
 * @brief Copy over timer info from xbmcTimer to addonTimer.
 * @param xbmcTimer The timer on XBMC's side.
 * @param addonTimer The timer on the addon's side.
 */
void CPVRClient::WriteClientTimerInfo(const CPVRTimerInfoTag &xbmcTimer, PVR_TIMER &addonTimer)
{
  time_t start, end, firstDay;
  xbmcTimer.StartAsUTC().GetAsTime(start);
  xbmcTimer.EndAsUTC().GetAsTime(end);
  xbmcTimer.FirstDayAsUTC().GetAsTime(firstDay);
  CPVREpgInfoTagPtr epgTag = xbmcTimer.GetEpgInfoTag();

  int iPVRTimeCorrection = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;

  addonTimer = {0};
  addonTimer.iClientIndex              = xbmcTimer.m_iClientIndex;
  addonTimer.iParentClientIndex        = xbmcTimer.m_iParentClientIndex;
  addonTimer.state                     = xbmcTimer.m_state;
  addonTimer.iTimerType                = xbmcTimer.GetTimerType() ? xbmcTimer.GetTimerType()->GetTypeId() : PVR_TIMER_TYPE_NONE;
  addonTimer.iClientChannelUid         = xbmcTimer.m_iClientChannelUid;
  strncpy(addonTimer.strTitle, xbmcTimer.m_strTitle.c_str(), sizeof(addonTimer.strTitle) - 1);
  strncpy(addonTimer.strEpgSearchString, xbmcTimer.m_strEpgSearchString.c_str(), sizeof(addonTimer.strEpgSearchString) - 1);
  addonTimer.bFullTextEpgSearch        = xbmcTimer.m_bFullTextEpgSearch;
  strncpy(addonTimer.strDirectory, xbmcTimer.m_strDirectory.c_str(), sizeof(addonTimer.strDirectory) - 1);
  addonTimer.iPriority                 = xbmcTimer.m_iPriority;
  addonTimer.iLifetime                 = xbmcTimer.m_iLifetime;
  addonTimer.iMaxRecordings            = xbmcTimer.m_iMaxRecordings;
  addonTimer.iPreventDuplicateEpisodes = xbmcTimer.m_iPreventDupEpisodes;
  addonTimer.iRecordingGroup           = xbmcTimer.m_iRecordingGroup;
  addonTimer.iWeekdays                 = xbmcTimer.m_iWeekdays;
  addonTimer.startTime                 = start - iPVRTimeCorrection;
  addonTimer.endTime                   = end - iPVRTimeCorrection;
  addonTimer.bStartAnyTime             = xbmcTimer.m_bStartAnyTime;
  addonTimer.bEndAnyTime               = xbmcTimer.m_bEndAnyTime;
  addonTimer.firstDay                  = firstDay - iPVRTimeCorrection;
  addonTimer.iEpgUid                   = epgTag ? epgTag->UniqueBroadcastID() : PVR_TIMER_NO_EPG_UID;
  strncpy(addonTimer.strSummary, xbmcTimer.m_strSummary.c_str(), sizeof(addonTimer.strSummary) - 1);
  addonTimer.iMarginStart              = xbmcTimer.m_iMarginStart;
  addonTimer.iMarginEnd                = xbmcTimer.m_iMarginEnd;
  addonTimer.iGenreType                = epgTag ? epgTag->GenreType() : 0;
  addonTimer.iGenreSubType             = epgTag ? epgTag->GenreSubType() : 0;
  strncpy(addonTimer.strSeriesLink, xbmcTimer.SeriesLink().c_str(), sizeof(addonTimer.strSeriesLink) - 1);
}

/*!
 * @brief Copy over channel info from xbmcChannel to addonClient.
 * @param xbmcChannel The channel on XBMC's side.
 * @param addonChannel The channel on the addon's side.
 */
void CPVRClient::WriteClientChannelInfo(const CPVRChannelPtr &xbmcChannel, PVR_CHANNEL &addonChannel)
{
  addonChannel = {0};
  addonChannel.iUniqueId         = xbmcChannel->UniqueID();
  addonChannel.iChannelNumber    = xbmcChannel->ClientChannelNumber().GetChannelNumber();
  addonChannel.iSubChannelNumber = xbmcChannel->ClientChannelNumber().GetSubChannelNumber();
  strncpy(addonChannel.strChannelName, xbmcChannel->ClientChannelName().c_str(), sizeof(addonChannel.strChannelName) - 1);
  strncpy(addonChannel.strIconPath, xbmcChannel->IconPath().c_str(), sizeof(addonChannel.strIconPath) - 1);
  addonChannel.iEncryptionSystem = xbmcChannel->EncryptionSystem();
  addonChannel.bIsRadio          = xbmcChannel->IsRadio();
  addonChannel.bIsHidden         = xbmcChannel->IsHidden();
  strncpy(addonChannel.strInputFormat, xbmcChannel->InputFormat().c_str(), sizeof(addonChannel.strInputFormat) - 1);
}

bool CPVRClient::GetAddonProperties(void)
{
  std::string strBackendName, strConnectionString, strFriendlyName, strBackendVersion, strBackendHostname;
  PVR_ADDON_CAPABILITIES addonCapabilities = {0};
  CPVRTimerTypes timerTypes;

  /* get the capabilities */
  PVR_ERROR retVal = DoAddonCall(__FUNCTION__, [&addonCapabilities](const AddonInstance* addon) {
    return addon->GetAddonCapabilities(&addonCapabilities);
  }, true, false);

  if (retVal != PVR_ERROR_NO_ERROR)
    return false;

  /* get the name of the backend */
  DoAddonCall(__FUNCTION__, [&strBackendName](const AddonInstance* addon) {
    strBackendName = addon->GetBackendName();
    return PVR_ERROR_NO_ERROR;
   }, true, false);

  /* get the connection string */
  DoAddonCall(__FUNCTION__, [&strConnectionString](const AddonInstance* addon) {
    strConnectionString = addon->GetConnectionString();
    return PVR_ERROR_NO_ERROR;
  }, true, false);

  /* display name = backend name:connection string */
  strFriendlyName = StringUtils::Format("%s:%s", strBackendName.c_str(), strConnectionString.c_str());

  /* backend version number */
  DoAddonCall(__FUNCTION__, [&strBackendVersion](const AddonInstance* addon) {
    strBackendVersion = addon->GetBackendVersion();
    return PVR_ERROR_NO_ERROR;
  }, true, false);

  /* backend hostname */
  DoAddonCall(__FUNCTION__, [&strBackendHostname](const AddonInstance* addon) {
    strBackendHostname = addon->GetBackendHostname();
    return PVR_ERROR_NO_ERROR;
  }, true, false);

  /* timer types */
  retVal = DoAddonCall(__FUNCTION__, [this, strFriendlyName, &addonCapabilities, &timerTypes](const AddonInstance* addon) {
    std::unique_ptr<PVR_TIMER_TYPE[]> types_array(new PVR_TIMER_TYPE[PVR_ADDON_TIMERTYPE_ARRAY_SIZE]);
    int size = PVR_ADDON_TIMERTYPE_ARRAY_SIZE;

    PVR_ERROR retval = addon->GetTimerTypes(types_array.get(), &size);

    if (retval == PVR_ERROR_NOT_IMPLEMENTED)
    {
      // begin compat section
      CLog::LogF(LOGWARNING, "Add-on %s does not support timer types. It will work, but not benefit from the timer features introduced with PVR Addon API 2.0.0", strFriendlyName.c_str());

      // Create standard timer types (mostly) matching the timer functionality available in Isengard.
      // This is for migration only and does not make changes to the addons obsolete. Addons should
      // work and benefit from some UI changes (e.g. some of the timer settings dialog enhancements),
      // but all old problems/bugs due to static attributes and values will remain the same as in
      // Isengard. Also, new features (like epg search) are not available to addons automatically.
      // This code can be removed once all addons actually support the respective PVR Addon API version.

      size = 0;
      // manual one time
      memset(&types_array[size], 0, sizeof(types_array[size]));
      types_array[size].iId         = size + 1;
      types_array[size].iAttributes = PVR_TIMER_TYPE_IS_MANUAL               |
                                      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE |
                                      PVR_TIMER_TYPE_SUPPORTS_CHANNELS       |
                                      PVR_TIMER_TYPE_SUPPORTS_START_TIME     |
                                      PVR_TIMER_TYPE_SUPPORTS_END_TIME       |
                                      PVR_TIMER_TYPE_SUPPORTS_PRIORITY       |
                                      PVR_TIMER_TYPE_SUPPORTS_LIFETIME       |
                                      PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS;
      ++size;

      // manual timer rule
      memset(&types_array[size], 0, sizeof(types_array[size]));
      types_array[size].iId         = size + 1;
      types_array[size].iAttributes = PVR_TIMER_TYPE_IS_MANUAL               |
                                      PVR_TIMER_TYPE_IS_REPEATING            |
                                      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE |
                                      PVR_TIMER_TYPE_SUPPORTS_CHANNELS       |
                                      PVR_TIMER_TYPE_SUPPORTS_START_TIME     |
                                      PVR_TIMER_TYPE_SUPPORTS_END_TIME       |
                                      PVR_TIMER_TYPE_SUPPORTS_PRIORITY       |
                                      PVR_TIMER_TYPE_SUPPORTS_LIFETIME       |
                                      PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY      |
                                      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS       |
                                      PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS;
      ++size;

      if (addonCapabilities.bSupportsEPG)
      {
        // One-shot epg-based
        memset(&types_array[size], 0, sizeof(types_array[size]));
        types_array[size].iId         = size + 1;
        types_array[size].iAttributes = PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE    |
                                        PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE |
                                        PVR_TIMER_TYPE_SUPPORTS_CHANNELS          |
                                        PVR_TIMER_TYPE_SUPPORTS_START_TIME        |
                                        PVR_TIMER_TYPE_SUPPORTS_END_TIME          |
                                        PVR_TIMER_TYPE_SUPPORTS_PRIORITY          |
                                        PVR_TIMER_TYPE_SUPPORTS_LIFETIME          |
                                        PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS;
        ++size;
      }

      retval = PVR_ERROR_NO_ERROR;
      // end compat section
    }

    if (retval == PVR_ERROR_NO_ERROR)
    {
      timerTypes.reserve(size);
      for (int i = 0; i < size; ++i)
      {
        if (types_array[i].iId == PVR_TIMER_TYPE_NONE)
        {
          CLog::LogF(LOGERROR, "Invalid timer type supplied by add-on '%s'. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
          continue;
        }

        if (strlen(types_array[i].strDescription) == 0)
        {
          int id;
          if (types_array[i].iAttributes & PVR_TIMER_TYPE_IS_REPEATING)
          {
            id = (types_array[i].iAttributes & PVR_TIMER_TYPE_IS_MANUAL)
                ? 822  // "Timer rule"
                : 823; // "Timer rule (guide-based)"
          }
          else
          {
            id = (types_array[i].iAttributes & PVR_TIMER_TYPE_IS_MANUAL)
                ? 820  // "One time"
                : 821; // "One time (guide-based)
          }
          std::string descr(g_localizeStrings.Get(id));
          strncpy(types_array[i].strDescription, descr.c_str(), descr.size());
        }
        timerTypes.emplace_back(CPVRTimerTypePtr(new CPVRTimerType(types_array[i], m_iClientId)));
      }
    }
    return retval;
  }, addonCapabilities.bSupportsTimers, false);

  if (retVal == PVR_ERROR_NOT_IMPLEMENTED)
    retVal = PVR_ERROR_NO_ERROR; // timer support is optional.

  /* update the members */
  CSingleLock lock(m_critSection);
  m_strBackendName      = strBackendName;
  m_strConnectionString = strConnectionString;
  m_strFriendlyName     = strFriendlyName;
  m_strBackendVersion   = strBackendVersion;
  m_clientCapabilities  = addonCapabilities;
  m_strBackendHostname  = strBackendHostname;
  m_timertypes          = timerTypes;

  return retVal == PVR_ERROR_NO_ERROR;
}

const std::string& CPVRClient::GetBackendName(void) const
{
  return m_strBackendName;
}

const std::string& CPVRClient::GetBackendVersion(void) const
{
  return m_strBackendVersion;
}

const std::string& CPVRClient::GetBackendHostname(void) const
{
  return m_strBackendHostname;
}

const std::string& CPVRClient::GetConnectionString(void) const
{
  return m_strConnectionString;
}

const std::string& CPVRClient::GetFriendlyName(void) const
{
  return m_strFriendlyName;
}

PVR_ERROR CPVRClient::GetDriveSpace(long long &iTotal, long long &iUsed)
{
  /* default to 0 in case of error */
  iTotal = 0;
  iUsed  = 0;

  return DoAddonCall(__FUNCTION__, [this, &iTotal, &iUsed](const AddonInstance* addon) {
    long long iTotalSpace = 0;
    long long iUsedSpace = 0;
    PVR_ERROR error = m_struct.toAddon.GetDriveSpace(&iTotalSpace, &iUsedSpace);
    if (error == PVR_ERROR_NO_ERROR)
    {
      iTotal = iTotalSpace;
      iUsed = iUsedSpace;
    }
    return error;
  });
}

PVR_ERROR CPVRClient::StartChannelScan(void)
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    return addon->OpenDialogChannelScan();
  }, m_clientCapabilities.SupportsChannelScan());
}

PVR_ERROR CPVRClient::OpenDialogChannelAdd(const CPVRChannelPtr &channel)
{
  return DoAddonCall(__FUNCTION__, [channel](const AddonInstance* addon) {
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);
    return addon->OpenDialogChannelAdd(addonChannel);
  }, m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::OpenDialogChannelSettings(const CPVRChannelPtr &channel)
{
  return DoAddonCall(__FUNCTION__, [channel] (const AddonInstance* addon){
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);
    return addon->OpenDialogChannelSettings(addonChannel);
  }, m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::DeleteChannel(const CPVRChannelPtr &channel)
{
  return DoAddonCall(__FUNCTION__, [channel](const AddonInstance* addon) {
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);
    return addon->DeleteChannel(addonChannel);
  }, m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::RenameChannel(const CPVRChannelPtr &channel)
{
  return DoAddonCall(__FUNCTION__, [channel](const AddonInstance* addon) {
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);
    return addon->RenameChannel(addonChannel);
  }, m_clientCapabilities.SupportsChannelSettings());
}

PVR_ERROR CPVRClient::GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start /* = 0 */, time_t end /* = 0 */, bool bSaveInDb /* = false*/)
{
  return DoAddonCall(__FUNCTION__, [this, channel, epg, start, end, bSaveInDb](const AddonInstance* addon) {
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);

    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress  = this;
    handle.dataAddress    = epg;
    handle.dataIdentifier = bSaveInDb ? 1 : 0; // used by the callback method CPVRClient::cb_transfer_epg_entry()

    int iPVRTimeCorrection = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;

    return addon->GetEPGForChannel(&handle,
                                   addonChannel,
                                   start ? start - iPVRTimeCorrection : 0,
                                   end ? end - iPVRTimeCorrection : 0);
  }, m_clientCapabilities.SupportsEPG());
}

PVR_ERROR CPVRClient::SetEPGTimeFrame(int iDays)
{
  return DoAddonCall(__FUNCTION__, [iDays](const AddonInstance* addon) {
    return addon->SetEPGTimeFrame(iDays);
  }, m_clientCapabilities.SupportsEPG());
}

// This class wraps an EPG_TAG (PVR Addon API struct) to ensure that the string members of
// that struct, which are const char pointers, stay valid until the EPG_TAG gets destructed.
// Please note that this struct is also used to transfer huge amount of EPG_TAGs from
// addon to Kodi. Thus, changing the struct to contain char arrays is not recommened,
// because this would lead to huge amount of string copies when transferring epg data
// from addon to Kodi.
class CAddonEpgTag : public EPG_TAG
{
public:
  CAddonEpgTag() = delete;
  explicit CAddonEpgTag(const CConstPVREpgInfoTagPtr kodiTag) :
    m_strTitle(kodiTag->Title(true)),
    m_strPlotOutline(kodiTag->PlotOutline(true)),
    m_strPlot(kodiTag->Plot(true)),
    m_strOriginalTitle(kodiTag->OriginalTitle(true)),
    m_strCast(kodiTag->DeTokenize(kodiTag->Cast())),
    m_strDirector(kodiTag->DeTokenize(kodiTag->Directors())),
    m_strWriter(kodiTag->DeTokenize(kodiTag->Writers())),
    m_strIMDBNumber(kodiTag->IMDBNumber()),
    m_strEpisodeName(kodiTag->EpisodeName(true)),
    m_strIconPath(kodiTag->Icon()),
    m_strSeriesLink(kodiTag->SeriesLink()),
    m_strGenreDescription(kodiTag->GetGenresLabel())
  {
    time_t t;
    kodiTag->StartAsUTC().GetAsTime(t);
    startTime = t;
    kodiTag->EndAsUTC().GetAsTime(t);
    endTime = t;
    kodiTag->FirstAiredAsUTC().GetAsTime(t);
    firstAired = t;
    iUniqueBroadcastId = kodiTag->UniqueBroadcastID();
    iUniqueChannelId = kodiTag->UniqueChannelID();
    iParentalRating = kodiTag->ParentalRating();
    iSeriesNumber = kodiTag->SeriesNumber();
    iEpisodeNumber = kodiTag->EpisodeNumber();
    iEpisodePartNumber = kodiTag->EpisodePart();
    iStarRating = kodiTag->StarRating();
    iYear = kodiTag->Year();
    iFlags = kodiTag->Flags();
    iGenreType = kodiTag->GenreType();
    iGenreSubType = kodiTag->GenreSubType();
    bNotify = kodiTag->Notify();
    strTitle = m_strTitle.c_str();
    strPlotOutline = m_strPlotOutline.c_str();
    strPlot = m_strPlot.c_str();
    strOriginalTitle = m_strOriginalTitle.c_str();
    strCast = m_strCast.c_str();
    strDirector = m_strDirector.c_str();
    strWriter = m_strWriter.c_str();
    strIMDBNumber = m_strIMDBNumber.c_str();
    strEpisodeName = m_strEpisodeName.c_str();
    strIconPath = m_strIconPath.c_str();
    strSeriesLink = m_strSeriesLink.c_str();
    strGenreDescription = m_strGenreDescription.c_str();
  }

  virtual ~CAddonEpgTag() = default;

private:
  std::string m_strTitle;
  std::string m_strPlotOutline;
  std::string m_strPlot;
  std::string m_strOriginalTitle;
  std::string m_strCast;
  std::string m_strDirector;
  std::string m_strWriter;
  std::string m_strIMDBNumber;
  std::string m_strEpisodeName;
  std::string m_strIconPath;
  std::string m_strSeriesLink;
  std::string m_strGenreDescription;
};

PVR_ERROR CPVRClient::IsRecordable(const CConstPVREpgInfoTagPtr &tag, bool &bIsRecordable) const
{
  return DoAddonCall(__FUNCTION__, [tag, &bIsRecordable](const AddonInstance* addon) {
    CAddonEpgTag addonTag(tag);
    return addon->IsEPGTagRecordable(&addonTag, &bIsRecordable);
  }, m_clientCapabilities.SupportsRecordings() && m_clientCapabilities.SupportsEPG());
}

PVR_ERROR CPVRClient::IsPlayable(const CConstPVREpgInfoTagPtr &tag, bool &bIsPlayable) const
{
  return DoAddonCall(__FUNCTION__, [tag, &bIsPlayable](const AddonInstance* addon) {
    CAddonEpgTag addonTag(tag);
    return addon->IsEPGTagPlayable(&addonTag, &bIsPlayable);
  }, m_clientCapabilities.SupportsEPG());
}

void CPVRClient::WriteFileItemProperties(const PVR_NAMED_VALUE *properties, unsigned int iPropertyCount, CFileItem &fileItem)
{
  for (unsigned int i = 0; i < iPropertyCount; ++i)
  {
    if (strncmp(properties[i].strName, PVR_STREAM_PROPERTY_STREAMURL, strlen(PVR_STREAM_PROPERTY_STREAMURL)) == 0)
    {
        fileItem.SetDynPath(properties[i].strValue);
    }
    else if (strncmp(properties[i].strName, PVR_STREAM_PROPERTY_MIMETYPE, strlen(PVR_STREAM_PROPERTY_MIMETYPE)) == 0)
    {
      fileItem.SetMimeType(properties[i].strValue);
      fileItem.SetContentLookup(false);
    }

    fileItem.SetProperty(properties[i].strName, properties[i].strValue);
  }
}

PVR_ERROR CPVRClient::FillEpgTagStreamFileItem(CFileItem &fileItem)
{
  return DoAddonCall(__FUNCTION__, [&fileItem](const AddonInstance* addon) {
    CAddonEpgTag addonTag(fileItem.GetEPGInfoTag());

    unsigned int iPropertyCount = PVR_STREAM_MAX_PROPERTIES;
    std::unique_ptr<PVR_NAMED_VALUE[]> properties(new PVR_NAMED_VALUE[iPropertyCount]);
    memset(properties.get(), 0, iPropertyCount * sizeof(PVR_NAMED_VALUE));

    PVR_ERROR error = addon->GetEPGTagStreamProperties(&addonTag, properties.get(), &iPropertyCount);
    if (error ==  PVR_ERROR_NO_ERROR)
      WriteFileItemProperties(properties.get(), iPropertyCount, fileItem);

    return error;
  });
}

PVR_ERROR CPVRClient::GetEpgTagEdl(const CConstPVREpgInfoTagPtr &epgTag, std::vector<PVR_EDL_ENTRY> &edls)
{
  edls.clear();
  return DoAddonCall(__FUNCTION__, [&epgTag, &edls](const AddonInstance* addon) {
    CAddonEpgTag addonTag(epgTag);

    PVR_EDL_ENTRY edl_array[PVR_ADDON_EDL_LENGTH];
    int size = PVR_ADDON_EDL_LENGTH;
    PVR_ERROR error = addon->GetEPGTagEdl(&addonTag, edl_array, &size);
    if (error == PVR_ERROR_NO_ERROR)
    {
      edls.reserve(size);
      for (int i = 0; i < size; ++i)
        edls.emplace_back(edl_array[i]);
    }
    return error;
  }, m_clientCapabilities.SupportsEpgTagEdl());
}

PVR_ERROR CPVRClient::GetChannelGroupsAmount(int &iGroups)
{
  iGroups = -1;
  return DoAddonCall(__FUNCTION__, [&iGroups](const AddonInstance* addon) {
    iGroups = addon->GetChannelGroupsAmount();
    return (iGroups == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.SupportsChannelGroups());
}

PVR_ERROR CPVRClient::GetChannelGroups(CPVRChannelGroups *groups)
{
  return DoAddonCall(__FUNCTION__, [this, groups](const AddonInstance* addon) {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = groups;
    return addon->GetChannelGroups(&handle, groups->IsRadio());
  }, m_clientCapabilities.SupportsChannelGroups());
}

PVR_ERROR CPVRClient::GetChannelGroupMembers(CPVRChannelGroup *group)
{
  return DoAddonCall(__FUNCTION__, [this, group](const AddonInstance* addon) {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = group;

    PVR_CHANNEL_GROUP tag;
    WriteClientGroupInfo(*group, tag);
    return addon->GetChannelGroupMembers(&handle, tag);
  }, m_clientCapabilities.SupportsChannelGroups());
}

PVR_ERROR CPVRClient::GetChannelsAmount(int &iChannels)
{
  iChannels = -1;
  return DoAddonCall(__FUNCTION__, [&iChannels](const AddonInstance* addon) {
    iChannels = addon->GetChannelsAmount();
    return (iChannels == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::GetChannels(CPVRChannelGroup &channels, bool radio)
{
  return DoAddonCall(__FUNCTION__, [this, &channels, radio](const AddonInstance* addon) {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = &channels;
    return addon->GetChannels(&handle, radio);
  }, (radio && m_clientCapabilities.SupportsRadio()) || (!radio && m_clientCapabilities.SupportsTV()));
}

PVR_ERROR CPVRClient::GetRecordingsAmount(bool deleted, int &iRecordings)
{
  iRecordings = -1;
  return DoAddonCall(__FUNCTION__, [deleted, &iRecordings](const AddonInstance* addon) {
    iRecordings = addon->GetRecordingsAmount(deleted);
    return (iRecordings == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.SupportsRecordings() && (!deleted || m_clientCapabilities.SupportsRecordingsUndelete()));
}

PVR_ERROR CPVRClient::GetRecordings(CPVRRecordings *results, bool deleted) {
  return DoAddonCall(__FUNCTION__, [this, results, deleted](const AddonInstance* addon)
  {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = results;
    return addon->GetRecordings(&handle, deleted);
  }, m_clientCapabilities.SupportsRecordings() && (!deleted || m_clientCapabilities.SupportsRecordingsUndelete()));
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecording &recording)
{
  return DoAddonCall(__FUNCTION__, [&recording](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    return addon->DeleteRecording(tag);
  }, m_clientCapabilities.SupportsRecordings());
}

PVR_ERROR CPVRClient::UndeleteRecording(const CPVRRecording &recording)
{
  return DoAddonCall(__FUNCTION__, [&recording](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    return addon->UndeleteRecording(tag);
  }, m_clientCapabilities.SupportsRecordingsUndelete());
}

PVR_ERROR CPVRClient::DeleteAllRecordingsFromTrash()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    return addon->DeleteAllRecordingsFromTrash();
  }, m_clientCapabilities.SupportsRecordingsUndelete());
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording &recording)
{
  return DoAddonCall(__FUNCTION__, [&recording](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    return addon->RenameRecording(tag);
  }, m_clientCapabilities.SupportsRecordings());
}

PVR_ERROR CPVRClient::SetRecordingLifetime(const CPVRRecording &recording)
{
  return DoAddonCall(__FUNCTION__, [&recording](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    return addon->SetRecordingLifetime(&tag);
  }, m_clientCapabilities.SupportsRecordingsLifetimeChange());
}

PVR_ERROR CPVRClient::SetRecordingPlayCount(const CPVRRecording &recording, int count)
{
  return DoAddonCall(__FUNCTION__, [&recording, count](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    return addon->SetRecordingPlayCount(tag, count);
  }, m_clientCapabilities.SupportsRecordingsPlayCount());
}

PVR_ERROR CPVRClient::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition)
{
  return DoAddonCall(__FUNCTION__, [&recording, lastplayedposition](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    return addon->SetRecordingLastPlayedPosition(tag, lastplayedposition);
  }, m_clientCapabilities.SupportsRecordingsLastPlayedPosition());
 }

PVR_ERROR CPVRClient::GetRecordingLastPlayedPosition(const CPVRRecording &recording, int &iPosition)
{
  iPosition = -1;
  return DoAddonCall(__FUNCTION__, [&recording, &iPosition](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);
    iPosition = addon->GetRecordingLastPlayedPosition(tag);
    return (iPosition == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.SupportsRecordingsLastPlayedPosition());
}

PVR_ERROR CPVRClient::GetRecordingEdl(const CPVRRecording &recording, std::vector<PVR_EDL_ENTRY> &edls)
{
  edls.clear();
  return DoAddonCall(__FUNCTION__, [&recording, &edls](const AddonInstance* addon) {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    PVR_EDL_ENTRY edl_array[PVR_ADDON_EDL_LENGTH];
    int size = PVR_ADDON_EDL_LENGTH;
    PVR_ERROR error = addon->GetRecordingEdl(tag, edl_array, &size);
    if (error == PVR_ERROR_NO_ERROR)
    {
      edls.reserve(size);
      for (int i = 0; i < size; ++i)
        edls.emplace_back(edl_array[i]);
    }
    return error;
  }, m_clientCapabilities.SupportsRecordingsEdl());
}

PVR_ERROR CPVRClient::GetTimersAmount(int &iTimers)
{
  iTimers = -1;
  return DoAddonCall(__FUNCTION__, [&iTimers](const AddonInstance* addon) {
    iTimers = addon->GetTimersAmount();
    return (iTimers == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::GetTimers(CPVRTimersContainer *results)
{
  return DoAddonCall(__FUNCTION__, [this, results](const AddonInstance* addon) {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = results;
    return addon->GetTimers(&handle);
  }, m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag &timer)
{
  return DoAddonCall(__FUNCTION__, [&timer](const AddonInstance* addon) {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);
    return addon->AddTimer(tag);
  }, m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce /* = false */)
{
  return DoAddonCall(__FUNCTION__, [&timer, bForce](const AddonInstance* addon) {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);
    return addon->DeleteTimer(tag, bForce);
  }, m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  return DoAddonCall(__FUNCTION__, [&timer](const AddonInstance* addon) {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);
    return addon->UpdateTimer(tag);
  }, m_clientCapabilities.SupportsTimers());
}

PVR_ERROR CPVRClient::GetTimerTypes(CPVRTimerTypes& results) const
{
  CSingleLock lock(m_critSection);
  results = m_timertypes;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClient::GetStreamReadChunkSize(int& iChunkSize)
{
  return DoAddonCall(__FUNCTION__, [&iChunkSize](const AddonInstance* addon) {
    return addon->GetStreamReadChunkSize(&iChunkSize);
  }, m_clientCapabilities.SupportsRecordings() || m_clientCapabilities.HandlesInputStream());
}

PVR_ERROR CPVRClient::ReadLiveStream(void* lpBuf, int64_t uiBufSize, int &iRead)
{
  iRead = -1;
  return DoAddonCall(__FUNCTION__, [&lpBuf, uiBufSize, &iRead](const AddonInstance* addon) {
    iRead = addon->ReadLiveStream(static_cast<unsigned char *>(lpBuf), static_cast<int>(uiBufSize));
    return (iRead == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::ReadRecordedStream(void* lpBuf, int64_t uiBufSize, int &iRead)
{
  iRead = -1;
  return DoAddonCall(__FUNCTION__, [&lpBuf, uiBufSize, &iRead](const AddonInstance* addon) {
    iRead = addon->ReadRecordedStream(static_cast<unsigned char *>(lpBuf), static_cast<int>(uiBufSize));
    return (iRead == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::SeekLiveStream(int64_t iFilePosition, int iWhence, int64_t &iPosition)
{
  iPosition = -1;
  return DoAddonCall(__FUNCTION__, [iFilePosition, iWhence, &iPosition](const AddonInstance* addon) {
    iPosition = addon->SeekLiveStream(iFilePosition, iWhence);
    return (iPosition == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::SeekRecordedStream(int64_t iFilePosition, int iWhence, int64_t &iPosition)
{
  iPosition = -1;
  return DoAddonCall(__FUNCTION__, [iFilePosition, iWhence, &iPosition](const AddonInstance* addon) {
    iPosition = addon->SeekRecordedStream(iFilePosition, iWhence);
    return (iPosition == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::SeekTime(double time, bool backwards, double *startpts)
{
  return DoAddonCall(__FUNCTION__, [time, backwards, &startpts](const AddonInstance* addon) {
    return addon->SeekTime(time, backwards, startpts) ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_IMPLEMENTED;
  });
}

PVR_ERROR CPVRClient::GetLiveStreamLength(int64_t &iLength)
{
  iLength = -1;
  return DoAddonCall(__FUNCTION__, [&iLength](const AddonInstance* addon) {
    iLength = addon->LengthLiveStream();
    return (iLength == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::GetRecordedStreamLength(int64_t &iLength)
{
  iLength = -1;
  return DoAddonCall(__FUNCTION__, [&iLength](const AddonInstance* addon) {
    iLength = addon->LengthRecordedStream();
    return (iLength == -1) ? PVR_ERROR_NOT_IMPLEMENTED : PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::SignalQuality(PVR_SIGNAL_STATUS &qualityinfo)
{
  return DoAddonCall(__FUNCTION__, [&qualityinfo](const AddonInstance* addon) {
    return addon->SignalStatus(qualityinfo);
  });
}

PVR_ERROR CPVRClient::GetDescrambleInfo(PVR_DESCRAMBLE_INFO &descrambleinfo) const
{
  return DoAddonCall(__FUNCTION__, [&descrambleinfo](const AddonInstance* addon) {
    return addon->GetDescrambleInfo(&descrambleinfo);
  }, m_clientCapabilities.SupportsDescrambleInfo());
}

PVR_ERROR CPVRClient::FillChannelStreamFileItem(CFileItem &fileItem)
{
  return DoAddonCall(__FUNCTION__, [this, &fileItem](const AddonInstance* addon) {
    const CPVRChannelPtr channel = fileItem.GetPVRChannelInfoTag();
    if (!CanPlayChannel(channel))
      return PVR_ERROR_NO_ERROR; // no error, but no need to obtain the values from the addon

    PVR_CHANNEL tag = {0};
    WriteClientChannelInfo(channel, tag);

    unsigned int iPropertyCount = PVR_STREAM_MAX_PROPERTIES;
    std::unique_ptr<PVR_NAMED_VALUE[]> properties(new PVR_NAMED_VALUE[iPropertyCount]);
    memset(properties.get(), 0, iPropertyCount * sizeof(PVR_NAMED_VALUE));

    PVR_ERROR error = addon->GetChannelStreamProperties(&tag, properties.get(), &iPropertyCount);
    if (error == PVR_ERROR_NO_ERROR)
      WriteFileItemProperties(properties.get(), iPropertyCount, fileItem);

    return error;
  });
}

PVR_ERROR CPVRClient::FillRecordingStreamFileItem(CFileItem &fileItem)
{
  return DoAddonCall(__FUNCTION__, [this, &fileItem](const AddonInstance* addon) {
    if (!m_clientCapabilities.SupportsRecordings())
      return PVR_ERROR_NO_ERROR; // no error, but no need to obtain the values from the addon

    const CPVRRecordingPtr recording = fileItem.GetPVRRecordingInfoTag();

    PVR_RECORDING tag = {{0}};
    WriteClientRecordingInfo(*recording, tag);

    unsigned int iPropertyCount = PVR_STREAM_MAX_PROPERTIES;
    std::unique_ptr<PVR_NAMED_VALUE[]> properties(new PVR_NAMED_VALUE[iPropertyCount]);
    memset(properties.get(), 0, iPropertyCount * sizeof(PVR_NAMED_VALUE));

    PVR_ERROR error = addon->GetRecordingStreamProperties(&tag, properties.get(), &iPropertyCount);
    if (error == PVR_ERROR_NO_ERROR)
      WriteFileItemProperties(properties.get(), iPropertyCount, fileItem);

    return error;
  });
}

PVR_ERROR CPVRClient::GetStreamProperties(PVR_STREAM_PROPERTIES *props)
{
  return DoAddonCall(__FUNCTION__, [&props](const AddonInstance* addon) {
    return addon->GetStreamProperties(props);
  });
}

PVR_ERROR CPVRClient::DemuxReset()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->DemuxReset();
    return PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.HandlesDemuxing());
}

PVR_ERROR CPVRClient::DemuxAbort()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->DemuxAbort();
    return PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.HandlesDemuxing());
}

PVR_ERROR CPVRClient::DemuxFlush()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->DemuxFlush();
    return PVR_ERROR_NO_ERROR;
  }, m_clientCapabilities.HandlesDemuxing());
}

PVR_ERROR CPVRClient::DemuxRead(DemuxPacket* &packet)
{
  return DoAddonCall(__FUNCTION__, [&packet](const AddonInstance* addon) {
    packet = addon->DemuxRead();
    return packet ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_IMPLEMENTED;
  }, m_clientCapabilities.HandlesDemuxing());
}

const char *CPVRClient::ToString(const PVR_ERROR error)
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

PVR_ERROR CPVRClient::DoAddonCall(const char* strFunctionName, std::function<PVR_ERROR(const AddonInstance*)> function, bool bIsImplemented /* = true */, bool bCheckReadyToUse /* = true */) const
{
  // Check preconditions.
  if (!bIsImplemented)
    return PVR_ERROR_NOT_IMPLEMENTED;

  if (m_bBlockAddonCalls)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_bReadyToUse && bCheckReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  // Call.
  const PVR_ERROR error = function(&m_struct.toAddon);

  // Log error, if any.
  if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
    CLog::LogFunction(LOGERROR, strFunctionName, "Add-on '%s' returned an error: %s", GetFriendlyName().c_str(), ToString(error));

  return error;
}

bool CPVRClient::CanPlayChannel(const CPVRChannelPtr &channel) const
{
  return (m_bReadyToUse &&
           ((m_clientCapabilities.SupportsTV() && !channel->IsRadio()) ||
            (m_clientCapabilities.SupportsRadio() && channel->IsRadio())));
}

PVR_ERROR CPVRClient::OpenLiveStream(const CPVRChannelPtr &channel)
{
  return DoAddonCall(__FUNCTION__, [this, channel](const AddonInstance* addon) {
    CloseLiveStream();

    if (!CanPlayChannel(channel))
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Add-on '%s' can not play channel '%s'", GetFriendlyName().c_str(), channel->ChannelName().c_str());
      return PVR_ERROR_SERVER_ERROR;
    }
    else
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Opening live stream for channel '%s'", channel->ChannelName().c_str());
      PVR_CHANNEL tag;
      WriteClientChannelInfo(channel, tag);
      return addon->OpenLiveStream(tag) ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_IMPLEMENTED;
    }
  });
}

PVR_ERROR CPVRClient::OpenRecordedStream(const CPVRRecordingPtr &recording)
{
  return DoAddonCall(__FUNCTION__, [this, recording](const AddonInstance* addon) {
    CloseRecordedStream();

    PVR_RECORDING tag;
    WriteClientRecordingInfo(*recording, tag);
    CLog::LogFC(LOGDEBUG, LOGPVR, "Opening stream for recording '%s'", recording->m_strTitle.c_str());
    return addon->OpenRecordedStream(tag) ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_IMPLEMENTED;
  }, m_clientCapabilities.SupportsRecordings());
}

PVR_ERROR CPVRClient::CloseLiveStream()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->CloseLiveStream();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::CloseRecordedStream()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->CloseRecordedStream();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::PauseStream(bool bPaused)
{
  return DoAddonCall(__FUNCTION__, [bPaused](const AddonInstance* addon) {
    addon->PauseStream(bPaused);
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::SetSpeed(int speed)
{
  return DoAddonCall(__FUNCTION__, [speed](const AddonInstance* addon) {
    addon->SetSpeed(speed);
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::CanPauseStream(bool &bCanPause) const
{
  bCanPause = false;
  return DoAddonCall(__FUNCTION__, [&bCanPause](const AddonInstance* addon) {
    bCanPause = addon->CanPauseStream();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::CanSeekStream(bool &bCanSeek) const
{
  bCanSeek = false;
  return DoAddonCall(__FUNCTION__, [&bCanSeek](const AddonInstance* addon) {
    bCanSeek = addon->CanSeekStream();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::GetStreamTimes(PVR_STREAM_TIMES *times)
{
  return DoAddonCall(__FUNCTION__, [&times](const AddonInstance* addon) {
    return addon->GetStreamTimes(times);
  });
}

PVR_ERROR CPVRClient::IsRealTimeStream(bool &bRealTime) const
{
  bRealTime = false;
  return DoAddonCall(__FUNCTION__, [&bRealTime](const AddonInstance* addon) {
    bRealTime = addon->IsRealTimeStream();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::OnSystemSleep()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->OnSystemSleep();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::OnSystemWake()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->OnSystemWake();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::OnPowerSavingActivated()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->OnPowerSavingActivated();
    return PVR_ERROR_NO_ERROR;
  });
}

PVR_ERROR CPVRClient::OnPowerSavingDeactivated()
{
  return DoAddonCall(__FUNCTION__, [](const AddonInstance* addon) {
    addon->OnPowerSavingDeactivated();
    return PVR_ERROR_NO_ERROR;
  });
}

std::shared_ptr<CPVRClientMenuHooks> CPVRClient::GetMenuHooks()
{
  if (!m_menuhooks)
    m_menuhooks.reset(new CPVRClientMenuHooks(ID()));

  return m_menuhooks;
}

PVR_ERROR CPVRClient::CallMenuHook(const CPVRClientMenuHook &hook, const CFileItemPtr &item)
{
  return DoAddonCall(__FUNCTION__, [&hook, &item](const AddonInstance* addon) {
    PVR_MENUHOOK_DATA hookData;
    hookData.cat = PVR_MENUHOOK_UNKNOWN;

    if (item)
    {
      if (item->IsEPG())
      {
        hookData.cat = PVR_MENUHOOK_EPG;
        hookData.data.iEpgUid = item->GetEPGInfoTag()->UniqueBroadcastID();
      }
      else if (item->IsPVRChannel())
      {
        hookData.cat = PVR_MENUHOOK_CHANNEL;
        WriteClientChannelInfo(item->GetPVRChannelInfoTag(), hookData.data.channel);
      }
      else if (item->IsUsablePVRRecording())
      {
        hookData.cat = PVR_MENUHOOK_RECORDING;
        WriteClientRecordingInfo(*item->GetPVRRecordingInfoTag(), hookData.data.recording);
      }
      else if (item->IsDeletedPVRRecording())
      {
        hookData.cat = PVR_MENUHOOK_DELETED_RECORDING;
        WriteClientRecordingInfo(*item->GetPVRRecordingInfoTag(), hookData.data.recording);
      }
      else if (item->IsPVRTimer())
      {
        hookData.cat = PVR_MENUHOOK_TIMER;
        WriteClientTimerInfo(*item->GetPVRTimerInfoTag(), hookData.data.timer);
      }
      else
      {
        CLog::LogF(LOGERROR, "Unhandled item type.");
        return PVR_ERROR_INVALID_PARAMETERS;
      }
    }
    else
    {
      hookData.cat = PVR_MENUHOOK_SETTING;
    }

    PVR_MENUHOOK menuHook = {0};
    menuHook.category = hookData.cat;
    menuHook.iHookId = hook.GetId();
    menuHook.iLocalizedStringId = hook.GetLabelId();

    return addon->MenuHook(menuHook, hookData);
  });
}

void CPVRClient::SetPriority(int iPriority)
{
  CSingleLock lock(m_critSection);
  if (m_iPriority != iPriority)
  {
    m_iPriority = iPriority;
    if (m_iClientId > PVR_INVALID_CLIENT_ID)
    {
      CServiceBroker::GetPVRManager().GetTVDatabase()->Persist(*this);
      m_bPriorityFetched = true;
    }
  }
}

int CPVRClient::GetPriority() const
{
  CSingleLock lock(m_critSection);
  if (!m_bPriorityFetched && m_iClientId > PVR_INVALID_CLIENT_ID)
  {
    m_iPriority = CServiceBroker::GetPVRManager().GetTVDatabase()->GetPriority(*this);
    m_bPriorityFetched = true;
  }
  return m_iPriority;
}

void CPVRClient::cb_transfer_channel_group(void *kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRChannelGroups *kodiGroups = static_cast<CPVRChannelGroups *>(handle->dataAddress);
  if (!group || !kodiGroups)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  if (strlen(group->strGroupName) == 0)
  {
    CLog::LogF(LOGERROR, "Empty group name");
    return;
  }

  /* transfer this entry to the groups container */
  CPVRChannelGroup transferGroup(*group);
  kodiGroups->UpdateFromClient(transferGroup);
}

void CPVRClient::cb_transfer_channel_group_member(void *kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRChannelGroup *group = static_cast<CPVRChannelGroup *>(handle->dataAddress);
  if (!member || !client || !group)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRChannelPtr channel  = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(member->iChannelUniqueId, client->GetID());
  if (!channel)
  {
    CLog::LogF(LOGERROR, "Cannot find group '%s' or channel '%d'", member->strGroupName, member->iChannelUniqueId);
  }
  else if (group->IsRadio() == channel->IsRadio())
  {
    /* transfer this entry to the group */
    group->AddToGroup(channel, CPVRChannelNumber(member->iChannelNumber, member->iSubChannelNumber), true);
  }
}

void CPVRClient::cb_transfer_epg_entry(void *kodiInstance, const ADDON_HANDLE handle, const EPG_TAG *epgentry)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVREpg *kodiEpg = static_cast<CPVREpg *>(handle->dataAddress);
  if (!epgentry || !client || !kodiEpg)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  /* transfer this entry to the epg */
  kodiEpg->UpdateEntry(epgentry, client->GetID(), handle->dataIdentifier == 1 /* update db */);
}

void CPVRClient::cb_transfer_channel_entry(void *kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL *channel)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRChannelGroupInternal *kodiChannels = static_cast<CPVRChannelGroupInternal *>(handle->dataAddress);
  if (!channel || !client || !kodiChannels)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  /* transfer this entry to the internal channels group */
  CPVRChannelPtr transferChannel(new CPVRChannel(*channel, client->GetID()));
  kodiChannels->UpdateFromClient(transferChannel, CPVRChannelNumber());
}

void CPVRClient::cb_transfer_recording_entry(void *kodiInstance, const ADDON_HANDLE handle, const PVR_RECORDING *recording)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRRecordings *kodiRecordings = static_cast<CPVRRecordings *>(handle->dataAddress);
  if (!recording || !client || !kodiRecordings)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  /* transfer this entry to the recordings container */
  CPVRRecordingPtr transferRecording(new CPVRRecording(*recording, client->GetID()));
  kodiRecordings->UpdateFromClient(transferRecording);
}

void CPVRClient::cb_transfer_timer_entry(void *kodiInstance, const ADDON_HANDLE handle, const PVR_TIMER *timer)
{
  if (!handle)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRTimersContainer *kodiTimers = static_cast<CPVRTimersContainer *>(handle->dataAddress);
  if (!timer || !client || !kodiTimers)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  /* Note: channel can be NULL here, for instance for epg-based timer rules ("record on any channel" condition). */
  CPVRChannelPtr channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(timer->iClientChannelUid, client->GetID());

  /* transfer this entry to the timers container */
  CPVRTimerInfoTagPtr transferTimer(new CPVRTimerInfoTag(*timer, channel, client->GetID()));
  kodiTimers->UpdateFromClient(transferTimer);
}

void CPVRClient::cb_add_menu_hook(void *kodiInstance, PVR_MENUHOOK *hook)
{
  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  if (!hook || !client)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  client->GetMenuHooks()->AddHook(*hook);
}

void CPVRClient::cb_recording(void *kodiInstance, const char *strName, const char *strFileName, bool bOnOff)
{
  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  if (!client || !strFileName)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  std::string strLine1 = StringUtils::Format(g_localizeStrings.Get(bOnOff ? 19197 : 19198).c_str(), client->Name().c_str());
  std::string strLine2;
  if (strName)
    strLine2 = strName;
  else if (strFileName)
    strLine2 = strFileName;

  /* display a notification for 5 seconds */
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strLine1, strLine2, 5000, false);
  CServiceBroker::GetEventLog().Add(EventPtr(new CNotificationEvent(client->Name(), strLine1, client->Icon(), strLine2)));

  CLog::LogFC(LOGDEBUG, LOGPVR, "Recording %s on client '%s'. name='%s' filename='%s'",
              bOnOff ? "started" : "finished", client->Name().c_str(), strName, strFileName);
}

void CPVRClient::cb_trigger_channel_update(void *kodiInstance)
{
  /* update the channels table in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerChannelsUpdate();
}

void CPVRClient::cb_trigger_timer_update(void *kodiInstance)
{
  /* update the timers table in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerTimersUpdate();
}

void CPVRClient::cb_trigger_recording_update(void *kodiInstance)
{
  /* update the recordings table in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
}

void CPVRClient::cb_trigger_channel_groups_update(void *kodiInstance)
{
  /* update all channel groups in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerChannelGroupsUpdate();
}

void CPVRClient::cb_trigger_epg_update(void *kodiInstance, unsigned int iChannelUid)
{
  // get the client
  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  if (!client)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CServiceBroker::GetPVRManager().EpgContainer().UpdateRequest(client->GetID(), iChannelUid);
}

void CPVRClient::cb_free_demux_packet(void *kodiInstance, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CPVRClient::cb_allocate_demux_packet(void *kodiInstance, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

void CPVRClient::cb_connection_state_change(void* kodiInstance, const char* strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage)
{
  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  if (!client || !strConnectionString)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  const PVR_CONNECTION_STATE prevState(client->GetConnectionState());
  if (prevState == newState)
    return;

  CLog::LogFC(LOGDEBUG, LOGPVR, "State for connection '%s' on client '%s' changed from '%d' to '%d'", strConnectionString, client->Name().c_str(), prevState, newState);

  client->SetConnectionState(newState);

  std::string msg;
  if (strMessage != nullptr)
    msg = strMessage;

  CServiceBroker::GetPVRManager().ConnectionStateChange(client, std::string(strConnectionString), newState, msg);
}

void CPVRClient::cb_epg_event_state_change(void* kodiInstance, EPG_TAG* tag, EPG_EVENT_STATE newState)
{
  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  if (!client || !tag)
  {
    CLog::LogF(LOGERROR, "Invalid handler data");
    return;
  }

  CServiceBroker::GetPVRManager().EpgContainer().UpdateFromClient(std::make_shared<CPVREpgInfoTag>(*tag, client->GetID()), newState);
}

class CCodecIds
{
public:
  virtual ~CCodecIds(void) = default;

  static CCodecIds& GetInstance()
  {
    static CCodecIds _instance;
    return _instance;
  }

  xbmc_codec_t GetCodecByName(const char* strCodecName)
  {
    xbmc_codec_t retVal = XBMC_INVALID_CODEC;
    if (strlen(strCodecName) == 0)
      return retVal;

    std::string strUpperCodecName = strCodecName;
    StringUtils::ToUpper(strUpperCodecName);

    std::map<std::string, xbmc_codec_t>::const_iterator it = m_lookup.find(strUpperCodecName);
    if (it != m_lookup.end())
      retVal = it->second;

    return retVal;
  }

private:
  CCodecIds(void)
  {
    // get ids and names
    const AVCodec* codec = nullptr;
    void* i = nullptr;
    xbmc_codec_t tmp;
    while ((codec = av_codec_iterate(&i)))
    {
      if (av_codec_is_decoder(codec))
      {
        tmp.codec_type = (xbmc_codec_type_t)codec->type;
        tmp.codec_id   = codec->id;

        std::string strUpperCodecName = codec->name;
        StringUtils::ToUpper(strUpperCodecName);

        m_lookup.insert(std::make_pair(strUpperCodecName, tmp));
      }
    }

    // teletext is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = XBMC_CODEC_TYPE_SUBTITLE;
    tmp.codec_id   = AV_CODEC_ID_DVB_TELETEXT;
    m_lookup.insert(std::make_pair("TELETEXT", tmp));

    // rds is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = XBMC_CODEC_TYPE_RDS;
    tmp.codec_id   = AV_CODEC_ID_NONE;
    m_lookup.insert(std::make_pair("RDS", tmp));
  }

  std::map<std::string, xbmc_codec_t> m_lookup;
};

xbmc_codec_t CPVRClient::cb_get_codec_by_name(const void* kodiInstance, const char* strCodecName)
{
  return CCodecIds::GetInstance().GetCodecByName(strCodecName);
}

CPVRClientCapabilities::CPVRClientCapabilities(const CPVRClientCapabilities& other)
{
  if (other.m_addonCapabilities)
    m_addonCapabilities.reset(new PVR_ADDON_CAPABILITIES(*other.m_addonCapabilities));
  InitRecordingsLifetimeValues();
}

const CPVRClientCapabilities& CPVRClientCapabilities::operator =(const CPVRClientCapabilities& other)
{
  if (other.m_addonCapabilities)
    m_addonCapabilities.reset(new PVR_ADDON_CAPABILITIES(*other.m_addonCapabilities));
  InitRecordingsLifetimeValues();
  return *this;
}

const CPVRClientCapabilities& CPVRClientCapabilities::operator =(const PVR_ADDON_CAPABILITIES& addonCapabilities)
{
  m_addonCapabilities.reset(new PVR_ADDON_CAPABILITIES(addonCapabilities));
  InitRecordingsLifetimeValues();
  return *this;
}

void CPVRClientCapabilities::clear()
{
  m_recordingsLifetimeValues.clear();
  m_addonCapabilities.reset();
}

void CPVRClientCapabilities::InitRecordingsLifetimeValues()
{
  m_recordingsLifetimeValues.clear();
  if (m_addonCapabilities && m_addonCapabilities->iRecordingsLifetimesSize > 0)
  {
    for (unsigned int i = 0; i < m_addonCapabilities->iRecordingsLifetimesSize; ++i)
    {
      int iValue = m_addonCapabilities->recordingsLifetimeValues[i].iValue;
      std::string strDescr(m_addonCapabilities->recordingsLifetimeValues[i].strDescription);
      if (strDescr.empty())
      {
        // No description given by addon. Create one from value.
        strDescr = StringUtils::Format("%d", iValue);
      }
      m_recordingsLifetimeValues.push_back(std::make_pair(strDescr, iValue));
    }
  }
  else if (SupportsRecordingsLifetimeChange())
  {
    // No values given by addon, but lifetime supported. Use default values 1..365
    for (int i = 1; i < 366; ++i)
    {
      m_recordingsLifetimeValues.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(17999).c_str(), i), i)); // "%s days"
    }
  }
  else
  {
    // No lifetime supported.
  }
}

void CPVRClientCapabilities::GetRecordingsLifetimeValues(std::vector<std::pair<std::string, int>> &list) const
{
  for (const auto &lifetime : m_recordingsLifetimeValues)
    list.push_back(lifetime);
}

}
