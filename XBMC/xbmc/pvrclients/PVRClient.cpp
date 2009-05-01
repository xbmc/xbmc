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
#include "stdafx.h"
#include <vector>

#include "PVRClient.h"
#include "URL.h"
#include "../../addons/PVRClientTypes.h"
#include "../utils/log.h"

CPVRClient::CPVRClient(long clientID, struct PVRClient* pClient, DllPVRClient* pDll,
                       const ADDON::CAddon& addon, ADDON::IAddonCallback* addonCB, IPVRClientCallback* pvrCB)
                              : IPVRClient(clientID, addon, addonCB, pvrCB)
                              , m_clientID(clientID)
                              , m_pClient(pClient)
                              , m_pDll(pDll)
                              , m_manager(pvrCB)
{
  InitializeCriticalSection(&m_critSection);
}

CPVRClient::~CPVRClient()
{
  // tell the plugin to disconnect and prepare for destruction
  Disconnect();
  /*DeInit();*/
  DeleteCriticalSection(&m_critSection);
}

bool CPVRClient::Init()
{
  PVRCallbacks *callbacks               = new PVRCallbacks;
  callbacks->userData                   = this;
  callbacks->Event                      = PVREventCallback;
  callbacks->AddOn.ReportStatus         = AddOnStatusCallback;
  callbacks->AddOn.Log                  = AddOnLogCallback;
  callbacks->AddOn.OpenSettings         = AddOnOpenSettings;
  callbacks->AddOn.OpenOwnSettings      = AddOnOpenOwnSettings;

  callbacks->Dialog.OpenOK              = CAddon::OpenDialogOK;
  callbacks->Dialog.OpenYesNo           = CAddon::OpenDialogYesNo;
  callbacks->Dialog.OpenBrowse          = CAddon::OpenDialogBrowse;
  callbacks->Dialog.OpenNumeric         = CAddon::OpenDialogNumeric;
  callbacks->Dialog.OpenSelect          = CAddon::OpenDialogSelect;
  callbacks->Dialog.ProgressCreate      = CAddon::ProgressDialogCreate;
  callbacks->Dialog.ProgressUpdate      = CAddon::ProgressDialogUpdate;
  callbacks->Dialog.ProgressIsCanceled  = CAddon::ProgressDialogIsCanceled;
  callbacks->Dialog.ProgressClose       = CAddon::ProgressDialogClose;

  callbacks->GUI.Lock                   = CAddon::GUILock;
  callbacks->GUI.Unlock                 = CAddon::GUIUnlock;
  callbacks->GUI.GetCurrentWindowId     = CAddon::GUIGetCurrentWindowId;
  callbacks->GUI.GetCurrentWindowDialogId   = CAddon::GUIGetCurrentWindowDialogId;
      
  callbacks->Utils.Shutdown             = CAddon::Shutdown;
  callbacks->Utils.Restart              = CAddon::Restart;
  callbacks->Utils.Dashboard            = CAddon::Dashboard;
  callbacks->Utils.ExecuteScript        = CAddon::ExecuteScript;
  callbacks->Utils.ExecuteBuiltIn       = CAddon::ExecuteBuiltIn;
  callbacks->Utils.ExecuteHttpApi       = CAddon::ExecuteHttpApi;
  callbacks->Utils.UnknownToUTF8        = CAddon::UnknownToUTF8;
  callbacks->Utils.LocalizedString      = AddOnGetLocalizedString;
  callbacks->Utils.GetSkinDir           = CAddon::GetSkinDir;
  callbacks->Utils.GetLanguage          = CAddon::GetLanguage;
  callbacks->Utils.GetIPAddress         = CAddon::GetIPAddress;
  callbacks->Utils.GetDVDState          = CAddon::GetDVDState;
  callbacks->Utils.GetInfoLabel         = CAddon::GetInfoLabel;
  callbacks->Utils.GetInfoImage         = CAddon::GetInfoImage;
  callbacks->Utils.GetFreeMem           = CAddon::GetFreeMem;
  callbacks->Utils.GetCondVisibility    = CAddon::GetCondVisibility;
  callbacks->Utils.EnableNavSounds      = CAddon::EnableNavSounds;
  callbacks->Utils.PlaySFX              = CAddon::PlaySFX;
  callbacks->Utils.GetSupportedMedia    = CAddon::GetSupportedMedia;
  callbacks->Utils.GetGlobalIdleTime    = CAddon::GetGlobalIdleTime;
  callbacks->Utils.GetCacheThumbName    = CAddon::GetCacheThumbName;
  callbacks->Utils.MakeLegalFilename    = CAddon::MakeLegalFilename;
  callbacks->Utils.TranslatePath        = CAddon::TranslatePath;
  callbacks->Utils.GetRegion            = CAddon::GetRegion;
  callbacks->Utils.SkinHasImage         = CAddon::SkinHasImage;

  m_pClient->Create(callbacks);
  
  return true;
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
  catch (std::exception e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception from GetProperties", m_strName.c_str(), m_hostName.c_str());
    return PVR_ERROR_UNKOWN;
  }
}

