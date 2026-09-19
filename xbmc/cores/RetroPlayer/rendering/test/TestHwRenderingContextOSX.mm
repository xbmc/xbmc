/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/RenderBufferManager.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolFBO.h"
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"
#include "cores/RetroPlayer/rendering/contexts/HwRenderingContextOSX.h"
#include "cores/RetroPlayer/rendering/contexts/IHwRenderingContext.h"
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#include "cores/RetroPlayer/streams/RetroPlayerRendering.h"
#include "games/addons/streams/GameClientStreamHwFramebuffer.h"
#include "guilib/GUITextureGL.h"
#include "rendering/MatrixGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"

#include <algorithm>
#include <array>
#include <thread>
#include <vector>

#import <AppKit/NSOpenGL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <gtest/gtest.h>
#include <objc/runtime.h>

using namespace KODI::RETRO;

class TestHwRenderingContextOSX : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previousNSContext = [NSOpenGLContext currentContext];
    m_previousCGLContext = CGLGetCurrentContext();
    if (m_previousCGLContext)
      CGLRetainContext(m_previousCGLContext);

    const NSOpenGLPixelFormatAttribute attributes[] = {NSOpenGLPFAOpenGLProfile,
                                                       NSOpenGLProfileVersion3_2Core,
                                                       NSOpenGLPFAAccelerated,
                                                       NSOpenGLPFANoRecovery,
                                                       NSOpenGLPFAColorSize,
                                                       32,
                                                       NSOpenGLPFAAlphaSize,
                                                       8,
                                                       0};
    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    m_guiContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
    if (!m_guiContext)
      GTEST_SKIP() << "An accelerated native core-profile context is unavailable";
    [m_guiContext makeCurrentContext];
    ASSERT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
  }

  void TearDown() override
  {
    if (m_previousNSContext)
      [m_previousNSContext makeCurrentContext];
    else
      [NSOpenGLContext clearCurrentContext];
    CGLSetCurrentContext(m_previousCGLContext);
    if (m_previousCGLContext)
      CGLReleaseContext(m_previousCGLContext);
    m_guiContext = nil;
  }

  NSOpenGLContext* m_guiContext{nil};
  NSOpenGLContext* m_previousNSContext{nil};
  CGLContextObj m_previousCGLContext{nullptr};
};

TEST(TestHwRenderingContextOSXUnavailable, MissingSharedContextRejectsHardwareRendering)
{
  auto context = CreateHwRenderingContextOSX(nil);
  EXPECT_FALSE(context->SupportsHardwareRendering());
  EXPECT_FALSE(context->Create({}));
  EXPECT_FALSE(context->MakeCurrent());
  EXPECT_FALSE(context->IsCreated());
}

TEST_F(TestHwRenderingContextOSX, CapabilityProbePreservesGuiContextAndState)
{
  GLuint vertexArray = 0;
  glGenVertexArrays(1, &vertexArray);
  glBindVertexArray(vertexArray);

  auto context = CreateHwRenderingContextOSX(m_guiContext);
  EXPECT_TRUE(context->SupportsHardwareRendering());
  EXPECT_FALSE(context->IsCreated());
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
  GLint currentVertexArray = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVertexArray);
  EXPECT_EQ(currentVertexArray, vertexArray);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
  glDeleteVertexArrays(1, &vertexArray);
}

TEST_F(TestHwRenderingContextOSX, HigherActualVersionSatisfiesMinimum33)
{
  GLint major = 0;
  GLint minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  if (major < 3 || (major == 3 && minor < 3))
    GTEST_SKIP() << "The native renderer does not support OpenGL 3.3";

  HwContextProperties properties;
  properties.versionMajor = 3;
  properties.versionMinor = 3;
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  ASSERT_TRUE(context->Create(properties));
  ASSERT_TRUE(context->MakeCurrent());
  GLint actualMajor = 0;
  GLint actualMinor = 0;
  GLint profile = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &actualMajor);
  glGetIntegerv(GL_MINOR_VERSION, &actualMinor);
  glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
  RecordProperty("GL_VERSION", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
  RecordProperty("GL_RENDERER", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
  EXPECT_TRUE(actualMajor > 3 || (actualMajor == 3 && actualMinor >= 3));
  EXPECT_NE(profile & GL_CONTEXT_CORE_PROFILE_BIT, 0);
  context->RestoreCurrent();
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
}

TEST_F(TestHwRenderingContextOSX, RejectsUnsupportedVersionsAndProfiles)
{
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  HwContextProperties properties;
  properties.versionMajor = 4;
  properties.versionMinor = 2;
  EXPECT_FALSE(context->Create(properties));
  properties = {};
  properties.coreProfile = false;
  EXPECT_FALSE(context->Create(properties));
  properties = {};
  properties.embedded = true;
  EXPECT_FALSE(context->Create(properties));
  EXPECT_FALSE(context->IsCreated());
  EXPECT_FALSE(context->MakeCurrent());
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
}

TEST_F(TestHwRenderingContextOSX, RejectsDebugContextAndAllowsOrdinaryContextAfterwards)
{
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  HwContextProperties properties;
  properties.debugContext = true;
  EXPECT_FALSE(context->Create(properties));
  EXPECT_FALSE(context->IsCreated());
  EXPECT_FALSE(context->MakeCurrent());
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);

  properties.debugContext = false;
  ASSERT_TRUE(context->Create(properties));
  ASSERT_TRUE(context->MakeCurrent());
  context->RestoreCurrent();
  context->Destroy();
  EXPECT_FALSE(context->IsCreated());
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestHwRenderingContextOSX, LegacyGuiProfileCannotAdvertiseSharedCoreContext)
{
  const NSOpenGLPixelFormatAttribute attributes[] = {
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy, NSOpenGLPFAAccelerated,
      NSOpenGLPFANoRecovery, 0};
  NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
  NSOpenGLContext* legacyContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
  if (!legacyContext)
    GTEST_SKIP() << "A legacy native context is unavailable";

  auto context = CreateHwRenderingContextOSX(legacyContext);
  EXPECT_FALSE(context->SupportsHardwareRendering());
  EXPECT_FALSE(context->Create({}));
  EXPECT_FALSE(context->MakeCurrent());
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
}

TEST_F(TestHwRenderingContextOSX, SharesTextureContentsWithGuiContext)
{
  const std::array<unsigned char, 4> expected{17, 83, 149, 255};
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, expected.data());
  glFlush();

  auto context = CreateHwRenderingContextOSX(m_guiContext);
  ASSERT_TRUE(context->Create({}));
  ASSERT_TRUE(context->MakeCurrent());
  EXPECT_NE([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetShareGroup(CGLGetCurrentContext()),
            CGLGetShareGroup([m_guiContext CGLContextObj]));
  EXPECT_TRUE(glIsTexture(texture));
  glBindTexture(GL_TEXTURE_2D, texture);
  std::array<unsigned char, 4> actual{};
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, actual.data());
  EXPECT_EQ(actual, expected);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
  context->RestoreCurrent();
  glDeleteTextures(1, &texture);
}

TEST_F(TestHwRenderingContextOSX, RestoresNullContextAfterProbeAndClientScope)
{
  [NSOpenGLContext clearCurrentContext];
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  EXPECT_EQ([NSOpenGLContext currentContext], nil);
  EXPECT_EQ(CGLGetCurrentContext(), nullptr);
  ASSERT_TRUE(context->Create({}));
  ASSERT_TRUE(context->MakeCurrent());
  context->RestoreCurrent();
  EXPECT_EQ([NSOpenGLContext currentContext], nil);
  EXPECT_EQ(CGLGetCurrentContext(), nullptr);
}

