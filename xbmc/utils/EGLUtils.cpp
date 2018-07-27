/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EGLUtils.h"
#include "log.h"

#include "StringUtils.h"
#include "guilib/IDirtyRegionSolver.h"
#include "settings/AdvancedSettings.h"

#include <EGL/eglext.h>

std::set<std::string> CEGLUtils::GetClientExtensions()
{
  const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (!extensions)
  {
    return {};
  }
  std::set<std::string> result;
  StringUtils::SplitTo(std::inserter(result, result.begin()), extensions, " ");
  return result;
}

std::set<std::string> CEGLUtils::GetExtensions(EGLDisplay eglDisplay)
{
  const char* extensions = eglQueryString(eglDisplay, EGL_EXTENSIONS);
  if (!extensions)
  {
    throw std::runtime_error("Could not query EGL for extensions");
  }
  std::set<std::string> result;
  StringUtils::SplitTo(std::inserter(result, result.begin()), extensions, " ");
  return result;
}

bool CEGLUtils::HasExtension(EGLDisplay eglDisplay, const std::string& name)
{
  auto exts = GetExtensions(eglDisplay);
  return (exts.find(name) != exts.end());
}

bool CEGLUtils::HasClientExtension(const std::string& name)
{
  auto exts = GetClientExtensions();
  return (exts.find(name) != exts.end());
}

void CEGLUtils::LogError(const std::string& what)
{
  CLog::Log(LOGERROR, "%s (EGL error %d)", what.c_str(), eglGetError());
}

CEGLContextUtils::CEGLContextUtils()
{
}

CEGLContextUtils::CEGLContextUtils(EGLenum platform, std::string const& platformExtension)
: m_platform{platform}
{
  m_platformSupported = CEGLUtils::HasClientExtension("EGL_EXT_platform_base") && CEGLUtils::HasClientExtension(platformExtension);
}

bool CEGLContextUtils::IsPlatformSupported() const
{
  return m_platformSupported;
}

CEGLContextUtils::~CEGLContextUtils()
{
  Destroy();
}

bool CEGLContextUtils::CreateDisplay(EGLNativeDisplayType nativeDisplay, EGLint renderableType, EGLint renderingApi)
{
  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    throw std::logic_error("Do not call CreateDisplay when display has already been created");
  }

  m_eglDisplay = eglGetDisplay(nativeDisplay);
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CEGLUtils::LogError("failed to get EGL display");
    return false;
  }

  return InitializeDisplay(renderableType, renderingApi);
}

bool CEGLContextUtils::CreatePlatformDisplay(void* nativeDisplay, EGLNativeDisplayType nativeDisplayLegacy, EGLint renderableType, EGLint renderingApi)
{
  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    throw std::logic_error("Do not call CreateDisplay when display has already been created");
  }

#if defined(EGL_EXT_platform_base)
  if (IsPlatformSupported())
  {
    // Theoretically it is possible to use eglGetDisplay() and eglCreateWindowSurface,
    // but then the EGL library basically has to guess which platform we want
    // if it supports multiple which is usually the case -
    // it's better and safer to make it explicit

    auto getPlatformDisplayEXT = CEGLUtils::GetRequiredProcAddress<PFNEGLGETPLATFORMDISPLAYEXTPROC>("eglGetPlatformDisplayEXT");
    m_eglDisplay = getPlatformDisplayEXT(m_platform, nativeDisplay, nullptr);

    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CEGLUtils::LogError("failed to get platform display");
      return false;
    }
  }
#endif

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    return CreateDisplay(nativeDisplayLegacy, renderableType, renderingApi);
  }
  return InitializeDisplay(renderableType, renderingApi);
}

bool CEGLContextUtils::InitializeDisplay(EGLint renderableType, EGLint renderingApi)
{
  int major, minor;
  if (!eglInitialize(m_eglDisplay, &major, &minor))
  {
    CEGLUtils::LogError("failed to initialize EGL display");
    Destroy();
    return false;
  }
  CLog::Log(LOGINFO, "EGL v%d.%d", major, minor);

  if (eglBindAPI(renderingApi) != EGL_TRUE)
  {
    CEGLUtils::LogError("failed to bind EGL API");
    Destroy();
    return false;
  }

  EGLint surfaceType = EGL_WINDOW_BIT;
  // for the non-trivial dirty region modes, we need the EGL buffer to be preserved across updates
  if (g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_COST_REDUCTION ||
      g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_UNION)
    surfaceType |= EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

  EGLint attribs[] =
  {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_STENCIL_SIZE, 0,
    EGL_SAMPLE_BUFFERS, 0,
    EGL_SAMPLES, 0,
    EGL_SURFACE_TYPE, surfaceType,
    EGL_RENDERABLE_TYPE, renderableType,
    EGL_NONE
  };

  EGLint neglconfigs = 0;
  if (eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &neglconfigs) != EGL_TRUE)
  {
    CEGLUtils::LogError("failed to query number of EGL configs");
    Destroy();
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    Destroy();
    return false;
  }

  return true;
}

