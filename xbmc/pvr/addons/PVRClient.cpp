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
#include "pvr/epg/PVREpg.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace std;
using namespace ADDON;

/* TODO factor out the annoying time conversion */

CPVRClient::CPVRClient(const AddonProps& props) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>(props),
    m_bReadyToUse(false),
    m_strHostName("unknown"),
    m_strBackendName("unknown"),
    m_bGotBackendName(false),
    m_strBackendVersion("unknown"),
    m_bGotBackendVersion(false),
    m_strConnectionString("unknown"),
    m_bGotConnectionString(false),
    m_strFriendlyName("unknown"),
    m_bGotFriendlyName(false),
    m_bGotServerProperties(false)
{
}

CPVRClient::CPVRClient(const cp_extension_t *ext) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>(ext),
    m_bReadyToUse(false),
    m_strHostName("unknown"),
    m_strBackendName("unknown"),
    m_bGotBackendName(false),
    m_strBackendVersion("unknown"),
    m_bGotBackendVersion(false),
    m_strConnectionString("unknown"),
    m_bGotConnectionString(false),
    m_strFriendlyName("unknown"),
    m_bGotFriendlyName(false),
    m_bGotServerProperties(false)
{
}

CPVRClient::~CPVRClient(void)
{
}

bool CPVRClient::Create(int iClientId, IPVRClientCallback *pvrCB)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "PVRClient - %s - creating PVR add-on instance '%s'", __FUNCTION__, Name().c_str());

  /* initialise members */
  m_manager             = pvrCB;
  m_pInfo               = new PVR_PROPERTIES;
  m_pInfo->iClienId     = iClientId;
  CStdString userpath   = _P(Profile());
  m_pInfo->strUserPath     = userpath.c_str();
  CStdString clientpath = _P(Path());
  m_pInfo->strClientPath   = clientpath.c_str();

  /* initialise the add-on */
  if (CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::Create())
  {
    m_strHostName = m_pStruct->GetConnectionString();
    m_bReadyToUse = true;
    bReturn = true;
  }
  /* don't log failed inits here because it will spam the log file as this is called in a loop */

  return bReturn;
}

void CPVRClient::Destroy(void)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "PVRClient - %s - destroying PVR add-on '%s'", __FUNCTION__, GetFriendlyName());
  m_bReadyToUse = false;

  try
  {
    /* Tell the client to destroy */
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>::Destroy();
    m_menuhooks.clear();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to destroy addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }
}

bool CPVRClient::ReCreate(void)
{
  long clientID             = m_pInfo->iClienId;
  IPVRClientCallback *pvrCB = m_manager;

  Destroy();
  return Create(clientID, pvrCB);
}

bool CPVRClient::ReadyToUse(void) const
{
  CSingleLock lock(m_critSection);

  return m_bReadyToUse;
}

int CPVRClient::GetID(void) const
{
  CSingleLock lock(m_critSection);

  return m_pInfo->iClienId;
}

PVR_ERROR CPVRClient::GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities)
{
  CSingleLock lock(m_critSection);

  /* cached locally */
  PVR_ERROR retVal = SetProperties();
  *pCapabilities = m_serverProperties;

  return retVal;
}

const char *CPVRClient::GetBackendName(void)
{
  CSingleLock lock(m_critSection);

  /* cached locally */
  SetBackendName();

  return m_strBackendName.c_str();
}

const char *CPVRClient::GetBackendVersion(void)
{
  CSingleLock lock(m_critSection);

  /* cached locally */
  SetBackendVersion();

  return m_strBackendVersion.c_str();
}

const char *CPVRClient::GetConnectionString(void)
{
  CSingleLock lock(m_critSection);

  /* cached locally */
  SetConnectionString();

  return m_strConnectionString.c_str();
}

