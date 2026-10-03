/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/DataCacheCore.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"
#include "games/addons/streams/GameClientStreamHwFramebuffer.h"

#include <algorithm>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
class CTestBuffer : public CRenderBufferSysMem
{
public:
  bool UploadTexture() override { return true; }
  int References() const { return m_refCount; }
};

class CHardwareTestBuffer : public CTestBuffer
{
public:
  bool Allocate(AVPixelFormat, unsigned int width, unsigned int height) override
  {
    ++allocations;
    if (throwOnAllocation)
      throw std::runtime_error("allocation failed");
    if (failAllocation)
      return false;
    SetSize(width, height);
    return true;
  }
  uintptr_t GetCurrentFramebuffer() override { return 17; }
  bool failAllocation{false};
  bool throwOnAllocation{false};
  unsigned int allocations{0};
};

class CTestPool : public CBaseRenderBufferPool
{
public:
  bool IsCompatible(const CRenderVideoSettings&) const override
  {
    if (auto callback = std::exchange(onCompatibilityCheck, {}))
      callback();
    return compatible;
  }

  bool SupportsHardwareRendering() const override { return hardware; }
  bool CreateContext(const HwContextProperties& properties) override
  {
    ++creates;
    contextProperties = properties;
    return createSucceeds && !(properties.debugContext && refuseDebugContext);
  }
  bool BeginClientFrame() override
  {
    ++begins;
    return bindSucceeds;
  }
  void EndClientFrame() override { ++ends; }
  void DestroyContext() override { ++destroys; }
  void FlushRendered() override
  {
    if (onFlushRendered)
      onFlushRendered();
  }
  void Return(IRenderBuffer* buffer) override
  {
    if (onReturn)
      onReturn();
    CBaseRenderBufferPool::Return(buffer);
  }
  void Flush() override
  {
    if (onFlush)
      onFlush();
    CBaseRenderBufferPool::Flush();
  }
  IRenderBuffer* GetBuffer(unsigned int width, unsigned int height) override
  {
    lastBuffer = static_cast<CTestBuffer*>(CBaseRenderBufferPool::GetBuffer(width, height));
    return lastBuffer;
  }

  IRenderBuffer* CaptureClientFrame(IRenderBuffer*,
                                    unsigned int width,
                                    unsigned int height) override
  {
    ++captures;
    if (failCapture)
      return nullptr;
    captured = static_cast<CHardwareTestBuffer*>(GetBuffer(width, height));
    return captured;
  }
  CHardwareTestBuffer* clientBuffer{nullptr};
  CHardwareTestBuffer* captured{nullptr};
  unsigned int captures{0};
  bool failCapture{false};
  bool hardware{false};
  bool compatible{true};
  bool restrictFormat{false};
  CTestBuffer* lastBuffer{nullptr};
  HwContextProperties contextProperties;
  AVPixelFormat rendererFormat{AV_PIX_FMT_NONE};
  bool createSucceeds{true};
  bool refuseDebugContext{false};
  bool bindSucceeds{true};
  unsigned int creates{0};
  unsigned int begins{0};
  unsigned int ends{0};
  unsigned int destroys{0};
  mutable std::function<void()> onCompatibilityCheck;
  std::function<void()> onFlushRendered;
  std::function<void()> onReturn;
  std::function<void()> onFlush;
  std::function<void()> onRendererFlush;
  CRPBaseRenderer* renderer{nullptr};

protected:
  bool ConfigureInternal() override
  {
    return !restrictFormat ||
           (hardware ? m_format == AV_PIX_FMT_NONE : m_format != AV_PIX_FMT_NONE);
  }

  IRenderBuffer* CreateRenderBuffer(void*) override
  {
    if (!hardware)
      return new CTestBuffer;
    auto* buffer = new CHardwareTestBuffer;
    if (!clientBuffer)
      clientBuffer = buffer;
    return buffer;
  }
};

class CTestRenderer : public CRPBaseRenderer
{
public:
  using CRPBaseRenderer::CRPBaseRenderer;
  bool Supports(RENDERFEATURE) const override { return false; }
  SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

protected:
  bool ConfigureInternal() override
  {
    static_cast<CTestPool*>(GetBufferPool())->rendererFormat = m_format;
    return true;
  }

  void RenderInternal(bool, uint8_t) override {}
  void FlushInternal() override
  {
    const auto& callback = static_cast<CTestPool*>(GetBufferPool())->onRendererFlush;
    if (callback)
      callback();
  }
};

class CTestHardwareCallback : public KODI::GAME::IHwFramebufferCallback
{
public:
  bool HardwareContextReset() override { return true; }
  void HardwareContextDestroy() override {}
};

