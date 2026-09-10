/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HwRenderingContextEGL.h"

#include "HwRenderingContextEGLUtils.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "rendering/RenderSystem.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include "system_gl.h"
#if defined(TARGET_ANDROID)
#include "windowing/android/WinSystemAndroidGLESContext.h"
#else
#include "windowing/linux/WinSystemEGL.h"
#endif

#include <string>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace KODI::RETRO;

namespace
{
#if defined(TARGET_ANDROID)
using CWinSystemEGL = CWinSystemAndroidGLESContext;
#else
using CWinSystemEGL = KODI::WINDOWING::LINUX::CWinSystemEGL;
#endif

#if defined(HAS_GLES)
constexpr EGLenum CLIENT_API = EGL_OPENGL_ES_API;
#else
constexpr EGLenum CLIENT_API = EGL_OPENGL_API;
#endif

class CRestoreEGLAPI
{
public:
  ~CRestoreEGLAPI() { eglBindAPI(m_api); }

private:
  const EGLenum m_api{eglQueryAPI()};
};

bool IsDebugContext()
{
#if defined(HAS_GLES)
  GLint major = 0, minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  bool supportsDebugOutput = major > 3 || (major == 3 && minor >= 2);
  if (!supportsDebugOutput)
  {
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint index = 0; index < count; ++index)
    {
      const auto* extension = glGetStringi(GL_EXTENSIONS, index);
      if (extension && std::string_view(reinterpret_cast<const char*>(extension)) == "GL_KHR_debug")
      {
        supportsDebugOutput = true;
        break;
      }
    }
  }
  // ES 3.0/3.1 cannot query context flags; KHR_debug specifies this initial state instead.
  return supportsDebugOutput && glIsEnabled(GL_DEBUG_OUTPUT_KHR) == GL_TRUE &&
         glGetError() == GL_NO_ERROR;
#else
  GLint flags = 0;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  return glGetError() == GL_NO_ERROR && (flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
#endif
}

class CHwRenderingContextEGL : public IHwRenderingContext
{
public:
  explicit CHwRenderingContextEGL(CRenderContext& context) : m_context(context) {}
  ~CHwRenderingContextEGL() override { Destroy(); }

  bool SupportsHardwareRendering() const override;
  bool Create(const HwContextProperties& properties) override;
  bool IsCreated() const override { return m_eglContext != EGL_NO_CONTEXT; }
  bool MakeCurrent() override;
  void RestoreCurrent() override;
  void Destroy() override;

private:
  bool RestorePreviousContext();

  CRenderContext& m_context;
  EGLenum m_prevAPI{EGL_OPENGL_ES_API};
  EGLDisplay m_prevDisplay{EGL_NO_DISPLAY};
  EGLSurface m_prevDraw{EGL_NO_SURFACE};
  EGLSurface m_prevRead{EGL_NO_SURFACE};
  EGLContext m_prevContext{EGL_NO_CONTEXT};
  EGLDisplay m_eglDisplay{EGL_NO_DISPLAY};
  EGLConfig m_eglConfig{};
  EGLContext m_eglContext{EGL_NO_CONTEXT};
};
} // namespace

bool CHwRenderingContextEGL::SupportsHardwareRendering() const
{
  // Asked of the window system rather than the build: a build carrying this
  // pool can still be running where there is no EGL display to share.
  auto* winSystem = dynamic_cast<CWinSystemEGL*>(m_context.Windowing());
  if (winSystem == nullptr)
    return false;

  auto* renderSystem = m_context.Rendering();
  if (!renderSystem || winSystem->GetEGLDisplay() == EGL_NO_DISPLAY)
    return false;
  const EGLDisplay display = winSystem->GetEGLDisplay();
  if (!SupportsEGLHardwareRendering(eglQueryString(display, EGL_VERSION),
                                    eglQueryString(display, EGL_EXTENSIONS)))
    return false;
  unsigned int major = 0, minor = 0;
  renderSystem->GetRenderVersion(major, minor);
#if defined(HAS_GLES)
  return major >= 3;
#else
  return major > 3 || (major == 3 && minor >= 2);
#endif
}

