/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameClient.h"
#include "GameClientCallbacks.h"
#include "GameClientInGameSaves.h"
#include "GameClientProperties.h"
#include "GameClientTranslator.h"
#include "addons/AddonManager.h"
#include "addons/BinaryAddonCache.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/playback/GameClientRealtimePlayback.h"
#include "games/addons/playback/GameClientReversiblePlayback.h"
#include "games/addons/streams/GameClientStreams.h"
#include "games/addons/streams/IGameClientStream.h"
#include "games/GameServices.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/Action.h"
#include "input/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>

using namespace KODI;
using namespace GAME;
using namespace KODI::MESSAGING;

#define EXTENSION_SEPARATOR          "|"
#define EXTENSION_WILDCARD           "*"

#define GAME_PROPERTY_EXTENSIONS           "extensions"
#define GAME_PROPERTY_SUPPORTS_VFS         "supports_vfs"
#define GAME_PROPERTY_SUPPORTS_STANDALONE  "supports_standalone"

// --- NormalizeExtension ------------------------------------------------------

namespace
{
  /*
   * \brief Convert to lower case and canonicalize with a leading "."
   */
  std::string NormalizeExtension(const std::string& strExtension)
  {
    std::string ext = strExtension;

    if (!ext.empty() && ext != EXTENSION_WILDCARD)
    {
      StringUtils::ToLower(ext);

      if (ext[0] != '.')
        ext.insert(0, ".");
    }

    return ext;
  }
}

// --- CGameClient -------------------------------------------------------------

std::unique_ptr<CGameClient> CGameClient::FromExtension(ADDON::CAddonInfo addonInfo, const cp_extension_t* ext)
{
  using namespace ADDON;

  static const std::vector<std::string> properties = {
      GAME_PROPERTY_EXTENSIONS,
      GAME_PROPERTY_SUPPORTS_VFS,
      GAME_PROPERTY_SUPPORTS_STANDALONE,
  };

  for (const auto& property : properties)
  {
    std::string strProperty = CServiceBroker::GetAddonMgr().GetExtValue(ext->configuration, property.c_str());
    if (!strProperty.empty())
      addonInfo.AddExtraInfo(property, strProperty);
  }

  return std::unique_ptr<CGameClient>(new CGameClient(std::move(addonInfo)));
}

CGameClient::CGameClient(ADDON::CAddonInfo addonInfo) :
  CAddonDll(std::move(addonInfo)),
  m_subsystems(CGameClientSubsystem::CreateSubsystems(*this, m_struct, m_critSection)),
  m_bSupportsVFS(false),
  m_bSupportsStandalone(false),
  m_bSupportsAllExtensions(false),
  m_bIsPlaying(false),
  m_serializeSize(0),
  m_region(GAME_REGION_UNKNOWN)
{
  const ADDON::InfoMap& extraInfo = m_addonInfo.ExtraInfo();
  ADDON::InfoMap::const_iterator it;

  it = extraInfo.find(GAME_PROPERTY_EXTENSIONS);
  if (it != extraInfo.end())
  {
    std::vector<std::string> extensions = StringUtils::Split(it->second, EXTENSION_SEPARATOR);
    std::transform(extensions.begin(), extensions.end(),
      std::inserter(m_extensions, m_extensions.begin()), NormalizeExtension);

    // Check for wildcard extension
    if (m_extensions.find(EXTENSION_WILDCARD) != m_extensions.end())
    {
      m_bSupportsAllExtensions = true;
      m_extensions.clear();
    }
  }

  it = extraInfo.find(GAME_PROPERTY_SUPPORTS_VFS);
  if (it != extraInfo.end())
    m_bSupportsVFS = (it->second == "true");

  it = extraInfo.find(GAME_PROPERTY_SUPPORTS_STANDALONE);
  if (it != extraInfo.end())
    m_bSupportsStandalone = (it->second == "true");

  ResetPlayback();
}

CGameClient::~CGameClient(void)
{
  CloseFile();
  CGameClientSubsystem::DestroySubsystems(m_subsystems);
}

std::string CGameClient::LibPath() const
{
  // If the game client requires a proxy, load its DLL instead
  if (m_struct.props.proxy_dll_count > 0)
    return GetDllPath(m_struct.props.proxy_dll_paths[0]);

  return CAddonDll::LibPath();
}

ADDON::AddonPtr CGameClient::GetRunningInstance() const
{
  using namespace ADDON;

  CBinaryAddonCache& addonCache = CServiceBroker::GetBinaryAddonCache();
  return addonCache.GetAddonInstance(ID(), Type());
}

bool CGameClient::SupportsPath() const
{
  return !m_extensions.empty() || m_bSupportsAllExtensions;
}

