/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRClient.h"

#include <algorithm>
#include <cmath>
#include <memory>

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

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

void CPVRClient::OnDisabled()
{
  CAddon::OnDisabled();
  CServiceBroker::GetPVRManager().Clients()->UpdateAddons();
}

void CPVRClient::OnEnabled()
{
  CAddon::OnEnabled();
  CServiceBroker::GetPVRManager().Clients()->UpdateAddons();
}

void CPVRClient::StopRunningInstance()
{
  const ADDON::AddonPtr addon(GetRunningInstance());
  if (addon)
  {
    // stop the pvr manager and stop and unload the running pvr addon
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

void CPVRClient::OnPostInstall(bool update, bool modal)
{
  CAddon::OnPostInstall(update, modal);
  CServiceBroker::GetPVRManager().Clients()->UpdateAddons();
}

void CPVRClient::OnPreUnInstall()
{
  StopRunningInstance();
  CAddon::OnPreUnInstall();
}

void CPVRClient::OnPostUnInstall()
{
  CAddon::OnPostUnInstall();
  CServiceBroker::GetPVRManager().Clients()->UpdateAddons();
}

ADDON::AddonPtr CPVRClient::GetRunningInstance() const
{
  ADDON::AddonPtr addon;
  CServiceBroker::GetPVRManager().Clients()->GetClient(ID(), addon);
  return addon;
}

void CPVRClient::ResetProperties(int iClientId /* = PVR_INVALID_CLIENT_ID */)
{
  /* initialise members */
  m_strUserPath           = CSpecialProtocol::TranslatePath(Profile());
  m_strClientPath         = CSpecialProtocol::TranslatePath(Path());
  m_bReadyToUse           = false;
  m_connectionState       = PVR_CONNECTION_STATE_UNKNOWN;
  m_prevConnectionState   = PVR_CONNECTION_STATE_UNKNOWN;
  m_ignoreClient          = false;
  m_iClientId             = iClientId;
  m_strBackendVersion     = DEFAULT_INFO_STRING_VALUE;
  m_strConnectionString   = DEFAULT_INFO_STRING_VALUE;
  m_strFriendlyName       = DEFAULT_INFO_STRING_VALUE;
  m_strBackendName        = DEFAULT_INFO_STRING_VALUE;
  m_bIsPlayingTV          = false;
  m_bIsPlayingRecording   = false;
  m_bIsPlayingEpgTag      = false;
  m_strBackendHostname.clear();
  m_menuhooks.clear();
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
  CLog::Log(LOGDEBUG, "PVR - %s - creating PVR add-on instance '%s'", __FUNCTION__, Name().c_str());
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
  CLog::Log(LOGDEBUG, "PVR - %s - destroying PVR add-on '%s'", __FUNCTION__, GetFriendlyName().c_str());

  /* destroy the add-on */
  CAddonDll::Destroy();

  /* reset all properties to defaults */
  ResetProperties();
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
  addonRecording.recordingTime       = recTime - g_advancedSettings.m_iPVRTimeCorrection;
  strncpy(addonRecording.strRecordingId, xbmcRecording.m_strRecordingId.c_str(), sizeof(addonRecording.strRecordingId) - 1);
  strncpy(addonRecording.strTitle, xbmcRecording.m_strTitle.c_str(), sizeof(addonRecording.strTitle) - 1);
  strncpy(addonRecording.strPlotOutline, xbmcRecording.m_strPlotOutline.c_str(), sizeof(addonRecording.strPlotOutline) - 1);
  strncpy(addonRecording.strPlot, xbmcRecording.m_strPlot.c_str(), sizeof(addonRecording.strPlot) - 1);
  strncpy(addonRecording.strChannelName, xbmcRecording.m_strChannelName.c_str(), sizeof(addonRecording.strChannelName) - 1);
  addonRecording.iDuration           = xbmcRecording.GetDuration();
  addonRecording.iPriority           = xbmcRecording.m_iPriority;
  addonRecording.iLifetime           = xbmcRecording.m_iLifetime;
  addonRecording.iPlayCount          = xbmcRecording.GetLocalPlayCount();
  addonRecording.iLastPlayedPosition = lrint(xbmcRecording.GetLocalResumePoint().timeInSeconds);
  addonRecording.bIsDeleted          = xbmcRecording.IsDeleted();
  strncpy(addonRecording.strDirectory, xbmcRecording.m_strDirectory.c_str(), sizeof(addonRecording.strDirectory) - 1);
  strncpy(addonRecording.strIconPath, xbmcRecording.m_strIconPath.c_str(), sizeof(addonRecording.strIconPath) - 1);
  strncpy(addonRecording.strThumbnailPath, xbmcRecording.m_strThumbnailPath.c_str(), sizeof(addonRecording.strThumbnailPath) - 1);
  strncpy(addonRecording.strFanartPath, xbmcRecording.m_strFanartPath.c_str(), sizeof(addonRecording.strFanartPath) - 1);
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
  addonTimer.startTime                 = start - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.endTime                   = end - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.bStartAnyTime             = xbmcTimer.m_bStartAnyTime;
  addonTimer.bEndAnyTime               = xbmcTimer.m_bEndAnyTime;
  addonTimer.firstDay                  = firstDay - g_advancedSettings.m_iPVRTimeCorrection;
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
  addonChannel.iChannelNumber    = xbmcChannel->ClientChannelNumber();
  addonChannel.iSubChannelNumber = xbmcChannel->ClientSubChannelNumber();
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
  PVR_ERROR retVal = m_struct.toAddon.GetAddonCapabilities(&addonCapabilities);
  if (retVal != PVR_ERROR_NO_ERROR)
  {
    CLog::Log(LOGERROR, "PVR - couldn't get the capabilities for add-on '%s'. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
    return false;
  }

  /* get the name of the backend */
  strBackendName = m_struct.toAddon.GetBackendName();

  /* get the connection string */
  strConnectionString = m_struct.toAddon.GetConnectionString();

  /* display name = backend name:connection string */
  strFriendlyName = StringUtils::Format("%s:%s", strBackendName.c_str(), strConnectionString.c_str());

  /* backend version number */
  strBackendVersion = m_struct.toAddon.GetBackendVersion();

  /* backend hostname */
  strBackendHostname = m_struct.toAddon.GetBackendHostname();

  /* timer types */
  if (addonCapabilities.bSupportsTimers)
  {
    std::unique_ptr<PVR_TIMER_TYPE[]> types_array(new PVR_TIMER_TYPE[PVR_ADDON_TIMERTYPE_ARRAY_SIZE]);
    int size = PVR_ADDON_TIMERTYPE_ARRAY_SIZE;

    PVR_ERROR retval = m_struct.toAddon.GetTimerTypes(types_array.get(), &size);

    if (retval == PVR_ERROR_NOT_IMPLEMENTED)
    {
      // begin compat section
      CLog::Log(LOGWARNING, "%s - Addon %s does not support timer types. It will work, but not benefit from the timer features introduced with PVR Addon API 2.0.0", __FUNCTION__, strFriendlyName.c_str());

      // Create standard timer types (mostly) matching the timer functionality available in Isengard.
      // This is for migration only and does not make changes to the addons obsolete. Addons should
      // work and benefit from some UI changes (e.g. some of the timer settings dialog enhancements),
      // but all old problems/bugs due to static attributes and values will remain the same as in
      // Isengard. Also, new features (like epg search) are not available to addons automatically.
      // This code can be removed once all addons actually support the respective PVR Addon API version.

      size = 0;
      // manual one time
      types_array[size] = {0};
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
      types_array[size] = {0};
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
        types_array[size] = {0};
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
          CLog::Log(LOGERROR, "PVR - invalid timer type supplied by add-on '%s'. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
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
        timerTypes.push_back(CPVRTimerTypePtr(new CPVRTimerType(types_array[i], m_iClientId)));
      }
    }
    else
    {
      CLog::Log(LOGERROR, "PVR - couldn't get the timer types for add-on '%s'. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
      return false;
    }
  }

  /* update the members */
  m_strBackendName      = strBackendName;
  m_strConnectionString = strConnectionString;
  m_strFriendlyName     = strFriendlyName;
  m_strBackendVersion   = strBackendVersion;
  m_clientCapabilities  = addonCapabilities;
  m_strBackendHostname  = strBackendHostname;
  m_timertypes          = timerTypes;

  return true;
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

  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  long long iTotalSpace = 0;
  long long iUsedSpace = 0;
  PVR_ERROR error = m_struct.toAddon.GetDriveSpace(&iTotalSpace, &iUsedSpace);
  if (error == PVR_ERROR_NO_ERROR)
  {
    iTotal = iTotalSpace;
    iUsed = iUsedSpace;
  }
  return error;
}

PVR_ERROR CPVRClient::StartChannelScan(void)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelScan())
    return PVR_ERROR_NOT_IMPLEMENTED;

  return m_struct.toAddon.OpenDialogChannelScan();
}

PVR_ERROR CPVRClient::OpenDialogChannelAdd(const CPVRChannelPtr &channel)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelSettings())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_CHANNEL addonChannel;
  WriteClientChannelInfo(channel, addonChannel);

  PVR_ERROR retVal = m_struct.toAddon.OpenDialogChannelAdd(addonChannel);
  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::OpenDialogChannelSettings(const CPVRChannelPtr &channel)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelSettings())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_CHANNEL addonChannel;
  WriteClientChannelInfo(channel, addonChannel);

  PVR_ERROR retVal = m_struct.toAddon.OpenDialogChannelSettings(addonChannel);
  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::DeleteChannel(const CPVRChannelPtr &channel)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelSettings())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_CHANNEL addonChannel;
  WriteClientChannelInfo(channel, addonChannel);

  PVR_ERROR retVal = m_struct.toAddon.DeleteChannel(addonChannel);
  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::RenameChannel(const CPVRChannelPtr &channel)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelSettings())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_CHANNEL addonChannel;
  WriteClientChannelInfo(channel, addonChannel);

  PVR_ERROR retVal = m_struct.toAddon.RenameChannel(addonChannel);
  LogError(retVal, __FUNCTION__);
  return retVal;
}

