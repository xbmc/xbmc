/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// always define GL_GLEXT_PROTOTYPES before include gl headers
#if !defined(GL_GLEXT_PROTOTYPES)
  #define GL_GLEXT_PROTOTYPES
#endif

#include "GLContextEGL.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <clocale>
#include <mutex>

#include <EGL/eglext.h>
#include <unistd.h>

#include "PlatformDefs.h"
#include "system_gl.h"

#define EGL_NO_CONFIG (EGLConfig)0

CGLContextEGL::CGLContextEGL(Display* dpy, EGLint renderingApi)
  : CGLContext(dpy),
    m_renderingApi(renderingApi),
    m_eglConfig(EGL_NO_CONFIG),
    m_eglGetPlatformDisplayEXT(
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT"))
{
  m_extPrefix = "EGL_";

  const auto settings = CServiceBroker::GetSettingsComponent();
  if (settings)
  {
    m_omlSync = settings->GetAdvancedSettings()->m_omlSync;
  }
}

CGLContextEGL::~CGLContextEGL()
{
  Destroy();
}

bool CGLContextEGL::Refresh(bool force, int screen, Window glWindow, bool &newContext)
{
  m_sync.cont = 0;

  // refresh context
  if (m_eglContext && !force)
  {
    if (m_eglSurface == EGL_NO_SURFACE)
    {
      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, glWindow, NULL);
      if (m_eglSurface == EGL_NO_SURFACE)
      {
        CLog::Log(LOGERROR, "failed to create EGL window surface {}", eglGetError());
        return false;
      }
    }

    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshEGLContext: refreshing context");
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    return true;
  }

  Destroy();
  newContext = true;

  if (m_eglGetPlatformDisplayEXT)
  {
    EGLint attribs[] =
    {
      EGL_PLATFORM_X11_SCREEN_EXT, screen,
      EGL_NONE
    };
    m_eglDisplay = m_eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT,(EGLNativeDisplayType)m_dpy,
                                            attribs);
  }
  else
    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get egl display");
    return false;
  }
  if (!eglInitialize(m_eglDisplay, NULL, NULL))
  {
    CLog::Log(LOGERROR, "failed to initialize egl");
    Destroy();
    return false;
  }
  if (!eglBindAPI(m_renderingApi))
  {
    CLog::Log(LOGERROR, "failed to bind rendering API");
    Destroy();
    return false;
  }

  // create context

  XVisualInfo vMask;
  XVisualInfo *vInfo = nullptr;
  int availableVisuals    = 0;
  vMask.screen = screen;
  XWindowAttributes winAttr;

  if (!XGetWindowAttributes(m_dpy, glWindow, &winAttr))
  {
    CLog::Log(LOGWARNING, "Failed to get window attributes");
    Destroy();
    return false;
  }

  vMask.visualid = XVisualIDFromVisual(winAttr.visual);
  vInfo = XGetVisualInfo(m_dpy, VisualScreenMask | VisualIDMask, &vMask, &availableVisuals);
  if (!vInfo)
  {
    CLog::Log(LOGERROR, "Failed to get VisualInfo of visual 0x{:x}", (unsigned)vMask.visualid);
    Destroy();
    return false;
  }

  unsigned int visualid = static_cast<unsigned int>(vInfo->visualid);
  m_eglConfig = GetEGLConfig(m_eglDisplay, vInfo);
  XFree(vInfo);

  if (m_eglConfig == EGL_NO_CONFIG)
  {
    CLog::Log(LOGERROR, "failed to get suitable eglconfig for visual 0x{:x}", visualid);
    Destroy();
    return false;
  }

  CLog::Log(LOGINFO, "Using visual 0x{:x}", visualid);

  m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, glWindow, NULL);
  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CLog::Log(LOGERROR, "failed to create EGL window surface {}", eglGetError());
    Destroy();
    return false;
  }

  EGLint contextAttributes[] =
  {
      EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
      EGL_CONTEXT_MINOR_VERSION_KHR, 2,
      EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
      EGL_NONE
  };
  m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    EGLint contextAttributes[] =
    {
      EGL_CONTEXT_MAJOR_VERSION_KHR, 2,
      EGL_NONE
    };
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);

    if (m_eglContext == EGL_NO_CONTEXT)
    {
      CLog::Log(LOGERROR, "failed to create EGL context");
      Destroy();
      return false;
    }

    CLog::Log(LOGWARNING, "Failed to get an OpenGL context supporting core profile 3.2, "
                          "using legacy mode with reduced feature set");
  }

  if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
  {
    CLog::Log(LOGERROR, "Failed to make context current {} {} {}", fmt::ptr(m_eglDisplay),
              fmt::ptr(m_eglSurface), fmt::ptr(m_eglContext));
    Destroy();
    return false;
  }

  m_eglGetSyncValuesCHROMIUM = (PFNEGLGETSYNCVALUESCHROMIUMPROC)eglGetProcAddress("eglGetSyncValuesCHROMIUM");

  m_usePB = false;
  return true;
}

