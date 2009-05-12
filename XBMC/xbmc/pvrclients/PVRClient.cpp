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
#include "../../addons/PVRClientTypes.h"
#include "../utils/log.h"

using namespace std;
using namespace ADDON;

/**********************************************************
 * CPVRClient Class constructor/destructor
 */

CPVRClient::CPVRClient(long clientID, struct PVRClient* pClient, DllPVRClient* pDll,
                       const ADDON::CAddon& addon, ADDON::IAddonCallback* addonCB, IPVRClientCallback* pvrCB)
                              : IPVRClient(clientID, addon, addonCB, pvrCB)
                              , m_clientID(clientID)
                              , m_pClient(pClient)
                              , m_pDll(pDll)
                              , m_manager(pvrCB)
                              , m_ReadyToUse(false)
                              , m_hostName("unknown")
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
  m_callbacks = new PVRCallbacks;

  /* PVR Helper functions */
  m_callbacks->userData                 = this;
  m_callbacks->Event                    = PVREventCallback;

  /* AddOn Helper functions */
  m_callbacks->AddOn.ReportStatus       = AddOnStatusCallback;
  m_callbacks->AddOn.Log                = AddOnLogCallback;
  m_callbacks->AddOn.GetSetting         = AddOnGetSetting;
  m_callbacks->AddOn.OpenSettings       = AddOnOpenSettings;
  m_callbacks->AddOn.OpenOwnSettings    = AddOnOpenOwnSettings;
  m_callbacks->AddOn.GetAddonDirectory  = AddOnGetAddonDirectory;
  m_callbacks->AddOn.GetUserDirectory   = AddOnGetUserDirectory;

  /* GUI Dialog Helper functions */
  m_callbacks->Dialog.OpenOK            = CAddon::OpenDialogOK;
  m_callbacks->Dialog.OpenYesNo         = CAddon::OpenDialogYesNo;
  m_callbacks->Dialog.OpenBrowse        = CAddon::OpenDialogBrowse;
  m_callbacks->Dialog.OpenNumeric       = CAddon::OpenDialogNumeric;
  m_callbacks->Dialog.OpenKeyboard      = CAddon::OpenDialogKeyboard;
  m_callbacks->Dialog.OpenSelect        = CAddon::OpenDialogSelect;
  m_callbacks->Dialog.ProgressCreate    = CAddon::ProgressDialogCreate;
  m_callbacks->Dialog.ProgressUpdate    = CAddon::ProgressDialogUpdate;
  m_callbacks->Dialog.ProgressIsCanceled= CAddon::ProgressDialogIsCanceled;
  m_callbacks->Dialog.ProgressClose     = CAddon::ProgressDialogClose;

  /* Utilities Helper functions */
  m_callbacks->Utils.Shutdown           = CAddon::Shutdown;
  m_callbacks->Utils.Restart            = CAddon::Restart;
  m_callbacks->Utils.Dashboard          = CAddon::Dashboard;
  m_callbacks->Utils.ExecuteScript      = CAddon::ExecuteScript;
  m_callbacks->Utils.ExecuteBuiltIn     = CAddon::ExecuteBuiltIn;
  m_callbacks->Utils.ExecuteHttpApi     = CAddon::ExecuteHttpApi;
  m_callbacks->Utils.UnknownToUTF8      = CAddon::UnknownToUTF8;
  m_callbacks->Utils.LocalizedString    = AddOnGetLocalizedString;
  m_callbacks->Utils.GetSkinDir         = CAddon::GetSkinDir;
  m_callbacks->Utils.GetLanguage        = CAddon::GetLanguage;
  m_callbacks->Utils.GetIPAddress       = CAddon::GetIPAddress;
  m_callbacks->Utils.GetDVDState        = CAddon::GetDVDState;
  m_callbacks->Utils.GetInfoLabel       = CAddon::GetInfoLabel;
  m_callbacks->Utils.GetInfoImage       = CAddon::GetInfoImage;
  m_callbacks->Utils.GetFreeMem         = CAddon::GetFreeMem;
  m_callbacks->Utils.GetCondVisibility  = CAddon::GetCondVisibility;
  m_callbacks->Utils.EnableNavSounds    = CAddon::EnableNavSounds;
  m_callbacks->Utils.PlaySFX            = CAddon::PlaySFX;
  m_callbacks->Utils.GetSupportedMedia  = CAddon::GetSupportedMedia;
  m_callbacks->Utils.GetGlobalIdleTime  = CAddon::GetGlobalIdleTime;
  m_callbacks->Utils.GetCacheThumbName  = CAddon::GetCacheThumbName;
  m_callbacks->Utils.MakeLegalFilename  = CAddon::MakeLegalFilename;
  m_callbacks->Utils.TranslatePath      = AddOnTranslatePath;
  m_callbacks->Utils.GetRegion          = CAddon::GetRegion;
  m_callbacks->Utils.SkinHasImage       = CAddon::SkinHasImage;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  try
  {
    ADDON_STATUS status = m_pClient->Create(m_callbacks);
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

// Addon specific functions //////////////////////////////////////////////////
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

  if (status != STATUS_OK)
  {
    CStdString message;
    if (msg != NULL)
      message = msg;

    /* Delete is performed by the calling class */
    new CAddonStatusHandler(client, status, message, false);
  }
}

void CPVRClient::AddOnLogCallback(void *userData, const ADDON_LOG loglevel, const char *format, ... )
{
  CPVRClient* client = (CPVRClient*) userData;
  if (!client)
    return;

  try
  {
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
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s/%s - exception '%s' during AddOnLogCallback occurred, contact Developer '%s' of this AddOn", client->m_strName.c_str(), client->m_hostName.c_str(), e.what(), client->m_strCreator.c_str());
    return;
  }
}

bool CPVRClient::AddOnGetSetting(void *userData, const char *settingName, void *settingValue)
{
  CPVRClient* client=(CPVRClient*) userData;
  if (!client)
    return NULL;

  return CAddon::GetAddonSetting(client, settingName, settingValue);
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

const char* CPVRClient::AddOnGetAddonDirectory(void *userData)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (!client)
    return "";

  static CStdString retString = CAddon::GetAddonDirectory(client);
  return retString.c_str();
}

const char* CPVRClient::AddOnGetUserDirectory(void *userData)
{
  CPVRClient* client = (CPVRClient*) userData;
  if (!client)
    return "";

  static CStdString retString = CAddon::GetUserDirectory(client);
  return retString.c_str();
}

const char* CPVRClient::AddOnTranslatePath(const char *path)
{
  static CStdString retString = CAddon::TranslatePath(path);
  return retString.c_str();
}
