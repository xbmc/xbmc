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
#include "MythXml.h"

using namespace std;

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
CStdString   g_szHostname             = DEFAULT_HOST;         ///< The Host name or IP of MythTV
int          g_iMythXmlPort           = DEFAULT_MYTHXML_PORT; ///< The MyhtXML Port of MythTV (default is 6544)
int          g_iPin                   = DEFAULT_PIN; ///< The Mythtv server PIN (default is 0000)
int          g_iMythXmlConnectTimeout = DEFAULT_MYTHXML_CONNECTION_TIMEOUT; ///< The MYTHXML Connection Timeout value (default is 30 seconds)
///* Client member variables */

bool         m_recordingFirstRead;
char         m_noSignalStreamData[ 6 + 0xffff ];
long         m_noSignalStreamSize     = 0;
long         m_noSignalStreamReadPos  = 0;
bool         m_bPlayingNoSignal       = false;
int          m_iCurrentChannel        = 1;
ADDON_STATUS m_CurStatus              = STATUS_UNKNOWN;
bool         g_bCreated               = false;
int          g_iClientID              = -1;
CStdString   g_szUserPath             = "";
CStdString   g_szClientPath           = "";
MythXml		 *MythXmlApi			  = NULL;
cHelper_libXBMC_addon *XBMC           = NULL;
cHelper_libXBMC_pvr   *PVR            = NULL;


extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;

  PVR_PROPS* pvrprops = (PVR_PROPS*)props;

  XBMC = new cHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  PVR = new cHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
    return STATUS_UNKNOWN;

  XBMC->Log(LOG_DEBUG, "Creating MythTV PVR-Client");

  m_CurStatus    = STATUS_UNKNOWN;
  g_iClientID    = pvrprops->clientID;
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
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_szHostname = DEFAULT_HOST;
  }
  free (buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("mythXMLPort", &g_iMythXmlPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'mythXMLPort' setting, falling back to '%i' as default", DEFAULT_MYTHXML_PORT);
    g_iMythXmlPort = DEFAULT_MYTHXML_PORT;
  }
  
  /* Read setting "pin" from settings.xml */
  if (!XBMC->GetSetting("mythXMLTimeout", &g_iMythXmlConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'mythXMLTimeout' setting, falling back to '%i' as default", DEFAULT_MYTHXML_CONNECTION_TIMEOUT);
    g_iMythXmlConnectTimeout = DEFAULT_MYTHXML_CONNECTION_TIMEOUT;
  } else {
	// we need to multiply by 1000 the value as the settings file is in seconds
    g_iMythXmlConnectTimeout *= 1000;
  }
  
  /* Read setting "pin" from settings.xml */
  if (!XBMC->GetSetting("pin", &g_iPin))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'pin' setting, falling back to '%i' as default", DEFAULT_PIN);
    g_iPin = DEFAULT_PIN;
  }

  MythXmlApi = new MythXml();
  if (!MythXmlApi->open(g_szHostname, g_iMythXmlPort, "", "", g_iPin, g_iMythXmlConnectTimeout))
  {
	m_CurStatus = STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = STATUS_OK;

  g_bCreated = true;
  return m_CurStatus;
}

void Destroy()
{
  if (g_bCreated)
  {
	  delete MythXmlApi;
	  MythXmlApi = NULL;
    g_bCreated = false;
  }
  m_CurStatus = STATUS_UNKNOWN;
}

ADDON_STATUS GetStatus()
{
  return m_CurStatus;
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
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_szHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*) settingValue;
    if (tmp_sHostname != g_szHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "mythXMLPort")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iMythXmlPort, *(int*) settingValue);
    if (g_iMythXmlPort != *(int*) settingValue)
    {
	  g_iMythXmlPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "mythXMLTimeout")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'mythXMLTimeout' from %u to %u", g_iMythXmlConnectTimeout, *(int*) settingValue);
    if (g_iMythXmlConnectTimeout / 1000  != *(int*) settingValue)
    {
	  g_iMythXmlConnectTimeout = *(int*) settingValue;
	  g_iMythXmlConnectTimeout *= 1000;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "pin")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'pin' from %u to %u", g_iPin, *(int*) settingValue);
    if (g_iPin != *(int*) settingValue)
    {
	  g_iPin = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  return STATUS_OK;
}

void Stop()
{
  return;
}

void FreeSettings()
{
  return;
}


/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = false;
  props->SupportTimers             = false;
  props->SupportTV                 = false;
  props->SupportRadio              = false;
  props->SupportChannelSettings    = false;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;
  props->HandleInputStream         = false;
  props->HandleDemuxing            = false;
  props->SupportChannelScan        = false;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* props)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

const char * GetBackendName()
{
  return "";
}

const char * GetBackendVersion()
{
  return "";
}

const char * GetConnectionString()
{
  return "";
}

PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogChannelScan()
{
  return PVR_ERROR_NOT_POSSIBLE;
}

PVR_ERROR MenuHook(const PVR_MENUHOOK &menuhook)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
	if (MythXmlApi == NULL)
	    return PVR_ERROR_SERVER_ERROR;

	return MythXmlApi->requestEPGForChannel(handle, channel, start, end);
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
	if (MythXmlApi == NULL)
		return PVR_ERROR_SERVER_ERROR;

	return MythXmlApi->getNumChannels();
}

PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
	if (MythXmlApi == NULL)
			return PVR_ERROR_SERVER_ERROR;

	return MythXmlApi->requestChannelList(handle, radio);
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
  return 0;
}

PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
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
  return 0;
}

PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  return false;
}

void CloseLiveStream()
{
  return;
}

int ReadLiveStream(unsigned char* buf, int buf_size)
{
	return -1;
}

int GetCurrentClientChannel()
{
  return m_iCurrentChannel;
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  return false;
}

PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR Secondary Stream Functions        **/

bool SwapLiveTVSecondaryStream()
{
  return false;
}

bool OpenSecondaryStream(const PVR_CHANNEL &channelinfo)
{
  return false;
}

void CloseSecondaryStream()
{
  return;
}

int ReadSecondaryStream(unsigned char* buf, int buf_size)
{
  return 0;
}


/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  return false;
}

void CloseRecordedStream(void)
{
  return;
}

int ReadRecordedStream(unsigned char* buf, int buf_size)
{
  return 0;
}

long long SeekRecordedStream(long long pos, int whence)
{
	return -1;
}

long long PositionRecordedStream(void)
{
  return -1;
}

long long LengthRecordedStream(void)
{
	return 0;
}


/** UNUSED API FUNCTIONS */
DemuxPacket* DemuxRead() { return NULL; }
void DemuxAbort() {}
void DemuxReset() {}
void DemuxFlush() {}
long long SeekLiveStream(long long pos, int whence) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo) { return ""; }

} //end extern "C"
