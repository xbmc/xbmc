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
#include "tools.h"
#include "vtptransceiver.h"
#include "channelscan.h"
#include "ringbuffer.h"
#include "xbmc_pvr_dll.h"

using namespace std;

class cTSBuffer;
class cDataResp;

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
CStdString   g_szHostname             = DEFAULT_HOST;         ///< The Host name or IP of the VDR
int          g_iPort                  = DEFAULT_PORT;         ///< The VTP Port of the VDR (default is 2004)
int          g_iPriority              = DEFAULT_PRIORITY;     ///< The Priority this client have in response to other clients
int          g_iConnectTimeout        = DEFAULT_TIMEOUT;      ///< The Socket connection timeout
bool         g_bOnlyFTA               = DEFAULT_FTA_ONLY;     ///< Send only Free-To-Air Channels inside Channel list to XBMC
bool         g_bRadioEnabled          = DEFAULT_RADIO;        ///< Send also Radio channels list to XBMC
bool         g_bCharsetConv           = DEFAULT_CHARCONV;     ///< Convert VDR's incoming strings to UTF8 character set
bool         g_bNoBadChannels         = DEFAULT_BADCHANNELS;  ///< Ignore channels without a PID, APID and DPID
bool         g_bHandleMessages        = DEFAULT_HANDLE_MSG;   ///< Send VDR's OSD status messages to XBMC OSD
bool         g_bUseRecordingsDir      = DEFAULT_USE_REC_DIR;  ///< Use a normal directory if true for recordings
CStdString   g_szRecordingsDir        = DEFAULT_REC_DIR;      ///< The path to the recordings directory
//
///* Client member variables */
uint64_t     m_currentPlayingRecordBytes;
uint32_t     m_currentPlayingRecordFrames;
uint64_t     m_currentPlayingRecordPosition;
uint64_t     m_recordingTotalBytesReaded;
bool         m_recordingFirstRead;
cPoller     *m_recordingPoller        = NULL;
char         m_noSignalStreamData[ 6 + 0xffff ];
long         m_noSignalStreamSize     = 0;
long         m_noSignalStreamReadPos  = 0;
bool         m_bPlayingNoSignal       = false;
int          m_iCurrentChannel        = 1;
ADDON_STATUS m_CurStatus              = STATUS_UNKNOWN;
cTSBuffer   *m_TSBuffer               = NULL;
cDataResp   *m_pDataResponse          = NULL;
bool         g_bCreated               = false;
int          g_iClientID              = -1;
CStdString   g_szUserPath             = "";
CStdString   g_szClientPath           = "";
cHelper_libXBMC_addon *XBMC           = NULL;
cHelper_libXBMC_pvr   *PVR            = NULL;



/***********************************************************
 * Internal functions
 ***********************************************************/

/// Derived cDevice classes that can receive channels will have to provide
/// Transport Stream (TS) packets one at a time. cTSBuffer implements a
/// simple buffer that allows the device to read a larger amount of data
/// from the driver with each call to Read(), thus avoiding the overhead
/// of getting each TS packet separately from the driver. It also makes
/// sure the returned data points to a TS packet and automatically
/// re-synchronizes after broken packets.

class cTSBuffer : public cThread
{
private:
  int f;
  bool delivered;
  cRingBufferLinear *ringBuffer;
  virtual void Action(void);
public:
  cTSBuffer(int File, int Size);
  ~cTSBuffer();
  unsigned char *Get(void);
};

cTSBuffer::cTSBuffer(int File, int Size)
{
  SetDescription("TS Live buffer");
  f           = File;
  delivered   = false;
  ringBuffer  = new cRingBufferLinear(Size, TS_SIZE, true, "TS");
  ringBuffer->SetTimeouts(100, 100);
  Start();
}

cTSBuffer::~cTSBuffer()
{
  Cancel(3);
  delete ringBuffer;
}

void cTSBuffer::Action(void)
{
  if (ringBuffer)
  {
    bool firstRead = true;
    cPoller Poller(f);
    while (Running())
    {
      if (firstRead || Poller.Poll(100))
      {
        firstRead = false;
        int r = ringBuffer->Read(f);
        if (r < 0 && FATALERRNO)
        {
          if (errno == EOVERFLOW)
            XBMC->Log(LOG_ERROR, "driver buffer overflow");
          else
          {
            XBMC->Log(LOG_ERROR, "ERROR (%s,%d): %m", __FILE__, __LINE__);
            break;
          }
        }
      }
    }
  }
}

