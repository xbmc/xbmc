/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"

#if (defined(HAS_EGL) || defined(TARGET_DARWIN_OSX)) && (defined(HAS_GL) || HAS_GLES == 3)

#include "ServiceBroker.h"
#include "cores/RetroPlayer/buffers/RenderBufferManager.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolFBO.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/contexts/IHwRenderingContext.h"
#include "messaging/ApplicationMessenger.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
class CTestCaptureBuffer : public CRenderBufferSysMem
{
public:
  bool Allocate(AVPixelFormat, unsigned int width, unsigned int height) override
  {
    SetSize(width, height);
    return true;
  }
  bool UploadTexture() override { return true; }
};

class CTestCapturePool : public CRenderBufferPoolFBO
{
public:
  using CRenderBufferPoolFBO::CRenderBufferPoolFBO;
  using CRenderBufferPoolFBO::GetCaptureBuffer;

  void Return(IRenderBuffer* buffer) override { CBaseRenderBufferPool::Return(buffer); }

protected:
  IRenderBuffer* CreateRenderBuffer(void*) override { return new CTestCaptureBuffer; }
};
} // namespace

TEST(TestRenderBufferPoolFBO, SameSizeCaptureResumesAfterFlush)
{
  CPlaybackTestEnvironment environment;
  auto pool = std::make_shared<CTestCapturePool>(environment.ProcessInfo().GetRenderContext());
  auto* first = pool->GetCaptureBuffer(320, 240);
  ASSERT_NE(first, nullptr);
  first->Release();

  pool->Flush();
  ASSERT_FALSE(pool->IsConfigured());

  auto* second = pool->GetCaptureBuffer(320, 240);
  ASSERT_NE(second, nullptr);
  EXPECT_TRUE(pool->IsConfigured());
  EXPECT_EQ(second->GetWidth(), 320);
  EXPECT_EQ(second->GetHeight(), 240);
  second->Release();
}

TEST(TestRenderBufferPoolFBO, CaptureDimensionsCanChangeAfterFlush)
{
  CPlaybackTestEnvironment environment;
  auto pool = std::make_shared<CTestCapturePool>(environment.ProcessInfo().GetRenderContext());
  auto* first = pool->GetCaptureBuffer(320, 240);
  ASSERT_NE(first, nullptr);
  first->Release();
  pool->Flush();

  auto* second = pool->GetCaptureBuffer(640, 480);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->GetWidth(), 640);
  EXPECT_EQ(second->GetHeight(), 480);
  second->Release();
}

namespace
{
class CFailingContext : public IHwRenderingContext
{
public:
  bool SupportsHardwareRendering() const override { return available; }
  bool Create(const HwContextProperties&) override
  {
    created = true;
    return createSucceeds;
  }
  bool IsCreated() const override { return created; }
  bool MakeCurrent() override
  {
    ++bindAttempts;
    return false;
  }
  void RestoreCurrent() override {}
  void Destroy() override { created = false; }
  bool available{false};
  bool created{false};
  bool createSucceeds{false};
  unsigned int bindAttempts{0};
};
} // namespace

TEST(TestRenderBufferPoolFBO, MissingNativeContextDoesNotAdvertiseHardware)
{
  CPlaybackTestEnvironment environment;
  auto pool =
      std::make_shared<CRenderBufferPoolFBO>(environment.ProcessInfo().GetRenderContext(), nullptr);
  environment.ProcessInfo().GetBufferManager().RegisterPools(nullptr, {pool});
  EXPECT_FALSE(environment.ProcessInfo().HasHardwareRendering());
  EXPECT_FALSE(pool->CreateContext({}));
  EXPECT_FALSE(pool->BeginClientFrame());
}

TEST(TestRenderBufferPoolFBO, NativeCapabilityPropagatesThroughProcessInfo)
{
  CPlaybackTestEnvironment environment;
  auto context = std::make_unique<CFailingContext>();
  context->available = true;
  auto pool = std::make_shared<CRenderBufferPoolFBO>(environment.ProcessInfo().GetRenderContext(),
                                                     std::move(context));
  environment.ProcessInfo().GetBufferManager().RegisterPools(nullptr, {pool});
  EXPECT_TRUE(environment.ProcessInfo().HasHardwareRendering());
}