TEST_F(TestHwRenderingContextOSX, RestoresRawCGLContext)
{
  CGLContextObj rawContext = nullptr;
  ASSERT_EQ(CGLCreateContext([[m_guiContext pixelFormat] CGLPixelFormatObj], nullptr, &rawContext),
            kCGLNoError);
  [NSOpenGLContext clearCurrentContext];
  ASSERT_EQ(CGLSetCurrentContext(rawContext), kCGLNoError);
  NSOpenGLContext* previousNSContext = [NSOpenGLContext currentContext];
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), rawContext);
  EXPECT_EQ([NSOpenGLContext currentContext], previousNSContext);
  ASSERT_TRUE(context->Create({}));
  ASSERT_TRUE(context->MakeCurrent());
  context->RestoreCurrent();
  EXPECT_EQ(CGLGetCurrentContext(), rawContext);
  EXPECT_EQ([NSOpenGLContext currentContext], previousNSContext);
  [m_guiContext makeCurrentContext];
  CGLReleaseContext(rawContext);
}

TEST_F(TestHwRenderingContextOSX, GameThreadScopeLeavesGuiThreadCurrentContextUntouched)
{
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  ASSERT_TRUE(context->Create({}));
  std::thread gameThread(
      [&context]()
      {
        @autoreleasepool
        {
          EXPECT_EQ([NSOpenGLContext currentContext], nil);
          ASSERT_TRUE(context->MakeCurrent());
          EXPECT_NE(CGLGetCurrentContext(), nullptr);
          context->RestoreCurrent();
          EXPECT_EQ([NSOpenGLContext currentContext], nil);
          EXPECT_EQ(CGLGetCurrentContext(), nullptr);
        }
      });
  gameThread.join();
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
}

TEST_F(TestHwRenderingContextOSX, FailedBindRestoresGuiContextAndAllowsRetry)
{
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  ASSERT_TRUE(context->Create({}));
  NSOpenGLContext* previousNSContext = [NSOpenGLContext currentContext];
  CGLContextObj previousCGLContext = CGLGetCurrentContext();
  ASSERT_EQ(previousNSContext, m_guiContext);
  ASSERT_EQ(previousCGLContext, [m_guiContext CGLContextObj]);

  {
    const SEL selector = @selector(makeCurrentContext);
    const Method method = class_getInstanceMethod([NSOpenGLContext class], selector);
    const IMP original = method_getImplementation(method);
    const auto callingThread = std::this_thread::get_id();
    const IMP replacement = imp_implementationWithBlock(^(NSOpenGLContext* receiver) {
      reinterpret_cast<void (*)(id, SEL)>(original)(receiver, selector);
      if (receiver != previousNSContext && std::this_thread::get_id() == callingThread)
      {
        // Invalidate the binding after Cocoa has switched away from the caller.
        EXPECT_EQ(CGLSetCurrentContext(nullptr), kCGLNoError);
      }
    });
    struct CRestoreImplementation
    {
      ~CRestoreImplementation()
      {
        method_setImplementation(method, original);
        imp_removeBlock(replacement);
      }
      Method method;
      IMP original;
      IMP replacement;
    } restore{method, original, replacement};
    method_setImplementation(method, replacement);

    EXPECT_FALSE(context->MakeCurrent());
    EXPECT_EQ([NSOpenGLContext currentContext], previousNSContext);
    EXPECT_EQ(CGLGetCurrentContext(), previousCGLContext);
  }

  ASSERT_TRUE(context->MakeCurrent());
  context->RestoreCurrent();
  EXPECT_EQ([NSOpenGLContext currentContext], previousNSContext);
  EXPECT_EQ(CGLGetCurrentContext(), previousCGLContext);
}

TEST_F(TestHwRenderingContextOSX, RejectedNestedBindPreservesOriginalCaller)
{
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  ASSERT_TRUE(context->Create({}));
  ASSERT_TRUE(context->MakeCurrent());
  NSOpenGLContext* clientContext = [NSOpenGLContext currentContext];
  CGLContextObj clientCGLContext = CGLGetCurrentContext();
  EXPECT_FALSE(context->MakeCurrent());
  EXPECT_EQ([NSOpenGLContext currentContext], clientContext);
  EXPECT_EQ(CGLGetCurrentContext(), clientCGLContext);
  context->RestoreCurrent();
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
  context->RestoreCurrent();
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(CGLGetCurrentContext(), [m_guiContext CGLContextObj]);
}

TEST_F(TestHwRenderingContextOSX, DestroyRestoresGuiAndAllowsRecreation)
{
  auto context = CreateHwRenderingContextOSX(m_guiContext);
  ASSERT_TRUE(context->Create({}));
  ASSERT_TRUE(context->MakeCurrent());
  context->Destroy();
  EXPECT_FALSE(context->IsCreated());
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_TRUE(context->SupportsHardwareRendering());
  ASSERT_TRUE(context->Create({}));
  ASSERT_TRUE(context->MakeCurrent());
  context->RestoreCurrent();
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
}

namespace
{
struct CReleaseRenderBuffer
{
  void operator()(IRenderBuffer* buffer) const { buffer->Release(); }
};
using FBOBufferPtr = std::unique_ptr<CRenderBufferFBO, CReleaseRenderBuffer>;

class CControlledContext : public IHwRenderingContext
{
public:
  explicit CControlledContext(std::unique_ptr<IHwRenderingContext> context)
    : m_context(std::move(context))
  {
  }
  bool SupportsHardwareRendering() const override { return m_context->SupportsHardwareRendering(); }
  bool Create(const HwContextProperties& properties) override
  {
    return m_context->Create(properties);
  }
  bool IsCreated() const override { return m_context->IsCreated(); }
  bool MakeCurrent() override
  {
    ++binds;
    return !failBind && m_context->MakeCurrent();
  }
  void RestoreCurrent() override
  {
    ++restores;
    m_context->RestoreCurrent();
  }
  void Destroy() override
  {
    ++destroys;
    m_context->Destroy();
  }
  bool failBind{false};
  unsigned int binds{0};
  unsigned int restores{0};
  unsigned int destroys{0};

private:
  std::unique_ptr<IHwRenderingContext> m_context;
};
} // namespace

class TestRenderBufferPoolFBOOSX : public TestHwRenderingContextOSX
{
protected:
  void SetUp() override
  {
    TestHwRenderingContextOSX::SetUp();
    if (!m_guiContext)
      return;

    m_environment = std::make_unique<CPlaybackTestEnvironment>();
    auto context = std::make_unique<CControlledContext>(CreateHwRenderingContextOSX(m_guiContext));
    m_native = context.get();
    m_pool = std::make_shared<CRenderBufferPoolFBO>(m_environment->ProcessInfo().GetRenderContext(),
                                                    std::move(context));
    ASSERT_TRUE(m_pool->SupportsHardwareRendering());
    ASSERT_TRUE(m_pool->CreateContext({}));
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
  }

  void TearDown() override
  {
    if (m_pool)
      m_pool->DestroyContext();
    m_pool.reset();
    m_environment.reset();
    TestHwRenderingContextOSX::TearDown();
  }

  FBOBufferPtr GetClient(unsigned int width, unsigned int height)
  {
    return FBOBufferPtr(static_cast<CRenderBufferFBO*>(m_pool->GetBuffer(width, height)));
  }

  FBOBufferPtr Capture(IRenderBuffer* client, unsigned int width, unsigned int height)
  {
    return FBOBufferPtr(
        static_cast<CRenderBufferFBO*>(m_pool->CaptureClientFrame(client, width, height)));
  }

  std::unique_ptr<CPlaybackTestEnvironment> m_environment;
  std::shared_ptr<CRenderBufferPoolFBO> m_pool;
  CControlledContext* m_native{nullptr};
};