bool CGLContextEGL::CreatePB()
{
  const EGLint configAttribs[] =
  {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
  };

  const EGLint pbufferAttribs[] =
  {
    EGL_WIDTH, 9,
    EGL_HEIGHT, 9,
    EGL_NONE,
  };

  Destroy();

  if (m_eglGetPlatformDisplayEXT)
  {
    m_eglDisplay = m_eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT,(EGLNativeDisplayType)m_dpy,
                                            NULL);
  }
  else
    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get egl display");
    return false;
  }
  if (!eglInitialize(m_eglDisplay, NULL, NULL))
  {
    CLog::Log(LOGERROR, "failed to initialize egl");
    Destroy();
    return false;
  }
  if (!eglBindAPI(m_renderingApi))
  {
    CLog::Log(LOGERROR, "failed to bind rendering API");
    Destroy();
    return false;
  }

  EGLint numConfigs;

  eglChooseConfig(m_eglDisplay, configAttribs, &m_eglConfig, 1, &numConfigs);
  m_eglSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttribs);
  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CLog::Log(LOGERROR, "failed to create EGL window surface {}", eglGetError());
    Destroy();
    return false;
  }

  EGLint contextAttributes[] =
  {
      EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
      EGL_CONTEXT_MINOR_VERSION_KHR, 2,
      EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
      EGL_NONE
  };
  m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    EGLint contextAttributes[] =
    {
      EGL_CONTEXT_MAJOR_VERSION_KHR, 2,
      EGL_NONE
    };
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);

    if (m_eglContext == EGL_NO_CONTEXT)
    {
      CLog::Log(LOGERROR, "failed to create EGL context");
      Destroy();
      return false;
    }
  }

  if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
  {
    CLog::Log(LOGERROR, "Failed to make context current {} {} {}", fmt::ptr(m_eglDisplay),
              fmt::ptr(m_eglSurface), fmt::ptr(m_eglContext));
    Destroy();
    return false;
  }

  m_usePB = true;
  return true;
}

void CGLContextEGL::Destroy()
{
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_eglDisplay, m_eglContext);
    m_eglContext = EGL_NO_CONTEXT;
  }

  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }

  if (m_eglDisplay)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }

  m_eglConfig = EGL_NO_CONFIG;
}

void CGLContextEGL::Detach()
{
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }
}

