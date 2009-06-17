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

/**********************************************************
 * CPVRClient Class constructor/destructor
 */

CPVRClient::CPVRClient(long clientID, struct PVRClient* pClient,
                       IPVRClientCallback* pvrCB)
                              : IPVRClient(clientID, pvrCB)
                              , m_clientID(clientID)
                              , m_ReadyToUse(false)
                              , m_hostName("unknown")
                              , m_pClient(pClient)
                              , m_manager(pvrCB)
                              , m_callbacks(NULL)
{
  InitializeCriticalSection(&m_critSection);
}

CPVRClient::~CPVRClient()
{
  /* tell the AddOn to deinitialize */
  DeInit();

  DeleteCriticalSection(&m_critSection);
}


/**********************************************************
 * AddOn/PVR specific init and status functions
 */

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
  m_callbacks->PVR.TransferChannelEntry = PVRTransferChannelEntry;

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

/***** CHANNEL TRANSFER TEST *****/
//VECCHANNELS channels;
//GetChannelList(channels);
//  VECCHANNELS::iterator itr = channels.begin();
//  while (itr != channels.end())
//  {
//    fprintf(stderr, "<< %s __ %s\n", (*itr).m_strChannel.c_str(), (*itr).m_strFileNameAndPath.c_str());
//    itr++;
//  }

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
 * Bouquets PVR Functions
 */

int CPVRClient::GetNumBouquets()
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetNumBouquets();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumBouquets occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  return -1;
}

PVR_ERROR CPVRClient::GetBouquetInfo(const unsigned int number, PVR_BOUQUET& info)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/**********************************************************
 * Channels PVR Functions
 */

int CPVRClient::GetNumChannels()
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetNumChannels();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumChannels occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  return -1;
}

PVR_ERROR CPVRClient::GetChannelList(VECCHANNELS &channels)
{
  if (m_ReadyToUse)
  {
    try
    {
      const PVRHANDLE handle = (VECCHANNELS*) &channels;
      PVR_ERROR err = m_pClient->RequestChannelList(handle);
      if (err != PVR_ERROR_NO_ERROR)
        throw err;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetChannelList occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR err)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetChannelList", m_strName.c_str(), m_hostName.c_str(), err);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

void CPVRClient::PVRTransferChannelEntry(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *chan)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (!client || handle == NULL || chan == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferChannelEntry is called with NULL-Pointer");
    return;
  }

  VECCHANNELS *xbmcChans = (VECCHANNELS*) handle;

  CTVChannelInfoTag channel;
  channel.m_clientID            = client->m_clientID;
  channel.m_iClientNum          = chan->number;
  channel.m_IconPath            = chan->iconpath;
  channel.m_strChannel          = chan->name;
  channel.m_radio               = chan->radio;
  channel.m_encrypted           = chan->encrypted;
  channel.m_strFileNameAndPath  = chan->stream_url;

  xbmcChans->push_back(channel);
  return;
}


/**********************************************************
 * EPG PVR Functions
 */

PVR_ERROR CPVRClient::GetEPGForChannel(const unsigned int number, CFileItemList &results, const CDateTime &start, const CDateTime &end)
{
//  time_t start_t, end_t;
//  start.GetAsTime(start_t);
//  end.GetAsTime(end_t);
//
//  // all memory allocation takes place in client
//  PVR_ERROR err;
//  PVR_PROGLIST **epg = NULL;
//
//  err = m_pClient->GetEPGForChannel(number, epg, start_t, end_t);
//  if (err != PVR_ERROR_NO_ERROR)
//    return PVR_ERROR_SERVER_ERROR;

  // after client returns, we take ownership over the allocated memory

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClient::GetEPGNowInfo(const unsigned int number, PVR_PROGINFO *result)
{
  return PVR_ERROR_SERVER_ERROR;//m_pClient->GetEPGNowInfo(number, result);
}

PVR_ERROR CPVRClient::GetEPGNextInfo(const unsigned int number, PVR_PROGINFO *result)
{
  return PVR_ERROR_SERVER_ERROR;//m_pClient->GetEPGNextInfo(number, result);
}

PVR_ERROR CPVRClient::GetEPGDataEnd(time_t end)
{
  return PVR_ERROR_NO_ERROR;
}


/**********************************************************
 * Recordings PVR Functions
 */

int CPVRClient::GetNumRecordings()
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetNumRecordings();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumRecordings occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  return -1;
}


/**********************************************************
 * Timers PVR Functions
 */

int CPVRClient::GetNumTimers()
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->GetNumTimers();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetNumTimers occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  return -1;
}

PVR_ERROR CPVRClient::GetTimers(VECTVTIMERS &timers)
{
  //PVR_ERROR err =
  return PVR_ERROR_NO_ERROR;

}

PVR_ERROR CPVRClient::AddTimer(const CTVTimerInfoTag &timerinfo)
{
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClient::RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)
{
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClient::UpdateTimer(const CTVTimerInfoTag &timerinfo)
{
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClient::DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force /* = false */)
{
  return PVR_ERROR_NO_ERROR;
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
