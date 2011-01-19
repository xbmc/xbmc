/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * Description:
 *
 * Class CPVRClient is used as a specific interface between the PVR-Client
 * library and the PVRManager. Every loaded Client have his own CPVRClient
 * Class, it handle default data for the Manager in the case the Client
 * can't provide the data and it act as exception handler for all function
 * called inside client. Further it translate the "C" compatible data
 * strucures to classes that can easily used by the PVRManager.
 *
 * It generate also a callback table with pointers to useful helper
 * functions, that can be used inside the client to access XBMC
 * internals.
 */

#include <vector>
#include "Application.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"
#include "FileItem.h"
#include "PVRClient.h"
#include "pvr/PVRManager.h"
#include "URL.h"
#include "AdvancedSettings.h"
#include "../utils/log.h"
#include "../utils/SingleLock.h"

using namespace std;
using namespace ADDON;

CPVRClient::CPVRClient(const ADDON::AddonProps& props) : CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>(props)
                              , m_ReadyToUse(false)
                              , m_hostName("unknown")
                              , m_iTimeCorrection(0)
{
}

CPVRClient::CPVRClient(const cp_extension_t *ext) : CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>(ext)
                              , m_ReadyToUse(false)
                              , m_hostName("unknown")
                              , m_iTimeCorrection(0)
{
}

CPVRClient::~CPVRClient()
{
}

bool CPVRClient::Create(long clientID, IPVRClientCallback *pvrCB)
{
  CLog::Log(LOGDEBUG, "PVR: %s - Creating PVR-Client AddOn", Name().c_str());

  m_manager = pvrCB;

  m_pInfo           = new PVR_PROPS;
  m_pInfo->clientID = clientID;
  CStdString userpath = _P(Profile());
  m_pInfo->userpath = userpath.c_str();
  CStdString clientpath = _P(Path());
  m_pInfo->clientpath = clientpath.c_str();

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  if (CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>::Create())
  {
    m_ReadyToUse = true;
    m_hostName   = m_pStruct->GetConnectionString();
    if (!g_advancedSettings.m_bDisableEPGTimeCorrection)
    {
      time_t localTime;
      time_t backendTime = 0;
      int    gmtOffset   = 0;
      CDateTime::GetCurrentDateTime().GetAsTime(localTime);
      PVR_ERROR err = GetBackendTime(&backendTime, &gmtOffset);
      if (err == PVR_ERROR_NO_ERROR && gmtOffset != 0)
      {
        /* Is really a big time difference between PVR Backend and XBMC or only a bad GMT Offset? */
        if (backendTime-localTime >= 30 || backendTime-localTime <= -30)
        {
          m_iTimeCorrection = gmtOffset;
          CLog::Log(LOGDEBUG, "PVR: %s/%s - Using a timezone difference of '%i' minutes to correct EPG times", Name().c_str(), m_hostName.c_str(), m_iTimeCorrection/60);
        }
        else
        {
          m_iTimeCorrection = 0;
          CLog::Log(LOGDEBUG, "PVR: %s/%s - Ignoring the timezone difference of '%i' minutes (No difference betweem XBMC and Backend Clock found)", Name().c_str(), m_hostName.c_str(), m_iTimeCorrection/60);
        }
      }
    }
    else if (g_advancedSettings.m_iUserDefinedEPGTimeCorrection > 0)
    {
      m_iTimeCorrection = g_advancedSettings.m_iUserDefinedEPGTimeCorrection*60;
      CLog::Log(LOGDEBUG, "PVR: %s/%s - Using a userdefined timezone difference of '%i' minutes (taken from advancedsettings.xml)", Name().c_str(), m_hostName.c_str(), g_advancedSettings.m_iUserDefinedEPGTimeCorrection);
    }
    else
    {
      m_iTimeCorrection = 0;
      CLog::Log(LOGDEBUG, "PVR: %s/%s - Timezone difference correction is disabled in advancedsettings.xml", Name().c_str(), m_hostName.c_str());
    }
  }

  return m_ReadyToUse;
}

void CPVRClient::Destroy()
{
  /* tell the AddOn to disconnect and prepare for destruction */
  try
  {
    CLog::Log(LOGDEBUG, "PVR: %s/%s - Destroying PVR-Client AddOn", Name().c_str(), m_hostName.c_str());
    m_ReadyToUse = false;

    /* Tell the client to destroy */
    CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>::Destroy();
    m_menuhooks.clear();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during destruction of AddOn occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
  }
}

