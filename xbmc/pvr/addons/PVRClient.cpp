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

#include <vector>
#include "Application.h"
#include "FileItem.h"
#include "PVRClient.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "epg/Epg.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace std;
using namespace ADDON;
using namespace PVR;
using namespace EPG;

CPVRClient::CPVRClient(const AddonProps& props) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>(props)
{
  ResetProperties();
}

CPVRClient::CPVRClient(const cp_extension_t *ext) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>(ext)
{
  ResetProperties();
}

CPVRClient::~CPVRClient(void)
{
  if (m_pInfo)
    SAFE_DELETE(m_pInfo);
}

void CPVRClient::ResetProperties(void)
{
  m_bReadyToUse           = false;
  m_bGotBackendName       = false;
  m_bGotBackendVersion    = false;
  m_bGotConnectionString  = false;
  m_bGotFriendlyName      = false;
  m_bGotAddonCapabilities = false;
  m_strBackendVersion     = "unknown";
  m_strConnectionString   = "unknown";
  m_strFriendlyName       = "unknown";
  m_strHostName           = "unknown";
  m_strBackendName        = "unknown";
  ResetAddonCapabilities();
}

void CPVRClient::ResetAddonCapabilities(void)
{
  m_addonCapabilities.bSupportsChannelSettings = false;
  m_addonCapabilities.bSupportsTimeshift       = false;
  m_addonCapabilities.bSupportsEPG             = false;
  m_addonCapabilities.bSupportsTV              = false;
  m_addonCapabilities.bSupportsRadio           = false;
  m_addonCapabilities.bSupportsRecordings      = false;
  m_addonCapabilities.bSupportsTimers          = false;
  m_addonCapabilities.bSupportsChannelGroups   = false;
  m_addonCapabilities.bSupportsChannelScan     = false;
  m_addonCapabilities.bHandlesInputStream      = false;
  m_addonCapabilities.bHandlesDemuxing         = false;
}

void CPVRClient::Create(int iClientId)
{
  CLog::Log(LOGDEBUG, "PVR - %s - creating PVR add-on instance '%s'", __FUNCTION__, Name().c_str());

  /* initialise members */
  if (!m_pInfo)
    m_pInfo              = new PVR_PROPERTIES;
  m_pInfo->iClientId     = iClientId;
  CStdString userpath    = _P(Profile());
  m_pInfo->strUserPath   = userpath.c_str();
  CStdString clientpath  = _P(Path());
  m_pInfo->strClientPath = clientpath.c_str();

  /* initialise the add-on */
  if (CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::Create())
  {
    SetAddonCapabilities();
    m_strHostName = m_pStruct->GetConnectionString();
    m_bReadyToUse = true;
  }
  /* don't log failed inits here because it will spam the log file as this is called in a loop */
}

void CPVRClient::Destroy(void)
{
  CLog::Log(LOGDEBUG, "PVR - %s - destroying PVR add-on '%s'", __FUNCTION__, GetFriendlyName().c_str());
  m_bReadyToUse = false;

  try
  {
    /* Tell the client to destroy */
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::Destroy();
    m_menuhooks.clear();
    SAFE_DELETE(m_pInfo);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to destroy addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }
}

void CPVRClient::ReCreate(void)
{
  int clientID = m_pInfo->iClientId;
  Destroy();
  Create(clientID);
}

bool CPVRClient::ReadyToUse(void) const
{
  return m_bReadyToUse;
}

int CPVRClient::GetID(void) const
{
  return m_pInfo->iClientId;
}

/*!
 * @brief Copy over group info from xbmcGroup to addonGroup.
 * @param xbmcGroup The group on XBMC's side.
 * @param addonGroup The group on the addon's side.
 */
inline void PVRWriteClientGroupInfo(const CPVRChannelGroup &xbmcGroup, PVR_CHANNEL_GROUP &addonGroup)
{
  addonGroup.bIsRadio     = xbmcGroup.IsRadio();
  addonGroup.strGroupName = xbmcGroup.GroupName();
}

/*!
 * @brief Copy over recording info from xbmcRecording to addonRecording.
 * @param xbmcRecording The recording on XBMC's side.
 * @param addonRecording The recording on the addon's side.
 */
