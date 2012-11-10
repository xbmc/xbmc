/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>
#include "PVRClient.h"
#include "pvr/PVRManager.h"
#include "epg/Epg.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "settings/GUISettings.h"

using namespace std;
using namespace ADDON;
using namespace PVR;
using namespace EPG;

#define DEFAULT_INFO_STRING_VALUE "unknown"

CPVRClient::CPVRClient(const AddonProps& props) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>(props),
    m_apiVersion("0.0.0")
{
  ResetProperties();
}

CPVRClient::CPVRClient(const cp_extension_t *ext) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>(ext),
    m_apiVersion("0.0.0")
{
  ResetProperties();
}

CPVRClient::~CPVRClient(void)
{
  Destroy();
  SAFE_DELETE(m_pInfo);
}

void CPVRClient::ResetProperties(int iClientId /* = PVR_INVALID_CLIENT_ID */)
{
  /* initialise members */
  SAFE_DELETE(m_pInfo);
  m_pInfo = new PVR_PROPERTIES;
  m_strUserPath           = CSpecialProtocol::TranslatePath(Profile());
  m_pInfo->strUserPath    = m_strUserPath.c_str();
  m_strClientPath         = CSpecialProtocol::TranslatePath(Path());
  m_pInfo->strClientPath  = m_strClientPath.c_str();
  m_menuhooks.clear();
  m_bReadyToUse           = false;
  m_iClientId             = iClientId;
  m_strBackendVersion     = DEFAULT_INFO_STRING_VALUE;
  m_strConnectionString   = DEFAULT_INFO_STRING_VALUE;
  m_strFriendlyName       = DEFAULT_INFO_STRING_VALUE;
  m_strBackendName        = DEFAULT_INFO_STRING_VALUE;
  m_bIsPlayingTV          = false;
  m_bIsPlayingRecording   = false;
  memset(&m_addonCapabilities, 0, sizeof(m_addonCapabilities));
  ResetQualityData(m_qualityInfo);
  m_apiVersion = AddonVersion("0.0.0");
  m_bCanPauseStream       = false;
  m_bCanSeekStream        = false;
}

ADDON_STATUS CPVRClient::Create(int iClientId)
{
  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
  if (iClientId <= PVR_INVALID_CLIENT_ID || iClientId == PVR_VIRTUAL_CLIENT_ID)
    return status;

  /* ensure that a previous instance is destroyed */
  Destroy();

  /* reset all properties to defaults */
  ResetProperties(iClientId);

  /* initialise the add-on */
  bool bReadyToUse(false);
  CLog::Log(LOGDEBUG, "PVR - %s - creating PVR add-on instance '%s'", __FUNCTION__, Name().c_str());
  try
  {
    if ((status = CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::Create()) == ADDON_STATUS_OK)
      bReadyToUse = GetAddonProperties();
  }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  m_bReadyToUse = bReadyToUse;

  return status;
}

