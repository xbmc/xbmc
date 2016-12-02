/*
 *      Copyright (C) 2012-2016 Team Kodi
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
#include "GameClientInput.h"
#include "GameClientKeyboard.h"
#include "GameClientMouse.h"
#include "GameClientTranslator.h"
#include "addons/AddonManager.h"
#include "addons/BinaryAddonCache.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "dialogs/GUIDialogOK.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/addons/playback/GameClientRealtimePlayback.h"
#include "games/addons/playback/GameClientReversiblePlayback.h"
#include "games/controllers/Controller.h"
#include "games/ports/PortManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/DefaultJoystick.h" // for DEFAULT_CONTROLLER_ID
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/Peripherals.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>

using namespace GAME;

#define EXTENSION_SEPARATOR          "|"
#define EXTENSION_WILDCARD           "*"

#define GAME_PROPERTY_EXTENSIONS           "extensions"
#define GAME_PROPERTY_SUPPORTS_VFS         "supports_vfs"
#define GAME_PROPERTY_SUPPORTS_STANDALONE  "supports_standalone"
#define GAME_PROPERTY_SUPPORTS_KEYBOARD    "supports_keyboard"
#define GAME_PROPERTY_SUPPORTS_MOUSE       "supports_mouse"

#define INPUT_SCAN_RATE  125 // Hz

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

std::unique_ptr<CGameClient> CGameClient::FromExtension(ADDON::AddonProps props, const cp_extension_t* ext)
{
  using namespace ADDON;

  static const std::vector<std::string> properties = {
      GAME_PROPERTY_EXTENSIONS,
      GAME_PROPERTY_SUPPORTS_VFS,
      GAME_PROPERTY_SUPPORTS_STANDALONE,
      GAME_PROPERTY_SUPPORTS_KEYBOARD,
      GAME_PROPERTY_SUPPORTS_MOUSE,
  };

  for (const auto& property : properties)
  {
    std::string strProperty = CAddonMgr::GetInstance().GetExtValue(ext->configuration, property.c_str());
    if (!strProperty.empty())
      props.extrainfo[property] = strProperty;
  }

  return std::unique_ptr<CGameClient>(new CGameClient(std::move(props)));
}

CGameClient::CGameClient(ADDON::AddonProps props) :
  CAddonDll<DllGameClient, GameClient, game_client_properties>(std::move(props)),
  m_apiVersion("0.0.0"),
  m_libraryProps(this, m_pInfo),
  m_bSupportsVFS(false),
  m_bSupportsStandalone(false),
  m_bSupportsKeyboard(false),
  m_bSupportsMouse(false),
  m_bSupportsAllExtensions(false),
  m_bIsPlaying(false),
  m_serializeSize(0),
  m_audio(nullptr),
  m_video(nullptr),
  m_region(GAME_REGION_UNKNOWN)
{
  const ADDON::InfoMap& extraInfo = m_props.extrainfo;
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

  it = extraInfo.find(GAME_PROPERTY_SUPPORTS_KEYBOARD);
  if (it != extraInfo.end())
    m_bSupportsKeyboard = (it->second == "true");

  it = extraInfo.find(GAME_PROPERTY_SUPPORTS_MOUSE);
  if (it != extraInfo.end())
    m_bSupportsMouse = (it->second == "true");

  ResetPlayback();
}

CGameClient::~CGameClient(void)
{
}

std::string CGameClient::LibPath() const
{
  // If the game client requires a proxy, load its DLL instead
  if (m_pInfo->proxy_dll_count > 0)
    return m_pInfo->proxy_dll_paths[0];

  return CAddon::LibPath();
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
  std::string savestatesDir = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetSavestatesFolder(), ID());
  if (!CDirectory::Exists(savestatesDir))
    CDirectory::Create(savestatesDir);

  m_libraryProps.InitializeProperties();

  if (Create() == ADDON_STATUS_OK)
  {
    LogAddonProperties();
    return true;
  }

  return false;
}

void CGameClient::Unload()
{
  Destroy();
}

bool CGameClient::OpenFile(const CFileItem& file, IGameAudioCallback* audio, IGameVideoCallback* video)
{
  if (audio == nullptr || video == nullptr)
    return false;

  // Check if we should open in standalone mode
  if (file.GetPath().empty())
    return OpenStandalone(audio, video);

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

  GAME_ERROR error = GAME_ERROR_FAILED;

  try { LogError(error = m_pStruct->LoadGame(path.c_str()), "LoadGame()"); }
  catch (...) { LogException("LoadGame()"); }

  if (error != GAME_ERROR_NO_ERROR)
  {
    NotifyError(error);
    return false;
  }

  if (!InitializeGameplay(file.GetPath(), audio, video))
    return false;

  return true;
}

bool CGameClient::OpenStandalone(IGameAudioCallback* audio, IGameVideoCallback* video)
{
  CLog::Log(LOGDEBUG, "GameClient: Loading %s in standalone mode", ID().c_str());

  CSingleLock lock(m_critSection);

  if (!Initialized())
    return false;

  CloseFile();

  GAME_ERROR error = GAME_ERROR_FAILED;

  try { LogError(error = m_pStruct->LoadStandalone(), "LoadStandalone()"); }
  catch (...) { LogException("LoadStandalone()"); }

  if (error != GAME_ERROR_NO_ERROR)
  {
    NotifyError(error);
    return false;
  }

  if (!InitializeGameplay(ID(), audio, video))
    return false;

  return true;
}

bool CGameClient::InitializeGameplay(const std::string& gamePath, IGameAudioCallback* audio, IGameVideoCallback* video)
{
  if (LoadGameInfo() && NormalizeAudio(audio))
  {
    m_bIsPlaying      = true;
    m_gamePath        = gamePath;
    m_serializeSize   = GetSerializeSize();
    m_audio           = audio;
    m_video           = video;
    m_inputRateHandle = PERIPHERALS::g_peripherals.SetEventScanRate(INPUT_SCAN_RATE);

    if (m_bSupportsKeyboard)
      OpenKeyboard();

    if (m_bSupportsMouse)
      OpenMouse();

    // Start playback
    CreatePlayback();

    return true;
  }

  return false;
}

bool CGameClient::NormalizeAudio(IGameAudioCallback* audioCallback)
{
  unsigned int originalSampleRate = m_timing.GetSampleRate();

  if (m_timing.NormalizeAudio(audioCallback))
  {
    const bool bChanged = (originalSampleRate != m_timing.GetSampleRate());
    if (bChanged)
    {
      CLog::Log(LOGDEBUG, "GAME: Correcting audio and video by %f to avoid resampling", m_timing.GetCorrectionFactor());
      CLog::Log(LOGDEBUG, "GAME: Audio sample rate normalized to %u", m_timing.GetSampleRate());
      CLog::Log(LOGDEBUG, "GAME: Video frame rate scaled to %f", m_timing.GetFrameRate());
    }
    else
    {
      CLog::Log(LOGDEBUG, "GAME: Audio sample rate is supported, no scaling or resampling needed");
    }
  }
  else
  {
    CLog::Log(LOGERROR, "GAME: Failed to normalize audio sample rate: exceeds %u%% difference", CGameClientTiming::MAX_CORRECTION_FACTOR_PERCENT);
    return false;
  }

  return true;
}

bool CGameClient::LoadGameInfo()
{
  // Get information about system audio/video timings and geometry
  // Can be called only after retro_load_game()
  game_system_av_info av_info = { };

  bool bSuccess = false;
  try { bSuccess = LogError(m_pStruct->GetGameInfo(&av_info), "GetGameInfo()"); }
  catch (...) { LogException("GetGameInfo()"); }

  if (!bSuccess)
    return false;

  GAME_REGION region;
  try { region = m_pStruct->GetRegion(); }
  catch (...) { LogException("GetRegion()"); return false; }

  CLog::Log(LOGINFO, "GAME: ---------------------------------------");
  CLog::Log(LOGINFO, "GAME: Base Width:   %u", av_info.geometry.base_width);
  CLog::Log(LOGINFO, "GAME: Base Height:  %u", av_info.geometry.base_height);
  CLog::Log(LOGINFO, "GAME: Max Width:    %u", av_info.geometry.max_width);
  CLog::Log(LOGINFO, "GAME: Max Height:   %u", av_info.geometry.max_height);
  CLog::Log(LOGINFO, "GAME: Aspect Ratio: %f", av_info.geometry.aspect_ratio);
  CLog::Log(LOGINFO, "GAME: FPS:          %f", av_info.timing.fps);
  CLog::Log(LOGINFO, "GAME: Sample Rate:  %f", av_info.timing.sample_rate);
  CLog::Log(LOGINFO, "GAME: Region:       %s", CGameClientTranslator::TranslateRegion(region));
  CLog::Log(LOGINFO, "GAME: ---------------------------------------");

  m_timing.SetFrameRate(av_info.timing.fps);
  m_timing.SetSampleRate(av_info.timing.sample_rate);
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
    CGUIDialogOK::ShowAndGetInput(CVariant{ 35210 }, StringUtils::Format(g_localizeStrings.Get(35211).c_str(), missingResource.c_str()));
  }
  else
  {
    // Failed to play game
    // The emulator "%s" had an internal error.
    CGUIDialogOK::ShowAndGetInput(CVariant{ 35210 }, StringUtils::Format(g_localizeStrings.Get(35213).c_str(), Name().c_str()));
  }
}

std::string CGameClient::GetMissingResource()
{
  using namespace ADDON;

  std::string strAddonId;

  const ADDONDEPS& dependencies = GetDeps();
  for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    const std::string& strDependencyId = it->first;
    if (StringUtils::StartsWith(strDependencyId, "resource.games"))
    {
      AddonPtr addon;
      const bool bInstalled = CAddonMgr::GetInstance().GetAddon(strDependencyId, addon);
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

  try { bRequiresGameLoop = m_pStruct->RequiresGameLoop(); }
  catch (...) { LogException("RequiresGameLoop()"); }

  if (bRequiresGameLoop)
  {
    m_playback.reset(new CGameClientReversiblePlayback(this, m_timing.GetFrameRate(), m_serializeSize));
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
    try { LogError(m_pStruct->Reset(), "Reset()"); }
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
    try { LogError(m_pStruct->UnloadGame(), "UnloadGame()"); }
    catch (...) { LogException("UnloadGame()"); }
  }

  ClearPorts();

  if (m_bSupportsKeyboard)
    CloseKeyboard();

  if (m_bSupportsMouse)
    CloseMouse();

  m_bIsPlaying = false;
  m_gamePath.clear();
  m_serializeSize = 0;
  if (m_inputRateHandle)
  {
    m_inputRateHandle->Release();
    m_inputRateHandle.reset();
  }

  m_audio = nullptr;
  m_video = nullptr;
  m_timing.Reset();
}

void CGameClient::RunFrame()
{
  CSingleLock lock(m_critSection);

  if (m_bIsPlaying)
  {
    try { LogError(m_pStruct->RunFrame(), "RunFrame()"); }
    catch (...) { LogException("RunFrame()"); }
  }
}

bool CGameClient::OpenPixelStream(GAME_PIXEL_FORMAT format, unsigned int width, unsigned int height, GAME_VIDEO_ROTATION rotation)
{
  if (!m_video)
    return false;

  AVPixelFormat pixelFormat = CGameClientTranslator::TranslatePixelFormat(format);
  if (pixelFormat == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "GAME: Unknown pixel format: %d", format);
    return false;
  }

  unsigned int orientation = 0;
  switch (rotation)
  {
  case GAME_VIDEO_ROTATION_90:
    orientation = 360 - 90;
    break;
  case GAME_VIDEO_ROTATION_180:
    orientation = 360 - 180;
    break;
  case GAME_VIDEO_ROTATION_270:
    orientation = 360 - 270;
    break;
  default:
    break;
  }

  return m_video->OpenPixelStream(pixelFormat, width, height, m_timing.GetFrameRate(), orientation);
}

bool CGameClient::OpenVideoStream(GAME_VIDEO_CODEC codec)
{
  if (!m_video)
    return false;

  AVCodecID videoCodec = CGameClientTranslator::TranslateVideoCodec(codec);
  if (videoCodec == AV_CODEC_ID_NONE)
  {
    CLog::Log(LOGERROR, "GAME: Unknown video format: %d", codec);
    return false;
  }

  return m_video->OpenEncodedStream(videoCodec);
}

bool CGameClient::OpenPCMStream(GAME_PCM_FORMAT format, const GAME_AUDIO_CHANNEL* channelMap)
{
  if (!m_audio || channelMap == nullptr)
    return false;

  AEDataFormat pcmFormat = CGameClientTranslator::TranslatePCMFormat(format);
  if (pcmFormat == AE_FMT_INVALID)
  {
    CLog::Log(LOGERROR, "GAME: Unknown PCM format: %d", format);
    return false;
  }

  CAEChannelInfo channelLayout;
  for (const GAME_AUDIO_CHANNEL* channelPtr = channelMap; *channelPtr != GAME_CH_NULL; channelPtr++)
  {
    AEChannel channel = CGameClientTranslator::TranslateAudioChannel(*channelPtr);
    if (channel == AE_CH_NULL)
    {
      CLog::Log(LOGERROR, "GAME: Unknown channel ID: %d", *channelPtr);
      return false;
    }
    channelLayout += channel;
  }

  return m_audio->OpenPCMStream(pcmFormat, m_timing.GetSampleRate(), channelLayout);
}

bool CGameClient::OpenAudioStream(GAME_AUDIO_CODEC codec, const GAME_AUDIO_CHANNEL* channelMap)
{
  if (!m_audio)
    return false;

  AVCodecID audioCodec = CGameClientTranslator::TranslateAudioCodec(codec);
  if (audioCodec == AV_CODEC_ID_NONE)
  {
    CLog::Log(LOGERROR, "GAME: Unknown audio codec: %d", codec);
    return false;
  }

  CAEChannelInfo channelLayout;
  for (const GAME_AUDIO_CHANNEL* channelPtr = channelMap; *channelPtr != GAME_CH_NULL; channelPtr++)
  {
    AEChannel channel = CGameClientTranslator::TranslateAudioChannel(*channelPtr);
    if (channel == AE_CH_NULL)
    {
      CLog::Log(LOGERROR, "GAME: Unknown channel ID: %d", *channelPtr);
      return false;
    }
    channelLayout += channel;
  }

  return m_audio->OpenEncodedStream(audioCodec, m_timing.GetSampleRate(), channelLayout);
}

void CGameClient::AddStreamData(GAME_STREAM_TYPE stream, const uint8_t* data, unsigned int size)
{
  switch (stream)
  {
  case GAME_STREAM_AUDIO:
  {
    if (m_audio)
      m_audio->AddData(data, size);
    break;
  }
  case GAME_STREAM_VIDEO:
  {
    if (m_video)
      m_video->AddData(data, size);
    break;
  }
  default:
    break;
  }
}

void CGameClient::CloseStream(GAME_STREAM_TYPE stream)
{
  switch (stream)
  {
  case GAME_STREAM_AUDIO:
  {
    if (m_audio)
      m_audio->CloseStream();
    break;
  }
  case GAME_STREAM_VIDEO:
  {
    if (m_video)
      m_video->CloseStream();
    break;
  }
  default:
    break;
  }
}

size_t CGameClient::GetSerializeSize()
{
  CSingleLock lock(m_critSection);

  size_t serializeSize = 0;
  if (m_bIsPlaying)
  {
    try { serializeSize = m_pStruct->SerializeSize(); }
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
    try { bSuccess = LogError(m_pStruct->Serialize(data, size), "Serialize()"); }
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
    try { bSuccess = LogError(m_pStruct->Deserialize(data, size), "Deserialize()"); }
    catch (...) { LogException("Deserialize()"); }
  }

  return bSuccess;
}

bool CGameClient::OpenPort(unsigned int port)
{
  // Fail if port is already open
  if (m_ports.find(port) != m_ports.end())
    return false;

  ControllerVector controllers = GetControllers();
  if (!controllers.empty())
  {
    //! @todo Choose controller
    ControllerPtr& controller = controllers[0];

    if (controller->LoadLayout())
    {
      m_ports[port].reset(new CGameClientInput(this, port, controller, m_pStruct));

      // If keyboard input is being captured by this add-on, force the port type to PERIPHERAL_JOYSTICK
      PERIPHERALS::PeripheralType device = PERIPHERALS::PERIPHERAL_UNKNOWN;
      if (m_bSupportsKeyboard)
        device = PERIPHERALS::PERIPHERAL_JOYSTICK;

      CPortManager::GetInstance().OpenPort(m_ports[port].get(), port, device);

      UpdatePort(port, controller);

      return true;
    }
  }

  return false;
}

void CGameClient::ClosePort(unsigned int port)
{
  // Can't close port if it doesn't exist
  if (m_ports.find(port) == m_ports.end())
    return;

  CPortManager::GetInstance().ClosePort(m_ports[port].get());

  m_ports.erase(port);

  UpdatePort(port, CController::EmptyPtr);
}

void CGameClient::UpdatePort(unsigned int port, const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  if (controller != CController::EmptyPtr)
  {
    std::string strId = controller->ID();

    game_controller controllerStruct;

    controllerStruct.controller_id        = strId.c_str();
    controllerStruct.digital_button_count = controller->Layout().FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
    controllerStruct.analog_button_count  = controller->Layout().FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
    controllerStruct.analog_stick_count   = controller->Layout().FeatureCount(FEATURE_TYPE::ANALOG_STICK);
    controllerStruct.accelerometer_count  = controller->Layout().FeatureCount(FEATURE_TYPE::ACCELEROMETER);
    controllerStruct.key_count            = 0; //! @todo
    controllerStruct.rel_pointer_count    = controller->Layout().FeatureCount(FEATURE_TYPE::RELPOINTER);
    controllerStruct.abs_pointer_count    = 0; //! @todo
    controllerStruct.motor_count          = controller->Layout().FeatureCount(FEATURE_TYPE::MOTOR);

    try { m_pStruct->UpdatePort(port, true, &controllerStruct); }
    catch (...) { LogException("UpdatePort()"); }
  }
  else
  {
    try { m_pStruct->UpdatePort(port, false, nullptr); }
    catch (...) { LogException("UpdatePort()"); }
  }
}

bool CGameClient::AcceptsInput(void) const
{
  return g_application.IsAppFocused() &&
         g_windowManager.GetActiveWindowID() == WINDOW_FULLSCREEN_GAME;
}

void CGameClient::ClearPorts(void)
{
  while (!m_ports.empty())
    ClosePort(m_ports.begin()->first);
}

ControllerVector CGameClient::GetControllers(void) const
{
  using namespace ADDON;

  ControllerVector controllers;

  const ADDONDEPS& dependencies = GetDeps();
  for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(it->first, addon, ADDON_GAME_CONTROLLER))
    {
      ControllerPtr controller = std::dynamic_pointer_cast<CController>(addon);
      if (controller)
        controllers.push_back(controller);
    }
  }

  if (controllers.empty())
  {
    // Use the default controller
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(DEFAULT_CONTROLLER_ID, addon, ADDON_GAME_CONTROLLER))
      controllers.push_back(std::static_pointer_cast<CController>(addon));
  }

  return controllers;
}

bool CGameClient::ReceiveInputEvent(const game_input_event& event)
{
  bool bHandled = false;

  switch (event.type)
  {
  case GAME_INPUT_EVENT_MOTOR:
    if (event.feature_name)
      bHandled = SetRumble(event.port, event.feature_name, event.motor.magnitude);
    break;
  default:
    break;
  }

  return bHandled;
}

bool CGameClient::SetRumble(unsigned int port, const std::string& feature, float magnitude)
{
  bool bHandled = false;

  if (m_ports.find(port) != m_ports.end())
    bHandled = m_ports[port]->SetRumble(feature, magnitude);

  return bHandled;
}

void CGameClient::OpenKeyboard(void)
{
  m_keyboard.reset(new CGameClientKeyboard(this, m_pStruct));
}

void CGameClient::CloseKeyboard(void)
{
  m_keyboard.reset();
}

void CGameClient::OpenMouse(void)
{
  m_mouse.reset(new CGameClientMouse(this, m_pStruct));

  std::string strId = m_mouse->ControllerID();

  game_controller controllerStruct = { strId.c_str() };

  try { m_pStruct->UpdatePort(GAME_INPUT_PORT_MOUSE, true, &controllerStruct); }
  catch (...) { LogException("UpdatePort()"); }
}

void CGameClient::CloseMouse(void)
{
  try { m_pStruct->UpdatePort(GAME_INPUT_PORT_MOUSE, false, nullptr); }
  catch (...) { LogException("UpdatePort()"); }

  m_mouse.reset();
}

void CGameClient::LogAddonProperties(void) const
{
  CLog::Log(LOGINFO, "GAME: ------------------------------------");
  CLog::Log(LOGINFO, "GAME: Loaded DLL for %s", ID().c_str());
  CLog::Log(LOGINFO, "GAME: Client: %s at version %s", Name().c_str(), Version().asString().c_str());
  CLog::Log(LOGINFO, "GAME: Valid extensions: %s", StringUtils::Join(m_extensions, " ").c_str());
  CLog::Log(LOGINFO, "GAME: Supports VFS:                  %s", m_bSupportsVFS ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: Supports standalone execution: %s", m_bSupportsStandalone ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: Supports keyboard:             %s", m_bSupportsKeyboard ? "yes" : "no");
  CLog::Log(LOGINFO, "GAME: Supports mouse:                %s", m_bSupportsMouse ? "yes" : "no");
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