unsigned char *cTSBuffer::Get(void)
{
  int Count = 0;
  if (delivered)
  {
    ringBuffer->Del(TS_SIZE);
    delivered = false;
  }
  unsigned char *p = ringBuffer->Get(Count);
  if (p && Count >= TS_SIZE)
  {
    if (*p != TS_SYNC_BYTE)
    {
      for (int i = 1; i < Count; i++)
      {
        if (p[i] == TS_SYNC_BYTE)
        {
          Count = i;
          break;
        }
      }
      ringBuffer->Del(Count);
      XBMC->Log(LOG_ERROR, "skipped %d bytes to sync on TS packet", Count);
      return NULL;
    }
    delivered = true;
    return p;
  }
  return NULL;
}

class cDataResp : public cThread
{
private:
  virtual void Action(void);
  bool VDRToXBMCCommand(char *Cmd);
  bool CallBackMODT(const char *Option);
  bool CallBackDELT(const char *Option);
  bool CallBackADDT(const char *Option);
  bool CallBackSMSG(const char *Option);
  bool CallBackIMSG(const char *Option);
  bool CallBackWMSG(const char *Option);
  bool CallBackEMSG(const char *Option);

public:
  cDataResp();
  ~cDataResp();
};


cDataResp::cDataResp()
{
  /* Open Streamdev-Server VTP-Connection to VDR Backend Server */
  if (!VTPTransceiver.CheckConnection())
    return;

  if (!VTPTransceiver.CreateDataConnection(siDataRespond))
  {
    XBMC->Log(LOG_ERROR, "Couldn't create socket for data response");
    return;
  }

  SetDescription("VDR Data Response");
  Start();
}

cDataResp::~cDataResp()
{
  Cancel(3);
  VTPTransceiver.CloseDataConnection(siDataRespond);
}

void cDataResp::Action(void)
{
  char   		      data[1024];
  fd_set          set_r, set_e;
  struct timeval  tv;
  int             ret;
  memset(data,0,1024);

  while (Running())
  {
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(VTPTransceiver.DataSocket(siDataRespond), &set_r);
    FD_SET(VTPTransceiver.DataSocket(siDataRespond), &set_e);
    ret = __select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
    if (ret < 0)
    {
      XBMC->Log(LOG_ERROR, "CallbackRcvThread - select failed");
      continue;
    }
    else if (ret == 0)
      continue;

    ret = __recv(VTPTransceiver.DataSocket(siDataRespond), (char*)data, sizeof(data), 0);
    if (ret < 0)
    {
      XBMC->Log(LOG_ERROR, "CallbackRcvThread - receive failed");
      continue;
    }
    else if (ret == 0)
      continue;

    /* Check the received command and perform associated action*/
    VDRToXBMCCommand(data);
    memset(data,0,1024);
  }
  return;
}

bool cDataResp::VDRToXBMCCommand(char *Cmd)
{
	char *param = NULL;

	if (Cmd != NULL)
	{
		if ((param = strchr(Cmd, ' ')) != NULL)
			*(param++) = '\0';
		else
			param = Cmd + strlen(Cmd);
	}
	else
	{
    XBMC->Log(LOG_ERROR, "VDRToXBMCCommand - called without command from %s:%d", g_szHostname.c_str(), g_iPort);
		return false;
	}

/* !!! DISABLED UNTIL STREAMDEV HAVE SUPPORT FOR IT INSIDE CVS !!!
	if      (strcasecmp(Cmd, "MODT") == 0) return CallBackMODT(param);
	else if (strcasecmp(Cmd, "DELT") == 0) return CallBackDELT(param);
	else if (strcasecmp(Cmd, "ADDT") == 0) return CallBackADDT(param);
*/

	if (strcasecmp(Cmd, "IMSG") == 0) return CallBackIMSG(param);
	else if (strcasecmp(Cmd, "WMSG") == 0) return CallBackWMSG(param);
	else if (strcasecmp(Cmd, "EMSG") == 0) return CallBackEMSG(param);
	else
	{
    XBMC->Log(LOG_ERROR, "VDRToXBMCCommand - Unkown respond command %s", Cmd);
		return false;
	}
}

bool cDataResp::CallBackMODT(const char *Option)
{
//  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
  return true;
}

bool cDataResp::CallBackDELT(const char *Option)
{
//  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
  return true;
}

bool cDataResp::CallBackADDT(const char *Option)
{
//  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
  return true;
}