void CPVRClient::CallMenuHook(const PVR_MENUHOOK &hook, const CFileItemPtr item)
{
  if (!m_bReadyToUse)
    return;

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
  }

  m_struct.toAddon.MenuHook(hook, hookData);
}

PVR_ERROR CPVRClient::GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start /* = 0 */, time_t end /* = 0 */, bool bSaveInDb /* = false*/)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsEPG())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_CHANNEL addonChannel;
  WriteClientChannelInfo(channel, addonChannel);

  ADDON_HANDLE_STRUCT handle;
  handle.callerAddress  = this;
  handle.dataAddress    = epg;
  handle.dataIdentifier = bSaveInDb ? 1 : 0; // used by the callback method CPVRClient::cb_transfer_epg_entry()
  PVR_ERROR retVal = m_struct.toAddon.GetEPGForChannel(&handle,
      addonChannel,
      start ? start - g_advancedSettings.m_iPVRTimeCorrection : 0,
      end ? end - g_advancedSettings.m_iPVRTimeCorrection : 0);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::SetEPGTimeFrame(int iDays)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsEPG())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal = m_struct.toAddon.SetEPGTimeFrame(iDays);
  LogError(retVal, __FUNCTION__);
  return retVal;
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
    m_strCast(kodiTag->Cast()),
    m_strDirector(kodiTag->Director()),
    m_strWriter(kodiTag->Writer()),
    m_strIMDBNumber(kodiTag->IMDBNumber()),
    m_strEpisodeName(kodiTag->EpisodeName()),
    m_strIconPath(kodiTag->Icon()),
    m_strSeriesLink(kodiTag->SeriesLink())
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
};