TEST(TestRenderBufferPoolFBO, FailedCreationReleasesPartialNativeContext)
{
  CPlaybackTestEnvironment environment;
  auto context = std::make_unique<CFailingContext>();
  context->available = true;
  auto* native = context.get();
  auto pool = std::make_shared<CRenderBufferPoolFBO>(environment.ProcessInfo().GetRenderContext(),
                                                     std::move(context));
  EXPECT_FALSE(pool->CreateContext({}));
  EXPECT_FALSE(pool->BeginClientFrame());
  EXPECT_FALSE(native->created);
  native->createSucceeds = true;
  EXPECT_TRUE(pool->CreateContext({}));
}

TEST(TestRenderBufferPoolFBO, FailedBindingReleasesContextLock)
{
  CPlaybackTestEnvironment environment;
  auto context = std::make_unique<CFailingContext>();
  context->available = true;
  context->createSucceeds = true;
  auto* native = context.get();
  auto pool = std::make_shared<CRenderBufferPoolFBO>(environment.ProcessInfo().GetRenderContext(),
                                                     std::move(context));
  ASSERT_TRUE(pool->CreateContext({}));
  EXPECT_FALSE(pool->BeginClientFrame());
  EXPECT_EQ(native->bindAttempts, 1);
  EXPECT_FALSE(std::async(std::launch::async, [&] { return pool->BeginClientFrame(); }).get());
  EXPECT_EQ(native->bindAttempts, 2);
  pool->DestroyContext();
  EXPECT_FALSE(pool->BeginClientFrame());
}

#if defined(HAS_EGL)
#include "cores/RetroPlayer/rendering/contexts/EGLClientContext.h"
#include "cores/RetroPlayer/rendering/contexts/HwRenderingContextEGLUtils.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace
{
class CTestEGLContext : public IHwRenderingContext
{
public:
  CTestEGLContext(EGLenum api,
                  EGLDisplay display,
                  EGLConfig config,
                  EGLContext shared,
                  const EGLint* attributes,
                  bool surfaceless)
    : m_client(api)
  {
    m_client.Create(display, config, shared, attributes, surfaceless);
  }
  bool SupportsHardwareRendering() const override { return IsCreated(); }
  bool Create(const HwContextProperties&) override { return IsCreated(); }
  bool IsCreated() const override { return m_client.IsCreated(); }
  bool MakeCurrent() override { return m_client.MakeCurrent(); }
  void RestoreCurrent() override { m_client.RestoreCurrent(); }
  void Destroy() override { m_client.Destroy(); }

private:
  CEGLClientContext m_client;
};
} // namespace

