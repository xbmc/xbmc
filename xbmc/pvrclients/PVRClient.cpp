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
#include "../utils/SingleLock.h"
#include "Application.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"
#include "FileItem.h"
#include "PVRClient.h"
#include "PVRManager.h"
#include "URL.h"
#include "../utils/log.h"
#include "../utils/AddonHelpers.h"

using namespace std;
using namespace ADDON;

CPVRClient::CPVRClient(const ADDON::AddonProps& props) : ADDON::CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>(props)
                              , m_ReadyToUse(false)
                              , m_hostName("unknown")
{
}

CPVRClient::~CPVRClient()
{
//  /* tell the AddOn to deinitialize */
//  DeInit();
//  /* Unload library file */
//  m_pDll->Unload();
}

bool CPVRClient::Create(long clientID, IPVRClientCallback *pvrCB)
{
  CLog::Log(LOGDEBUG, "PVR: %s - Creating PVR-Client AddOn", Name().c_str());

  m_manager = pvrCB;

  /* Allocate the callback table to save all the pointers
     to the helper callback functions */
  m_callbacks = new AddonCB;

  /* PVR Helper functions */
  m_callbacks->userData     = this;
  m_callbacks->addonData    = (CAddon*) this;

  /* Write XBMC Global Add-on function addresses to callback table */
  CAddonUtils::CreateAddOnCallbacks(m_callbacks);

  /* Write XBMC PVR specific Add-on function addresses to callback table */
  m_callbacks->PVR.EventCallback          = PVREventCallback;
  m_callbacks->PVR.TransferEpgEntry       = PVRTransferEpgEntry;
  m_callbacks->PVR.TransferChannelEntry   = PVRTransferChannelEntry;
  m_callbacks->PVR.TransferTimerEntry     = PVRTransferTimerEntry;
  m_callbacks->PVR.TransferRecordingEntry = PVRTransferRecordingEntry;

  m_pInfo           = new PVR_PROPS;
  m_pInfo->clientID = clientID;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  if (CAddonDll<DllPVRClient, PVRClient, PVR_PROPS>::Create())
  {
    try
    {
      ADDON_STATUS status = m_pStruct->Create(m_callbacks, m_pInfo->clientID);
      if (status != STATUS_OK)
        throw status;
      m_ReadyToUse = true;
      m_hostName   = m_pStruct->GetConnectionString();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during Create occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
      m_ReadyToUse = false;
    }
    catch (ADDON_STATUS status)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad status (%i) after Create and is not usable", Name().c_str(), m_hostName.c_str(), status);
      m_ReadyToUse = false;

      /* Delete is performed by the calling class */
      new CAddonStatusHandler(this, status, "", false);
    }
  }

  return m_ReadyToUse;
}

void CPVRClient::DeInit()
{
//  /* tell the AddOn to disconnect and prepare for destruction */
//  try
//  {
//    CLog::Log(LOGDEBUG, "PVR: %s/%s - Destroying PVR-Client AddOn", Name().c_str(), m_hostName.c_str());
//    m_ReadyToUse = false;
//
//    /* Tell the client to destroy */
//    m_pDll->Destroy();
//
//    /* Release Callback table in memory */
//    delete m_callbacks;
//    m_callbacks = NULL;
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during destruction of AddOn occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
//  }
}

