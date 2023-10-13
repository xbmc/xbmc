/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClient.h"

#include "FileItem.h"
#include "GameClientCallbacks.h"
#include "GameClientInGameSaves.h"
#include "GameClientProperties.h"
#include "GameClientTranslator.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/BinaryAddonCache.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameServices.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/streams/GameClientStreams.h"
#include "games/addons/streams/IGameClientStream.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>

using namespace KODI;
using namespace GAME;

#define EXTENSION_SEPARATOR "|"
#define EXTENSION_WILDCARD "*"

#define GAME_PROPERTY_EXTENSIONS "extensions"
#define GAME_PROPERTY_SUPPORTS_VFS "supports_vfs"
#define GAME_PROPERTY_SUPPORTS_STANDALONE "supports_standalone"

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
} // namespace

// --- CGameClient -------------------------------------------------------------

CGameClient::CGameClient(const ADDON::AddonInfoPtr& addonInfo)
  : CAddonDll(addonInfo, ADDON::AddonType::GAMEDLL),
    m_subsystems(CGameClientSubsystem::CreateSubsystems(*this, *m_ifc.game, m_critSection)),
    m_bIsPlaying(false)
{
  using namespace ADDON;

  std::vector<std::string> extensions = StringUtils::Split(
      Type(AddonType::GAMEDLL)->GetValue(GAME_PROPERTY_EXTENSIONS).asString(), EXTENSION_SEPARATOR);
  std::transform(extensions.begin(), extensions.end(),
                 std::inserter(m_extensions, m_extensions.begin()), NormalizeExtension);

  // Check for wildcard extension
  if (m_extensions.find(EXTENSION_WILDCARD) != m_extensions.end())
  {
    m_bSupportsAllExtensions = true;
    m_extensions.clear();
  }

  m_bSupportsVFS =
      addonInfo->Type(AddonType::GAMEDLL)->GetValue(GAME_PROPERTY_SUPPORTS_VFS).asBoolean();
  m_bSupportsStandalone =
      addonInfo->Type(AddonType::GAMEDLL)->GetValue(GAME_PROPERTY_SUPPORTS_STANDALONE).asBoolean();

  std::tie(m_emulatorName, m_platforms) = ParseLibretroName(Name());
}

CGameClient::~CGameClient(void)
{
  CloseFile();
  CGameClientSubsystem::DestroySubsystems(m_subsystems);
}

std::string CGameClient::LibPath() const
{
  // If the game client requires a proxy, load its DLL instead
  if (m_ifc.game->props->proxy_dll_count > 0)
    return GetDllPath(m_ifc.game->props->proxy_dll_paths[0]);

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
  const CGameServices& gameServices = CServiceBroker::GetGameServices();
  std::string savestatesDir = URIUtils::AddFileToFolder(gameServices.GetSavestatesFolder(), ID());
  if (!CDirectory::Exists(savestatesDir))
    CDirectory::Create(savestatesDir);

  if (!AddonProperties().InitializeProperties())
    return false;

  m_ifc.game->toKodi->kodiInstance = this;
  m_ifc.game->toKodi->CloseGame = cb_close_game;
  m_ifc.game->toKodi->OpenStream = cb_open_stream;
  m_ifc.game->toKodi->GetStreamBuffer = cb_get_stream_buffer;
  m_ifc.game->toKodi->AddStreamData = cb_add_stream_data;
  m_ifc.game->toKodi->ReleaseStreamBuffer = cb_release_stream_buffer;
  m_ifc.game->toKodi->CloseStream = cb_close_stream;
  m_ifc.game->toKodi->HwGetProcAddress = cb_hw_get_proc_address;
  m_ifc.game->toKodi->InputEvent = cb_input_event;

  memset(m_ifc.game->toAddon, 0, sizeof(KodiToAddonFuncTable_Game));

  if (CreateInstance(&m_ifc) == ADDON_STATUS_OK)
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

  DestroyInstance(&m_ifc);
}