class CTestRendererFactory : public IRendererFactory
{
public:
  explicit CTestRendererFactory(RenderBufferPoolVector pools) : m_pools(std::move(pools)) {}
  std::string RenderSystemName() const override { return "test"; }
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> pool) override
  {
    auto* renderer = new CTestRenderer(settings, context, pool);
    static_cast<CTestPool*>(pool.get())->renderer = renderer;
    return renderer;
  }
  RenderBufferPoolVector CreateBufferPools(CRenderContext&) override { return m_pools; }

private:
  RenderBufferPoolVector m_pools;
};

class CScopedRendererFactories : public CRPProcessInfo
{
public:
  class CRegistration
  {
  public:
    explicit CRegistration(RenderBufferPoolVector pools)
    {
      std::unique_lock lock(m_createSection);
      m_previous = std::move(m_rendererFactories);
      m_rendererFactories.clear();
      m_rendererFactories.emplace_back(std::make_unique<CTestRendererFactory>(std::move(pools)));
    }
    ~CRegistration()
    {
      std::unique_lock lock(m_createSection);
      m_rendererFactories = std::move(m_previous);
    }

  private:
    std::vector<std::unique_ptr<IRendererFactory>> m_previous;
  };
};
} // namespace

class TestRPRenderManager : public testing::Test
{
protected:
  void TearDown() override
  {
    m_environment.Renderer().Flush();
    m_environment.Renderer().Deinitialize();
    m_environment.Renderer().FrameMove();
  }

  void Configure()
  {
    ASSERT_TRUE(m_environment.Renderer().Configure(AV_PIX_FMT_BGR0, 160, 144, 0.0f, 256, 224));
  }

  void RenderControl()
  {
    // With no submitted frame, this exercises renderer creation without GPU drawing.
    m_environment.Renderer().RenderControl(false, false, CRect{}, nullptr);
  }

  void ExpectVideoBuffer()
  {
    VideoStreamBuffer buffer{};
    ASSERT_TRUE(m_environment.Renderer().GetVideoBuffer(160, 144, buffer));
    EXPECT_NE(buffer.data, nullptr);
  }

  void Start()
  {
    m_environment.Renderer().Initialize();
    Configure();
    m_environment.Renderer().FrameMove();
    RenderControl();
    ASSERT_TRUE(m_pool->IsConfigured());
    ExpectVideoBuffer();
  }

  void Reset()
  {
    auto& manager = m_environment.Renderer();
    manager.Flush();
    manager.Deinitialize();
    manager.Initialize();
    Configure();
  }

  std::shared_ptr<CTestPool> m_pool = std::make_shared<CTestPool>();
  CScopedRendererFactories::CRegistration m_factories{{m_pool}};
  CPlaybackTestEnvironment m_environment;
};

TEST_F(TestRPRenderManager, InitialStartup)
{
  m_environment.Renderer().Initialize();
  Configure();
  RenderControl();
  m_environment.Renderer().FrameMove();
  RenderControl();
  ExpectVideoBuffer();
}

TEST_F(TestRPRenderManager, ResetWithFlushBeforeRendering)
{
  Start();
  Reset();
  m_environment.Renderer().FrameMove();
  EXPECT_FALSE(m_pool->IsConfigured());
  RenderControl();
  ExpectVideoBuffer();
}

TEST_F(TestRPRenderManager, ResetWithRenderingBeforeFlush)
{
  Start();
  Reset();
  RenderControl();
  m_environment.Renderer().FrameMove();

  for (unsigned int frame = 0; frame < 3; ++frame)
  {
    RenderControl();
    EXPECT_TRUE(m_pool->IsConfigured());
    ExpectVideoBuffer();
    m_environment.Renderer().FrameMove();
  }
}

TEST_F(TestRPRenderManager, ResetDuringRendererLookup)
{
  Start();
  // Interrupt lookup after its state check, before it acquires the renderer mutex.
  m_pool->onCompatibilityCheck = [this] { Reset(); };
  RenderControl();
  m_environment.Renderer().FrameMove();
  RenderControl();
  ExpectVideoBuffer();
}

TEST_F(TestRPRenderManager, SoftwareCallsDoNotBindPools)
{
  auto& manager = m_environment.Renderer();
  ASSERT_TRUE(manager.BeginClientFrame());
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->begins, 0);
  EXPECT_EQ(m_pool->ends, 0);
  EXPECT_FALSE(manager.CreateContext({}));
  EXPECT_EQ(m_pool->creates, 0);
}

TEST_F(TestRPRenderManager, ContextCreatedInsideNestedClientCall)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  ASSERT_TRUE(manager.BeginClientFrame());
  EXPECT_TRUE(manager.CreateContext({}));
  EXPECT_EQ(m_pool->begins, 1);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 0);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 1);
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 1);
}

