/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h"
#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"

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
  int References() const { return m_refCount; }
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
    return true;
  }

  bool SupportsHardwareRendering() const override { return hardware; }
  bool CreateContext(const HwContextProperties&) override
  {
    ++creates;
    return true;
  }
  bool BeginClientFrame() override
  {
    ++begins;
    return bindSucceeds;
  }
  void EndClientFrame() override { ++ends; }
  void DestroyContext() override { ++destroys; }

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
  bool bindSucceeds{true};
  unsigned int creates{0};
  unsigned int begins{0};
  unsigned int ends{0};
  unsigned int destroys{0};
  mutable std::function<void()> onCompatibilityCheck;

protected:
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
  void RenderInternal(bool, uint8_t) override {}
};

class CTestRendererFactory : public IRendererFactory
{
public:
  explicit CTestRendererFactory(std::shared_ptr<CTestPool> pool) : m_pool(std::move(pool)) {}
  std::string RenderSystemName() const override { return "test"; }
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> pool) override
  {
    return new CTestRenderer(settings, context, std::move(pool));
  }
  RenderBufferPoolVector CreateBufferPools(CRenderContext&) override { return {m_pool}; }

private:
  std::shared_ptr<CTestPool> m_pool;
};

class CScopedRendererFactories : public CRPProcessInfo
{
public:
  class CRegistration
  {
  public:
    explicit CRegistration(std::shared_ptr<CTestPool> pool)
    {
      std::unique_lock lock(m_createSection);
      m_previous = std::move(m_rendererFactories);
      m_rendererFactories.clear();
      m_rendererFactories.emplace_back(std::make_unique<CTestRendererFactory>(std::move(pool)));
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
  CScopedRendererFactories::CRegistration m_factories{m_pool};
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
  manager.RenderFrame(320, 240);
  auto* previous = m_pool->captured;
  ASSERT_NE(previous, nullptr);
  EXPECT_EQ(previous->References(), 1);
  previous->Acquire();
  manager.RenderFrame(640, 400);
  EXPECT_NE(m_pool->captured, previous);
  EXPECT_EQ(previous->References(), 1);
  EXPECT_EQ(previous->GetWidth(), 320);
  EXPECT_EQ(previous->GetHeight(), 240);
  previous->Release();
  manager.EndClientFrame();
  manager.DestroyContext();
}

TEST_F(TestRPRenderManager, InvalidAndFailedCapturesKeepPreviousPublication)
{
  auto& manager = m_environment.Renderer();
  m_pool->hardware = true;
  ASSERT_TRUE(manager.BeginClientFrame());
  ASSERT_TRUE(manager.CreateContext({}));
  ASSERT_TRUE(manager.Create(1024, 768));
  manager.RenderFrame(640, 400);
  auto* previous = m_pool->captured;
  ASSERT_NE(previous, nullptr);
  manager.RenderFrame(0, 400);
  manager.RenderFrame(1280, 720);
  EXPECT_EQ(m_pool->captures, 1);
  m_pool->failCapture = true;
  manager.RenderFrame(1024, 768);
  EXPECT_EQ(previous->References(), 1);
  EXPECT_FALSE(m_pool->clientBuffer->IsLoaded());
  manager.EndClientFrame();
  manager.DestroyContext();
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