bool CPVRClient::ReCreate()
{
  long clientID = m_pInfo->clientID;
  IPVRClientCallback *pvrCB = m_manager;
  Destroy();
  return Create(clientID, pvrCB);
}

long CPVRClient::GetID()
{
  return m_pInfo->clientID;
}

PVR_ERROR CPVRClient::GetProperties(PVR_SERVERPROPS *props)
{
  CSingleLock lock(m_critSection);

  try
  {
    return m_pStruct->GetProperties(props);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetProperties occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());

    /* Set all properties in a case of exception to not supported */
    props->SupportChannelLogo        = false;
    props->SupportTimeShift          = false;
    props->SupportEPG                = false;
    props->SupportRecordings         = false;
    props->SupportTimers             = false;
    props->SupportRadio              = false;
    props->SupportChannelSettings    = false;
    props->SupportDirector           = false;
    props->SupportBouquets           = false;
    props->SupportChannelScan        = false;
  }
  return PVR_ERROR_UNKOWN;
}


/**********************************************************
 * General PVR Functions
 */

const std::string CPVRClient::GetBackendName()
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetBackendName();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetBackendName occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

const std::string CPVRClient::GetBackendVersion()
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetBackendVersion();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetBackendVersion occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

const std::string CPVRClient::GetConnectionString()
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetConnectionString();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetConnectionString occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *total, long long *used)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetDriveSpace(total, used);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetDriveSpace occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  *total = 0;
  *used  = 0;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR CPVRClient::GetBackendTime(time_t *localTime, int *gmtOffset)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetBackendTime(localTime, gmtOffset);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetBackendTime occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  *localTime = 0;
  *gmtOffset = 0;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR CPVRClient::StartChannelScan()
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->DialogChannelScan();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during StartChannelScan occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  return PVR_ERROR_NOT_IMPLEMENTED;
}

void CPVRClient::CallMenuHook(const PVR_MENUHOOK &hook)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      m_pStruct->MenuHook(hook);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during CallMenuHook occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
}

/**********************************************************
 * EPG PVR Functions
 */

PVR_ERROR CPVRClient::GetEPGForChannel(const CPVRChannel &channelinfo, CPVREpg *epg, time_t start, time_t end, bool toDB/* = false*/)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      if (start)
        start -= m_iTimeCorrection;
      if (end)
        end -= m_iTimeCorrection;
      PVR_CHANNEL tag;
      PVRHANDLE_STRUCT handle;
      handle.CALLER_ADDRESS = this;
      handle.DATA_ADDRESS = (CPVREpg*) epg;
      handle.DATA_IDENTIFIER = toDB ? 1 : 0;
      WriteClientChannelInfo(channelinfo, tag);
      ret = m_pStruct->RequestEPGForChannel(&handle, tag, start, end);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetEPGForChannel occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetEPGForChannel", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}


/**********************************************************
 * Channels PVR Functions
 */

int CPVRClient::GetNumChannels()
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetNumChannels();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumChannels occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  return -1;
}

PVR_ERROR CPVRClient::GetChannelList(CPVRChannelGroup &channels, bool radio)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVRHANDLE_STRUCT handle;
      handle.CALLER_ADDRESS = this;
      handle.DATA_ADDRESS = (CPVRChannelGroup*) &channels;
      ret = m_pStruct->RequestChannelList(&handle, radio);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetChannelList occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetChannelList", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

/**********************************************************
 * Recordings PVR Functions
 */

int CPVRClient::GetNumRecordings(void)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetNumRecordings();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumRecordings occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  return -1;
}

PVR_ERROR CPVRClient::GetAllRecordings(CPVRRecordings *results)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVRHANDLE_STRUCT handle;
      handle.CALLER_ADDRESS = this;
      handle.DATA_ADDRESS = (CPVRRecordings*) results;
      ret = m_pStruct->RequestRecordingsList(&handle);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetAllRecordings occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetAllRecordings", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteRecording(const CPVRRecordingInfoTag &recinfo)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_RECORDINGINFO tag;
      WriteClientRecordingInfo(recinfo, tag);

      ret = m_pStruct->DeleteRecording(tag);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during DeleteRecording occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after DeleteRecording", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::RenameRecording(const CPVRRecordingInfoTag &recinfo, CStdString &newname)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_RECORDINGINFO tag;
      WriteClientRecordingInfo(recinfo, tag);

      ret = m_pStruct->RenameRecording(tag, newname);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during RenameRecording occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after RenameRecording", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

