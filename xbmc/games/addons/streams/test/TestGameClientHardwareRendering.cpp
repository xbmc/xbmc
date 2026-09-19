/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "cores/RetroPlayer/playback/ReversiblePlayback.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/savestates/SavestateFlatBuffer.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientInGameSaves.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/addons/streams/GameClientStreams.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "windowing/WinSystem.h"

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#endif

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

AddonGlobalInterface* kodi::addon::CPrivateBase::m_interface = nullptr;

using namespace KODI;
using namespace KODI::GAME;

namespace
{
#if defined(HAS_GL) || (defined(HAS_GLES) && HAS_GLES >= 3)
constexpr bool HARDWARE_API_SUPPORTED = true;
#else
constexpr bool HARDWARE_API_SUPPORTED = false;
#endif

#if defined(HAS_GL)
class GuiContext : public CWinSystemBase, public CRenderSystemGL
{
public:
  GuiContext() : m_previous(CServiceBroker::GetWinSystem())
  {
    m_RenderVersionMajor = 3;
    m_RenderVersionMinor = 2;
    CServiceBroker::RegisterWinSystem(this);
  }
  ~GuiContext() override
  {
    if (m_previous)
      CServiceBroker::RegisterWinSystem(m_previous);
    else
      CServiceBroker::UnregisterWinSystem();
  }
  CRenderSystemBase* GetRenderSystem() override { return this; }
  bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return true; }
  bool ResizeWindow(int, int, int, int) override { return true; }
  bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return true; }
  void Register(IDispResource*) override {}
  void Unregister(IDispResource*) override {}
  void SetVSyncImpl(bool) override {}
  void PresentRenderImpl(bool) override {}

private:
  CWinSystemBase* m_previous;
};
#endif

// Explicit instantiation provides fixture access without production test hooks.
template<typename Tag, typename Tag::Type member>
struct MemberAccess
{
  friend typename Tag::Type GetMember(Tag) { return member; }
};

struct Playing
{
  using Type = std::atomic_bool CGameClient::*;
  friend Type GetMember(Playing);
};
template struct MemberAccess<Playing, &CGameClient::m_bIsPlaying>;

struct FrameRate
{
  using Type = std::atomic<double> CGameClient::*;
  friend Type GetMember(FrameRate);
};
template struct MemberAccess<FrameRate, &CGameClient::m_framerate>;

struct InGameSaves
{
  using Type = std::unique_ptr<CGameClientInGameSaves> CGameClient::*;
  friend Type GetMember(InGameSaves);
};
template struct MemberAccess<InGameSaves, &CGameClient::m_inGameSaves>;

struct InitializeGameplay
{
  using Type = bool (CGameClient::*)(const std::string&,
                                     RETRO::IStreamManager&,
                                     IGameInputCallback*);
  friend Type GetMember(InitializeGameplay);
};
template struct MemberAccess<InitializeGameplay, &CGameClient::InitializeGameplay>;

struct SupportsDiscs
{
  using Type = bool CGameClient::*;
  friend Type GetMember(SupportsDiscs);
};
template struct MemberAccess<SupportsDiscs, &CGameClient::m_supportsDiscControl>;

struct PersistedDiscs
{
  using Type = bool CGameClientDiscs::*;
  friend Type GetMember(PersistedDiscs);
};
template struct MemberAccess<PersistedDiscs, &CGameClientDiscs::m_hasPersistedState>;

struct Initialized
{
  using Type = bool ADDON::CAddonDll::*;
  friend Type GetMember(Initialized);
};
template struct MemberAccess<Initialized, &ADDON::CAddonDll::m_initialized>;

std::weak_ptr<IGameClientStream> GetStreamLifetime(CGameClientStreams& streams,
                                                   IGameClientStream* stream);
bool HasStreams(CGameClientStreams& streams);

template<auto member>
struct StreamAccess
{
  friend bool HasStreams(CGameClientStreams& streams) { return !(streams.*member).empty(); }
  friend std::weak_ptr<IGameClientStream> GetStreamLifetime(CGameClientStreams& streams,
                                                            IGameClientStream* stream)
  {
    return (streams.*member).at(stream).gameStream;
  }
};
template struct StreamAccess<&CGameClientStreams::m_streams>;

struct Core
{
  unsigned int frames{0};
  unsigned int sizeQueries{0};
  unsigned int serializations{0};
  unsigned int deserializations{0};
  unsigned int resets{0};
  unsigned int destroys{0};
  unsigned int unloads{0};
  unsigned int loads{0};
  unsigned int readyFrame{1};
  bool serializationNeedsReset{false};
  bool deserializeNeedsFrame{false};
  GAME_ERROR frameResult{GAME_ERROR_NO_ERROR};
  GAME_ERROR resetResult{GAME_ERROR_NO_ERROR};
  GAME_ERROR unloadResult{GAME_ERROR_NO_ERROR};
  std::function<void()> onFrame;
  std::function<void()> onReset;
  std::function<void()> onDestroy;
  std::function<void()> onUnload;
  std::function<GAME_ERROR()> onLoad;
};

Core& GetCore(const AddonInstance_Game* game)
{
  return *static_cast<Core*>(game->toAddon->addonInstance);
}

struct StreamState
{
  bool openResult{true};
  unsigned int opened{0};
  unsigned int closed{0};
  unsigned int deleted{0};
  std::function<void()> onClose;
};

class ProcessInfo : public RETRO::CRPProcessInfo
{
public:
  ProcessInfo() : CRPProcessInfo("test") {}
};

