/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

void CEGLUtils::LogError(const std::string& what)
{
  CLog::Log(LOGERROR, "%s (EGL error %d)", what.c_str(), eglGetError());
}

CEGLContextUtils::CEGLContextUtils()
{
}

CEGLContextUtils::~CEGLContextUtils()
{
  Destroy();
}

bool CEGLContextUtils::CreateDisplay(EGLDisplay display,
                                     EGLint renderableType,
                                     EGLint renderingApi)
{
  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    throw std::logic_error("Do not call CreateDisplay when display has already been created");
  }

  EGLint neglconfigs = 0;
  int major, minor;

  EGLint surfaceType = EGL_WINDOW_BIT;
  // for the non-trivial dirty region modes, we need the EGL buffer to be preserved across updates
  if (g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_COST_REDUCTION ||
      g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_UNION)
    surfaceType |= EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

  EGLint attribs[] =
  {
    EGL_RED_SIZE,        8,
    EGL_GREEN_SIZE,      8,
    EGL_BLUE_SIZE,       8,
    EGL_ALPHA_SIZE,      8,
    EGL_DEPTH_SIZE,     16,
    EGL_STENCIL_SIZE,    0,
    EGL_SAMPLE_BUFFERS,  0,
    EGL_SAMPLES,         0,
    EGL_SURFACE_TYPE,    surfaceType,
    EGL_RENDERABLE_TYPE, renderableType,
    EGL_NONE
  };

#if defined(EGL_EXT_platform_base) && defined(EGL_KHR_platform_gbm) && defined(HAVE_GBM)
  if (m_eglDisplay == EGL_NO_DISPLAY &&
      CEGLUtils::HasExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_base") &&
      CEGLUtils::HasExtension(EGL_NO_DISPLAY, "EGL_KHR_platform_gbm"))
  {
    PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (getPlatformDisplayEXT)
    {
      m_eglDisplay = getPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, (EGLNativeDisplayType)display, NULL);
    }

    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CEGLUtils::LogError("Failed to get EGL platform display");
      return false;
    }
  }
#endif

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)display);
  }

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CEGLUtils::LogError("failed to get EGL display");
    return false;
  }

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

bool CEGLContextUtils::SurfaceAttrib()
{
  // for the non-trivial dirty region modes, we need the EGL buffer to be preserved across updates
  if (g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_COST_REDUCTION ||
      g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_UNION)
  {
    if ((m_eglDisplay == EGL_NO_DISPLAY) || (m_eglSurface == EGL_NO_SURFACE))
    {
      return false;
    }

    if (eglSurfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED) != EGL_TRUE)
    {
      CLog::Log(LOGDEBUG, "%s: Could not set EGL_SWAP_BEHAVIOR",__FUNCTION__);
    }
  }

  return true;
}

bool CEGLContextUtils::CreateSurface(EGLNativeWindowType surface)
{
  m_eglSurface = eglCreateWindowSurface(m_eglDisplay,
                                        m_eglConfig,
                                        surface,
                                        nullptr);

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CLog::Log(LOGERROR, "failed to create EGL window surface %d", eglGetError());
    return false;
  }

  return true;
}

void CEGLContextUtils::Destroy()
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    eglDestroyContext(m_eglDisplay, m_eglContext);
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    m_eglContext = EGL_NO_CONTEXT;
  }

  if (m_eglSurface != EGL_NO_SURFACE)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }

  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CEGLContextUtils::Detach()
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }

  if (m_eglSurface != EGL_NO_SURFACE)
  {
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