TEST_F(TestRPRenderManager, FailedContextBindUnwindsCreation)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  m_pool->bindSucceeds = false;
  ASSERT_TRUE(manager.BeginClientFrame());
  EXPECT_FALSE(manager.CreateContext({}));
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->begins, 1);
  EXPECT_EQ(m_pool->ends, 0);
  EXPECT_EQ(m_pool->destroys, 1);
  m_pool->bindSucceeds = true;
  EXPECT_TRUE(manager.CreateContext({}));
  manager.DestroyContext();
  EXPECT_EQ(m_pool->creates, 2);
}

TEST_F(TestRPRenderManager, FailedClientBindDoesNotEndAnotherScope)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.BeginClientFrame());
  EXPECT_FALSE(
      std::async(std::launch::async, [&manager] { return manager.BeginClientFrame(); }).get());
  EXPECT_EQ(m_pool->begins, 1);
  EXPECT_EQ(m_pool->ends, 0);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 1);
  m_pool->bindSucceeds = false;
  EXPECT_FALSE(manager.BeginClientFrame());
  EXPECT_EQ(m_pool->ends, 1);
  manager.DestroyContext();
}

TEST_F(TestRPRenderManager, FramebufferGrowthPreservesBufferAndOtherAxis)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.Create(1024, 512));
  auto* buffer = m_pool->clientBuffer;
  const auto framebuffer = manager.GetCurrentFramebuffer(320, 240);
  EXPECT_EQ(manager.GetCurrentFramebuffer(640, 480), framebuffer);
  EXPECT_EQ(buffer->allocations, 1);
  EXPECT_EQ(manager.GetCurrentFramebuffer(640, 928), framebuffer);
  EXPECT_EQ(buffer->GetWidth(), 1024);
  EXPECT_EQ(buffer->GetHeight(), 928);
  buffer->failAllocation = true;
  EXPECT_EQ(manager.GetCurrentFramebuffer(4096, 2048), 0);
  EXPECT_EQ(manager.GetCurrentFramebuffer(1024, 928), framebuffer);
  buffer->failAllocation = false;
  EXPECT_EQ(manager.GetCurrentFramebuffer(1280, 720), framebuffer);
  EXPECT_EQ(buffer->GetHeight(), 928);
  manager.Flush();
  EXPECT_EQ(manager.GetCurrentFramebuffer(320, 240), framebuffer);
  manager.DestroyContext();
}

TEST_F(TestRPRenderManager, CapturedFrameOwnsOneReferenceAndCanRemainInUse)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.Create(1024, 768));
  manager.RenderFrame(320, 240, 0.0f, 0);
  auto* previous = m_pool->captured;
  ASSERT_NE(previous, nullptr);
  EXPECT_EQ(previous->References(), 1);
  previous->Acquire();
  manager.RenderFrame(640, 400, 0.0f, 0);
  EXPECT_NE(m_pool->captured, previous);
  EXPECT_EQ(previous->References(), 1);
  EXPECT_EQ(previous->GetWidth(), 320);
  EXPECT_EQ(previous->GetHeight(), 240);
  previous->Release();
  manager.EndClientFrame();
  manager.DestroyContext();
}

TEST_F(TestRPRenderManager, GUIHandoffPrecedesCaptureReleaseAndPoolFlush)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.Configure(AV_PIX_FMT_NONE, 320, 240, 0.0f, 320, 240));
  manager.FrameMove();
  RenderControl();
  ASSERT_NE(m_pool->renderer, nullptr);
  ASSERT_TRUE(manager.Create(320, 240));
  ASSERT_TRUE(manager.BeginClientFrame());
  manager.RenderFrame(320, 240, 0.0f, 0);
  manager.EndClientFrame();
  m_pool->renderer->SetBuffer(m_pool->captured);

  std::vector<std::string> events;
  m_pool->onFlushRendered = [&] { events.emplace_back("submit"); };
  m_pool->onReturn = [&] { events.emplace_back("return"); };
  m_pool->onFlush = [&] { events.emplace_back("flush pool"); };
  m_pool->onRendererFlush = [&] { events.emplace_back("flush renderer"); };
  manager.Flush();
  manager.FrameMove();
  EXPECT_EQ(events, (std::vector<std::string>{"submit", "return", "flush renderer", "flush pool"}));
  m_pool->onFlushRendered = {};
  m_pool->onReturn = {};
  m_pool->onFlush = {};
  m_pool->onRendererFlush = {};
}

TEST_F(TestRPRenderManager, InvalidAndFailedCapturesKeepPreviousPublication)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.Create(1024, 768));
  manager.RenderFrame(640, 400, 0.0f, 0);
  auto* previous = m_pool->captured;
  ASSERT_NE(previous, nullptr);
  manager.RenderFrame(0, 400, 0.0f, 0);
  manager.RenderFrame(1280, 720, 0.0f, 0);
  EXPECT_EQ(m_pool->captures, 1);
  m_pool->failCapture = true;
  manager.RenderFrame(1024, 768, 0.0f, 0);
  EXPECT_EQ(previous->References(), 1);
  EXPECT_FALSE(m_pool->clientBuffer->IsLoaded());
  manager.EndClientFrame();
  manager.DestroyContext();
}

