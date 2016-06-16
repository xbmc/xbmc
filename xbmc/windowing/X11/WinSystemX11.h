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

#pragma once

#include <string>
#include <vector>

#include "windowing/WinSystem.h"
#include "utils/Stopwatch.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "settings/lib/ISettingCallback.h"
#include "X11/Xlib.h"
#include "X11/Xutil.h"

class IDispResource;

class CWinSystemX11 : public CWinSystemBase
{
public:
  CWinSystemX11();
  virtual ~CWinSystemX11();

  // CWinSystemBase
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  int  GetNumScreens() override { return 1; }
  int  GetCurrentScreen() override { return m_nScreen; }
  void ShowOSMouse(bool show) override;
  void ResetOSScreensaver() override;
  void EnableSystemScreenSaver(bool bEnable) override;

  void NotifyAppActiveChange(bool bActivated) override;
  void NotifyAppFocusChange(bool bGaining) override;

  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  bool HasCalibration(const RESOLUTION_INFO &resInfo) override;

  // Local to WinSystemX11 only
  Display*  GetDisplay() { return m_dpy; }
  void NotifyXRREvent();
  void GetConnectedOutputs(std::vector<std::string> *outputs);
  bool IsCurrentOutput(std::string output);
  void RecreateWindow();
  int GetCrtc() { return m_crtc; }

protected:
  virtual bool SetWindow(int width, int height, bool fullscreen, const std::string &output, int *winstate = NULL) = 0;
  virtual XVisualInfo* GetVisual() = 0;

  void OnLostDevice();

  Window m_glWindow, m_mainWindow;
  Display *m_dpy;
  Cursor m_invisibleCursor;
  Pixmap m_icon;
  bool m_bIsRotated;
  bool m_bWasFullScreenBeforeMinimize;
  bool m_minimized;
  bool m_bIgnoreNextFocusMessage;
  CCriticalSection m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  bool m_delayDispReset;
  XbmcThreads::EndTime m_dispResetTimer;
  std::string m_currentOutput;
  std::string m_userOutput;
  bool m_windowDirty;
  bool m_bIsInternalXrr;
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
