/*
 *      Copyright (C) 2010-2011 Marcel Groothuis, Fred Hoogduin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "pvrclient-fortherecord.h"
#include "utils.h"
#include "uri.h"

using namespace std;
using namespace ADDON;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_szHostname           = DEFAULT_HOST;         ///< The Host name or IP of the ForTheRecord Argus tuner/recorder
int         g_iPort                = DEFAULT_PORT;         ///< The TVServerXBMC listening port (default: 49943)
int         g_iConnectTimeout      = DEFAULT_TIMEOUT;      ///< The Socket connection timeout
bool        g_bRadioEnabled        = DEFAULT_RADIO;        ///< Send also Radio channels list to XBMC
                                                           ///< ForTheRecord uses shares to communicate with clients 
std::string g_szUser               = DEFAULT_USER;         ///< Windows user account used to access share
std::string g_szPass               = DEFAULT_PASS;         ///< Windows user password used to access share
                                                           ///< Leave empty to use current user when running on Windows

std::string  g_szBaseURL;

//
///* Client member variables */
ADDON_STATUS            m_CurStatus    = ADDON_STATUS_UNKNOWN;
cPVRClientForTheRecord *g_client       = NULL;
bool                    g_bCreated     = false;
int                     g_iClientID    = -1;
std::string             g_szUserPath   = "";
std::string             g_szClientPath = "";
CHelper_libXBMC_addon  *XBMC           = NULL;
CHelper_libXBMC_pvr    *PVR            = NULL;

extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

//-- Create -------------------------------------------------------------------
// Called after loading of the dll, all steps to become Client functional
// must be performed here.
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_UNKNOWN;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_UNKNOWN;
  }

#ifdef TSREADER
  XBMC->Log(LOG_INFO, "Creating the ForTheRecord PVR-client (TsReader version)");
#else
  XBMC->Log(LOG_INFO, "Creating the ForTheRecord PVR-client (ffmpeg rtsp version)");
#endif

  m_CurStatus    = ADDON_STATUS_UNKNOWN;
  g_client       = new cPVRClientForTheRecord();
  g_iClientID    = pvrprops->iClientId;
  g_szUserPath   = pvrprops->strUserPath;
  g_szClientPath = pvrprops->strClientPath;

  /* Read ForTheRecord PVR client settings */
  //  See also addons/pvr.fortherecord.argus/resources/settings.xml
  //  and addons/pvr.fortherecord.argus/resources/language/.../strings.xml

  /* Read setting "host" from settings.xml */
  char buffer[1024];

  if (XBMC->GetSetting("host", &buffer))
  {
    g_szHostname = buffer;
    uri::decode(g_szHostname);
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    g_szHostname = DEFAULT_HOST;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '49943' as default");
    g_iPort = DEFAULT_PORT;
  }

  /* Read setting "useradio" from settings.xml */
  if (!XBMC->GetSetting("useradio", &g_bRadioEnabled))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'useradio' setting, falling back to 'true' as default");
    g_bRadioEnabled = DEFAULT_RADIO;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC->GetSetting("timeout", &g_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    g_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* read setting "user" from settings.xml */
  if (XBMC->GetSetting("user", buffer))
    g_szUser = buffer;
  else
    g_szUser = "";
  buffer[0] = 0; /* Set the end of string */

  /* read setting "pass" from settings.xml */
  if (XBMC->GetSetting("pass", buffer))
    g_szPass = buffer;
  else
    g_szPass = "";

  /* Connect to ForTheRecord */
  if (!g_client->Connect())
  {
    SAFE_DELETE(g_client);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
  }
  else
  {
    m_CurStatus = ADDON_STATUS_OK;
  }

  g_bCreated = true;

  return m_CurStatus;
}

//-- Destroy ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  if ((g_bCreated) && (g_client))
  {
    g_client->Disconnect();
    SAFE_DELETE(g_client);

    g_bCreated = false;
  }

  if (PVR)
  {
    SAFE_DELETE(PVR);
  }
  if (XBMC)
  {
    SAFE_DELETE(XBMC);
  }

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

//-- GetStatus ----------------------------------------------------------------
// Report the current Add-On Status to XBMC
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

