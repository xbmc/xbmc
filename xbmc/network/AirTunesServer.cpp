/*
 * Many concepts and protocol specification in this code are taken
 * from Shairport, by James Laird.
 *
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AirTunesServer.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationActionListeners.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxBXA.h"
#include "filesystem/File.h"
#include "filesystem/PipeFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "music/tags/MusicInfoTag.h"
#include "network/Network.h"
#include "network/Zeroconf.h"
#include "network/ZeroconfBrowser.h"
#include "network/dacp/dacp.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/EndianSwap.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <utility>

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
using namespace std::chrono_literals;

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

unsigned int CAirTunesServer::m_cachedStartTime = 0;
unsigned int CAirTunesServer::m_cachedEndTime = 0;
unsigned int CAirTunesServer::m_cachedCurrentTime = 0;


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
    uint32_t length = Endian_SwapBE32(*(const uint32_t *)(buffer + offset));
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
  std::unique_lock<CCriticalSection> lock(m_metadataLock);

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
  std::unique_lock<CCriticalSection> lock(m_metadataLock);
  MUSIC_INFO::CMusicInfoTag tag;
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  if (infoMgr.GetCurrentSongTag())
    tag = *infoMgr.GetCurrentSongTag();
  if (m_metadata[0].length())
    tag.SetAlbum(m_metadata[0]);//album
  if (m_metadata[1].length())
    tag.SetTitle(m_metadata[1]);//title
  if (m_metadata[2].length())
    tag.SetArtist(m_metadata[2]);//artist

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_UPDATE_CURRENT_ITEM, 1, -1,
                                             static_cast<void*>(new CFileItem(tag)));
}

void CAirTunesServer::RefreshCoverArt(const char *outputFilename/* = NULL*/)
{
  static std::string coverArtFile = TMP_COVERART_PATH_JPG;

  if (outputFilename != NULL)
    coverArtFile = std::string(outputFilename);

  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  std::unique_lock<CCriticalSection> lock(m_metadataLock);
  //reset to empty before setting the new one
  //else it won't get refreshed because the name didn't change
  infoMgr.SetCurrentAlbumThumb("");
  //update the ui
  infoMgr.SetCurrentAlbumThumb(coverArtFile);
  //update the ui
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_REFRESH_THUMBS);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CAirTunesServer::SetMetadataFromBuffer(const char *buffer, unsigned int size)
{

  std::map<std::string, std::string> metadata = decodeDMAP(buffer, size);
  std::unique_lock<CCriticalSection> lock(m_metadataLock);

  if(metadata["asal"].length())
    m_metadata[0] = metadata["asal"];//album
  if(metadata["minm"].length())
    m_metadata[1] = metadata["minm"];//title
  if(metadata["asar"].length())
    m_metadata[2] = metadata["asar"];//artist

  RefreshMetadata();
}