TEST_F(TestRPRenderManager, HardwareStreamPreservesNominalAndCurrentDisplayAspectRatio)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  manager.Initialize();
  CRetroPlayerRendering rendering(manager, m_environment.ProcessInfo());
  CTestHardwareCallback callback;
  game_hw_rendering_properties context{};
  context.context_type = GAME_HW_CONTEXT_OPENGL;
  KODI::GAME::CGameClientStreamHwFramebuffer stream(callback, context);
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 320;
  properties.hw_framebuffer.max_height = 240;
  properties.hw_framebuffer.nominal_display_aspect_ratio = 16.0f / 9.0f;
  ASSERT_TRUE(stream.OpenStream(&rendering, properties));
  EXPECT_FLOAT_EQ(manager.GetNominalDisplayAspectRatio(), 16.0f / 9.0f);

  game_stream_buffer buffer{};
  buffer.type = GAME_STREAM_HW_FRAMEBUFFER;
  ASSERT_TRUE(stream.GetBuffer(320, 240, buffer));
  ASSERT_TRUE(manager.BeginClientFrame());
  game_stream_packet packet{};
  packet.type = GAME_STREAM_HW_FRAMEBUFFER;
  packet.hw_framebuffer.framebuffer = buffer.hw_framebuffer.framebuffer;
  packet.hw_framebuffer.width = 320;
  packet.hw_framebuffer.height = 240;
  packet.hw_framebuffer.display_aspect_ratio = 1.5f;
  stream.AddData(packet);
  ASSERT_NE(m_pool->captured, nullptr);
  EXPECT_EQ(m_pool->captured->GetWidth(), 320);
  EXPECT_EQ(m_pool->captured->GetHeight(), 240);
  EXPECT_FLOAT_EQ(m_pool->captured->GetDisplayAspectRatio(), 1.5f);

  packet.hw_framebuffer.display_aspect_ratio = 0.0f;
  stream.AddData(packet);
  ASSERT_NE(m_pool->captured, nullptr);
  EXPECT_FLOAT_EQ(m_pool->captured->GetDisplayAspectRatio(), 0.0f);
  EXPECT_FLOAT_EQ(manager.GetNominalDisplayAspectRatio(), 16.0f / 9.0f);
  manager.EndClientFrame();
  stream.CloseStream();
}

TEST_F(TestRPRenderManager, HardwarePacketsPreserveRotationAcrossFrameSizes)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  manager.Initialize();
  CRetroPlayerRendering rendering(manager, m_environment.ProcessInfo());
  CTestHardwareCallback callback;
  game_hw_rendering_properties context{};
  context.context_type = GAME_HW_CONTEXT_OPENGL;
  context.bottom_left_origin = true;
  KODI::GAME::CGameClientStreamHwFramebuffer stream(callback, context);
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = 640;
  properties.hw_framebuffer.max_height = 480;
  ASSERT_TRUE(stream.OpenStream(&rendering, properties));
  ASSERT_TRUE(manager.BeginClientFrame());

  const std::pair<GAME_VIDEO_ROTATION, unsigned int> rotations[] = {
      {GAME_VIDEO_ROTATION_0, 0},
      {GAME_VIDEO_ROTATION_90_CCW, 90},
      {GAME_VIDEO_ROTATION_180_CCW, 180},
      {GAME_VIDEO_ROTATION_270_CCW, 270}};
  for (const auto& [rotation, degrees] : rotations)
  {
    for (const auto& [width, height] : {std::pair{320u, 240u}, {640u, 400u}, {320u, 240u}})
    {
      SCOPED_TRACE(testing::Message()
                   << "rotation=" << degrees << " size=" << width << "x" << height);
      game_stream_buffer buffer{};
      buffer.type = GAME_STREAM_HW_FRAMEBUFFER;
      ASSERT_TRUE(stream.GetBuffer(width, height, buffer));
      game_stream_packet packet{};
      packet.type = GAME_STREAM_HW_FRAMEBUFFER;
      packet.hw_framebuffer.framebuffer = buffer.hw_framebuffer.framebuffer;
      packet.hw_framebuffer.width = width;
      packet.hw_framebuffer.height = height;
      packet.hw_framebuffer.display_aspect_ratio = 1.5f;
      packet.hw_framebuffer.rotation = rotation;
      stream.AddData(packet);
      ASSERT_NE(m_pool->captured, nullptr);
      EXPECT_EQ(m_pool->captured->GetRotation(), degrees);
      EXPECT_EQ(m_pool->captured->GetWidth(), width);
      EXPECT_EQ(m_pool->captured->GetHeight(), height);
      EXPECT_FLOAT_EQ(m_pool->captured->GetDisplayAspectRatio(), 1.5f);
      EXPECT_EQ(m_pool->clientBuffer->allocations, 1);
    }
  }
  manager.EndClientFrame();
  stream.CloseStream();
}

