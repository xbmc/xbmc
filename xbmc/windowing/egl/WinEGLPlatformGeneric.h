#pragma once
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

#include "utils/StringUtils.h"

#include <EGL/egl.h>
#ifndef TARGET_WINDOWS
#include <EGL/eglext.h>
#endif

#include "guilib/gui3d.h"
#include "guilib/Resolution.h"

class CWinEGLPlatformGeneric
{
public:
  CWinEGLPlatformGeneric();
  virtual ~CWinEGLPlatformGeneric();

  virtual EGLNativeWindowType InitWindowSystem(EGLNativeDisplayType nativeDisplay, int width, int height, int bpp);
  virtual void DestroyWindowSystem(EGLNativeWindowType native_window);
  virtual bool SetDisplayResolution(RESOLUTION_INFO& res);
  virtual bool ClampToGUIDisplayLimits(int &width, int &height);
  virtual bool ProbeDisplayResolutions(std::vector<RESOLUTION_INFO> &resolutions);
  
  virtual bool InitializeDisplay();
  virtual bool UninitializeDisplay();
  virtual bool CreateWindow();
  virtual bool DestroyWindow();
  virtual bool BindSurface();
  virtual bool ReleaseSurface();
  
  virtual bool ShowWindow(bool show);
  virtual void SwapBuffers();
  virtual bool SetVSync(bool enable);
  virtual bool IsExtSupported(const char* extension);

  virtual EGLDisplay GetEGLDisplay();
  virtual EGLSurface GetEGLSurface();
  virtual EGLContext GetEGLContext();

  virtual bool                  FixedDesktop() { return true; }
  virtual RESOLUTION_INFO       GetDesktopRes() { return m_desktopRes; }
  virtual bool                  Support3D() { return false; }

protected:
  virtual EGLNativeWindowType getNativeWindow();

  EGLNativeWindowType   m_nativeWindow;
  EGLNativeDisplayType  m_nativeDisplay;
  EGLDisplay            m_display;
  EGLSurface            m_surface;
  EGLConfig             m_config;
  EGLContext            m_context;
  CStdString            m_eglext;
  int                   m_width;
  int                   m_height;
  RESOLUTION_INFO       m_desktopRes;
};