PVR_ERROR CPVRClient::IsRecordable(const CConstPVREpgInfoTagPtr &tag, bool &bIsRecordable) const
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordings() || !m_clientCapabilities.SupportsEPG())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);

  CAddonEpgTag addonTag(tag);
  retVal = m_struct.toAddon.IsEPGTagRecordable(&addonTag, &bIsRecordable);
  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::IsPlayable(const CConstPVREpgInfoTagPtr &tag, bool &bIsPlayable) const
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsEPG())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);

  CAddonEpgTag addonTag(tag);
  retVal = m_struct.toAddon.IsEPGTagPlayable(&addonTag, &bIsPlayable);
  LogError(retVal, __FUNCTION__);
  return retVal;
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

bool CPVRClient::FillEpgTagStreamFileItem(CFileItem &fileItem)
{
  if (!m_bReadyToUse)
    return false;

  CAddonEpgTag addonTag(fileItem.GetEPGInfoTag());

  PVR_NAMED_VALUE properties[PVR_STREAM_MAX_PROPERTIES] = {{{0}}};
  unsigned int iPropertyCount = PVR_STREAM_MAX_PROPERTIES;

  if (m_struct.toAddon.GetEPGTagStreamProperties(&addonTag, properties, &iPropertyCount) != PVR_ERROR_NO_ERROR)
    return false;

  WriteFileItemProperties(properties, iPropertyCount, fileItem);
  return true;
}

int CPVRClient::GetChannelGroupsAmount(void)
{
  int iReturn(-EINVAL);

  if (!m_bReadyToUse)
    return iReturn;

  if (!m_clientCapabilities.SupportsChannelGroups())
    return iReturn;

  return m_struct.toAddon.GetChannelGroupsAmount();
}