class RenderingStream : public RETRO::CRetroPlayerRendering
{
public:
  RenderingStream(RETRO::CRPRenderManager& renderer, ProcessInfo& process, StreamState& state)
    : CRetroPlayerRendering(renderer, process),
      m_state(state)
  {
  }
  ~RenderingStream() override { ++m_state.deleted; }
  bool OpenStream(const RETRO::StreamProperties&) override
  {
    ++m_state.opened;
    return m_open = m_state.openResult;
  }
  bool GetStreamBuffer(unsigned int width,
                       unsigned int height,
                       RETRO::StreamBuffer& buffer) override
  {
    if (!m_open || width == 0 || height == 0)
      return false;
    static_cast<RETRO::HwFramebufferBuffer&>(buffer).framebuffer = 42;
    return true;
  }
  void CloseStream() override
  {
    if (m_open)
    {
      ++m_state.closed;
      if (m_state.onClose)
        m_state.onClose();
    }
    m_open = false;
  }

private:
  StreamState& m_state;
  bool m_open{false};
};

class StreamManager : public RETRO::IStreamManager
{
public:
  RETRO::StreamPtr CreateStream(RETRO::StreamType) override
  {
    ++created;
    return factory ? factory() : RETRO::StreamPtr{};
  }
  void CloseStream(RETRO::StreamPtr stream) override
  {
    ++closed;
    stream->CloseStream();
  }
  void SetVideoFps(float) override {}
  RETRO::HwProcedureAddress GetHwProcedureAddress(const char*) override { return nullptr; }
  bool HasHardwareRendering() const override { return hardware; }
  bool BeginClientFrame() override
  {
    ++begins;
    if (!bind)
      return false;
    ++depth;
    return true;
  }
  void EndClientFrame() override
  {
    EXPECT_GT(depth, 0U);
    --depth;
    ++ends;
  }

  bool hardware{true};
  bool bind{true};
  unsigned int begins{0};
  unsigned int ends{0};
  unsigned int depth{0};
  unsigned int created{0};
  unsigned int closed{0};
  std::function<RETRO::StreamPtr()> factory;
};
class DevKitInstance
{
public:
  explicit DevKitInstance(CGameClientStreams& streams)
    : m_previous(kodi::addon::CPrivateBase::m_interface)
  {
    m_callbacks.kodiInstance = &streams;
    m_callbacks.OpenStream = [](KODI_HANDLE instance,
                                const game_stream_properties* properties) -> KODI_GAME_STREAM_HANDLE
    { return static_cast<CGameClientStreams*>(instance)->OpenStream(*properties); };
    m_callbacks.StartStream = [](KODI_HANDLE instance, KODI_GAME_STREAM_HANDLE stream)
    {
      return static_cast<CGameClientStreams*>(instance)->StartStream(
          static_cast<IGameClientStream*>(stream));
    };
    m_callbacks.CloseStream = [](KODI_HANDLE instance, KODI_GAME_STREAM_HANDLE stream)
    {
      static_cast<CGameClientStreams*>(instance)->CloseStream(
          static_cast<IGameClientStream*>(stream));
    };
    m_callbacks.GetStreamBuffer = [](KODI_HANDLE, KODI_GAME_STREAM_HANDLE stream,
                                     unsigned int width, unsigned int height,
                                     game_stream_buffer* buffer)
    { return static_cast<IGameClientStream*>(stream)->GetBuffer(width, height, *buffer); };
    m_instance.info = &m_info;
    m_instance.functions = &m_functions;
    m_instance.game = &m_game;
    m_global.firstKodiInstance = &m_instance;
    kodi::addon::CPrivateBase::m_interface = &m_global;
    m_addon = std::make_unique<kodi::addon::CInstanceGame>();
  }
  ~DevKitInstance()
  {
    m_addon.reset();
    kodi::addon::CPrivateBase::m_interface = m_previous;
  }

private:
  AddonGlobalInterface* m_previous;
  AddonToKodiFuncTable_Game m_callbacks{};
  KodiToAddonFuncTable_Game m_addonCallbacks{};
  AddonInstance_Game m_game{nullptr, &m_callbacks, &m_addonCallbacks};
  KODI_ADDON_INSTANCE_INFO m_info{};
  KODI_ADDON_INSTANCE_FUNC m_functions{};
  KODI_ADDON_INSTANCE_STRUCT m_instance{};
  AddonGlobalInterface m_global{};
  std::unique_ptr<kodi::addon::CInstanceGame> m_addon;
};
} // namespace

