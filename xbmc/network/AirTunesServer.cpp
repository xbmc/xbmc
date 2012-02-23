/*
 * Many concepts and protocol specification in this code are taken
 * from Shairport, by James Laird.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "AirTunesServer.h"

#ifdef HAS_AIRPLAY
#include "network/AirPlayServer.h"
#endif

#ifdef HAS_AIRTUNES

#include "utils/log.h"
#include "utils/StdString.h"
#include "network/Zeroconf.h"
#include "ApplicationMessenger.h"
#include "filesystem/FilePipe.h"
#include "Application.h"
#include "cores/paplayer/BXAcodec.h"
#include "music/tags/MusicInfoTag.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "utils/Variant.h"
#include "settings/AdvancedSettings.h"

using namespace XFILE;

DllLibShairport *CAirTunesServer::m_pLibShairport = NULL;
CAirTunesServer *CAirTunesServer::ServerInstance = NULL;
CStdString CAirTunesServer::m_macAddress;

struct ao_device_xbmc
{
  XFILE::CFilePipe *pipe;
};

//audio output interface
void CAirTunesServer::AudioOutputFunctions::ao_initialize(void)
{
}

int CAirTunesServer::AudioOutputFunctions::ao_play(ao_device *device, char *output_samples, uint32_t num_bytes)
{
  if (!device)
    return 0;

  /*if (num_bytes && g_application.m_pPlayer)
    g_application.m_pPlayer->SetCaching(CACHESTATE_NONE);*///TODO

  ao_device_xbmc* device_xbmc = (ao_device_xbmc*) device;

#define NUM_OF_BYTES 64

  unsigned int sentBytes = 0;
  unsigned char buf[NUM_OF_BYTES];
  while (sentBytes < num_bytes)
  {
    int n = (num_bytes - sentBytes < NUM_OF_BYTES ? num_bytes - sentBytes : NUM_OF_BYTES);
    memcpy(buf, (char*) output_samples + sentBytes, n);

    if (device_xbmc->pipe->Write(buf, n) == 0)
      return 0;

    sentBytes += n;
  }

  return 1;
}

int CAirTunesServer::AudioOutputFunctions::ao_default_driver_id(void)
{
  return 0;
}

ao_device* CAirTunesServer::AudioOutputFunctions::ao_open_live(int driver_id, ao_sample_format *format,
    ao_option *option)
{
  ao_device_xbmc* device = new ao_device_xbmc();

  device->pipe = new XFILE::CFilePipe;
  device->pipe->OpenForWrite(XFILE::PipesManager::GetInstance().GetUniquePipeName());
  device->pipe->SetOpenThreashold(300);

  BXA_FmtHeader header;
  strncpy(header.fourcc, "BXA ", 4);
  header.type = BXA_PACKET_TYPE_FMT;
  header.bitsPerSample = format->bits;
  header.channels = format->channels;
  header.sampleRate = format->rate;
  header.durationMs = 0;

  if (device->pipe->Write(&header, sizeof(header)) == 0)
    return 0;

  ThreadMessage tMsg = { TMSG_MEDIA_STOP };
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  CFileItem item;
  item.SetPath(device->pipe->GetName());
  item.SetMimeType("audio/x-xbmc-pcm");

  if (ao_get_option(option, "artist"))
    item.GetMusicInfoTag()->SetArtist(ao_get_option(option, "artist"));

  if (ao_get_option(option, "album"))
    item.GetMusicInfoTag()->SetAlbum(ao_get_option(option, "album"));

  if (ao_get_option(option, "name"))
    item.GetMusicInfoTag()->SetTitle(ao_get_option(option, "name"));

  g_application.getApplicationMessenger().PlayFile(item);

  ThreadMessage tMsg2 = { TMSG_GUI_ACTIVATE_WINDOW, WINDOW_VISUALISATION, 0 };
  g_application.getApplicationMessenger().SendMessage(tMsg2, true);

  return (ao_device*) device;
}

int CAirTunesServer::AudioOutputFunctions::ao_close(ao_device *device)
{
  ao_device_xbmc* device_xbmc = (ao_device_xbmc*) device;
  device_xbmc->pipe->SetEof();
  device_xbmc->pipe->Close();
  delete device_xbmc->pipe;

  //fix airplay video for ios5 devices
  //on ios5 when airplaying video
  //the client first opens an airtunes stream
  //while the movie is loading
  //in that case we don't want to stop the player here
  //because this would stop the airplaying video
#ifdef HAS_AIRPLAY
  if (!CAirPlayServer::IsPlaying())
#endif
  {
    ThreadMessage tMsg = { TMSG_MEDIA_STOP };
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
    CLog::Log(LOGDEBUG, "AIRTUNES: AirPlay not running - stopping player");
  }

  delete device_xbmc;

  return 0;
}

/* -- Device Setup/Playback/Teardown -- */
int CAirTunesServer::AudioOutputFunctions::ao_append_option(ao_option **options, const char *key, const char *value)
{
  ao_option *op, *list;

  op = (ao_option*) calloc(1,sizeof(ao_option));
  if (op == NULL) return 0;

  op->key = strdup(key);
  op->value = strdup(value?value:"");
  op->next = NULL;

  if ((list = *options) != NULL)
  {
    list = *options;
    while (list->next != NULL)
      list = list->next;
    list->next = op;
  }
  else
  {
    *options = op;
  }

  return 1;
}