inline void PVRWriteClientRecordingInfo(const CPVRRecording &xbmcRecording, PVR_RECORDING &addonRecording)
{
  time_t recTime;
  xbmcRecording.RecordingTimeAsUTC().GetAsTime(recTime);

  addonRecording.recordingTime  = recTime - g_advancedSettings.m_iPVRTimeCorrection;
  addonRecording.strRecordingId = xbmcRecording.m_strRecordingId.c_str();
  addonRecording.strTitle       = xbmcRecording.m_strTitle.c_str();
  addonRecording.strPlotOutline = xbmcRecording.m_strPlotOutline.c_str();
  addonRecording.strPlot        = xbmcRecording.m_strPlot.c_str();
  addonRecording.strChannelName = xbmcRecording.m_strChannelName.c_str();
  addonRecording.iDuration      = xbmcRecording.GetDuration();
  addonRecording.iPriority      = xbmcRecording.m_iPriority;
  addonRecording.iLifetime      = xbmcRecording.m_iLifetime;
  addonRecording.strDirectory   = xbmcRecording.m_strDirectory.c_str();
  addonRecording.strStreamURL   = xbmcRecording.m_strStreamURL.c_str();
}

/*!
 * @brief Copy over timer info from xbmcTimer to addonTimer.
 * @param xbmcTimer The timer on XBMC's side.
 * @param addonTimer The timer on the addon's side.
 */