//-- HasSettings --------------------------------------------------------------
// Report "true", yes this AddOn have settings
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- SetSetting ---------------------------------------------------------------
// Called everytime a setting is changed by the user and to inform AddOn about
// new setting and to do required stuff to apply it.
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;

  // SetSetting can occur when the addon is enabled, but TV support still
  // disabled. In that case the addon is not loaded, so we should not try
  // to change its settings.
  if (!g_bCreated)
    return ADDON_STATUS_OK;

  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_szHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*) settingValue;
    if (tmp_sHostname != g_szHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iPort, *(int*) settingValue);
    if (g_iPort != *(int*) settingValue)
    {
      g_iPort = *(int*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "useradio")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'useradio' from %u to %u", g_bRadioEnabled, *(bool*) settingValue);
    g_bRadioEnabled = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'timeout' from %u to %u", g_iConnectTimeout, *(int*) settingValue);
    g_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "user")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'user' from %s to %s", g_szUser.c_str(), (const char*) settingValue);
    g_szUser = (const char*) settingValue;
  }
  else if (str == "pass")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'pass' from %s to %s", g_szPass.c_str(), (const char*) settingValue);
    g_szPass = (const char*) settingValue;
  }

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
  ADDON_Destroy();
}

void ADDON_FreeSettings()
{

}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

//-- GetAddonCapabilities -----------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities)
{
  XBMC->Log(LOG_DEBUG, "->GetProperties()");

  pCapabilities->bSupportsTimeshift          = false;
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = g_bRadioEnabled;
  pCapabilities->bSupportsChannelSettings    = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = false;
  pCapabilities->bSupportsChannelScan        = false;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

//-- GetBackendName -----------------------------------------------------------
// Return the Name of the Backend
//-----------------------------------------------------------------------------
const char * GetBackendName(void)
{
  return g_client->GetBackendName();
}

//-- GetBackendVersion --------------------------------------------------------
// Return the Version of the Backend as String
//-----------------------------------------------------------------------------
const char * GetBackendVersion(void)
{
  return g_client->GetBackendVersion();
}

//-- GetConnectionString ------------------------------------------------------
// Return a String with connection info, if available
//-----------------------------------------------------------------------------
const char * GetConnectionString(void)
{
  return g_client->GetConnectionString();
}

//-- GetDriveSpace ------------------------------------------------------------
// Return the Total and Free Drive space on the PVR Backend
//-----------------------------------------------------------------------------
PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return g_client->GetDriveSpace(iTotal, iUsed);
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  return g_client->GetBackendTime(localTime, gmtOffset);
}

PVR_ERROR DialogChannelScan()
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  return g_client->GetEpg(handle, channel, iStart, iEnd);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount()
{
  return g_client->GetNumChannels();
}

PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio)
{
  return g_client->GetChannels(handle, bRadio);
}

PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channelinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channelinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR Channel group Functions           **/

int GetChannelGroupsAmount(void)
{
  return g_client->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  return g_client->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  return g_client->GetChannelGroupMembers(handle, group);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  return g_client->GetNumRecordings();
}

PVR_ERROR GetRecordings(PVR_HANDLE handle)
{
  return g_client->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  return g_client->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  return g_client->RenameRecording(recording);
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  return g_client->GetNumTimers();
}

PVR_ERROR GetTimers(PVR_HANDLE handle)
{
  return g_client->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  return g_client->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  return g_client->DeleteTimer(timer, bForceDelete);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  return g_client->UpdateTimer(timer);
}


/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  return g_client->OpenLiveStream(channelinfo);
}

void CloseLiveStream()
{
  return g_client->CloseLiveStream();
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return g_client->ReadLiveStream(pBuffer, iBufferSize);
}

int GetCurrentClientChannel()
{
  return g_client->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  return g_client->SwitchChannel(channelinfo);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  return g_client->SignalStatus(signalStatus);
}

/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  return g_client->OpenRecordedStream(recording);
}

void CloseRecordedStream(void)
{
  return g_client->CloseRecordedStream();
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return g_client->ReadRecordedStream(pBuffer, iBufferSize);
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channel)
{
  return g_client->GetLiveStreamURL(channel);
}

/** UNUSED API FUNCTIONS */
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
DemuxPacket* DemuxRead(void) { return NULL; }
void DemuxAbort(void) {}
void DemuxReset(void) {}
void DemuxFlush(void) {}

long long SeekRecordedStream(long long iPosition, int iWhence) { return -1; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return -1; }

long long SeekLiveStream(long long pos, int whence) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1 ; }

} //end extern "C"
