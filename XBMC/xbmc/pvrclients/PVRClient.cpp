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

#include "stdafx.h"
#include <vector>

#include "PVRClient.h"
#include "URL.h"
#include "../utils/log.h"
#include "../utils/AddonHelpers.h"

using namespace std;
using namespace ADDON;

CPVRClient::CPVRClient(long clientID, struct PVRClient* pClient, DllPVRClient* pDll,
                       const ADDON::CAddon& addon, IPVRClientCallback* pvrCB)
                              : IPVRClient(clientID, addon, pvrCB)
                              , m_clientID(clientID)
                              , m_pClient(pClient)
                              , m_pDll(pDll)
                              , m_manager(pvrCB)
                              , m_ReadyToUse(false)
                              , m_hostName("unknown")
                              , m_callbacks(NULL)
{

}

CPVRClient::~CPVRClient()
{
  /* tell the AddOn to deinitialize */
  DeInit();
}

bool CPVRClient::Init()
{
  CLog::Log(LOGDEBUG, "PVR: %s - Initializing PVR-Client AddOn", m_strName.c_str());

  /* Allocate the callback table to save all the pointers
     to the helper callback functions */
  m_callbacks = new AddonCB;

  /* PVR Helper functions */
  m_callbacks->userData     = this;
  m_callbacks->addonData    = (CAddon*) this;

  /* Write XBMC Global Add-on function addresses to callback table */
  CAddonUtils::CreateAddOnCallbacks(m_callbacks);

  /* Write XBMC PVR specific Add-on function addresses to callback table */
  m_callbacks->PVR.EventCallback        = PVREventCallback;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  try
  {
    ADDON_STATUS status = m_pClient->Create(m_callbacks, m_clientID);
    if (status != STATUS_OK)
      throw status;
    m_ReadyToUse = true;
    m_hostName   = m_pClient->GetConnectionString();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during Create occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    m_ReadyToUse = false;
  }
  catch (ADDON_STATUS status)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad status (%i) after Create and is not usable", m_strName.c_str(), m_hostName.c_str(), status);
    m_ReadyToUse = false;

    /* Delete is performed by the calling class */
    new CAddonStatusHandler(this, status, "", false);
  }
  
  return m_ReadyToUse;
}

void CPVRClient::DeInit()
{
  /* tell the AddOn to disconnect and prepare for destruction */
  try
  {
    CLog::Log(LOGDEBUG, "PVR: %s/%s - Destroying PVR-Client AddOn", m_strName.c_str(), m_hostName.c_str());
    m_ReadyToUse = false;

    /* Tell the client to destroy */
    m_pClient->Destroy();

    /* Release Callback table in memory */
    delete m_callbacks;
    m_callbacks = NULL;
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during destruction of AddOn occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
  }
}

void CPVRClient::ReInit()
{
  DeInit();
  Init();
}

ADDON_STATUS CPVRClient::GetStatus()
{
  try
  {
    return m_pDll->GetStatus();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetStatus occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
  }
  return STATUS_UNKNOWN;
}

long CPVRClient::GetID()
{
  return m_clientID;
}

PVR_ERROR CPVRClient::GetProperties(PVR_SERVERPROPS *props)
{
  try
  {
    return m_pClient->GetProperties(props);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetProperties occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());

    /* Set all properties in a case of exception to not supported */
    props->SupportChannelLogo        = false;
    props->SupportTimeShift          = false;
    props->SupportEPG                = false;
    props->SupportRecordings         = false;
    props->SupportTimers             = false;
    props->SupportRadio              = false;
    props->SupportChannelSettings    = false;
    props->SupportTeletext           = false;
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
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetBackendName();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetBackendName occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

const std::string CPVRClient::GetBackendVersion()
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetBackendVersion();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetBackendVersion occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

const std::string CPVRClient::GetConnectionString()
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetConnectionString();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetConnectionString occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  /* return string "Unavailable" as fallback */
  return g_localizeStrings.Get(161);
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *total, long long *used)
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetDriveSpace(total, used);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetDriveSpace occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  *total = 0;
  *used  = 0;
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/**********************************************************
 * EPG PVR Functions
 */

PVR_ERROR CPVRClient::GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end)
{
  return m_pClient->GetEPGForChannel(number, epg, start, end);
}

PVR_ERROR CPVRClient::GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return m_pClient->GetEPGNowInfo(number, result);
}

PVR_ERROR CPVRClient::GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return m_pClient->GetEPGNextInfo(number, result);
}


/**********************************************************
 * Channels PVR Functions
 */
 
int CPVRClient::GetNumChannels()
{
  return m_pClient->GetNumChannels();
}