bool CPVRClient::DllLoaded(void) const
{
  try { return CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::DllLoaded(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  return false;
}

void CPVRClient::Destroy(void)
{
  if (!m_bReadyToUse)
    return;
  m_bReadyToUse = false;

  /* reset 'ready to use' to false */
  CLog::Log(LOGDEBUG, "PVR - %s - destroying PVR add-on '%s'", __FUNCTION__, GetFriendlyName().c_str());

  /* destroy the add-on */
  try { CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::Destroy(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

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
  memset(&addonGroup, 0, sizeof(addonGroup));

  addonGroup.bIsRadio     = xbmcGroup.IsRadio();
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

  memset(&addonRecording, 0, sizeof(addonRecording));

  addonRecording.recordingTime  = recTime - g_advancedSettings.m_iPVRTimeCorrection;
  strncpy(addonRecording.strRecordingId, xbmcRecording.m_strRecordingId.c_str(), sizeof(addonRecording.strRecordingId) - 1);
  strncpy(addonRecording.strTitle, xbmcRecording.m_strTitle.c_str(), sizeof(addonRecording.strTitle) - 1);
  strncpy(addonRecording.strPlotOutline, xbmcRecording.m_strPlotOutline.c_str(), sizeof(addonRecording.strPlotOutline) - 1);
  strncpy(addonRecording.strPlot, xbmcRecording.m_strPlot.c_str(), sizeof(addonRecording.strPlot) - 1);
  strncpy(addonRecording.strChannelName, xbmcRecording.m_strChannelName.c_str(), sizeof(addonRecording.strChannelName) - 1);
  addonRecording.iDuration      = xbmcRecording.GetDuration();
  addonRecording.iPriority      = xbmcRecording.m_iPriority;
  addonRecording.iLifetime      = xbmcRecording.m_iLifetime;
  strncpy(addonRecording.strDirectory, xbmcRecording.m_strDirectory.c_str(), sizeof(addonRecording.strDirectory) - 1);
  strncpy(addonRecording.strStreamURL, xbmcRecording.m_strStreamURL.c_str(), sizeof(addonRecording.strStreamURL) - 1);
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
  CEpgInfoTagPtr epgTag = xbmcTimer.GetEpgInfoTag();

  memset(&addonTimer, 0, sizeof(addonTimer));

  addonTimer.iClientIndex      = xbmcTimer.m_iClientIndex;
  addonTimer.state             = xbmcTimer.m_state;
  addonTimer.iClientIndex      = xbmcTimer.m_iClientIndex;
  addonTimer.iClientChannelUid = xbmcTimer.m_iClientChannelUid;
  strncpy(addonTimer.strTitle, xbmcTimer.m_strTitle.c_str(), sizeof(addonTimer.strTitle) - 1);
  strncpy(addonTimer.strDirectory, xbmcTimer.m_strDirectory.c_str(), sizeof(addonTimer.strDirectory) - 1);
  addonTimer.iPriority         = xbmcTimer.m_iPriority;
  addonTimer.iLifetime         = xbmcTimer.m_iLifetime;
  addonTimer.bIsRepeating      = xbmcTimer.m_bIsRepeating;
  addonTimer.iWeekdays         = xbmcTimer.m_iWeekdays;
  addonTimer.startTime         = start - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.endTime           = end - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.firstDay          = firstDay - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.iEpgUid           = epgTag ? epgTag->UniqueBroadcastID() : -1;
  strncpy(addonTimer.strSummary, xbmcTimer.m_strSummary.c_str(), sizeof(addonTimer.strSummary) - 1);
  addonTimer.iMarginStart      = xbmcTimer.m_iMarginStart;
  addonTimer.iMarginEnd        = xbmcTimer.m_iMarginEnd;
  addonTimer.iGenreType        = xbmcTimer.m_iGenreType;
  addonTimer.iGenreSubType     = xbmcTimer.m_iGenreSubType;
}

/*!
 * @brief Copy over channel info from xbmcChannel to addonClient.
 * @param xbmcChannel The channel on XBMC's side.
 * @param addonChannel The channel on the addon's side.
 */
void CPVRClient::WriteClientChannelInfo(const CPVRChannel &xbmcChannel, PVR_CHANNEL &addonChannel)
{
  memset(&addonChannel, 0, sizeof(addonChannel));

  addonChannel.iUniqueId         = xbmcChannel.UniqueID();
  addonChannel.iChannelNumber    = xbmcChannel.ClientChannelNumber();
  strncpy(addonChannel.strChannelName, xbmcChannel.ClientChannelName().c_str(), sizeof(addonChannel.strChannelName) - 1);
  strncpy(addonChannel.strIconPath, xbmcChannel.IconPath().c_str(), sizeof(addonChannel.strIconPath) - 1);
  addonChannel.iEncryptionSystem = xbmcChannel.EncryptionSystem();
  addonChannel.bIsRadio          = xbmcChannel.IsRadio();
  addonChannel.bIsHidden         = xbmcChannel.IsHidden();
  strncpy(addonChannel.strInputFormat, xbmcChannel.InputFormat().c_str(), sizeof(addonChannel.strInputFormat) - 1);
  strncpy(addonChannel.strStreamURL, xbmcChannel.StreamURL().c_str(), sizeof(addonChannel.strStreamURL) - 1);
}

bool CPVRClient::IsCompatibleAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version)
{
  AddonVersion myMinVersion = AddonVersion(XBMC_PVR_MIN_API_VERSION);
  AddonVersion myVersion = AddonVersion(XBMC_PVR_API_VERSION);
  return (version >= myMinVersion && minVersion <= myVersion);
}

bool CPVRClient::GetAddonProperties(void)
{
  CStdString strHostName, strBackendName, strConnectionString, strFriendlyName, strBackendVersion;
  PVR_ADDON_CAPABILITIES addonCapabilities;

  /* check the API version */
  AddonVersion minVersion = AddonVersion("0.0.0");
  try { m_apiVersion = AddonVersion(m_pStruct->GetPVRAPIVersion()); }
  catch (exception &e) { LogException(e, "GetPVRAPIVersion()"); return false;  }

  try { minVersion = AddonVersion(m_pStruct->GetMininumPVRAPIVersion()); }
  catch (exception &e) { LogException(e, "GetMininumPVRAPIVersion()"); return false;  }

  if (!IsCompatibleAPIVersion(minVersion, m_apiVersion))
  {
    CLog::Log(LOGERROR, "PVR - Add-on '%s' is using an incompatible API version. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
    return false;
  }

  /* get the capabilities */
  try
  {
    memset(&addonCapabilities, 0, sizeof(addonCapabilities));
    PVR_ERROR retVal = m_pStruct->GetAddonCapabilities(&addonCapabilities);
    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - couldn't get the capabilities for add-on '%s'. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
      return false;
    }
  }
  catch (exception &e) { LogException(e, "GetAddonCapabilities()"); return false; }

  /* get the name of the backend */
  try { strBackendName = m_pStruct->GetBackendName(); }
  catch (exception &e) { LogException(e, "GetBackendName()"); return false;  }

  /* get the connection string */
  try { strConnectionString = m_pStruct->GetConnectionString(); }
  catch (exception &e) { LogException(e, "GetConnectionString()"); return false;  }

  /* display name = backend name:connection string */
  strFriendlyName.Format("%s:%s", strBackendName.c_str(), strConnectionString.c_str());

  /* backend version number */
  try { strBackendVersion = m_pStruct->GetBackendVersion(); }
  catch (exception &e) { LogException(e, "GetBackendVersion()"); return false;  }

  /* update the members */
  m_strBackendName      = strBackendName;
  m_strConnectionString = strConnectionString;
  m_strFriendlyName     = strFriendlyName;
  m_strBackendVersion   = strBackendVersion;
  m_addonCapabilities   = addonCapabilities;

  return true;
}

PVR_ADDON_CAPABILITIES CPVRClient::GetAddonCapabilities(void) const
{
  PVR_ADDON_CAPABILITIES addonCapabilities(m_addonCapabilities);
  return addonCapabilities;
}

CStdString CPVRClient::GetBackendName(void) const
{
  CStdString strReturn(m_strBackendName);
  return strReturn;
}

CStdString CPVRClient::GetBackendVersion(void) const
{
  CStdString strReturn(m_strBackendVersion);
  return strReturn;
}

CStdString CPVRClient::GetConnectionString(void) const
{
  CStdString strReturn(m_strConnectionString);
  return strReturn;
}

CStdString CPVRClient::GetFriendlyName(void) const
{
  CStdString strReturn(m_strFriendlyName);
  return strReturn;
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  try { return m_pStruct->GetDriveSpace(iTotal, iUsed); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  /* default to 0 on error */
  *iTotal = 0;
  *iUsed  = 0;

  return PVR_ERROR_UNKNOWN;
}

PVR_ERROR CPVRClient::StartChannelScan(void)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsChannelScan)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try { return m_pStruct->DialogChannelScan(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  return PVR_ERROR_UNKNOWN;
}

void CPVRClient::CallMenuHook(const PVR_MENUHOOK &hook)
{
  if (!m_bReadyToUse)
    return;

  try { m_pStruct->MenuHook(hook); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
}

PVR_ERROR CPVRClient::GetEPGForChannel(const CPVRChannel &channel, CEpg *epg, time_t start /* = 0 */, time_t end /* = 0 */, bool bSaveInDb /* = false*/)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsEPG)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);

    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress  = this;
    handle.dataAddress    = epg;
    handle.dataIdentifier = bSaveInDb ? 1 : 0; // used by the callback method CAddonCallbacksPVR::PVRTransferEpgEntry()
    retVal = m_pStruct->GetEpg(&handle,
        addonChannel,
        start ? start - g_advancedSettings.m_iPVRTimeCorrection : 0,
        end ? end - g_advancedSettings.m_iPVRTimeCorrection : 0);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

int CPVRClient::GetChannelGroupsAmount(void)
{
  int iReturn(-EINVAL);

  if (!m_bReadyToUse)
    return iReturn;

  if (!m_addonCapabilities.bSupportsChannelGroups)
    return iReturn;

  try { iReturn = m_pStruct->GetChannelGroupsAmount(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannelGroups(CPVRChannelGroups *groups)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsChannelGroups)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = groups;
    retVal = m_pStruct->GetChannelGroups(&handle, groups->IsRadio());

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::GetChannelGroupMembers(CPVRChannelGroup *group)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsChannelGroups)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = group;

    PVR_CHANNEL_GROUP tag;
    WriteClientGroupInfo(*group, tag);

    CLog::Log(LOGDEBUG, "PVR - %s - get group members for group '%s' from add-on '%s'",
        __FUNCTION__, tag.strGroupName, GetFriendlyName().c_str());
    retVal = m_pStruct->GetChannelGroupMembers(&handle, tag);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

int CPVRClient::GetChannelsAmount(void)
{
  int iReturn(-EINVAL);
  if (m_bReadyToUse && (m_addonCapabilities.bSupportsTV || m_addonCapabilities.bSupportsRadio))
  {
    try { iReturn = m_pStruct->GetChannelsAmount(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannels(CPVRChannelGroup &channels, bool radio)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if ((!m_addonCapabilities.bSupportsRadio && radio) ||
      (!m_addonCapabilities.bSupportsTV && !radio))
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);

  try
  {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRChannelGroup*) &channels;
    retVal = m_pStruct->GetChannels(&handle, radio);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

int CPVRClient::GetRecordingsAmount(void)
{
  int iReturn(-EINVAL);

  if (m_addonCapabilities.bSupportsRecordings)
  {
    try { iReturn = m_pStruct->GetRecordingsAmount(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetRecordings(CPVRRecordings *results)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsRecordings)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRRecordings*) results;
    retVal = m_pStruct->GetRecordings(&handle);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecording &recording)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsRecordings)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->DeleteRecording(tag);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording &recording)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsRecordings)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->RenameRecording(tag);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::SetRecordingPlayCount(const CPVRRecording &recording, int count)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsRecordingPlayCount)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->SetRecordingPlayCount(tag, count);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsLastPlayedPosition)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->SetRecordingLastPlayedPosition(tag, lastplayedposition);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

int CPVRClient::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int iReturn(-EINVAL);
  if (!m_bReadyToUse)
    return iReturn;

  if (!m_addonCapabilities.bSupportsLastPlayedPosition)
    return iReturn;

  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    iReturn = m_pStruct->GetRecordingLastPlayedPosition(tag);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return iReturn;
}

int CPVRClient::GetTimersAmount(void)
{
  int iReturn(-EINVAL);
  if (!m_bReadyToUse)
    return iReturn;

  if (!m_addonCapabilities.bSupportsTimers)
    return iReturn;

  try { iReturn = m_pStruct->GetTimersAmount(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  return iReturn;
}

PVR_ERROR CPVRClient::GetTimers(CPVRTimers *results)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    ADDON_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRTimers*) results;
    retVal = m_pStruct->GetTimers(&handle);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag &timer)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->AddTimer(tag);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce /* = false */)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->DeleteTimer(tag, bForce);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->UpdateTimer(tag);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_REJECTED;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  PVR_ERROR retVal(PVR_ERROR_UNKNOWN);
  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->UpdateTimer(tag);

    LogError(retVal, __FUNCTION__);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return retVal;
}