TEST_F(TestRenderBufferPoolFBOOSX, LostContextRetiresSurvivingBuffersAndAllowsRepeatedRecovery)
{
  for (unsigned int cycle = 0; cycle < 3; ++cycle)
  {
    ASSERT_TRUE(m_pool->BeginClientFrame());
    auto client = GetClient(4, 4);
    ASSERT_NE(client, nullptr);
    auto captured = Capture(client.get(), 4, 4);
    ASSERT_NE(captured, nullptr);
    auto freeCapture = Capture(client.get(), 4, 4);
    ASSERT_NE(freeCapture, nullptr);
    freeCapture.reset();
    m_pool->EndClientFrame();
    captured->WaitForCapture();
    captured->FinishRender();
    const GLuint texture = captured->TextureID();
    const GLuint clientTexture = client->TextureID();
    const auto restores = m_native->restores;
    const auto destroys = m_native->destroys;

    m_native->failBind = true;
    EXPECT_FALSE(m_pool->BeginClientFrame());
    m_pool->DestroyContext();
    EXPECT_FALSE(m_native->IsCreated());
    EXPECT_EQ(m_native->destroys, destroys + 1);
    EXPECT_EQ(m_native->restores, restores);
    EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
    EXPECT_EQ(client->GetCurrentFramebuffer(), 0);
    EXPECT_EQ(captured->TextureID(), 0);
    EXPECT_FALSE(client->Allocate(AV_PIX_FMT_NONE, 8, 8));
    EXPECT_FALSE(captured->SetReady());
    captured->WaitForCapture();
    captured->FinishRender();
    EXPECT_TRUE(glIsTexture(texture));
    EXPECT_TRUE(glIsTexture(clientTexture));
    m_pool->DestroyContext();
    EXPECT_EQ(m_native->destroys, destroys + 1);

    m_native->failBind = false;
    ASSERT_TRUE(m_pool->CreateContext({}));
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
    ASSERT_TRUE(m_pool->BeginClientFrame());
    auto next = GetClient(4, 4);
    ASSERT_NE(next, nullptr);
    auto nextCapture = Capture(next.get(), 4, 4);
    ASSERT_NE(nextCapture, nullptr);
    m_pool->EndClientFrame();
    client.reset();
    captured.reset();
    EXPECT_TRUE(glIsTexture(texture));
    EXPECT_TRUE(glIsTexture(clientTexture));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    // These are known shared objects in the controlled, healthy test share group.
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &clientTexture);
  }
}

TEST_F(TestRenderBufferPoolFBOOSX, OrdinaryDestructionDeletesResourcesAndRetiresHeldBuffers)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  auto captured = Capture(client.get(), 4, 4);
  ASSERT_NE(captured, nullptr);
  const GLuint texture = captured->TextureID();
  m_pool->EndClientFrame();
  m_pool->DestroyContext();
  EXPECT_FALSE(m_native->IsCreated());
  EXPECT_EQ(m_native->destroys, 1);
  EXPECT_EQ(m_native->restores, 2);
  EXPECT_FALSE(glIsTexture(texture));
  EXPECT_EQ(client->GetCurrentFramebuffer(), 0);
  EXPECT_EQ(captured->TextureID(), 0);
  captured->FinishRender();
  EXPECT_FALSE(captured->SetReady());
  EXPECT_FALSE(client->Allocate(AV_PIX_FMT_NONE, 4, 4));
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestRenderBufferPoolFBOOSX, AbandonedGameStreamReleasesManagerAndPoolForNextGame)
{
  class CCallbacks : public KODI::GAME::IHwFramebufferCallback
  {
  public:
    bool HardwareContextReset() override
    {
      ++resets;
      return true;
    }
    void HardwareContextDestroy() override { ++destroys; }
    unsigned int resets{0};
    unsigned int destroys{0};
  } callbacks;

  m_pool->DestroyContext();
  m_environment->ProcessInfo().GetBufferManager().RegisterPools(nullptr, {m_pool});
  auto& manager = m_environment->Renderer();
  CRetroPlayerRendering rendering(manager, m_environment->ProcessInfo());
  game_hw_rendering_properties hardware{};
  hardware.context_type = GAME_HW_CONTEXT_OPENGL_CORE;
  hardware.version_major = 3;
  hardware.version_minor = 2;
  KODI::GAME::CGameClientStreamHwFramebuffer stream(callbacks, hardware);
  game_stream_properties properties{};
  properties.type = GAME_STREAM_HW_FRAMEBUFFER;
  properties.hw_framebuffer.max_width = properties.hw_framebuffer.max_height = 4;

  for (unsigned int cycle = 0; cycle < 4; ++cycle)
  {
    ASSERT_TRUE(stream.OpenStream(&rendering, properties));
    ASSERT_TRUE(manager.BeginClientFrame());
    ASSERT_TRUE(stream.ResetHwContext());
    game_stream_buffer buffer{};
    buffer.type = GAME_STREAM_HW_FRAMEBUFFER;
    ASSERT_TRUE(stream.GetBuffer(4, 4, buffer));
    EXPECT_NE(buffer.hw_framebuffer.framebuffer, 0);
    manager.RenderFrame(4, 4, 1.0f, 0);
    if (cycle == 3)
      stream.DestroyHwContext();
    manager.EndClientFrame();

    if (cycle != 3)
    {
      m_native->failBind = true;
      EXPECT_FALSE(manager.BeginClientFrame());
      stream.AbandonHwContext();
    }
    const auto restores = m_native->restores;
    stream.CloseStream();
    EXPECT_FALSE(m_native->IsCreated());
    EXPECT_EQ(callbacks.resets, cycle + 1);
    EXPECT_EQ(callbacks.destroys, cycle == 3 ? 1 : 0);
    EXPECT_EQ(m_native->restores, restores + (cycle == 3 ? 1 : 0));
    stream.CloseStream();
    EXPECT_EQ(m_native->destroys, cycle + 2);
    m_native->failBind = false;
  }
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestRenderBufferPoolFBOOSX, NestedScopesRestoreGuiOnlyAfterOutermostEnd)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  NSOpenGLContext* clientContext = [NSOpenGLContext currentContext];
  ASSERT_NE(clientContext, m_guiContext);
  ASSERT_TRUE(m_pool->BeginClientFrame());
  EXPECT_EQ([NSOpenGLContext currentContext], clientContext);

  std::thread otherThread(
      [this]()
      {
        EXPECT_FALSE(m_pool->BeginClientFrame());
        EXPECT_EQ(CGLGetCurrentContext(), nullptr);
      });
  otherThread.join();

  m_pool->EndClientFrame();
  EXPECT_EQ([NSOpenGLContext currentContext], clientContext);
  m_pool->EndClientFrame();
  EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
}