void CAirTunesServer::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                               const std::string& sender,
                               const std::string& message,
                               const CVariant& data)
{
  if ((flag & ANNOUNCEMENT::Player) &&
      sender == ANNOUNCEMENT::CAnnouncementManager::ANNOUNCEMENT_SENDER)
  {
    if ((message == "OnPlay" || message == "OnResume") && m_streamStarted)
    {
      RefreshMetadata();
      RefreshCoverArt();
      std::unique_lock<CCriticalSection> lock(m_dacpLock);
      if (m_pDACP)
        m_pDACP->Play();
    }

    if (message == "OnStop" && m_streamStarted)
    {
      std::unique_lock<CCriticalSection> lock(m_dacpLock);
      if (m_pDACP)
        m_pDACP->Stop();
    }

    if (message == "OnPause" && m_streamStarted)
    {
      std::unique_lock<CCriticalSection> lock(m_dacpLock);
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
      std::unique_lock<CCriticalSection> lock(m_actionQueueLock);
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

    m_processActions.Wait(1000ms); // timeout for being able to stop
    std::list<CAction> currentActions;
    {
      std::unique_lock<CCriticalSection> lock(m_actionQueueLock); // copy and clear the source queue
      currentActions.insert(currentActions.begin(), m_actionQueue.begin(), m_actionQueue.end());
      m_actionQueue.clear();
    }

    for (const auto& currentAction : currentActions)
    {
      std::unique_lock<CCriticalSection> lock(m_dacpLock);
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

  std::unique_lock<CCriticalSection> lock(m_metadataLock);

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
  std::unique_lock<CCriticalSection> lock(m_dacpLock);
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
  CAirTunesServer::SetMetadataFromBuffer((const char *)buffer, buflen);
}

void CAirTunesServer::AudioOutputFunctions::audio_set_coverart(void *cls, void *session, const void *buffer, int buflen)
{
  CAirTunesServer::SetCoverArtFromBuffer((const char *)buffer, buflen);
}

char session[]="Kodi-AirTunes";

void* CAirTunesServer::AudioOutputFunctions::audio_init(void *cls, int bits, int channels, int samplerate)
{
  XFILE::CPipeFile *pipe=(XFILE::CPipeFile *)cls;
  const CURL pathToUrl(XFILE::PipesManager::GetInstance().GetUniquePipeName());
  pipe->OpenForWrite(pathToUrl);
  pipe->SetOpenThreshold(300);

  Demux_BXA_FmtHeader header;
  std::memcpy(header.fourcc, "BXA ", 4);
  header.type = BXA_PACKET_TYPE_FMT_DEMUX;
  header.bitsPerSample = bits;
  header.channels = channels;
  header.sampleRate = samplerate;
  header.durationMs = 0;

  if (pipe->Write(&header, sizeof(header)) == 0)
    return 0;

  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);

  CFileItem *item = new CFileItem();
  item->SetPath(pipe->GetName());
  item->SetMimeType("audio/x-xbmc-pcm");
  m_streamStarted = true;
  m_sampleRate = samplerate;

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));

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

void CAirTunesServer::InformPlayerAboutPlayTimes()
{
  if (m_cachedEndTime > 0)
  {
    unsigned int duration = m_cachedEndTime - m_cachedStartTime;
    unsigned int position = m_cachedCurrentTime - m_cachedStartTime;
    duration /= m_sampleRate;
    position /= m_sampleRate;

    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlaying())
    {
      appPlayer->SetTime(position * 1000);
      appPlayer->SetTotalTime(duration * 1000);

      // reset play times now that we have informed the player
      m_cachedEndTime = 0;
      m_cachedCurrentTime = 0;
      m_cachedStartTime = 0;

    }
  }
}

void CAirTunesServer::AudioOutputFunctions::audio_set_progress(void *cls, void *session, unsigned int start, unsigned int curr, unsigned int end)
{
  m_cachedStartTime = start;
  m_cachedCurrentTime = curr;
  m_cachedEndTime = end;
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
  {
    // player is there - directly inform him about play times
    InformPlayerAboutPlayTimes();
  }
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
    if (StringUtils::EqualsNoCase(service.GetType(), std::string(ZEROCONF_DACP_SERVICE) + "."))
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
          std::unique_lock<CCriticalSection> lock(m_dacpLock);
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
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL))
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetVolume(volPercent, false); //non-percent volume 0.0-1.0
  }
}

void  CAirTunesServer::AudioOutputFunctions::audio_process(void *cls, void *session, const void *buffer, int buflen)
{
  XFILE::CPipeFile *pipe=(XFILE::CPipeFile *)cls;
  pipe->Write(buffer, buflen);

  // in case there are some play times cached that are not yet sent to the player - do it here
  InformPlayerAboutPlayTimes();
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
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
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
  if (!CServiceBroker::GetLogging().CanLogComponent(LOGAIRTUNES))
    return;

  switch(level)
  {
    case RAOP_LOG_EMERG:    // system is unusable
    case RAOP_LOG_ALERT:    // action must be taken immediately
    case RAOP_LOG_CRIT:     // critical conditions
      xbmcLevel = LOGFATAL;
      break;
    case RAOP_LOG_ERR:      // error conditions
      xbmcLevel = LOGERROR;
      break;
    case RAOP_LOG_WARNING:  // warning conditions
      xbmcLevel = LOGWARNING;
      break;
    case RAOP_LOG_NOTICE:   // normal but significant condition
    case RAOP_LOG_INFO:     // informational
      xbmcLevel = LOGINFO;
      break;
    case RAOP_LOG_DEBUG:    // debug-level messages
      xbmcLevel = LOGDEBUG;
      break;
    default:
      break;
  }
  CLog::Log(xbmcLevel, "AIRTUNES: {}", msg);
}