class TestGameClientHardwareRendering : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml =
        R"(<addon id="game.test.hardware" name="Hardware test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so"><extensions>rom</extensions></extension>
      <extension point="kodi.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_unique<CGameClient>(info);
    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->addonInstance = &m_core;
    callbacks->RunFrame = [](const AddonInstance_Game* game)
    {
      auto& core = GetCore(game);
      ++core.frames;
      if (core.onFrame)
        core.onFrame();
      return core.frameResult;
    };
    callbacks->AudioAvailable = [](const AddonInstance_Game*)
    { return GAME_ERROR_NOT_IMPLEMENTED; };
    callbacks->SerializeSize = [](const AddonInstance_Game* game) -> size_t
    {
      auto& core = GetCore(game);
      ++core.sizeQueries;
      return core.frames >= core.readyFrame && (!core.serializationNeedsReset || core.resets == 1)
                 ? 1
                 : 0;
    };
    callbacks->Serialize = [](const AddonInstance_Game* game, uint8_t* data, size_t)
    {
      ++GetCore(game).serializations;
      *data = 1;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->Deserialize = [](const AddonInstance_Game* game, const uint8_t*, size_t)
    {
      auto& core = GetCore(game);
      ++core.deserializations;
      return !core.deserializeNeedsFrame || core.frames > 0 ? GAME_ERROR_NO_ERROR
                                                            : GAME_ERROR_FAILED;
    };
    callbacks->DeserializeAchievements = [](const AddonInstance_Game*, const uint8_t*, size_t)
    { return GAME_ERROR_NOT_IMPLEMENTED; };
    callbacks->HwContextReset = [](const AddonInstance_Game* game)
    {
      auto& core = GetCore(game);
      ++core.resets;
      if (core.onReset)
        core.onReset();
      return core.resetResult;
    };
    callbacks->HwContextDestroy = [](const AddonInstance_Game* game)
    {
      auto& core = GetCore(game);
      ++core.destroys;
      if (core.onDestroy)
        core.onDestroy();
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->UnloadGame = [](const AddonInstance_Game* game)
    {
      auto& core = GetCore(game);
      ++core.unloads;
      if (core.onUnload)
        core.onUnload();
      return core.unloadResult;
    };
    callbacks->GetMemory = [](const AddonInstance_Game*, GAME_MEMORY, uint8_t**, size_t*)
    { return GAME_ERROR_NOT_IMPLEMENTED; };
    callbacks->CheatReset = [](const AddonInstance_Game*) { return GAME_ERROR_NOT_IMPLEMENTED; };
    callbacks->LoadStandalone = [](const AddonInstance_Game* game)
    {
      auto& core = GetCore(game);
      ++core.loads;
      return core.onLoad ? core.onLoad() : GAME_ERROR_NO_ERROR;
    };
    callbacks->SetRetroAchievementsCredentials =
        [](const AddonInstance_Game*, const char*, const char*)
    { return GAME_ERROR_NOT_IMPLEMENTED; };
    callbacks->RequiresGameLoop = [](const AddonInstance_Game*) { return true; };
    callbacks->GetGameTiming = [](const AddonInstance_Game*, game_system_timing* timing)
    {
      timing->fps = 60;
      timing->sample_rate = 48000;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->GetRegion = [](const AddonInstance_Game*) { return GAME_REGION_NTSC; };
    m_client.get()->*GetMember(Playing{}) = true;
    m_client.get()->*GetMember(FrameRate{}) = 60.0;
    m_client->Streams().Initialize(m_manager);
  }

  void TearDown() override
  {
    m_manager.bind = true;
    m_client->Streams().Deinitialize();
    m_client.get()->*GetMember(Playing{}) = false;
    m_client.get()->*GetMember(Initialized{}) = false;
    m_client.reset();
    EXPECT_EQ(m_manager.depth, 0U);
  }

  bool Negotiate()
  {
    game_hw_rendering_properties properties{};
#if defined(HAS_GLES)
    properties.context_type = GAME_HW_CONTEXT_OPENGLES3;
#elif defined(TARGET_DARWIN_OSX)
    properties.context_type = GAME_HW_CONTEXT_OPENGL_CORE;
    properties.version_major = 3;
    properties.version_minor = 3;
#else
    properties.context_type = GAME_HW_CONTEXT_OPENGL;
#endif
    return m_client->Streams().EnableHardwareRendering(properties);
  }

  IGameClientStream* OpenHardwareStream()
  {
    game_stream_properties properties{};
    properties.type = GAME_STREAM_HW_FRAMEBUFFER;
    properties.hw_framebuffer.max_width = 640;
    properties.hw_framebuffer.max_height = 480;
    auto* stream = m_client->Streams().OpenStream(properties);
    if (stream != nullptr && !m_client->Streams().StartStream(stream))
    {
      m_client->Streams().CloseStream(stream);
      return nullptr;
    }
    return stream;
  }

  void UseRenderingStream(RETRO::CPlaybackTestEnvironment& environment,
                          ProcessInfo& process,
                          StreamState& state)
  {
    m_manager.factory = [&environment, &process, &state]
    { return RETRO::StreamPtr(new RenderingStream(environment.Renderer(), process, state)); };
  }

  std::weak_ptr<IGameClientStream> ObserveStreamLifetime(IGameClientStream* stream)
  {
    return GetStreamLifetime(m_client->Streams(), stream);
  }

  void CheckStartupRestore(RETRO::CPlaybackTestEnvironment& environment)
  {
    std::unique_ptr<XFILE::CFile> file(XBMC_CREATETEMPFILE(".sav"));
    ASSERT_NE(file, nullptr);
    const std::string path = XBMC_TEMPFILEPATH(file.get());
    file->Close();
    RETRO::CSavestateFlatBuffer savestate;
    *savestate.GetMemoryBuffer(1) = 1;
    savestate.Finalize();
    RETRO::CSavestateDatabase database;
    ASSERT_TRUE(database.AddSavestate(path, {}, savestate));
    {
      RETRO::CReversiblePlayback playback(m_client.get(), environment.Renderer(),
                                          environment.Messenger(), 60.0, 0);
      EXPECT_EQ(m_core.sizeQueries, 0U);
      EXPECT_TRUE(playback.LoadSavestate(path));
      EXPECT_EQ(m_core.frames, 0U);
      EXPECT_EQ(m_core.sizeQueries, 1U);
      EXPECT_EQ(m_core.deserializations, 1U);
      EXPECT_EQ(m_client->GetSerializeSize(), 1U);
      EXPECT_EQ(m_core.sizeQueries, 1U);
    }
    EXPECT_TRUE(XBMC_DELETETEMPFILE(file.release()));
  }

  void PrepareCloseFile()
  {
    m_client.get()->*GetMember(InGameSaves{}) =
        std::make_unique<CGameClientInGameSaves>(m_client.get(), m_client->GetInstanceInterface());
  }

  void CheckFailedBindUnload(bool throws, GAME_ERROR result)
  {
    RETRO::CPlaybackTestEnvironment environment;
    ProcessInfo process;
    StreamState state;
    UseRenderingStream(environment, process, state);
    ASSERT_TRUE(Negotiate());
    ASSERT_NE(OpenHardwareStream(), nullptr);
    PrepareCloseFile();
    std::vector<std::string> events;
    state.onClose = [&events] { events.emplace_back("close"); };
    m_core.unloadResult = result;
    m_core.onUnload = [&]
    {
      events.emplace_back("unload");
      EXPECT_EQ(m_manager.depth, 0U);
      EXPECT_EQ(m_core.destroys, 0U);
      // Recovery during unload must not resurrect a destroy callback after it.
      m_manager.bind = true;
      if (throws)
        throw std::runtime_error("unload failed");
    };
    m_manager.bind = false;

    EXPECT_NO_THROW(m_client->CloseFile());
    EXPECT_FALSE(m_client->IsPlaying());
    EXPECT_EQ(m_core.unloads, 1U);
    EXPECT_EQ(m_core.destroys, 0U);
    EXPECT_EQ(state.closed, 1U);
    EXPECT_EQ(state.deleted, 1U);
    EXPECT_EQ(events, (std::vector<std::string>{"unload", "close"}));
    EXPECT_FALSE(HasStreams(m_client->Streams()));
    m_client->CloseFile();
    EXPECT_EQ(m_core.unloads, 1U);

    m_core.onUnload = {};
    m_core.unloadResult = GAME_ERROR_NO_ERROR;
    m_core.onLoad = [&]
    {
      return Negotiate() && OpenHardwareStream() != nullptr ? GAME_ERROR_NO_ERROR
                                                            : GAME_ERROR_FAILED;
    };
    m_client.get()->*GetMember(Initialized{}) = true;
    ASSERT_TRUE(m_client->OpenStandalone(m_manager, nullptr));
    EXPECT_EQ(m_core.loads, 1U);
    EXPECT_TRUE(m_client->IsPlaying());
    m_client->RunFrame(false);
    EXPECT_EQ(m_core.frames, 1U);
    m_client->CloseFile();
    EXPECT_EQ(m_core.unloads, 2U);
    EXPECT_EQ(m_core.destroys, 1U);
    EXPECT_EQ(state.closed, 2U);
    EXPECT_EQ(state.deleted, 2U);
  }

  Core m_core;
  StreamManager m_manager;
  std::unique_ptr<CGameClient> m_client;
};

TEST_F(TestGameClientHardwareRendering, CloseFileDestroysBeforeUnloadAndStreamTeardown)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  ASSERT_NE(OpenHardwareStream(), nullptr);
  PrepareCloseFile();
  std::vector<std::string> events;
  m_core.onDestroy = [&] { events.emplace_back("destroy"); };
  m_core.onUnload = [&]
  {
    EXPECT_GT(m_manager.depth, 0U);
    events.emplace_back("unload");
  };
  state.onClose = [&] { events.emplace_back("close"); };

  m_client->CloseFile();
  m_client->CloseFile();

  EXPECT_EQ(events, (std::vector<std::string>{"destroy", "unload", "close"}));
  EXPECT_EQ(m_core.unloads, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_FALSE(m_client->IsPlaying());
}

TEST_F(TestGameClientHardwareRendering, FailedBindStillUnloadsAndAllowsAnotherGame)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  CheckFailedBindUnload(false, GAME_ERROR_NO_ERROR);
}