PVR_ERROR CPVRClient::GetChannelGroups(CPVRChannelGroups *groups)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelGroups())
    return PVR_ERROR_NOT_IMPLEMENTED;

  ADDON_HANDLE_STRUCT handle;
  handle.callerAddress = this;
  handle.dataAddress = groups;
  PVR_ERROR retVal = m_struct.toAddon.GetChannelGroups(&handle, groups->IsRadio());

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::GetChannelGroupMembers(CPVRChannelGroup *group)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsChannelGroups())
    return PVR_ERROR_NOT_IMPLEMENTED;

  ADDON_HANDLE_STRUCT handle;
  handle.callerAddress = this;
  handle.dataAddress = group;

  PVR_CHANNEL_GROUP tag;
  WriteClientGroupInfo(*group, tag);

  CLog::Log(LOGDEBUG, "PVR - %s - get group members for group '%s' from add-on '%s'",
      __FUNCTION__, tag.strGroupName, GetFriendlyName().c_str());
  PVR_ERROR retVal = m_struct.toAddon.GetChannelGroupMembers(&handle, tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

int CPVRClient::GetChannelsAmount(void)
{
  return m_struct.toAddon.GetChannelsAmount();
}

PVR_ERROR CPVRClient::GetChannels(CPVRChannelGroup &channels, bool radio)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if ((!m_clientCapabilities.SupportsRadio() && radio) ||
      (!m_clientCapabilities.SupportsTV() && !radio))
    return PVR_ERROR_NOT_IMPLEMENTED;

  ADDON_HANDLE_STRUCT handle;
  handle.callerAddress = this;
  handle.dataAddress = (CPVRChannelGroup*) &channels;
  PVR_ERROR retVal = m_struct.toAddon.GetChannels(&handle, radio);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

int CPVRClient::GetRecordingsAmount(bool deleted)
{
  if (!m_clientCapabilities.SupportsRecordings() || (deleted && !m_clientCapabilities.SupportsRecordingsUndelete()))
    return -EINVAL;

  return m_struct.toAddon.GetRecordingsAmount(deleted);
}

PVR_ERROR CPVRClient::GetRecordings(CPVRRecordings *results, bool deleted)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordings() || (deleted && !m_clientCapabilities.SupportsRecordingsUndelete()))
    return PVR_ERROR_NOT_IMPLEMENTED;

  ADDON_HANDLE_STRUCT handle;
  handle.callerAddress = this;
  handle.dataAddress = results;
  PVR_ERROR retVal = m_struct.toAddon.GetRecordings(&handle, deleted);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecording &recording)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordings())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_ERROR retVal = m_struct.toAddon.DeleteRecording(tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::UndeleteRecording(const CPVRRecording &recording)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordingsUndelete())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_ERROR retVal = m_struct.toAddon.UndeleteRecording(tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::DeleteAllRecordingsFromTrash()
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordingsUndelete())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal = m_struct.toAddon.DeleteAllRecordingsFromTrash();

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording &recording)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordings())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_ERROR retVal = m_struct.toAddon.RenameRecording(tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::SetRecordingLifetime(const CPVRRecording &recording)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordingsLifetimeChange())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_ERROR retVal = m_struct.toAddon.SetRecordingLifetime(&tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::SetRecordingPlayCount(const CPVRRecording &recording, int count)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordingsPlayCount())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_ERROR retVal = m_struct.toAddon.SetRecordingPlayCount(tag, count);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsRecordingsLastPlayedPosition())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_ERROR retVal = m_struct.toAddon.SetRecordingLastPlayedPosition(tag, lastplayedposition);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

int CPVRClient::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int iReturn(-EINVAL);
  if (!m_bReadyToUse)
    return iReturn;

  if (!m_clientCapabilities.SupportsRecordingsLastPlayedPosition())
    return iReturn;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  return m_struct.toAddon.GetRecordingLastPlayedPosition(tag);
}

std::vector<PVR_EDL_ENTRY> CPVRClient::GetRecordingEdl(const CPVRRecording &recording)
{
  std::vector<PVR_EDL_ENTRY> edl;
  if (!m_bReadyToUse)
    return edl;

  if (!m_clientCapabilities.SupportsRecordingsEdl())
    return edl;

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);

  PVR_EDL_ENTRY edl_array[PVR_ADDON_EDL_LENGTH];
  int size = PVR_ADDON_EDL_LENGTH;
  PVR_ERROR retval = m_struct.toAddon.GetRecordingEdl(tag, edl_array, &size);
  if (retval == PVR_ERROR_NO_ERROR)
  {
    edl.reserve(size);
    for (int i = 0; i < size; ++i)
    {
      edl.push_back(edl_array[i]);
    }
  }

  return edl;
}

int CPVRClient::GetTimersAmount(void)
{
  if (!m_bReadyToUse)
    return -EINVAL;

  if (!m_clientCapabilities.SupportsTimers())
    return -EINVAL;

  return m_struct.toAddon.GetTimersAmount();
}

