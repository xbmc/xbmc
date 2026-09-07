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
#include <memory>
#include <mutex>
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

class CTestPool : public CBaseRenderBufferPool
{
public:
  bool IsCompatible(const CRenderVideoSettings&) const override
  {
    if (auto callback = std::exchange(onCompatibilityCheck, {}))
      callback();
    return true;
  }

  mutable std::function<void()> onCompatibilityCheck;

protected:
  IRenderBuffer* CreateRenderBuffer(void*) override { return new CTestBuffer; }
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