PVR_ERROR CPVRClient::GetChannelList(VECCHANNELS &channels, bool radio)
{
  PVR_ERROR ret = m_pClient->GetChannelList(&channels, radio);

  for (unsigned int i = 0; i < channels.size(); i++)
  {
    channels[i].m_clientID = m_clientID;
  }
  return ret;
}

PVR_ERROR CPVRClient::GetChannelSettings(CTVChannelInfoTag *result)
{
  return m_pClient->GetChannelSettings(result);
}

PVR_ERROR CPVRClient::UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)
{
  return m_pClient->UpdateChannelSettings(chaninfo);
}

PVR_ERROR CPVRClient::AddChannel(const CTVChannelInfoTag &info)
{
  return m_pClient->AddChannel(info);
}

PVR_ERROR CPVRClient::DeleteChannel(unsigned int number)
{
  return m_pClient->DeleteChannel(number);
}

PVR_ERROR CPVRClient::RenameChannel(unsigned int number, CStdString &newname)
{
  return m_pClient->RenameChannel(number, newname);
}

PVR_ERROR CPVRClient::MoveChannel(unsigned int number, unsigned int newnumber)
{
  return m_pClient->MoveChannel(number, newnumber);
}


/**********************************************************
 * Recordings PVR Functions
 */

int CPVRClient::GetNumRecordings(void)
{
  return m_pClient->GetNumRecordings();
}

PVR_ERROR CPVRClient::GetAllRecordings(VECRECORDINGS *results)
{
  PVR_ERROR ret = m_pClient->GetAllRecordings(results);

  for (unsigned int i = 0; i < results->size(); i++)
  {
    results->at(i).m_clientID = m_clientID;
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteRecording(const CTVRecordingInfoTag &recinfo)
{
  return m_pClient->DeleteRecording(recinfo);
}

PVR_ERROR CPVRClient::RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname)
{
  return m_pClient->RenameRecording(recinfo, newname);
}


/**********************************************************
 * Timers PVR Functions
 */

int CPVRClient::GetNumTimers(void)
{
  return m_pClient->GetNumTimers();
}

PVR_ERROR CPVRClient::GetAllTimers(VECTVTIMERS *results)
{
  PVR_ERROR ret = m_pClient->GetAllTimers(results);

  for (unsigned int i = 0; i < results->size(); i++)
  {
    results->at(i).m_clientID = m_clientID;
  }
  return ret;
}

PVR_ERROR CPVRClient::AddTimer(const CTVTimerInfoTag &timerinfo)
{
  return m_pClient->AddTimer(timerinfo);
}

PVR_ERROR CPVRClient::DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force)
{
  return m_pClient->DeleteTimer(timerinfo, force);
}

PVR_ERROR CPVRClient::RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)
{
  return m_pClient->RenameTimer(timerinfo, newname);
}

PVR_ERROR CPVRClient::UpdateTimer(const CTVTimerInfoTag &timerinfo)
{
  return m_pClient->UpdateTimer(timerinfo);
}


/**********************************************************
 * Stream PVR Functions
 */

bool CPVRClient::OpenLiveStream(unsigned int channel)
{
  return m_pClient->OpenLiveStream(channel);
}

void CPVRClient::CloseLiveStream()
{
  return m_pClient->CloseLiveStream();
}

int CPVRClient::ReadLiveStream(BYTE* buf, int buf_size)
{
  return m_pClient->ReadLiveStream(buf, buf_size);
}

int CPVRClient::GetCurrentClientChannel()
{
  return m_pClient->GetCurrentClientChannel();
}

bool CPVRClient::SwitchChannel(unsigned int channel)
{
  return m_pClient->SwitchChannel(channel);
}

bool CPVRClient::OpenRecordedStream(const CTVRecordingInfoTag &recinfo)
{
  return m_pClient->OpenRecordedStream(recinfo);
}

void CPVRClient::CloseRecordedStream(void)
{
  return m_pClient->CloseRecordedStream();
}

int CPVRClient::ReadRecordedStream(BYTE* buf, int buf_size)
{
  return m_pClient->ReadRecordedStream(buf, buf_size);
}

__int64 CPVRClient::SeekRecordedStream(__int64 pos, int whence)
{
  return m_pClient->SeekRecordedStream(pos, whence);
}

__int64 CPVRClient::LengthRecordedStream(void)
{
  return m_pClient->LengthRecordedStream();
}


/**********************************************************
 * Addon specific functions
 * Are used for every type of AddOn
 */
void CPVRClient::Remove()
{
  DeInit();

  /* Unload library file */
  m_pDll->Unload();
}

ADDON_STATUS CPVRClient::SetSetting(const char *settingName, const void *settingValue)
{
  try
  {
    return m_pDll->SetSetting(settingName, settingValue);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during SetSetting occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    return STATUS_UNKNOWN;
  }
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

  client->m_manager->OnClientMessage(client->m_clientID, pvrevent, msg);
}
