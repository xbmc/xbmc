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
#include "PVRManager.h"
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
  /* Unload library file */
  m_pDll->Unload();
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
  m_callbacks->PVR.TransferChannelEntry = PVRTransferChannelEntry;
  m_callbacks->PVR.TransferTimerEntry   = PVRTransferTimerEntry;

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
    m_pDll->Destroy();

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
  return m_pClient->GetEPGNowInfo(number, *result);
}

PVR_ERROR CPVRClient::GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)
{
  return m_pClient->GetEPGNextInfo(number, *result);
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

PVR_ERROR CPVRClient::GetChannelList(VECCHANNELS &channels, bool radio)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      const PVRHANDLE handle = (VECCHANNELS*) &channels;
      ret = m_pClient->RequestChannelList(handle, radio);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      for (unsigned int i = 0; i < channels.size(); i++)
      {
        channels[i].m_clientID = m_clientID;
      }
      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetChannelList occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetChannelList", m_strName.c_str(), m_hostName.c_str(), ret);
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

  VECCHANNELS *xbmcChannels = (VECCHANNELS*) handle;
  CTVChannelInfoTag tag;

  tag.m_iIdChannel          = -1;
  tag.m_iChannelNum         = -1;
  tag.m_iClientNum          = channel->number;
  tag.m_iGroupID            = 0;
  tag.m_clientID            = client->m_clientID;
  tag.m_strChannel          = channel->name;
  //= channel->callsign;
  tag.m_IconPath            = channel->iconpath;
  tag.m_encrypted           = channel->encrypted;
  tag.m_bTeletext           = channel->teletext;
  tag.m_radio               = channel->radio;
  tag.m_hide                = channel->hide;
  tag.m_isRecording         = channel->recording;
  tag.m_strFileNameAndPath  = channel->stream_url;
    
  xbmcChannels->push_back(tag);
  return;
}

PVR_ERROR CPVRClient::GetChannelSettings(CTVChannelInfoTag *result)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pClient->GetChannelSettings(result);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetChannelSettings occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetChannelSettings", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pClient->UpdateChannelSettings(chaninfo);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during UpdateChannelSettings occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after UpdateChannelSettings", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::AddChannel(const CTVChannelInfoTag &info)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pClient->AddChannel(info);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during AddChannel occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after AddChannel", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteChannel(unsigned int number)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pClient->DeleteChannel(number);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during DeleteChannel occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after DeleteChannel", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::RenameChannel(unsigned int number, CStdString &newname)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pClient->RenameChannel(number, newname);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during RenameChannel occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after RenameChannel", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::MoveChannel(unsigned int number, unsigned int newnumber)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
//      ret = m_pClient->MoveChannel(number, newnumber);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during MoveChannel occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after MoveChannel", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}


/**********************************************************
 * Recordings PVR Functions
 */

int CPVRClient::GetNumRecordings(void)
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

PVR_ERROR CPVRClient::GetAllRecordings(VECRECORDINGS *results)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      ret = m_pClient->GetAllRecordings(results);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      for (unsigned int i = 0; i < results->size(); i++)
      {
        results->at(i).m_clientID = m_clientID;
      }
      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetAllRecordings occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetAllRecordings", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteRecording(const CTVRecordingInfoTag &recinfo)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      ret = m_pClient->DeleteRecording(recinfo);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during DeleteRecording occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after DeleteRecording", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      ret = m_pClient->RenameRecording(recinfo, newname);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during RenameRecording occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after RenameRecording", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}


/**********************************************************
 * Timers PVR Functions
 */

int CPVRClient::GetNumTimers(void)
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

