/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HwRenderingContextEGL.h"

#include "EGLClientContext.h"
#include "HwRenderingContextEGLUtils.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "rendering/RenderSystem.h"
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
constexpr EGLint CLIENT_RENDERABLE_TYPE = EGL_OPENGL_ES3_BIT;
#else
constexpr EGLenum CLIENT_API = EGL_OPENGL_API;
constexpr EGLint CLIENT_RENDERABLE_TYPE = EGL_OPENGL_BIT;
#endif

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
  ~CHwRenderingContextEGL() override = default;

  bool SupportsHardwareRendering() const override;
  bool Create(const HwContextProperties& properties) override;
  bool IsCreated() const override { return m_client.IsCreated(); }
  bool MakeCurrent() override;
  void RestoreCurrent() override;
  void Destroy() override;

private:
  CRenderContext& m_context;
  CEGLClientContext m_client{CLIENT_API};
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
  if (!renderSystem || winSystem->GetEGLDisplay() == EGL_NO_DISPLAY ||
      winSystem->GetEGLContext() == EGL_NO_CONTEXT)
    return false;
  const EGLDisplay display = winSystem->GetEGLDisplay();
  unsigned int major = 0, minor = 0;
  renderSystem->GetRenderVersion(major, minor);
#if defined(HAS_GLES)
  constexpr bool embedded = true;
#else
  constexpr bool embedded = false;
#endif
  const char* version = eglQueryString(display, EGL_VERSION);
  const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
  const auto capabilities = GetEGLCapabilities(version, extensions);
  if (!capabilities.createContext)
    return false;

  // Creation remains authoritative for the client's version and share group.
  const bool configAvailable =
      !GetEGLClientConfigs(display, winSystem->GetEGLConfig(), CLIENT_RENDERABLE_TYPE,
                           capabilities.surfaceless)
           .empty();
  return configAvailable &&
         SupportsEGLHardwareRendering(version, extensions, embedded, major, minor, configAvailable);
}

bool CHwRenderingContextEGL::Create(const HwContextProperties& properties)
{
  Destroy();
  if (!SupportsHardwareRendering())
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

  const EGLDisplay display = winSystem->GetEGLDisplay();
  const char* eglVersion = eglQueryString(display, EGL_VERSION);
  const auto capabilities = GetEGLCapabilities(eglVersion, eglQueryString(display, EGL_EXTENSIONS));
  const auto configs = GetEGLClientConfigs(display, winSystem->GetEGLConfig(),
                                           CLIENT_RENDERABLE_TYPE, capabilities.surfaceless);
  if (configs.empty())
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: No usable EGL client config (EGL error {:#x})",
              eglGetError());
    return false;
  }

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

  for (const EGLConfig config : configs)
  {
    EGLint configID = 0, renderable = 0, surfaces = 0;
    eglGetConfigAttrib(display, config, EGL_CONFIG_ID, &configID);
    eglGetConfigAttrib(display, config, EGL_RENDERABLE_TYPE, &renderable);
    eglGetConfigAttrib(display, config, EGL_SURFACE_TYPE, &surfaces);
    CLog::Log(LOGDEBUG,
              "RetroPlayer[RENDER]: EGL {} client config {} (GUI {}, renderable {:#x}, surfaces "
              "{:#x}), binding {}",
              eglVersion, configID, config == winSystem->GetEGLConfig(), renderable, surfaces,
              capabilities.surfaceless ? "surfaceless" : "pbuffer");

    for (const auto& [major, minor] : versions)
    {
      const auto attributes = BuildEGLContextAttributes(properties, major, minor, eglVersion);
      // EGL validates sharing across configs and ES versions against Kodi's actual context.
      if (!m_client.Create(display, config, winSystem->GetEGLContext(), attributes.data(),
                           capabilities.surfaceless))
        continue;

      const bool bound = m_client.MakeCurrent();
      const bool debugContext = !properties.debugContext || (bound && IsDebugContext());
      if (bound)
        CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Client GL_VERSION = {}",
                  reinterpret_cast<const char*>(glGetString(GL_VERSION)));
      const bool restored = !bound || m_client.RestoreCurrent();
      if (!bound || !debugContext || !restored)
      {
        CLog::Log(LOGERROR,
                  "RetroPlayer[RENDER]: Could not validate or restore the EGL client context");
        Destroy();
        return false;
      }
      CLog::Log(
          LOGINFO,
          "RetroPlayer[RENDER]: Created a {} {}.{} shared game context using {} (EGL config {})",
          apiName, major, minor, capabilities.surfaceless ? "surfaceless" : "pbuffer", configID);
      return true;
    }
  }

  CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Cannot create the requested shared {} {}.{} context",
            apiName, properties.versionMajor, properties.versionMinor);
  return false;
}

bool CHwRenderingContextEGL::MakeCurrent()
{
  return m_client.MakeCurrent();
}

void CHwRenderingContextEGL::RestoreCurrent()
{
  m_client.RestoreCurrent();
}

void CHwRenderingContextEGL::Destroy()
{
  m_client.Destroy();
}

std::unique_ptr<IHwRenderingContext> KODI::RETRO::CreateHwRenderingContextEGL(
    CRenderContext& context)
{
  return std::make_unique<CHwRenderingContextEGL>(context);
}