bool CGameClient::IsExtensionValid(const std::string& strExtension) const
{
  if (strExtension.empty())
    return false;

  if (SupportsAllExtensions())
    return true;

  return m_extensions.find(NormalizeExtension(strExtension)) != m_extensions.end();
}

bool CGameClient::Initialize(void)
{
  using namespace XFILE;

  // Ensure user profile directory exists for add-on
  if (!CDirectory::Exists(Profile()))
    CDirectory::Create(Profile());

  // Ensure directory exists for savestates
  const CGameServices &gameServices = CServiceBroker::GetGameServices();
  std::string savestatesDir = URIUtils::AddFileToFolder(gameServices.GetSavestatesFolder(), ID());
  if (!CDirectory::Exists(savestatesDir))
    CDirectory::Create(savestatesDir);

  AddonProperties().InitializeProperties();

  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.CloseGame = cb_close_game;
  m_struct.toKodi.OpenStream = cb_open_stream;
  m_struct.toKodi.GetStreamBuffer = cb_get_stream_buffer;
  m_struct.toKodi.AddStreamData = cb_add_stream_data;
  m_struct.toKodi.ReleaseStreamBuffer = cb_release_stream_buffer;
  m_struct.toKodi.CloseStream = cb_close_stream;
  m_struct.toKodi.HwGetProcAddress = cb_hw_get_proc_address;
  m_struct.toKodi.InputEvent = cb_input_event;

  if (Create(ADDON_INSTANCE_GAME, &m_struct, &m_struct.props) == ADDON_STATUS_OK)
  {
    Input().Initialize();
    LogAddonProperties();
    return true;
  }

  return false;
}

void CGameClient::Unload()
{
  Input().Deinitialize();

  Destroy();
}

bool CGameClient::OpenFile(const CFileItem& file, RETRO::IStreamManager& streamManager, IGameInputCallback *input)
{
  // Check if we should open in standalone mode
  if (file.GetPath().empty())
    return false;

  // Some cores "succeed" to load the file even if it doesn't exist
  if (!XFILE::CFile::Exists(file.GetPath()))
  {

    // Failed to play game
    // The required files can't be found.
    HELPERS::ShowOKDialogText(CVariant{ 35210 }, CVariant{ g_localizeStrings.Get(35219) });
    return false;
  }

  // Resolve special:// URLs
  CURL translatedUrl(CSpecialProtocol::TranslatePath(file.GetPath()));

  // Remove file:// from URLs if add-on doesn't support VFS
  if (!m_bSupportsVFS)
  {
    if (translatedUrl.GetProtocol() == "file")
      translatedUrl.SetProtocol("");
  }

  std::string path = translatedUrl.Get();
  CLog::Log(LOGDEBUG, "GameClient: Loading %s", CURL::GetRedacted(path).c_str());

  CSingleLock lock(m_critSection);

  if (!Initialized())
    return false;

  CloseFile();

  Streams().Initialize(streamManager);

  GAME_ERROR error = GAME_ERROR_FAILED;

  try { LogError(error = m_struct.toAddon.LoadGame(path.c_str()), "LoadGame()"); }
  catch (...) { LogException("LoadGame()"); }

  if (error != GAME_ERROR_NO_ERROR)
  {
    NotifyError(error);
    Streams().Deinitialize();
    return false;
  }

  if (!InitializeGameplay(file.GetPath(), streamManager, input))
  {
    Streams().Deinitialize();
    return false;
  }

  return true;
}

bool CGameClient::OpenStandalone(RETRO::IStreamManager& streamManager, IGameInputCallback *input)
{
  CLog::Log(LOGDEBUG, "GameClient: Loading %s in standalone mode", ID().c_str());

  CSingleLock lock(m_critSection);

  if (!Initialized())
    return false;

  CloseFile();

  Streams().Initialize(streamManager);

  GAME_ERROR error = GAME_ERROR_FAILED;

  try { LogError(error = m_struct.toAddon.LoadStandalone(), "LoadStandalone()"); }
  catch (...) { LogException("LoadStandalone()"); }

  if (error != GAME_ERROR_NO_ERROR)
  {
    NotifyError(error);
    Streams().Deinitialize();
    return false;
  }

  if (!InitializeGameplay("", streamManager, input))
  {
    Streams().Deinitialize();
    return false;
  }

  return true;
}

bool CGameClient::InitializeGameplay(const std::string& gamePath, RETRO::IStreamManager& streamManager, IGameInputCallback *input)
{
  if (LoadGameInfo())
  {
    Input().Start();

    m_bIsPlaying      = true;
    m_gamePath        = gamePath;
    m_serializeSize   = GetSerializeSize();
    m_input           = input;

    m_inGameSaves.reset(new CGameClientInGameSaves(this, &m_struct.toAddon));
    m_inGameSaves->Load();

    // Start playback
    CreatePlayback();

    return true;
  }

  return false;
}

