/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "HTSPData.h"
#include "HTSPDemux.h"

using namespace std;

bool         m_bCreated  = false;
ADDON_STATUS m_CurStatus = ADDON_STATUS_UNKNOWN;
int          g_iClientId = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strHostname             = DEFAULT_HOST;
int         g_iPortHTSP               = DEFAULT_HTSP_PORT;
int         g_iPortHTTP               = DEFAULT_HTTP_PORT;
int         g_iConnectTimeout         = DEFAULT_CONNECT_TIMEOUT;
int         g_iResponseTimeout        = DEFAULT_RESPONSE_TIMEOUT;
bool        g_bShowTimerNotifications = true;
std::string g_strUsername             = "";
std::string g_strPassword             = "";
std::string g_strUserPath             = "";
std::string g_strClientPath           = "";

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;
CHTSPDemux *           HTSPDemuxer    = NULL;
CHTSPData *            HTSPData       = NULL;

extern "C" {

void ADDON_ReadSettings(void)
{
  /* read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_strHostname = buffer;
  else
    g_strHostname = DEFAULT_HOST;
  buffer[0] = 0; /* Set the end of string */

  /* read setting "user" from settings.xml */
  if (XBMC->GetSetting("user", buffer))
    g_strUsername = buffer;
  else
    g_strUsername = "";
  buffer[0] = 0; /* Set the end of string */

  /* read setting "pass" from settings.xml */
  if (XBMC->GetSetting("pass", buffer))
    g_strPassword = buffer;
  else
    g_strPassword = "";

  free (buffer);

  /* read setting "htsp_port" from settings.xml */
  if (!XBMC->GetSetting("htsp_port", &g_iPortHTSP))
    g_iPortHTSP = DEFAULT_HTSP_PORT;

  /* read setting "http_port" from settings.xml */
  if (!XBMC->GetSetting("http_port", &g_iPortHTTP))
    g_iPortHTTP = DEFAULT_HTTP_PORT;

  /* read setting "connect_timeout" from settings.xml */
  if (!XBMC->GetSetting("connect_timeout", &g_iConnectTimeout))
    g_iConnectTimeout = DEFAULT_CONNECT_TIMEOUT;

  /* read setting "read_timeout" from settings.xml */
  if (!XBMC->GetSetting("response_timeout", &g_iResponseTimeout))
    g_iResponseTimeout = DEFAULT_RESPONSE_TIMEOUT;

  /* read setting "notifications_timers" from settings.xml */
  if (!XBMC->GetSetting("notifications_timers", &g_bShowTimerNotifications))
    g_bShowTimerNotifications = true;
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  XBMC->Log(LOG_DEBUG, "%s - Creating Tvheadend PVR-Client", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_iClientId     = pvrprops->iClienId;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  ADDON_ReadSettings();

  HTSPData = new CHTSPData;
  if (!HTSPData->Open())
  {
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_CurStatus == ADDON_STATUS_OK && !HTSPData->IsConnected())
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_CurStatus;
}

void ADDON_Destroy()
{
  if (m_bCreated)
  {
    if (HTSPData)
    {
      delete HTSPData;
      HTSPData = NULL;
    }
  }
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'host' from %s to %s", __FUNCTION__, g_strHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_strHostname;
    g_strHostname = (const char*) settingValue;
    if (tmp_sHostname != g_strHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "user")
  {
    string tmp_sUsername = g_strUsername;
    g_strUsername = (const char*) settingValue;
    if (tmp_sUsername != g_strUsername)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'user'", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "pass")
  {
    string tmp_sPassword = g_strPassword;
    g_strPassword = (const char*) settingValue;
    if (tmp_sPassword != g_strPassword)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'pass'", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "htsp_port")
  {
    if (g_iPortHTSP != *(int*) settingValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'htsp_port' from %u to %u", __FUNCTION__, g_iPortHTSP, *(int*) settingValue);
      g_iPortHTSP = *(int*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "http_port")
  {
    if (g_iPortHTTP != *(int*) settingValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'port' from %u to %u", __FUNCTION__, g_iPortHTTP, *(int*) settingValue);
      g_iPortHTTP = *(int*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "connect_timeout")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iConnectTimeout != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'connect_timeout' from %u to %u", __FUNCTION__, g_iConnectTimeout, iNewValue);
      g_iConnectTimeout = iNewValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "response_timeout")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iResponseTimeout != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'response_timeout' from %u to %u", __FUNCTION__, g_iResponseTimeout, iNewValue);
      g_iResponseTimeout = iNewValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "notifications_timers")
  {
    bool bNewValue = *(bool*) settingValue;
    if (g_bShowTimerNotifications != bNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'notifications_timers' from %u to %u", __FUNCTION__, g_bShowTimerNotifications, bNewValue);
      g_bShowTimerNotifications = bNewValue;
      return ADDON_STATUS_OK;
    }
  }

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsChannelLogo     = false;
  pCapabilities->bSupportsChannelSettings = false;
  pCapabilities->bSupportsTimeshift       = false;
  pCapabilities->bSupportsEPG             = true;
  pCapabilities->bSupportsTV              = true;
  pCapabilities->bSupportsRadio           = true;
  pCapabilities->bSupportsRecordings      = true;
  pCapabilities->bSupportsTimers          = true;
  pCapabilities->bSupportsChannelGroups   = true;
  pCapabilities->bSupportsChannelScan     = false;
  pCapabilities->bHandlesInputStream      = true;
  pCapabilities->bHandlesDemuxing         = true;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = HTSPData ? HTSPData->GetServerName() : "unknown";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static CStdString strBackendVersion;
  if (HTSPData)
    strBackendVersion.Format("%s (Protocol: %i)", HTSPData->GetVersion(), HTSPData->GetProtocol());
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static CStdString strConnectionString;
  if (HTSPData)
    strConnectionString.Format("%s:%i%s", g_strHostname.c_str(), g_iPortHTSP, HTSPData->IsConnected() ? "" : " (Not connected!)");
  else
    strConnectionString.Format("%s:%i (addon error!)", g_strHostname.c_str(), g_iPortHTSP);
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (HTSPData->GetDriveSpace(iTotal, iUsed))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetEpg(handle, channel, iStart, iEnd);
}

int GetChannelsAmount(void)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return 0;

  return HTSPData->GetNumChannels();
}

PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetChannels(handle, bRadio);
}

int GetRecordingsAmount(void)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return 0;

  return HTSPData->GetNumRecordings();
}

PVR_ERROR GetRecordings(PVR_HANDLE handle)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RenameRecording(recording, recording.strTitle);
}

int GetTimersAmount(void)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return 0;

  return HTSPData->GetNumTimers();
}

PVR_ERROR GetTimers(PVR_HANDLE handle)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->DeleteTimer(timer, bForceDelete);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->UpdateTimer(timer);
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  if (!HTSPData || !HTSPData->IsConnected())
    return false;

  HTSPDemuxer = new CHTSPDemux;
  return HTSPDemuxer->Open(channel);
}