bool cDataResp::CallBackIMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(text);
    XBMC->QueueNotification(QUEUE_INFO, text.c_str());
    return true;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "CallBackIMSG - missing option");
    return false;
  }
}

bool cDataResp::CallBackWMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(text);
    XBMC->QueueNotification(QUEUE_WARNING, text.c_str());
    return true;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "CallBackWMSG - missing option");
    return false;
  }
}

bool cDataResp::CallBackEMSG(const char *Option)
{
  if (*Option)
  {
    CStdString text = Option;
    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(text);
    XBMC->QueueNotification(QUEUE_ERROR, text.c_str());
    return true;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "CallBackEMSG - missing option");
    return false;
  }
}


bool readNoSignalStream()
{
  CStdString noSignalFileName = g_szClientPath + "/resources/data/noSignal.mpg";

  FILE *const f = fopen(noSignalFileName.c_str(), "rb");
  if (f)
  {
    m_noSignalStreamSize = fread(&m_noSignalStreamData[0] + 9, 1, sizeof (m_noSignalStreamData) - 9 - 9 - 4, f);
    if (m_noSignalStreamSize == sizeof (m_noSignalStreamData) - 9 - 9 - 4)
    {
      XBMC->Log(LOG_ERROR, "readNoSignalStream - '%s' exeeds limit of %ld bytes!", noSignalFileName.c_str(), (long)(sizeof (m_noSignalStreamData) - 9 - 9 - 4 - 1));
    }
    else if (m_noSignalStreamSize > 0)
    {
      m_noSignalStreamData[ 0 ] = 0x00;
      m_noSignalStreamData[ 1 ] = 0x00;
      m_noSignalStreamData[ 2 ] = 0x01;
      m_noSignalStreamData[ 3 ] = 0xe0;
      m_noSignalStreamData[ 4 ] = (m_noSignalStreamSize + 3) >> 8;
      m_noSignalStreamData[ 5 ] = (m_noSignalStreamSize + 3) & 0xff;
      m_noSignalStreamData[ 6 ] = 0x80;
      m_noSignalStreamData[ 7 ] = 0x00;
      m_noSignalStreamData[ 8 ] = 0x00;
      m_noSignalStreamSize += 9;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x01;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0xe0;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x07;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x80;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x00;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0x01;
      m_noSignalStreamData[ m_noSignalStreamSize++ ] = 0xb7;
    }
    fclose(f);
    return true;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "readNoSignalStream - couldn't open '%s'!", noSignalFileName.c_str());
  }

  return false;
}

int writeNoSignalStream(unsigned char* buf, int buf_size)
{
  int sizeToWrite = m_noSignalStreamSize-m_noSignalStreamReadPos;
  m_bPlayingNoSignal = true;
  if (buf_size > sizeToWrite)
  {
    memcpy(buf, m_noSignalStreamData+m_noSignalStreamReadPos, sizeToWrite);
    m_noSignalStreamReadPos = 0;
    return sizeToWrite;
  }
  else
  {
    memcpy(buf, m_noSignalStreamData+m_noSignalStreamReadPos, buf_size);
    m_noSignalStreamReadPos += buf_size;
    return buf_size;
  }
}