bool CEGLContextUtils::CreateContext(const EGLint* contextAttribs)
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    throw std::logic_error("Do not call CreateContext when context has already been created");
  }

  m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                  EGL_NO_CONTEXT, contextAttribs);

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    // This is expected to fail under some circumstances, so log as debug
    CLog::Log(LOGDEBUG, "Failed to create EGL context (EGL error %d)", eglGetError());
    return false;
  }

  return true;
}

bool CEGLContextUtils::BindContext()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE || m_eglContext == EGL_NO_CONTEXT)
  {
    throw std::logic_error("Activating an EGLContext requires display, surface, and context");
  }

  if (eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) != EGL_TRUE)
  {
    CLog::Log(LOGERROR, "Failed to make context current %p %p %p",
                         m_eglDisplay, m_eglSurface, m_eglContext);
    return false;
  }

  return true;
}

void CEGLContextUtils::SurfaceAttrib()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE)
  {
    throw std::logic_error("Setting surface attributes requires a surface");
  }

  // for the non-trivial dirty region modes, we need the EGL buffer to be preserved across updates
  if (g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_COST_REDUCTION ||
      g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_UNION)
  {
    if (eglSurfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED) != EGL_TRUE)
    {
      CEGLUtils::LogError("failed to set EGL_BUFFER_PRESERVED swap behavior");
    }
  }
}

bool CEGLContextUtils::CreateSurface(EGLNativeWindowType nativeWindow)
{
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    throw std::logic_error("Creating a surface requires a display");
  }
  if (m_eglSurface != EGL_NO_SURFACE)
  {
    throw std::logic_error("Do not call CreateSurface when surface has already been created");
  }

  m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, nativeWindow, nullptr);

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CEGLUtils::LogError("failed to create window surface");
    return false;
  }

  SurfaceAttrib();

  return true;
}

bool CEGLContextUtils::CreatePlatformSurface(void* nativeWindow, EGLNativeWindowType nativeWindowLegacy)
{
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    throw std::logic_error("Creating a surface requires a display");
  }
  if (m_eglSurface != EGL_NO_SURFACE)
  {
    throw std::logic_error("Do not call CreateSurface when surface has already been created");
  }

#if defined(EGL_EXT_platform_base)
  if (IsPlatformSupported())
  {
    auto createPlatformWindowSurfaceEXT = CEGLUtils::GetRequiredProcAddress<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>("eglCreatePlatformWindowSurfaceEXT");
    m_eglSurface = createPlatformWindowSurfaceEXT(m_eglDisplay, m_eglConfig, nativeWindow, nullptr);

    if (m_eglSurface == EGL_NO_SURFACE)
    {
      CEGLUtils::LogError("failed to create platform window surface");
      return false;
    }
  }
#endif

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    return CreateSurface(nativeWindowLegacy);
  }

  SurfaceAttrib();

  return true;
}

void CEGLContextUtils::Destroy()
{
  DestroyContext();
  DestroySurface();

  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CEGLContextUtils::DestroyContext()
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_eglDisplay, m_eglContext);
    m_eglContext = EGL_NO_CONTEXT;
  }
}

void CEGLContextUtils::DestroySurface()
{
  if (m_eglSurface != EGL_NO_SURFACE)
  {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }
}


bool CEGLContextUtils::SetVSync(bool enable)
{
  return (eglSwapInterval(m_eglDisplay, enable) == EGL_TRUE);
}

void CEGLContextUtils::SwapBuffers()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE)
  {
    return;
  }

  if (eglSwapBuffers(m_eglDisplay, m_eglSurface) != EGL_TRUE)
  {
    // For now we just hard fail if this fails
    // Theoretically, EGL_CONTEXT_LOST could be handled, but it needs to be checked
    // whether egl implementations actually use it (mesa does not)
    CEGLUtils::LogError("eglSwapBuffers failed");
    throw std::runtime_error("eglSwapBuffers failed");
  }
}
