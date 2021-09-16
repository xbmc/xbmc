/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "threads/Timer.h"
#include "windowing/WinSystem.h"

#include <chrono>
#include <string>
#include <vector>

typedef struct SDL_Surface SDL_Surface;

class IDispResource;
class CWinEventsOSX;
class CWinSystemOSXImpl;
#ifdef __OBJC__
@class NSOpenGLContext;
#endif

class CWinSystemOSX : public CWinSystemBase, public ITimerCallback
{
public:

  CWinSystemOSX();
  ~CWinSystemOSX() override;

  // ITimerCallback interface
  void OnTimeout() override;

  // CWinSystemBase
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  void NotifyAppFocusChange(bool bGaining) override;
  void ShowOSMouse(bool show) override;
  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  void OnMove(int x, int y) override;

  std::string GetClipboardText() override;

  void Register(IDispResource *resource) override;
  void Unregister(IDispResource *resource) override;

  std::unique_ptr<CVideoSync> GetVideoSync(void* clock) override;

  std::vector<std::string> GetConnectedOutputs() override;

  void        WindowChangedScreen();

  void        AnnounceOnLostDevice();
  void        AnnounceOnResetDevice();
  void        HandleOnResetDevice();
  void        StartLostDeviceTimer();
  void        StopLostDeviceTimer();

  void* GetCGLContextObj();
#ifdef __OBJC__
  NSOpenGLContext* GetNSOpenGLContext();
#else
  void* GetNSOpenGLContext();
#endif

  // winevents override
  bool MessagePump() override;

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;

  void  HandlePossibleRefreshrateChange();
  void  GetScreenResolution(int* w, int* h, double* fps, int screenIdx);
  void  EnableVSync(bool enable);
  bool  SwitchToVideoMode(int width, int height, double refreshrate);
  void  FillInVideoModes();
  bool  FlushBuffer(void);
  bool  IsObscured(void);
  void  StartTextInput();
  void  StopTextInput();

  std::unique_ptr<CWinSystemOSXImpl> m_impl;
  SDL_Surface* m_SDLSurface;
  CWinEventsOSX *m_osx_events;
  bool                         m_obscured;
  std::chrono::time_point<std::chrono::steady_clock> m_obscured_timecheck;

  bool                         m_movedToOtherScreen;
  int                          m_lastDisplayNr;
  double                       m_refreshRate;

  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  CTimer                       m_lostDeviceTimer;
  bool                         m_delayDispReset;
  XbmcThreads::EndTime         m_dispResetTimer;
  int m_updateGLContext = 0;
};
