/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
#include "pvrclient-mediaportal.h"
#include "utils.h"

using namespace std;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_szHostname           = DEFAULT_HOST;         ///< The Host name or IP of the MediaPortal TV Server
int         g_iPort                = DEFAULT_PORT;         ///< The TVServerXBMC listening port (default: 9596)
int         g_iConnectTimeout      = DEFAULT_TIMEOUT;      ///< The Socket connection timeout
int         g_iSleepOnRTSPurl      = DEFAULT_SLEEP_RTSP_URL; ///< An optional delay between tuning a channel and opening the corresponding RTSP stream in XBMC (default: 0)
bool        g_bOnlyFTA             = DEFAULT_FTA_ONLY;     ///< Send only Free-To-Air Channels inside Channel list to XBMC
bool        g_bRadioEnabled        = DEFAULT_RADIO;        ///< Send also Radio channels list to XBMC
bool        g_bHandleMessages      = DEFAULT_HANDLE_MSG;   ///< Send VDR's OSD status messages to XBMC OSD
bool        g_bResolveRTSPHostname = DEFAULT_RESOLVE_RTSP_HOSTNAME; ///< Resolve the server hostname in the rtsp URLs to an IP at the TV Server side (default: false)
bool        g_bReadGenre           = DEFAULT_READ_GENRE;   ///< Read the genre strings from MediaPortal and translate them into XBMC DVB genre id's (only English)
bool        g_bUseRecordingsDir    = DEFAULT_USE_REC_DIR;  ///< Use a normal directory if true for recordings
std::string g_szRecordingsDir      = DEFAULT_REC_DIR;      ///< The path to the recordings directory
std::string g_szTVGroup            = DEFAULT_TVGROUP;      ///< Import only TV channels from this TV Server TV group
std::string g_szRadioGroup         = DEFAULT_RADIOGROUP;   ///< Import only radio channels from this TV Server radio group
bool        g_bDirectTSFileRead    = DEFAULT_DIRECT_TS_FR; ///< Open the Live-TV timeshift buffer directly (skip RTSP streaming)

/* Client member variables */
ADDON_STATUS           m_CurStatus    = STATUS_UNKNOWN;
cPVRClientMediaPortal *g_client       = NULL;
bool                   g_bCreated     = false;
int                    g_iClientID    = -1;
std::string            g_szUserPath   = "";
std::string            g_szClientPath = "";
cHelper_libXBMC_addon *XBMC           = NULL;
cHelper_libXBMC_pvr   *PVR            = NULL;

extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

//-- Create -------------------------------------------------------------------
// Called after loading of the dll, all steps to become Client functional
// must be performed here.
//-----------------------------------------------------------------------------
ADDON_STATUS Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return STATUS_UNKNOWN;

  PVR_PROPS* pvrprops = (PVR_PROPS*)props;

  XBMC = new cHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  PVR = new cHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  XBMC->Log(LOG_DEBUG, "Creating MediaPortal PVR-Client (ffmpeg rtsp version)");

  m_CurStatus    = STATUS_UNKNOWN;
  g_client       = new cPVRClientMediaPortal();
  g_iClientID    = pvrprops->clientID;
  g_szUserPath   = pvrprops->userpath;
  g_szClientPath = pvrprops->clientpath;

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
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '9596' as default");
    g_iPort = DEFAULT_PORT;
  }

  /* Read setting "ftaonly" from settings.xml */
  if (!XBMC->GetSetting("ftaonly", &g_bOnlyFTA))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'ftaonly' setting, falling back to 'false' as default");
    g_bOnlyFTA = DEFAULT_FTA_ONLY;
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

  if (!XBMC->GetSetting("tvgroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_szTVGroup = buffer;
  }

  if (!XBMC->GetSetting("radiogroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_szRadioGroup = buffer;
  }

  /* Read setting "resolvertsphostname" from settings.xml */
  if (!XBMC->GetSetting("resolvertsphostname", &g_bResolveRTSPHostname))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'resolvertsphostname' setting, falling back to 'true' as default");
    g_bResolveRTSPHostname = DEFAULT_RESOLVE_RTSP_HOSTNAME;
  }

  /* Read setting "readgenre" from settings.xml */
  if (!XBMC->GetSetting("readgenre", &g_bReadGenre))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'resolvertsphostname' setting, falling back to 'true' as default");
    g_bReadGenre = DEFAULT_READ_GENRE;
  }

  /* Read setting "sleeponrtspurl" from settings.xml */
  if (!XBMC->GetSetting("sleeponrtspurl", &g_iSleepOnRTSPurl))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'sleeponrtspurl' setting, falling back to %i seconds as default", DEFAULT_SLEEP_RTSP_URL);
    g_iSleepOnRTSPurl = DEFAULT_SLEEP_RTSP_URL;
  }

  /* Read setting "userecordingsdir" from settings.xml */
  if (!XBMC->GetSetting("userecordingsdir", &g_bUseRecordingsDir))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'userecordingsdir' setting, falling back to 'false' as default");
    g_bReadGenre = DEFAULT_USE_REC_DIR;
  }

  if (!XBMC->GetSetting("recordingsdir", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'recordingsdir' setting, falling back to '%s' as default", DEFAULT_REC_DIR);
  } else {
    g_szRecordingsDir = buffer;
  }
  g_bDirectTSFileRead = false;
  /* Create connection to MediaPortal XBMC TV client */
  if (!g_client->Connect())
  {
    m_CurStatus = STATUS_LOST_CONNECTION;
  }
  else
  {
    m_CurStatus = STATUS_OK;
  }

  g_bCreated = true;

  return m_CurStatus;
}

