/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(HAS_EGL) && HAS_GLES == 3

#include "cores/RetroPlayer/buffers/RenderBufferOpenGLES.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolOpenGLES.h"

#include <array>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <EGL/egl.h>

using namespace KODI::RETRO;

namespace
{
class TestRenderBufferOpenGLES : public testing::Test
{
protected:
  static constexpr unsigned int WIDTH = 376;
  static constexpr unsigned int HEIGHT = 464;

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
    const EGLint attributes[]{EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT, EGL_NONE};
    EGLConfig config{};
    EGLint count = 0;
    ASSERT_EQ(eglChooseConfig(m_display, attributes, &config, 1, &count), EGL_TRUE);
    if (count == 0)
      GTEST_SKIP() << "EGL pbuffer unavailable";
    const EGLint contextAttributes[]{EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
    if (m_context == EGL_NO_CONTEXT)
      GTEST_SKIP() << "OpenGL ES 3 context unavailable";
    const EGLint surfaceAttributes[]{EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    m_surface = eglCreatePbufferSurface(m_display, config, surfaceAttributes);
    ASSERT_NE(m_surface, EGL_NO_SURFACE);
    ASSERT_EQ(eglMakeCurrent(m_display, m_surface, m_surface, m_context), EGL_TRUE);

    const char* vertexSource =
        "attribute vec2 position; varying vec2 uv;"
        "void main() { gl_Position = vec4(position, 0.0, 1.0); uv = (position + 1.0) * 0.5; }";
    const char* fragmentSource =
        "precision mediump float; uniform sampler2D source; uniform float alpha; varying vec2 uv;"
        "void main() { gl_FragColor = texture2D(source, uv) * vec4(1.0, 1.0, 1.0, alpha); }";
    m_program = glCreateProgram();
    for (auto [type, source] :
         {std::pair{GL_VERTEX_SHADER, vertexSource}, std::pair{GL_FRAGMENT_SHADER, fragmentSource}})
    {
      GLuint shader = glCreateShader(type);
      glShaderSource(shader, 1, &source, nullptr);
      glCompileShader(shader);
      GLint compiled = GL_FALSE;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
      EXPECT_EQ(compiled, GL_TRUE);
      glAttachShader(m_program, shader);
      glDeleteShader(shader);
    }
    glBindAttribLocation(m_program, 0, "position");
    glLinkProgram(m_program);
    GLint linked = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    ASSERT_EQ(linked, GL_TRUE);
    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "source"), 0);

    constexpr float vertices[]{-1, -1, 1, -1, -1, 1, 1, 1};
    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenTextures(1, &m_target);
    glBindTexture(GL_TEXTURE_2D, m_target);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_target, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    glViewport(0, 0, WIDTH, HEIGHT);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
  }

  void TearDown() override
  {
    if (m_context != EGL_NO_CONTEXT && eglGetCurrentContext() == m_context)
    {
      glDeleteFramebuffers(1, &m_framebuffer);
      glDeleteTextures(1, &m_target);
      glDeleteBuffers(1, &m_vertices);
      glDeleteProgram(m_program);
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

  void Draw(CRenderBufferOpenGLES& buffer, float alpha = 1.0f)
  {
    ASSERT_TRUE(buffer.UploadTexture());
    glBindTexture(GL_TEXTURE_2D, buffer.TextureID());
    glUniform1f(glGetUniformLocation(m_program, "alpha"), alpha);
    glDisable(GL_BLEND);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (alpha < 1.0f)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
  }

  std::array<uint8_t, 4> Pixel(unsigned int x, unsigned int y)
  {
    std::array<uint8_t, 4> pixel{};
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    return pixel;
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
  GLuint m_program{0};
  GLuint m_vertices{0};
  GLuint m_target{0};
  GLuint m_framebuffer{0};
};
} // namespace

TEST_F(TestRenderBufferOpenGLES, XRGBPixelsRemainOpaqueAndUnmodifiedAcrossUploads)
{
  auto pool = std::make_shared<CRenderBufferPoolOpenGLES>(true);
  ASSERT_TRUE(pool->Configure(AV_PIX_FMT_0RGB32));
  auto* buffer = static_cast<CRenderBufferOpenGLES*>(pool->GetBuffer(WIDTH, HEIGHT));
  ASSERT_NE(buffer, nullptr);
  std::vector<uint32_t> pixels(WIDTH * HEIGHT, 0x00123456);
  pixels.front() = 0x00ff0000;
  pixels[WIDTH - 1] = 0x0000ff00;
  pixels[WIDTH * (HEIGHT - 1)] = 0x000000ff;
  pixels.back() = 0x00ffffff;
  std::memcpy(buffer->GetMemory(), pixels.data(), buffer->GetFrameSize());

  for (int upload = 0; upload < 2; ++upload)
  {
    Draw(*buffer);
    EXPECT_EQ(Pixel(0, 0), (std::array<uint8_t, 4>{255, 0, 0, 255}));
    EXPECT_EQ(Pixel(WIDTH - 1, 0), (std::array<uint8_t, 4>{0, 255, 0, 255}));
    EXPECT_EQ(Pixel(0, HEIGHT - 1), (std::array<uint8_t, 4>{0, 0, 255, 255}));
    EXPECT_EQ(Pixel(WIDTH - 1, HEIGHT - 1), (std::array<uint8_t, 4>{255, 255, 255, 255}));
    EXPECT_EQ(Pixel(WIDTH / 2, HEIGHT / 2), (std::array<uint8_t, 4>{18, 52, 86, 255}));
    EXPECT_EQ(std::memcmp(buffer->GetMemory(), pixels.data(), buffer->GetFrameSize()), 0);
  }

  Draw(*buffer, 0.5f);
  const auto blended = Pixel(0, 0);
  EXPECT_NEAR(blended[0], 128, 1);
  EXPECT_EQ(blended[1], 0);
  EXPECT_EQ(blended[2], 0);
  buffer->Release();
}

TEST_F(TestRenderBufferOpenGLES, RGB565PixelsRetainTheirChannels)
{
  auto pool = std::make_shared<CRenderBufferPoolOpenGLES>(true);
  ASSERT_TRUE(pool->Configure(AV_PIX_FMT_RGB565));
  auto* buffer = static_cast<CRenderBufferOpenGLES*>(pool->GetBuffer(WIDTH, HEIGHT));
  ASSERT_NE(buffer, nullptr);
  std::vector<uint16_t> pixels(WIDTH * HEIGHT, 0xf800);
  pixels[WIDTH - 1] = 0x07e0;
  pixels[WIDTH * (HEIGHT - 1)] = 0x001f;
  pixels.back() = 0xffff;
  std::memcpy(buffer->GetMemory(), pixels.data(), buffer->GetFrameSize());
  Draw(*buffer);
  EXPECT_EQ(Pixel(0, 0), (std::array<uint8_t, 4>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(WIDTH - 1, 0), (std::array<uint8_t, 4>{0, 255, 0, 255}));
  EXPECT_EQ(Pixel(0, HEIGHT - 1), (std::array<uint8_t, 4>{0, 0, 255, 255}));
  EXPECT_EQ(Pixel(WIDTH - 1, HEIGHT - 1), (std::array<uint8_t, 4>{255, 255, 255, 255}));
  buffer->Release();
}

#endif