bool CHwRenderingContextEGL::Create(const HwContextProperties& properties)
{
  if (m_eglContext != EGL_NO_CONTEXT)
    Destroy();
  if (m_eglContext != EGL_NO_CONTEXT || !SupportsHardwareRendering())
    return false;
#if defined(HAS_GLES)
  if (!properties.embedded || properties.versionMajor < 3)
    return false;
#else
  if (properties.embedded)
    return false;
#endif

  auto* winSystem = dynamic_cast<CWinSystemEGL*>(m_context.Windowing());
  if (winSystem == nullptr)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Window system does not use EGL");
    return false;
  }

  m_eglDisplay = winSystem->GetEGLDisplay();

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get EGL display");
    return false;
  }

  CRestoreEGLAPI restoreAPI;
  if (!eglBindAPI(CLIENT_API))
    return false;

  // clang-format off

  EGLint attribs[] =
  {
#if defined(HAS_GLES)
    // ES3 rather than ES2: the framebuffer objects, the depth and stencil
    // attachments and the sampling this pool relies on are all core in ES3.
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#else
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
    // The context is only ever made current without a surface, so the surface
    // type is a formality -- but eglChooseConfig defaults it to EGL_WINDOW_BIT
    // and matches on it either way, so it has to name something the platform
    // really offers. Window is the one every platform Kodi runs on provides.
    // Pbuffer is not: on GBM, configs come from the GBM formats and advertise
    // window only, so asking for a pbuffer matches nothing at all and every
    // hardware core fails to get a context.
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
  };

  EGLint neglconfigs;
  if (!eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &neglconfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of EGL configs");
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    return false;
  }

  // clang-format on

  // In libretro the version a client asks for is a minimum, so try later ones
  // first and fall back to the request. ES only: its minor versions are purely
  // additive, whereas a desktop GL version interacts with the profile below.
  std::vector<std::pair<unsigned int, unsigned int>> versions;
  if (!properties.embedded && (properties.versionMajor < 3 ||
                               (properties.versionMajor == 3 && properties.versionMinor < 2)))
  {
    versions.emplace_back(3, 2);
  }
  else if (properties.versionMajor != 0)
  {
    if (properties.embedded && properties.versionMajor == 3)
    {
      for (unsigned int minor = 2; minor > properties.versionMinor; --minor)
        versions.emplace_back(3, minor);
    }
    versions.emplace_back(properties.versionMajor, properties.versionMinor);
  }
  else
  {
    // Nothing asked for, so let the driver decide
    versions.emplace_back(0, 0);
  }

  const std::string apiName = properties.embedded      ? "OpenGL ES"
                              : properties.coreProfile ? "OpenGL core profile"
                                                       : "OpenGL compatibility profile";

  // Ask the driver for each in turn rather than keeping a table of what it
  // supports. A refusal fails the stream cleanly and the client falls back.
  std::string contextName;
  const char* eglVersion = eglQueryString(m_eglDisplay, EGL_VERSION);
  for (const auto& [major, minor] : versions)
  {
    const auto contextAttribs = BuildEGLContextAttributes(properties, major, minor, eglVersion);

    contextName = apiName;
    if (major != 0)
      contextName += StringUtils::Format(" {}.{}", major, minor);

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, winSystem->GetEGLContext(),
                                    contextAttribs.data());
    if (m_eglContext != EGL_NO_CONTEXT)
    {
      if (properties.debugContext)
      {
        const bool bound = MakeCurrent();
        const bool debugContext = bound && IsDebugContext();
        const bool restored = !bound || RestorePreviousContext();
        if (!debugContext || !restored)
        {
          CLog::Log(LOGERROR,
                    "RetroPlayer[RENDER]: Could not verify the requested {} debug context",
                    contextName);
          Destroy();
          return false;
        }
      }
      CLog::Log(LOGINFO,
                "RetroPlayer[RENDER]: Created a {} context for the game client, sharing Kodi's "
                "objects",
                contextName);
      break;
    }
  }

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Game client asked for a {} context, which this system cannot "
              "provide (EGL error {:#x})",
              contextName, eglGetError());
    return false;
  }

  // Leave the caller's context and window surfaces current until BeginClientFrame().
  return true;
}

bool CHwRenderingContextEGL::MakeCurrent()
{
  if (!IsCreated())
    return false;
  m_prevAPI = eglQueryAPI();
  m_prevDisplay = eglGetCurrentDisplay();
  m_prevDraw = eglGetCurrentSurface(EGL_DRAW);
  m_prevRead = eglGetCurrentSurface(EGL_READ);
  m_prevContext = eglGetCurrentContext();

  if (!eglBindAPI(CLIENT_API) ||
      !eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to bind client context (EGL error {:#x})",
              eglGetError());
    eglBindAPI(m_prevAPI);
    return false;
  }
  return true;
}

void CHwRenderingContextEGL::RestoreCurrent()
{
  RestorePreviousContext();
}

bool CHwRenderingContextEGL::RestorePreviousContext()
{
  if (m_prevAPI != CLIENT_API)
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglBindAPI(m_prevAPI);
  const EGLBoolean restored =
      m_prevContext != EGL_NO_CONTEXT
          ? eglMakeCurrent(m_prevDisplay, m_prevDraw, m_prevRead, m_prevContext)
          : eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (!restored)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to restore EGL context (error {:#x})",
              eglGetError());
    // A failed restore must not leave later Kodi draws on the client context.
    eglBindAPI(CLIENT_API);
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglBindAPI(m_prevAPI);
  }
  m_prevDisplay = EGL_NO_DISPLAY;
  m_prevContext = EGL_NO_CONTEXT;
  m_prevDraw = m_prevRead = EGL_NO_SURFACE;
  return restored == EGL_TRUE;
}

void CHwRenderingContextEGL::Destroy()
{
  if (!IsCreated())
    return;
  if (!eglDestroyContext(m_eglDisplay, m_eglContext))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to destroy EGL context (error {:#x})",
              eglGetError());
    return;
  }
  m_eglContext = EGL_NO_CONTEXT;
  m_eglDisplay = EGL_NO_DISPLAY;
}

std::unique_ptr<IHwRenderingContext> KODI::RETRO::CreateHwRenderingContextEGL(
    CRenderContext& context)
{
  return std::make_unique<CHwRenderingContextEGL>(context);
}