bool CGameClient::OpenFile(const CFileItem& file,
                           RETRO::IStreamManager& streamManager,
                           IGameInputCallback* input)
{
  // Check if we should open in standalone mode
  if (file.GetPath().empty())
    return false;

  // Some cores "succeed" to load the file even if it doesn't exist
  if (!CFileUtils::Exists(file.GetPath()))
  {

    // Failed to play game
    // The required files can't be found.
    MESSAGING::HELPERS::ShowOKDialogText(CVariant{35210}, CVariant{g_localizeStrings.Get(35219)});
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
  CLog::Log(LOGDEBUG, "GameClient: Loading {}", CURL::GetRedacted(path));

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!Initialized())
    return false;

  CloseFile();

  GAME_ERROR error = GAME_ERROR_FAILED;

  // Loading the game might require the stream subsystem to be initialized
  Streams().Initialize(streamManager);

  try
  {
    LogError(error = m_ifc.game->toAddon->LoadGame(m_ifc.game, path.c_str()), "LoadGame()");
  }
  catch (...)
  {
    LogException("LoadGame()");
  }

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

bool CGameClient::OpenStandalone(RETRO::IStreamManager& streamManager, IGameInputCallback* input)
{
  CLog::Log(LOGDEBUG, "GameClient: Loading {} in standalone mode", ID());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!Initialized())
    return false;

  CloseFile();

  GAME_ERROR error = GAME_ERROR_FAILED;

  // Loading the game might require the stream subsystem to be initialized
  Streams().Initialize(streamManager);

  try
  {
    LogError(error = m_ifc.game->toAddon->LoadStandalone(m_ifc.game), "LoadStandalone()");
  }
  catch (...)
  {
    LogException("LoadStandalone()");
  }

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

bool CGameClient::InitializeGameplay(const std::string& gamePath,
                                     RETRO::IStreamManager& streamManager,
                                     IGameInputCallback* input)
{
  if (LoadGameInfo())
  {
    Input().Start(input);

    m_bIsPlaying = true;
    m_gamePath = gamePath;
    m_input = input;

    m_inGameSaves = std::make_unique<CGameClientInGameSaves>(this, m_ifc.game);
    m_inGameSaves->Load();

    return true;
  }

  return false;
}

bool CGameClient::LoadGameInfo()
{
  bool bRequiresGameLoop;
  try
  {
    bRequiresGameLoop = m_ifc.game->toAddon->RequiresGameLoop(m_ifc.game);
  }
  catch (...)
  {
    LogException("RequiresGameLoop()");
    return false;
  }

  // Get information about system timings
  // Can be called only after retro_load_game()
  game_system_timing timingInfo = {};

  bool bSuccess = false;
  try
  {
    bSuccess =
        LogError(m_ifc.game->toAddon->GetGameTiming(m_ifc.game, &timingInfo), "GetGameTiming()");
  }
  catch (...)
  {
    LogException("GetGameTiming()");
  }

  if (!bSuccess)
  {
    CLog::Log(LOGERROR, "GameClient: Failed to get timing info");
    return false;
  }

  GAME_REGION region;
  try
  {
    region = m_ifc.game->toAddon->GetRegion(m_ifc.game);
  }
  catch (...)
  {
    LogException("GetRegion()");
    return false;
  }

  size_t serializeSize;
  try
  {
    serializeSize = m_ifc.game->toAddon->SerializeSize(m_ifc.game);
  }
  catch (...)
  {
    LogException("SerializeSize()");
    return false;
  }

  CLog::Log(LOGINFO, "GAME: ---------------------------------------");
  CLog::Log(LOGINFO, "GAME: Game loop:      {}", bRequiresGameLoop ? "true" : "false");
  CLog::Log(LOGINFO, "GAME: FPS:            {:f}", timingInfo.fps);
  CLog::Log(LOGINFO, "GAME: Sample Rate:    {:f}", timingInfo.sample_rate);
  CLog::Log(LOGINFO, "GAME: Region:         {}", CGameClientTranslator::TranslateRegion(region));
  CLog::Log(LOGINFO, "GAME: Savestate size: {}", serializeSize);
  CLog::Log(LOGINFO, "GAME: ---------------------------------------");

  m_bRequiresGameLoop = bRequiresGameLoop;
  m_serializeSize = serializeSize;
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
    MESSAGING::HELPERS::ShowOKDialogText(
        CVariant{35210},
        CVariant{StringUtils::Format(g_localizeStrings.Get(35211), missingResource)});
  }
  else
  {
    // Failed to play game
    // The emulator "%s" had an internal error.
    MESSAGING::HELPERS::ShowOKDialogText(
        CVariant{35210}, CVariant{StringUtils::Format(g_localizeStrings.Get(35213), Name())});
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
      const bool bInstalled =
          CServiceBroker::GetAddonMgr().GetAddon(strDependencyId, addon, OnlyEnabled::CHOICE_YES);
      if (!bInstalled)
      {
        strAddonId = strDependencyId;
        break;
      }
    }
  }

  return strAddonId;
}