//-- Destroy ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void Destroy()
{
  if ((g_bCreated) && (g_client))
  {
    g_client->Disconnect();
    delete_null(g_client);

    g_bCreated = false;
  }

  if (PVR)
  {
    delete_null(PVR);
  }
  if (XBMC)
  {
    delete_null(XBMC);
  }

  m_CurStatus = STATUS_UNKNOWN;
}

//-- GetStatus ----------------------------------------------------------------
// Report the current Add-On Status to XBMC
//-----------------------------------------------------------------------------
ADDON_STATUS GetStatus()
{
  return m_CurStatus;
}

//-- HasSettings --------------------------------------------------------------
// Report "true", yes this AddOn have settings
//-----------------------------------------------------------------------------
bool HasSettings()
{
  return true;
}

unsigned int GetSettings(StructSetting ***sSet)
{
  return 0;
}

//-- SetSetting ---------------------------------------------------------------
// Called everytime a setting is changed by the user and to inform AddOn about
// new setting and to do required stuff to apply it.
//-----------------------------------------------------------------------------
ADDON_STATUS SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;

  // SetSetting can occur when the addon is enabled, but TV support still
  // disabled. In that case the addon is not loaded, so we should not try
  // to change its settings.
  if (!g_bCreated)
    return STATUS_OK;

  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_szHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*) settingValue;
    if (tmp_sHostname != g_szHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iPort, *(int*) settingValue);
    if (g_iPort != *(int*) settingValue)
    {
      g_iPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "ftaonly")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'ftaonly' from %u to %u", g_bOnlyFTA, *(bool*) settingValue);
    g_bOnlyFTA = *(bool*) settingValue;
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
  else if (str == "tvgroup")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'tvgroup' from %s to %s", g_szTVGroup.c_str(), (const char*) settingValue);
    g_szTVGroup = (const char*) settingValue;
  }
  else if (str == "radiogroup")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'radiogroup' from %s to %s", g_szRadioGroup.c_str(), (const char*) settingValue);
    g_szRadioGroup = (const char*) settingValue;
  }
  else if (str == "resolvertsphostname")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'resolvertsphostname' from %u to %u", g_bResolveRTSPHostname, *(bool*) settingValue);
    g_bResolveRTSPHostname = *(bool*) settingValue;
  }
  else if (str == "readgenre")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'readgenre' from %u to %u", g_bReadGenre, *(bool*) settingValue);
    g_bReadGenre = *(bool*) settingValue;
  }
  else if (str == "sleeponrtspurl")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'sleeponrtspurl' from %u to %u", g_iSleepOnRTSPurl, *(int*) settingValue);
    g_iSleepOnRTSPurl = *(int*) settingValue;
  }
  else if (str == "userecordingsdir")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'userecordingsdir' from %u to %u", g_bUseRecordingsDir, *(bool*) settingValue);
    g_bUseRecordingsDir = *(bool*) settingValue;
  }
  else if (str == "recordingsdir")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'recordingsdir' from %s to %s", g_szRecordingsDir.c_str(), (const char*) settingValue);
    g_szRecordingsDir = (const char*) settingValue;
  }
  return STATUS_OK;
}

void Stop()
{
  Destroy();
}

