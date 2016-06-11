/*
 * Many concepts and protocol specification in this code are taken
 * from Shairport, by James Laird.
 *
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef HAS_AIRTUNES
#include "AirTunesServer.h"

#include <map>
#include <string>
#include <utility>

#include "Application.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxBXA.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/PipeFile.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "music/tags/MusicInfoTag.h"
#include "network/dacp/dacp.h"
#include "network/Network.h"
#include "network/Zeroconf.h"
#include "network/ZeroconfBrowser.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "URL.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"

#if !defined(TARGET_WINDOWS)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#ifdef HAS_AIRPLAY
#include "network/AirPlayServer.h"
#endif

#define TMP_COVERART_PATH_JPG "special://temp/airtunes_album_thumb.jpg"
#define TMP_COVERART_PATH_PNG "special://temp/airtunes_album_thumb.png"
#define ZEROCONF_DACP_SERVICE "_dacp._tcp"

using namespace XFILE;
using namespace ANNOUNCEMENT;
using namespace KODI::MESSAGING;

DllLibShairplay *CAirTunesServer::m_pLibShairplay = NULL;
CAirTunesServer *CAirTunesServer::ServerInstance = NULL;
std::string CAirTunesServer::m_macAddress;
std::string CAirTunesServer::m_metadata[3];
CCriticalSection CAirTunesServer::m_metadataLock;
bool CAirTunesServer::m_streamStarted = false;
CCriticalSection CAirTunesServer::m_dacpLock;
CDACP *CAirTunesServer::m_pDACP = NULL;
std::string CAirTunesServer::m_dacp_id;
std::string CAirTunesServer::m_active_remote_header;
CCriticalSection CAirTunesServer::m_actionQueueLock;
std::list<CAction> CAirTunesServer::m_actionQueue;
CEvent CAirTunesServer::m_processActions;
int CAirTunesServer::m_sampleRate = 44100;


//parse daap metadata - thx to project MythTV
std::map<std::string, std::string> decodeDMAP(const char *buffer, unsigned int size)
{
  std::map<std::string, std::string> result;
  unsigned int offset = 8;
  while (offset < size)
  {
    std::string tag;
    tag.append(buffer + offset, 4);
    offset += 4;
    uint32_t length = Endian_SwapBE32(*(uint32_t *)(buffer + offset));
    offset += sizeof(uint32_t);
    std::string content;
    content.append(buffer + offset, length);//possible fixme - utf8?
    offset += length;
    result[tag] = content;
  }
  return result;
}

void CAirTunesServer::ResetMetadata()
{
  CSingleLock lock(m_metadataLock);

  XFILE::CFile::Delete(TMP_COVERART_PATH_JPG);
  XFILE::CFile::Delete(TMP_COVERART_PATH_PNG);
  RefreshCoverArt();

  m_metadata[0] = "";
  m_metadata[1] = "AirPlay";
  m_metadata[2] = "";
  RefreshMetadata();
}

void CAirTunesServer::RefreshMetadata()
{
  CSingleLock lock(m_metadataLock);
  MUSIC_INFO::CMusicInfoTag tag;
  if (g_infoManager.GetCurrentSongTag())
    tag = *g_infoManager.GetCurrentSongTag();
  if (m_metadata[0].length())
    tag.SetAlbum(m_metadata[0]);//album
  if (m_metadata[1].length())
    tag.SetTitle(m_metadata[1]);//title
  if (m_metadata[2].length())
    tag.SetArtist(m_metadata[2]);//artist
  
  CApplicationMessenger::GetInstance().PostMsg(TMSG_UPDATE_CURRENT_ITEM, 1, -1, static_cast<void*>(new CFileItem(tag)));
}

void CAirTunesServer::RefreshCoverArt(const char *outputFilename/* = NULL*/)
{
  static std::string coverArtFile = TMP_COVERART_PATH_JPG;

  if (outputFilename != NULL)
    coverArtFile = std::string(outputFilename);

  CSingleLock lock(m_metadataLock);
  //reset to empty before setting the new one
  //else it won't get refreshed because the name didn't change
  g_infoManager.SetCurrentAlbumThumb("");
  //update the ui
  g_infoManager.SetCurrentAlbumThumb(coverArtFile);
  //update the ui
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendThreadMessage(msg);
}

