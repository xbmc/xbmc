#ifndef WINDOW_SYSTEM_X11_H
#define WINDOW_SYSTEM_X11_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system_gl.h"

#if defined(HAS_GLX)
#include <GL/glx.h>
#endif

#if defined(HAS_EGL)
#include <EGL/egl.h>
#endif

#include "windowing/WinSystem.h"
#include "utils/Stopwatch.h"
#include "threads/CriticalSection.h"
#include "settings/lib/ISettingCallback.h"

class IDispResource;

class CWinSystemX11 : public CWinSystemBase, public ISettingCallback
{
public:
  CWinSystemX11();
  virtual ~CWinSystemX11();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual int  GetNumScreens() { return 1; }
  virtual int  GetCurrentScreen() { return m_nScreen; }
  virtual void ShowOSMouse(bool show);
  virtual void ResetOSScreensaver();
  virtual bool EnableFrameLimiter();
  virtual void EnableSystemScreenSaver(bool bEnable);

  virtual void NotifyAppActiveChange(bool bActivated);
  virtual void NotifyAppFocusChange(bool bGaining);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  virtual bool HasCalibration(const RESOLUTION_INFO &resInfo);

  // Local to WinSystemX11 only
  Display*  GetDisplay() { return m_dpy; }
#if defined(HAS_GLX)
  GLXWindow GetWindow() { return m_glWindow; }
  GLXContext GetGlxContext() { return m_glContext; }
#endif
#if defined(HAS_EGL)
  EGLDisplay GetEGLDisplay() const { return m_eglDisplay;}
  EGLSurface GetEGLSurface() const { return m_eglSurface;}
  EGLContext GetEGLContext() const { return m_eglContext;}
#endif
  void NotifyXRREvent();
  void GetConnectedOutputs(std::vector<std::string> *outputs);
  bool IsCurrentOutput(std::string output);
  void RecreateWindow();
  int GetCrtc() { return m_crtc; }

protected:
  bool RefreshGlxContext(bool force);
  void OnLostDevice();
  bool SetWindow(int width, int height, bool fullscreen, const std::string &output);

  Window       m_glWindow, m_mainWindow;
#if defined(HAS_GLX)
  GLXContext   m_glContext;
#endif
#if defined(HAS_EGL)
  EGLDisplay            m_eglDisplay;
  EGLSurface            m_eglSurface;
  EGLContext            m_eglContext;
  EGLConfig             m_eglConfig;
#endif
  Display*     m_dpy;
  Cursor       m_invisibleCursor;
  Pixmap       m_icon;
  bool         m_bIsRotated;
  bool         m_bWasFullScreenBeforeMinimize;
  bool         m_minimized;
  bool         m_bIgnoreNextFocusMessage;
  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  std::string                  m_currentOutput;
  std::string                  m_userOutput;
  bool                         m_windowDirty;
  bool                         m_bIsInternalXrr;
  bool                         m_newGlContext;
  int m_MouseX, m_MouseY;
  int m_crtc;

private:
  bool IsSuitableVisual(XVisualInfo *vInfo);
  static int XErrorHandler(Display* dpy, XErrorEvent* error);
  bool CreateIconPixmap();
  bool HasWindowManager();
  void UpdateCrtc();

  CStopWatch m_screensaverReset;
};

#endif // WINDOW_SYSTEM_H