TEST_F(TestRPRenderManager, StreamClosureKeepsContextUntilClientCallReturns)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.BeginClientFrame());
  ASSERT_TRUE(manager.BeginClientFrame());
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 0);
  EXPECT_EQ(m_pool->ends, 0);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->destroys, 0);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->destroys, 1);
  EXPECT_EQ(m_pool->ends, 1);
  EXPECT_TRUE(manager.CreateContext({}));
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 2);
}

TEST_F(TestRPRenderManager, ReopenReplacesPendingContextInsideClientCall)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  ASSERT_TRUE(manager.BeginClientFrame());
  EXPECT_TRUE(manager.CreateContext({}));
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 0U);

  EXPECT_TRUE(manager.CreateContext({}));
  EXPECT_EQ(m_pool->creates, 2U);
  EXPECT_EQ(m_pool->destroys, 1U);
  EXPECT_EQ(m_pool->begins, 2U);
  EXPECT_EQ(m_pool->ends, 1U);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 1U);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 2U);
  EXPECT_EQ(m_pool->destroys, 1U);
  EXPECT_FALSE(manager.CreateContext({}));
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 2U);
}

TEST_F(TestRPRenderManager, FailedReplacementCreationReleasesPendingContext)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  EXPECT_TRUE(manager.CreateContext({}));
  manager.DestroyContext();
  m_pool->createSucceeds = false;
  EXPECT_FALSE(manager.CreateContext({}));
  EXPECT_EQ(m_pool->creates, 2U);
  EXPECT_EQ(m_pool->destroys, 2U);
  EXPECT_EQ(m_pool->begins, 1U);
  EXPECT_EQ(m_pool->ends, 1U);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 1U);
  EXPECT_EQ(m_pool->destroys, 2U);

  m_pool->createSucceeds = true;
  EXPECT_TRUE(manager.CreateContext({}));
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 3U);
}

TEST_F(TestRPRenderManager, FailedReplacementBindAllowsAnotherOpenInsideClientCall)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  EXPECT_TRUE(manager.CreateContext({}));
  manager.DestroyContext();
  m_pool->bindSucceeds = false;
  EXPECT_FALSE(manager.CreateContext({}));
  EXPECT_EQ(m_pool->destroys, 2U);
  EXPECT_EQ(m_pool->begins, 2U);
  EXPECT_EQ(m_pool->ends, 1U);

  m_pool->bindSucceeds = true;
  EXPECT_TRUE(manager.CreateContext({}));
  EXPECT_EQ(m_pool->begins, 3U);
  manager.EndClientFrame();
  EXPECT_EQ(m_pool->ends, 2U);
  EXPECT_EQ(m_pool->destroys, 2U);
  manager.DestroyContext();
  EXPECT_EQ(m_pool->destroys, 3U);
}

TEST_F(TestRPRenderManager, AllocationExceptionBalancesNestedScope)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.Create(320, 240));
  ASSERT_TRUE(manager.BeginClientFrame());
  m_pool->clientBuffer->throwOnAllocation = true;
  EXPECT_THROW(manager.Create(640, 480), std::runtime_error);
  manager.EndClientFrame();
  EXPECT_TRUE(std::async(std::launch::async,
                         [&manager]
                         {
                           if (!manager.BeginClientFrame())
                             return false;
                           manager.EndClientFrame();
                           return true;
                         })
                  .get());
  manager.DestroyContext();
}