bool CAirTunesServer::StartServer(int port, bool nonlocal, bool usePassword, const std::string &password/*=""*/)
{
  bool success = false;
  std::string pw = password;
  CNetworkInterface *net = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
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
    std::string appName = StringUtils::Format("{}@{}", m_macAddress, CSysInfo::GetDeviceName());

    std::vector<std::pair<std::string, std::string> > txt;
    txt.emplace_back("txtvers", "1");
    txt.emplace_back("cn", "0,1");
    txt.emplace_back("ch", "2");
    txt.emplace_back("ek", "1");
    txt.emplace_back("et", "0,1");
    txt.emplace_back("sv", "false");
    txt.emplace_back("tp", "UDP");
    txt.emplace_back("sm", "false");
    txt.emplace_back("ss", "16");
    txt.emplace_back("sr", "44100");
    txt.emplace_back("pw", usePassword ? "true" : "false");
    txt.emplace_back("vn", "3");
    txt.emplace_back("da", "true");
    txt.emplace_back("md", "0,1,2");
    txt.emplace_back("am", "Kodi,1");
    txt.emplace_back("vs", "130.14");

    CZeroconf::GetInstance()->PublishService("servers.airtunes", "_raop._tcp",
                                             CSysInfo::GetDeviceName() + " airtunes", port, txt);
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

  return ServerInstance->IsRAOPRunningInternal();
}

bool CAirTunesServer::IsRAOPRunningInternal()
{
  if (m_pRaop)
  {
    return raop_is_running(m_pRaop) != 0;
  }

  return false;
}

CAirTunesServer::CAirTunesServer(int port, bool nonlocal)
  : CThread("AirTunesActionThread")
{
  m_port = port;
  m_pPipe = new XFILE::CPipeFile;
}

CAirTunesServer::~CAirTunesServer()
{
  delete m_pPipe;
}

void CAirTunesServer::RegisterActionListener(bool doRegister)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appListener = components.GetComponent<CApplicationActionListeners>();

  if (doRegister)
  {
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
    appListener->RegisterActionListener(this);
    ServerInstance->Create();
  }
  else
  {
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    appListener->UnregisterActionListener(this);
    ServerInstance->StopThread(true);
  }
}

bool CAirTunesServer::Initialize(const std::string &password)
{
  bool ret = false;

  Deinitialize();

  raop_callbacks_t ao = {};
  ao.cls = m_pPipe;
  ao.audio_init = AudioOutputFunctions::audio_init;
  ao.audio_set_volume = AudioOutputFunctions::audio_set_volume;
  ao.audio_set_metadata = AudioOutputFunctions::audio_set_metadata;
  ao.audio_set_coverart = AudioOutputFunctions::audio_set_coverart;
  ao.audio_process = AudioOutputFunctions::audio_process;
  ao.audio_destroy = AudioOutputFunctions::audio_destroy;
  ao.audio_remote_control_id = AudioOutputFunctions::audio_remote_control_id;
  ao.audio_set_progress = AudioOutputFunctions::audio_set_progress;
  m_pRaop = raop_init(1, &ao, RSA_KEY, nullptr); //1 - we handle one client at a time max

  if (m_pRaop)
  {
    char macAdr[6];
    unsigned short port = (unsigned short)m_port;

    raop_set_log_level(m_pRaop, RAOP_LOG_WARNING);
    if (CServiceBroker::GetLogging().CanLogComponent(LOGAIRTUNES))
    {
      raop_set_log_level(m_pRaop, RAOP_LOG_DEBUG);
    }

    raop_set_log_callback(m_pRaop, shairplay_log, NULL);

    CNetworkInterface* net = CServiceBroker::GetNetwork().GetFirstConnectedInterface();

    if (net)
    {
      net->GetMacAddressRaw(macAdr);
    }

    ret = raop_start(m_pRaop, &port, macAdr, 6, password.c_str()) >= 0;
  }
  return ret;
}

void CAirTunesServer::Deinitialize()
{
  RegisterActionListener(false);

  if (m_pRaop)
  {
    raop_stop(m_pRaop);
    raop_destroy(m_pRaop);
    m_pRaop = nullptr;
  }
}