const char *CPVRClient::GetFriendlyName(void)
{
  CSingleLock lock(m_critSection);

  /* cached locally */
  SetFriendlyName();

  return m_strFriendlyName.c_str();
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return PVR_ERROR_UNKOWN;

  try
  {
    return m_pStruct->GetDriveSpace(iTotal, iUsed);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetDriveSpace() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  /* default to 0 on error */
  *iTotal = 0;
  *iUsed  = 0;

  return PVR_ERROR_NOT_IMPLEMENTED;
}

//PVR_ERROR CPVRClient::GetBackendTime(time_t *localTime, int *iGmtOffset)
//{
//  CSingleLock lock(m_critSection);
//  if (!m_bReadyToUse)
//    return PVR_ERROR_UNKOWN;
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
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return PVR_ERROR_UNKOWN;

  try
  {
    return m_pStruct->DialogChannelScan();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call StartChannelScan() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return PVR_ERROR_NOT_IMPLEMENTED;
}

void CPVRClient::CallMenuHook(const PVR_MENUHOOK &hook)
{
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return;

  try
  {
    m_pStruct->MenuHook(hook);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call CallMenuHook() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }
}

PVR_ERROR CPVRClient::GetEPGForChannel(const CPVRChannel &channel, CPVREpg *epg, time_t start /* = 0 */, time_t end /* = 0 */, bool bSaveInDb /* = false*/)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_CHANNEL addonChannel;
    WriteClientChannelInfo(channel, addonChannel);

    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVREpg*) epg;
    handle.dataIdentifier = bSaveInDb ? 1 : 0; // used by the callback method CAddonCallbacksPVR::PVRTransferEpgEntry()
    retVal = m_pStruct->GetEpg(&handle,
        addonChannel,
        start ? start - g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0,
        end ? end - g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetEPGForChannel()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetEPGForChannel() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetChannelGroupsAmount(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetChannelGroupsAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelGroupsAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannelGroups(CPVRChannelGroups &groups)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRChannelGroups*) &groups;
    retVal = m_pStruct->GetChannelGroups(&handle, groups.IsRadio());

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetChannelGroups()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelGroups() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::GetChannelGroupMembers(CPVRChannelGroup &group)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRChannelGroup*) &group;

    PVR_CHANNEL_GROUP tag;
    WriteClientGroupInfo(group, tag);

    //Workaround for string transfer to PVRclient
    CStdString myName(group.GroupName());
    tag.strGroupName = myName.c_str();

    retVal = m_pStruct->GetChannelGroupMembers(&handle, tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetChannelGroupMembers()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelGroupMembers() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetChannelsAmount(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetChannelsAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannelsAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannels(CPVRChannelGroup &channels, bool radio)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRChannelGroup*) &channels;
    retVal = m_pStruct->GetChannels(&handle, radio);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetChannels()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetChannels() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetRecordingsAmount(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetRecordingsAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetRecordingsAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetRecordings(CPVRRecordings *results)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRRecordings*) results;
    retVal = m_pStruct->GetRecordings(&handle);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetRecordings()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetRecordings() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->DeleteRecording(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from DeleteRecording()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call DeleteRecording() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording &recording)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_RECORDING tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->RenameRecording(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RenameRecording()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RenameRecording() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

void CPVRClient::WriteClientRecordingInfo(const CPVRRecording &xbmcRecording, PVR_RECORDING &addonRecording)
{
  time_t recTime;
  xbmcRecording.RecordingTimeAsUTC().GetAsTime(recTime);

  addonRecording.recordingTime = recTime - g_advancedSettings.m_iUserDefinedEPGTimeCorrection;
  addonRecording.iClientIndex   = xbmcRecording.m_iClientIndex;
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

int CPVRClient::GetTimersAmount(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetTimersAmount();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetTimersAmount() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetTimers(CPVRTimers *results)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_HANDLE_STRUCT handle;
    handle.callerAddress = this;
    handle.dataAddress = (CPVRTimers*) results;
    retVal = m_pStruct->GetTimers(&handle);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from GetTimers()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetTimers() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myDir(timer.m_strDirectory);
    CStdString mySummary(timer.m_strSummary);
    CStdString myTitle(timer.m_strTitle);
    tag.strDirectory     = myDir.c_str();
    tag.strSummary = mySummary.c_str();
    tag.strTitle   = myTitle.c_str();

    retVal = m_pStruct->AddTimer(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from AddTimer()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call AddTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce /* = false */)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myDir(timer.m_strDirectory);
    CStdString mySummary(timer.m_strSummary);
    CStdString myTitle(timer.m_strTitle);
    tag.strDirectory     = myDir.c_str();
    tag.strSummary = mySummary.c_str();
    tag.strTitle   = myTitle.c_str();

    retVal = m_pStruct->DeleteTimer(tag, bForce);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from DeleteTimer()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call DeleteTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myDir(timer.m_strDirectory);
    CStdString mySummary(timer.m_strSummary);
    CStdString myTitle(timer.m_strTitle);
    tag.strDirectory     = myDir.c_str();
    tag.strSummary = mySummary.c_str();
    tag.strTitle   = myTitle.c_str();

    retVal = m_pStruct->UpdateTimer(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RenameTimer()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RenameTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_TIMER tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myTitle(timer.m_strTitle);
    CStdString myDirectory(timer.m_strDirectory);
    CStdString mySummary(timer.m_strSummary);
    tag.strTitle   = myTitle.c_str();
    tag.strDirectory     = myDirectory.c_str();
    tag.strSummary = mySummary.c_str();

    retVal = m_pStruct->UpdateTimer(tag);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from UpdateTimer()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call UpdateTimer() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

void CPVRClient::WriteClientTimerInfo(const CPVRTimerInfoTag &xbmcTimer, PVR_TIMER &addonTimer)
{
  time_t start, end, firstDay;
  xbmcTimer.StartAsUTC().GetAsTime(start);
  xbmcTimer.EndAsUTC().GetAsTime(end);
  xbmcTimer.FirstDayAsUTC().GetAsTime(firstDay);

  addonTimer.iClientIndex      = xbmcTimer.m_iClientIndex;
  addonTimer.bIsActive         = xbmcTimer.m_bIsActive;
  addonTimer.iClientIndex      = xbmcTimer.m_iClientIndex;
  addonTimer.iClientChannelUid = xbmcTimer.m_iClientChannelUid;
  addonTimer.bIsRecording      = xbmcTimer.m_bIsRecording;
  addonTimer.strTitle          = xbmcTimer.m_strTitle;
  addonTimer.strDirectory      = xbmcTimer.m_strDirectory;
  addonTimer.iPriority         = xbmcTimer.m_iPriority;
  addonTimer.iLifetime         = xbmcTimer.m_iLifetime;
  addonTimer.bIsRepeating      = xbmcTimer.m_bIsRepeating;
  addonTimer.iWeekdays         = xbmcTimer.m_iWeekdays;
  addonTimer.startTime         = start - g_advancedSettings.m_iUserDefinedEPGTimeCorrection;
  addonTimer.endTime           = end - g_advancedSettings.m_iUserDefinedEPGTimeCorrection;
  addonTimer.firstDay          = firstDay - g_advancedSettings.m_iUserDefinedEPGTimeCorrection;
  addonTimer.iEpgUid           = xbmcTimer.m_epgInfo ? xbmcTimer.m_epgInfo->UniqueBroadcastID() : -1;
  addonTimer.strSummary        = xbmcTimer.m_strSummary.c_str();
  addonTimer.iMarginStart      = xbmcTimer.m_iMarginStart;
  addonTimer.iMarginEnd        = xbmcTimer.m_iMarginEnd;
}

bool CPVRClient::OpenLiveStream(const CPVRChannel &channel)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return bReturn;

  try
  {
    PVR_CHANNEL tag;
    WriteClientChannelInfo(channel, tag);
    bReturn = m_pStruct->OpenLiveStream(tag);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call OpenLiveStream() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return bReturn;
}

void CPVRClient::CloseLiveStream(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return;

  try
  {
    m_pStruct->CloseLiveStream();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call CloseLiveStream() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
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
  CSingleLock lock(m_critSection);

  return m_pStruct->GetCurrentClientChannel();
}

bool CPVRClient::SwitchChannel(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);

  PVR_CHANNEL tag;
  WriteClientChannelInfo(channel, tag);
  return m_pStruct->SwitchChannel(tag);
}

bool CPVRClient::SignalQuality(PVR_SIGNAL_STATUS &qualityinfo)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return bReturn;

  try
  {
    PVR_ERROR error = m_pStruct->SignalStatus(qualityinfo);
    if (error != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from SignalQuality()",
          __FUNCTION__, GetFriendlyName(), error);
    }
    else
    {
      bReturn = true;
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call SignalQuality() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return bReturn;
}

const char *CPVRClient::GetLiveStreamURL(const CPVRChannel &channel)
{
  static CStdString strReturn = "";
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return strReturn.c_str();

  try
  {
    PVR_CHANNEL tag;
    WriteClientChannelInfo(channel, tag);
    strReturn = m_pStruct->GetLiveStreamURL(tag);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetLiveStreamURL() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return strReturn.c_str();
}

void CPVRClient::WriteClientChannelInfo(const CPVRChannel &xbmcChannel, PVR_CHANNEL &addonChannel)
{
  addonChannel.iUniqueId         = xbmcChannel.UniqueID();
  addonChannel.iChannelNumber    = xbmcChannel.ClientChannelNumber();
  addonChannel.strChannelName    = xbmcChannel.ClientChannelName().c_str();
  addonChannel.strIconPath       = xbmcChannel.IconPath().c_str();
  addonChannel.iEncryptionSystem = xbmcChannel.EncryptionSystem();
  addonChannel.bIsRadio          = xbmcChannel.IsRadio();
  addonChannel.bIsHidden         = xbmcChannel.IsHidden();
  addonChannel.bIsRecording      = xbmcChannel.IsRecording();
  addonChannel.strInputFormat    = xbmcChannel.InputFormat().c_str();
  addonChannel.strStreamURL      = xbmcChannel.StreamURL().c_str();
}

bool CPVRClient::OpenRecordedStream(const CPVRRecording &recording)
{
  CSingleLock lock(m_critSection);

  PVR_RECORDING tag;
  WriteClientRecordingInfo(recording, tag);
  return m_pStruct->OpenRecordedStream(tag);
}

void CPVRClient::CloseRecordedStream(void)
{
  CSingleLock lock(m_critSection);

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
  CSingleLock lock(m_critSection);

  try
  {
    return m_pStruct->GetStreamProperties(props);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetStreamProperties() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());

    /* Set all properties in a case of exception to not supported */
  }
  return PVR_ERROR_UNKOWN;
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
//  CSingleLock lock(m_critSection);
//
//  try
//  {
//    return m_pDll->SetSetting(settingName, settingValue);
//  }
//  catch (exception &e)
//  {
//    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during SetSetting occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    return STATUS_UNKNOWN;
//  }
}

int CPVRClient::GetClientID(void) const
{
  CSingleLock lock(m_critSection);

  return m_pInfo->iClienId;
}

bool CPVRClient::HaveMenuHooks(void) const
{
  CSingleLock lock(m_critSection);

  return m_menuhooks.size() > 0;
}

PVR_MENUHOOKS *CPVRClient::GetMenuHooks(void)
{
  CSingleLock lock(m_critSection);

  return &m_menuhooks;
}

void CPVRClient::SetBackendName(void)
{
  CSingleLock lock(m_critSection);

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
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }
}

void CPVRClient::SetBackendVersion(void)
{
  CSingleLock lock(m_critSection);

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
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }
}

void CPVRClient::SetConnectionString(void)
{
  CSingleLock lock(m_critSection);

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
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }
}

void CPVRClient::SetFriendlyName(void)
{
  CSingleLock lock(m_critSection);

   if (m_bGotFriendlyName || !m_bReadyToUse)
     return;

   m_bGotFriendlyName  = true;

  m_strFriendlyName.Format("%s:%s", GetBackendName(), GetConnectionString());
}

PVR_ERROR CPVRClient::SetProperties(void)
{
  CSingleLock lock(m_critSection);

  if (m_bGotServerProperties)
    return PVR_ERROR_NO_ERROR;

  /* reset all properties to disabled */
  m_serverProperties.bSupportsChannelLogo     = false;
  m_serverProperties.bSupportsTimeshift       = false;
  m_serverProperties.bSupportsEPG             = false;
  m_serverProperties.bSupportsRecordings      = false;
  m_serverProperties.bSupportsTimers          = false;
  m_serverProperties.bSupportsRadio           = false;
  m_serverProperties.bSupportsChannelSettings = false;
  m_serverProperties.bSupportsChannelGroups   = false;
  m_serverProperties.bSupportsChannelScan     = false;

  /* try to get the addon properties */
  try
  {
    PVR_ERROR retVal = m_pStruct->GetAddonCapabilities(&m_serverProperties);
    if (retVal == PVR_ERROR_NO_ERROR)
      m_bGotServerProperties = true;

    return retVal;
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetProperties() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return PVR_ERROR_SERVER_ERROR;
}

void CPVRClient::WriteClientGroupInfo(const CPVRChannelGroup &xbmcGroup, PVR_CHANNEL_GROUP &addonGroup)
{
  addonGroup.bIsRadio     = xbmcGroup.IsRadio();
  addonGroup.strGroupName = xbmcGroup.GroupName();
}