TEST_F(TestRPRenderManager, HardwareProcessDimensionsFollowFramesWithoutResizingClient)
{
  CDataCacheCore cache;
  auto& processInfo = m_environment.ProcessInfo();
  processInfo.SetDataCache(&cache);
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  manager.Initialize();
  CRetroPlayerRendering rendering(manager, processInfo);
  const HwFramebufferProperties properties{
      GAME_HW_CONTEXT_OPENGL_CORE, false, false, true, 3, 3, false, false, 1920, 1080, 0.0f};
  ASSERT_TRUE(rendering.OpenStream(properties));
  HwFramebufferBuffer buffer;
  ASSERT_TRUE(rendering.GetStreamBuffer(640, 480, buffer));
  const auto framebuffer = buffer.framebuffer;
  ASSERT_NE(framebuffer, 0);
  ASSERT_TRUE(manager.BeginClientFrame());

  for (const auto& [width, height] : {std::pair{640u, 480u}, {1280u, 720u}, {640u, 480u}})
  {
    ASSERT_TRUE(rendering.GetStreamBuffer(width, height, buffer));
    EXPECT_EQ(buffer.framebuffer, framebuffer);
    rendering.AddStreamData(
        HwFramebufferPacket{framebuffer, width, height, 0.0f, VideoRotation::ROTATION_0});
    EXPECT_EQ(cache.GetVideoWidth(), width);
    EXPECT_EQ(cache.GetVideoHeight(), height);
    EXPECT_EQ(m_pool->clientBuffer->GetWidth(), 1920);
    EXPECT_EQ(m_pool->clientBuffer->GetHeight(), 1080);
    EXPECT_EQ(m_pool->clientBuffer->allocations, 1);
  }

  for (const auto& packet :
       {HwFramebufferPacket{framebuffer, 0, 480, 0.0f, VideoRotation::ROTATION_0},
        HwFramebufferPacket{framebuffer, 640, 0, 0.0f, VideoRotation::ROTATION_0},
        HwFramebufferPacket{framebuffer, 1921, 1080, 0.0f, VideoRotation::ROTATION_0},
        HwFramebufferPacket{framebuffer, 1920, 1081, 0.0f, VideoRotation::ROTATION_0},
        HwFramebufferPacket{framebuffer + 1, 1280, 720, 0.0f, VideoRotation::ROTATION_0},
        HwFramebufferPacket{0, 1280, 720, 0.0f, VideoRotation::ROTATION_0}})
    rendering.AddStreamData(packet);
  EXPECT_EQ(cache.GetVideoWidth(), 640);
  EXPECT_EQ(cache.GetVideoHeight(), 480);
  EXPECT_EQ(m_pool->captures, 3);
  manager.EndClientFrame();
  rendering.CloseStream();
  processInfo.SetDataCache(nullptr);
}

TEST_F(TestRPRenderManager, HardwareStreamPropagatesDebugContextRequest)
{
  m_pool->hardware = true;
  CRetroPlayerRendering rendering(m_environment.Renderer(), m_environment.ProcessInfo());
  HwFramebufferProperties properties{
      GAME_HW_CONTEXT_OPENGL_CORE, false, false, true, 3, 3, false, false, 320, 240, 0.0f};
  ASSERT_TRUE(rendering.OpenStream(properties));
  EXPECT_FALSE(m_pool->contextProperties.debugContext);
  rendering.CloseStream();

  properties.debugContext = true;
  ASSERT_TRUE(rendering.OpenStream(properties));
  EXPECT_TRUE(m_pool->contextProperties.debugContext);
}

TEST_F(TestRPRenderManager, RefusedDebugContextDoesNotOpenHardwareStream)
{
  m_pool->hardware = true;
  m_pool->refuseDebugContext = true;
  CRetroPlayerRendering rendering(m_environment.Renderer(), m_environment.ProcessInfo());
  const HwFramebufferProperties properties{
      GAME_HW_CONTEXT_OPENGL_CORE, false, false, true, 3, 3, false, true, 320, 240, 0.0f};
  EXPECT_FALSE(rendering.OpenStream(properties));
  EXPECT_TRUE(m_pool->contextProperties.debugContext);
  EXPECT_EQ(m_pool->destroys, 1U);
  HwFramebufferBuffer buffer;
  EXPECT_FALSE(rendering.GetStreamBuffer(320, 240, buffer));
  EXPECT_EQ(m_environment.Renderer().GetCurrentFramebuffer(320, 240), 0U);

  m_pool->refuseDebugContext = false;
  EXPECT_TRUE(rendering.OpenStream(properties));
}

class TestRPStreamHandoff : public testing::Test
{
protected:
  TestRPStreamHandoff()
  {
    m_hwPool->hardware = true;
    m_hwPool->restrictFormat = true;
    m_swPool->restrictFormat = true;
  }

  StreamPtr OpenHardware()
  {
    auto stream = m_session->CreateStream(StreamType::HW_BUFFER);
    const HwFramebufferProperties properties{
        GAME_HW_CONTEXT_OPENGL_CORE, false, false, true, 3, 3, false, false, 320, 240, 0.0f};
    EXPECT_TRUE(stream->OpenStream(properties));
    m_environment.Renderer().FrameMove();
    m_environment.Renderer().RenderControl(false, false, CRect{}, nullptr);
    return stream;
  }

  StreamPtr OpenSoftware(AVPixelFormat format = AV_PIX_FMT_BGR0)
  {
    auto stream = m_session->CreateStream(StreamType::VIDEO);
    EXPECT_TRUE(stream->OpenStream(VideoStreamProperties{format, 160, 144, 0.0f, 160, 144}));
    m_environment.Renderer().FrameMove();
    m_environment.Renderer().RenderControl(false, false, CRect{}, nullptr);
    return stream;
  }

