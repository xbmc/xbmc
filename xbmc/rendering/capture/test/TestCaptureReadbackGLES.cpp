/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(HAS_EGL) && HAS_GLES == 3

#include "rendering/capture/CaptureReadback.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

#include <gtest/gtest.h>

#include "system_gl.h"

#include <EGL/egl.h>

using namespace KODI::RENDERING::CAPTURE;

namespace
{
class TestCaptureReadbackGLES : public testing::Test
{
protected:
  virtual EGLint ContextVersion() const { return 3; }

  void SetUp() override
  {
    m_previousDisplay = eglGetCurrentDisplay();
    m_previousContext = eglGetCurrentContext();
    m_previousDraw = eglGetCurrentSurface(EGL_DRAW);
    m_previousRead = eglGetCurrentSurface(EGL_READ);
    m_previousAPI = eglQueryAPI();
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
      GTEST_SKIP() << "EGL display unavailable";

    ASSERT_EQ(eglBindAPI(EGL_OPENGL_ES_API), EGL_TRUE);
    const EGLint attributes[]{EGL_RENDERABLE_TYPE,
                              ContextVersion() == 3 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT,
                              EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE};
    EGLConfig config{};
    EGLint count = 0;
    ASSERT_EQ(eglChooseConfig(m_display, attributes, &config, 1, &count), EGL_TRUE);
    if (count == 0)
      GTEST_SKIP() << "EGL pbuffer unavailable";
    const EGLint contextAttributes[]{EGL_CONTEXT_CLIENT_VERSION, ContextVersion(), EGL_NONE};
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
    if (m_context == EGL_NO_CONTEXT)
      GTEST_SKIP() << "Requested OpenGL ES context unavailable";
    const EGLint surfaceAttributes[]{EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    m_surface = eglCreatePbufferSurface(m_display, config, surfaceAttributes);
    ASSERT_NE(m_surface, EGL_NO_SURFACE);
    ASSERT_EQ(eglMakeCurrent(m_display, m_surface, m_surface, m_context), EGL_TRUE);
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  }

  void TearDown() override
  {
    if (m_context != EGL_NO_CONTEXT && eglGetCurrentContext() == m_context)
    {
      glDeleteFramebuffers(1, &m_framebuffer);
      glDeleteTextures(1, &m_texture);
    }
    if (m_display != EGL_NO_DISPLAY)
    {
      eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);
      if (m_context != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_context);
      if (m_previousDisplay == EGL_NO_DISPLAY)
        eglTerminate(m_display);
    }
    eglBindAPI(m_previousAPI);
    if (m_previousDisplay != EGL_NO_DISPLAY)
      eglMakeCurrent(m_previousDisplay, m_previousDraw, m_previousRead, m_previousContext);
  }

  void Attach(GLint format,
              GLenum type,
              const void* pixels,
              GLenum attachment = GL_COLOR_ATTACHMENT0)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, format, 1, 2, 0, GL_RGBA, type, pixels);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, m_texture, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
  }

private:
  EGLDisplay m_display{EGL_NO_DISPLAY};
  EGLContext m_context{EGL_NO_CONTEXT};
  EGLSurface m_surface{EGL_NO_SURFACE};
  EGLDisplay m_previousDisplay{EGL_NO_DISPLAY};
  EGLContext m_previousContext{EGL_NO_CONTEXT};
  EGLSurface m_previousDraw{EGL_NO_SURFACE};
  EGLSurface m_previousRead{EGL_NO_SURFACE};
  EGLenum m_previousAPI{EGL_OPENGL_ES_API};
  GLuint m_texture{0};
  GLuint m_framebuffer{0};
};

class TestCaptureReadbackGLES2 : public TestCaptureReadbackGLES
{
protected:
  EGLint ContextVersion() const override { return 2; }
};
} // namespace