PVR_ERROR CPVRClient::Connect()
{
  CLog::Log(LOGDEBUG, "PVR: %s - connecting", m_strName.c_str());
  return m_pClient->Connect();
}

void CPVRClient::Disconnect()
{
  m_pClient->Disconnect();
}

bool CPVRClient::IsUp()
{
  return m_pClient->IsUp();
}

const std::string CPVRClient::GetBackendName()
{
  return m_pClient->GetBackendName();
}

const std::string CPVRClient::GetBackendVersion()
{
  return m_pClient->GetBackendVersion();
}

const std::string CPVRClient::GetConnectionString()
{
  return m_pClient->GetConnectionString();
}

PVR_ERROR CPVRClient::GetDriveSpace(long long *total, long long *used)
{
  return m_pClient->GetDriveSpace(total, used);
}

int CPVRClient::GetNumBouquets()
{
  return 0;
}

PVR_ERROR CPVRClient::GetBouquetInfo(const unsigned int number, PVR_BOUQUET& info)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int CPVRClient::GetNumChannels()
{
  return 0;//m_pClient->GetNumChannels();
}

PVR_ERROR CPVRClient::GetChannelList(VECCHANNELS &channels)
{
  // all memory allocation takes place in client, 
  // as we don't know beforehand how many channels exist
//  unsigned int num;
//  PVR_CHANNEL **clientChans = NULL;
//
//  num = m_pClient->GetChannelList(&clientChans);
//  if (num < 1)
//    return PVR_ERROR_SERVER_ERROR;
//
//  // after client returns, we take ownership over the allocated memory
//  if (!ConvertChannels(num, clientChans, channels))
//    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

bool CPVRClient::ConvertChannels(unsigned int num, PVR_CHANNEL **clientChans, VECCHANNELS &xbmcChans)
{
  xbmcChans.clear();
  PVR_CHANNEL *chan;

  for (unsigned i = 0; i < num; i++)
  {
    // don't trust the client to have correctly initialized memory
    try 
    {
      chan = clientChans[i];
    }
    catch (std::exception e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s - unitialized PVR_CHANNEL", m_strName.c_str(), m_hostName.c_str());
      ReleaseClientData(num, clientChans);
      return false;
    }
    if (chan->number < 1 || chan->name == "" || chan->hide == true)
      continue;

    CTVChannelInfoTag channel;
    channel.m_clientID   = m_clientID;
    channel.m_iClientNum = chan->number;
    channel.m_IconPath   = chan->iconpath;
    channel.m_strChannel = chan->name;
    channel.m_radio      = chan->radio;
    channel.m_encrypted  = chan->encrypted;

    xbmcChans.push_back(channel);
  }

  /*ReleaseClientData(num, clientChans);*/

  return true;
}

void CPVRClient::ReleaseClientData(unsigned int num, PVR_CHANNEL **clientChans)
{
  PVR_CHANNEL *chan;

  for (unsigned i = 0; i < num; i++)
  {
    // don't trust the client to have correctly initialized memory
    try 
    {
      chan = &(*clientChans)[i];
    }
    catch (std::exception e)
    {
      CLog::Log(LOGERROR, "PVR: %s/%s: uninitialized PVR_CHANNEL", m_strName.c_str(), m_hostName.c_str());
      clientChans = NULL;
      continue;
    }
    free(chan);
    chan = NULL;
  }
}

// EPG ///////////////////////////////////////////////////////////////////////
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

// Timers ////////////////////////////////////////////////////////////////////
const unsigned int CPVRClient::GetNumTimers()
{
  return 0;
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

// Addon specific functions //////////////////////////////////////////////////
void CPVRClient::Remove()
{
  /*m_pClient->*/

}

bool CPVRClient::SetSetting(const char *settingName, const void *settingValue)
{
  return m_pDll->SetSetting(settingName, settingValue);
}

void CPVRClient::GetSettings(std::vector<DllSetting> **vecSettings)
{
  /*if (vecSettings) *vecSettings = NULL;
  if (m_pClient->GetSettings)
    m_pClient->GetSettings(vecSettings);*/
}

void CPVRClient::UpdateSetting(int num)
{
  /*if (m_pClient->UpdateSetting)
    m_pClient->UpdateSetting(num);*/
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


/**********************************************************
 * Addon specific Callbacks
 * Is a must do, to all types of available addons handler
 */

void CPVRClient::AddOnStatusCallback(void *userData, const ADDON_STATUS status, const char* msg)
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return;

  CLog::Log(LOGINFO, "PVR: %s/%s: Reported bad status: %i", client->m_strName.c_str(), client->m_hostName.c_str(), status);

  switch (status)
  {
     case STATUS_DATA_UPDATE:         /* Data structures handled by the AddOn are changed */
       CLog::Log(LOGINFO, "PVR: %s/%s: Data reloading", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_NEED_RESTART:       /* Request to restart the AddOn and data structures need updated */
       CLog::Log(LOGINFO, "PVR: %s/%s: Requesting AddOn Restart and data reloading", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_NEED_EMER_RESTART:  /* Request to restart XBMC (hope no AddOn need or do this) */
       CLog::Log(LOGERROR, "PVR: %s/%s: PANIC!!! Requesting XBMC restart", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_MISSING_SETTINGS:   /* Some required settings are missing */
       CLog::Log(LOGERROR, "PVR: %s/%s: Some required settings are missing", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_BAD_SETTINGS:       /* A setting value is invalid */
       CLog::Log(LOGERROR, "PVR: %s/%s: One setting is wrong", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_WRONG_HOST:         /* AddOn want to connect to unknown host (for ones that use Network) */
       CLog::Log(LOGERROR, "PVR: %s/%s: Wan't to connect to invalid host", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_INVALID_USER:       /* Invalid or unknown user */
       CLog::Log(LOGERROR, "PVR: %s/%s: Unknown user name", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_WRONG_PASS:         /* Invalid or wrong password */
       CLog::Log(LOGERROR, "PVR: %s/%s: Password is not accepted", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_MISSING_DATA:       /* Some AddOn data is missing (check log's for missing data) */
       CLog::Log(LOGERROR, "PVR: %s/%s: Some AddOn data is missing", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_MISSING_FILE:       /* A AddOn file is missing (check log's for missing data) */
       CLog::Log(LOGERROR, "PVR: %s/%s: A AddOn file is missing", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_OUTDATED:           /* Some data is outdated */
       CLog::Log(LOGERROR, "PVR: %s/%s: The AddOn is outdated and need to be updated", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_UNKNOWN:            /* A unknown event is occurred */
       CLog::Log(LOGERROR, "PVR: %s/%s: A Unknown Error is occoured", client->m_strName.c_str(), client->m_hostName.c_str());
       return;
     case STATUS_OK:                 /* Normally not returned (everything is ok) */
     default:
       return;
  }
}

void CPVRClient::AddOnLogCallback(void *userData, const ADDON_LOG loglevel, const char *format, ... )
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return;

  CStdString clientMsg, xbmcMsg;
  clientMsg.reserve(16384);

  va_list va;
  va_start(va, format);
  clientMsg.FormatV(format, va);
  va_end(va);

  /* insert internal identifiers for brevity */
  xbmcMsg.Format("PVR: %s/%s: ", client->m_strName, client->m_hostName);
  xbmcMsg += clientMsg;

  int xbmclog;
  switch (loglevel)
  {
    case LOG_ERROR:
      xbmclog = LOGERROR;
      break;
    case LOG_INFO:
      xbmclog = LOGINFO;
      break;
    case LOG_DEBUG:
    default:
      xbmclog = LOGDEBUG;
      break;
  }

  /* finally write the logmessage */
  CLog::Log(xbmclog, xbmcMsg);
}

void CPVRClient::AddOnOpenSettings(const char *url, bool bReload)
{
  CURL cUrl(url);
  CAddon::OpenAddonSettings(cUrl, bReload);
}

void CPVRClient::AddOnOpenOwnSettings(void *userData, bool bReload)
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return;

  CAddon::OpenAddonSettings(client, bReload);
}

const char* CPVRClient::AddOnGetLocalizedString(void *userData, long dwCode)
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return "";

  return CAddon::GetLocalizedString(client, dwCode);
}