TEST_F(TestGameClientHardwareRendering, FailedBindUnloadExceptionStillTearsDown)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  CheckFailedBindUnload(true, GAME_ERROR_NO_ERROR);
}

TEST_F(TestGameClientHardwareRendering, FailedBindUnloadErrorStillTearsDown)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  CheckFailedBindUnload(false, GAME_ERROR_FAILED);
}

TEST_F(TestGameClientHardwareRendering, FailedGameplayInitializationStillUnloadsWithoutContext)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  m_client.get()->*GetMember(Playing{}) = false;
  for (const std::string path : {"game.rom", ""})
  {
    m_client->Streams().Initialize(m_manager);
    m_manager.bind = true;
    ASSERT_TRUE(Negotiate());
    ASSERT_NE(OpenHardwareStream(), nullptr);
    const auto unloads = m_core.unloads;
    m_manager.bind = false;
    m_core.onUnload = [&] { m_manager.bind = true; };

    EXPECT_FALSE((m_client.get()->*GetMember(InitializeGameplay{}))(path, m_manager, nullptr));
    m_client->Streams().Deinitialize();
    m_client->CloseFile();

    EXPECT_EQ(m_core.unloads, unloads + 1);
    EXPECT_EQ(m_core.destroys, 0U);
    EXPECT_FALSE(m_client->IsPlaying());
    EXPECT_FALSE(HasStreams(m_client->Streams()));
  }
  EXPECT_EQ(state.deleted, 2U);
}