bool CGameClient::LoadGameInfo()
{
  // Get information about system timings
  // Can be called only after retro_load_game()
  game_system_timing timingInfo = { };

  bool bSuccess = false;
  try { bSuccess = LogError(m_struct.toAddon.GetGameTiming(&timingInfo), "GetGameTiming()"); }
  catch (...) { LogException("GetGameTiming()"); }

  if (!bSuccess)
  {
    CLog::Log(LOGERROR, "GameClient: Failed to get timing info");
    return false;
  }

  GAME_REGION region;
  try { region = m_struct.toAddon.GetRegion(); }
  catch (...) { LogException("GetRegion()"); return false; }

  CLog::Log(LOGINFO, "GAME: ---------------------------------------");
  CLog::Log(LOGINFO, "GAME: FPS:         %f", timingInfo.fps);
  CLog::Log(LOGINFO, "GAME: Sample Rate: %f", timingInfo.sample_rate);
  CLog::Log(LOGINFO, "GAME: Region:      %s", CGameClientTranslator::TranslateRegion(region));
  CLog::Log(LOGINFO, "GAME: ---------------------------------------");

  m_framerate = timingInfo.fps;
  m_samplerate = timingInfo.sample_rate;
  m_region = region;

  return true;
}

void CGameClient::NotifyError(GAME_ERROR error)
{
  std::string missingResource;

  if (error == GAME_ERROR_RESTRICTED)
    missingResource = GetMissingResource();

  if (!missingResource.empty())
  {
    // Failed to play game
    // This game requires the following add-on: %s
    HELPERS::ShowOKDialogText(CVariant{ 35210 }, CVariant{ StringUtils::Format(g_localizeStrings.Get(35211).c_str(), missingResource.c_str()) });
  }
  else
  {
    // Failed to play game
    // The emulator "%s" had an internal error.
    HELPERS::ShowOKDialogText(CVariant{ 35210 }, CVariant{ StringUtils::Format(g_localizeStrings.Get(35213).c_str(), Name().c_str()) });
  }
}

std::string CGameClient::GetMissingResource()
{
  using namespace ADDON;

  std::string strAddonId;

  const auto& dependencies = GetDependencies();
  for (auto it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    const std::string& strDependencyId = it->id;
    if (StringUtils::StartsWith(strDependencyId, "resource.games"))
    {
      AddonPtr addon;
      const bool bInstalled = CServiceBroker::GetAddonMgr().GetAddon(strDependencyId, addon);
      if (!bInstalled)
      {
        strAddonId = strDependencyId;
        break;
      }
    }
  }

  return strAddonId;
}

void CGameClient::CreatePlayback()
{
  bool bRequiresGameLoop = false;

  try { bRequiresGameLoop = m_struct.toAddon.RequiresGameLoop(); }
  catch (...) { LogException("RequiresGameLoop()"); }

  if (bRequiresGameLoop)
  {
    m_playback.reset(new CGameClientReversiblePlayback(this, m_framerate, m_serializeSize));
  }
  else
  {
    ResetPlayback();
  }
}

void CGameClient::ResetPlayback()
{
  m_playback.reset(new CGameClientRealtimePlayback);
}

void CGameClient::Reset()
{
  ResetPlayback();

  CSingleLock lock(m_critSection);

  if (m_bIsPlaying)
  {
    try { LogError(m_struct.toAddon.Reset(), "Reset()"); }
    catch (...) { LogException("Reset()"); }

    CreatePlayback();
  }
}

void CGameClient::CloseFile()
{
  ResetPlayback();

  CSingleLock lock(m_critSection);

  if (m_bIsPlaying)
  {
    m_inGameSaves->Save();
    m_inGameSaves.reset();

    m_bIsPlaying = false;
    m_gamePath.clear();
    m_serializeSize = 0;
    m_input = nullptr;

    Input().Stop();

    try { LogError(m_struct.toAddon.UnloadGame(), "UnloadGame()"); }
    catch (...) { LogException("UnloadGame()"); }

    Streams().Deinitialize();
  }
}

void CGameClient::RunFrame()
{
  IGameInputCallback *input;

  {
    CSingleLock lock(m_critSection);
    input = m_input;
  }

  if (input)
    input->PollInput();

  CSingleLock lock(m_critSection);

  if (m_bIsPlaying)
  {
    try { LogError(m_struct.toAddon.RunFrame(), "RunFrame()"); }
    catch (...) { LogException("RunFrame()"); }
  }
}