void CGameClient::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bIsPlaying)
  {
    try
    {
      LogError(m_ifc.game->toAddon->Reset(m_ifc.game), "Reset()");
    }
    catch (...)
    {
      LogException("Reset()");
    }
  }
}

void CGameClient::CloseFile()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bIsPlaying)
  {
    m_inGameSaves->Save();
    m_inGameSaves.reset();

    m_bIsPlaying = false;
    m_gamePath.clear();
    m_serializeSize = 0;
    m_input = nullptr;

    Input().Stop();

    try
    {
      LogError(m_ifc.game->toAddon->UnloadGame(m_ifc.game), "UnloadGame()");
    }
    catch (...)
    {
      LogException("UnloadGame()");
    }

    Streams().Deinitialize();
  }
}

void CGameClient::RunFrame()
{
  IGameInputCallback* input;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    input = m_input;
  }

  if (input)
    input->PollInput();

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bIsPlaying)
  {
    try
    {
      LogError(m_ifc.game->toAddon->RunFrame(m_ifc.game), "RunFrame()");
    }
    catch (...)
    {
      LogException("RunFrame()");
    }
  }
}

bool CGameClient::Serialize(uint8_t* data, size_t size)
{
  if (data == nullptr || size == 0)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool bSuccess = false;
  if (m_bIsPlaying)
  {
    try
    {
      bSuccess = LogError(m_ifc.game->toAddon->Serialize(m_ifc.game, data, size), "Serialize()");
    }
    catch (...)
    {
      LogException("Serialize()");
    }
  }

  return bSuccess;
}

bool CGameClient::Deserialize(const uint8_t* data, size_t size)
{
  if (data == nullptr || size == 0)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool bSuccess = false;
  if (m_bIsPlaying)
  {
    try
    {
      bSuccess =
          LogError(m_ifc.game->toAddon->Deserialize(m_ifc.game, data, size), "Deserialize()");
    }
    catch (...)
    {
      LogException("Deserialize()");
    }
  }

  return bSuccess;
}

void CGameClient::LogAddonProperties(void) const
{
  CLog::Log(LOGINFO, "GAME: ------------------------------------");
  CLog::Log(LOGINFO, "GAME: Loaded DLL for {}", ID());
  CLog::Log(LOGINFO, "GAME: Client:              {}", Name());
  CLog::Log(LOGINFO, "GAME: Version:             {}", Version().asString());
  CLog::Log(LOGINFO, "GAME: Valid extensions:    {}", StringUtils::Join(m_extensions, " "));
  CLog::Log(LOGINFO, "GAME: Supports VFS:        {}", m_bSupportsVFS);
  CLog::Log(LOGINFO, "GAME: Supports standalone: {}", m_bSupportsStandalone);
  CLog::Log(LOGINFO, "GAME: ------------------------------------");
}

bool CGameClient::LogError(GAME_ERROR error, const char* strMethod) const
{
  if (error != GAME_ERROR_NO_ERROR)
  {
    CLog::Log(LOGERROR, "GAME - {} - addon '{}' returned an error: {}", strMethod, ID(),
              CGameClientTranslator::ToString(error));
    return false;
  }
  return true;
}