TEST_F(TestGameClientHardwareRendering, PersistedDiscRetryUnloadsWithoutContextBeforeReload)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  m_client.get()->*GetMember(Playing{}) = false;
  m_client.get()->*GetMember(SupportsDiscs{}) = true;
  m_client->Discs().*GetMember(PersistedDiscs{}) = true;
  ASSERT_TRUE(Negotiate());
  ASSERT_NE(OpenHardwareStream(), nullptr);
  std::vector<std::string> events;
  state.onClose = [&] { events.emplace_back("close"); };
  m_core.onUnload = [&]
  {
    events.emplace_back("unload");
    m_manager.bind = true;
  };
  m_core.onLoad = [&]
  {
    events.emplace_back("reload");
    EXPECT_EQ(m_core.unloads, 1U);
    EXPECT_EQ(m_core.destroys, 0U);
    EXPECT_EQ(state.deleted, 1U);
    EXPECT_FALSE(HasStreams(m_client->Streams()));
    return GAME_ERROR_FAILED;
  };
  m_client->GetInstanceInterface()->toAddon->LoadGame =
      [](const AddonInstance_Game* game, const char*) { return GetCore(game).onLoad(); };
  m_manager.bind = false;

  EXPECT_FALSE((m_client.get()->*GetMember(InitializeGameplay{}))("game.rom", m_manager, nullptr));
  m_client->Streams().Deinitialize();
  m_client->CloseFile();

  EXPECT_EQ(events, (std::vector<std::string>{"unload", "close", "reload"}));
  EXPECT_EQ(m_core.unloads, 1U);
  EXPECT_EQ(m_core.destroys, 0U);
  EXPECT_FALSE(m_client->IsPlaying());
}

TEST_F(TestGameClientHardwareRendering, FailedBindDoesNotInvokeClientOrRestoreAnotherScope)
{
  m_client->RunFrame(false);
  m_manager.bind = false;
  const auto ends = m_manager.ends;
  uint8_t data{1};

  m_client->RunFrame(false);
  EXPECT_EQ(m_client->GetSerializeSize(), 0U);
  EXPECT_FALSE(m_client->Serialize(&data, 1));
  EXPECT_EQ(m_client->Deserialize(&data, 1), RestoreResult::Rejected);
  EXPECT_FALSE(m_client->HardwareContextReset());
  m_client->HardwareContextDestroy();

  EXPECT_EQ(m_core.frames, 1U);
  EXPECT_EQ(m_core.sizeQueries, 0U);
  EXPECT_EQ(m_core.serializations, 0U);
  EXPECT_EQ(m_core.deserializations, 0U);
  EXPECT_EQ(m_core.resets, 0U);
  EXPECT_EQ(m_core.destroys, 0U);
  EXPECT_EQ(m_manager.ends, ends);

  m_manager.bind = true;
  EXPECT_EQ(m_client->GetSerializeSize(), 1U);
  EXPECT_EQ(m_core.sizeQueries, 1U);
}

TEST_F(TestGameClientHardwareRendering, FailedNestedBindLeavesOuterScopeCurrent)
{
  m_core.onFrame = [this]
  {
    m_manager.bind = false;
    uint8_t data{1};
    EXPECT_FALSE(m_client->Serialize(&data, 1));
    EXPECT_EQ(m_manager.depth, 1U);
    m_manager.bind = true;
  };

  m_client->RunFrame(false);

  EXPECT_EQ(m_manager.begins, 2U);
  EXPECT_EQ(m_manager.ends, 1U);
  EXPECT_EQ(m_core.serializations, 0U);
}

TEST_F(TestGameClientHardwareRendering, ClientExceptionRestoresContext)
{
  m_core.onFrame = [] { throw std::runtime_error("frame failed"); };

  m_client->RunFrame(false);

  EXPECT_EQ(m_manager.begins, 1U);
  EXPECT_EQ(m_manager.ends, 1U);
  EXPECT_EQ(m_client->GetSerializeSize(), 0U);
  EXPECT_EQ(m_core.sizeQueries, 0U);
}

TEST_F(TestGameClientHardwareRendering, SerializationWaitsForSuccessfulFrameAndRetriesZero)
{
  EXPECT_EQ(m_client->GetSerializeSize(), 0U);
  EXPECT_EQ(m_core.sizeQueries, 0U);
  m_core.frameResult = GAME_ERROR_FAILED;
  m_client->RunFrame(false);
  EXPECT_EQ(m_client->GetSerializeSize(), 0U);
  EXPECT_EQ(m_core.sizeQueries, 0U);

  m_core.frameResult = GAME_ERROR_NO_ERROR;
  m_core.readyFrame = 3;
  m_client->RunFrame(false);
  EXPECT_EQ(m_client->GetSerializeSize(), 0U);
  EXPECT_EQ(m_core.sizeQueries, 1U);
  m_client->RunFrame(false);
  EXPECT_EQ(m_client->GetSerializeSize(), 1U);
  EXPECT_EQ(m_client->GetSerializeSize(), 1U);
  EXPECT_EQ(m_core.sizeQueries, 2U);
}

TEST_F(TestGameClientHardwareRendering, SoftwareClientKeepsRunningWithoutHardwareSupport)
{
  m_manager.hardware = false;
  EXPECT_FALSE(Negotiate());
  m_client->RunFrame(false);
  uint8_t data{0};
  EXPECT_EQ(m_client->GetSerializeSize(), 1U);
  EXPECT_TRUE(m_client->Serialize(&data, 1));
  EXPECT_EQ(m_client->Deserialize(&data, 1), RestoreResult::Restored);
  EXPECT_EQ(m_core.frames, 1U);
  EXPECT_EQ(m_manager.ends, m_manager.begins);
}

TEST_F(TestGameClientHardwareRendering, RejectedNegotiationCannotReuseAcceptedProperties)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  ASSERT_TRUE(Negotiate());
  game_hw_rendering_properties properties{};
  properties.context_type = GAME_HW_CONTEXT_VULKAN;
  EXPECT_FALSE(m_client->Streams().EnableHardwareRendering(properties));
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_manager.created, 0U);
}