void CAirTunesServer::AudioOutputFunctions::ao_free_options(ao_option *options)
{
  ao_option *rest;

  while (options != NULL)
  {
    rest = options->next;
    free(options->key);
    free(options->value);
    free(options);
    options = rest;
  }
}

char* CAirTunesServer::AudioOutputFunctions::ao_get_option(ao_option *options, const char* key)
{

  while (options != NULL)
  {
    if (strcmp(options->key, key) == 0)
      return options->value;
    options = options->next;
  }

  return NULL;
}

bool CAirTunesServer::StartServer(int port, bool nonlocal, bool usePassword, const CStdString &password/*=""*/)
{
  bool success = false;
  CStdString pw = password;
  CNetworkInterface *net = g_application.getNetwork().GetFirstConnectedInterface();
  StopServer(true);

  if (net)
  {
    m_macAddress = net->GetMacAddress();
    m_macAddress.Replace(":","");
    while (m_macAddress.size() < 12)
    {
      m_macAddress = CStdString("0") + m_macAddress;
    }
  }
  else
  {
    m_macAddress = "000102030405";
  }

  if (!usePassword)
  {
    pw.Empty();
  }

  ServerInstance = new CAirTunesServer(port, nonlocal);
  if (ServerInstance->Initialize(password))
  {
    ServerInstance->Create();
    success = true;
  }

  if (success)
  {
    CStdString appName;
    appName.Format("%s@%s", m_macAddress.c_str(), g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME).c_str());

    std::map<std::string, std::string> txt;
    txt["cn"] = "0,1";
    txt["ch"] = "2";
    txt["ek"] = "1";
    txt["et"] = "0,1";
    txt["sv"] = "false";
    txt["tp"] = "UDP";
    txt["sm"] = "false";
    txt["ss"] = "16";
    txt["sr"] = "44100";
    txt["pw"] = "false";
    txt["vn"] = "3";
    txt["txtvers"] = "1";

    CZeroconf::GetInstance()->PublishService("servers.airtunes", "_raop._tcp", appName, port, txt);
  }

  return success;
}

void CAirTunesServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    if (m_pLibShairport->IsLoaded())
    {
      m_pLibShairport->shairport_exit();
    }
    ServerInstance->StopThread(bWait);
    ServerInstance->Deinitialize();
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }

    CZeroconf::GetInstance()->RemoveService("servers.airtunes");
  }
}

CAirTunesServer::CAirTunesServer(int port, bool nonlocal)
{
  m_port = port;
  m_pLibShairport = new DllLibShairport();
}

CAirTunesServer::~CAirTunesServer()
{
  if (m_pLibShairport->IsLoaded())
  {
    m_pLibShairport->Unload();
  }
  delete m_pLibShairport;
}

void CAirTunesServer::Process()
{
  m_bStop = false;

  while (!m_bStop && m_pLibShairport->shairport_is_running())
  {
    m_pLibShairport->shairport_loop();
  }
}

int shairport_log(const char* msg, size_t msgSize)
{
  if( g_advancedSettings.m_logEnableAirtunes)
  {
    CLog::Log(LOGDEBUG, "AIRTUNES: %s", msg);
  }
  return 1;
}

bool CAirTunesServer::Initialize(const CStdString &password)
{
  bool ret = false;
  int numArgs = 3;
  CStdString hwStr;
  CStdString pwStr;
  CStdString portStr;

  Deinitialize();

  hwStr.Format("--mac=%s", m_macAddress.c_str());
  pwStr.Format("--password=%s",password.c_str());
  portStr.Format("--server_port=%d",m_port);

  if (!password.empty())
  {
    numArgs++;
  }

  char *argv[] = { "--apname=XBMC", (char*) portStr.c_str(), (char*) hwStr.c_str(), (char *)pwStr.c_str(), NULL };

  if (m_pLibShairport->Load())
  {

    struct AudioOutput ao;
    ao.ao_initialize = AudioOutputFunctions::ao_initialize;
    ao.ao_play = AudioOutputFunctions::ao_play;
    ao.ao_default_driver_id = AudioOutputFunctions::ao_default_driver_id;
    ao.ao_open_live = AudioOutputFunctions::ao_open_live;
    ao.ao_close = AudioOutputFunctions::ao_close;
    ao.ao_append_option = AudioOutputFunctions::ao_append_option;
    ao.ao_free_options = AudioOutputFunctions::ao_free_options;
    ao.ao_get_option = AudioOutputFunctions::ao_get_option;
    struct printfPtr funcPtr;
    funcPtr.extprintf = shairport_log;

    m_pLibShairport->EnableDelayedUnload(false);
    m_pLibShairport->shairport_set_ao(&ao);
    m_pLibShairport->shairport_set_printf(&funcPtr);
    m_pLibShairport->shairport_main(numArgs, argv);
    ret = true;
  }
  return ret;
}

void CAirTunesServer::Deinitialize()
{
  if (m_pLibShairport && m_pLibShairport->IsLoaded())
  {
    m_pLibShairport->shairport_exit();
    m_pLibShairport->Unload();
  }
}

#endif