PVR_ERROR CPVRClient::GetTimers(CPVRTimersContainer *results)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsTimers())
    return PVR_ERROR_NOT_IMPLEMENTED;

  ADDON_HANDLE_STRUCT handle;
  handle.callerAddress = this;
  handle.dataAddress = results;
  PVR_ERROR retVal = m_struct.toAddon.GetTimers(&handle);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag &timer)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsTimers())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_TIMER tag;
  WriteClientTimerInfo(timer, tag);

  PVR_ERROR retVal = m_struct.toAddon.AddTimer(tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce /* = false */)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsTimers())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_TIMER tag;
  WriteClientTimerInfo(timer, tag);

  PVR_ERROR retVal = m_struct.toAddon.DeleteTimer(tag, bForce);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::RenameTimer(const CPVRTimerInfoTag &timer, const std::string &strNewName)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsTimers())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_TIMER tag;
  WriteClientTimerInfo(timer, tag);

  PVR_ERROR retVal = m_struct.toAddon.UpdateTimer(tag);

  LogError(retVal, __FUNCTION__);

  return retVal;
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_clientCapabilities.SupportsTimers())
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_TIMER tag;
  WriteClientTimerInfo(timer, tag);

  PVR_ERROR retVal = m_struct.toAddon.UpdateTimer(tag);

  LogError(retVal, __FUNCTION__);
  return retVal;
}

PVR_ERROR CPVRClient::GetTimerTypes(CPVRTimerTypes& results) const
{
  if (!m_bReadyToUse)
    return PVR_ERROR_SERVER_ERROR;

  results = m_timertypes;
  return PVR_ERROR_NO_ERROR;
}

int CPVRClient::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  if (IsPlayingLiveStream())
  {
    return m_struct.toAddon.ReadLiveStream((unsigned char *)lpBuf, (int)uiBufSize);
  }
  else if (IsPlayingRecording())
  {
    return m_struct.toAddon.ReadRecordedStream((unsigned char *)lpBuf, (int)uiBufSize);
  }
  return -EINVAL;
}

int64_t CPVRClient::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  if (IsPlayingLiveStream())
  {
    return m_struct.toAddon.SeekLiveStream(iFilePosition, iWhence);
  }
  else if (IsPlayingRecording())
  {
    return m_struct.toAddon.SeekRecordedStream(iFilePosition, iWhence);
  }
  return -EINVAL;
}

bool CPVRClient::SeekTime(double time, bool backwards, double *startpts)
{
  if (IsPlaying())
  {
    return m_struct.toAddon.SeekTime(time, backwards, startpts);
  }
  return false;
}

int64_t CPVRClient::GetStreamPosition(void)
{
  if (IsPlayingLiveStream())
  {
    return m_struct.toAddon.PositionLiveStream();
  }
  else if (IsPlayingRecording())
  {
    return m_struct.toAddon.PositionRecordedStream();
  }
  return -EINVAL;
}

int64_t CPVRClient::GetStreamLength(void)
{
  if (IsPlayingLiveStream())
  {
    return m_struct.toAddon.LengthLiveStream();
  }
  else if (IsPlayingRecording())
  {
    return m_struct.toAddon.LengthRecordedStream();
  }
  return -EINVAL;
}

bool CPVRClient::SignalQuality(PVR_SIGNAL_STATUS &qualityinfo)
{
  if (IsPlayingLiveStream())
  {
    return m_struct.toAddon.SignalStatus(qualityinfo) == PVR_ERROR_NO_ERROR;
  }
  return false;
}

bool CPVRClient::GetDescrambleInfo(PVR_DESCRAMBLE_INFO &descrambleinfo) const
{
  if (m_bReadyToUse && m_clientCapabilities.SupportsDescrambleInfo() && IsPlayingLiveStream())
  {
    return m_struct.toAddon.GetDescrambleInfo(&descrambleinfo) == PVR_ERROR_NO_ERROR;
  }
  return false;
}

bool CPVRClient::FillChannelStreamFileItem(CFileItem &fileItem)
{
  const CPVRChannelPtr channel = fileItem.GetPVRChannelInfoTag();

  if (!m_bReadyToUse)
    return false;

  if (!CanPlayChannel(channel))
    return true; // no error, but no need to obtain the values from the addon

  PVR_CHANNEL tag = {0};
  WriteClientChannelInfo(channel, tag);

  PVR_NAMED_VALUE properties[PVR_STREAM_MAX_PROPERTIES] = {{{0}}};
  unsigned int iPropertyCount = PVR_STREAM_MAX_PROPERTIES;

  if (m_struct.toAddon.GetChannelStreamProperties(&tag, properties, &iPropertyCount) != PVR_ERROR_NO_ERROR)
    return false;

  WriteFileItemProperties(properties, iPropertyCount, fileItem);
  return true;
}

bool CPVRClient::FillRecordingStreamFileItem(CFileItem &fileItem)
{
  if (!m_bReadyToUse)
    return false;

  if (!m_clientCapabilities.SupportsRecordings())
    return true; // no error, but no need to obtain the values from the addon

  const CPVRRecordingPtr recording = fileItem.GetPVRRecordingInfoTag();

  PVR_RECORDING tag = {{0}};
  WriteClientRecordingInfo(*recording, tag);

  PVR_NAMED_VALUE properties[PVR_STREAM_MAX_PROPERTIES] = {{{0}}};
  unsigned int iPropertyCount = PVR_STREAM_MAX_PROPERTIES;

  if (m_struct.toAddon.GetRecordingStreamProperties(&tag, properties, &iPropertyCount) != PVR_ERROR_NO_ERROR)
    return false;

  WriteFileItemProperties(properties, iPropertyCount, fileItem);
  return true;
}

