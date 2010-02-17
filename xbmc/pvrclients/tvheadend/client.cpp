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
#include "pvrclient-tvheadend.h"

using namespace std;

cPVRClientTvheadend *g_client = NULL;
bool m_bCreated         = false;
ADDON_STATUS curStatus  = STATUS_UNKNOWN;
int g_clientID          = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string m_sHostname = DEFAULT_HOST;
int m_iPort             = DEFAULT_PORT;
bool m_bOnlyFTA         = DEFAULT_FTA_ONLY;
bool m_bRadioEnabled    = DEFAULT_RADIO;
bool m_bCharsetConv     = DEFAULT_CHARCONV;
int m_iConnectTimeout   = DEFAULT_TIMEOUT;
bool m_bNoBadChannels   = DEFAULT_BADCHANNELS;
bool m_bHandleMessages  = DEFAULT_HANDLE_MSG;
std::string g_szUserPath    = "";
std::string g_szClientPath  = "";

extern "C" {

/***********************************************************
 * Standart AddOn related public library functions
 ***********************************************************/

ADDON_STATUS Create(void* hdl, void* props)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  if (!hdl || !props)
    return STATUS_UNKNOWN;

  PVR_PROPS* pvrprops = (PVR_PROPS*)props;

  XBMC_register_me(hdl);
  PVR_register_me(hdl);

  //XBMC_log(LOG_DEBUG, "Creating Tvheadend PVR-Client");

  curStatus      = STATUS_UNKNOWN;
  g_client       = new cPVRClientTvheadend();
  g_clientID     = pvrprops->clientID;
  g_szUserPath   = pvrprops->userpath;
  g_szClientPath = pvrprops->clientpath;

  /* Read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC_get_setting("host", buffer))
    m_sHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    m_sHostname = DEFAULT_HOST;
  }
  free (buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC_get_setting("port", &m_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '9982' as default");
    m_iPort = DEFAULT_PORT;
  }

  printf("host %s, port %d\n", m_sHostname.c_str(), m_iPort);

  /* Read setting "ftaonly" from settings.xml */
  if (!XBMC_get_setting("ftaonly", &m_bOnlyFTA))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'ftaonly' setting, falling back to 'false' as default");
    m_bOnlyFTA = DEFAULT_FTA_ONLY;
  }

  /* Read setting "useradio" from settings.xml */
  if (!XBMC_get_setting("useradio", &m_bRadioEnabled))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'useradio' setting, falling back to 'true' as default");
    m_bRadioEnabled = DEFAULT_RADIO;
  }

  /* Read setting "convertchar" from settings.xml */
  if (!XBMC_get_setting("convertchar", &m_bCharsetConv))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'convertchar' setting, falling back to 'false' as default");
    m_bCharsetConv = DEFAULT_CHARCONV;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC_get_setting("timeout", &m_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    m_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* Read setting "ignorechannels" from settings.xml */
  if (!XBMC_get_setting("ignorechannels", &m_bNoBadChannels))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'ignorechannels' setting, falling back to 'true' as default");
    m_bNoBadChannels = DEFAULT_BADCHANNELS;
  }

  /* Read setting "ignorechannels" from settings.xml */
  if (!XBMC_get_setting("handlemessages", &m_bHandleMessages))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'handlemessages' setting, falling back to 'true' as default");
    m_bHandleMessages = DEFAULT_HANDLE_MSG;
  }

  /* Create connection to streamdev-server */
  if (!g_client->Connect(m_sHostname, m_iPort))
    curStatus = STATUS_LOST_CONNECTION;
  else
    curStatus = STATUS_OK;

  m_bCreated = true;
  return curStatus;
}

ADDON_STATUS GetStatus()
{
  return curStatus;
}