  CTestBuffer* PublishSoftware(IRetroPlayerStream& stream, VideoRotation rotation)
  {
    VideoStreamBuffer buffer;
    if (!stream.GetStreamBuffer(160, 144, buffer))
    {
      ADD_FAILURE() << "The configured software stream must accept frames";
      return nullptr;
    }
    auto* published = m_swPool->lastBuffer;
    std::fill_n(buffer.data, buffer.size, 0x5a);
    stream.AddStreamData(VideoStreamPacket{160, 144, 0.0f, rotation, buffer.data, buffer.size});
    return published;
  }

  void PublishHardware(IRetroPlayerStream& stream, VideoRotation rotation)
  {
    HwFramebufferBuffer buffer;
    ASSERT_TRUE(stream.GetStreamBuffer(320, 240, buffer));
    auto& manager = m_environment.Renderer();
    ASSERT_TRUE(manager.BeginClientFrame());
    stream.AddStreamData(HwFramebufferPacket{buffer.framebuffer, 320, 240, 0.0f, rotation});
    manager.EndClientFrame();
  }

  void ExpectConfigured(bool configured)
  {
    bool reachedPool = false;
    m_hwPool->onCompatibilityCheck = [&reachedPool] { reachedPool = true; };
    m_hwPool->compatible = false;
    m_swPool->compatible = false;
    m_environment.Renderer().RenderControl(false, false, CRect{}, nullptr);
    EXPECT_EQ(reachedPool, configured);
    m_hwPool->onCompatibilityCheck = {};
    m_hwPool->compatible = true;
    m_swPool->compatible = true;
  }

  std::shared_ptr<CTestPool> m_hwPool = std::make_shared<CTestPool>();
  std::shared_ptr<CTestPool> m_swPool = std::make_shared<CTestPool>();
  CScopedRendererFactories::CRegistration m_factories{{m_hwPool, m_swPool}};
  CPlaybackTestEnvironment m_environment;
  std::unique_ptr<CRPStreamManager> m_session =
      std::make_unique<CRPStreamManager>(m_environment.Renderer(), m_environment.ProcessInfo());
};

TEST_F(TestRPStreamHandoff, HardwareClosePreservesReplacementSoftwareFrame)
{
  auto hardware = OpenHardware();
  PublishHardware(*hardware, VideoRotation::ROTATION_0);
  auto software = OpenSoftware();
  auto* published = PublishSoftware(*software, VideoRotation::ROTATION_90_CCW);
  ASSERT_NE(published, nullptr);
  ASSERT_EQ(published->GetRotation(), 90U);
  ASSERT_EQ(published->References(), 1);

  m_session->CloseStream(std::move(hardware));
  EXPECT_EQ(m_hwPool->destroys, 1U);
  EXPECT_EQ(published->References(), 1);
  ExpectConfigured(true);
  auto* next = PublishSoftware(*software, VideoRotation::ROTATION_180_CCW);
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(next->GetRotation(), 180U);
  EXPECT_EQ(next->References(), 1);
  m_session->CloseStream(std::move(software));
}

TEST_F(TestRPStreamHandoff, SoftwareClosePreservesReplacementHardwareFrame)
{
  auto software = OpenSoftware();
  ASSERT_NE(PublishSoftware(*software, VideoRotation::ROTATION_0), nullptr);
  auto hardware = OpenHardware();
  PublishHardware(*hardware, VideoRotation::ROTATION_90_CCW);
  ASSERT_NE(m_hwPool->captured, nullptr);
  ASSERT_EQ(m_hwPool->captured->References(), 1);

  m_session->CloseStream(std::move(software));
  EXPECT_EQ(m_hwPool->captured->References(), 1);
  ExpectConfigured(true);
  PublishHardware(*hardware, VideoRotation::ROTATION_180_CCW);
  EXPECT_EQ(m_hwPool->captures, 2U);
  EXPECT_EQ(m_hwPool->captured->GetRotation(), 180U);
  EXPECT_EQ(m_hwPool->captured->References(), 1);
  m_session->CloseStream(std::move(hardware));
}

TEST_F(TestRPStreamHandoff, RepeatedHardwareSoftwareHardwareHandoffs)
{
  auto hardware = OpenHardware();
  for (unsigned int transition = 0; transition < 3; ++transition)
  {
    SCOPED_TRACE(transition);
    PublishHardware(*hardware, VideoRotation::ROTATION_90_CCW);
    auto software = OpenSoftware();
    auto* published = PublishSoftware(*software, VideoRotation::ROTATION_180_CCW);
    ASSERT_NE(published, nullptr);
    m_session->CloseStream(std::move(hardware));
    EXPECT_EQ(published->References(), 1);
    ExpectConfigured(true);
    ASSERT_NE(PublishSoftware(*software, VideoRotation::ROTATION_270_CCW), nullptr);
    hardware = OpenHardware();
    m_session->CloseStream(std::move(software));
    ExpectConfigured(true);
    PublishHardware(*hardware, VideoRotation::ROTATION_270_CCW);
    EXPECT_EQ(m_hwPool->captured->GetRotation(), 270U);
    EXPECT_EQ(m_hwPool->captured->References(), 1);
  }
  EXPECT_EQ(m_hwPool->captures, 6U);
  m_session->CloseStream(std::move(hardware));
  m_session.reset();
  EXPECT_EQ(m_hwPool->destroys, 4U);
  ExpectConfigured(false);
}