PVR_ERROR CPVRClient::GetAllTimers(cPVRTimers *results)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      const PVRHANDLE handle = (cPVRTimers*) results;
      ret = m_pClient->RequestTimerList(handle);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during GetAllTimers occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after GetAllTimers", m_strName.c_str(), m_hostName.c_str(), ret);
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
  CTVChannelInfoTag *channel  = CPVRManager::GetInstance()->GetChannelByClientNumber(timer->channelNum, client->m_clientID);
  
  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with not present channel");
    return;
  }

  cPVRTimerInfoTag tag;
  tag.m_clientID            = client->m_clientID;
  tag.m_clientIndex         = timer->index;
  tag.m_Active              = timer->active;
  tag.m_strTitle            = timer->title;
  tag.m_clientNum           = timer->channelNum;
  tag.m_StartTime           = (time_t) timer->starttime;
  tag.m_StopTime            = (time_t) timer->endtime;
  tag.m_FirstDay            = (time_t) timer->firstday;
  tag.m_Priority            = timer->priority;
  tag.m_Lifetime            = timer->lifetime;
  tag.m_recStatus           = timer->recording;
  tag.m_Repeat              = timer->repeat;
  tag.m_Weekdays            = timer->repeatflags;
  tag.m_channelNum          = channel->m_iChannelNum;
  tag.m_Radio               = channel->m_radio;
  tag.m_strFileNameAndPath.Format("pvr://client%i/timers/%i", tag.m_clientID, tag.m_clientIndex);

  if (!tag.m_Repeat)
  {
    tag.m_Summary.Format("%s %s %s %s %s", tag.m_StartTime.GetAsLocalizedDate()
                         , g_localizeStrings.Get(18078)
                         , tag.m_StartTime.GetAsLocalizedTime("", false)
                         , g_localizeStrings.Get(18079)
                         , tag.m_StopTime.GetAsLocalizedTime("", false));
  }
  else if (tag.m_FirstDay != NULL)
  {
    tag.m_Summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s"
                         , tag.m_Weekdays & 0x01 ? g_localizeStrings.Get(18080) : "__"
                         , tag.m_Weekdays & 0x02 ? g_localizeStrings.Get(18081) : "__"
                         , tag.m_Weekdays & 0x04 ? g_localizeStrings.Get(18082) : "__"
                         , tag.m_Weekdays & 0x08 ? g_localizeStrings.Get(18083) : "__"
                         , tag.m_Weekdays & 0x10 ? g_localizeStrings.Get(18084) : "__"
                         , tag.m_Weekdays & 0x20 ? g_localizeStrings.Get(18085) : "__"
                         , tag.m_Weekdays & 0x40 ? g_localizeStrings.Get(18086) : "__"
                         , g_localizeStrings.Get(18087)
                         , tag.m_FirstDay.GetAsLocalizedDate(false)
                         , g_localizeStrings.Get(18078)
                         , tag.m_StartTime.GetAsLocalizedTime("", false)
                         , g_localizeStrings.Get(18079)
                         , tag.m_StopTime.GetAsLocalizedTime("", false));
  }
  else
  {
    tag.m_Summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s"
                         , tag.m_Weekdays & 0x01 ? g_localizeStrings.Get(18080) : "__"
                         , tag.m_Weekdays & 0x02 ? g_localizeStrings.Get(18081) : "__"
                         , tag.m_Weekdays & 0x04 ? g_localizeStrings.Get(18082) : "__"
                         , tag.m_Weekdays & 0x08 ? g_localizeStrings.Get(18083) : "__"
                         , tag.m_Weekdays & 0x10 ? g_localizeStrings.Get(18084) : "__"
                         , tag.m_Weekdays & 0x20 ? g_localizeStrings.Get(18085) : "__"
                         , tag.m_Weekdays & 0x40 ? g_localizeStrings.Get(18086) : "__"
                         , g_localizeStrings.Get(18078)
                         , tag.m_StartTime.GetAsLocalizedTime("", false)
                         , g_localizeStrings.Get(18079)
                         , tag.m_StopTime.GetAsLocalizedTime("", false));
  }

  xbmcTimers->push_back(tag);
  return;
}

PVR_ERROR CPVRClient::AddTimer(const cPVRTimerInfoTag &timerinfo)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      ret = m_pClient->AddTimer(tag);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during AddTimer occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after AddTimer", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::DeleteTimer(const cPVRTimerInfoTag &timerinfo, bool force)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      ret = m_pClient->DeleteTimer(tag, force);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during DeleteTimer occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after DeleteTimer", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::RenameTimer(const cPVRTimerInfoTag &timerinfo, CStdString &newname)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);
      
      ret = m_pClient->RenameTimer(tag, newname.c_str());
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during RenameTimer occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after RenameTimer", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

PVR_ERROR CPVRClient::UpdateTimer(const cPVRTimerInfoTag &timerinfo)
{
  PVR_ERROR ret = PVR_ERROR_UNKOWN;

  if (m_ReadyToUse)
  {
    try
    {
      PVR_TIMERINFO tag;
      WriteClientTimerInfo(timerinfo, tag);

      ret = m_pClient->UpdateTimer(tag);
      if (ret != PVR_ERROR_NO_ERROR)
        throw ret;

      return PVR_ERROR_NO_ERROR;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during UpdateTimer occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
    catch (PVR_ERROR ret)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - Client returns bad error (%i) after UpdateTimer", m_strName.c_str(), m_hostName.c_str(), ret);
    }
  }
  return ret;
}

void CPVRClient::WriteClientTimerInfo(const cPVRTimerInfoTag &timerinfo, PVR_TIMERINFO &tag)
{
  tag.index = timerinfo.m_clientIndex;
  tag.active = timerinfo.m_Active;
  tag.channelNum = timerinfo.m_clientNum;
  tag.recording = timerinfo.m_recStatus;
  tag.title = timerinfo.m_strTitle;
  tag.priority = timerinfo.m_Priority;
  tag.lifetime = timerinfo.m_Lifetime;
  tag.repeat = timerinfo.m_Repeat;
  tag.repeatflags = timerinfo.m_Weekdays;

  timerinfo.m_StartTime.GetAsTime(tag.starttime);
  timerinfo.m_StopTime.GetAsTime(tag.endtime);
  timerinfo.m_FirstDay.GetAsTime(tag.firstday);
  return;
}

/**********************************************************
 * Stream PVR Functions
 */

bool CPVRClient::OpenLiveStream(unsigned int channel)
{
  if (m_ReadyToUse)
  {
    try
    {
      return m_pClient->OpenLiveStream(channel);
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during OpenLiveStream occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  return false;
}

void CPVRClient::CloseLiveStream()
{
  if (m_ReadyToUse)
  {
    try
    {
      m_pClient->CloseLiveStream();
      return;
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during CloseLiveStream occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), m_hostName.c_str(), e.what(), m_strCreator.c_str());
    }
  }
  return;
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

bool CPVRClient::TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage)
{
  return m_pClient->TeletextPagePresent(channel, Page, subPage);
}

bool CPVRClient::ReadTeletextPage(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage)
{
  return m_pClient->ReadTeletextPage(buf, channel, Page, subPage);
}

/**********************************************************
 * Addon specific functions
 * Are used for every type of AddOn
 */

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