void CAirTunesServer::SetMetadataFromBuffer(const char *buffer, unsigned int size)
{

  std::map<std::string, std::string> metadata = decodeDMAP(buffer, size);
  CSingleLock lock(m_metadataLock);

  if(metadata["asal"].length())
    m_metadata[0] = metadata["asal"];//album
  if(metadata["minm"].length())    
    m_metadata[1] = metadata["minm"];//title
  if(metadata["asar"].length())    
    m_metadata[2] = metadata["asar"];//artist
  
  RefreshMetadata();
}

void CAirTunesServer::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if ( (flag & Player) && strcmp(sender, "xbmc") == 0)
  {
    if (strcmp(message, "OnPlay") == 0 && m_streamStarted)
    {
      RefreshMetadata();
      RefreshCoverArt();
      CSingleLock lock(m_dacpLock);
      if (m_pDACP)
        m_pDACP->Play();
    }
    
    if (strcmp(message, "OnStop") == 0 && m_streamStarted)
    {
      CSingleLock lock(m_dacpLock);
      if (m_pDACP)
        m_pDACP->Stop();
    }

    if (strcmp(message, "OnPause") == 0 && m_streamStarted)
    {
      CSingleLock lock(m_dacpLock);
      if (m_pDACP)
        m_pDACP->Pause();
    }
  }
}

void CAirTunesServer::EnableActionProcessing(bool enable)
{
  ServerInstance->RegisterActionListener(enable);
}

bool CAirTunesServer::OnAction(const CAction &action)
{
  switch(action.GetID())
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PREV_ITEM:
    case ACTION_VOLUME_UP:
    case ACTION_VOLUME_DOWN:
    case ACTION_MUTE:
    {
      CSingleLock lock(m_actionQueueLock);
      m_actionQueue.push_back(action);
      m_processActions.Set();
    }
  }
  return false;
}

void CAirTunesServer::Process()
{
  m_bStop = false;
  while(!m_bStop)
  {
    if (m_streamStarted)
      SetupRemoteControl();// check for remote controls

    m_processActions.WaitMSec(1000);// timeout for beeing able to stop
    std::list<CAction> currentActions;
    {
      CSingleLock lock(m_actionQueueLock);// copy and clear the source queue
      currentActions.insert(currentActions.begin(), m_actionQueue.begin(), m_actionQueue.end());
      m_actionQueue.clear();
    }
    
    for (auto currentAction : currentActions)
    {
      CSingleLock lock(m_dacpLock);
      if (m_pDACP)
      {
        switch(currentAction.GetID())
        {
          case ACTION_NEXT_ITEM:
            m_pDACP->NextItem();
            break;
          case ACTION_PREV_ITEM:
            m_pDACP->PrevItem();
            break;
          case ACTION_VOLUME_UP:
            m_pDACP->VolumeUp();
            break;
          case ACTION_VOLUME_DOWN:
            m_pDACP->VolumeDown();
            break;
          case ACTION_MUTE:
            m_pDACP->ToggleMute();
            break;
        }
      }
    }
  }
}

bool IsJPEG(const char *buffer, unsigned int size)
{
  bool ret = false;
  if (size < 2)
    return false;

  //JPEG image files begin with FF D8 and end with FF D9.
  // check for FF D8 big + little endian on start
  if ((buffer[0] == (char)0xd8 && buffer[1] == (char)0xff) ||
      (buffer[1] == (char)0xd8 && buffer[0] == (char)0xff))
    ret = true;

  if (ret)
  {
    ret = false;
    //check on FF D9 big + little endian on end
    if ((buffer[size - 2] == (char)0xd9 && buffer[size - 1] == (char)0xff) ||
       (buffer[size - 1] == (char)0xd9 && buffer[size - 2] == (char)0xff))
        ret = true;
  }

  return ret;
}

void CAirTunesServer::SetCoverArtFromBuffer(const char *buffer, unsigned int size)
{
  XFILE::CFile tmpFile;
  std::string tmpFilename = TMP_COVERART_PATH_PNG;

  if(!size)
    return;

  CSingleLock lock(m_metadataLock);
  
  if (IsJPEG(buffer, size))
    tmpFilename = TMP_COVERART_PATH_JPG;

  if (tmpFile.OpenForWrite(tmpFilename, true))
  {
    int writtenBytes=0;
    writtenBytes = tmpFile.Write(buffer, size);
    tmpFile.Close();

    if (writtenBytes > 0)
      RefreshCoverArt(tmpFilename.c_str());
  }
}

void CAirTunesServer::FreeDACPRemote()
{
  CSingleLock lock(m_dacpLock);
  if (m_pDACP)
    delete m_pDACP;
  m_pDACP = NULL;
}