void CloseLiveStream(void)
{
  if (HTSPDemuxer)
  {
    HTSPDemuxer->Close();
    delete HTSPDemuxer;
    HTSPDemuxer = NULL;
  }
}

int GetCurrentClientChannel(void)
{
  if (HTSPDemuxer)
    return HTSPDemuxer->CurrentChannel();

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  if (HTSPDemuxer)
    return HTSPDemuxer->SwitchChannel(channel);

  return false;
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  if (HTSPDemuxer && HTSPDemuxer->GetStreamProperties(pProperties))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (HTSPDemuxer && HTSPDemuxer->GetSignalStatus(signalStatus))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

void DemuxAbort(void)
{
  if (HTSPDemuxer)
    HTSPDemuxer->Abort();
}

DemuxPacket* DemuxRead(void)
{
  if (HTSPDemuxer)
    return HTSPDemuxer->Read();
  else
    return NULL;
}

int GetChannelGroupsAmount(void)
{
  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetNumChannelGroups();
}

PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  /* tvheadend doesn't support separated groups, so we only support TV groups */
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetChannelGroups(handle);
}

PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  /* tvheadend doesn't support separated groups, so we only support TV groups */
  if (group.bIsRadio)
    return PVR_ERROR_NO_ERROR;

  if (!HTSPData || !HTSPData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetChannelGroupMembers(handle, group);
}

/** UNUSED API FUNCTIONS */
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }
}
