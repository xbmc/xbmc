/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "utils/Stopwatch.h"
#include "windowing/WinSystem.h"

#include <string>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

class IDispResource;

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

class CWinEventsX11;

class CWinSystemX11 : public CWinSystemBase
{
public:
  CWinSystemX11();
  ~CWinSystemX11() override;

  const std::string GetName() override { return "x11"; }

  // CWinSystemBase
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void FinishWindowResize(int newWidth, int newHeight) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  void ShowOSMouse(bool show) override;

  void NotifyAppActiveChange(bool bActivated) override;
  void NotifyAppFocusChange(bool bGaining) override;

  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  void Register(IDispResource *resource) override;
  void Unregister(IDispResource *resource) override;
  bool HasCalibration(const RESOLUTION_INFO &resInfo) override;
  bool UseLimitedColor() override;

  std::vector<std::string> GetConnectedOutputs() override;

  // Local to WinSystemX11 only
  Display*  GetDisplay() { return m_dpy; }
  int GetScreen() { return m_screen; }
  void NotifyXRREvent();
  bool IsCurrentOutput(const std::string& output);
  void RecreateWindow();
  int GetCrtc() { return m_crtc; }

  // winevents override
  bool MessagePump() override;

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;

  virtual bool SetWindow(int width, int height, bool fullscreen, const std::string &output, int *winstate = NULL) = 0;
  virtual XVisualInfo* GetVisual() = 0;

  void OnLostDevice();

  Window m_glWindow = 0, m_mainWindow = 0;
  int m_screen = 0;
  Display *m_dpy;
  Cursor m_invisibleCursor = 0;
  Pixmap m_icon;
  bool m_bIsRotated;
  bool m_bWasFullScreenBeforeMinimize;
  bool m_minimized;
  bool m_bIgnoreNextFocusMessage;
  CCriticalSection m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  bool m_delayDispReset;
  XbmcThreads::EndTime<> m_dispResetTimer;
  std::string m_currentOutput;
  std::string m_userOutput;
  bool m_windowDirty;
  bool m_bIsInternalXrr;
  int m_MouseX, m_MouseY;
  int m_crtc;
  CWinEventsX11 *m_winEventsX11;

private:
  bool IsSuitableVisual(XVisualInfo *vInfo);
  static int XErrorHandler(Display* dpy, XErrorEvent* error);
  bool CreateIconPixmap();
  bool HasWindowManager();
  void UpdateCrtc();
};

}
}
}
