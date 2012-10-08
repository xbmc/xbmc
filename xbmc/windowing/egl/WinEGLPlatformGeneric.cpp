/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"

#include "system_gl.h"

#ifdef HAS_EGL

#include "WinEGLPlatformGeneric.h"
#include "utils/log.h"

#include <string>

CWinEGLPlatformGeneric::CWinEGLPlatformGeneric()
{
  m_surface = EGL_NO_SURFACE;
  m_context = EGL_NO_CONTEXT;
  m_display = EGL_NO_DISPLAY;
  
  // most egl platforms cannot render 1080p
  // default to 720p
  m_width  = 1280;
  m_height = 720;

  m_desktopRes.iScreen = 0;
  m_desktopRes.iWidth  = 1280;
  m_desktopRes.iHeight = 720;
  m_desktopRes.iScreenWidth  = 1280;
  m_desktopRes.iScreenHeight = 720;
  m_desktopRes.fRefreshRate = 60.0f;
  m_desktopRes.bFullScreen = true;
  m_desktopRes.iSubtitles = (int)(0.965 * 720);
  m_desktopRes.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
  m_desktopRes.fPixelRatio = 1.0f;
  m_desktopRes.strMode = "720p 16:9";
}

CWinEGLPlatformGeneric::~CWinEGLPlatformGeneric()
{
  UninitializeDisplay();
}

EGLNativeWindowType CWinEGLPlatformGeneric::InitWindowSystem(EGLNativeDisplayType nativeDisplay, int width, int height, int bpp)
{
  m_nativeDisplay = nativeDisplay;
  m_width = width;
  m_height = height;

  return getNativeWindow();
}

void CWinEGLPlatformGeneric::DestroyWindowSystem(EGLNativeWindowType native_window)
{
  UninitializeDisplay();
}

bool CWinEGLPlatformGeneric::SetDisplayResolution(RESOLUTION_INFO& res)
{
  return false;
}

bool CWinEGLPlatformGeneric::ClampToGUIDisplayLimits(int &width, int &height)
{
  width  = m_width;
  height = m_height;
  return true;
}

bool CWinEGLPlatformGeneric::ProbeDisplayResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  int gui_width  = m_width;
  int gui_height = m_height;
  float gui_refresh = 60.0f;
  RESOLUTION_INFO res;

  ClampToGUIDisplayLimits(gui_width, gui_height);

  res.iScreen       = 0;
  res.bFullScreen   = true;
  res.iSubtitles    = (int)(0.965 * gui_height);
  res.dwFlags       = D3DPRESENTFLAG_PROGRESSIVE;
  res.fRefreshRate  = gui_refresh;
  res.fPixelRatio   = 1.0f;
  res.iWidth        = gui_width;
  res.iHeight       = gui_height;
  res.iScreenWidth  = gui_width;
  res.iScreenHeight = gui_height;
  res.dwFlags       = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
  res.strMode.Format("%dx%d @ %.2f - Full Screen", gui_width, gui_height, gui_refresh);

  resolutions.push_back(res);
  return true;
}

bool CWinEGLPlatformGeneric::InitializeDisplay()
{
  if (m_display != EGL_NO_DISPLAY && m_config != NULL)
    return true;

  EGLBoolean eglStatus;
  EGLint     configCount;
  EGLConfig* configList = NULL;

  m_display = eglGetDisplay(m_nativeDisplay);
  if (m_display == EGL_NO_DISPLAY) 
  {
    CLog::Log(LOGERROR, "EGL failed to obtain display");
    return false;
  }
  
  if (!eglInitialize(m_display, 0, 0)) 
  {
    CLog::Log(LOGERROR, "EGL failed to initialize");
    return false;
  } 
  
  EGLint configAttrs[] = {
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,    0,
        EGL_SAMPLE_BUFFERS,  0,
        EGL_SAMPLES,         0,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
  };

  // Find out how many configurations suit our needs  
  eglStatus = eglChooseConfig(m_display, configAttrs, NULL, 0, &configCount);
  if (!eglStatus || !configCount) 
  {
    CLog::Log(LOGERROR, "EGL failed to return any matching configurations: %d", eglStatus);
    return false;
  }

  // Allocate room for the list of matching configurations
  configList = (EGLConfig*)malloc(configCount * sizeof(EGLConfig));
  if (!configList) 
  {
    CLog::Log(LOGERROR, "kdMalloc failure obtaining configuration list");
    return false;
  }

  // Obtain the configuration list from EGL
  eglStatus = eglChooseConfig(m_display, configAttrs,
                                configList, configCount, &configCount);
  if (!eglStatus || !configCount) 
  {
    CLog::Log(LOGERROR, "EGL failed to populate configuration list: %d", eglStatus);
    return false;
  }

  // Select an EGL configuration that matches the native window
  m_config = configList[0];

  if (m_surface != EGL_NO_SURFACE)
    ReleaseSurface();
 
  free(configList);
  return true;
}