PVR_ERROR CPVRClient::GetStreamProperties(PVR_STREAM_PROPERTIES *props)
{
  if (!IsPlaying())
    return PVR_ERROR_REJECTED;

  return m_struct.toAddon.GetStreamProperties(props);
}

void CPVRClient::DemuxReset(void)
{
  if (m_bReadyToUse && m_clientCapabilities.HandlesDemuxing())
  {
    m_struct.toAddon.DemuxReset();
  }
}

void CPVRClient::DemuxAbort(void)
{
  if (m_bReadyToUse && m_clientCapabilities.HandlesDemuxing())
  {
    m_struct.toAddon.DemuxAbort();
  }
}

void CPVRClient::DemuxFlush(void)
{
  if (m_bReadyToUse && m_clientCapabilities.HandlesDemuxing())
  {
    m_struct.toAddon.DemuxFlush();
  }
}

DemuxPacket* CPVRClient::DemuxRead(void)
{
  if (m_bReadyToUse && m_clientCapabilities.HandlesDemuxing())
  {
    return m_struct.toAddon.DemuxRead();
  }
  return NULL;
}

bool CPVRClient::HasMenuHooks(PVR_MENUHOOK_CAT cat) const
{
  bool bReturn(false);
  if (m_bReadyToUse && !m_menuhooks.empty())
  {
    for (auto hook : m_menuhooks)
    {
      if (hook.category == cat || hook.category == PVR_MENUHOOK_ALL)
      {
        bReturn = true;
        break;
      }
    }
  }
  return bReturn;
}

PVR_MENUHOOKS& CPVRClient::GetMenuHooks(void)
{
  return m_menuhooks;
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

bool CPVRClient::LogError(PVR_ERROR error, const char *strMethod) const
{
  if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
  {
    CLog::Log(LOGERROR, "PVR - %s - addon '%s' returned an error: %s",
        strMethod, GetFriendlyName().c_str(), ToString(error));
    return false;
  }
  return true;
}

bool CPVRClient::CanPlayChannel(const CPVRChannelPtr &channel) const
{
  return (m_bReadyToUse &&
           ((m_clientCapabilities.SupportsTV() && !channel->IsRadio()) ||
            (m_clientCapabilities.SupportsRadio() && channel->IsRadio())));
}

bool CPVRClient::IsPlayingLiveStream(void) const
{
  CSingleLock lock(m_critSection);
  return m_bReadyToUse && m_bIsPlayingTV;
}

bool CPVRClient::IsPlayingLiveTV(void) const
{
  CSingleLock lock(m_critSection);
  return m_bReadyToUse && m_bIsPlayingTV && !m_playingChannel->IsRadio();
}

bool CPVRClient::IsPlayingLiveRadio(void) const
{
  CSingleLock lock(m_critSection);
  return m_bReadyToUse && m_bIsPlayingTV && m_playingChannel->IsRadio();
}

bool CPVRClient::IsPlayingEncryptedChannel(void) const
{
  CSingleLock lock(m_critSection);
  return m_bReadyToUse && m_bIsPlayingTV && m_playingChannel->IsEncrypted();
}

bool CPVRClient::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_bReadyToUse && m_bIsPlayingRecording;
}

bool CPVRClient::IsPlaying(void) const
{
  return IsPlayingLiveStream() ||
         IsPlayingRecording();
}

void CPVRClient::SetPlayingChannel(const CPVRChannelPtr channel)
{
  CSingleLock lock(m_critSection);
  m_playingChannel = channel;
  m_bIsPlayingTV = true;
}

CPVRChannelPtr CPVRClient::GetPlayingChannel() const
{
  CSingleLock lock(m_critSection);
  if (m_bReadyToUse && m_bIsPlayingTV)
    return m_playingChannel;

  return CPVRChannelPtr();
}

void CPVRClient::ClearPlayingChannel()
{
  CSingleLock lock(m_critSection);
  m_playingChannel.reset();
  m_bIsPlayingTV = false;
}

void CPVRClient::SetPlayingRecording(const CPVRRecordingPtr recording)
{
  CSingleLock lock(m_critSection);
  m_playingRecording = recording;
  m_bIsPlayingRecording = true;
}

CPVRRecordingPtr CPVRClient::GetPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  if (m_bReadyToUse && m_bIsPlayingRecording)
    return m_playingRecording;

  return CPVRRecordingPtr();
}

void CPVRClient::ClearPlayingRecording()
{
  CSingleLock lock(m_critSection);
  m_playingRecording.reset();
  m_bIsPlayingRecording = false;
}