TEST_F(TestGameClientHardwareRendering, ExplicitContextVersionMustBeSpecified)
{
  game_hw_rendering_properties properties{};
#if defined(HAS_GLES)
  properties.context_type = GAME_HW_CONTEXT_OPENGLES_VERSION;
#else
  properties.context_type = GAME_HW_CONTEXT_OPENGL_CORE;
#endif
  EXPECT_FALSE(m_client->Streams().EnableHardwareRendering(properties));
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_manager.created, 0U);
}

#if defined(TARGET_DARWIN_OSX) && defined(HAS_GL)
TEST_F(TestGameClientHardwareRendering, LegacyOpenGLIsRefusedBeforeHardwareSelection)
{
  game_hw_rendering_properties properties{};
  properties.context_type = GAME_HW_CONTEXT_OPENGL;
  EXPECT_FALSE(m_client->Streams().EnableHardwareRendering(properties));
  EXPECT_TRUE(m_client->Streams().HardwareRenderingRefused());
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_manager.created, 0U);
}
#endif

#if defined(HAS_GL)
TEST_F(TestGameClientHardwareRendering, GuiContextVersionDoesNotLimitClientNegotiation)
{
  GuiContext guiContext;
  game_hw_rendering_properties properties{};
  properties.context_type = GAME_HW_CONTEXT_OPENGL_CORE;
  properties.version_major = 4;
  properties.version_minor = 3;

  EXPECT_TRUE(m_client->Streams().EnableHardwareRendering(properties));
  EXPECT_TRUE(m_client->Streams().HardwareRenderingRefusedWanted().empty());
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_manager.created, 1U);
  EXPECT_EQ(m_client->Streams().HardwareRenderingRefusedWanted(), "OpenGL 4.3");
}
#endif

TEST_F(TestGameClientHardwareRendering, DeinitializeClosesStreamsAndNotifiesDestroyOnce)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  ASSERT_NE(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_core.resets, 1U);
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_manager.created, 1U);

  m_client->Streams().DestroyHwContext();
  m_client->Streams().Deinitialize();
  m_client->Streams().Deinitialize();

  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefused());
}

TEST_F(TestGameClientHardwareRendering, DestroyNotifiesOnceWithoutClosingStream)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  auto* stream = OpenHardwareStream();
  ASSERT_NE(stream, nullptr);
  const auto lifetime = ObserveStreamLifetime(stream);

  m_client->Streams().DestroyHwContext();
  m_client->Streams().DestroyHwContext();

  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_FALSE(lifetime.expired());
  EXPECT_EQ(state.closed, 0U);
  EXPECT_EQ(m_manager.closed, 0U);
  m_client->Streams().CloseStream(stream);
  EXPECT_TRUE(lifetime.expired());
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);
}

TEST_F(TestGameClientHardwareRendering, DestroyMayCloseItsStreamWithoutDeletingActiveCallback)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  auto* stream = OpenHardwareStream();
  ASSERT_NE(stream, nullptr);
  const auto lifetime = ObserveStreamLifetime(stream);
  m_core.onDestroy = [&]
  {
    EXPECT_GT(m_manager.depth, 0U);
    m_client->Streams().CloseStream(stream);
    EXPECT_FALSE(lifetime.expired());
    m_client->Streams().DestroyHwContext();
  };

  m_client->Streams().DestroyHwContext();

  EXPECT_TRUE(lifetime.expired());
  EXPECT_FALSE(m_client->Streams().StartStream(stream));
  m_client->Streams().CloseStream(stream);
  m_client->Streams().DestroyHwContext();
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);
  EXPECT_EQ(m_manager.depth, 0U);

  m_core.onDestroy = {};
  ASSERT_TRUE(Negotiate());
  auto* reopened = OpenHardwareStream();
  ASSERT_NE(reopened, nullptr);
  EXPECT_EQ(m_core.resets, 2U);
  m_client->Streams().CloseStream(reopened);
  EXPECT_EQ(m_core.destroys, 2U);
  EXPECT_EQ(state.closed, 2U);
  EXPECT_EQ(state.deleted, 2U);
  EXPECT_EQ(m_manager.closed, 2U);
}

TEST_F(TestGameClientHardwareRendering, DestroyPreservesStreamReopenedByItsCallback)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  auto* stream = OpenHardwareStream();
  ASSERT_NE(stream, nullptr);
  const auto lifetime = ObserveStreamLifetime(stream);
  IGameClientStream* reopened = nullptr;
  m_core.onDestroy = [&]
  {
    m_client->Streams().CloseStream(stream);
    EXPECT_FALSE(lifetime.expired());
    ASSERT_TRUE(Negotiate());
    reopened = OpenHardwareStream();
  };

  m_client->Streams().DestroyHwContext();
  m_core.onDestroy = {};

  EXPECT_TRUE(lifetime.expired());
  ASSERT_NE(reopened, nullptr);
  EXPECT_TRUE(m_client->Streams().StartStream(reopened));
  EXPECT_EQ(m_core.resets, 2U);
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);
  game_stream_buffer buffer{};
  buffer.type = GAME_STREAM_HW_FRAMEBUFFER;
  EXPECT_TRUE(reopened->GetBuffer(640, 480, buffer));
  EXPECT_EQ(buffer.hw_framebuffer.framebuffer, 42U);
  m_client->Streams().CloseStream(reopened);
  EXPECT_EQ(m_core.destroys, 2U);
  EXPECT_EQ(state.closed, 2U);
  EXPECT_EQ(state.deleted, 2U);
  EXPECT_EQ(m_manager.closed, 2U);
}