TEST_F(TestRenderBufferPoolFBOOSX, ClientFramebufferRemainsStableDuringGrowthAndRejectedAllocation)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  const GLuint framebuffer = client->GetCurrentFramebuffer();
  ASSERT_NE(framebuffer, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  ASSERT_TRUE(client->Allocate(AV_PIX_FMT_NONE, 8, 8));
  EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
  EXPECT_EQ(client->TextureWidth(), 8);
  EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

  GLint maximumTextureSize = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
  EXPECT_FALSE(client->Allocate(AV_PIX_FMT_NONE, maximumTextureSize + 1, 8));
  EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
  EXPECT_EQ(client->TextureWidth(), 8);
  EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestRenderBufferPoolFBOOSX, CapturedTextureIsOpaqueAndReadableThroughGuiFramebuffer)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  const GLuint framebuffer = client->GetCurrentFramebuffer();
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  auto captured = Capture(client.get(), 4, 4);
  ASSERT_NE(captured, nullptr);

  std::array<unsigned char, 4> pixel{};
  glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
  EXPECT_EQ(pixel, (std::array<unsigned char, 4>{255, 0, 0, 0}));
  EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
  m_pool->EndClientFrame();
  ASSERT_EQ([NSOpenGLContext currentContext], m_guiContext);

  captured->WaitForCapture();
  EXPECT_TRUE(glIsTexture(captured->TextureID()));
  GLuint guiFramebuffer = 0;
  glGenFramebuffers(1, &guiFramebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, guiFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, captured->TextureID(),
                         0);
  EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
  glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
  EXPECT_EQ(pixel, (std::array<unsigned char, 4>{255, 0, 0, 255}));
  captured->FinishRender();
  glDeleteFramebuffers(1, &guiFramebuffer);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestRenderBufferPoolFBOOSX, GuiShaderSamplesCapturedTextureWithNearestAndLinearFiltering)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, client->GetCurrentFramebuffer());
  glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  auto captured = Capture(client.get(), 4, 4);
  ASSERT_NE(captured, nullptr);
  m_pool->EndClientFrame();
  ASSERT_EQ([NSOpenGLContext currentContext], m_guiContext);
  captured->WaitForCapture();

  struct ShaderDrawResources
  {
    ~ShaderDrawResources()
    {
      glUseProgram(0);
      glDeleteProgram(program);
      glDeleteShader(vertexShader);
      glDeleteShader(fragmentShader);
      glDeleteVertexArrays(1, &vertexArray);
      glDeleteFramebuffers(1, &framebuffer);
      glDeleteTextures(1, &texture);
    }
    GLuint program{0};
    GLuint vertexShader{0};
    GLuint fragmentShader{0};
    GLuint vertexArray{0};
    GLuint framebuffer{0};
    GLuint texture{0};
  } resources;

  const char* vertexSource = R"(#version 150
    out vec2 texcoord;
    void main()
    {
      vec2 positions[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
      vec2 position = positions[gl_VertexID];
      texcoord = (position + vec2(1)) * 0.5;
      gl_Position = vec4(position, 0, 1);
    }
  )";
  const char* fragmentSource = R"(#version 150
    uniform sampler2D image;
    in vec2 texcoord;
    out vec4 color;
    void main() { color = texture(image, texcoord); }
  )";
  const auto compileShader = [](GLenum type, const char* source)
  {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    std::array<char, 1024> log{};
    glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
    EXPECT_EQ(compiled, GL_TRUE) << log.data();
    return shader;
  };
  resources.vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
  resources.fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
  resources.program = glCreateProgram();
  glAttachShader(resources.program, resources.vertexShader);
  glAttachShader(resources.program, resources.fragmentShader);
  glLinkProgram(resources.program);
  GLint linked = GL_FALSE;
  glGetProgramiv(resources.program, GL_LINK_STATUS, &linked);
  std::array<char, 1024> log{};
  glGetProgramInfoLog(resources.program, log.size(), nullptr, log.data());
  ASSERT_EQ(linked, GL_TRUE) << log.data();
  glUseProgram(resources.program);
  glUniform1i(glGetUniformLocation(resources.program, "image"), 0);

  glGenTextures(1, &resources.texture);
  glBindTexture(GL_TEXTURE_2D, resources.texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenFramebuffers(1, &resources.framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, resources.framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources.texture, 0);
  ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, 4, 4);
  glGenVertexArrays(1, &resources.vertexArray);
  glBindVertexArray(resources.vertexArray);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, captured->TextureID());
  GLint minFilter = 0;
  GLint sampler = 0;
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minFilter);
  glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
  EXPECT_EQ(minFilter, GL_LINEAR);
  EXPECT_EQ(sampler, 0);
  for (GLint filter : {GL_LINEAR, GL_NEAREST})
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::array<unsigned char, 4> pixel{};
    glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_EQ(pixel, (std::array<unsigned char, 4>{255, 0, 0, 255}));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
  }
  captured->FinishRender();
}

TEST_F(TestRenderBufferPoolFBOOSX, CaptureNormalizesBothOriginsWithinLargerClientFramebuffer)
{
  for (bool bottomLeft : {false, true})
  {
    m_pool->DestroyContext();
    HwContextProperties properties;
    properties.bottomLeftOrigin = bottomLeft;
    ASSERT_TRUE(m_pool->CreateContext(properties));
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
    ASSERT_TRUE(m_pool->BeginClientFrame());
    auto client = GetClient(16, 16);
    ASSERT_NE(client, nullptr);
    const auto framebuffer = client->GetCurrentFramebuffer();

    for (unsigned int height : {4u, 8u, 2u})
    {
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
      glDisable(GL_SCISSOR_TEST);
      glClearColor(0, 0, 1, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glEnable(GL_SCISSOR_TEST);
      glScissor(0, bottomLeft ? height / 2 : 0, 4, height / 2);
      glClearColor(1, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glScissor(0, bottomLeft ? 0 : height / 2, 4, height / 2);
      glClearColor(0, 1, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glReadBuffer(GL_NONE);
      glViewport(1, 2, 3, 4);

      auto captured = Capture(client.get(), 4, height);
      ASSERT_NE(captured, nullptr);
      EXPECT_FALSE(captured->BottomLeftOrigin());
      EXPECT_EQ(captured->TextureWidth(), 4);
      EXPECT_EQ(captured->TextureHeight(), height);
      EXPECT_EQ(client->GetCurrentFramebuffer(), framebuffer);
      EXPECT_EQ(client->TextureWidth(), 16);
      EXPECT_EQ(client->TextureHeight(), 16);
      GLint binding = 0;
      glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &binding);
      EXPECT_EQ(binding, framebuffer);
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &binding);
      EXPECT_EQ(binding, framebuffer);
      glGetIntegerv(GL_READ_BUFFER, &binding);
      EXPECT_EQ(binding, GL_NONE);
      EXPECT_TRUE(glIsEnabled(GL_SCISSOR_TEST));
      std::array<GLint, 4> viewport{};
      glGetIntegerv(GL_VIEWPORT, viewport.data());
      EXPECT_EQ(viewport, (std::array<GLint, 4>{1, 2, 3, 4}));
      glReadBuffer(GL_COLOR_ATTACHMENT0);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, captured->GetCurrentFramebuffer());
      std::array<unsigned char, 4> pixel{};
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
      EXPECT_EQ(pixel, (std::array<unsigned char, 4>{255, 0, 0, 255}));
      glReadPixels(0, height - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
      EXPECT_EQ(pixel, (std::array<unsigned char, 4>{0, 255, 0, 255}));
      EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
    glDisable(GL_SCISSOR_TEST);
    m_pool->EndClientFrame();
  }
}

TEST_F(TestRenderBufferPoolFBOOSX, SameSizeCaptureResumesAfterFlushAcrossContextRecreation)
{
  for (unsigned int cycle = 0; cycle < 3; ++cycle)
  {
    ASSERT_TRUE(m_pool->BeginClientFrame());
    auto client = GetClient(4, 4);
    ASSERT_NE(client, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, client->GetCurrentFramebuffer());
    glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    auto first = Capture(client.get(), 4, 4);
    ASSERT_NE(first, nullptr);
    first.reset();
    m_pool->Flush();
    EXPECT_FALSE(m_pool->IsConfigured());

    auto second = Capture(client.get(), 4, 4);
    ASSERT_NE(second, nullptr);
    EXPECT_TRUE(m_pool->IsConfigured());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, second->GetCurrentFramebuffer());
    std::array<unsigned char, 4> pixel{};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_EQ(pixel, (std::array<unsigned char, 4>{0, 255, 0, 255}));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    second.reset();
    client.reset();
    m_pool->DestroyContext();
    EXPECT_EQ([NSOpenGLContext currentContext], m_guiContext);
    EXPECT_FALSE(m_pool->BeginClientFrame());

    ASSERT_TRUE(m_pool->CreateContext({}));
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
  }
}

class TestRenderBufferPoolFBOAttachmentsOSX
  : public TestRenderBufferPoolFBOOSX,
    public testing::WithParamInterface<std::array<bool, 2>>
{
};

TEST_P(TestRenderBufferPoolFBOAttachmentsOSX,
       CaptureRemainsColorOnlyAcrossReuseFlushAndContextRecreation)
{
  const auto expectAttachments = [](CRenderBufferFBO& buffer, bool depth, bool stencil)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(buffer.GetCurrentFramebuffer()));
    EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    GLint attachment = GL_NONE;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachment);
    EXPECT_EQ(attachment, GL_TEXTURE);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachment);
    EXPECT_EQ(attachment, depth ? GL_RENDERBUFFER : GL_NONE);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachment);
    EXPECT_EQ(attachment, stencil ? GL_RENDERBUFFER : GL_NONE);
  };

  HwContextProperties properties;
  properties.depth = GetParam()[0];
  properties.stencil = GetParam()[1];
  for (unsigned int cycle = 0; cycle < 2; ++cycle)
  {
    SCOPED_TRACE(cycle);
    m_pool->DestroyContext();
    ASSERT_TRUE(m_pool->CreateContext(properties));
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
    ASSERT_TRUE(m_pool->BeginClientFrame());
    auto client = GetClient(4, 4);
    ASSERT_NE(client, nullptr);
    const auto clientFramebuffer = client->GetCurrentFramebuffer();
    const auto clientTexture = client->TextureID();
    uintptr_t captureFramebuffer = 0;
    for (unsigned int frame = 0; frame < 3; ++frame)
    {
      SCOPED_TRACE(frame);
      if (frame == 2)
      {
        m_pool->Flush();
        EXPECT_FALSE(m_pool->IsConfigured());
      }
      expectAttachments(*client, properties.depth, properties.stencil);
      glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      auto captured = Capture(client.get(), 4, 4);
      ASSERT_NE(captured, nullptr);
      EXPECT_TRUE(m_pool->IsConfigured());
      expectAttachments(*captured, false, false);
      std::array<unsigned char, 4> pixel{};
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
      EXPECT_EQ(pixel, (std::array<unsigned char, 4>{255, 0, 0, 255}));
      if (frame == 1)
        EXPECT_EQ(captured->GetCurrentFramebuffer(), captureFramebuffer);
      captureFramebuffer = captured->GetCurrentFramebuffer();
      EXPECT_EQ(client->GetCurrentFramebuffer(), clientFramebuffer);
      EXPECT_EQ(client->TextureID(), clientTexture);
      EXPECT_EQ(client->TextureWidth(), 4);
      EXPECT_EQ(client->TextureHeight(), 4);
      EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
    m_pool->EndClientFrame();
  }
}

