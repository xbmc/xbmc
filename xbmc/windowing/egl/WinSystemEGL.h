#ifndef WINDOW_SYSTEM_EGL_H
#define WINDOW_SYSTEM_EGL_H

#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>

#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GlobalsHandling.h"
#include <EGL/egl.h>
#include "windowing/WinSystem.h"
#include "threads/SystemClock.h"

class CEGLWrapper;
class IDispResource;

class CWinSystemEGL : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemEGL();
  virtual ~CWinSystemEGL();

  virtual bool  InitWindowSystem();
  virtual bool  DestroyWindowSystem();
  virtual bool  CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool  DestroyWindow();
  virtual bool  ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool  SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void  UpdateResolutions();
  virtual bool  IsExtSupported(const char* extension);
  virtual bool  CanDoWindowed() { return false; }

  virtual void  ShowOSMouse(bool show);
  virtual bool  HasCursor();

  virtual void  NotifyAppActiveChange(bool bActivated);

  virtual bool  Minimize();
  virtual bool  Restore() ;
  virtual bool  Hide();
  virtual bool  Show(bool raise = true);
  virtual void  Register(IDispResource *resource);
  virtual void  Unregister(IDispResource *resource);

  virtual bool  ClampToGUIDisplayLimits(int &width, int &height);

  EGLConfig     GetEGLConfig();

  EGLDisplay    GetEGLDisplay();
  EGLContext    GetEGLContext();
protected:
  virtual void  PresentRenderImpl(bool rendered);
  virtual void  SetVSyncImpl(bool enable);

  bool          CreateWindow(RESOLUTION_INFO &res);

  int                   m_displayWidth;
  int                   m_displayHeight;

  EGLDisplay            m_display;
  EGLSurface            m_surface;
  EGLContext            m_context;
  EGLConfig             m_config;
  RENDER_STEREO_MODE    m_stereo_mode;

  CEGLWrapper           *m_egl;
  std::string           m_extensions;
  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  bool m_delayDispReset;
  XbmcThreads::EndTime m_dispResetTimer;
};

XBMC_GLOBAL_REF(CWinSystemEGL,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemEGL)

#endif // WINDOW_SYSTEM_EGL_H