TEST_F(TestGameClientHardwareRendering, ResetFailureClosesStreamAndAllowsRenegotiation)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  m_core.resetResult = GAME_ERROR_FAILED;
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);

  ASSERT_TRUE(Negotiate());
  m_core.resetResult = GAME_ERROR_NO_ERROR;
  auto* stream = OpenHardwareStream();
  ASSERT_NE(stream, nullptr);
  m_client->Streams().CloseStream(stream);
  m_client->Streams().CloseStream(stream);
  EXPECT_EQ(state.closed, 2U);
  EXPECT_EQ(state.deleted, 2U);
  EXPECT_EQ(m_core.destroys, 2U);
}

TEST_F(TestGameClientHardwareRendering, StreamOpenFailureReleasesManagerOwnership)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  state.openResult = false;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());

  EXPECT_EQ(OpenHardwareStream(), nullptr);

  EXPECT_EQ(m_core.resets, 0U);
  EXPECT_EQ(m_core.destroys, 0U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);
}

TEST_F(TestGameClientHardwareRendering, RewindRetriesUntilSerializationBecomesAvailable)
{
  RETRO::CPlaybackTestEnvironment environment;
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const bool rewindEnabled = settings->GetBool("gamesgeneral.enablerewind");
  settings->SetBool("gamesgeneral.enablerewind", true);
  m_core.readyFrame = 3;
  {
    RETRO::CReversiblePlayback playback(m_client.get(), environment.Renderer(),
                                        environment.Messenger(), 60.0, 0);
    playback.FrameEvent();
    playback.FrameEvent();
    EXPECT_EQ(m_core.serializations, 0U);
    playback.FrameEvent();
    EXPECT_EQ(m_core.serializations, 1U);
    playback.FrameEvent();
    EXPECT_EQ(m_core.serializations, 2U);
    EXPECT_EQ(m_core.sizeQueries, 3U);
  }
  settings->SetBool("gamesgeneral.enablerewind", rewindEnabled);
}

TEST_F(TestGameClientHardwareRendering, DevKitInstallsHandleBeforeSingleReset)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  DevKitInstance addon(m_client->Streams());
  kodi::addon::CInstanceGame::CStream stream;
  m_core.onReset = [&]
  {
    EXPECT_GT(m_manager.depth, 0U);
    EXPECT_TRUE(stream.IsOpen());
    game_stream_buffer buffer{};
    buffer.type = GAME_STREAM_HW_FRAMEBUFFER;
    EXPECT_TRUE(stream.GetBuffer(640, 480, buffer));
    EXPECT_EQ(buffer.hw_framebuffer.framebuffer, 42U);
  };
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 640;
  properties.hw_framebuffer.max_height = 480;
  EXPECT_TRUE(stream.Open(properties));
  EXPECT_EQ(m_core.resets, 1U);
  stream.Close();
  stream.Close();
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
}

TEST_F(TestGameClientHardwareRendering, PreparedStreamDoesNotResetUntilStarted)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 640;
  properties.hw_framebuffer.max_height = 480;
  auto* stream = m_client->Streams().OpenStream(properties);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(m_core.resets, 0U);
  EXPECT_TRUE(m_client->Streams().StartStream(stream));
  EXPECT_TRUE(m_client->Streams().StartStream(stream));
  EXPECT_EQ(m_core.resets, 1U);
  m_core.onDestroy = [&] { EXPECT_GT(m_manager.depth, 0U); };
  m_client->Streams().DestroyHwContext();
  EXPECT_FALSE(m_client->Streams().StartStream(stream));
  m_client->Streams().CloseStream(stream);
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
}

TEST_F(TestGameClientHardwareRendering, DevKitResetFailureClosesOnceAndReopens)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  DevKitInstance addon(m_client->Streams());
  kodi::addon::CInstanceGame::CStream stream;
  m_core.onReset = [&]
  {
    EXPECT_TRUE(stream.IsOpen());
    EXPECT_GT(m_manager.depth, 0U);
  };
  m_core.onDestroy = [&]
  {
    EXPECT_TRUE(stream.IsOpen());
    EXPECT_GT(m_manager.depth, 0U);
  };
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 640;
  properties.hw_framebuffer.max_height = 480;
  m_core.resetResult = GAME_ERROR_FAILED;
  EXPECT_FALSE(stream.Open(properties));
  EXPECT_FALSE(stream.IsOpen());
  stream.Close();
  EXPECT_EQ(m_core.resets, 1U);
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.closed, 1U);
  EXPECT_EQ(state.deleted, 1U);
  EXPECT_EQ(m_manager.closed, 1U);

  ASSERT_TRUE(Negotiate());
  m_core.resetResult = GAME_ERROR_NO_ERROR;
  EXPECT_TRUE(stream.Open(properties));
  stream.Close();
  EXPECT_EQ(m_core.resets, 2U);
  EXPECT_EQ(m_core.destroys, 2U);
  EXPECT_EQ(state.closed, 2U);
}

TEST_F(TestGameClientHardwareRendering, SoftwareStartupSavestateLoadsBeforeFirstFrame)
{
  RETRO::CPlaybackTestEnvironment environment;
  m_core.readyFrame = 0;
  CheckStartupRestore(environment);
}

TEST_F(TestGameClientHardwareRendering, HardwareStartupSavestateLoadsAfterResetBeforeFirstFrame)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  auto* stream = OpenHardwareStream();
  ASSERT_NE(stream, nullptr);
  m_core.readyFrame = 0;
  m_core.serializationNeedsReset = true;
  CheckStartupRestore(environment);
  m_client->Streams().CloseStream(stream);
}

