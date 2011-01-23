/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "pvrclient-mediaportal.h"
#include "utils.h"

using namespace std;

cPVRClientMediaPortal *g_client = NULL;
bool m_bCreated         = false;
ADDON_STATUS curStatus  = STATUS_UNKNOWN;
int g_clientID          = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string m_sHostname     = DEFAULT_HOST;
int m_iPort                 = DEFAULT_PORT;
bool m_bOnlyFTA             = DEFAULT_FTA_ONLY;
bool m_bRadioEnabled        = DEFAULT_RADIO;
bool m_bCharsetConv         = DEFAULT_CHARCONV;
int m_iConnectTimeout       = DEFAULT_TIMEOUT;
bool m_bNoBadChannels       = DEFAULT_BADCHANNELS;
bool m_bHandleMessages      = DEFAULT_HANDLE_MSG;
std::string g_szUserPath    = "";
std::string g_szClientPath  = "";
std::string g_sTVGroup      = "";
std::string g_sRadioGroup   = "";
bool m_bResolveRTSPHostname = DEFAULT_RESOLVE_RTSP_HOSTNAME;
bool m_bReadGenre           = DEFAULT_READ_GENRE;
int m_iSleepOnRTSPurl       = DEFAULT_SLEEP_RTSP_URL;

cHelper_libXBMC_addon *XBMC = NULL;
cHelper_libXBMC_pvr   *PVR  = NULL;

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

  XBMC->Log(LOG_DEBUG, "Creating MediaPortal PVR-Client");

  curStatus      = STATUS_UNKNOWN;
  g_client       = new cPVRClientMediaPortal();
  g_clientID     = pvrprops->clientID;
  g_szUserPath   = pvrprops->userpath;
  g_szClientPath = pvrprops->clientpath;

  /* Read setting "host" from settings.xml */
  char buffer[1024];

  if (XBMC->GetSetting("host", &buffer))
    m_sHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    m_sHostname = DEFAULT_HOST;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &m_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '9596' as default");
    m_iPort = DEFAULT_PORT;
  }

  /* Read setting "ftaonly" from settings.xml */
  if (!XBMC->GetSetting("ftaonly", &m_bOnlyFTA))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'ftaonly' setting, falling back to 'false' as default");
    m_bOnlyFTA = DEFAULT_FTA_ONLY;
  }

  /* Read setting "useradio" from settings.xml */
  if (!XBMC->GetSetting("useradio", &m_bRadioEnabled))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'useradio' setting, falling back to 'true' as default");
    m_bRadioEnabled = DEFAULT_RADIO;
  }

  /* Read setting "convertchar" from settings.xml */
  if (!XBMC->GetSetting("convertchar", &m_bCharsetConv))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'convertchar' setting, falling back to 'false' as default");
    m_bCharsetConv = DEFAULT_CHARCONV;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC->GetSetting("timeout", &m_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    m_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  if (!XBMC->GetSetting("tvgroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_sTVGroup = buffer;
  }

  if (!XBMC->GetSetting("radiogroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_sRadioGroup = buffer;
  }

  /* Read setting "resolvertsphostname" from settings.xml */
  if (!XBMC->GetSetting("resolvertsphostname", &m_bResolveRTSPHostname))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'resolvertsphostname' setting, falling back to 'true' as default");
    m_bRadioEnabled = DEFAULT_RESOLVE_RTSP_HOSTNAME;
  }

  /* Read setting "readgenre" from settings.xml */
  if (!XBMC->GetSetting("readgenre", &m_bReadGenre))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'resolvertsphostname' setting, falling back to 'true' as default");
    m_bReadGenre = DEFAULT_READ_GENRE;
  }

  /* Read setting "sleeponrtspurl" from settings.xml */
  if (!XBMC->GetSetting("sleeponrtspurl", &m_iSleepOnRTSPurl))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'sleeponrtspurl' setting, falling back to %i seconds as default", DEFAULT_SLEEP_RTSP_URL);
    m_iSleepOnRTSPurl = DEFAULT_SLEEP_RTSP_URL;
  }

  /* Create connection to MediaPortal XBMC TV client */
  if (!g_client->Connect())
  {
    curStatus = STATUS_LOST_CONNECTION;
  }
  else
  {
    curStatus = STATUS_OK;
  }

  m_bCreated = true;

  return curStatus;
}