INSTANTIATE_TEST_SUITE_P(RequestedAttachments,
                         TestRenderBufferPoolFBOAttachmentsOSX,
                         testing::Values(std::array{false, false},
                                         std::array{true, false},
                                         std::array{true, true}));

TEST_F(TestRenderBufferPoolFBOOSX, TeardownPreservesGuiContainerObjectsAndBindings)
{
  GLuint guiVertexArray = 0;
  GLuint guiFramebuffer = 0;
  glGenVertexArrays(1, &guiVertexArray);
  glGenFramebuffers(1, &guiFramebuffer);
  glBindVertexArray(guiVertexArray);
  glBindFramebuffer(GL_FRAMEBUFFER, guiFramebuffer);

  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  m_pool->DestroyContext();
  EXPECT_EQ(client->GetCurrentFramebuffer(), 0);
  EXPECT_EQ(client->TextureID(), 0);
  ASSERT_EQ([NSOpenGLContext currentContext], m_guiContext);
  EXPECT_TRUE(glIsVertexArray(guiVertexArray));
  EXPECT_TRUE(glIsFramebuffer(guiFramebuffer));
  GLint binding = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &binding);
  EXPECT_EQ(binding, guiVertexArray);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &binding);
  EXPECT_EQ(binding, guiFramebuffer);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
  glDeleteFramebuffers(1, &guiFramebuffer);
  glDeleteVertexArrays(1, &guiVertexArray);
}

namespace
{
class CTestGLWindow : public CWinSystemBase, public CRenderSystemGL
{
public:
  CTestGLWindow() : m_previous(CServiceBroker::GetWinSystem())
  {
    CServiceBroker::RegisterWinSystem(this);
    glMatrixProject.Push();
    glMatrixModview.Push();
  }
  ~CTestGLWindow() override
  {
    DestroyRenderSystem();
    glMatrixProject.Pop();
    glMatrixModview.Pop();
    CServiceBroker::RegisterWinSystem(m_previous);
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

class CDirectionalTestPreset : public KODI::SHADER::CShaderPresetGL
{
public:
  explicit CDirectionalTestPreset(CRenderContext& context, bool sRGBPass) : CShaderPresetGL(context)
  {
    m_presetPath = "directional-alpha-test";
    KODI::SHADER::ShaderPass pass;
    pass.sourcePath = "directional-alpha-test.glsl";
    pass.filterType = KODI::SHADER::FilterType::NEAREST;
    pass.vertexSource = R"(#version 150
#ifdef VERTEX
in vec4 VertexCoord;
in vec2 TexCoord;
uniform mat4 MVPMatrix;
out vec2 texcoord;
void main() { gl_Position = MVPMatrix * VertexCoord; texcoord = TexCoord; }
#endif
#ifdef FRAGMENT
uniform sampler2D Texture;
in vec2 texcoord;
out vec4 color;
void main()
{
  vec3 rgb = texture(Texture, texcoord).rgb;
  rgb.b = texcoord.y < 0.5 ? 0.5 : 0.0;
  color = vec4(rgb, 0.0);
}
#endif
)";
    if (sRGBPass)
    {
      auto intermediate = pass;
      intermediate.fbo.sRgbFramebuffer = true;
      m_passes.push_back(std::move(intermediate));
    }
    m_passes.push_back(std::move(pass));
    EXPECT_TRUE(CreateShaders());
  }

  bool RenderUpdate(KODI::SHADER::IShaderTexture& source,
                    KODI::SHADER::IShaderTexture& target) override
  {
    ++calls;
    sourceID = static_cast<KODI::SHADER::CShaderTextureGLRef&>(source).GetTextureID();
    rendered = CShaderPresetGL::RenderUpdate(source, target);
    return rendered;
  }

  void FailNextUpdate()
  {
    m_fail = true;
    m_bPresetNeedsUpdate = true;
  }

  GLuint sourceID{0};
  unsigned int calls{0};
  bool rendered{false};

protected:
  bool CreateShaderTextures() override
  {
    return !m_fail && CShaderPresetGL::CreateShaderTextures();
  }

private:
  bool m_fail{false};
};

class CRecordingTestPreset : public KODI::SHADER::CShaderPresetGL
{
public:
  explicit CRecordingTestPreset(CRenderContext& context) : CShaderPresetGL(context)
  {
    m_passes.emplace_back();
  }

  bool RenderUpdate(KODI::SHADER::IShaderTexture& source,
                    KODI::SHADER::IShaderTexture& target) override
  {
    sourceTexture = &source;
    targetTexture = &target;
    ++calls;
    return true;
  }

  KODI::SHADER::IShaderTexture* sourceTexture{nullptr};
  KODI::SHADER::IShaderTexture* targetTexture{nullptr};
  unsigned int calls{0};
};

class CTestFBORenderer : public CRPRendererFBO
{
public:
  using CRPRendererFBO::CRPRendererFBO;

  std::vector<GLuint> VertexArrays() const { return {m_mainVAO, m_blackbarsVAO}; }