extern "C" {

/***********************************************************
 * Standart AddOn related public library functions
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

  XBMC->Log(LOG_DEBUG, "Creating VDR PVR-Client");

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
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    g_szHostname = DEFAULT_HOST;
  }
  free (buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '2004' as default");
    g_iPort = DEFAULT_PORT;
  }

  /* Read setting "priority" from settings.xml */
  if (!XBMC->GetSetting("priority", &g_iPriority))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'priority' setting, falling back to %i as default", DEFAULT_PRIORITY);
    g_iPriority = DEFAULT_PRIORITY;
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

  /* Read setting "ignorechannels" from settings.xml */
  if (!XBMC->GetSetting("ignorechannels", &g_bNoBadChannels))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'ignorechannels' setting, falling back to 'true' as default");
    g_bNoBadChannels = DEFAULT_BADCHANNELS;
  }

  /* Read setting "ignorechannels" from settings.xml */
  if (!XBMC->GetSetting("handlemessages", &g_bHandleMessages))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'handlemessages' setting, falling back to 'true' as default");
    g_bHandleMessages = DEFAULT_HANDLE_MSG;
  }

  /* Read setting "ignorechannels" from settings.xml */
  if (!XBMC->GetSetting("usedirectory", &g_bUseRecordingsDir))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'usedirectory' setting, falling back to 'false' as default");
    g_bUseRecordingsDir = DEFAULT_USE_REC_DIR;
  }

  if (g_bUseRecordingsDir)
  {
    /* Read setting "recordingdir" from settings.xml */
    buffer = (char*) malloc (2048);
    buffer[0] = 0; /* Set the end of string */

    if (XBMC->GetSetting("recordingdir", buffer))
      g_szRecordingsDir = buffer;
    else
    {
      /* If setting is unknown fallback to defaults */
      XBMC->Log(LOG_ERROR, "Couldn't get 'recordingdir' setting, directory not set");
      g_szRecordingsDir = DEFAULT_REC_DIR;
      g_bUseRecordingsDir = false;
    }
    free (buffer);
  }

  /* Create connection to streamdev-server */
  if (!VTPTransceiver.CheckConnection())
  {
    m_CurStatus = STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  /* Check VDR streamdev is patched by calling a newly added command */
  if (VTPTransceiver.GetNumChannels() == -1)
  {
    XBMC->Log(LOG_ERROR, "PCRClient-vdr: Detected unsupported Streamdev-Version");
    m_CurStatus = STATUS_UNKNOWN;
    return STATUS_UNKNOWN;
  }
  m_CurStatus = STATUS_OK;

  readNoSignalStream();
	if (g_bHandleMessages)
    m_pDataResponse = new cDataResp();

  g_bCreated = true;
  return m_CurStatus;
}

void Destroy()
{
  if (g_bCreated)
  {
    DELETENULL(m_TSBuffer);
    DELETENULL(m_pDataResponse);
    VTPTransceiver.Quit();
    VTPTransceiver.Reset();
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
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iPort, *(int*) settingValue);
    if (g_iPort != *(int*) settingValue)
    {
      g_iPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "priority")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'priority' from %u to %u", g_iPriority, *(int*) settingValue);
    g_iPriority = *(int*) settingValue;
  }
  else if (str == "ftaonly")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'ftaonly' from %u to %u", g_bOnlyFTA, *(bool*) settingValue);
    g_bOnlyFTA = *(bool*) settingValue;
  }
  else if (str == "useradio")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'useradio' from %u to %u", g_bRadioEnabled, *(bool*) settingValue);
    g_bRadioEnabled = *(bool*) settingValue;
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
  else if (str == "ignorechannels")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'ignorechannels' from %u to %u", g_bNoBadChannels, *(bool*) settingValue);
    g_bNoBadChannels = *(bool*) settingValue;
  }
  else if (str == "handlemessages")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'handlemessages' from %u to %u", g_bHandleMessages, *(bool*) settingValue);
    g_bHandleMessages = *(bool*) settingValue;
  }
  else if (str == "usedirectory")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'usedirectory' from %u to %u", g_bUseRecordingsDir, *(bool*) settingValue);
    g_bUseRecordingsDir = *(bool*) settingValue;
  }
  else if (str == "recordingdir")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'recordingdir' from %s to %s", g_szRecordingsDir.c_str(), (const char*) settingValue);
    g_szRecordingsDir = (const char*) settingValue;
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
  props->SupportRadio              = g_bRadioEnabled;
  props->SupportChannelSettings    = false;
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

const char * GetBackendName()
{
  static CStdString BackendName = VTPTransceiver.GetBackendName();
  return BackendName.c_str();
}

const char * GetBackendVersion()
{
  static CStdString BackendName = VTPTransceiver.GetBackendVersion();
  return BackendName.c_str();
}

const char * GetConnectionString()
{
  static CStdString ConnectionString;
  ConnectionString.Format("%s:%i%s", g_szHostname.c_str(), g_iPort, VTPTransceiver.CheckConnection() ? "" : " (Not connected!)");
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  return VTPTransceiver.GetDriveSpace(total, used);
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  return VTPTransceiver.GetBackendTime(localTime, gmtOffset);
}

PVR_ERROR DialogChannelScan()
{
  cVDRChannelScan *VDRChannelScan = new cVDRChannelScan;
  if (VDRChannelScan->Possible())
  {

    return PVR_ERROR_NO_ERROR;
  }
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
  return VTPTransceiver.RequestEPGForChannel(channel, handle, start, end);
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
  return VTPTransceiver.GetNumChannels();
}

PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
  return VTPTransceiver.RequestChannelList(handle, radio);
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
  return VTPTransceiver.GetNumRecordings();
}

PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  return VTPTransceiver.RequestRecordingsList(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  return VTPTransceiver.DeleteRecording(recinfo);
}

PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  return VTPTransceiver.RenameRecording(recinfo, newname);
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
  return VTPTransceiver.GetNumTimers();
}

PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  return VTPTransceiver.RequestTimerList(handle);
}

PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  return VTPTransceiver.AddTimer(timerinfo);
}

PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  return VTPTransceiver.DeleteTimer(timerinfo, force);
}

PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  return VTPTransceiver.RenameTimer(timerinfo, newname);
}

PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  return VTPTransceiver.UpdateTimer(timerinfo);
}


/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  if (!VTPTransceiver.CheckConnection())
    return false;

  if (!VTPTransceiver.ProvidesChannel(channelinfo.number, g_iPriority))
  {
    XBMC->Log(LOG_ERROR, "VDR does not provide channel %i", channelinfo.number);
    return false;
  }

  if (!VTPTransceiver.SetChannelDevice(channelinfo.number))
  {
    XBMC->Log(LOG_ERROR, "Could't tune to channel %i", channelinfo.number);
    return false;
  }

  if (!VTPTransceiver.CreateDataConnection(siLive))
  {
    XBMC->Log(LOG_ERROR, "Could't create connection to VDR for Live streaming");
    return false;
  }

  m_iCurrentChannel       = channelinfo.number;
  m_noSignalStreamReadPos = 0;
  m_bPlayingNoSignal      = false;
  m_TSBuffer              = new cTSBuffer(VTPTransceiver.DataSocket(siLive), MEGABYTE(2));
  return true;
}

void CloseLiveStream()
{
  if (!VTPTransceiver.CheckConnection())
    XBMC->Log(LOG_DEBUG, "CloseLiveStream(): Control connection gone !");

  VTPTransceiver.CloseDataConnection(siLive);
  m_iCurrentChannel = 1;
  DELETENULL(m_TSBuffer);
  return;
}

int ReadLiveStream(unsigned char* buf, int buf_size)
{
  bool tryReconnect         = true;
  int TSReadNeeded          = buf_size / TS_SIZE;
  int TSReadDone            = 0;
  static int read_timeouts  = 0;

  while (TSReadDone < TSReadNeeded)
  {
    if (!m_TSBuffer)
      return writeNoSignalStream(buf, buf_size);

    unsigned char *Data = m_TSBuffer->Get();
    if (!Data)
    {
      if (m_bPlayingNoSignal)
        return writeNoSignalStream(buf, buf_size);

      if (VTPTransceiver.DataSocket(siLive) == INVALID_SOCKET)
      {
        if (tryReconnect)
        {
          tryReconnect = false;
          XBMC->Log(LOG_INFO, "Streaming connections lost during ReadLiveStream, trying reconnect");

          if (!VTPTransceiver.ProvidesChannel(m_iCurrentChannel, g_iPriority))
            return -1;

          DELETENULL(m_TSBuffer);
          VTPTransceiver.CloseDataConnection(siLive);

          if (!VTPTransceiver.SetChannelDevice(m_iCurrentChannel))
            continue; /* Continue here to read NoSignal stream */

          if (!VTPTransceiver.CreateDataConnection(siLive))
            continue; /* Continue here to read NoSignal stream */

          m_noSignalStreamReadPos = 0;
          m_bPlayingNoSignal      = false;
          m_TSBuffer              = new cTSBuffer(VTPTransceiver.DataSocket(siLive), MEGABYTE(2));
          continue;
        }
        XBMC->Log(LOG_ERROR, "Reconnect in ReadLiveStream not possible");
        continue; /* Continue here to read NoSignal stream */
      }

      if (read_timeouts > 20)
      {
        XBMC->Log(LOG_INFO, "No data in 2 seconds, queuing no signal image");
        read_timeouts = 0;
        return writeNoSignalStream(buf, buf_size);
      }
      read_timeouts++;
      usleep(10*1000);

      continue;
    }
    memcpy(buf+TSReadDone*TS_SIZE, Data, TS_SIZE);
    TSReadDone++;
  }
  read_timeouts = 0;
  m_bPlayingNoSignal = false;
  return TSReadDone*TS_SIZE;
}