int CPVRClient::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  if (IsPlayingLiveStream())
  {
    try { return m_pStruct->ReadLiveStream((unsigned char *)lpBuf, (int)uiBufSize); }
    catch (exception &e) { LogException(e, "ReadLiveStream()"); }
  }
  else if (IsPlayingRecording())
  {
    try { return m_pStruct->ReadRecordedStream((unsigned char *)lpBuf, (int)uiBufSize); }
    catch (exception &e) { LogException(e, "ReadRecordedStream()"); }
  }
  return -EINVAL;
}

int64_t CPVRClient::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  if (IsPlayingLiveStream())
  {
    try { return m_pStruct->SeekLiveStream(iFilePosition, iWhence); }
    catch (exception &e) { LogException(e, "SeekLiveStream()"); }
  }
  else if (IsPlayingRecording())
  {
    try { return m_pStruct->SeekRecordedStream(iFilePosition, iWhence); }
    catch (exception &e) { LogException(e, "SeekRecordedStream()"); }
  }
  return -EINVAL;
}

bool CPVRClient::SeekTime(int time, bool backwards, double *startpts)
{
  if (IsPlaying())
  {
    try { return m_pStruct->SeekTime(time, backwards, startpts); }
    catch (exception &e) { LogException(e, "SeekTime()"); }
  }
  return false;
}