  CDirectionalTestPreset* UseDirectionalPreset(bool sRGBPass = false)
  {
    auto preset = std::make_unique<CDirectionalTestPreset>(m_context, sRGBPass);
    auto* result = preset.get();
    m_shaderPreset = std::move(preset);
    m_bShadersNeedUpdate = false;
    m_bUseShaderPreset = true;
    return result;
  }

  void UsePreset(std::unique_ptr<KODI::SHADER::IShaderPreset> preset)
  {
    m_shaderPreset = std::move(preset);
    m_bShadersNeedUpdate = false;
    m_bUseShaderPreset = true;
  }

  size_t CachedTextureCount() const { return m_RBTexturesMap.size(); }
  CSize TargetSize() const { return {m_fullDestWidth, m_fullDestHeight}; }

  void Draw(CRenderBufferFBO* buffer, const CRect& crop, bool clear = false, uint8_t alpha = 128)
  {
    SetBuffer(buffer);
    m_crop = crop;
    RenderFrame(clear, alpha);
  }

protected:
  void RenderInternal(bool clear, uint8_t alpha) override
  {
    m_sourceRect = m_crop;
    m_shaderPreset->SetVideoSize(m_renderBuffer->GetWidth(), m_renderBuffer->GetHeight());
    CRPRendererFBO::RenderInternal(clear, alpha);
  }

private:
  CRect m_crop;
};
} // namespace

TEST_F(TestRenderBufferPoolFBOOSX, ShaderTextureCacheTracksBuffersAllocationsAndDestinationSize)
{
  CTestGLWindow window;
  ASSERT_TRUE(window.InitRenderSystem());
  ASSERT_TRUE(window.ResetRenderSystem(8, 8));
  CRenderContext context(&window, &window, window.GetGfxContext(), CDisplaySettings::GetInstance(),
                         CMediaSettings::GetInstance(), CServiceBroker::GetGameServices(),
                         CServiceBroker::GetGUI());
  context.SetViewWindow(0, 0, 8, 8);
  KODI::SHADER::CShaderTextureGL output(8, 8, GL_UNSIGNED_BYTE, GL_RGBA8, GL_RGBA, true);
  output.CreateTexture();
  CTestFBORenderer renderer({}, context, m_pool);
  ASSERT_TRUE(renderer.Configure(AV_PIX_FMT_NONE));
  auto preset = std::make_unique<CRecordingTestPreset>(context);
  auto* recording = preset.get();
  renderer.UsePreset(std::move(preset));
  ASSERT_TRUE(output.BindFBO());

  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(16, 16);
  ASSERT_NE(client, nullptr);
  auto first = Capture(client.get(), 16, 16);
  auto second = Capture(client.get(), 16, 16);
  m_pool->EndClientFrame();
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  first->SetSize(4, 4);
  second->SetSize(4, 4);
  renderer.Draw(first.get(), {0, 0, 4, 4});
  ASSERT_EQ(renderer.CachedTextureCount(), 1u);
  EXPECT_EQ(recording->sourceTexture->GetWidth(), 16);
  EXPECT_EQ(recording->sourceTexture->GetHeight(), 16);
  auto* firstSource = recording->sourceTexture;
  auto* firstTarget = recording->targetTexture;
  EXPECT_EQ(firstTarget->GetWidth(), renderer.TargetSize().Width());
  EXPECT_EQ(firstTarget->GetHeight(), renderer.TargetSize().Height());

  renderer.Draw(second.get(), {0, 0, 4, 4});
  EXPECT_EQ(renderer.CachedTextureCount(), 2u);
  EXPECT_NE(recording->sourceTexture, firstSource);
  EXPECT_NE(recording->targetTexture, firstTarget);
  renderer.Draw(first.get(), {0, 0, 4, 4});
  EXPECT_EQ(recording->sourceTexture, firstSource);
  EXPECT_EQ(recording->targetTexture, firstTarget);

  first->SetSize(8, 8);
  renderer.Draw(first.get(), {0, 0, 8, 8});
  EXPECT_EQ(recording->sourceTexture, firstSource);
  EXPECT_EQ(recording->targetTexture, firstTarget);
  EXPECT_EQ(recording->sourceTexture->GetWidth(), 16);

  ASSERT_TRUE(m_pool->BeginClientFrame());
  first->PrepareForCapture();
  ASSERT_TRUE(first->Allocate(AV_PIX_FMT_NONE, 32, 16));
  ASSERT_TRUE(first->SetReady());
  m_pool->EndClientFrame();
  first->SetSize(8, 8);
  renderer.Draw(first.get(), {0, 0, 8, 8});
  EXPECT_EQ(recording->sourceTexture->GetWidth(), 32);
  EXPECT_EQ(recording->sourceTexture->GetHeight(), 16);
  EXPECT_EQ(
      static_cast<KODI::SHADER::CShaderTextureGLRef*>(recording->sourceTexture)->GetTextureID(),
      first->TextureID());

  context.SetViewWindow(0, 0, 4, 4);
  renderer.Draw(first.get(), {0, 0, 8, 8});
  EXPECT_EQ(renderer.CachedTextureCount(), 2u);
  EXPECT_EQ(recording->targetTexture->GetWidth(), 8);
  EXPECT_EQ(recording->targetTexture->GetHeight(), 8);

  context.SetViewPort({0, 0, 4, 4});
  renderer.Draw(first.get(), {0, 0, 8, 8});
  EXPECT_EQ(renderer.CachedTextureCount(), 1u);
  EXPECT_EQ(recording->targetTexture->GetWidth(), 4);
  EXPECT_EQ(recording->targetTexture->GetHeight(), 4);
  renderer.Flush();
  EXPECT_EQ(renderer.CachedTextureCount(), 0u);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestRenderBufferPoolFBOOSX, FailedShaderPresentsCaptureAndDisablesPreset)
{
  CTestGLWindow window;
  ASSERT_TRUE(window.InitRenderSystem());
  ASSERT_TRUE(window.ResetRenderSystem(8, 8));
  CRenderContext context(&window, &window, window.GetGfxContext(), CDisplaySettings::GetInstance(),
                         CMediaSettings::GetInstance(), CServiceBroker::GetGameServices(),
                         CServiceBroker::GetGUI());
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho2D(0, 8, 8, 0);
  glMatrixModview->LoadIdentity();
  KODI::SHADER::CShaderTextureGL output(8, 8, GL_UNSIGNED_BYTE, GL_RGBA8, GL_RGBA, true);
  output.CreateTexture();
  ASSERT_TRUE(output.BindFBO());
  GLint framebuffer = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);

  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, client->GetCurrentFramebuffer());
  glClearColor(1, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  auto red = Capture(client.get(), 4, 4);
  glClearColor(0, 1, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  auto green = Capture(client.get(), 4, 4);
  m_pool->EndClientFrame();
  ASSERT_NE(red, nullptr);
  ASSERT_NE(green, nullptr);

  for (unsigned int stage : {0u, 1u, 2u})
  {
    SCOPED_TRACE(testing::Message() << "failure stage=" << stage);
    context.SetViewPort({0, 0, 8, 8});
    context.SetViewWindow(2, 2, 6, 6);
    CTestFBORenderer renderer({}, context, m_pool);
    ASSERT_TRUE(renderer.Configure(AV_PIX_FMT_NONE));
    auto* preset = renderer.UseDirectionalPreset();
    if (stage != 0)
    {
      ASSERT_TRUE(output.BindFBO());
      renderer.Draw(red.get(), {0, 0, 4, 4}, false, 255);
      ASSERT_TRUE(preset->rendered);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
      std::array<unsigned char, 4> pixel{};
      glReadPixels(4, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
      EXPECT_EQ(pixel, (std::array<unsigned char, 4>{255, 0, 128, 255}));
    }
    if (stage == 2)
      context.SetViewPort({0, 0, 4, 4});
    preset->FailNextUpdate();

    for (auto* capture : {red.get(), green.get()})
    {
      ASSERT_TRUE(output.BindFBO());
      glDisable(GL_SCISSOR_TEST);
      glClearColor(0, 0, 1, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      glScissor(3, 3, 2, 2);
      glEnable(GL_SCISSOR_TEST);
      renderer.Draw(capture, {0, 0, 4, 4}, false, 255);
      EXPECT_FALSE(preset->rendered);
      EXPECT_EQ(preset->calls, stage == 0 ? 1u : 2u);

      glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
      std::array<unsigned char, 4> inside{}, outside{};
      const int x = stage == 2 ? 2 : 4;
      const int y = stage == 2 ? 6 : 4;
      glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, inside.data());
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outside.data());
      EXPECT_EQ(inside, (capture == red.get() ? std::array<unsigned char, 4>{255, 0, 0, 255}
                                              : std::array<unsigned char, 4>{0, 255, 0, 255}));
      EXPECT_EQ(outside, (std::array<unsigned char, 4>{0, 0, 255, 255}));
      CGUITextureGL::DrawQuad({0, 0, 8, 8}, 0xFFFFFFFF);
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, inside.data());
      EXPECT_EQ(inside, outside);
      EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
  }
}

TEST_F(TestRenderBufferPoolFBOOSX, ShaderPresentationUsesGuiTargetAcrossAllocationReuseAndResize)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 4);
  ASSERT_NE(client, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, client->GetCurrentFramebuffer());
  glClearColor(0.25f, 0.125f, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  auto captured = Capture(client.get(), 4, 4);
  m_pool->EndClientFrame();
  ASSERT_NE(captured, nullptr);

  CTestGLWindow window;
  ASSERT_TRUE(window.InitRenderSystem());
  ASSERT_TRUE(window.ResetRenderSystem(8, 8));
  CRenderContext context(&window, &window, window.GetGfxContext(), CDisplaySettings::GetInstance(),
                         CMediaSettings::GetInstance(), CServiceBroker::GetGameServices(),
                         CServiceBroker::GetGUI());
  glMatrixModview->LoadIdentity();

  for (bool sRGBTarget : {false, true})
  {
    KODI::SHADER::CShaderTextureGL output(8, 8, GL_UNSIGNED_BYTE,
                                          sRGBTarget ? GL_SRGB8_ALPHA8 : GL_RGBA8, GL_RGBA, true);
    output.CreateTexture();
    for (bool sRGBEnabled : {false, true})
    {
      CTestFBORenderer renderer({}, context, m_pool);
      ASSERT_TRUE(renderer.Configure(AV_PIX_FMT_NONE));
      auto* preset = renderer.UseDirectionalPreset(true);
      unsigned int frame = 0;
      for (int size : {8, 8, 4})
      {
        SCOPED_TRACE(testing::Message() << "sRGB target=" << sRGBTarget
                                        << " conversion=" << sRGBEnabled << " frame=" << frame++);
        context.SetViewPort({0, 0, static_cast<float>(size), static_cast<float>(size)});
        context.SetViewWindow(size / 4, size / 4, 3 * size / 4, 3 * size / 4);
        glMatrixProject->LoadIdentity();
        glMatrixProject->Ortho2D(0, size, size, 0);
        ASSERT_TRUE(output.BindFBO());
        GLint framebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glScissor(0, 8 - size, size / 4, size / 4);
        glEnable(GL_SCISSOR_TEST);
        if (sRGBEnabled)
          glEnable(GL_FRAMEBUFFER_SRGB);
        else
          glDisable(GL_FRAMEBUFFER_SRGB);

        renderer.Draw(captured.get(), {0, 0, 4, 4}, false, 255);
        ASSERT_TRUE(preset->rendered);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        std::array<unsigned char, 4> inside{}, outside{};
        glReadPixels(size / 2, 8 - size / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, inside.data());
        glReadPixels(0, 8 - size, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outside.data());
        const bool encoded = sRGBTarget && sRGBEnabled;
        EXPECT_NEAR(inside[0], encoded ? 137 : 64, 1);
        EXPECT_NEAR(inside[1], encoded ? 99 : 32, 1);
        EXPECT_NEAR(inside[2], encoded ? 188 : 128, 1);
        EXPECT_EQ(inside[3], 255);
        EXPECT_EQ(outside, (std::array<unsigned char, 4>{0, 0, 255, 255}));

        // PreRender disables scissoring for the game; PostRender enables it for GUI drawing.
        CGUITextureGL::DrawQuad({0, 0, static_cast<float>(size), static_cast<float>(size)},
                                0xFF00FF00);
        std::array<unsigned char, 4> after{};
        glReadPixels(0, 8 - size, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, after.data());
        EXPECT_EQ(after, (std::array<unsigned char, 4>{0, 255, 0, 255}));
        glReadPixels(size / 2, 8 - size / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, after.data());
        EXPECT_EQ(after, inside);
        EXPECT_EQ(glGetError(), GL_NO_ERROR);
      }
    }
  }
}