class TestRenderBufferPoolFBOWithContext : public testing::TestWithParam<bool>
{
protected:
  void SetUp() override
  {
    if (eglGetCurrentContext() != EGL_NO_CONTEXT)
      GTEST_SKIP() << "An EGL context is already current";

    m_previousAPI = eglQueryAPI();
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
      GTEST_SKIP() << "No EGL display is available";
    m_initialized = true;

#if defined(HAS_GLES)
    const EGLenum api = EGL_OPENGL_ES_API;
    const EGLint renderable = EGL_OPENGL_ES3_BIT_KHR;
    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
#else
    const EGLenum api = EGL_OPENGL_API;
    const EGLint renderable = EGL_OPENGL_BIT;
    const EGLint contextAttributes[] = {EGL_CONTEXT_MAJOR_VERSION_KHR,
                                        3,
                                        EGL_CONTEXT_MINOR_VERSION_KHR,
                                        2,
                                        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
                                        EGL_NONE};
#endif
    if (!eglBindAPI(api))
      GTEST_SKIP() << "The required EGL client API is unavailable";
    const bool surfaceless = GetParam();
    if (surfaceless && !GetEGLCapabilities(eglQueryString(m_display, EGL_VERSION),
                                           eglQueryString(m_display, EGL_EXTENSIONS))
                            .surfaceless)
      GTEST_SKIP() << "Surfaceless EGL contexts are unavailable";
    const EGLint configAttributes[] = {EGL_SURFACE_TYPE, surfaceless ? 0 : EGL_PBUFFER_BIT,
                                       EGL_RENDERABLE_TYPE, renderable, EGL_NONE};
    EGLConfig config{};
    EGLint count = 0;
    if (!eglChooseConfig(m_display, configAttributes, &config, 1, &count) || count == 0)
      GTEST_SKIP() << "No compatible EGL configuration is available";
    m_sharedContext = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
    if (m_sharedContext == EGL_NO_CONTEXT)
      GTEST_SKIP() << "The required GL context is unavailable";
    if (!surfaceless)
    {
      const EGLint surfaceAttributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
      m_guiSurface = eglCreatePbufferSurface(m_display, config, surfaceAttributes);
      if (m_guiSurface == EGL_NO_SURFACE)
        GTEST_SKIP() << "The GUI pbuffer is unavailable";
    }
    ASSERT_TRUE(eglMakeCurrent(m_display, m_guiSurface, m_guiSurface, m_sharedContext));
    auto messenger = std::make_shared<KODI::MESSAGING::CApplicationMessenger>();
    messenger->SetProcessThread(std::this_thread::get_id());
    CServiceBroker::RegisterAppMessenger(messenger);

    m_sync = std::make_shared<CRenderBufferFBO::Sync>();
    m_sync->fence = [this]
    {
      GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      m_fences.emplace_back(fence);
      return fence;
    };
    m_sync->wait = [this](GLsync fence)
    {
      m_waited.emplace_back(fence);
      glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
    };
    m_sync->destroy = [](GLsync fence) { glDeleteSync(fence); };
    m_sync->flush = [this]
    {
      m_flushContext.store(eglGetCurrentContext());
      ++m_flushes;
      glFlush();
    };

    m_pool = std::make_shared<CRenderBufferPoolFBO>(
        m_environment.ProcessInfo().GetRenderContext(),
        std::make_unique<CTestEGLContext>(api, m_display, config, m_sharedContext,
                                          contextAttributes, surfaceless),
        m_sync);
    m_current = m_pool->BeginClientFrame();
    ASSERT_TRUE(m_current) << "The shared client context must bind using the selected surface mode";
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
  }

  void TearDown() override
  {
    if (m_current)
      m_pool->EndClientFrame();
    m_pool.reset();
    CServiceBroker::RegisterAppMessenger(m_previousMessenger);
    if (m_initialized)
      eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_guiSurface != EGL_NO_SURFACE)
      eglDestroySurface(m_display, m_guiSurface);
    if (m_sharedContext != EGL_NO_CONTEXT)
      eglDestroyContext(m_display, m_sharedContext);
    if (m_initialized)
      eglTerminate(m_display);
    if (m_previousAPI != EGL_NONE)
      eglBindAPI(m_previousAPI);
  }

  CRenderBufferFBO* Capture(IRenderBuffer* client)
  {
    return static_cast<CRenderBufferFBO*>(m_pool->CaptureClientFrame(client, 4, 4));
  }

  void Sample(CRenderBufferFBO* buffer)
  {
    auto lock = buffer->Lock();
    buffer->WaitForCapture();
    buffer->MarkRendered();
  }

  bool BindGUI()
  {
    m_pool->EndClientFrame();
    m_current = false;
    return eglGetCurrentContext() == m_sharedContext;
  }

  bool BindClient()
  {
    m_current = m_pool->BeginClientFrame();
    return m_current;
  }

  template<typename ClientAction, typename GUIAction>
  void RunAfterClientCallbackQueued(ClientAction clientAction, GUIAction guiAction)
  {
    auto& graphics = m_environment.ProcessInfo().GetRenderContext().GraphicsMutex();
    std::promise<void> entered;
    auto enteredFuture = entered.get_future();
    std::atomic<bool> completed{false};
    std::thread clientThread(
        [&, pool = m_pool]
        {
          std::unique_lock lock(graphics);
          entered.set_value();
          clientAction(*pool);
          completed = true;
        });
    const auto started = enteredFuture.wait_for(std::chrono::seconds(5));
    if (started == std::future_status::ready)
    {
      std::unique_lock lock(graphics);
      EXPECT_FALSE(completed);
      guiAction();
    }
    else
    {
      m_pool->FlushRendered();
      CServiceBroker::GetAppMessenger()->ProcessMessages();
    }
    clientThread.join();
    EXPECT_EQ(started, std::future_status::ready);
    EXPECT_TRUE(completed);
  }

  std::shared_ptr<KODI::MESSAGING::CApplicationMessenger> m_previousMessenger{
      CServiceBroker::GetAppMessenger()};
  CPlaybackTestEnvironment m_environment;
  std::shared_ptr<CRenderBufferPoolFBO> m_pool;
  EGLDisplay m_display{EGL_NO_DISPLAY};
  EGLContext m_sharedContext{EGL_NO_CONTEXT};
  EGLSurface m_guiSurface{EGL_NO_SURFACE};
  EGLenum m_previousAPI{EGL_NONE};
  std::shared_ptr<CRenderBufferFBO::Sync> m_sync;
  std::vector<GLsync> m_fences;
  std::vector<GLsync> m_waited;
  std::atomic<unsigned int> m_flushes{0};
  std::atomic<EGLContext> m_flushContext{EGL_NO_CONTEXT};
  bool m_initialized{false};
  bool m_current{false};
};