TEST_F(TestCaptureReadbackGLES, FloatFramebufferPreservesSixteenBitPrecisionAndRowOrder)
{
  const auto* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  if (!extensions || (!std::strstr(extensions, "GL_EXT_color_buffer_half_float") &&
                      !std::strstr(extensions, "GL_EXT_color_buffer_float")))
    GTEST_SKIP() << "Floating-point color attachment unavailable";

  const std::array<float, 8> pixels{0.5f, 0.25f, 0.125f, 0.75f, 0.50048828125f, 1.0f, 0.0f, 0.5f};
  Attach(GL_RGBA16F, GL_FLOAT, pixels.data(), GL_COLOR_ATTACHMENT1);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  GLint readFramebuffer = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  ASSERT_EQ(glGetError(), GL_NO_ERROR);
  for (int depth : {8, 16})
  {
    for (bool flipY : {false, true})
    {
      ReadbackBuffer buffer;
      ASSERT_TRUE(ReadFramebufferRegion(0, 0, 1, 2, depth, flipY, buffer));
      ASSERT_EQ(buffer.format, AV_PIX_FMT_RGBA64LE);
      EXPECT_EQ(buffer.width, 1u);
      EXPECT_EQ(buffer.height, 2u);
      EXPECT_EQ(buffer.stride, flipY ? -8 : 8);
      std::array<uint8_t, 16> actual{};
      std::memcpy(actual.data(), buffer.pixels.get(), sizeof(actual));
      EXPECT_EQ(actual, (std::array<uint8_t, 16>{0x00, 0x80, 0x00, 0x40, 0x00, 0x20, 0xff, 0xbf,
                                                 0x1f, 0x80, 0xff, 0xff, 0x00, 0x00, 0x00, 0x80}));
      GLint state = 0;
      glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &state);
      EXPECT_EQ(state, readFramebuffer);
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state);
      EXPECT_EQ(state, 0);
      glGetIntegerv(GL_READ_BUFFER, &state);
      EXPECT_EQ(state, GL_COLOR_ATTACHMENT1);
      glGetIntegerv(GL_PACK_ALIGNMENT, &state);
      EXPECT_EQ(state, 4);
      EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
  }
}