bool CWinEGLPlatformGeneric::UninitializeDisplay()
{
  EGLBoolean eglStatus;
  
  DestroyWindow();

  if (m_display != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglStatus = eglTerminate(m_display);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error terminating EGL");
    m_display = EGL_NO_DISPLAY;
  }

  return true;
}

bool CWinEGLPlatformGeneric::CreateWindow()
{
  if (m_display == EGL_NO_DISPLAY || m_config == NULL)
  {
    if (!InitializeDisplay())
      return false;
  }
  
  if (m_surface != EGL_NO_SURFACE)
    return true;
  
  m_nativeWindow = getNativeWindow();

  m_surface = eglCreateWindowSurface(m_display, m_config, m_nativeWindow, NULL);
  if (!m_surface)
  {
    CLog::Log(LOGERROR, "EGL couldn't create window surface");
    return false;
  }

  // Let's get the current width and height
  EGLint width, height;
  if (!eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width) || !eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height) ||
      width <= 0 || height <= 0)
  {
    CLog::Log(LOGERROR, "EGL couldn't provide the surface's width and/or height");
    return false;
  }

  m_width = width;
  m_height = height;
  
  return true;
}

bool CWinEGLPlatformGeneric::DestroyWindow()
{
  EGLBoolean eglStatus;
  
  ReleaseSurface();

  if (m_surface == EGL_NO_SURFACE)
    return true;

  eglStatus = eglDestroySurface(m_display, m_surface);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "Error destroying EGL surface");
    return false;
  }

  m_surface = EGL_NO_SURFACE;
  m_width = 0;
  m_height = 0;

  return true;
}

bool CWinEGLPlatformGeneric::BindSurface()
{
  EGLBoolean eglStatus;
  
  if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE || m_config == NULL)
  {
    CLog::Log(LOGINFO, "EGL not configured correctly. Let's try to do that now...");
    if (!CreateWindow())
    {
      CLog::Log(LOGERROR, "EGL not configured correctly to create a surface");
      return false;
    }
  }

  eglStatus = eglBindAPI(EGL_OPENGL_ES_API);
  if (!eglStatus) 
  {
    CLog::Log(LOGERROR, "EGL failed to bind API: %d", eglStatus);
    return false;
  }

  EGLint contextAttrs[] = 
  {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  // Create an EGL context
  if (m_context == EGL_NO_CONTEXT)
  {
    m_context = eglCreateContext(m_display, m_config, NULL, contextAttrs);
    if (!m_context)
    {
      CLog::Log(LOGERROR, "EGL couldn't create context");
      return false;
    }
  }

  // Make the context and surface current to this thread for rendering
  eglStatus = eglMakeCurrent(m_display, m_surface, m_surface, m_context);
  if (!eglStatus) 
  {
    CLog::Log(LOGERROR, "EGL couldn't make context/surface current: %d", eglStatus);
    return false;
  }

  eglSwapInterval(m_display, 0);

  // For EGL backend, it needs to clear all the back buffers of the window
  // surface before drawing anything, otherwise the image will be blinking
  // heavily.  The default eglWindowSurface has 3 gdl surfaces as the back
  // buffer, that's why glClear should be called 3 times.
  glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(m_display, m_surface);

  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(m_display, m_surface);

  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(m_display, m_surface);

  m_eglext  = " ";
  m_eglext += eglQueryString(m_display, EGL_EXTENSIONS);
  m_eglext += " ";
  CLog::Log(LOGDEBUG, "EGL extensions:%s", m_eglext.c_str());

  // setup for vsync disabled
  eglSwapInterval(m_display, 0);

  CLog::Log(LOGINFO, "EGL window and context creation complete");

  return true;
}

bool CWinEGLPlatformGeneric::ReleaseSurface()
{
  EGLBoolean eglStatus;
  
  if (m_context != EGL_NO_CONTEXT)
  {
    eglStatus = eglDestroyContext(m_display, m_context);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL context");
    m_context = EGL_NO_CONTEXT;
  }

  if (m_display != EGL_NO_DISPLAY)
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  return true;
}

bool CWinEGLPlatformGeneric::ShowWindow(bool show)
{
  return true;
}

void CWinEGLPlatformGeneric::SwapBuffers()
{
  eglSwapBuffers(m_display, m_surface);
}

bool CWinEGLPlatformGeneric::SetVSync(bool enable)
{
  // depending how buffers are setup, eglSwapInterval
  // might fail so let caller decide if this is an error.
  return eglSwapInterval(m_display, enable ? 1 : 0);
}

bool CWinEGLPlatformGeneric::IsExtSupported(const char* extension)
{
  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != std::string::npos;
}

EGLNativeWindowType CWinEGLPlatformGeneric::getNativeWindow()
{
  // most egl platforms can handle EGLNativeWindowType == 0
  return 0;
}

EGLDisplay CWinEGLPlatformGeneric::GetEGLDisplay()
{
  return m_display;
}

EGLSurface CWinEGLPlatformGeneric::GetEGLSurface()
{
  return m_surface;
}

EGLContext CWinEGLPlatformGeneric::GetEGLContext()
{
  return m_context;
}

#endif