ADDON_STATUS CPVRClient::GetStatus()
{
//  CSingleLock lock(m_critSection);
//
//  try
//  {
//    return m_pDll->GetStatus();
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetStatus occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
//  }
  return STATUS_UNKNOWN;
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


/**********************************************************
 * EPG PVR Functions
 */

PVR_ERROR CPVRClient::GetEPGForChannel(const cPVRChannelInfoTag &channelinfo, cPVREpg *epg, time_t start, time_t end)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_CHANNEL tag;
      const PVRHANDLE handle = (cPVREpg*) epg;
      WriteClientChannelInfo(channelinfo, tag);
      ret = m_pStruct->RequestEPGForChannel(handle, tag, start, end);
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

void CPVRClient::PVRTransferEpgEntry(void *userData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (client == NULL || handle == NULL || epgentry == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferEpgEntry is called with NULL-Pointer!!!");
    return;
  }

  cPVREpg *xbmcEpg = (cPVREpg*) handle;
  cPVREpg::Add(epgentry, xbmcEpg);
  return;
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

PVR_ERROR CPVRClient::GetChannelList(cPVRChannels &channels, bool radio)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      const PVRHANDLE handle = (cPVRChannels*) &channels;
      ret = m_pStruct->RequestChannelList(handle, radio);
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

void CPVRClient::PVRTransferChannelEntry(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *channel)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (client == NULL || handle == NULL || channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferChannelEntry is called with NULL-Pointer!!!");
    return;
  }

  cPVRChannels *xbmcChannels = (cPVRChannels*) handle;
  cPVRChannelInfoTag tag;

  tag.SetChannelID(-1);
  tag.SetNumber(-1);
  tag.SetClientNumber(channel->number);
  tag.SetGroupID(0);
  tag.SetClientID(client->m_pInfo->clientID);
  tag.SetUniqueID(channel->uid);
  tag.SetName(channel->name);
  tag.SetClientName(channel->callsign);
  tag.SetIcon(channel->iconpath);
  tag.SetEncryptionSystem(channel->encryption);
  tag.SetRadio(channel->radio);
  tag.SetHidden(channel->hide);
  tag.SetRecording(channel->recording);
  tag.SetStreamURL(channel->stream_url);

  xbmcChannels->push_back(tag);
  return;
}

PVR_ERROR CPVRClient::GetChannelSettings(cPVRChannelInfoTag *result)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pStruct->GetChannelSettings(result);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetChannelSettings occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetChannelSettings", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pStruct->UpdateChannelSettings(chaninfo);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during UpdateChannelSettings occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after UpdateChannelSettings", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::AddChannel(const cPVRChannelInfoTag &info)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pStruct->AddChannel(info);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during AddChannel occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after AddChannel", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteChannel(unsigned int number)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pStruct->DeleteChannel(number);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during DeleteChannel occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after DeleteChannel", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::RenameChannel(unsigned int number, CStdString &newname)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pStruct->RenameChannel(number, newname);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during RenameChannel occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after RenameChannel", Name().c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::MoveChannel(unsigned int number, unsigned int newnumber)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pStruct->MoveChannel(number, newnumber);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during MoveChannel occurred, contact Developer '%s' of this AddOn", Name().c_str(), m_hostName.c_str(), e.what(), Author().c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after MoveChannel", Name().c_str(), m_hostName.c_str(), ret);
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

PVR_ERROR CPVRClient::GetAllRecordings(cPVRRecordings *results)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      const PVRHANDLE handle = (cPVRRecordings*) results;
      ret = m_pStruct->RequestRecordingsList(handle);
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

void CPVRClient::PVRTransferRecordingEntry(void *userData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (client == NULL || handle == NULL || recording == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferRecordingEntry is called with NULL-Pointer!!!");
    return;
  }

  cPVRRecordings *xbmcRecordings = (cPVRRecordings*) handle;

  cPVRRecordingInfoTag tag;

  tag.SetClientIndex(recording->index);
  tag.SetClientID(client->m_pInfo->clientID);
  tag.SetTitle(recording->title);
  tag.SetRecordingTime(recording->recording_time);
  tag.SetDuration(CDateTimeSpan(0, 0, recording->duration / 60, recording->duration % 60));
  tag.SetPriority(recording->priority);
  tag.SetLifetime(recording->lifetime);
  tag.SetDirectory(recording->directory);
  tag.SetPlot(recording->description);
  tag.SetPlotOutline(recording->subtitle);
  tag.SetStreamURL(recording->stream_url);
  tag.SetChannelName(recording->channel_name);

  xbmcRecordings->push_back(tag);
  return;
}

PVR_ERROR CPVRClient::DeleteRecording(const cPVRRecordingInfoTag &recinfo)
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

PVR_ERROR CPVRClient::RenameRecording(const cPVRRecordingInfoTag &recinfo, CStdString &newname)
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

void CPVRClient::WriteClientRecordingInfo(const cPVRRecordingInfoTag &recordinginfo, PVR_RECORDINGINFO &tag)
{
  time_t recTime;
  recordinginfo.RecordingTime().GetAsTime(recTime);
  tag.recording_time= recTime;
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

PVR_ERROR CPVRClient::GetAllTimers(cPVRTimers *results)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      const PVRHANDLE handle = (cPVRTimers*) results;
      ret = m_pStruct->RequestTimerList(handle);
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

void CPVRClient::PVRTransferTimerEntry(void *userData, const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (client == NULL || handle == NULL || timer == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with NULL-Pointer!!!");
    return;
  }

  cPVRTimers *xbmcTimers     = (cPVRTimers*) handle;
  cPVRChannelInfoTag *channel  = cPVRChannels::GetByClientFromAll(timer->channelNum, client->m_pInfo->clientID);

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with not present channel");
    return;
  }

  cPVRTimerInfoTag tag;
  tag.SetClientID(client->m_pInfo->clientID);
  tag.SetClientIndex(timer->index);
  tag.SetActive(timer->active);
  tag.SetTitle(timer->title);
  tag.SetClientNumber(timer->channelNum);
  tag.SetStart((time_t) timer->starttime);
  tag.SetStop((time_t) timer->endtime);
  tag.SetFirstDay((time_t) timer->firstday);
  tag.SetPriority(timer->priority);
  tag.SetLifetime(timer->lifetime);
  tag.SetRecording(timer->recording);
  tag.SetRepeating(timer->repeat);
  tag.SetWeekdays(timer->repeatflags);
  tag.SetNumber(channel->Number());
  tag.SetRadio(channel->IsRadio());
  CStdString path;
  path.Format("pvr://client%i/timers/%i", tag.ClientID(), tag.ClientIndex());
  tag.SetPath(path);

  CStdString summary;
  if (!tag.IsRepeating())
  {
    summary.Format("%s %s %s %s %s", tag.Start().GetAsLocalizedDate()
                   , g_localizeStrings.Get(18078)
                   , tag.Start().GetAsLocalizedTime("", false)
                   , g_localizeStrings.Get(18079)
                   , tag.Stop().GetAsLocalizedTime("", false));
  }
  else if (tag.FirstDay() != NULL)
  {
    summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s"
                   , timer->repeatflags & 0x01 ? g_localizeStrings.Get(18080) : "__"
                   , timer->repeatflags & 0x02 ? g_localizeStrings.Get(18081) : "__"
                   , timer->repeatflags & 0x04 ? g_localizeStrings.Get(18082) : "__"
                   , timer->repeatflags & 0x08 ? g_localizeStrings.Get(18083) : "__"
                   , timer->repeatflags & 0x10 ? g_localizeStrings.Get(18084) : "__"
                   , timer->repeatflags & 0x20 ? g_localizeStrings.Get(18085) : "__"
                   , timer->repeatflags & 0x40 ? g_localizeStrings.Get(18086) : "__"
                   , g_localizeStrings.Get(18087)
                   , tag.FirstDay().GetAsLocalizedDate(false)
                   , g_localizeStrings.Get(18078)
                   , tag.Start().GetAsLocalizedTime("", false)
                   , g_localizeStrings.Get(18079)
                   , tag.Stop().GetAsLocalizedTime("", false));
  }
  else
  {
    summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s"
                   , timer->repeatflags & 0x01 ? g_localizeStrings.Get(18080) : "__"
                   , timer->repeatflags & 0x02 ? g_localizeStrings.Get(18081) : "__"
                   , timer->repeatflags & 0x04 ? g_localizeStrings.Get(18082) : "__"
                   , timer->repeatflags & 0x08 ? g_localizeStrings.Get(18083) : "__"
                   , timer->repeatflags & 0x10 ? g_localizeStrings.Get(18084) : "__"
                   , timer->repeatflags & 0x20 ? g_localizeStrings.Get(18085) : "__"
                   , timer->repeatflags & 0x40 ? g_localizeStrings.Get(18086) : "__"
                   , g_localizeStrings.Get(18078)
                   , tag.Start().GetAsLocalizedTime("", false)
                   , g_localizeStrings.Get(18079)
                   , tag.Stop().GetAsLocalizedTime("", false));
  }
  tag.SetSummary(summary);

  xbmcTimers->push_back(tag);
  return;
}

