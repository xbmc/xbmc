/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"

#if (defined(HAS_EGL) || defined(TARGET_DARWIN_OSX)) && (defined(HAS_GL) || HAS_GLES == 3)

#include "cores/RetroPlayer/buffers/RenderBufferManager.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolFBO.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/rendering/contexts/IHwRenderingContext.h"

#include <future>
#include <memory>

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

    m_pool = std::make_shared<CRenderBufferPoolFBO>(
        m_environment.ProcessInfo().GetRenderContext(),
        std::make_unique<CTestEGLContext>(api, m_display, config, m_sharedContext,
                                          contextAttributes, surfaceless));
    m_current = m_pool->BeginClientFrame();
    ASSERT_TRUE(m_current) << "The shared client context must bind using the selected surface mode";
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
  }

  void TearDown() override
  {
    if (m_current)
      m_pool->EndClientFrame();
    m_pool.reset();
    if (m_sharedContext != EGL_NO_CONTEXT)
      eglDestroyContext(m_display, m_sharedContext);
    if (m_initialized)
      eglTerminate(m_display);
    if (m_previousAPI != EGL_NONE)
      eglBindAPI(m_previousAPI);
  }

  CPlaybackTestEnvironment m_environment;
  std::shared_ptr<CRenderBufferPoolFBO> m_pool;
  EGLDisplay m_display{EGL_NO_DISPLAY};
  EGLContext m_sharedContext{EGL_NO_CONTEXT};
  EGLenum m_previousAPI{EGL_NONE};
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

INSTANTIATE_TEST_SUITE_P(BindingModes, TestRenderBufferPoolFBOWithContext, testing::Bool());

#endif

#endif