void CPVRClient::WriteClientRecordingInfo(const CPVRRecordingInfoTag &recordinginfo, PVR_RECORDINGINFO &tag)
{
  time_t recTime;
  recordinginfo.RecordingTime().GetAsTime(recTime);
  tag.recording_time= recTime+m_iTimeCorrection;
  tag.index         = recordinginfo.ClientIndex();
  tag.title         = recordinginfo.Title();
  tag.subtitle      = recordinginfo.PlotOutline();
  tag.description   = recordinginfo.Plot();
  tag.channel_name  = recordinginfo.ChannelName();
  tag.duration      = recordinginfo.GetDuration();
  tag.priority      = recordinginfo.Priority();
  tag.lifetime      = recordinginfo.Lifetime();
  tag.directory     = recordinginfo.Directory();
  tag.stream_url    = recordinginfo.StreamURL();
  return;
}


/**********************************************************
 * Timers PVR Functions
 */

int CPVRClient::GetNumTimers(void)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      return m_pStruct->GetNumTimers();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumTimers occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  return -1;
}

PVR_ERROR CPVRClient::GetAllTimers(CPVRTimers *results)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVRHANDLE_STRUCT handle;
      handle.CALLER_ADDRESS = this;
      handle.DATA_ADDRESS = (CPVRTimers*) results;
      ret = m_pStruct->RequestTimerList(&handle);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetAllTimers occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetAllTimers", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::AddTimer(const CPVRTimerInfoTag &timerinfo)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      //Workaround for string transfer to PVRclient
      CStdString myTitle = timerinfo.Title();
      CStdString myDirectory = timerinfo.Dir();
      tag.title = myTitle.c_str();
      tag.directory = myDirectory.c_str();

      ret = m_pStruct->AddTimer(tag);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during AddTimer occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after AddTimer", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteTimer(const CPVRTimerInfoTag &timerinfo, bool force)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      //Workaround for string transfer to PVRclient
      CStdString myTitle = timerinfo.Title();
      tag.title = myTitle.c_str();

      ret = m_pStruct->DeleteTimer(tag, force);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during DeleteTimer occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after DeleteTimer", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::RenameTimer(const CPVRTimerInfoTag &timerinfo, const CStdString &newname)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      //Workaround for string transfer to PVRclient
      CStdString myTitle = timerinfo.Title();
      tag.title = myTitle.c_str();

      ret = m_pStruct->RenameTimer(tag, newname.c_str());
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during RenameTimer occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after RenameTimer", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::UpdateTimer(const CPVRTimerInfoTag &timerinfo)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;
//
  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      //Workaround for string transfer to PVRclient
      CStdString myTitle = timerinfo.Title();
      CStdString myDirectory = timerinfo.Dir();
      tag.title = myTitle.c_str();
      tag.directory = myDirectory.c_str();

      ret = m_pStruct->UpdateTimer(tag);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during UpdateTimer occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after UpdateTimer", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

void CPVRClient::WriteClientTimerInfo(const CPVRTimerInfoTag &timerinfo, PVR_TIMERINFO &tag)
{
  tag.index         = timerinfo.ClientIndex();
  tag.active        = timerinfo.Active();
  tag.channelNum    = timerinfo.ClientNumber();
  tag.recording     = timerinfo.IsRecording();
  tag.title         = timerinfo.Title();
  tag.directory     = timerinfo.Dir();
  tag.priority      = timerinfo.Priority();
  tag.lifetime      = timerinfo.Lifetime();
  tag.repeat        = timerinfo.IsRepeating();
  tag.repeatflags   = timerinfo.Weekdays();
  tag.starttime     = timerinfo.StartTime();
  tag.starttime    -= m_iTimeCorrection;
  tag.endtime       = timerinfo.StopTime();
  tag.endtime      -= m_iTimeCorrection;
  tag.firstday      = timerinfo.FirstDayTime();
  tag.firstday     -= m_iTimeCorrection;
  return;
}

/**********************************************************
 * Stream PVR Functions
 */

bool CPVRClient::OpenLiveStream(const CPVRChannel &channelinfo)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      PVR_CHANNEL tag;
      WriteClientChannelInfo(channelinfo, tag);
      return m_pStruct->OpenLiveStream(tag);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during OpenLiveStream occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  return false;
}