int64_t CPVRClient::GetStreamPosition(void)
{
  if (IsPlayingLiveStream())
  {
    try { return m_pStruct->PositionLiveStream(); }
    catch (exception &e) { LogException(e, "PositionLiveStream()"); }
  }
  else if (IsPlayingRecording())
  {
    try { return m_pStruct->PositionRecordedStream(); }
    catch (exception &e) { LogException(e, "PositionRecordedStream()"); }
  }
  return -EINVAL;
}

int64_t CPVRClient::GetStreamLength(void)
{
  if (IsPlayingLiveStream())
  {
    try { return m_pStruct->LengthLiveStream(); }
    catch (exception &e) { LogException(e, "LengthLiveStream()"); }
  }
  else if (IsPlayingRecording())
  {
    try { return m_pStruct->LengthRecordedStream(); }
    catch (exception &e) { LogException(e, "LengthRecordedStream()"); }
  }
  return -EINVAL;
}

int CPVRClient::GetCurrentClientChannel(void)
{
  if (IsPlayingLiveStream())
  {
    try { return m_pStruct->GetCurrentClientChannel(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }
  return -EINVAL;
}

bool CPVRClient::SwitchChannel(const CPVRChannel &channel)
{
  bool bSwitched(false);

  if (IsPlayingLiveStream() && CanPlayChannel(channel))
  {
    PVR_CHANNEL tag;
    WriteClientChannelInfo(channel, tag);
    try
    {
      bSwitched = m_pStruct->SwitchChannel(tag);
      if (bSwitched)
      {
        m_bCanPauseStream = m_pStruct->CanPauseStream();
        m_bCanSeekStream = m_pStruct->CanSeekStream();
      }
    }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  if (bSwitched)
  {
    CPVRChannelPtr currentChannel = g_PVRChannelGroups->GetByUniqueID(channel.UniqueID(), channel.ClientID());
    CSingleLock lock(m_critSection);
    ResetQualityData(m_qualityInfo);
    m_playingChannel = currentChannel;
  }

  return bSwitched;
}

bool CPVRClient::SignalQuality(PVR_SIGNAL_STATUS &qualityinfo)
{
  if (IsPlayingLiveStream())
  {
    try
    {
      return m_pStruct->SignalStatus(qualityinfo) == PVR_ERROR_NO_ERROR;
    }
    catch (exception &e)
    {
      LogException(e, __FUNCTION__);
    }
  }
  return false;
}

CStdString CPVRClient::GetLiveStreamURL(const CPVRChannel &channel)
{
  CStdString strReturn;

  if (!m_bReadyToUse || !CanPlayChannel(channel))
    return strReturn;

  try
  {
    PVR_CHANNEL tag;
    WriteClientChannelInfo(channel, tag);
    strReturn = m_pStruct->GetLiveStreamURL(tag);
  }
  catch (exception &e)
  {
    LogException(e, __FUNCTION__);
  }

  return strReturn;
}

PVR_ERROR CPVRClient::GetStreamProperties(PVR_STREAM_PROPERTIES *props)
{
  if (!IsPlaying())
    return PVR_ERROR_REJECTED;

  try { return m_pStruct->GetStreamProperties(props); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  return PVR_ERROR_UNKNOWN;
}

void CPVRClient::DemuxReset(void)
{
  if (m_bReadyToUse && m_addonCapabilities.bHandlesDemuxing)
  {
    try { m_pStruct->DemuxReset(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }
}

void CPVRClient::DemuxAbort(void)
{
  if (m_bReadyToUse && m_addonCapabilities.bHandlesDemuxing)
  {
    try { m_pStruct->DemuxAbort(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }
}

void CPVRClient::DemuxFlush(void)
{
  if (m_bReadyToUse && m_addonCapabilities.bHandlesDemuxing)
  {
    try { m_pStruct->DemuxFlush(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }
}

DemuxPacket* CPVRClient::DemuxRead(void)
{
  if (m_bReadyToUse && m_addonCapabilities.bHandlesDemuxing)
  {
    try { return m_pStruct->DemuxRead(); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }
  return NULL;
}

bool CPVRClient::HaveMenuHooks(PVR_MENUHOOK_CAT cat) const
{
  bool bReturn(false);
  if (m_bReadyToUse && m_menuhooks.size() > 0)
  {
    for (unsigned int i = 0; i < m_menuhooks.size(); i++)
    {
      if (m_menuhooks[i].category == cat || m_menuhooks[i].category == PVR_MENUHOOK_ALL)
      {
        bReturn = true;
        break;
      }
    }
  }
  return bReturn;
}

PVR_MENUHOOKS *CPVRClient::GetMenuHooks(void)
{
  return &m_menuhooks;
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

bool CPVRClient::LogError(const PVR_ERROR error, const char *strMethod) const
{
  if (error != PVR_ERROR_NO_ERROR)
  {
    CLog::Log(LOGERROR, "PVR - %s - addon '%s' returned an error: %s",
        strMethod, GetFriendlyName().c_str(), ToString(error));
    return false;
  }
  return true;
}

void CPVRClient::LogException(const exception &e, const char *strFunctionName) const
{
  CLog::Log(LOGERROR, "PVR - exception '%s' caught while trying to call '%s' on add-on '%s'. Please contact the developer of this add-on: %s", e.what(), strFunctionName, GetFriendlyName().c_str(), Author().c_str());
}

bool CPVRClient::CanPlayChannel(const CPVRChannel &channel) const
{
  return (m_bReadyToUse &&
           ((m_addonCapabilities.bSupportsTV && !channel.IsRadio()) ||
            (m_addonCapabilities.bSupportsRadio && channel.IsRadio())));
}

bool CPVRClient::SupportsChannelGroups(void) const
{
  return m_addonCapabilities.bSupportsChannelGroups;
}

bool CPVRClient::SupportsChannelScan(void) const
{
  return m_addonCapabilities.bSupportsChannelScan;
}

bool CPVRClient::SupportsEPG(void) const
{
  return m_addonCapabilities.bSupportsEPG;
}

bool CPVRClient::SupportsLastPlayedPosition(void) const
{
  return m_addonCapabilities.bSupportsLastPlayedPosition;
}

bool CPVRClient::SupportsRadio(void) const
{
  return m_addonCapabilities.bSupportsRadio;
}

bool CPVRClient::SupportsRecordings(void) const
{
  return m_addonCapabilities.bSupportsRecordings;
}

bool CPVRClient::SupportsRecordingFolders(void) const
{
  return m_addonCapabilities.bSupportsRecordingFolders;
}

bool CPVRClient::SupportsRecordingPlayCount(void) const
{
  return m_addonCapabilities.bSupportsRecordingPlayCount;
}

bool CPVRClient::SupportsTimers(void) const
{
  return m_addonCapabilities.bSupportsTimers;
}

bool CPVRClient::SupportsTV(void) const
{
  return m_addonCapabilities.bSupportsTV;
}

bool CPVRClient::HandlesDemuxing(void) const
{
  return m_addonCapabilities.bHandlesDemuxing;
}

bool CPVRClient::HandlesInputStream(void) const
{
  return m_addonCapabilities.bHandlesInputStream;
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

bool CPVRClient::GetPlayingChannel(CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  if (m_bReadyToUse && m_bIsPlayingTV)
  {
    channel = m_playingChannel;
    return true;
  }
  return false;
}

bool CPVRClient::GetPlayingRecording(CPVRRecording &recording) const
{
  CSingleLock lock(m_critSection);
  if (m_bReadyToUse && m_bIsPlayingRecording)
  {
    recording = m_playingRecording;
    return true;
  }
  return false;
}

bool CPVRClient::OpenStream(const CPVRChannel &channel, bool bIsSwitchingChannel)
{
  bool bReturn(false);
  CloseStream();

  if(!CanPlayChannel(channel))
  {
    CLog::Log(LOGDEBUG, "add-on '%s' can not play channel '%s'", GetFriendlyName().c_str(), channel.ChannelName().c_str());
  }
  else if (!channel.StreamURL().IsEmpty())
  {
    CLog::Log(LOGDEBUG, "opening live stream on url '%s'", channel.StreamURL().c_str());
    bReturn = true;

    // the Njoy N7 sometimes doesn't switch channels, but opens a stream to the previous channel
    // when not waiting for a short period.
    // added in 1.1.0
    AddonVersion checkVersion("1.1.0");
    if (m_apiVersion >= checkVersion)
    {
      unsigned int iWaitTimeMs = m_pStruct->GetChannelSwitchDelay();
      if (iWaitTimeMs > 0)
        XbmcThreads::ThreadSleep(iWaitTimeMs);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "opening live stream for channel '%s'", channel.ChannelName().c_str());
    PVR_CHANNEL tag;
    WriteClientChannelInfo(channel, tag);

    try
    {
      bReturn = m_pStruct->OpenLiveStream(tag);
      if (bReturn)
      {
        m_bCanPauseStream = m_pStruct->CanPauseStream();
        m_bCanSeekStream = m_pStruct->CanSeekStream();
      }
    }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  if (bReturn)
  {
    CPVRChannelPtr currentChannel = g_PVRChannelGroups->GetByUniqueID(channel.UniqueID(), channel.ClientID());
    CSingleLock lock(m_critSection);
    m_playingChannel      = currentChannel;
    m_bIsPlayingTV        = true;
    m_bIsPlayingRecording = false;
  }

  return bReturn;
}

bool CPVRClient::OpenStream(const CPVRRecording &recording)
{
  bool bReturn(false);
  CloseStream();

  if (m_bReadyToUse && m_addonCapabilities.bSupportsRecordings)
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    try
    {
      bReturn = m_pStruct->OpenRecordedStream(tag);
      if (bReturn)
      {
        m_bCanPauseStream = m_pStruct->CanPauseStream();
        m_bCanSeekStream = m_pStruct->CanSeekStream();
      }
    }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  if (bReturn)
  {
    CSingleLock lock(m_critSection);
    m_playingRecording    = recording;
    m_bIsPlayingTV        = false;
    m_bIsPlayingRecording = true;
  }

  return bReturn;
}

void CPVRClient::CloseStream(void)
{
  if (IsPlayingLiveStream())
  {
    try { m_pStruct->CloseLiveStream(); }
    catch (exception &e) { LogException(e, "CloseLiveStream()"); }

    CSingleLock lock(m_critSection);
    m_bIsPlayingTV = false;
  }
  else if (IsPlayingRecording())
  {
    try { m_pStruct->CloseRecordedStream(); }
    catch (exception &e) { LogException(e, "CloseRecordedStream()"); }

    CSingleLock lock(m_critSection);
    m_bIsPlayingRecording = false;
  }

  m_bCanPauseStream = false;
  m_bCanSeekStream = false;
}

void CPVRClient::PauseStream(bool bPaused)
{
  if (IsPlaying())
  {
    try { m_pStruct->PauseStream(bPaused); }
    catch (exception &e) { LogException(e, "PauseStream()"); }
  }
}

void CPVRClient::SetSpeed(int speed)
{
  if (IsPlaying())
  {
    try { m_pStruct->SetSpeed(speed); }
    catch (exception &e) { LogException(e, "SetSpeed()"); }
  }
}

bool CPVRClient::CanPauseStream(void) const
{
  if (IsPlaying())
    return m_bCanPauseStream;

  return false;
}

bool CPVRClient::CanSeekStream(void) const
{
  if (IsPlaying())
    return m_bCanSeekStream;

  return false;
}

void CPVRClient::ResetQualityData(PVR_SIGNAL_STATUS &qualityInfo)
{
  memset(&qualityInfo, 0, sizeof(qualityInfo));
  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
  {
    strncpy(qualityInfo.strAdapterName, g_localizeStrings.Get(13205).c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
    strncpy(qualityInfo.strAdapterStatus, g_localizeStrings.Get(13205).c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
  }
  else
  {
    strncpy(qualityInfo.strAdapterName, g_localizeStrings.Get(13106).c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
    strncpy(qualityInfo.strAdapterStatus, g_localizeStrings.Get(13106).c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
  }
}

void CPVRClient::GetQualityData(PVR_SIGNAL_STATUS *status) const
{
  CSingleLock lock(m_critSection);
  *status = m_qualityInfo;
}

int CPVRClient::GetSignalLevel(void) const
{
  CSingleLock lock(m_critSection);
  return (int) ((float) m_qualityInfo.iSignal / 0xFFFF * 100);
}

int CPVRClient::GetSNR(void) const
{
  CSingleLock lock(m_critSection);
  return (int) ((float) m_qualityInfo.iSNR / 0xFFFF * 100);
}

void CPVRClient::UpdateCharInfoSignalStatus(void)
{
  PVR_SIGNAL_STATUS qualityInfo;
  ResetQualityData(qualityInfo);

  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
    SignalQuality(qualityInfo);

  CSingleLock lock(m_critSection);
  m_qualityInfo = qualityInfo;
}