void CPVRClient::SetPlayingEpgTag(const CPVREpgInfoTagPtr epgTag)
{
  CSingleLock lock(m_critSection);
  m_playingEpgTag = epgTag;
  m_bIsPlayingEpgTag = true;
}

CPVREpgInfoTagPtr CPVRClient::GetPlayingEpgTag(void) const
{
  CSingleLock lock(m_critSection);
  if (m_bReadyToUse && m_bIsPlayingEpgTag)
    return m_playingEpgTag;

  return CPVREpgInfoTagPtr();
}

void CPVRClient::ClearPlayingEpgTag()
{
  CSingleLock lock(m_critSection);
  m_playingEpgTag.reset();
  m_bIsPlayingEpgTag = false;
}

bool CPVRClient::OpenStream(const CPVRChannelPtr &channel)
{
  bool bReturn(false);
  CloseStream();

  if(!CanPlayChannel(channel))
  {
    CLog::Log(LOGDEBUG, "add-on '%s' can not play channel '%s'", GetFriendlyName().c_str(), channel->ChannelName().c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "opening live stream for channel '%s'", channel->ChannelName().c_str());
    PVR_CHANNEL tag;
    WriteClientChannelInfo(channel, tag);

    bReturn = m_struct.toAddon.OpenLiveStream(tag);
  }

  return bReturn;
}

bool CPVRClient::OpenStream(const CPVRRecordingPtr &recording)
{
  bool bReturn(false);
  CloseStream();

  if (m_bReadyToUse && m_clientCapabilities.SupportsRecordings())
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(*recording, tag);

    bReturn = m_struct.toAddon.OpenRecordedStream(tag);
  }

  return bReturn;
}

void CPVRClient::CloseStream(void)
{
  if (IsPlayingLiveStream())
  {
    m_struct.toAddon.CloseLiveStream();
    ClearPlayingChannel();
  }
  else if (IsPlayingRecording())
  {
    m_struct.toAddon.CloseRecordedStream();
    ClearPlayingRecording();
  }
}

void CPVRClient::PauseStream(bool bPaused)
{
  if (IsPlaying())
  {
    m_struct.toAddon.PauseStream(bPaused);
  }
}

void CPVRClient::SetSpeed(int speed)
{
  if (IsPlaying())
  {
    m_struct.toAddon.SetSpeed(speed);
  }
}

bool CPVRClient::CanPauseStream(void) const
{
  bool bReturn(false);
  if (IsPlaying())
  {
    bReturn = m_struct.toAddon.CanPauseStream();
  }

  return bReturn;
}

bool CPVRClient::CanSeekStream(void) const
{
  bool bReturn(false);
  if (IsPlaying())
  {
    bReturn = m_struct.toAddon.CanSeekStream();
  }
  return bReturn;
}

bool CPVRClient::IsTimeshifting(void) const
{
  bool bReturn(false);
  if (IsPlaying())
  {
    if (m_struct.toAddon.IsTimeshifting)
      bReturn = m_struct.toAddon.IsTimeshifting();
  }
  return bReturn;
}

time_t CPVRClient::GetPlayingTime(void) const
{
  time_t time = 0;
  if (IsPlaying())
  {
    time = m_struct.toAddon.GetPlayingTime();
  }
  // fallback if not implemented by addon
  if (time == 0)
  {
    CDateTime::GetUTCDateTime().GetAsTime(time);
  }
  return time;
}

time_t CPVRClient::GetBufferTimeStart(void) const
{
  time_t time = 0;
  if (IsPlaying())
  {
    time = m_struct.toAddon.GetBufferTimeStart();
  }
  return time;
}

time_t CPVRClient::GetBufferTimeEnd(void) const
{
  time_t time = 0;
  if (IsPlaying())
  {
    time = m_struct.toAddon.GetBufferTimeEnd();
  }
  return time;
}

bool CPVRClient::GetStreamTimes(PVR_STREAM_TIMES *times)
{
  bool ret = false;
  if (IsPlaying())
  {
    if (m_struct.toAddon.GetStreamTimes(times) == PVR_ERROR_NO_ERROR)
      ret = true;
  }
  return ret;
}

bool CPVRClient::IsRealTimeStream(void) const
{
  bool bReturn(false);
  if (IsPlaying())
  {
    bReturn = m_struct.toAddon.IsRealTimeStream();
  }
  return bReturn;
}

void CPVRClient::OnSystemSleep(void)
{
  if (!m_bReadyToUse)
    return;

  m_struct.toAddon.OnSystemSleep();
}

void CPVRClient::OnSystemWake(void)
{
  if (!m_bReadyToUse)
    return;

  m_struct.toAddon.OnSystemWake();
}

void CPVRClient::OnPowerSavingActivated(void)
{
  if (!m_bReadyToUse)
    return;

  m_struct.toAddon.OnPowerSavingActivated();
}