//-- Destroy ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void Destroy()
{
  if (m_bCreated)
  {
    g_client->Disconnect();
    delete_null(g_client);

    m_bCreated = false;
  }

  if (PVR)
  {
    delete_null(PVR);
  }
  if (XBMC)
  {
    delete_null(XBMC);
  }


  curStatus = STATUS_UNKNOWN;
}

//-- GetStatus ----------------------------------------------------------------
// Report the current Add-On Status to XBMC
//-----------------------------------------------------------------------------
ADDON_STATUS GetStatus()
{
  return curStatus;
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
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", m_sHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = m_sHostname;
    m_sHostname = (const char*) settingValue;
    if (tmp_sHostname != m_sHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", m_iPort, *(int*) settingValue);
    if (m_iPort != *(int*) settingValue)
    {
      m_iPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "ftaonly")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'ftaonly' from %u to %u", m_bOnlyFTA, *(bool*) settingValue);
    m_bOnlyFTA = *(bool*) settingValue;
  }
  else if (str == "useradio")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'useradio' from %u to %u", m_bRadioEnabled, *(bool*) settingValue);
    m_bRadioEnabled = *(bool*) settingValue;
  }
  else if (str == "convertchar")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'convertchar' from %u to %u", m_bCharsetConv, *(bool*) settingValue);
    m_bCharsetConv = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'timeout' from %u to %u", m_iConnectTimeout, *(int*) settingValue);
    m_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "tvgroup")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'tvgroup' from %s to %s", g_sTVGroup.c_str(), (const char*) settingValue);
    g_sTVGroup = (const char*) settingValue;
  }
  else if (str == "radiogroup")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'radiogroup' from %s to %s", g_sTVGroup.c_str(), (const char*) settingValue);
    g_sTVGroup = (const char*) settingValue;
  }
  else if (str == "resolvertsphostname")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'resolvertsphostname' from %u to %u", m_bResolveRTSPHostname, *(bool*) settingValue);
    m_bResolveRTSPHostname = *(bool*) settingValue;
  }
  else if (str == "readgenre")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'readgenre' from %u to %u",m_bReadGenre, *(bool*) settingValue);
    m_bReadGenre = *(bool*) settingValue;
  }
  else if (str == "sleeponrtspurl")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'sleeponrtspurl' from %u to %u", m_iSleepOnRTSPurl, *(int*) settingValue);
    m_iSleepOnRTSPurl = *(int*) settingValue;
  }

  return STATUS_OK;
}

//-- Remove ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void Remove()
{
  if (m_bCreated)
  {
    g_client->Disconnect();

    delete g_client;
    g_client = NULL;

    m_bCreated = false;
  }
  curStatus = STATUS_UNKNOWN;
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
  props->SupportRadio              = m_bRadioEnabled;
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

long long SeekLiveStream(long long pos, int whence)
{
  return -1;
}

long long PositionLiveStream(void)
{
  return -1;
}

long long LengthLiveStream(void)
{
  return -1;
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

long long SeekRecordedStream(long long pos, int whence)
{
  return g_client->SeekRecordedStream(pos, whence);
}

long long PositionRecordedStream(void)
{
  return -1;
}

long long LengthRecordedStream(void)
{
  return g_client->LengthRecordedStream();
}

// MG: added for Mediaportal
const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  return g_client->GetLiveStreamURL(channelinfo);
}

/** UNUSED API FUNCTIONS */
DemuxPacket* DemuxRead(){return NULL;}
void DemuxAbort(){}
void DemuxReset(){}
void DemuxFlush(){}
bool SwapLiveTVSecondaryStream() { return false; }
bool OpenSecondaryStream(const PVR_CHANNEL &channelinfo) { return false; }
void CloseSecondaryStream() {}
int ReadSecondaryStream(unsigned char* buf, int buf_size) { return 0; }

} //extern "C"
