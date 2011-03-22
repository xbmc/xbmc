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
int g_iEpgOffsetCorrection    = DEFAULT_EPG_OFFSET_CORRECTION;
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

  PVR_PROPS* pvrprops = (PVR_PROPS*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  XBMC->Log(LOG_DEBUG, "%s - Creating Tvheadend PVR-Client", __FUNCTION__);

  m_CurStatus    = STATUS_UNKNOWN;
  g_clientID     = pvrprops->clientID;
  g_szUserPath   = pvrprops->userpath;
  g_szClientPath = pvrprops->clientpath;

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

  /* Read setting "epg_offset_correction" from settings.xml */
  if (!XBMC->GetSetting("epg_offset_correction", &g_iEpgOffsetCorrection))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "%s - Couldn't get 'epg_offset_correction' setting, falling back to '%i' as default", __FUNCTION__, DEFAULT_EPG_OFFSET_CORRECTION);
    g_iEpgOffsetCorrection = DEFAULT_EPG_OFFSET_CORRECTION;
  }else
  {
    g_iEpgOffsetCorrection -= 12;
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
  else if (str == "epg_offset_correction")
  {
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'epg_offset_correction' from %d to %d", __FUNCTION__, g_iEpgOffsetCorrection, *(int*) settingValue);
    if (g_iEpgOffsetCorrection != *(int*) settingValue - 12)
    {
      g_iEpgOffsetCorrection = *(int*) settingValue - 12;
      return STATUS_NEED_RESTART;
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

PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportTV                 = true;
  props->SupportRadio              = true;
  props->SupportChannelSettings    = false;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;
  props->HandleInputStream         = true;
  props->HandleDemuxing            = true;
  props->SupportChannelScan        = false;

  return PVR_ERROR_NO_ERROR;
}

const char * GetBackendName()
{
  static CStdString BackendName = HTSPData ? HTSPData->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion()
{
  static CStdString BackendVersion;
  if (HTSPData)
    BackendVersion.Format("%s (Protocol: %i)", HTSPData->GetVersion(), HTSPData->GetProtocol());
  return BackendVersion.c_str();
}

const char * GetConnectionString()
{
  static CStdString ConnectionString;
  if (HTSPData)
    ConnectionString.Format("%s:%i%s", g_szHostname.c_str(), g_iPortHTSP, HTSPData->CheckConnection() ? "" : " (Not connected!)");
  else
    ConnectionString.Format("%s:%i (addon error!)", g_szHostname.c_str(), g_iPortHTSP);
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  if (HTSPData && HTSPData->GetDriveSpace(total, used))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  if (HTSPData && HTSPData->GetTime(localTime, gmtOffset))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}


/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RequestEPGForChannel(handle, channel, start, end);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetNumChannels()
{
  if (!HTSPData)
    return 0;

  return HTSPData->GetNumChannels();
}

PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RequestChannelList(handle, radio);
}

/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  CloseLiveStream();

  HTSPDemuxer = new cHTSPDemux;
  return HTSPDemuxer->Open(channelinfo);
}

void CloseLiveStream()
{
  if (HTSPDemuxer)
  {
    HTSPDemuxer->Close();
    delete HTSPDemuxer;
    HTSPDemuxer = NULL;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* props)
{
  if (HTSPDemuxer && HTSPDemuxer->GetStreamProperties(props))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

void DemuxAbort()
{
  if (HTSPDemuxer) HTSPDemuxer->Abort();
}

DemuxPacket* DemuxRead()
{
  return HTSPDemuxer->Read();
}

int GetCurrentClientChannel()
{
  if (HTSPDemuxer)
    return HTSPDemuxer->CurrentChannel();

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  if (HTSPDemuxer)
    return HTSPDemuxer->SwitchChannel(channelinfo);

  return false;
}

PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  if (HTSPDemuxer && HTSPDemuxer->GetSignalStatus(qualityinfo))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

/*******************************************/
/** PVR Recording Functions               **/

PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RequestRecordingsList(handle);
}

int GetNumRecordings(void)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetNumRecordings();
}

PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->DeleteRecording(recinfo);
}

/*******************************************/
/** PVR Timer Functions               **/

int GetNumTimers(void)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->GetNumTimers();
}

PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RequestTimerList(handle);
}

PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->DeleteTimer(timerinfo, force);
}

PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  if (!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->AddTimer(timerinfo);
}

PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo) 
{ 
  if(!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->UpdateTimer(timerinfo);
}

PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname) 
{ 
  if(!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  // The new name is already in the timerinfo.title, so we don't need it
  return HTSPData->UpdateTimer(timerinfo);
}

PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname) 
{ 
  if(!HTSPData)
    return PVR_ERROR_SERVER_ERROR;

  return HTSPData->RenameRecording(recinfo, newname);
}

/** UNUSED API FUNCTIONS */
PVR_ERROR DialogChannelScan() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetNumBouquets() { return 0; }
PVR_ERROR RequestBouquetsList(PVRHANDLE handle, int radio) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(unsigned int number) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(unsigned int number, const char *newname) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channelinfo) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channelinfo) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool HaveCutmarks() { return false; }
PVR_ERROR RequestCutMarksList(PVRHANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddCutMark(const PVR_CUT_MARK &cutmark) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteCutMark(const PVR_CUT_MARK &cutmark) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR StartCut() { return PVR_ERROR_NOT_IMPLEMENTED; }
bool SwapLiveTVSecondaryStream() { return false; }
bool OpenSecondaryStream(const PVR_CHANNEL &channelinfo) { return false; }
void CloseSecondaryStream() {}
int ReadSecondaryStream(unsigned char* buf, int buf_size) { return 0; }
bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char* buf, int buf_size) { return 0; }
long long SeekRecordedStream(long long pos, int whence) { return 0; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(){}
void DemuxFlush(){}
int ReadLiveStream(unsigned char* buf, int buf_size) { return 0; }
long long SeekLiveStream(long long pos, int whence) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo) { return ""; }

}
