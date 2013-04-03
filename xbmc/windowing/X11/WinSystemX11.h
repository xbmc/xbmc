#ifndef WINDOW_SYSTEM_X11_H
#define WINDOW_SYSTEM_X11_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system_gl.h"
#include <GL/glx.h>

#include "windowing/WinSystem.h"
#include "utils/Stopwatch.h"
#include "threads/CriticalSection.h"

class IDispResource;

class CWinSystemX11 : public CWinSystemBase
{
public:
  CWinSystemX11();
  virtual ~CWinSystemX11();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual int  GetNumScreens();
  virtual int  GetCurrentScreen();
  virtual void ShowOSMouse(bool show);
  virtual void ResetOSScreensaver();
  virtual bool EnableFrameLimiter();

  virtual void OnMove(int x, int y)
  {
    if(!m_bFullScreen)
    {
      m_nLeft = x;
      m_nTop  = y;
    }
  }

  virtual void NotifyAppFocusChange(bool bGaining);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

  // Local to WinSystemX11 only
  Display*  GetDisplay() { return m_dpy; }
  GLXWindow GetWindow() { return m_glWindow; }
  XVisualInfo* GetVisual() { return m_visual; }
  void NotifyXRREvent();

  bool IsWindowManagerControlled() { return m_wm_controlled; }

protected:
  void ProbeWindowManager();
  void RefreshWindowState();
  void CheckDisplayEvents();
  void OnLostDevice();
  void OnResetDevice();

  XVisualInfo* m_visual;
  GLXContext   m_glContext;
  GLXWindow    m_glWindow;
  Window       m_wmWindow;
  Display*     m_dpy;

  Cursor       m_invisibleCursor;
  Pixmap       m_icon;
  bool         m_bWasFullScreenBeforeMinimize;
  bool         m_minimized;
  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  uint64_t                     m_dpyLostTime;
  CStdString                   m_outputName;
  int                          m_outputIndex;

  bool         m_wm;
  CStdString   m_wm_name;
  bool         m_wm_fullscreen;
  bool         m_wm_controlled;

  Atom m_NET_SUPPORTING_WM_CHECK;
  Atom m_NET_WM_STATE;
  Atom m_NET_WM_STATE_FULLSCREEN;
  Atom m_NET_WM_STATE_MAXIMIZED_VERT;
  Atom m_NET_WM_STATE_MAXIMIZED_HORZ;
  Atom m_NET_SUPPORTED;
  Atom m_NET_WM_NAME;
  Atom m_WM_DELETE_WINDOW;

private:
  bool IsSuitableVisual(XVisualInfo *vInfo);
  static int XErrorHandler(Display* dpy, XErrorEvent* error);
  bool SetResolution(RESOLUTION_INFO& res, bool fullScreen);

  CStopWatch m_screensaverReset;
};

#endif // WINDOW_SYSTEM_H