TEST_F(TestRenderBufferPoolFBOOSX, RendererOwnsInitializedVertexArraysUntilDestruction)
{
  CTestGLWindow window;
  ASSERT_TRUE(window.InitRenderSystem());
  ASSERT_TRUE(window.ResetRenderSystem(8, 8));
  CRenderContext context(&window, &window, window.GetGfxContext(), CDisplaySettings::GetInstance(),
                         CMediaSettings::GetInstance(), CServiceBroker::GetGameServices(),
                         CServiceBroker::GetGUI());
  GLint guiVertexArray = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &guiVertexArray);

  for (unsigned int cycle = 0; cycle < 20; ++cycle)
  {
    SCOPED_TRACE(cycle);
    std::vector<GLuint> arrays;
    std::vector<GLuint> buffers;
    {
      CTestFBORenderer renderer({}, context, m_pool);
      arrays = renderer.VertexArrays();
      ASSERT_EQ(arrays.size(), 2u);
      EXPECT_NE(arrays[0], arrays[1]);
      GLint binding = 0;
      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &binding);
      EXPECT_EQ(binding, guiVertexArray);
      for (size_t i = 0; i < arrays.size(); ++i)
      {
        ASSERT_TRUE(glIsVertexArray(arrays[i]));
        glBindVertexArray(arrays[i]);
        GLint count = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &count);
        unsigned int enabledCount = 0;
        for (GLint attribute = 0; attribute < count; ++attribute)
        {
          GLint enabled = 0;
          glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
          if (!enabled)
            continue;
          ++enabledCount;
          glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding);
          EXPECT_TRUE(glIsBuffer(binding));
          buffers.push_back(binding);
        }
        EXPECT_EQ(enabledCount, i == 0 ? 2u : 1u);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &binding);
        if (i == 0)
        {
          EXPECT_TRUE(glIsBuffer(binding));
          buffers.push_back(binding);
          std::array<GLubyte, 4> indices{};
          glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size(), indices.data());
          EXPECT_EQ(indices, (std::array<GLubyte, 4>{0, 1, 3, 2}));
        }
        else
          EXPECT_EQ(binding, 0);
      }
      glBindVertexArray(guiVertexArray);
      std::sort(buffers.begin(), buffers.end());
      buffers.erase(std::unique(buffers.begin(), buffers.end()), buffers.end());
      EXPECT_EQ(buffers.size(), 3u);
    }
    for (const auto array : arrays)
      EXPECT_FALSE(glIsVertexArray(array));
    for (const auto buffer : buffers)
      EXPECT_FALSE(glIsBuffer(buffer));
    EXPECT_TRUE(glIsVertexArray(guiVertexArray));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
  }
}