TEST_P(TestRenderBufferPoolFBOWithContext, CaptureDiscardsAlphaBeforeShaderCopy)
{
  auto* client = static_cast<CRenderBufferFBO*>(m_pool->GetBuffer(4, 4));
  ASSERT_NE(client, nullptr);
  const auto clientFramebuffer = client->GetCurrentFramebuffer();
  glBindFramebuffer(GL_FRAMEBUFFER, clientFramebuffer);
  glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  auto* captured = static_cast<CRenderBufferFBO*>(m_pool->CaptureClientFrame(client, 4, 4));
  EXPECT_NE(captured, nullptr);
  if (captured)
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, captured->GetCurrentFramebuffer());
    unsigned char pixel[4]{};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_EQ(pixel[0], 255);
    EXPECT_EQ(pixel[1], 0);
    EXPECT_EQ(pixel[2], 0);
    EXPECT_EQ(pixel[3], 255);

    auto* shaderCopy = static_cast<CRenderBufferFBO*>(m_pool->GetBuffer(4, 4));
    EXPECT_NE(shaderCopy, nullptr);
    if (shaderCopy)
    {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shaderCopy->GetCurrentFramebuffer());
      glBlitFramebuffer(0, 4, 4, 0, 0, 0, 4, 4, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, shaderCopy->GetCurrentFramebuffer());
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
      EXPECT_EQ(pixel[0], 255);
      EXPECT_EQ(pixel[3], 255);
      shaderCopy->Release();
    }
    captured->Release();
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, clientFramebuffer);
  unsigned char clientPixel[4]{};
  glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clientPixel);
  EXPECT_EQ(clientPixel[0], 255);
  EXPECT_EQ(clientPixel[3], 0);
  EXPECT_EQ(client->GetCurrentFramebuffer(), clientFramebuffer);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
  client->Release();
}

TEST_P(TestRenderBufferPoolFBOWithContext, ClientFramebufferRemainsStableAcrossSizeChanges)
{
  auto* client = static_cast<CRenderBufferFBO*>(m_pool->GetBuffer(4, 4));
  ASSERT_NE(client, nullptr);
  const auto framebuffer = client->GetCurrentFramebuffer();
  ASSERT_NE(framebuffer, 0u);
  EXPECT_TRUE(client->Allocate(AV_PIX_FMT_NONE, 8, 8));
  EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
  EXPECT_TRUE(client->Allocate(AV_PIX_FMT_NONE, 2, 2));
  EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
  EXPECT_FALSE(client->Allocate(AV_PIX_FMT_NONE, 0, 0));
  EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
  client->Release();
}

