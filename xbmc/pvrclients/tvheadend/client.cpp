/*
 *      Copyright (C) 2005-2009 Team XBMC
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

//cPVRClientTvheadend *g_client = NULL;
bool m_bCreated               = false;
ADDON_STATUS m_CurStatus      = STATUS_UNKNOWN;
int g_clientID                = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
CStdString g_szHostname       = DEFAULT_HOST;
int g_iPortHTSP               = DEFAULT_HTSP_PORT;
int g_iPortHTTP               = DEFAULT_HTTP_PORT;
int g_iConnectTimout          = DEFAULT_TIMEOUT;
int g_iSkipIFrame             = DEFAULT_SKIP_I_FRAME;
CStdString g_szUsername       = "";
CStdString g_szPassword       = "";
CStdString g_szUserPath       = "";
CStdString g_szClientPath     = "";
CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;
cHTSPDemux *HTSPDemuxer       = NULL;
cHTSPData  *HTSPData          = NULL;

extern "C" {

/***********************************************************
 * Standart AddOn related public library functions
 ***********************************************************/

ADDON_STATUS Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  XBMC->Log(LOG_DEBUG, "%s - Creating Tvheadend PVR-Client", __FUNCTION__);

  m_CurStatus    = STATUS_UNKNOWN;
  g_clientID     = pvrprops->iClienId;
  g_szUserPath   = pvrprops->strUserPath;
  g_szClientPath = pvrprops->strClientPath;

  /* Read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_szHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'host' setting, falling back to '%s' as default", __FUNCTION__, DEFAULT_HOST);
    g_szHostname = DEFAULT_HOST;
  }
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("user", buffer))
    g_szUsername = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'user' setting", __FUNCTION__);
    g_szUsername = "";
  }
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("pass", buffer))
    g_szPassword = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'pass' setting", __FUNCTION__);
    g_szPassword = "";
  }
  free (buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("htsp_port", &g_iPortHTSP))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'htsp_port' setting, falling back to '%i' as default", __FUNCTION__, DEFAULT_HTSP_PORT);
    g_iPortHTSP = DEFAULT_HTSP_PORT;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("http_port", &g_iPortHTTP))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'http_port' setting, falling back to '%i' as default", __FUNCTION__, DEFAULT_HTTP_PORT);
    g_iPortHTTP = DEFAULT_HTTP_PORT;
  }

  /* Read setting "skip_I_frame_count" from settings.xml */
  if (!XBMC->GetSetting("skip_I_frame_count", &g_iSkipIFrame))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'skip_I_frame_count' setting, falling back to '%d' as default", __FUNCTION__, DEFAULT_SKIP_I_FRAME);
    g_iSkipIFrame = DEFAULT_SKIP_I_FRAME;
  }

  HTSPData = new cHTSPData;
  if (!HTSPData->Open(g_szHostname, g_iPortHTSP, g_szUsername, g_szPassword, g_iConnectTimout))
  {
    m_CurStatus = STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS GetStatus()
{
  return m_CurStatus;
}

void Destroy()
{
  if (m_bCreated)
  {
    delete HTSPData;
    HTSPData = NULL;
  }
  m_CurStatus = STATUS_UNKNOWN;
}

bool HasSettings()
{
  return true;
}

unsigned int GetSettings(StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'host' from %s to %s", __FUNCTION__, g_szHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*) settingValue;
    if (tmp_sHostname != g_szHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "user")
  {
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'user'", __FUNCTION__);
    string tmp_sUsername = g_szUsername;
    g_szUsername = (const char*) settingValue;
    if (tmp_sUsername != g_szUsername)
      return STATUS_NEED_RESTART;
  }
  else if (str == "pass")
  {
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'pass'", __FUNCTION__);
    string tmp_sPassword = g_szPassword;
    g_szPassword = (const char*) settingValue;
    if (tmp_sPassword != g_szPassword)
      return STATUS_NEED_RESTART;
  }
  else if (str == "htsp_port")
  {
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'port' from %u to %u", __FUNCTION__, g_iPortHTSP, *(int*) settingValue);
    if (g_iPortHTSP != *(int*) settingValue)
    {
      g_iPortHTSP = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "http_port")
  {
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'port' from %u to %u", __FUNCTION__, g_iPortHTTP, *(int*) settingValue);
    if (g_iPortHTTP != *(int*) settingValue)
    {
      g_iPortHTTP = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "skip_I_frame_count")
  {
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'skip_I_frame_count' from %u to %u", __FUNCTION__, g_iSkipIFrame, *(int*) settingValue);
    if (g_iSkipIFrame != *(int*) settingValue)
    {
      g_iSkipIFrame = *(int*) settingValue;
      return STATUS_OK;
    }
  }

  return STATUS_OK;
}

void Stop()
{
}

void FreeSettings()
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
  pCapabilities->bSupportsChannelGroups   = false;
  pCapabilities->bSupportsChannelScan     = false;
  pCapabilities->bHandlesInputStream      = true;
  pCapabilities->bHandlesDemuxing         = true;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  if (HTSPDemuxer && HTSPDemuxer->GetStreamProperties(pProperties))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

const char * GetBackendName(void)
{
  static CStdString strBackendName = HTSPData ? HTSPData->GetServerName() : "unknown";
  return strBackendName.c_str();
}

const char * GetBackendVersion(void)
{
  static CStdString strBackendVersion;
  if (HTSPData)
    strBackendVersion.Format("%s (Protocol: %i)", HTSPData->GetVersion(), HTSPData->GetProtocol());
  return strBackendVersion.c_str();
}

const char * GetConnectionString(void)
{
  static CStdString strConnectionString;
  if (HTSPData)
    strConnectionString.Format("%s:%i%s", g_szHostname.c_str(), g_iPortHTSP, HTSPData->CheckConnection() ? "" : " (Not connected!)");
  else
    strConnectionString.Format("%s:%i (addon error!)", g_szHostname.c_str(), g_iPortHTSP);
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (HTSPData && HTSPData->GetDriveSpace(iTotal, iUsed))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetEpg(handle, channel, iStart, iEnd);
}

int GetChannelsAmount(void)
{
  if (!HTSPData)
    return 0;

  return HTSPData->GetNumChannels();
}

PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetChannels(handle, bRadio);
}

int GetRecordingsAmount(void)
{
  if (!HTSPData)
    return 0;

  return HTSPData->GetNumRecordings();
}

PVR_ERROR GetRecordings(PVR_HANDLE handle)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  if(!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RenameRecording(recording, recording.strTitle);
}

int GetTimersAmount(void)
{
  if (!HTSPData)
    return 0;

  return HTSPData->GetNumTimers();
}

PVR_ERROR GetTimers(PVR_HANDLE handle)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->DeleteTimer(timer, bForceDelete);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if(!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->UpdateTimer(timer);
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  HTSPDemuxer = new cHTSPDemux;
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

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (HTSPDemuxer && HTSPDemuxer->GetSignalStatus(signalStatus))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

void DemuxAbort(void)
{
  if (HTSPDemuxer) HTSPDemuxer->Abort();
}

DemuxPacket* DemuxRead(void)
{
  return HTSPDemuxer->Read();
}

int GetChannelGroupsAmount(void)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetNumChannelGroups();
}

PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  /* tvheadend doesn't support separated groups, so we only support TV groups */
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetChannelGroups(handle);
}

PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  /* tvheadend doesn't support separated groups, so we only support TV groups */
  if (group.bIsRadio)
    return PVR_ERROR_NO_ERROR;

  if (!HTSPData)
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