inline void PVRWriteClientTimerInfo(const CPVRTimerInfoTag &xbmcTimer, PVR_TIMER &addonTimer)
{
  time_t start, end, firstDay;
  xbmcTimer.StartAsUTC().GetAsTime(start);
  xbmcTimer.EndAsUTC().GetAsTime(end);
  xbmcTimer.FirstDayAsUTC().GetAsTime(firstDay);
  CEpgInfoTag *epgTag = xbmcTimer.GetEpgInfoTag();

  addonTimer.iClientIndex      = xbmcTimer.m_iClientIndex;
  addonTimer.state             = xbmcTimer.m_state;
  addonTimer.iClientIndex      = xbmcTimer.m_iClientIndex;
  addonTimer.iClientChannelUid = xbmcTimer.m_iClientChannelUid;
  addonTimer.strTitle          = xbmcTimer.m_strTitle;
  addonTimer.strDirectory      = xbmcTimer.m_strDirectory;
  addonTimer.iPriority         = xbmcTimer.m_iPriority;
  addonTimer.iLifetime         = xbmcTimer.m_iLifetime;
  addonTimer.bIsRepeating      = xbmcTimer.m_bIsRepeating;
  addonTimer.iWeekdays         = xbmcTimer.m_iWeekdays;
  addonTimer.startTime         = start - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.endTime           = end - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.firstDay          = firstDay - g_advancedSettings.m_iPVRTimeCorrection;
  addonTimer.iEpgUid           = epgTag ? epgTag->UniqueBroadcastID() : -1;
  addonTimer.strSummary        = xbmcTimer.m_strSummary.c_str();
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
inline void PVRWriteClientChannelInfo(const CPVRChannel &xbmcChannel, PVR_CHANNEL &addonChannel)
{
  addonChannel.iUniqueId         = xbmcChannel.UniqueID();
  addonChannel.iChannelNumber    = xbmcChannel.ClientChannelNumber();
  addonChannel.strChannelName    = xbmcChannel.ClientChannelName().c_str();
  addonChannel.strIconPath       = xbmcChannel.IconPath().c_str();
  addonChannel.iEncryptionSystem = xbmcChannel.EncryptionSystem();
  addonChannel.bIsRadio          = xbmcChannel.IsRadio();
  addonChannel.bIsHidden         = xbmcChannel.IsHidden();
  addonChannel.strInputFormat    = xbmcChannel.InputFormat().c_str();
  addonChannel.strStreamURL      = xbmcChannel.StreamURL().c_str();
}

PVR_ADDON_CAPABILITIES CPVRClient::GetAddonCapabilities(void)
{
  return m_addonCapabilities;
}

CStdString CPVRClient::GetBackendName(void)
{
  /* cached locally */
  SetBackendName();

  CStdString strReturn;
  strReturn = m_strBackendName;
  return strReturn;
}

CStdString CPVRClient::GetBackendVersion(void)
{
  /* cached locally */
  SetBackendVersion();

  CStdString strReturn;
  strReturn = m_strBackendVersion;
  return strReturn;
}

CStdString CPVRClient::GetConnectionString(void)
{
  /* cached locally */
  SetConnectionString();

  CStdString strReturn;
  strReturn = m_strConnectionString;
  return strReturn;
}

CStdString CPVRClient::GetFriendlyName(void)
{
  /* cached locally */
  SetFriendlyName();

  CStdString strReturn;
  strReturn = m_strFriendlyName;
  return strReturn;
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_UNKNOWN;

  try
  {
    return m_pStruct->GetDriveSpace(iTotal, iUsed);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetDriveSpace() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  /* default to 0 on error */
  *iTotal = 0;
  *iUsed  = 0;

  return PVR_ERROR_NOT_IMPLEMENTED;
}

//PVR_ERROR CPVRClient::GetBackendTime(time_t *localTime, int *iGmtOffset)
//{
//  if (!m_bReadyToUse)
//    return PVR_ERROR_UNKNOWN;
//
//  try
//  {
//    return m_pStruct->GetBackendTime(localTime, iGmtOffset);
//  }
//  catch (exception &e)
//  {
//    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetBackendTime() on addon '%s'. please contact the developer of this addon: %s",
//        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
//  }
//
//  /* default to 0 on error */
//  *localTime = 0;
//  *iGmtOffset = 0;
//
//  return PVR_ERROR_NOT_IMPLEMENTED;
//}

PVR_ERROR CPVRClient::StartChannelScan(void)
{
  if (!m_bReadyToUse)
    return PVR_ERROR_UNKNOWN;

  if (!m_addonCapabilities.bSupportsChannelScan)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    return m_pStruct->DialogChannelScan();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call StartChannelScan() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return PVR_ERROR_NOT_IMPLEMENTED;
}

void CPVRClient::CallMenuHook(const PVR_MENUHOOK &hook)
{
  if (!m_bReadyToUse)
    return;

  try
  {
    m_pStruct->MenuHook(hook);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call CallMenuHook() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }
}

PVR_ERROR CPVRClient::GetEPGForChannel(const CPVRChannel &channel, CEpg *epg, time_t start /* = 0 */, time_t end /* = 0 */, bool bSaveInDb /* = false*/)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsEPG)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_CHANNEL addonChannel;
    PVRWriteClientChannelInfo(channel, addonChannel);

    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CEpg*) epg;
    handle.dataIdentifier = bSaveInDb ? 1 : 0; // used by the callback method CAddonCallbacksPVR::PVRTransferEpgEntry()
    retVal = m_pStruct->GetEpg(&handle,
        addonChannel,
        start ? start - g_advancedSettings.m_iPVRTimeCorrection : 0,
        end ? end - g_advancedSettings.m_iPVRTimeCorrection : 0);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetEPGForChannel()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetEPGForChannel() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetChannelGroupsAmount(void)
{
  int iReturn = -1;
  if (!m_bReadyToUse)
    return iReturn;

  if (!m_addonCapabilities.bSupportsChannelGroups)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetChannelGroupsAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelGroupsAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannelGroups(CPVRChannelGroups *groups)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsChannelGroups)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = groups;
    retVal = m_pStruct->GetChannelGroups(&handle, groups->IsRadio());

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetChannelGroups()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelGroups() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::GetChannelGroupMembers(CPVRChannelGroup *group)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsChannelGroups)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = group;

    PVR_CHANNEL_GROUP tag;
    PVRWriteClientGroupInfo(*group, tag);

    CLog::Log(LOGDEBUG, "PVRClient - %s - get group members for group '%s' from add-on '%s'",
        __FUNCTION__, tag.strGroupName, GetFriendlyName().c_str());
    retVal = m_pStruct->GetChannelGroupMembers(&handle, tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetChannelGroupMembers()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelGroupMembers() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetChannelsAmount(void)
{
  int iReturn = -1;
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetChannelsAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelsAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannels(CPVRChannelGroup &channels, bool radio)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if ((!m_addonCapabilities.bSupportsRadio && radio) ||
      (!m_addonCapabilities.bSupportsTV && !radio))
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRChannelGroup*) &channels;
    retVal = m_pStruct->GetChannels(&handle, radio);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetChannels()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannels() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetRecordingsAmount(void)
{
  int iReturn = -1;
  if (!m_bReadyToUse)
    return iReturn;

  if (!m_addonCapabilities.bSupportsRecordings)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetRecordingsAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetRecordingsAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetRecordings(CPVRRecordings *results)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsRecordings)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRRecordings*) results;
    retVal = m_pStruct->GetRecordings(&handle);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetRecordings()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetRecordings() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsRecordings)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_RECORDING tag;
    PVRWriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->DeleteRecording(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from DeleteRecording()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call DeleteRecording() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording &recording)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsRecordings)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_RECORDING tag;
    PVRWriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->RenameRecording(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RenameRecording()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RenameRecording() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetTimersAmount(void)
{
  int iReturn = -1;
  if (!m_bReadyToUse)
    return iReturn;

  if (!m_addonCapabilities.bSupportsTimers)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetTimersAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetTimersAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetTimers(CPVRTimers *results)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRTimers*) results;
    retVal = m_pStruct->GetTimers(&handle);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetTimers()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetTimers() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_TIMER tag;
    PVRWriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->AddTimer(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from AddTimer()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call AddTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce /* = false */)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_TIMER tag;
    PVRWriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->DeleteTimer(tag, bForce);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from DeleteTimer()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call DeleteTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_TIMER tag;
    PVRWriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->UpdateTimer(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RenameTimer()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RenameTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR retVal = PVR_ERROR_UNKNOWN;
  if (!m_bReadyToUse)
    return retVal;

  if (!m_addonCapabilities.bSupportsTimers)
    return PVR_ERROR_NOT_IMPLEMENTED;

  try
  {
    PVR_TIMER tag;
    PVRWriteClientTimerInfo(timer, tag);

    retVal = m_pStruct->UpdateTimer(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from UpdateTimer()",
          __FUNCTION__, GetFriendlyName().c_str(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call UpdateTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return retVal;
}

bool CPVRClient::OpenLiveStream(const CPVRChannel &channel)
{
  bool bReturn = false;
  if (!m_bReadyToUse)
    return bReturn;

  if ((!m_addonCapabilities.bSupportsTV && !channel.IsRadio()) ||
      (!m_addonCapabilities.bSupportsRadio && channel.IsRadio()))
    return bReturn;

  try
  {
    PVR_CHANNEL tag;
    PVRWriteClientChannelInfo(channel, tag);
    bReturn = m_pStruct->OpenLiveStream(tag);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call OpenLiveStream() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return bReturn;
}

void CPVRClient::CloseLiveStream(void)
{
  if (!m_bReadyToUse)
    return;

  try
  {
    m_pStruct->CloseLiveStream();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call CloseLiveStream() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }
}

int CPVRClient::ReadLiveStream(void* lpBuf, int64_t uiBufSize)
{
  return m_pStruct->ReadLiveStream((unsigned char *)lpBuf, (int)uiBufSize);
}

int64_t CPVRClient::SeekLiveStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  return m_pStruct->SeekLiveStream(iFilePosition, iWhence);
}

int64_t CPVRClient::PositionLiveStream(void)
{
  return m_pStruct->PositionLiveStream();
}

int64_t CPVRClient::LengthLiveStream(void)
{
  return m_pStruct->LengthLiveStream();
}

int CPVRClient::GetCurrentClientChannel(void)
{
  return m_pStruct->GetCurrentClientChannel();
}

bool CPVRClient::SwitchChannel(const CPVRChannel &channel)
{
  PVR_CHANNEL tag;
  PVRWriteClientChannelInfo(channel, tag);
  return m_pStruct->SwitchChannel(tag);
}

bool CPVRClient::SignalQuality(PVR_SIGNAL_STATUS &qualityinfo)
{
  bool bReturn = false;
  if (!m_bReadyToUse)
    return bReturn;

  try
  {
    PVR_ERROR error = m_pStruct->SignalStatus(qualityinfo);
    if (error != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from SignalQuality()",
          __FUNCTION__, GetFriendlyName().c_str(), error);
    }
    else
    {
      bReturn = true;
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call SignalQuality() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return bReturn;
}

CStdString CPVRClient::GetLiveStreamURL(const CPVRChannel &channel)
{
  if (!m_bReadyToUse)
    return StringUtils::EmptyString;

  CStdString strReturn;
  try
  {
    PVR_CHANNEL tag;
    PVRWriteClientChannelInfo(channel, tag);
    strReturn = m_pStruct->GetLiveStreamURL(tag);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetLiveStreamURL() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return strReturn;
}

bool CPVRClient::OpenRecordedStream(const CPVRRecording &recording)
{
  if (!m_addonCapabilities.bSupportsRecordings)
    return false;

  PVR_RECORDING tag;
  PVRWriteClientRecordingInfo(recording, tag);
  return m_pStruct->OpenRecordedStream(tag);
}

void CPVRClient::CloseRecordedStream(void)
{
  return m_pStruct->CloseRecordedStream();
}

int CPVRClient::ReadRecordedStream(void* lpBuf, int64_t uiBufSize)
{
  return m_pStruct->ReadRecordedStream((unsigned char *)lpBuf, (int)uiBufSize);
}

int64_t CPVRClient::SeekRecordedStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  return m_pStruct->SeekRecordedStream(iFilePosition, iWhence);
}

int64_t CPVRClient::PositionRecordedStream()
{
  return m_pStruct->PositionRecordedStream();
}

int64_t CPVRClient::LengthRecordedStream(void)
{
  return m_pStruct->LengthRecordedStream();
}

PVR_ERROR CPVRClient::GetStreamProperties(PVR_STREAM_PROPERTIES *props)
{
  try
  {
    return m_pStruct->GetStreamProperties(props);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetStreamProperties() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());

    /* Set all properties in a case of exception to not supported */
  }
  return PVR_ERROR_UNKNOWN;
}

void CPVRClient::DemuxReset(void)
{
  m_pStruct->DemuxReset();
}

void CPVRClient::DemuxAbort(void)
{
  m_pStruct->DemuxAbort();
}

void CPVRClient::DemuxFlush(void)
{
  m_pStruct->DemuxFlush();
}

DemuxPacket* CPVRClient::DemuxRead(void)
{
  return m_pStruct->DemuxRead();
}

ADDON_STATUS CPVRClient::SetSetting(const char *settingName, const void *settingValue)
{
//  try
//  {
//    return m_pDll->SetSetting(settingName, settingValue);
//  }
//  catch (exception &e)
//  {
//    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during SetSetting occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    return ADDON_STATUS_UNKNOWN;
//  }
}

int CPVRClient::GetClientID(void) const
{
  return m_pInfo->iClientId;
}

bool CPVRClient::HaveMenuHooks(void) const
{
  return m_menuhooks.size() > 0;
}

PVR_MENUHOOKS *CPVRClient::GetMenuHooks(void)
{
  return &m_menuhooks;
}

void CPVRClient::SetBackendName(void)
{
  if (m_bGotBackendName || !m_bReadyToUse)
    return;

  m_bGotBackendName = true;

  try
  {
    m_strBackendName = m_pStruct->GetBackendName();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetBackendName() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }
}

void CPVRClient::SetBackendVersion(void)
{
  if (m_bGotBackendVersion || !m_bReadyToUse)
    return;

  m_bGotBackendVersion = true;

  try
  {
    m_strBackendVersion = m_pStruct->GetBackendVersion();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetBackendVersion() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }
}

void CPVRClient::SetConnectionString(void)
{
  if (m_bGotConnectionString || !m_bReadyToUse)
    return;

  m_bGotConnectionString  = true;

  try
  {
    m_strConnectionString = m_pStruct->GetConnectionString();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetConnectionString() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }
}

void CPVRClient::SetFriendlyName(void)
{
   if (m_bGotFriendlyName || !m_bReadyToUse)
     return;

   m_bGotFriendlyName  = true;

  m_strFriendlyName.Format("%s:%s", GetBackendName().c_str(), GetConnectionString().c_str());
}

PVR_ERROR CPVRClient::SetAddonCapabilities(void)
{
  if (m_bGotAddonCapabilities)
    return PVR_ERROR_NO_ERROR;

  ResetAddonCapabilities();

  /* try to get the addon properties */
  try
  {
    PVR_ERROR retVal = m_pStruct->GetAddonCapabilities(&m_addonCapabilities);
    if (retVal == PVR_ERROR_NO_ERROR)
      m_bGotAddonCapabilities = true;

    return retVal;
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetProperties() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName().c_str(), Author().c_str());
  }

  return PVR_ERROR_SERVER_ERROR;
}

