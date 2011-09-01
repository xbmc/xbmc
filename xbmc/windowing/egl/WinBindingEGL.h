/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#ifndef WINDOW_BINDING_EGL_H
#define WINDOW_BINDING_EGL_H

#include <EGL/egl.h>
#ifndef TARGET_WINDOWS
#include <EGL/eglext.h>
#endif

class CWinBindingEGL
{
public:
  CWinBindingEGL();
  ~CWinBindingEGL();

  bool CreateWindow(EGLNativeDisplayType nativeDisplay, EGLNativeWindowType nativeWindow);
  bool DestroyWindow();
  bool ReleaseSurface();

  EGLNativeWindowType GetNativeWindow() { return m_nativeWindow; }
  EGLNativeDisplayType GetNativeDisplay() { return m_nativeDisplay; }
  EGLDisplay GetDisplay() { return m_display; }
  EGLSurface GetSurface() { return m_surface; }
  EGLContext GetContext() { return m_context; }

protected:
  EGLNativeWindowType  m_nativeWindow;
  EGLNativeDisplayType m_nativeDisplay;
  EGLDisplay           m_display;
  EGLSurface           m_surface;
  EGLConfig            m_config;
  EGLContext           m_context;
};

#endif // WINDOW_BINDING_EGL_H