TEST_P(TestRenderBufferPoolFBOWithContext, RenderedCaptureCannotBeReusedBeforeGUIHandoff)
{
  auto* client = m_pool->GetBuffer(4, 4);
  ASSERT_NE(client, nullptr);
  auto* displayed = Capture(client);
  ASSERT_NE(displayed, nullptr);
  const auto texture = displayed->TextureID();
  ASSERT_TRUE(BindGUI());
  Sample(displayed);
  displayed->Release();

  ASSERT_TRUE(BindClient());
  auto* next = Capture(client);
  ASSERT_NE(next, nullptr);
  EXPECT_NE(next->TextureID(), texture);
  next->Release();
  client->Release();
}

TEST_P(TestRenderBufferPoolFBOWithContext, RepeatedSamplesSubmitOnceAtGUIHandoff)
{
  auto* client = m_pool->GetBuffer(4, 4);
  ASSERT_NE(client, nullptr);
  auto* captured = Capture(client);
  ASSERT_NE(captured, nullptr);
  ASSERT_TRUE(BindGUI());

  for (unsigned int sample = 0; sample < 5; ++sample)
    Sample(captured);
  EXPECT_EQ(m_fences.size(), 0u);
  EXPECT_EQ(m_flushes, 0u);
  captured->Release();
  m_pool->FlushRendered();
  EXPECT_EQ(m_fences.size(), 1u);
  EXPECT_EQ(m_flushes, 1u);
  EXPECT_EQ(m_flushContext.load(), m_sharedContext);
  m_pool->FlushRendered();
  m_pool->FlushRendered();
  EXPECT_EQ(m_flushes, 1u);
  client->Release();
}

TEST_P(TestRenderBufferPoolFBOWithContext, BothDisplayedBuffersWaitForLatestFenceBeforeReuse)
{
  auto* client = m_pool->GetBuffer(4, 4);
  ASSERT_NE(client, nullptr);
  auto* old = Capture(client);
  ASSERT_NE(old, nullptr);
  const GLuint oldTexture = old->TextureID();
  ASSERT_TRUE(BindGUI());

  Sample(old);
  Sample(old);
  EXPECT_TRUE(m_fences.empty());
  ASSERT_TRUE(BindClient());
  auto* current = Capture(client);
  ASSERT_NE(current, nullptr);
  const GLuint currentTexture = current->TextureID();
  ASSERT_NE(oldTexture, currentTexture);
  ASSERT_TRUE(BindGUI());
  Sample(current);
  old->Release();
  ASSERT_TRUE(BindClient());
  auto* beforeHandoff = Capture(client);
  ASSERT_NE(beforeHandoff, nullptr);
  EXPECT_NE(beforeHandoff->TextureID(), oldTexture);
  ASSERT_TRUE(BindGUI());

  m_pool->FlushRendered();
  ASSERT_EQ(m_fences.size(), 2u);
  const GLsync oldFence = m_fences[m_fences.size() - 2];
  EXPECT_EQ(m_flushes, 1u);
  EXPECT_EQ(m_flushContext.load(), m_sharedContext);
  ASSERT_TRUE(BindClient());
  auto* reused = Capture(client);
  ASSERT_NE(reused, nullptr);
  EXPECT_EQ(reused->TextureID(), oldTexture);
  ASSERT_FALSE(m_waited.empty());
  EXPECT_EQ(m_waited.back(), oldFence);
  reused->Release();
  beforeHandoff->Release();
  current->Release();
  client->Release();
}