#define RSA_KEY " \
-----BEGIN RSA PRIVATE KEY-----\
MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt\
wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U\
wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf\
/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/\
UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW\
BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa\
LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5\
NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm\
lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz\
aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu\
a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM\
oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z\
oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+\
k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL\
AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA\
cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf\
54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov\
17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc\
1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI\
LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ\
2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=\
-----END RSA PRIVATE KEY-----"

void CAirTunesServer::AudioOutputFunctions::audio_set_metadata(void *cls, void *session, const void *buffer, int buflen)
{
  CAirTunesServer::SetMetadataFromBuffer((char *)buffer, buflen);
}

void CAirTunesServer::AudioOutputFunctions::audio_set_coverart(void *cls, void *session, const void *buffer, int buflen)
{
  CAirTunesServer::SetCoverArtFromBuffer((char *)buffer, buflen);
}

char session[]="Kodi-AirTunes";

void* CAirTunesServer::AudioOutputFunctions::audio_init(void *cls, int bits, int channels, int samplerate)
{
  XFILE::CPipeFile *pipe=(XFILE::CPipeFile *)cls;
  const CURL pathToUrl(XFILE::PipesManager::GetInstance().GetUniquePipeName());
  pipe->OpenForWrite(pathToUrl);
  pipe->SetOpenThreashold(300);

  Demux_BXA_FmtHeader header;
  strncpy(header.fourcc, "BXA ", 4);
  header.type = BXA_PACKET_TYPE_FMT_DEMUX;
  header.bitsPerSample = bits;
  header.channels = channels;
  header.sampleRate = samplerate;
  header.durationMs = 0;

  if (pipe->Write(&header, sizeof(header)) == 0)
    return 0;

  CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);

  CFileItem *item = new CFileItem();
  item->SetPath(pipe->GetName());
  item->SetMimeType("audio/x-xbmc-pcm");
  m_streamStarted = true;
  m_sampleRate = samplerate;

  CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));

  // Not all airplay streams will provide metadata (e.g. if using mirroring,
  // no metadata will be sent).  If there *is* metadata, it will be received
  // in a later call to audio_set_metadata/audio_set_coverart.
  ResetMetadata();
  
  // browse for dacp services protocol which gives us the remote control service
  CZeroconfBrowser::GetInstance()->Start();
  CZeroconfBrowser::GetInstance()->AddServiceType(ZEROCONF_DACP_SERVICE);
  CAirTunesServer::EnableActionProcessing(true);

  return session;//session
}

void CAirTunesServer::AudioOutputFunctions::audio_remote_control_id(void *cls, const char *dacp_id, const char *active_remote_header)
{
  if (dacp_id && active_remote_header)
  {
    m_dacp_id = dacp_id;
    m_active_remote_header = active_remote_header;
  }
}

void CAirTunesServer::AudioOutputFunctions::audio_set_progress(void *cls, void *session, unsigned int start, unsigned int curr, unsigned int end)
{
  unsigned int duration = end - start;
  unsigned int position = curr - start;
  duration /= m_sampleRate;
  position /= m_sampleRate;

  g_application.m_pPlayer->SetTime(position * 1000);
  g_application.m_pPlayer->SetTotalTime(duration * 1000);
}

void CAirTunesServer::SetupRemoteControl()
{
  // check if we found the remote control service via zeroconf already or
  // if no valid id and headers was received yet
  if (m_dacp_id.empty() || m_active_remote_header.empty() || m_pDACP != NULL)
    return;

  // check for the service matching m_dacp_id
  std::vector<CZeroconfBrowser::ZeroconfService> services = CZeroconfBrowser::GetInstance()->GetFoundServices();
  for (auto service : services )
  {
    if (StringUtils::CompareNoCase(service.GetType(), std::string(ZEROCONF_DACP_SERVICE) + ".") == 0)
    {
#define DACP_NAME_PREFIX "iTunes_Ctrl_"
      // name has the form "iTunes_Ctrl_56B29BB6CB904862"
      // were we are interested in the 56B29BB6CB904862 identifier
      if (StringUtils::StartsWithNoCase(service.GetName(), DACP_NAME_PREFIX))
      {
        std::vector<std::string> tokens = StringUtils::Split(service.GetName(), DACP_NAME_PREFIX);
        // if we found the service matching the given identifier
        if (tokens.size() > 1 && tokens[1] == m_dacp_id)
        {
          // resolve the service and save it
          CZeroconfBrowser::GetInstance()->ResolveService(service);
          CSingleLock lock(m_dacpLock);
          // recheck with lock hold
          if (m_pDACP == NULL)
          {
            // we can control the client with this object now
            m_pDACP = new CDACP(m_active_remote_header, service.GetIP(), service.GetPort());
          }
          break;
        }
      }
    }
  }
}

