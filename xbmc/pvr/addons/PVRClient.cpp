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
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace std;
using namespace ADDON;

/* TODO factor out the annoying time conversion */

CPVRClient::CPVRClient(const AddonProps& props) :
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>(props),
    m_bReadyToUse(false),
    m_strHostName("unknown"),
    m_iTimeCorrection(0),
    m_bGotTimeCorrection(false),
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
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>(ext),
    m_bReadyToUse(false),
    m_strHostName("unknown"),
    m_iTimeCorrection(0),
    m_bGotTimeCorrection(false),
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
  m_pInfo               = new PVR_PROPS;
  m_pInfo->clientID     = iClientId;
  CStdString userpath   = _P(Profile());
  m_pInfo->userpath     = userpath.c_str();
  CStdString clientpath = _P(Path());
  m_pInfo->clientpath   = clientpath.c_str();

  /* initialise the add-on */
  if (CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>::Create())
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
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>::Destroy();
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
  long clientID             = m_pInfo->clientID;
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

  return m_pInfo->clientID;
}

PVR_ERROR CPVRClient::GetProperties(PVR_SERVERPROPS *props)
{
  CSingleLock lock(m_critSection);

  /* cached locally */
  PVR_ERROR retVal = SetProperties();
  *props = m_serverProperties;

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

PVR_ERROR CPVRClient::GetBackendTime(time_t *localTime, int *iGmtOffset)
{
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return PVR_ERROR_UNKOWN;

  try
  {
    return m_pStruct->GetBackendTime(localTime, iGmtOffset);
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetBackendTime() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  /* default to 0 on error */
  *localTime = 0;
  *iGmtOffset = 0;

  return PVR_ERROR_NOT_IMPLEMENTED;
}

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
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call DialogChannelScan() on addon '%s'. please contact the developer of this addon: %s",
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
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call MenuHook() on addon '%s'. please contact the developer of this addon: %s",
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

    int iTimeCorrection = GetTimeCorrection(); // XXX time correction

    PVRHANDLE_STRUCT handle;
    handle.CALLER_ADDRESS = this;
    handle.DATA_ADDRESS = (CPVREpg*) epg;
    handle.DATA_IDENTIFIER = bSaveInDb ? 1 : 0; // used by the callback method CAddonCallbacksPVR::PVRTransferEpgEntry()
    retVal = m_pStruct->RequestEPGForChannel(&handle, addonChannel, start ? start - iTimeCorrection : 0, end ? end - iTimeCorrection : 0);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RequestEPGForChannel()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RequestEPGForChannel() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetNumChannels(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetNumChannels();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetNumChannels() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetChannelList(CPVRChannelGroup &channels, bool radio)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVRHANDLE_STRUCT handle;
    handle.CALLER_ADDRESS = this;
    handle.DATA_ADDRESS = (CPVRChannelGroup*) &channels;
    retVal = m_pStruct->RequestChannelList(&handle, radio);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RequestChannelList()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RequestChannelList() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return retVal;
}

int CPVRClient::GetNumRecordings(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetNumRecordings();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetNumRecordings() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetAllRecordings(CPVRRecordings *results)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVRHANDLE_STRUCT handle;
    handle.CALLER_ADDRESS = this;
    handle.DATA_ADDRESS = (CPVRRecordings*) results;
    retVal = m_pStruct->RequestRecordingsList(&handle);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RequestRecordingsList()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RequestRecordingsList() on addon '%s'. please contact the developer of this addon: %s",
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
    PVR_RECORDINGINFO tag;
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

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecording &recording, const CStdString &strNewName)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVR_RECORDINGINFO tag;
    WriteClientRecordingInfo(recording, tag);

    retVal = m_pStruct->RenameRecording(tag, strNewName);

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

void CPVRClient::WriteClientRecordingInfo(const CPVRRecording &xbmcRecording, PVR_RECORDINGINFO &addonRecording)
{
  int iTimeCorrection = GetTimeCorrection(); // XXX time correction
  time_t recTime;
  xbmcRecording.m_recordingTime.GetAsTime(recTime);

  addonRecording.recording_time = recTime + iTimeCorrection;
  addonRecording.index          = xbmcRecording.m_clientIndex;
  addonRecording.title          = xbmcRecording.m_strTitle.c_str();
  addonRecording.subtitle       = xbmcRecording.m_strPlotOutline.c_str();
  addonRecording.description    = xbmcRecording.m_strPlot.c_str();
  addonRecording.channel_name   = xbmcRecording.m_strChannel.c_str();
  addonRecording.duration       = xbmcRecording.GetDuration();
  addonRecording.priority       = xbmcRecording.m_Priority;
  addonRecording.lifetime       = xbmcRecording.m_Lifetime;
  addonRecording.directory      = xbmcRecording.m_strDirectory.c_str();
  addonRecording.stream_url     = xbmcRecording.m_strStreamURL.c_str();
}

int CPVRClient::GetNumTimers(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return iReturn;

  try
  {
    iReturn = m_pStruct->GetNumTimers();
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call GetNumTimers() on addon '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), GetFriendlyName(), Author().c_str());
  }

  return iReturn;
}

PVR_ERROR CPVRClient::GetAllTimers(CPVRTimers *results)
{
  PVR_ERROR retVal = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return retVal;

  try
  {
    PVRHANDLE_STRUCT handle;
    handle.CALLER_ADDRESS = this;
    handle.DATA_ADDRESS = (CPVRTimers*) results;
    retVal = m_pStruct->RequestTimerList(&handle);

    if (retVal != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - addon '%s' returns bad error (%i) from RequestTimerList()",
          __FUNCTION__, GetFriendlyName(), retVal);
    }
  }
  catch (exception &e)
  {
    CLog::Log(LOGERROR, "PVRClient - %s - exception '%s' caught while trying to call RequestTimerList() on addon '%s'. please contact the developer of this addon: %s",
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
    PVR_TIMERINFO tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myTitle(timer.m_strTitle);
    CStdString myDirectory(timer.m_strDir);
    CStdString myDescription(timer.m_strSummary);
    tag.title = myTitle.c_str();
    tag.directory = myDirectory.c_str();
    tag.description = myDescription.c_str();

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
    PVR_TIMERINFO tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myTitle(timer.m_strTitle);
    tag.title = myTitle.c_str();

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
    PVR_TIMERINFO tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myTitle(timer.m_strTitle);
    tag.title = myTitle.c_str();

    retVal = m_pStruct->RenameTimer(tag, strNewName.c_str());

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
    PVR_TIMERINFO tag;
    WriteClientTimerInfo(timer, tag);

    //Workaround for string transfer to PVRclient
    CStdString myTitle(timer.m_strTitle);
    CStdString myDirectory(timer.m_strDir);
    tag.title = myTitle.c_str();
    tag.directory = myDirectory.c_str();

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

void CPVRClient::WriteClientTimerInfo(const CPVRTimerInfoTag &xbmcTimer, PVR_TIMERINFO &addonTimer)
{
  int iTimeCorrection = GetTimeCorrection(); // XXX time correction

  addonTimer.index       = xbmcTimer.m_iClientIndex;
  addonTimer.active      = xbmcTimer.m_bIsActive;
  addonTimer.channelNum  = xbmcTimer.m_iClientNumber;
  addonTimer.channelUid  = xbmcTimer.m_iClientChannelUid;
  addonTimer.recording   = xbmcTimer.m_bIsRecording;
  addonTimer.title       = xbmcTimer.m_strTitle;
  addonTimer.directory   = xbmcTimer.m_strDir;
  addonTimer.priority    = xbmcTimer.m_iPriority;
  addonTimer.lifetime    = xbmcTimer.m_iLifetime;
  addonTimer.repeat      = xbmcTimer.m_bIsRepeating;
  addonTimer.repeatflags = xbmcTimer.m_iWeekdays;
  addonTimer.starttime   = xbmcTimer.StartTime() - iTimeCorrection;
  addonTimer.endtime     = xbmcTimer.StopTime() - iTimeCorrection;
  addonTimer.firstday    = xbmcTimer.FirstDayTime() - iTimeCorrection;
  addonTimer.epgid       = xbmcTimer.m_EpgInfo ? xbmcTimer.m_EpgInfo->UniqueBroadcastID() : -1;
  addonTimer.description = xbmcTimer.m_strSummary.c_str();
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

bool CPVRClient::SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);
  if (!m_bReadyToUse)
    return bReturn;

  try
  {
    PVR_ERROR error = m_pStruct->SignalQuality(qualityinfo);
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
  addonChannel.uid              = xbmcChannel.UniqueID();
  addonChannel.number           = xbmcChannel.ClientChannelNumber();
  addonChannel.name             = xbmcChannel.ChannelName().c_str();
  addonChannel.callsign         = xbmcChannel.ClientChannelName().c_str();
  addonChannel.iconpath         = xbmcChannel.IconPath().c_str();
  addonChannel.encryption       = xbmcChannel.EncryptionSystem();
  addonChannel.radio            = xbmcChannel.IsRadio();
  addonChannel.hide             = xbmcChannel.IsHidden();
  addonChannel.recording        = xbmcChannel.IsRecording();
  addonChannel.bouquet          = 0;
  addonChannel.multifeed        = false;
  addonChannel.multifeed_master = 0;
  addonChannel.multifeed_number = 0;
  addonChannel.input_format     = xbmcChannel.InputFormat().c_str();
  addonChannel.stream_url       = xbmcChannel.StreamURL().c_str();
}

bool CPVRClient::OpenRecordedStream(const CPVRRecording &recording)
{
  CSingleLock lock(m_critSection);

  PVR_RECORDINGINFO tag;
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

PVR_ERROR CPVRClient::GetStreamProperties(PVR_STREAMPROPS *props)
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

int CPVRClient::GetTimeCorrection(void)
{
  CSingleLock lock(m_critSection);
  SetTimeCorrection();

  return m_iTimeCorrection;
}

int CPVRClient::GetClientID(void) const
{
  CSingleLock lock(m_critSection);

  return m_pInfo->clientID;
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

void CPVRClient::SetTimeCorrection(void)
{
  CSingleLock lock(m_critSection);
  if (m_bGotTimeCorrection)
    return;

  m_bGotTimeCorrection = true;
  m_iTimeCorrection = 0;

  if (g_advancedSettings.m_bDisableEPGTimeCorrection)
  {
    CLog::Log(LOGDEBUG, "PVRClient - %s - timezone correction is disabled in advancedsettings.xml", __FUNCTION__);
  }
  else if (g_advancedSettings.m_iUserDefinedEPGTimeCorrection != 0)
  {
    m_iTimeCorrection = g_advancedSettings.m_iUserDefinedEPGTimeCorrection * 60;
    CLog::Log(LOGDEBUG, "PVRClient - %s - using user defined timezone correction of '%i' minutes (taken from advancedsettings.xml)",
        __FUNCTION__, g_advancedSettings.m_iUserDefinedEPGTimeCorrection);
  }
  else
  {
    /* check whether the backend tells us to use a GMT offset */
    time_t localTime;
    CDateTime::GetCurrentDateTime().GetAsTime(localTime);

    time_t backendTime = 0;
    int iGmtOffset = 0;
    PVR_ERROR err = GetBackendTime(&backendTime, &iGmtOffset);
    if (err != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVRClient - %s - failed to get the backend time from '%s'",
          __FUNCTION__, GetFriendlyName());
    }
    else if (iGmtOffset != 0)
    {
      /* only use a correction if the difference between our local time and the backend's local time is larger than 30 minutes */
      if (backendTime - localTime > 30 || backendTime - localTime < -30)
      {
        m_iTimeCorrection = iGmtOffset;
        CLog::Log(LOGDEBUG, "PVRClient - %s - using GMT offset '%d' to correct EPG times for backend '%s'",
            __FUNCTION__, m_iTimeCorrection, GetFriendlyName());
      }
      else
      {
        CLog::Log(LOGDEBUG, "PVRClient - %s/%s - ignoring GMT offset '%d' because the backend's time and XBMC's time don't differ (much)",
            __FUNCTION__, GetFriendlyName(), iGmtOffset);
      }
    }
  }
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
  m_serverProperties.SupportChannelLogo     = false;
  m_serverProperties.SupportTimeShift       = false;
  m_serverProperties.SupportEPG             = false;
  m_serverProperties.SupportRecordings      = false;
  m_serverProperties.SupportTimers          = false;
  m_serverProperties.SupportRadio           = false;
  m_serverProperties.SupportChannelSettings = false;
  m_serverProperties.SupportDirector        = false;
  m_serverProperties.SupportBouquets        = false;
  m_serverProperties.SupportChannelScan     = false;

  /* try to get the addon properties */
  try
  {
    PVR_ERROR retVal = m_pStruct->GetProperties(&m_serverProperties);
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
