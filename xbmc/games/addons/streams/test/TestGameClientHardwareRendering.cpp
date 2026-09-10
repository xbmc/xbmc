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
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "games/addons/GameClient.h"
#include "games/addons/streams/GameClientStreams.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/XBMCTinyXML2.h"
#include "windowing/WinSystem.h"

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#endif

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

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

struct Core
{
  unsigned int frames{0};
  unsigned int sizeQueries{0};
  unsigned int serializations{0};
  unsigned int deserializations{0};
  unsigned int resets{0};
  unsigned int destroys{0};
  unsigned int readyFrame{1};
  GAME_ERROR frameResult{GAME_ERROR_NO_ERROR};
  GAME_ERROR resetResult{GAME_ERROR_NO_ERROR};
  std::function<void()> onFrame;
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
  void CloseStream() override
  {
    if (m_open)
      ++m_state.closed;
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
} // namespace

class TestGameClientHardwareRendering : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml =
        R"(<addon id="game.test.hardware" name="Hardware test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so" />
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
      return core.frames >= core.readyFrame ? 1 : 0;
    };
    callbacks->Serialize = [](const AddonInstance_Game* game, uint8_t* data, size_t)
    {
      ++GetCore(game).serializations;
      *data = 1;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->Deserialize = [](const AddonInstance_Game* game, const uint8_t*, size_t)
    {
      ++GetCore(game).deserializations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->HwContextReset = [](const AddonInstance_Game* game)
    {
      ++GetCore(game).resets;
      return GetCore(game).resetResult;
    };
    callbacks->HwContextDestroy = [](const AddonInstance_Game* game)
    {
      ++GetCore(game).destroys;
      return GAME_ERROR_NO_ERROR;
    };
    m_client.get()->*GetMember(Playing{}) = true;
    m_client.get()->*GetMember(FrameRate{}) = 60.0;
    m_client->Streams().Initialize(m_manager);
  }

  void TearDown() override
  {
    m_manager.bind = true;
    m_client->Streams().Deinitialize();
    m_client.get()->*GetMember(Playing{}) = false;
    m_client.reset();
    EXPECT_EQ(m_manager.depth, 0U);
  }

  bool Negotiate()
  {
    game_hw_rendering_properties properties{};
#if defined(HAS_GLES)
    properties.context_type = GAME_HW_CONTEXT_OPENGLES3;
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
    return m_client->Streams().OpenStream(properties);
  }

  void UseRenderingStream(RETRO::CPlaybackTestEnvironment& environment,
                          ProcessInfo& process,
                          StreamState& state)
  {
    m_manager.factory = [&environment, &process, &state]
    { return RETRO::StreamPtr(new RenderingStream(environment.Renderer(), process, state)); };
  }

  Core m_core;
  StreamManager m_manager;
  std::unique_ptr<CGameClient> m_client;
};

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
  EXPECT_FALSE(m_client->Streams().HardwareRenderingAttempted());
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