TEST_P(TestRenderBufferPoolFBOWithContext, PausedFrameSamplesReuseOneCapture)
{
  auto* client = m_pool->GetBuffer(4, 4);
  ASSERT_NE(client, nullptr);
  auto* captured = Capture(client);
  ASSERT_NE(captured, nullptr);
  const GLuint texture = captured->TextureID();
  ASSERT_TRUE(BindGUI());
  for (unsigned int tick = 0; tick < 60; ++tick)
  {
    for (unsigned int sample = 0; sample < 5; ++sample)
    {
      Sample(captured);
      EXPECT_EQ(captured->TextureID(), texture);
    }
    EXPECT_EQ(m_fences.size(), tick);
    EXPECT_EQ(m_flushes, tick);
    m_pool->FlushRendered();
    EXPECT_EQ(m_fences.size(), tick + 1);
    EXPECT_EQ(m_flushes, tick + 1);
  }
  ASSERT_EQ(m_fences.size(), 60u);
  const GLsync latest = m_fences.back();
  captured->Release();
  m_pool->FlushRendered();
  EXPECT_EQ(m_flushes, 60u);
  ASSERT_TRUE(BindClient());
  auto* reused = Capture(client);
  ASSERT_NE(reused, nullptr);
  EXPECT_EQ(reused->TextureID(), texture);
  ASSERT_FALSE(m_waited.empty());
  EXPECT_EQ(m_waited.back(), latest);
  reused->Release();
  client->Release();
}

TEST_P(TestRenderBufferPoolFBOWithContext, ClientThreadTeardownWaitsForGUISubmission)
{
  auto* client = m_pool->GetBuffer(4, 4);
  ASSERT_NE(client, nullptr);
  auto* captured = Capture(client);
  ASSERT_NE(captured, nullptr);
  const GLuint texture = captured->TextureID();
  ASSERT_TRUE(BindGUI());
  Sample(captured);
  captured->Release();
  client->Release();

  RunAfterClientCallbackQueued([](CRenderBufferPoolFBO& pool) { pool.DestroyContext(); },
                               [&]
                               {
                                 EXPECT_EQ(m_flushes, 0u);
                                 EXPECT_EQ(glIsTexture(texture), GL_TRUE);
                                 CServiceBroker::GetAppMessenger()->ProcessMessages();
                                 EXPECT_EQ(m_flushes, 1u);
                                 EXPECT_EQ(m_flushContext.load(), m_sharedContext);
                                 EXPECT_EQ(glIsTexture(texture), GL_TRUE);
                               });
  EXPECT_EQ(glIsTexture(texture), GL_FALSE);
}

TEST_P(TestRenderBufferPoolFBOWithContext, GUIDrainWakesClientAndStaleCallbackCannotFlushNextBatch)
{
  auto* client = m_pool->GetBuffer(4, 4);
  ASSERT_NE(client, nullptr);
  auto* captured = Capture(client);
  ASSERT_NE(captured, nullptr);
  ASSERT_TRUE(BindGUI());
  Sample(captured);
  captured->Release();

  RunAfterClientCallbackQueued([](CRenderBufferPoolFBO& pool) { pool.FlushRendered(); },
                               [&]
                               {
                                 m_pool->FlushRendered();
                                 EXPECT_EQ(m_flushes, 1u);
                               });

  ASSERT_TRUE(BindClient());
  auto* next = Capture(client);
  ASSERT_NE(next, nullptr);
  ASSERT_TRUE(BindGUI());
  Sample(next);
  next->Release();
  CServiceBroker::GetAppMessenger()->ProcessMessages();
  EXPECT_EQ(m_flushes, 1u);
  m_pool->FlushRendered();
  EXPECT_EQ(m_flushes, 2u);

  ASSERT_TRUE(BindClient());
  auto* closing = Capture(client);
  ASSERT_NE(closing, nullptr);
  ASSERT_TRUE(BindGUI());
  Sample(closing);
  closing->Release();
  client->Release();
  RunAfterClientCallbackQueued([](CRenderBufferPoolFBO& pool) { pool.DestroyContext(); },
                               [&]
                               {
                                 m_pool->FlushRendered();
                                 EXPECT_EQ(m_flushes, 3u);
                               });
  m_pool.reset();
  CServiceBroker::GetAppMessenger()->ProcessMessages();
  EXPECT_EQ(m_flushes, 3u);
}

INSTANTIATE_TEST_SUITE_P(BindingModes, TestRenderBufferPoolFBOWithContext, testing::Bool());

#endif

#endif
