/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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
#include "XVDRDemux.h"
#include "XVDRRecording.h"
#include "XVDRData.h"
#include "XVDRChannelScan.h"

#include <sstream>
#include <string>
#include <iostream>

using namespace std;
using namespace ADDON;

bool m_bCreated               = false;
ADDON_STATUS m_CurStatus      = ADDON_STATUS_UNKNOWN;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string   g_szHostname              = DEFAULT_HOST;
bool          g_bCharsetConv            = DEFAULT_CHARCONV;     ///< Convert VDR's incoming strings to UTF8 character set
bool          g_bHandleMessages         = DEFAULT_HANDLE_MSG;   ///< Send VDR's OSD status messages to XBMC OSD
int           g_iConnectTimeout         = DEFAULT_TIMEOUT;      ///< The Socket connection timeout
int           g_iPriority               = DEFAULT_PRIORITY;     ///< The Priority this client have in response to other clients
bool          g_bAutoChannelGroups      = DEFAULT_AUTOGROUPS;
int           g_iCompression            = DEFAULT_COMPRESSION;
int           g_iAudioType              = DEFAULT_AUDIOTYPE;
int           g_iUpdateChannels         = DEFAULT_UPDATECHANNELS;

CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_gui   *GUI    = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;

cXVDRDemux      *XVDRDemuxer       = NULL;
cXVDRData       *XVDRData          = NULL;
cXVDRRecording  *XVDRRecording     = NULL;

extern "C" {

/***********************************************************
 * Standart AddOn related public library functions
 ***********************************************************/

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    delete PVR;
    delete XBMC;
    PVR = NULL;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  XBMC->Log(LOG_DEBUG, "Creating VDR XVDR PVR-Client");

  m_CurStatus    = ADDON_STATUS_UNKNOWN;

  /* Read setting "host" from settings.xml */
  char * buffer = (char*) malloc(128);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_szHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_szHostname = DEFAULT_HOST;
  }
  free(buffer);

  /* Read setting "compression" from settings.xml */
  if (!XBMC->GetSetting("compression", &g_iCompression))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'compression' setting, falling back to %i as default", DEFAULT_COMPRESSION);
    g_iCompression = DEFAULT_COMPRESSION;
  }
  else
    g_iCompression *= 3;

  /* Read setting "priority" from settings.xml */
  if (!XBMC->GetSetting("priority", &g_iPriority))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'priority' setting, falling back to %i as default", DEFAULT_PRIORITY);
    g_iPriority = DEFAULT_PRIORITY;
  }

  /* Read setting "convertchar" from settings.xml */
  if (!XBMC->GetSetting("convertchar", &g_bCharsetConv))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'convertchar' setting, falling back to 'false' as default");
    g_bCharsetConv = DEFAULT_CHARCONV;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC->GetSetting("timeout", &g_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    g_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* Read setting "handlemessages" from settings.xml */
  if (!XBMC->GetSetting("handlemessages", &g_bHandleMessages))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'handlemessages' setting, falling back to 'true' as default");
    g_bHandleMessages = DEFAULT_HANDLE_MSG;
  }

  /* Read setting "autochannelgroups" from settings.xml */
  if (!XBMC->GetSetting("autochannelgroups", &g_bAutoChannelGroups))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'autochannelgroups' setting, falling back to 'false' as default");
    g_bAutoChannelGroups = DEFAULT_AUTOGROUPS;
  }

  /* Read setting "audiotype" from settings.xml */
  if (!XBMC->GetSetting("audiotype", &g_iAudioType))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'audiotype' setting, falling back to type %i as default", DEFAULT_AUDIOTYPE);
    g_iAudioType = DEFAULT_TIMEOUT;
  }

  /* Read setting "updatechannels" from settings.xml */
  if (!XBMC->GetSetting("updatechannels", &g_iUpdateChannels))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'updatechannels' setting, falling back to type %i as default", DEFAULT_UPDATECHANNELS);
    g_iUpdateChannels = DEFAULT_UPDATECHANNELS;
  }

  XVDRData = new cXVDRData;
  if (!XVDRData->Open(g_szHostname, DEFAULT_PORT))
  {
    delete XVDRData;
    delete PVR;
    delete XBMC;
    XVDRData = NULL;
    PVR = NULL;
    XBMC = NULL;
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  if (!XVDRData->Login())
  {
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  if (!XVDRData->EnableStatusInterface(g_bHandleMessages))
  {
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  XVDRData->SetUpdateChannels(g_iUpdateChannels);

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  if (m_bCreated)
  {
    delete XVDRData;
    XVDRData = NULL;
  }

  if (PVR)
  {
    delete PVR;
    PVR = NULL;
  }

  if (XBMC)
  {
    delete XBMC;
    XBMC = NULL;
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
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_szHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*) settingValue;
    if (tmp_sHostname != g_szHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "compression")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'compression' from %u to %u", g_iCompression, *(int*) settingValue);
    int old_value = g_iCompression;
    g_iCompression = (*(int*) settingValue) * 3;

    if (old_value != g_iCompression)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "priority")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'priority' from %u to %u", g_iPriority, *(int*) settingValue);
    g_iPriority = *(int*) settingValue;
  }
  else if (str == "convertchar")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'convertchar' from %u to %u", g_bCharsetConv, *(bool*) settingValue);
    g_bCharsetConv = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'timeout' from %u to %u", g_iConnectTimeout, *(int*) settingValue);
    g_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "handlemessages")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'handlemessages' from %u to %u", g_bHandleMessages, *(bool*) settingValue);
    g_bHandleMessages = *(bool*) settingValue;
    if (XVDRData) XVDRData->EnableStatusInterface(g_bHandleMessages);
  }
  else if (str == "autochannelgroups")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'autochannelgroups' from %u to %u", g_bAutoChannelGroups, *(bool*) settingValue);
    if (g_bAutoChannelGroups != *(bool*) settingValue)
    {
      g_bAutoChannelGroups = *(bool*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "audiotype")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'audiotype' from %i to %i", g_iAudioType, *(int*) settingValue);
    g_iAudioType = *(bool*) settingValue;
  }
  else if (str == "updatechannels")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'updatechannels' from %i to %i", g_iUpdateChannels, *(int*) settingValue);
    g_iUpdateChannels = *(bool*) settingValue;
    if (XVDRData != NULL)
      XVDRData->SetUpdateChannels(g_iUpdateChannels);
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
  pCapabilities->bSupportsTimeshift          = false;
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsChannelSettings    = false;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = true;
  if (XVDRData && XVDRData->SupportChannelScan())
    pCapabilities->bSupportsChannelScan      = true;
  else
    pCapabilities->bSupportsChannelScan      = false;

  return PVR_ERROR_NO_ERROR;
}