int GetCurrentClientChannel()
{
  return m_iCurrentChannel;
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  if (!VTPTransceiver.CheckConnection())
    return false;

  if (!VTPTransceiver.ProvidesChannel(channelinfo.number, g_iPriority))
  {
    XBMC->Log(LOG_ERROR, "VDR does not provide channel %i", channelinfo.number);
    return false;
  }

  DELETENULL(m_TSBuffer);
  VTPTransceiver.CloseDataConnection(siLive);

  if (!VTPTransceiver.SetChannelDevice(channelinfo.number))
  {
    XBMC->Log(LOG_ERROR, "Could't tune to channel %i", channelinfo.number);
    return false;
  }

  if (!VTPTransceiver.CreateDataConnection(siLive))
  {
    XBMC->Log(LOG_ERROR, "Could't create connection to VDR for Live streaming");
    return false;
  }

  m_iCurrentChannel       = channelinfo.number;
  m_noSignalStreamReadPos = 0;
  m_bPlayingNoSignal      = false;
  m_TSBuffer              = new cTSBuffer(VTPTransceiver.DataSocket(siLive), MEGABYTE(2));
  return true;
}

PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  return VTPTransceiver.SignalQuality(qualityinfo, m_iCurrentChannel);
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
  if (!VTPTransceiver.CheckConnection())
    return false;

  if (!VTPTransceiver.SetRecordingIndex(recinfo.index))
  {
    XBMC->Log(LOG_ERROR, "Could't open recording %i", recinfo.index);
    return false;
  }

  if (!VTPTransceiver.CreateDataConnection(siReplay))
  {
    XBMC->Log(LOG_ERROR, "Could't create connection to VDR for recording streaming");
    return false;
  }

  m_currentPlayingRecordPosition = 0;
  m_recordingTotalBytesReaded    = 0;
  m_recordingPoller              = new cPoller(VTPTransceiver.DataSocket(siReplay));
  m_recordingFirstRead           = true;
  return true;
}

void CloseRecordedStream(void)
{
  if (!VTPTransceiver.CheckConnection())
    XBMC->Log(LOG_DEBUG, "CloseRecordedStream(): Control connection gone !");

  VTPTransceiver.CloseDataConnection(siReplay);

  DELETENULL(m_recordingPoller);
}

int ReadRecordedStream(unsigned char* buf, int buf_size)
{
  if (VTPTransceiver.DataSocket(siReplay) == INVALID_SOCKET)
    return 0;

  int res = 0;
  if (m_recordingFirstRead || m_recordingPoller->Poll(100))
  {
    m_recordingFirstRead = false;
    res = safe_read(VTPTransceiver.DataSocket(siReplay), buf, buf_size);
    if (res < 0 && FATALERRNO)
    {
      if (errno == EOVERFLOW)
        XBMC->Log(LOG_ERROR, "driver buffer overflow");
      else
        XBMC->Log(LOG_ERROR, "ERROR (%s,%d): %m", __FILE__, __LINE__);
      return 0;
    }
  }

  m_currentPlayingRecordPosition += res;
  m_recordingTotalBytesReaded    += res;
  return res;
}

long long SeekRecordedStream(long long pos, int whence)
{
  if (!VTPTransceiver.CheckConnection())
    return -1;

  long long nextPos = m_currentPlayingRecordPosition;

  switch (whence)
  {
    case SEEK_SET:
      nextPos = pos;
      break;

    case SEEK_CUR:
      nextPos += pos;
      break;

    case SEEK_END:
      if (m_currentPlayingRecordBytes)
        nextPos = m_currentPlayingRecordBytes - pos;
      else
        return -1;
      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos > (long long) m_currentPlayingRecordBytes)
    return 0;

  m_currentPlayingRecordPosition = nextPos;

  /* The VDR streamdev seek command returns the amount which is sendet into the
     network. */
  uint64_t bytesToFlush = VTPTransceiver.SeekRecordingPosition(nextPos);
  /* I don't like the following code, but have no better idea todo this.
     It flush the already by VDR transmitted data from the TCP stack, so if XBMC
     perform the next Data read the right stream position is on top of the Socket. */
  while (m_recordingTotalBytesReaded < bytesToFlush)
  {
    unsigned char buffer[32768];
    ReadRecordedStream(&*buffer, sizeof(buffer));
  }
  return nextPos;
}

long long PositionRecordedStream(void)
{
  return m_currentPlayingRecordPosition;
}

long long LengthRecordedStream(void)
{
  VTPTransceiver.GetPlayingRecordingSize(&m_currentPlayingRecordBytes, &m_currentPlayingRecordFrames);
  return m_currentPlayingRecordBytes;
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