TEST_F(TestRPStreamHandoff, FinalSoftwareStreamResourcesAreReleasedAtSessionTeardown)
{
  auto software = OpenSoftware();
  auto* published = PublishSoftware(*software, VideoRotation::ROTATION_90_CCW);
  ASSERT_NE(published, nullptr);
  VideoStreamBuffer pending;
  ASSERT_TRUE(software->GetStreamBuffer(160, 144, pending));
  auto* pendingBuffer = m_swPool->lastBuffer;
  ASSERT_EQ(published->References(), 1);
  ASSERT_EQ(pendingBuffer->References(), 1);
  m_environment.Renderer().CacheVideoFrame("handoff.sav");
  ASSERT_EQ(published->References(), 2);
  m_session->CloseStream(std::move(software));
  EXPECT_EQ(published->References(), 2);
  EXPECT_EQ(pendingBuffer->References(), 1);
  ExpectConfigured(true);
  m_session.reset();
  EXPECT_EQ(published->References(), 0);
  EXPECT_EQ(pendingBuffer->References(), 0);
  ExpectConfigured(false);
  EXPECT_FALSE(m_swPool->HasVisibleRenderer());
  EXPECT_EQ(m_environment.Renderer().GetCurrentFramebuffer(320, 240), 0U);

  m_session =
      std::make_unique<CRPStreamManager>(m_environment.Renderer(), m_environment.ProcessInfo());
  software = OpenSoftware();
  published = PublishSoftware(*software, VideoRotation::ROTATION_270_CCW);
  ASSERT_NE(published, nullptr);
  EXPECT_EQ(published->GetRotation(), 270U);
  EXPECT_EQ(published->References(), 1);
  m_session->CloseStream(std::move(software));
}

TEST_F(TestRPStreamHandoff, FinalHardwareStreamResourcesAreReleasedAtSessionTeardown)
{
  auto hardware = OpenHardware();
  PublishHardware(*hardware, VideoRotation::ROTATION_90_CCW);
  ASSERT_NE(m_hwPool->captured, nullptr);
  EXPECT_EQ(m_hwPool->clientBuffer->References(), 1);
  EXPECT_EQ(m_hwPool->captured->References(), 1);
  m_session->CloseStream(std::move(hardware));
  EXPECT_EQ(m_hwPool->clientBuffer->References(), 0);
  EXPECT_EQ(m_hwPool->captured->References(), 0);
  ExpectConfigured(true);
  m_session.reset();
  EXPECT_EQ(m_hwPool->destroys, 1U);
  EXPECT_EQ(m_environment.Renderer().GetCurrentFramebuffer(320, 240), 0U);
  ExpectConfigured(false);
  EXPECT_FALSE(m_hwPool->HasVisibleRenderer());
}

TEST_F(TestRPStreamHandoff, SoftwareReplacementUsesNewPixelFormat)
{
  auto previous = OpenSoftware();
  VideoStreamBuffer buffer;
  ASSERT_TRUE(previous->GetStreamBuffer(160, 144, buffer));
  ASSERT_EQ(buffer.pixfmt, AV_PIX_FMT_BGR0);
  auto* previousBuffer = m_swPool->lastBuffer;

  auto replacement = OpenSoftware(AV_PIX_FMT_RGB565);
  EXPECT_EQ(m_swPool->rendererFormat, AV_PIX_FMT_RGB565);
  EXPECT_EQ(previousBuffer->References(), 1);
  m_session->CloseStream(std::move(previous));
  EXPECT_EQ(previousBuffer->References(), 1);
  ASSERT_TRUE(replacement->GetStreamBuffer(160, 144, buffer));
  EXPECT_EQ(buffer.pixfmt, AV_PIX_FMT_RGB565);
  EXPECT_EQ(buffer.size, 160U * 144U * 2U);
  auto* published = PublishSoftware(*replacement, VideoRotation::ROTATION_90_CCW);
  ASSERT_NE(published, nullptr);
  EXPECT_EQ(published->GetRotation(), 90U);
  EXPECT_EQ(published->GetFormat(), AV_PIX_FMT_RGB565);
  EXPECT_EQ(published->References(), 1);
  m_session->CloseStream(std::move(replacement));
}
