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
#include "HTSPSession.h"
#include "HTSPDemux.h"

using namespace std;

//cPVRClientTvheadend *g_client = NULL;
bool m_bCreated               = false;
ADDON_STATUS curStatus        = STATUS_UNKNOWN;
int g_clientID                = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
CStdString g_szHostname       = DEFAULT_HOST;
int g_iPort                   = DEFAULT_PORT;
CStdString g_szUsername       = "";
CStdString g_szPassword       = "";
CStdString g_szUserPath       = "";
CStdString g_szClientPath     = "";
cHelper_libXBMC_addon *XBMC   = NULL;
cHelper_libXBMC_pvr   *PVR    = NULL;
cHTSPDemux *HTSPDemuxer       = NULL;

bool CheckConnection()
{
  return true;
}

extern "C" {

/***********************************************************
 * Standart AddOn related public library functions
 ***********************************************************/

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

  XBMC->Log(LOG_DEBUG, "Creating Tvheadend PVR-Client");

  curStatus      = STATUS_UNKNOWN;
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
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_szHostname = DEFAULT_HOST;
  }
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("user", buffer))
    g_szUsername = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'user' setting");
    g_szUsername = "";
  }
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("pass", buffer))
    g_szPassword = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'pass' setting");
    g_szPassword = "";
  }
  free (buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '%i' as default", DEFAULT_PORT);
    g_iPort = DEFAULT_PORT;
  }

  /* Create connection to streamdev-server */
  curStatus = STATUS_LOST_CONNECTION;
  if(!g_pSession.Connect(g_szHostname, g_iPort))
    return curStatus;

  if(g_pSession.GetProtocol() < 2)
  {
    XBMC->Log(LOG_ERROR, "Incompatible protocol version %d", g_pSession.GetProtocol());
    return curStatus;
  }

  if(!g_szUsername.IsEmpty())
    g_pSession.Auth(g_szUsername, g_szPassword);

  if(!g_pSession.SendEnableAsync())
    return curStatus;

  if (!g_pSession.Start())
    return curStatus;

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
  if (m_bCreated)
  {
    g_pSession.Stop();
    g_pSession.Close();
    m_bCreated = false;
  }
  curStatus = STATUS_UNKNOWN;
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
  else if (str == "user")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'user'");
    string tmp_sUsername = g_szUsername;
    g_szUsername = (const char*) settingValue;
    if (tmp_sUsername != g_szUsername)
      return STATUS_NEED_RESTART;
  }
  else if (str == "pass")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'pass'");
    string tmp_sPassword = g_szPassword;
    g_szPassword = (const char*) settingValue;
    if (tmp_sPassword != g_szPassword)
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
  props->SupportEPG                = false;
  props->SupportRecordings         = false;
  props->SupportTimers             = false;
  props->SupportTV                 = true;
  props->SupportRadio              = false;
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
  static CStdString BackendName = g_pSession.GetServerName();
  return BackendName.c_str();
}

const char * GetBackendVersion()
{
  static CStdString BackendVersion;
  BackendVersion.Format("%s (Protocol: %i)", g_pSession.GetVersion(), g_pSession.GetProtocol());
  return BackendVersion.c_str();
}

const char * GetConnectionString()
{
  static CStdString ConnectionString;
  ConnectionString.Format("%s:%i%s", g_szHostname.c_str(), g_iPort, CheckConnection() ? "" : " (Not connected!)");
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  if (g_pSession.GetDriveSpace(total, used))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  if (g_pSession.GetTime(localTime, gmtOffset))
    return PVR_ERROR_NO_ERROR;

  return PVR_ERROR_SERVER_ERROR;
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
  return PVR_ERROR_SERVER_ERROR;//g_client->RequestEPGForChannel(handle, channel, start, end);
}


/*******************************************/
/** PVR Bouquets Functions                **/

int GetNumBouquets()
{
  return (int)(g_pSession.GetTags().size());
}

PVR_ERROR RequestBouquetsList(PVRHANDLE handle, int radio)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetNumChannels()
{
  return (int)(g_pSession.GetChannels().size());
}

PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  if (radio)
    return PVR_ERROR_NO_ERROR;

  SChannels channels = g_pSession.GetChannels();
  for(SChannels::iterator it = channels.begin(); it != channels.end(); ++it)
  {
    SChannel& channel = it->second;

    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(tag));
    tag.uid           = channel.id;
    tag.number        = channel.id;//num;
    tag.name          = channel.name.c_str();
    tag.callsign      = channel.name.c_str();
    tag.input_format  = "";

    char url[128];
    sprintf(url, "htsp://%s:%d/tags/0/%d.ts", g_szHostname.c_str(), g_iPort, channel.id);
    tag.stream_url  = "";
    tag.bouquet     = 0;

    fprintf(stderr, "%s - %s - %i\n", __PRETTY_FUNCTION__, channel.name.c_str(), tag.bouquet);

    PVR->TransferChannelEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
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

long long PositionRecordedStream(void)
{
  return -1;
}

long long LengthRecordedStream(void)
{
  printf("%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR_SERVER_ERROR; // TODO g_client->LengthRecordedStream();
}

/** UNUSED API FUNCTIONS */
void DemuxReset(){}
void DemuxFlush(){}
int ReadLiveStream(unsigned char* buf, int buf_size) { return 0; }
long long SeekLiveStream(long long pos, int whence) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo) { return ""; }

}