TEST_F(TestRenderBufferPoolFBOOSX, FilteredAndUnfilteredControlsPreserveOrientationCropAndAlpha)
{
  CTestGLWindow window;
  ASSERT_TRUE(window.InitRenderSystem());
  ASSERT_TRUE(window.ResetRenderSystem(8, 8));
  CRenderContext context(&window, &window, window.GetGfxContext(), CDisplaySettings::GetInstance(),
                         CMediaSettings::GetInstance(), CServiceBroker::GetGameServices(),
                         CServiceBroker::GetGUI());
  context.SetViewWindow(0, 0, 8, 8);
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho2D(0, 8, 8, 0);
  glMatrixModview->LoadIdentity();
  KODI::SHADER::CShaderTextureGL output(8, 8, GL_UNSIGNED_BYTE, GL_RGBA8, GL_RGBA, true);
  output.CreateTexture();
  KODI::SHADER::CShaderTextureGL readback(1, 1, GL_UNSIGNED_BYTE, GL_RGBA8, GL_RGBA, true);
  readback.CreateTexture();
  ASSERT_TRUE(readback.BindFBO());
  GLint readFramebuffer = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
  readback.UnbindFBO();

  for (bool bottomLeft : {false, true})
  {
    m_pool->DestroyContext();
    HwContextProperties properties;
    properties.bottomLeftOrigin = bottomLeft;
    ASSERT_TRUE(m_pool->CreateContext(properties));
    ASSERT_TRUE(m_pool->Configure(AV_PIX_FMT_NONE));
    ASSERT_TRUE(m_pool->BeginClientFrame());
    auto client = GetClient(16, 16);
    ASSERT_NE(client, nullptr);
    for (unsigned int height : {4u, 8u})
    {
      glBindFramebuffer(GL_FRAMEBUFFER, client->GetCurrentFramebuffer());
      glDisable(GL_SCISSOR_TEST);
      glClearColor(0, 0, 1, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glEnable(GL_SCISSOR_TEST);
      glScissor(0, bottomLeft ? height / 2 : 0, 4, height / 2);
      glClearColor(1, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glScissor(0, bottomLeft ? 0 : height / 2, 4, height / 2);
      glClearColor(0, 1, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      auto captured = Capture(client.get(), 4, height);
      ASSERT_NE(captured, nullptr);
      m_pool->EndClientFrame();

      for (unsigned int passes : {0u, 1u, 2u})
      {
        const bool filtered = passes != 0;
        CTestFBORenderer renderer({}, context, m_pool);
        ASSERT_TRUE(renderer.Configure(AV_PIX_FMT_NONE));
        auto* preset = filtered ? renderer.UseDirectionalPreset(passes == 2) : nullptr;
        for (unsigned int rotation : {0u, 90u, 180u, 270u})
        {
          for (bool cropped : {false, true})
          {
            SCOPED_TRACE(testing::Message() << "bottomLeft=" << bottomLeft << " height=" << height
                                            << " passes=" << passes << " rotation=" << rotation
                                            << " cropped=" << cropped);
            ASSERT_TRUE(output.BindFBO());
            GLint framebuffer = 0;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_DEPTH_TEST);
            glViewport(0, 0, 8, 8);
            glClearColor(0, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
            glScissor(1, 2, 3, 4);
            glEnable(GL_SCISSOR_TEST);
            const float inset = cropped ? height / 4.0f : 0.0f;
            captured->SetRotation(rotation);
            captured->SetDisplayAspectRatio(1.0f);
            renderer.Draw(captured.get(), {0, inset, 4, height - inset});
            glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
            if (preset)
            {
              EXPECT_TRUE(preset->rendered);
              EXPECT_EQ(preset->sourceID, captured->TextureID());
            }
            for (int y : {1, 6})
            {
              for (int x : {1, 6})
              {
                const bool top = rotation == 0     ? y > 4
                                 : rotation == 90  ? x < 4
                                 : rotation == 180 ? y < 4
                                                   : x > 4;
                std::array<unsigned char, 4> pixel{};
                glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
                EXPECT_NEAR(pixel[0], top ? 128 : 0, 1);
                EXPECT_NEAR(pixel[1], top ? 0 : 128, 1);
                EXPECT_NEAR(pixel[2], filtered && top ? 191 : 127, 1);
                EXPECT_NEAR(pixel[3], 191, 1);
              }
            }
            std::array<unsigned char, 4> before{}, after{}, outsideBefore{}, outsideAfter{};
            glReadPixels(2, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, before.data());
            glReadPixels(6, 6, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outsideBefore.data());
            CGUITextureGL::DrawQuad({0, 0, 8, 8}, 0x8000FF00);
            glReadPixels(2, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, after.data());
            glReadPixels(6, 6, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outsideAfter.data());
            EXPECT_EQ(outsideAfter, outsideBefore);
            EXPECT_NEAR(after[0], before[0] * 127.0f / 255.0f, 1);
            EXPECT_NEAR(after[1], 128 + before[1] * 127.0f / 255.0f, 1);
            EXPECT_NEAR(after[2], before[2] * 127.0f / 255.0f, 1);
            EXPECT_EQ(glGetError(), GL_NO_ERROR);
          }
        }
      }
      ASSERT_TRUE(m_pool->BeginClientFrame());
    }
    m_pool->EndClientFrame();
  }
}

TEST_F(TestRenderBufferPoolFBOOSX, OpaqueFramesClearBlackBarsAndAllowSubsequentGuiDrawing)
{
  ASSERT_TRUE(m_pool->BeginClientFrame());
  auto client = GetClient(4, 2);
  ASSERT_NE(client, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(client->GetCurrentFramebuffer()));
  glClearColor(1, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  auto captured = Capture(client.get(), 4, 2);
  ASSERT_NE(captured, nullptr);
  m_pool->EndClientFrame();

  CTestGLWindow window;
  ASSERT_TRUE(window.InitRenderSystem());
  ASSERT_TRUE(window.ResetRenderSystem(8, 8));
  CRenderContext context(&window, &window, window.GetGfxContext(), CDisplaySettings::GetInstance(),
                         CMediaSettings::GetInstance(), CServiceBroker::GetGameServices(),
                         CServiceBroker::GetGUI());
  context.SetViewWindow(0, 0, 8, 8);
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho2D(0, 8, 8, 0);
  glMatrixModview->LoadIdentity();
  KODI::SHADER::CShaderTextureGL output(8, 8, GL_UNSIGNED_BYTE, GL_RGBA8, GL_RGBA, true);
  output.CreateTexture();

  for (bool filtered : {false, true})
  {
    CTestFBORenderer renderer({}, context, m_pool);
    ASSERT_TRUE(renderer.Configure(AV_PIX_FMT_NONE));
    if (filtered)
      renderer.UseDirectionalPreset();
    for (unsigned int rotation : {0u, 90u, 180u, 270u})
    {
      SCOPED_TRACE(testing::Message() << "filtered=" << filtered << " rotation=" << rotation);
      ASSERT_TRUE(output.BindFBO());
      GLint framebuffer = 0;
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
      glDisable(GL_DEPTH_TEST);
      glScissor(0, 0, 8, 8);
      captured->SetRotation(rotation);
      renderer.Draw(captured.get(), {0, 0, 4, 2}, true, 255);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
      for (int y : {0, 7})
      {
        std::array<unsigned char, 4> pixel{};
        glReadPixels(0, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        EXPECT_EQ(pixel, (std::array<unsigned char, 4>{0, 0, 0, 255}));
      }
      std::array<unsigned char, 4> pixel{};
      glReadPixels(4, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
      EXPECT_EQ(pixel[0], 255);
      EXPECT_EQ(pixel[3], 255);

      CGUITextureGL::DrawQuad({0, 0, 8, 8}, 0xFF00FF00);
      glReadPixels(4, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
      EXPECT_EQ(pixel, (std::array<unsigned char, 4>{0, 255, 0, 255}));
      EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
  }
}