void FreeSettings()
{

}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

//-- GetProperties ------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
  XBMC->Log(LOG_DEBUG, "->GetProperties()");

  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportTV                 = true;
  props->SupportRadio              = g_bRadioEnabled;
  props->SupportChannelSettings    = true;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;
  props->HandleInputStream         = true;
  props->HandleDemuxing            = false;
  props->SupportChannelScan        = false;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* props)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

//-- GetBackendName -----------------------------------------------------------
// Return the Name of the Backend
//-----------------------------------------------------------------------------
const char * GetBackendName()
{
  return g_client->GetBackendName();
}

//-- GetBackendVersion --------------------------------------------------------
// Return the Version of the Backend as String
//-----------------------------------------------------------------------------
const char * GetBackendVersion()
{
  return g_client->GetBackendVersion();
}

//-- GetConnectionString ------------------------------------------------------
// Return a String with connection info, if available
//-----------------------------------------------------------------------------
const char * GetConnectionString()
{
  return g_client->GetConnectionString();
}

//-- GetDriveSpace ------------------------------------------------------------
// Return the Total and Free Drive space on the PVR Backend
//-----------------------------------------------------------------------------
PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  return g_client->GetDriveSpace(total, used);
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  return g_client->GetMPTVTime(localTime, gmtOffset);
}

PVR_ERROR DialogChannelScan()
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR MenuHook(const PVR_MENUHOOK &menuhook)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  return g_client->RequestEPGForChannel(channel, handle, start, end);
}


/*******************************************/
/** PVR Bouquets Functions                **/

int GetNumBouquets()
{
  return 0;
}

PVR_ERROR RequestBouquetsList(PVRHANDLE handle, int radio)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetNumChannels()
{
  return g_client->GetNumChannels();
}

PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
  return g_client->RequestChannelList(handle, radio);
}

PVR_ERROR DeleteChannel(unsigned int number)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameChannel(unsigned int number, const char *newname)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber)
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
/** PVR Recording Functions               **/

int GetNumRecordings(void)
{
  return g_client->GetNumRecordings();
}

PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  return g_client->RequestRecordingsList(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  return g_client->DeleteRecording(recinfo);
}

PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  return g_client->RenameRecording(recinfo, newname);
}


/*******************************************/
/** PVR Recording cut marks Functions     **/

bool HaveCutmarks()
{
  return false;
}

PVR_ERROR RequestCutMarksList(PVRHANDLE handle)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR AddCutMark(const PVR_CUT_MARK &cutmark)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteCutMark(const PVR_CUT_MARK &cutmark)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR StartCut()
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

/*******************************************/
/** PVR Timer Functions                   **/

int GetNumTimers(void)
{
  return g_client->GetNumTimers();
}

PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  return g_client->RequestTimerList(handle);
}

PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  return g_client->AddTimer(timerinfo);
}

PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  return g_client->DeleteTimer(timerinfo, force);
}

PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  return g_client->RenameTimer(timerinfo, newname);
}

PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  return g_client->UpdateTimer(timerinfo);
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

int ReadLiveStream(unsigned char* buf, int buf_size)
{
  return g_client->ReadLiveStream(buf, buf_size);
}

int GetCurrentClientChannel()
{
  return g_client->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  return g_client->SwitchChannel(channelinfo);
}

PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  return g_client->SignalQuality(qualityinfo);
}

/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  return g_client->OpenRecordedStream(recinfo);
}

void CloseRecordedStream(void)
{
  return g_client->CloseRecordedStream();
}

int ReadRecordedStream(unsigned char* buf, int buf_size)
{
  return g_client->ReadRecordedStream(buf, buf_size);
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  return g_client->GetLiveStreamURL(channelinfo);
}

/** UNUSED API FUNCTIONS */
DemuxPacket* DemuxRead() { return NULL; }
void DemuxAbort() {}
void DemuxReset() {}
void DemuxFlush() {}

long long SeekRecordedStream(long long pos, int whence) { return -1; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return -1; }

long long SeekLiveStream(long long pos, int whence) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1 ; }

/*******************************************/
/** PVR Secondary Stream Functions        **/
bool SwapLiveTVSecondaryStream() { return false; }
bool OpenSecondaryStream(const PVR_CHANNEL &channelinfo) { return false; }
void CloseSecondaryStream() {}
int ReadSecondaryStream(unsigned char* buf, int buf_size) { return -1; }

} //end extern "C"