PVR_ERROR CPVRClient::AddTimer(const cPVRTimerInfoTag &timerinfo)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

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

PVR_ERROR CPVRClient::DeleteTimer(const cPVRTimerInfoTag &timerinfo, bool force)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

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

PVR_ERROR CPVRClient::RenameTimer(const cPVRTimerInfoTag &timerinfo, CStdString &newname)
{
  CSingleLock lock(m_critSection);

  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

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

PVR_ERROR CPVRClient::UpdateTimer(const cPVRTimerInfoTag &timerinfo)
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

void CPVRClient::WriteClientTimerInfo(const cPVRTimerInfoTag &timerinfo, PVR_TIMERINFO &tag)
{
  tag.index         = timerinfo.ClientIndex();
  tag.active        = timerinfo.Active();
  tag.channelNum    = timerinfo.ClientNumber();
  tag.recording     = timerinfo.IsRecording();
  tag.title         = timerinfo.Title();
  tag.priority      = timerinfo.Priority();
  tag.lifetime      = timerinfo.Lifetime();
  tag.repeat        = timerinfo.IsRepeating();
  tag.repeatflags   = timerinfo.Weekdays();
  tag.starttime     = timerinfo.StartTime();
  tag.endtime       = timerinfo.StopTime();
  tag.firstday      = timerinfo.FirstDayTime();
  return;
}

/**********************************************************
 * Stream PVR Functions
 */

bool CPVRClient::OpenLiveStream(const cPVRChannelInfoTag &channelinfo)
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

int CPVRClient::ReadLiveStream(BYTE* buf, int buf_size)
{
  return m_pStruct->ReadLiveStream(buf, buf_size);
}

__int64 CPVRClient::SeekLiveStream(__int64 pos, int whence)
{
  return m_pStruct->SeekLiveStream(pos, whence);
}

__int64 CPVRClient::LengthLiveStream(void)
{
  return m_pStruct->LengthLiveStream();
}

int CPVRClient::GetCurrentClientChannel()
{
  CSingleLock lock(m_critSection);

  return m_pStruct->GetCurrentClientChannel();
}

bool CPVRClient::SwitchChannel(const cPVRChannelInfoTag &channelinfo)
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

void CPVRClient::WriteClientChannelInfo(const cPVRChannelInfoTag &channelinfo, PVR_CHANNEL &tag)
{
  tag.uid               = channelinfo.UniqueID();
  tag.number            = channelinfo.ClientNumber();
  tag.name              = channelinfo.Name().c_str();
  tag.callsign          = channelinfo.ClientName().c_str();
  tag.iconpath          = channelinfo.Icon().c_str();
  tag.encryption        = channelinfo.EncryptionSystem();
  tag.radio             = channelinfo.IsRadio();
  tag.hide              = channelinfo.IsHidden();
  tag.recording         = channelinfo.IsRecording();
  tag.bouquet           = 0;
  tag.multifeed         = false;
  tag.multifeed_master  = 0;
  tag.multifeed_number  = 0;
  tag.stream_url        = channelinfo.StreamURL();
  return;
}

bool CPVRClient::OpenRecordedStream(const cPVRRecordingInfoTag &recinfo)
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

int CPVRClient::ReadRecordedStream(BYTE* buf, int buf_size)
{
  return m_pStruct->ReadRecordedStream(buf, buf_size);
}

__int64 CPVRClient::SeekRecordedStream(__int64 pos, int whence)
{
  return m_pStruct->SeekRecordedStream(pos, whence);
}

__int64 CPVRClient::LengthRecordedStream(void)
{
  return m_pStruct->LengthRecordedStream();
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


/**********************************************************
 * Client specific Callbacks
 * Are independent and can be different for every type of
 * AddOn
 */

void CPVRClient::PVREventCallback(void *userData, const PVR_EVENT pvrevent, const char *msg)
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return;

  client->m_manager->OnClientMessage(client->m_pInfo->clientID, pvrevent, msg);
}