TEST_F(TestCaptureReadbackGLES, FloatFramebufferClipsOutOfRangeAndNonFiniteSamples)
{
  const auto* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  if (!extensions || (!std::strstr(extensions, "GL_EXT_color_buffer_half_float") &&
                      !std::strstr(extensions, "GL_EXT_color_buffer_float")))
    GTEST_SKIP() << "Floating-point color attachment unavailable";

  const float infinity = std::numeric_limits<float>::infinity();
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const std::array<float, 8> pixels{-0.5f, 1.5f, infinity, -infinity, nan, 0.5f, 0.0f, nan};
  Attach(GL_RGBA16F, GL_FLOAT, pixels.data());
  ReadbackBuffer buffer;
  ASSERT_TRUE(ReadFramebufferRegion(0, 0, 1, 2, 16, false, buffer));
  ASSERT_EQ(buffer.format, AV_PIX_FMT_RGBA64LE);
  std::array<uint8_t, 16> actual{};
  std::memcpy(actual.data(), buffer.pixels.get(), sizeof(actual));
  EXPECT_EQ(actual, (std::array<uint8_t, 16>{0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00}));
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestCaptureReadbackGLES, NormalizedFramebufferKeepsEightBitPixels)
{
  const std::array<uint8_t, 8> pixels{240, 120, 60, 255, 30, 20, 10, 128};
  Attach(GL_RGBA8, GL_UNSIGNED_BYTE, pixels.data());
  ReadbackBuffer buffer;
  ASSERT_TRUE(ReadFramebufferRegion(0, 0, 1, 2, 8, false, buffer));
  EXPECT_EQ(buffer.format, AV_PIX_FMT_RGBA);
  EXPECT_EQ(buffer.stride, 4);
  EXPECT_EQ(std::memcmp(buffer.pixels.get(), pixels.data(), pixels.size()), 0);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestCaptureReadbackGLES, DefaultFramebufferKeepsEightBitPixels)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  ASSERT_EQ(glGetError(), GL_NO_ERROR);
  ReadbackBuffer buffer;
  ASSERT_TRUE(ReadFramebufferRegion(0, 0, 1, 1, 8, false, buffer));
  EXPECT_EQ(buffer.format, AV_PIX_FMT_RGBA);
  EXPECT_EQ(buffer.stride, 4);
  const std::array<uint8_t, 4> expected{255, 0, 0, 255};
  EXPECT_EQ(std::memcmp(buffer.pixels.get(), expected.data(), expected.size()), 0);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestCaptureReadbackGLES, PackedFramebufferKeepsTenBitPixels)
{
  const std::array<uint32_t, 2> pixels{0xc00003ff, 0xfff00000};
  Attach(GL_RGB10_A2, GL_UNSIGNED_INT_2_10_10_10_REV, pixels.data());
  GLint readType = 0;
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &readType);
  if (readType != GL_UNSIGNED_INT_2_10_10_10_REV)
    GTEST_SKIP() << "Packed 10-bit readback unavailable";

  ReadbackBuffer buffer;
  ASSERT_TRUE(ReadFramebufferRegion(0, 0, 1, 2, 10, false, buffer));
  EXPECT_EQ(buffer.format, AV_PIX_FMT_X2BGR10LE);
  EXPECT_EQ(buffer.stride, 4);
  EXPECT_EQ(std::memcmp(buffer.pixels.get(), pixels.data(), sizeof(pixels)), 0);
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TestCaptureReadbackGLES, FailedReadbackLeavesThePreviousBufferUntouched)
{
  ReadbackBuffer buffer;
  buffer.pixels = std::make_unique<uint8_t[]>(4);
  buffer.pixels[0] = 42;
  buffer.width = 1;
  buffer.height = 1;
  buffer.stride = 4;
  buffer.format = AV_PIX_FMT_RGBA;
  const auto* previous = buffer.pixels.get();

  const auto expectFailure = [&]
  {
    EXPECT_FALSE(ReadFramebufferRegion(0, 0, 1, 1, 8, false, buffer));
    EXPECT_EQ(buffer.pixels.get(), previous);
    EXPECT_EQ(buffer.pixels[0], 42);
    EXPECT_EQ(buffer.width, 1u);
    EXPECT_EQ(buffer.height, 1u);
    EXPECT_EQ(buffer.stride, 4);
    EXPECT_EQ(buffer.format, AV_PIX_FMT_RGBA);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
  };
  ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
  expectFailure();

  GLuint renderbuffer = 0;
  glGenRenderbuffers(1, &renderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_RGBA8, 1, 1);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
  ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
  ASSERT_EQ(glGetError(), GL_NO_ERROR);
  expectFailure();
  glDeleteRenderbuffers(1, &renderbuffer);
}

TEST_F(TestCaptureReadbackGLES2, RuntimeEs2KeepsEightBitPixels)
{
  const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  ASSERT_NE(version, nullptr);
  if (std::strncmp(version, "OpenGL ES 2.", 12) != 0)
    GTEST_SKIP() << "Driver supplied a newer context: " << version;

  const std::array<uint8_t, 8> pixels{240, 120, 60, 255, 30, 20, 10, 128};
  Attach(GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  ReadbackBuffer buffer;
  ASSERT_TRUE(ReadFramebufferRegion(0, 0, 1, 2, 8, false, buffer));
  EXPECT_EQ(buffer.format, AV_PIX_FMT_RGBA);
  EXPECT_EQ(buffer.stride, 4);
  EXPECT_EQ(std::memcmp(buffer.pixels.get(), pixels.data(), pixels.size()), 0);
}

#endif
