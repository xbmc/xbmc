/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include "windowing/WinSystem.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"

typedef struct SDL_Surface SDL_Surface;

class IDispResource;
class CWinEventsOSX;

class CWinSystemOSX : public CWinSystemBase, public ITimerCallback
{
public:

  CWinSystemOSX();
  virtual ~CWinSystemOSX();

  // ITimerCallback interface
  virtual void OnTimeout() override;

  // CWinSystemBase
  virtual bool InitWindowSystem() override;
  virtual bool DestroyWindowSystem() override;
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  virtual bool DestroyWindow() override;
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool         ResizeWindowInternal(int newWidth, int newHeight, int newLeft, int newTop, void *additional);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  virtual void UpdateResolutions() override;
  virtual void NotifyAppFocusChange(bool bGaining) override;
  virtual void ShowOSMouse(bool show) override;
  virtual bool Minimize() override;
  virtual bool Restore() override;
  virtual bool Hide() override;
  virtual bool Show(bool raise = true) override;
  virtual void OnMove(int x, int y) override;

  virtual std::string GetClipboardText(void) override;

  void Register(IDispResource *resource) override;
  void Unregister(IDispResource *resource) override;

  virtual std::unique_ptr<CVideoSync> GetVideoSync(void *clock) override;

  void        WindowChangedScreen();

  void        AnnounceOnLostDevice();
  void        AnnounceOnResetDevice();
  void        HandleOnResetDevice();
  void        StartLostDeviceTimer();
  void        StopLostDeviceTimer();

  void* GetCGLContextObj();
  void* GetNSOpenGLContext();
  void GetConnectedOutputs(std::vector<std::string> *outputs);

  // winevents override
  bool MessagePump() override;

protected:
  virtual std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;

  void  HandlePossibleRefreshrateChange();
  void* CreateWindowedContext(void* shareCtx);
  void* CreateFullScreenContext(int screen_index, void* shareCtx);
  void  GetScreenResolution(int* w, int* h, double* fps, int screenIdx);
  void  EnableVSync(bool enable);
  bool  SwitchToVideoMode(int width, int height, double refreshrate);
  void  FillInVideoModes();
  bool  FlushBuffer(void);
  bool  IsObscured(void);
  void  StartTextInput();
  void  StopTextInput();

  void* m_glContext;
  static void* m_lastOwnedContext;
  SDL_Surface* m_SDLSurface;
  CWinEventsOSX *m_osx_events;
  bool                         m_obscured;
  unsigned int                 m_obscured_timecheck;

  bool                         m_movedToOtherScreen;
  int                          m_lastDisplayNr;
  void                        *m_windowDidMove;
  void                        *m_windowDidReSize;
  void                        *m_windowChangedScreen;
  double                       m_refreshRate;

  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  CTimer                       m_lostDeviceTimer;
  bool                         m_delayDispReset;
  XbmcThreads::EndTime         m_dispResetTimer;
  int m_updateGLContext = 0;
};