void  CAirTunesServer::AudioOutputFunctions::audio_set_volume(void *cls, void *session, float volume)
{
  //volume from -30 - 0 - -144 means mute
  float volPercent = volume < -30.0f ? 0 : 1 - volume/-30;
#ifdef HAS_AIRPLAY
  CAirPlayServer::backupVolume();
#endif
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL))
    g_application.SetVolume(volPercent, false);//non-percent volume 0.0-1.0
}

void  CAirTunesServer::AudioOutputFunctions::audio_process(void *cls, void *session, const void *buffer, int buflen)
{
  XFILE::CPipeFile *pipe=(XFILE::CPipeFile *)cls;
  pipe->Write(buffer, buflen);
}

void  CAirTunesServer::AudioOutputFunctions::audio_destroy(void *cls, void *session)
{
  XFILE::CPipeFile *pipe=(XFILE::CPipeFile *)cls;
  pipe->SetEof();
  pipe->Close();
  
  CAirTunesServer::FreeDACPRemote();
  m_dacp_id.clear();
  m_active_remote_header.clear();

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
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
    CLog::Log(LOGDEBUG, "AIRTUNES: AirPlay not running - stopping player");
  }
  
  m_streamStarted = false;

  // no need to browse for dacp services while we don't receive
  // any airtunes streams...
  CZeroconfBrowser::GetInstance()->RemoveServiceType(ZEROCONF_DACP_SERVICE);
  CZeroconfBrowser::GetInstance()->Stop();
  CAirTunesServer::EnableActionProcessing(false);
}

void shairplay_log(void *cls, int level, const char *msg)
{
  int xbmcLevel = LOGINFO;
  if(!g_advancedSettings.CanLogComponent(LOGAIRTUNES))
    return;

  switch(level)
  {
    case RAOP_LOG_EMERG:    // system is unusable 
      xbmcLevel = LOGFATAL;
      break;
    case RAOP_LOG_ALERT:    // action must be taken immediately
    case RAOP_LOG_CRIT:     // critical conditions
      xbmcLevel = LOGSEVERE;
      break;
    case RAOP_LOG_ERR:      // error conditions
      xbmcLevel = LOGERROR;
      break;
    case RAOP_LOG_WARNING:  // warning conditions
      xbmcLevel = LOGWARNING;
      break;
    case RAOP_LOG_NOTICE:   // normal but significant condition
      xbmcLevel = LOGNOTICE;
      break;
    case RAOP_LOG_INFO:     // informational
      xbmcLevel = LOGINFO;
      break;
    case RAOP_LOG_DEBUG:    // debug-level messages
      xbmcLevel = LOGDEBUG;
      break;
    default:
      break;
  }
    CLog::Log(xbmcLevel, "AIRTUNES: %s", msg);
}

bool CAirTunesServer::StartServer(int port, bool nonlocal, bool usePassword, const std::string &password/*=""*/)
{
  bool success = false;
  std::string pw = password;
  CNetworkInterface *net = g_application.getNetwork().GetFirstConnectedInterface();
  StopServer(true);

  if (net)
  {
    m_macAddress = net->GetMacAddress();
    StringUtils::Replace(m_macAddress, ":","");
    while (m_macAddress.size() < 12)
    {
      m_macAddress = '0' + m_macAddress;
    }
  }
  else
  {
    m_macAddress = "000102030405";
  }

  if (!usePassword)
  {
    pw.clear();
  }

  ServerInstance = new CAirTunesServer(port, nonlocal);
  if (ServerInstance->Initialize(pw))
  {
    success = true;
    std::string appName = StringUtils::Format("%s@%s",
                                             m_macAddress.c_str(),
                                             CSysInfo::GetDeviceName().c_str());

    std::vector<std::pair<std::string, std::string> > txt;
    txt.push_back(std::make_pair("txtvers",  "1"));
    txt.push_back(std::make_pair("cn", "0,1"));
    txt.push_back(std::make_pair("ch", "2"));
    txt.push_back(std::make_pair("ek", "1"));
    txt.push_back(std::make_pair("et", "0,1"));
    txt.push_back(std::make_pair("sv", "false"));
    txt.push_back(std::make_pair("tp",  "UDP"));
    txt.push_back(std::make_pair("sm",  "false"));
    txt.push_back(std::make_pair("ss",  "16"));
    txt.push_back(std::make_pair("sr",  "44100"));
    txt.push_back(std::make_pair("pw",  usePassword?"true":"false"));
    txt.push_back(std::make_pair("vn",  "3"));
    txt.push_back(std::make_pair("da",  "true"));
    txt.push_back(std::make_pair("md",  "0,1,2"));
    txt.push_back(std::make_pair("am",  "Kodi,1"));
    txt.push_back(std::make_pair("vs",  "130.14"));

    CZeroconf::GetInstance()->PublishService("servers.airtunes", "_raop._tcp", appName, port, txt);
  }

  return success;
}

void CAirTunesServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->Deinitialize();
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }

    CZeroconf::GetInstance()->RemoveService("servers.airtunes");
  }
}

 bool CAirTunesServer::IsRunning()
 {
   if (ServerInstance == NULL)
     return false;

   return ((CThread*)ServerInstance)->IsRunning();
 }

CAirTunesServer::CAirTunesServer(int port, bool nonlocal)
: CThread("AirTunesActionThread"),
  m_pRaop(nullptr)
{
  m_port = port;
  m_pLibShairplay = new DllLibShairplay();
  m_pPipe         = new XFILE::CPipeFile;
}

CAirTunesServer::~CAirTunesServer()
{
  if (m_pLibShairplay->IsLoaded())
  {
    m_pLibShairplay->Unload();
  }
  delete m_pLibShairplay;
  delete m_pPipe;
}

void CAirTunesServer::RegisterActionListener(bool doRegister)
{
  if (doRegister)
  {
    CAnnouncementManager::GetInstance().AddAnnouncer(this);
    g_application.RegisterActionListener(this);
    ServerInstance->Create();
  }
  else
  {
    CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
    g_application.UnregisterActionListener(this);
    ServerInstance->StopThread(true);
  }
}

bool CAirTunesServer::Initialize(const std::string &password)
{
  bool ret = false;

  Deinitialize();

  if (m_pLibShairplay->Load())
  {

    raop_callbacks_t ao = {};
    ao.cls                  = m_pPipe;
    ao.audio_init           = AudioOutputFunctions::audio_init;
    ao.audio_set_volume     = AudioOutputFunctions::audio_set_volume;
    ao.audio_set_metadata   = AudioOutputFunctions::audio_set_metadata;
    ao.audio_set_coverart   = AudioOutputFunctions::audio_set_coverart;
    ao.audio_process        = AudioOutputFunctions::audio_process;
    ao.audio_destroy        = AudioOutputFunctions::audio_destroy;
    ao.audio_remote_control_id = AudioOutputFunctions::audio_remote_control_id;
    ao.audio_set_progress   = AudioOutputFunctions::audio_set_progress;
    m_pLibShairplay->EnableDelayedUnload(false);
    m_pRaop = m_pLibShairplay->raop_init(1, &ao, RSA_KEY);//1 - we handle one client at a time max
    ret = m_pRaop != NULL;    

    if(ret)
    {
      char macAdr[6];    
      unsigned short port = (unsigned short)m_port;
      
      m_pLibShairplay->raop_set_log_level(m_pRaop, RAOP_LOG_WARNING);
      if(g_advancedSettings.CanLogComponent(LOGAIRTUNES))
      {
        m_pLibShairplay->raop_set_log_level(m_pRaop, RAOP_LOG_DEBUG);
      }

      m_pLibShairplay->raop_set_log_callback(m_pRaop, shairplay_log, NULL);

      CNetworkInterface *net = g_application.getNetwork().GetFirstConnectedInterface();

      if (net)
      {
        net->GetMacAddressRaw(macAdr);
      }

      ret = m_pLibShairplay->raop_start(m_pRaop, &port, macAdr, 6, password.c_str()) >= 0;
    }
  }
  return ret;
}

void CAirTunesServer::Deinitialize()
{
  RegisterActionListener(false);

  if (m_pLibShairplay && m_pLibShairplay->IsLoaded())
  {
    m_pLibShairplay->raop_stop(m_pRaop);
    m_pLibShairplay->raop_destroy(m_pRaop);
    m_pLibShairplay->Unload();
  }
}

#endif