size_t CGameClient::GetSerializeSize()
{
  CSingleLock lock(m_critSection);

  size_t serializeSize = 0;
  if (m_bIsPlaying)
  {
    try { serializeSize = m_struct.toAddon.SerializeSize(); }
    catch (...) { LogException("SerializeSize()"); }
  }

  return serializeSize;
}

bool CGameClient::Serialize(uint8_t* data, size_t size)
{
  if (data == nullptr || size == 0)
    return false;

  CSingleLock lock(m_critSection);

  bool bSuccess = false;
  if (m_bIsPlaying)
  {
    try { bSuccess = LogError(m_struct.toAddon.Serialize(data, size), "Serialize()"); }
    catch (...) { LogException("Serialize()"); }
  }

  return bSuccess;
}

bool CGameClient::Deserialize(const uint8_t* data, size_t size)
{
  if (data == nullptr || size == 0)
    return false;

  CSingleLock lock(m_critSection);

  bool bSuccess = false;
  if (m_bIsPlaying)
  {
    try { bSuccess = LogError(m_struct.toAddon.Deserialize(data, size), "Deserialize()"); }
    catch (...) { LogException("Deserialize()"); }
  }

  return bSuccess;
}

void CGameClient::LogAddonProperties(void) const
{
  CLog::Log(LOGINFO, "GAME: ------------------------------------");
  CLog::Log(LOGINFO, "GAME: Loaded DLL for %s", ID().c_str());
  CLog::Log(LOGINFO, "GAME: Client: %s at version %s", Name().c_str(), Version().asString().c_str());
  CLog::Log(LOGINFO, "GAME: Valid extensions: %s", StringUtils::Join(m_extensions, " ").c_str());
  CLog::Log(LOGINFO, "GAME: Supports VFS:                  %s", m_bSupportsVFS ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: Supports standalone execution: %s", m_bSupportsStandalone ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: ------------------------------------");
}

bool CGameClient::LogError(GAME_ERROR error, const char* strMethod) const
{
  if (error != GAME_ERROR_NO_ERROR)
  {
    CLog::Log(LOGERROR, "GAME - %s - addon '%s' returned an error: %s",
        strMethod, ID().c_str(), CGameClientTranslator::ToString(error));
    return false;
  }
  return true;
}

void CGameClient::LogException(const char* strFunctionName) const
{
  CLog::Log(LOGERROR, "GAME: exception caught while trying to call '%s' on add-on %s",
      strFunctionName, ID().c_str());
  CLog::Log(LOGERROR, "Please contact the developer of this add-on: %s", Author().c_str());
}


void CGameClient::cb_close_game(void* kodiInstance)
{
  using namespace MESSAGING;

  CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
}

void* CGameClient::cb_open_stream(void* kodiInstance, const game_stream_properties *properties)
{
  if (properties == nullptr)
    return nullptr;

  CGameClient *gameClient = static_cast<CGameClient*>(kodiInstance);
  if (gameClient == nullptr)
    return nullptr;

  return gameClient->Streams().OpenStream(*properties);
}

bool CGameClient::cb_get_stream_buffer(void* kodiInstance, void *stream, unsigned int width, unsigned int height, game_stream_buffer *buffer)
{
  if (buffer == nullptr)
    return false;

  IGameClientStream *gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return false;

  return gameClientStream->GetBuffer(width, height, *buffer);
}

void CGameClient::cb_add_stream_data(void* kodiInstance, void *stream, const game_stream_packet *packet)
{
  if (packet == nullptr)
    return;

  IGameClientStream *gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return;

  gameClientStream->AddData(*packet);
}

void CGameClient::cb_release_stream_buffer(void* kodiInstance, void *stream, game_stream_buffer *buffer)
{
  if (buffer == nullptr)
    return;

  IGameClientStream *gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return;

  gameClientStream->ReleaseBuffer(*buffer);
}

void CGameClient::cb_close_stream(void* kodiInstance, void *stream)
{
  CGameClient *gameClient = static_cast<CGameClient*>(kodiInstance);
  if (gameClient == nullptr)
    return;

  IGameClientStream *gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return;

  gameClient->Streams().CloseStream(gameClientStream);
}

game_proc_address_t CGameClient::cb_hw_get_proc_address(void* kodiInstance, const char *sym)
{
  CGameClient *gameClient = static_cast<CGameClient*>(kodiInstance);
  if (!gameClient)
    return nullptr;

  //! @todo
  return nullptr;
}

bool CGameClient::cb_input_event(void* kodiInstance, const game_input_event* event)
{
  CGameClient *gameClient = static_cast<CGameClient*>(kodiInstance);
  if (!gameClient)
    return false;

  if (event == nullptr)
    return false;

  return gameClient->Input().ReceiveInputEvent(*event);
}
