/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/GLBufferObject.h"

#include <cstring>

#include <gtest/gtest.h>

#include "system_gl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

using KODI::UTILS::GL::CGLBufferObject;

namespace
{
bool HasExtension(const char* extensions, const char* name)
{
  return extensions && std::strstr(extensions, name) != nullptr;
}
} // namespace

// Runs against a headless EGL context (Mesa surfaceless or a 1x1 pbuffer) and skips when no
// EGL driver is available, so it is safe on CI runners without a GPU or display.
class TestGLBufferObject : public ::testing::Test
{
protected:
  void SetUp() override
  {
#if defined(EGL_MESA_platform_surfaceless)
    if (HasExtension(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS),
                     "EGL_MESA_platform_surfaceless"))
    {
      auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
          eglGetProcAddress("eglGetPlatformDisplayEXT"));
      if (getPlatformDisplay)
        m_display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    }
#endif
    if (m_display == EGL_NO_DISPLAY)
      m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
      GTEST_SKIP() << "no EGL display available";

#if defined(HAS_GL)
    eglBindAPI(EGL_OPENGL_API);
    const EGLint renderableType = EGL_OPENGL_BIT;
#else
    eglBindAPI(EGL_OPENGL_ES_API);
    const EGLint renderableType = EGL_OPENGL_ES2_BIT;
#endif

    EGLConfig config{};
    EGLint numConfigs = 0;
    const EGLint pbufferConfigAttribs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
                                           renderableType, EGL_NONE};
    if (eglChooseConfig(m_display, pbufferConfigAttribs, &config, 1, &numConfigs) &&
        numConfigs == 1)
    {
      const EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
      m_surface = eglCreatePbufferSurface(m_display, config, pbufferAttribs);
    }
    if (m_surface == EGL_NO_SURFACE)
    {
      const EGLint configAttribs[] = {EGL_RENDERABLE_TYPE, renderableType, EGL_NONE};
      if (!eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs) || numConfigs != 1 ||
          !HasExtension(eglQueryString(m_display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context"))
        GTEST_SKIP() << "no usable EGL config";
    }

#if defined(HAS_GL)
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, nullptr);
#else
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttribs);
#endif
    if (m_context == EGL_NO_CONTEXT || !eglMakeCurrent(m_display, m_surface, m_surface, m_context))
      GTEST_SKIP() << "failed to create an EGL context";
  }

  void TearDown() override
  {
    if (m_display != EGL_NO_DISPLAY)
    {
      eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (m_context != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_context);
      if (m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);
      eglTerminate(m_display);
    }
  }

  static GLuint BoundBuffer(GLenum binding)
  {
    GLint name = 0;
    glGetIntegerv(binding, &name);
    return static_cast<GLuint>(name);
  }

  static GLint BufferSize(GLenum target)
  {
    GLint size = 0;
    glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);
    return size;
  }

  EGLDisplay m_display = EGL_NO_DISPLAY;
  EGLSurface m_surface = EGL_NO_SURFACE;
  EGLContext m_context = EGL_NO_CONTEXT;
};

TEST_F(TestGLBufferObject, CreatesLazilyOnSetData)
{
  CGLBufferObject vbo{GL_ARRAY_BUFFER};
  EXPECT_FALSE(static_cast<bool>(vbo));

  const GLfloat data[3] = {1.0f, 2.0f, 3.0f};
  vbo.SetData(data, GL_STREAM_DRAW);

  EXPECT_TRUE(static_cast<bool>(vbo));
  const GLuint name = BoundBuffer(GL_ARRAY_BUFFER_BINDING);
  EXPECT_NE(name, 0u);
  EXPECT_TRUE(glIsBuffer(name));
  EXPECT_EQ(BufferSize(GL_ARRAY_BUFFER), static_cast<GLint>(sizeof(data)));
}

TEST_F(TestGLBufferObject, SetDataReuploadsEveryCall)
{
  CGLBufferObject vbo{GL_ARRAY_BUFFER};
  const GLfloat first[2] = {};
  const GLfloat second[5] = {};

  vbo.SetData(first, GL_STREAM_DRAW);
  const GLuint name = BoundBuffer(GL_ARRAY_BUFFER_BINDING);
  EXPECT_EQ(BufferSize(GL_ARRAY_BUFFER), static_cast<GLint>(sizeof(first)));

  vbo.SetData(second, GL_STREAM_DRAW);
  EXPECT_EQ(BoundBuffer(GL_ARRAY_BUFFER_BINDING), name);
  EXPECT_EQ(BufferSize(GL_ARRAY_BUFFER), static_cast<GLint>(sizeof(second)));
}

TEST_F(TestGLBufferObject, SetDataOnceIgnoresLaterData)
{
  CGLBufferObject ibo{GL_ELEMENT_ARRAY_BUFFER};
  const GLubyte first[4] = {0, 1, 3, 2};
  const GLubyte second[8] = {};

  ibo.SetDataOnce(first);
  const GLuint name = BoundBuffer(GL_ELEMENT_ARRAY_BUFFER_BINDING);
  EXPECT_EQ(BufferSize(GL_ELEMENT_ARRAY_BUFFER), static_cast<GLint>(sizeof(first)));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  ibo.SetDataOnce(second);
  EXPECT_EQ(BoundBuffer(GL_ELEMENT_ARRAY_BUFFER_BINDING), name);
  EXPECT_EQ(BufferSize(GL_ELEMENT_ARRAY_BUFFER), static_cast<GLint>(sizeof(first)));
}

TEST_F(TestGLBufferObject, BindsToItsOwnTargetOnly)
{
  CGLBufferObject ibo{GL_ELEMENT_ARRAY_BUFFER};
  const GLubyte idx[4] = {0, 1, 3, 2};
  ibo.SetDataOnce(idx);

  EXPECT_NE(BoundBuffer(GL_ELEMENT_ARRAY_BUFFER_BINDING), 0u);
  EXPECT_EQ(BoundBuffer(GL_ARRAY_BUFFER_BINDING), 0u);
}

TEST_F(TestGLBufferObject, DestroyDeletesAndAllowsRecreation)
{
  CGLBufferObject vbo{GL_ARRAY_BUFFER};
  const GLfloat data[3] = {};
  vbo.SetData(data, GL_STREAM_DRAW);
  const GLuint name = BoundBuffer(GL_ARRAY_BUFFER_BINDING);

  vbo.Destroy();
  EXPECT_FALSE(static_cast<bool>(vbo));
  EXPECT_FALSE(glIsBuffer(name));

  vbo.SetData(data, GL_STREAM_DRAW);
  EXPECT_TRUE(static_cast<bool>(vbo));
  EXPECT_TRUE(glIsBuffer(BoundBuffer(GL_ARRAY_BUFFER_BINDING)));
}

TEST_F(TestGLBufferObject, DestructorDeletesBuffer)
{
  GLuint name = 0;
  {
    CGLBufferObject vbo{GL_ARRAY_BUFFER};
    const GLfloat data[3] = {};
    vbo.SetData(data, GL_STREAM_DRAW);
    name = BoundBuffer(GL_ARRAY_BUFFER_BINDING);
    EXPECT_TRUE(glIsBuffer(name));
  }
  EXPECT_FALSE(glIsBuffer(name));
}
