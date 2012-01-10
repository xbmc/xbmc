/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#ifdef HAS_EGL

#include "WinBindingEGL.h"
#include "utils/log.h"

#include <string>

CWinBindingEGL::CWinBindingEGL()
{
  m_surface = EGL_NO_SURFACE;
  m_context = EGL_NO_CONTEXT;
  m_display = EGL_NO_DISPLAY;
}

CWinBindingEGL::~CWinBindingEGL()
{
  DestroyWindow();
}

bool CWinBindingEGL::ReleaseSurface()
{
  EGLBoolean eglStatus;

  if (m_surface == EGL_NO_SURFACE)
  {
    return true;
  }

  eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  eglStatus = eglDestroySurface(m_display, m_surface);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "Error destroying EGL surface");
    return false;
  }

  m_surface = EGL_NO_SURFACE;

  return true;
}

bool CWinBindingEGL::CreateWindow(EGLNativeDisplayType nativeDisplay, EGLNativeWindowType nativeWindow)
{
  EGLBoolean eglStatus;
  EGLint     configCount;
  EGLConfig* configList = NULL;  

  m_nativeDisplay = nativeDisplay;
  m_nativeWindow  = nativeWindow;

  m_display = eglGetDisplay(nativeDisplay);
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
  {
    ReleaseSurface();
  }

  m_surface = eglCreateWindowSurface(m_display, m_config, m_nativeWindow, NULL);
  if (!m_surface)
  { 
    CLog::Log(LOGERROR, "EGL couldn't create window surface");
    return false;
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
 
  free(configList);

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

bool CWinBindingEGL::DestroyWindow()
{
  EGLBoolean eglStatus;
  if (m_context != EGL_NO_CONTEXT)
  {
    eglStatus = eglDestroyContext(m_display, m_context);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL context");
    m_context = EGL_NO_CONTEXT;
  }

  if (m_surface != EGL_NO_SURFACE)
  {
    eglStatus = eglDestroySurface(m_display, m_surface);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL surface");
    m_surface = EGL_NO_SURFACE;
  }

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

void CWinBindingEGL::SwapBuffers()
{
  eglSwapBuffers(m_display, m_surface);
}

bool CWinBindingEGL::SetVSync(bool enable)
{
  // depending how buffers are setup, eglSwapInterval
  // might fail so let caller decide if this is an error.
  return eglSwapInterval(m_display, enable ? 1 : 0);
}

bool CWinBindingEGL::IsExtSupported(const char* extension)
{
  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != std::string::npos;
}

EGLNativeWindowType CWinBindingEGL::GetNativeWindow()
{
  return m_nativeWindow;
}

EGLNativeDisplayType CWinBindingEGL::GetNativeDisplay()
{
  return m_nativeDisplay;
}

EGLDisplay CWinBindingEGL::GetDisplay()
{
  return m_display;
}

EGLSurface CWinBindingEGL::GetSurface()
{
  return m_surface;
}

EGLContext CWinBindingEGL::GetContext()
{
  return m_context;
}

#endif