void CGameClient::LogException(const char* strFunctionName) const
{
  CLog::Log(LOGERROR, "GAME: exception caught while trying to call '{}' on add-on {}",
            strFunctionName, ID());
  CLog::Log(LOGERROR, "Please contact the developer of this add-on: {}", Author());
}

void CGameClient::cb_close_game(KODI_HANDLE kodiInstance)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                             static_cast<void*>(new CAction(ACTION_STOP)));
}

KODI_GAME_STREAM_HANDLE CGameClient::cb_open_stream(KODI_HANDLE kodiInstance,
                                                    const game_stream_properties* properties)
{
  if (properties == nullptr)
    return nullptr;

  CGameClient* gameClient = static_cast<CGameClient*>(kodiInstance);
  if (gameClient == nullptr)
    return nullptr;

  return gameClient->Streams().OpenStream(*properties);
}

bool CGameClient::cb_get_stream_buffer(KODI_HANDLE kodiInstance,
                                       KODI_GAME_STREAM_HANDLE stream,
                                       unsigned int width,
                                       unsigned int height,
                                       game_stream_buffer* buffer)
{
  if (buffer == nullptr)
    return false;

  IGameClientStream* gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return false;

  return gameClientStream->GetBuffer(width, height, *buffer);
}

void CGameClient::cb_add_stream_data(KODI_HANDLE kodiInstance,
                                     KODI_GAME_STREAM_HANDLE stream,
                                     const game_stream_packet* packet)
{
  if (packet == nullptr)
    return;

  IGameClientStream* gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return;

  gameClientStream->AddData(*packet);
}

void CGameClient::cb_release_stream_buffer(KODI_HANDLE kodiInstance,
                                           KODI_GAME_STREAM_HANDLE stream,
                                           game_stream_buffer* buffer)
{
  if (buffer == nullptr)
    return;

  IGameClientStream* gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return;

  gameClientStream->ReleaseBuffer(*buffer);
}

void CGameClient::cb_close_stream(KODI_HANDLE kodiInstance, KODI_GAME_STREAM_HANDLE stream)
{
  CGameClient* gameClient = static_cast<CGameClient*>(kodiInstance);
  if (gameClient == nullptr)
    return;

  IGameClientStream* gameClientStream = static_cast<IGameClientStream*>(stream);
  if (gameClientStream == nullptr)
    return;

  gameClient->Streams().CloseStream(gameClientStream);
}

game_proc_address_t CGameClient::cb_hw_get_proc_address(KODI_HANDLE kodiInstance, const char* sym)
{
  CGameClient* gameClient = static_cast<CGameClient*>(kodiInstance);
  if (!gameClient)
    return nullptr;

  //! @todo
  return nullptr;
}

bool CGameClient::cb_input_event(KODI_HANDLE kodiInstance, const game_input_event* event)
{
  CGameClient* gameClient = static_cast<CGameClient*>(kodiInstance);
  if (!gameClient)
    return false;

  if (event == nullptr)
    return false;

  return gameClient->Input().ReceiveInputEvent(*event);
}

std::pair<std::string, std::string> CGameClient::ParseLibretroName(const std::string& addonName)
{
  std::string emulatorName;
  std::string platforms;

  // libretro has a de-facto standard for naming their cores. If the
  // core emulates one or more platforms, then the format is:
  //
  //   "Platforms (Emulator name)"
  //
  // Otherwise, the format is just the name with no platforms:
  //
  //   "Emulator name"
  //
  // The has been observed for all 130 cores we package.
  //
  size_t beginPos = addonName.find('(');
  size_t endPos = addonName.find(')');

  if (beginPos != std::string::npos && endPos != std::string::npos && beginPos < endPos)
  {
    emulatorName = addonName.substr(beginPos + 1, endPos - beginPos - 1);
    platforms = addonName.substr(0, beginPos);
    StringUtils::TrimRight(platforms);
  }
  else
  {
    emulatorName = addonName;
    platforms.clear();
  }

  return std::make_pair(emulatorName, platforms);
}