bool CGLContextEGL::SuitableCheck(EGLDisplay eglDisplay, EGLConfig config)
{
  if (config == EGL_NO_CONFIG)
    return false;

  EGLint value;
  if (!eglGetConfigAttrib(eglDisplay, config, EGL_RED_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(eglDisplay, config, EGL_DEPTH_SIZE, &value) || value < 16)
    return false;

  return true;
}

EGLConfig CGLContextEGL::GetEGLConfig(EGLDisplay eglDisplay, XVisualInfo *vInfo)
{
  EGLint numConfigs;

  if (!eglGetConfigs(eglDisplay, nullptr, 0, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of egl configs");
    return EGL_NO_CONFIG;
  }
  if (numConfigs == 0)
  {
    CLog::Log(LOGERROR, "No suitable egl configs found");
    return EGL_NO_CONFIG;
  }

  EGLConfig *eglConfigs;
  eglConfigs = (EGLConfig*)malloc(numConfigs * sizeof(EGLConfig));
  if (!eglConfigs)
  {
    CLog::Log(LOGERROR, "eglConfigs malloc failed");
    return EGL_NO_CONFIG;
  }
  EGLConfig eglConfig = EGL_NO_CONFIG;
  if (!eglGetConfigs(eglDisplay, eglConfigs, numConfigs, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query egl configs");
    goto Exit;
  }
  for (EGLint i = 0; i < numConfigs; ++i)
  {
    if (!SuitableCheck(eglDisplay, eglConfigs[i]))
      continue;

    EGLint value;
    if (!eglGetConfigAttrib(eglDisplay, eglConfigs[i], EGL_NATIVE_VISUAL_ID, &value))
    {
      CLog::Log(LOGERROR, "Failed to query EGL_NATIVE_VISUAL_ID for egl config.");
      break;
    }
    if (value == (EGLint)vInfo->visualid)
    {
      eglConfig = eglConfigs[i];
      break;
    }
  }

Exit:
  free(eglConfigs);
  return eglConfig;
}

void CGLContextEGL::SetVSync(bool enable)
{
  eglSwapInterval(m_eglDisplay, enable ? 1 : 0);
}

void CGLContextEGL::SwapBuffers()
{
  if ((m_eglDisplay == EGL_NO_DISPLAY) || (m_eglSurface == EGL_NO_SURFACE))
    return;

  if (m_usePB)
  {
    eglSwapBuffers(m_eglDisplay, m_eglSurface);
    usleep(20 * 1000);
    return;
  }

  uint64_t ust1, ust2;
  uint64_t msc1, msc2;
  uint64_t sbc1, sbc2;
  struct timespec nowTs;
  uint64_t now;
  uint64_t cont = m_sync.cont;
  uint64_t interval = m_sync.interval;

  if (m_eglGetSyncValuesCHROMIUM)
  {
    m_eglGetSyncValuesCHROMIUM(m_eglDisplay, m_eglSurface, &ust1, &msc1, &sbc1);
  }

  eglSwapBuffers(m_eglDisplay, m_eglSurface);

  if (!m_eglGetSyncValuesCHROMIUM)
    return;

  clock_gettime(CLOCK_MONOTONIC, &nowTs);
  now = static_cast<uint64_t>(nowTs.tv_sec) * 1000000000ULL + nowTs.tv_nsec;

  m_eglGetSyncValuesCHROMIUM(m_eglDisplay, m_eglSurface, &ust2, &msc2, &sbc2);

  if ((msc1 - m_sync.msc1) > 2)
  {
    cont = 0;
  }

  // we want to block in SwapBuffers
  // if a vertical retrace occurs 5 times in a row outside
  // of this function, we take action
  if (m_sync.cont < 5)
  {
    if ((msc1 - m_sync.msc1) == 2)
    {
      cont = 0;
    }
    else if ((msc1 - m_sync.msc1) == 1)
    {
      interval = (ust1 - m_sync.ust1) / (msc1 - m_sync.msc1);
      cont++;
    }
  }
  else if (m_sync.cont == 5 && m_omlSync)
  {
    CLog::Log(LOGDEBUG, "CGLContextEGL::SwapBuffers: sync check blocking");

    if (msc2 == msc1)
    {
      // if no vertical retrace has occurred in eglSwapBuffers,
      // sleep until next vertical retrace
      uint64_t lastIncrement = (now / 1000 - ust2);
      if (lastIncrement > m_sync.interval)
      {
        lastIncrement = m_sync.interval;
        CLog::Log(LOGWARNING, "CGLContextEGL::SwapBuffers: last msc time greater than interval");
      }
      uint64_t sleeptime = m_sync.interval - lastIncrement;
      usleep(sleeptime);
      cont++;
      msc2++;
      CLog::Log(LOGDEBUG, "CGLContextEGL::SwapBuffers: sync sleep: {}", sleeptime);
    }
  }
  else if ((m_sync.cont > 5) && (msc2 == m_sync.msc2))
  {
    // sleep until next vertical retrace
    // this avoids blocking outside of this function
    uint64_t lastIncrement = (now / 1000 - ust2);
    if (lastIncrement > m_sync.interval)
    {
      lastIncrement = m_sync.interval;
      CLog::Log(LOGWARNING, "CGLContextEGL::SwapBuffers: last msc time greater than interval (1)");
    }
    uint64_t sleeptime = m_sync.interval - lastIncrement;
    usleep(sleeptime);
    msc2++;
  }
  {
    std::unique_lock<CCriticalSection> lock(m_syncLock);
    m_sync.ust1 = ust1;
    m_sync.ust2 = ust2;
    m_sync.msc1 = msc1;
    m_sync.msc2 = msc2;
    m_sync.interval = interval;
    m_sync.cont = cont;
  }
}

uint64_t CGLContextEGL::GetVblankTiming(uint64_t &msc, uint64_t &interval)
{
  struct timespec nowTs;
  uint64_t now;
  clock_gettime(CLOCK_MONOTONIC, &nowTs);
  now = static_cast<uint64_t>(nowTs.tv_sec) * 1000000000ULL + nowTs.tv_nsec;
  now /= 1000;

  std::unique_lock<CCriticalSection> lock(m_syncLock);
  msc = m_sync.msc2;

  interval = (m_sync.cont >= 5) ? m_sync.interval : m_sync.ust2 - m_sync.ust1;
  if (interval == 0)
    return 0;

  if (now < m_sync.ust2)
  {
    return 0;
  }

  uint64_t ret = now - m_sync.ust2;
  while (ret > interval)
  {
    ret -= interval;
    msc++;
  }

  return ret;
}

void CGLContextEGL::QueryExtensions()
{
  std::string extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
  m_extensions = std::string(" ") + extensions + " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:{}", m_extensions);
}