void CPVRClient::OnPowerSavingDeactivated(void)
{
  if (!m_bReadyToUse)
    return;

  m_struct.toAddon.OnPowerSavingDeactivated();
}

void CPVRClient::cb_transfer_channel_group(void *kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRChannelGroups *kodiGroups = static_cast<CPVRChannelGroups *>(handle->dataAddress);
  if (!group || !kodiGroups)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  if (strlen(group->strGroupName) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty group name", __FUNCTION__);
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
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRChannelGroup *group = static_cast<CPVRChannelGroup *>(handle->dataAddress);
  if (!member || !client || !group)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRChannelPtr channel  = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(member->iChannelUniqueId, client->GetID());
  if (!channel)
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot find group '%s' or channel '%d'", __FUNCTION__, member->strGroupName, member->iChannelUniqueId);
  }
  else if (group->IsRadio() == channel->IsRadio())
  {
    /* transfer this entry to the group */
    group->AddToGroup(channel, member->iChannelNumber);
  }
}

void CPVRClient::cb_transfer_epg_entry(void *kodiInstance, const ADDON_HANDLE handle, const EPG_TAG *epgentry)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVREpg *kodiEpg = static_cast<CPVREpg *>(handle->dataAddress);
  if (!epgentry || !client || !kodiEpg)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* transfer this entry to the epg */
  kodiEpg->UpdateEntry(epgentry, client->GetID(), handle->dataIdentifier == 1 /* update db */);
}

void CPVRClient::cb_transfer_channel_entry(void *kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL *channel)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRChannelGroupInternal *kodiChannels = static_cast<CPVRChannelGroupInternal *>(handle->dataAddress);
  if (!channel || !client || !kodiChannels)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* transfer this entry to the internal channels group */
  CPVRChannelPtr transferChannel(new CPVRChannel(*channel, client->GetID()));
  kodiChannels->UpdateFromClient(transferChannel);
}

void CPVRClient::cb_transfer_recording_entry(void *kodiInstance, const ADDON_HANDLE handle, const PVR_RECORDING *recording)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRRecordings *kodiRecordings = static_cast<CPVRRecordings *>(handle->dataAddress);
  if (!recording || !client || !kodiRecordings)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
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
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  CPVRTimersContainer *kodiTimers = static_cast<CPVRTimersContainer *>(handle->dataAddress);
  if (!timer || !client || !kodiTimers)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
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
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  PVR_MENUHOOKS& hooks = client->GetMenuHooks();

  PVR_MENUHOOK hookInt;
  hookInt.iHookId            = hook->iHookId;
  hookInt.iLocalizedStringId = hook->iLocalizedStringId;
  hookInt.category           = hook->category;

  /* add this new hook */
  hooks.emplace_back(hookInt);
}

void CPVRClient::cb_recording(void *kodiInstance, const char *strName, const char *strFileName, bool bOnOff)
{
  CPVRClient *client = static_cast<CPVRClient*>(kodiInstance);
  if (!client || !strFileName)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
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
  CEventLog::GetInstance().Add(EventPtr(new CNotificationEvent(client->Name(), strLine1, client->Icon(), strLine2)));

  CLog::Log(LOGDEBUG, "PVR - %s - recording %s on client '%s'. name='%s' filename='%s'",
      __FUNCTION__, bOnOff ? "started" : "finished", client->Name().c_str(), strName, strFileName);
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
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
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
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  const PVR_CONNECTION_STATE prevState(client->GetConnectionState());
  if (prevState == newState)
    return;

  CLog::Log(LOGDEBUG, "PVR - %s - state for connection '%s' on client '%s' changed from '%d' to '%d'", __FUNCTION__, strConnectionString, client->Name().c_str(), prevState, newState);

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
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
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
    AVCodec* codec = NULL;
    xbmc_codec_t tmp;
    while ((codec = av_codec_next(codec)))
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

CPVRClientCapabilities::CPVRClientCapabilities()
{
  m_addonCapabilities = {0};
}

const CPVRClientCapabilities& CPVRClientCapabilities::operator =(const PVR_ADDON_CAPABILITIES& addonCapabilities)
{
  m_addonCapabilities = addonCapabilities;
  InitRecordingsLifetimeValues();
  return *this;
}

void CPVRClientCapabilities::clear()
{
  m_recordingsLifetimeValues.clear();
  m_addonCapabilities = {0};
}

void CPVRClientCapabilities::InitRecordingsLifetimeValues()
{
  m_recordingsLifetimeValues.clear();
  if (m_addonCapabilities.iRecordingsLifetimesSize > 0)
  {
    for (unsigned int i = 0; i < m_addonCapabilities.iRecordingsLifetimesSize; ++i)
    {
      int iValue = m_addonCapabilities.recordingsLifetimeValues[i].iValue;
      std::string strDescr(m_addonCapabilities.recordingsLifetimeValues[i].strDescription);
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