TEST_F(TestGameClientHardwareRendering, DeserializeRetainsFirstFrameFallback)
{
  m_core.deserializeNeedsFrame = true;
  uint8_t data{1};
  EXPECT_EQ(m_client->Deserialize(&data, 1), RestoreResult::Restored);
  EXPECT_EQ(m_core.frames, 1U);
  EXPECT_EQ(m_core.deserializations, 2U);
}

TEST_F(TestGameClientHardwareRendering, PreFrameZeroSizeDoesNotDisableLazyRetry)
{
  EXPECT_EQ(m_client->GetSerializeSize(CGameClient::SerializeSizeMode::Restore), 0U);
  EXPECT_EQ(m_core.sizeQueries, 1U);
  EXPECT_EQ(m_client->GetSerializeSize(), 0U);
  EXPECT_EQ(m_core.sizeQueries, 1U);
  m_client->RunFrame(false);
  EXPECT_EQ(m_client->GetSerializeSize(), 1U);
  EXPECT_EQ(m_client->GetSerializeSize(CGameClient::SerializeSizeMode::Restore), 1U);
  EXPECT_EQ(m_core.sizeQueries, 2U);
}

TEST_F(TestGameClientHardwareRendering, AcceptedHardwarePreservesUnrelatedLoadErrors)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefused());
  auto* stream = OpenHardwareStream();
  ASSERT_NE(stream, nullptr);
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefused());
  EXPECT_TRUE(m_client->Streams().HardwareRenderingRefusedWanted().empty());
  m_client->Streams().CloseStream(stream);
}

TEST_F(TestGameClientHardwareRendering, RefusedHardwareClassifiesLoadFailure)
{
  game_hw_rendering_properties properties{};
  properties.context_type = GAME_HW_CONTEXT_VULKAN;
  EXPECT_FALSE(m_client->Streams().EnableHardwareRendering(properties));
  EXPECT_TRUE(m_client->Streams().HardwareRenderingRefused());
  EXPECT_EQ(m_client->Streams().HardwareRenderingRefusedWanted(), "Vulkan");
}

TEST_F(TestGameClientHardwareRendering, ContextCreationFailureRecordsRequest)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  state.openResult = false;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  EXPECT_EQ(OpenHardwareStream(), nullptr);
  EXPECT_TRUE(m_client->Streams().HardwareRenderingRefused());
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefusedWanted().empty());
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefusedAvailable().empty());
  EXPECT_EQ(m_core.resets, 0U);
}

TEST_F(TestGameClientHardwareRendering, SoftwareLoadErrorsHaveNoHardwareRefusal)
{
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefused());
  EXPECT_TRUE(m_client->Streams().HardwareRenderingRefusedWanted().empty());
}

TEST_F(TestGameClientHardwareRendering, ResetMayCloseItsStreamWithoutDeletingActiveCallback)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  DevKitInstance addon(m_client->Streams());
  kodi::addon::CInstanceGame::CStream stream;
  m_core.onReset = [&] { stream.Close(); };
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 640;
  properties.hw_framebuffer.max_height = 480;
  EXPECT_FALSE(stream.Open(properties));
  EXPECT_FALSE(stream.IsOpen());
  EXPECT_EQ(m_core.resets, 1U);
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_EQ(state.deleted, 1U);
}

TEST_F(TestGameClientHardwareRendering, FailedOuterResetDoesNotCloseReopenedStream)
{
  if (!HARDWARE_API_SUPPORTED)
    GTEST_SKIP() << "This build has no supported hardware rendering API";

  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  StreamState state;
  UseRenderingStream(environment, process, state);
  ASSERT_TRUE(Negotiate());
  DevKitInstance addon(m_client->Streams());
  kodi::addon::CInstanceGame::CStream stream;
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 640;
  properties.hw_framebuffer.max_height = 480;
  m_core.onReset = [&]
  {
    if (m_core.resets == 1)
    {
      stream.Close();
      ASSERT_TRUE(Negotiate());
      ASSERT_TRUE(stream.Open(properties));
      m_core.resetResult = GAME_ERROR_FAILED;
    }
  };
  EXPECT_FALSE(stream.Open(properties));
  EXPECT_TRUE(stream.IsOpen());
  EXPECT_EQ(m_core.resets, 2U);
  EXPECT_EQ(m_core.destroys, 1U);
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefused());
  stream.Close();
  EXPECT_EQ(m_core.destroys, 2U);
  EXPECT_EQ(state.closed, 2U);
  EXPECT_EQ(state.deleted, 2U);
}

TEST_F(TestGameClientHardwareRendering, SoftwareFallbackClearsHardwareRefusal)
{
  RETRO::CPlaybackTestEnvironment environment;
  ProcessInfo process;
  game_hw_rendering_properties hardware{};
  hardware.context_type = GAME_HW_CONTEXT_VULKAN;
  ASSERT_FALSE(m_client->Streams().EnableHardwareRendering(hardware));
  ASSERT_TRUE(m_client->Streams().HardwareRenderingRefused());
  m_manager.factory = [&]
  { return RETRO::StreamPtr(new RETRO::CRetroPlayerVideo(environment.Renderer(), process)); };
  game_stream_properties properties{};
  properties.type = GAME_STREAM_VIDEO;
  properties.video.format = GAME_PIXEL_FORMAT_0RGB8888;
  properties.video.nominal_width = properties.video.max_width = 320;
  properties.video.nominal_height = properties.video.max_height = 240;
  auto* stream = m_client->Streams().OpenStream(properties);
  ASSERT_NE(stream, nullptr);
  EXPECT_FALSE(m_client->Streams().HardwareRenderingRefused());
  EXPECT_EQ(m_core.resets, 0U);
  m_client->Streams().CloseStream(stream);
}