void Destroy()
{

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
    XBMC_log(LOG_INFO, "Changed Setting 'host' from %s to %s", m_sHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = m_sHostname;
    m_sHostname = (const char*) settingValue;
    if (tmp_sHostname != m_sHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'port' from %u to %u", m_iPort, *(int*) settingValue);
    if (m_iPort != *(int*) settingValue)
    {
      m_iPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "ftaonly")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'ftaonly' from %u to %u", m_bOnlyFTA, *(bool*) settingValue);
    m_bOnlyFTA = *(bool*) settingValue;
  }
  else if (str == "useradio")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'useradio' from %u to %u", m_bRadioEnabled, *(bool*) settingValue);
    m_bRadioEnabled = *(bool*) settingValue;
  }
  else if (str == "convertchar")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'convertchar' from %u to %u", m_bCharsetConv, *(bool*) settingValue);
    m_bCharsetConv = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'timeout' from %u to %u", m_iConnectTimeout, *(int*) settingValue);
    m_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "ignorechannels")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'ignorechannels' from %u to %u", m_bNoBadChannels, *(bool*) settingValue);
    m_bNoBadChannels = *(bool*) settingValue;
  }
  else if (str == "handlemessages")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'handlemessages' from %u to %u", m_bHandleMessages, *(bool*) settingValue);
    m_bHandleMessages = *(bool*) settingValue;
  }

  return STATUS_OK;
}

void Remove()
{
  if (m_bCreated)
  {
    // TODO g_client->Disconnect();

    delete g_client;
    g_client = NULL;

    m_bCreated = false;
  }
  curStatus = STATUS_UNKNOWN;
}

void FreeSettings()
{

}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return g_client->GetProperties(props);
}

PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* props)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return g_client->GetStreamProperties(props);
}

const char bn[] = "Tvheadend";
const char * GetBackendName()
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return bn; // TODO g_client->GetBackendName();
}

const char * GetBackendVersion()
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return bn; // TODO g_client->GetBackendVersion();
}

const char * GetConnectionString()
{
  return g_client->GetConnectionString();
}

PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->GetDriveSpace(total, used);
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  printf("%s\n", __PRETTY_FUNCTION__);

  *localTime = time(NULL);
  *gmtOffset = 1;
  return PVR_ERROR_NO_ERROR; // TODO g_client->GetTvheadendTime(localTime, gmtOffset);
}

PVR_ERROR MenuHook(const PVR_MENUHOOK &menuhook)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  return g_client->RequestEPGForChannel(handle, channel, start, end);
}


/*******************************************/
/** PVR Bouquets Functions                **/

int GetNumBouquets()
{
  return g_client->GetNumBouquets();
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
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->GetNumRecordings();
}

PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->RequestRecordingsList(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->DeleteRecording(recinfo);
}

PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->RenameRecording(recinfo, newname);
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
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->GetNumTimers();
}

PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->RequestTimerList(handle);
}

PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->AddTimer(timerinfo);
}

PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->DeleteTimer(timerinfo, force);
}

PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->RenameTimer(timerinfo, newname);
}

PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->UpdateTimer(timerinfo);
}


/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  return g_client->OpenLiveStream(channelinfo);
}

void CloseLiveStream()
{
  g_client->CloseLiveStream();
}

int ReadLiveStream(unsigned char* buf, int buf_size)
{
  return g_client->ReadLiveStream(buf, buf_size);
}

long long SeekLiveStream(long long pos, int whence)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return -1;
}

long long LengthLiveStream(void)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return -1;
}

int GetCurrentClientChannel()
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return g_client->SwitchChannel(channelinfo);
}

PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->SignalQuality(qualityinfo);
}


/*******************************************/
/** PVR Secondary Stream Functions        **/

bool OpenSecondaryStream(const PVR_CHANNEL &channelinfo)
{
  return false;
}

void CloseSecondaryStream()
{

}

int ReadSecondaryStream(unsigned char* buf, int buf_size)
{
  return 0;
}


/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->OpenRecordedStream(recinfo);
}

void CloseRecordedStream(void)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return ; // TODO g_client->CloseRecordedStream();
}

int ReadRecordedStream(unsigned char* buf, int buf_size)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->ReadRecordedStream(buf, buf_size);
}

long long SeekRecordedStream(long long pos, int whence)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->SeekRecordedStream(pos, whence);
}

long long LengthRecordedStream(void)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->LengthRecordedStream();
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  return channelinfo.stream_url;
}
}