void CPVRClient::CloseLiveStream()
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      m_pStruct->CloseLiveStream();
      return;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during CloseLiveStream occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  return;
}

int CPVRClient::ReadLiveStream(void* lpBuf, int64_t uiBufSize)
{
  return m_pStruct->ReadLiveStream((unsigned char *)lpBuf, uiBufSize);
}

int64_t CPVRClient::SeekLiveStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  return m_pStruct->SeekLiveStream(iFilePosition, iWhence);
}

int64_t CPVRClient::PositionLiveStream()
{
  return m_pStruct->PositionLiveStream();
}

int64_t CPVRClient::LengthLiveStream(void)
{
  return m_pStruct->LengthLiveStream();
}

int CPVRClient::GetCurrentClientChannel()
{
  CSingleLock lock(m_critSection);

  return m_pStruct->GetCurrentClientChannel();
}

bool CPVRClient::SwitchChannel(const CPVRChannel &channelinfo)
{
  CSingleLock lock(m_critSection);

  PVR_CHANNEL tag;
  WriteClientChannelInfo(channelinfo, tag);
  return m_pStruct->SwitchChannel(tag);
}

bool CPVRClient::SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    PVR_ERROR ret = PVR_ERROR_UNKOWN;
    try
    {
      ret = m_pStruct->SignalQuality(qualityinfo);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return true;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during SignalQuality occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after SignalQuality", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return false;
}

const std::string CPVRClient::GetLiveStreamURL(const CPVRChannel &channelinfo)
{
  CSingleLock lock(m_critSection);

  if (m_ReadyToUse)
  {
    try
    {
      PVR_CHANNEL tag;
      WriteClientChannelInfo(channelinfo, tag);
      return m_pStruct->GetLiveStreamURL(tag);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetLiveStreamURL occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

void CPVRClient::WriteClientChannelInfo(const CPVRChannel &channelinfo, PVR_CHANNEL &tag)
{
  tag.uid               = channelinfo.UniqueID();
  tag.number            = channelinfo.ClientChannelNumber();
  tag.name              = channelinfo.ChannelName().c_str();
  tag.callsign          = channelinfo.ClientChannelName().c_str();
  tag.iconpath          = channelinfo.IconPath().c_str();
  tag.encryption        = channelinfo.EncryptionSystem();
  tag.radio             = channelinfo.IsRadio();
  tag.hide              = channelinfo.IsHidden();
  tag.recording         = channelinfo.IsRecording();
  tag.bouquet           = 0;
  tag.multifeed         = false;
  tag.multifeed_master  = 0;
  tag.multifeed_number  = 0;
  tag.input_format      = channelinfo.InputFormat();
  tag.stream_url        = channelinfo.StreamURL();
  return;
}

bool CPVRClient::OpenRecordedStream(const CPVRRecordingInfoTag &recinfo)
{
  CSingleLock lock(m_critSection);

  PVR_RECORDINGINFO tag;
  WriteClientRecordingInfo(recinfo, tag);
  return m_pStruct->OpenRecordedStream(tag);
}

void CPVRClient::CloseRecordedStream(void)
{
  CSingleLock lock(m_critSection);

  return m_pStruct->CloseRecordedStream();
}

int CPVRClient::ReadRecordedStream(void* lpBuf, int64_t uiBufSize)
{
  return m_pStruct->ReadRecordedStream((unsigned char *)lpBuf, uiBufSize);
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
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetStreamProperties occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());

    /* Set all properties in a case of exception to not supported */
  }
  return PVR_ERROR_UNKOWN;
}

void CPVRClient::DemuxReset()
{
  m_pStruct->DemuxReset();
}

void CPVRClient::DemuxAbort()
{
  m_pStruct->DemuxAbort();
}

void CPVRClient::DemuxFlush()
{
  m_pStruct->DemuxFlush();
}

DemuxPacket* CPVRClient::DemuxRead()
{
  return m_pStruct->DemuxRead();
}

/**********************************************************
 * Addon specific functions
 * Are used for every type of AddOn
 */

ADDON_STATUS CPVRClient::SetSetting(const char *settingName, const void *settingValue)
{
//  CSingleLock lock(m_critSection);
//
//  try
//  {
//    return m_pDll->SetSetting(settingName, settingValue);
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during SetSetting occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    return STATUS_UNKNOWN;
//  }
}