const char * GetBackendName(void)
{
  static std::string BackendName = XVDRData ? XVDRData->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion(void)
{
  static std::string BackendVersion;
  if (XVDRData) {
    std::stringstream format;
    format << XVDRData->GetVersion() << "(Protocol: " << XVDRData->GetProtocol() << ")";
    BackendVersion = format.str();
  }
  return BackendVersion.c_str();
}

const char * GetConnectionString(void)
{
  static std::string ConnectionString;
  std::stringstream format;

  if (XVDRData) {
    format << g_szHostname << ":" << DEFAULT_PORT;
  }
  else {
    format << g_szHostname << ":" << DEFAULT_PORT << " (addon error!)";
  }
  ConnectionString = format.str();
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetDriveSpace(iTotal, iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR DialogChannelScan(void)
{
  cXVDRChannelScan scanner;
  scanner.Open(g_szHostname, DEFAULT_PORT);
  return PVR_ERROR_NO_ERROR;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetEPGForChannel(handle, channel, iStart, iEnd) ? PVR_ERROR_NO_ERROR: PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount(void)
{
  if (!XVDRData)
    return 0;

  return XVDRData->GetChannelsCount();
}

PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetChannelsList(handle, bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channelgroups Functions           **/

int GetChannelGroupsAmount()
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->GetChannelGroupCount(g_bAutoChannelGroups);
}

PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  if(XVDRData->GetChannelGroupCount(g_bAutoChannelGroups) > 0)
    return XVDRData->GetChannelGroupList(handle, bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  XVDRData->GetChannelGroupMembers(handle, group);
  return PVR_ERROR_NO_ERROR;
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  if (!XVDRData)
    return 0;

  return XVDRData->GetTimersCount();
}

PVR_ERROR GetTimers(PVR_HANDLE handle)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetTimersList(handle) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteTimer(timer, bForce);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->UpdateTimer(timer);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  if (!XVDRData)
    return 0;

  return XVDRData->GetRecordingsCount();
}

PVR_ERROR GetRecordings(PVR_HANDLE handle)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->GetRecordingsList(handle);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->RenameRecording(recording, recording.strTitle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteRecording(recording);
}

/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  XVDRDemuxer = new cXVDRDemux;
  return XVDRDemuxer->OpenChannel(channel);
}

void CloseLiveStream(void)
{
  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
    XVDRDemuxer = NULL;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRDemuxer->GetStreamProperties(pProperties) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

void DemuxAbort(void)
{
  if (XVDRDemuxer) XVDRDemuxer->Abort();
}

void DemuxReset(void)
{
  XBMC->Log(LOG_DEBUG, "DemuxReset");
}

void DemuxFlush(void)
{
  XBMC->Log(LOG_DEBUG, "DemuxFlush");
}

DemuxPacket* DemuxRead(void)
{
  if (!XVDRDemuxer)
    return NULL;

  return XVDRDemuxer->Read();
}

int GetCurrentClientChannel(void)
{
  if (XVDRDemuxer)
    return XVDRDemuxer->CurrentChannel();

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  if (XVDRDemuxer)
    return XVDRDemuxer->SwitchChannel(channel);

  return false;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRDemuxer->GetSignalStatus(signalStatus) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);

}


/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  if(!XVDRData)
    return false;

  CloseRecordedStream();

  XVDRRecording = new cXVDRRecording;
  return XVDRRecording->OpenRecording(recording);
}

void CloseRecordedStream(void)
{
  if (XVDRRecording)
  {
    XVDRRecording->Close();
    delete XVDRRecording;
    XVDRRecording = NULL;
  }
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!XVDRRecording)
    return -1;

  return XVDRRecording->Read(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (XVDRRecording)
    return XVDRRecording->Seek(iPosition, iWhence);

  return -1;
}

long long PositionRecordedStream(void)
{
  if (XVDRRecording)
    return XVDRRecording->Position();

  return 0;
}

long long LengthRecordedStream(void)
{
  if (XVDRRecording)
    return XVDRRecording->Length();

  return 0;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }

}
