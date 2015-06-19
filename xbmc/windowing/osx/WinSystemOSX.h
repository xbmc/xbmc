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

#if defined(TARGET_DARWIN_OSX)

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
  virtual void OnTimeout();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  bool         ResizeWindowInternal(int newWidth, int newHeight, int newLeft, int newTop, void *additional);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual void NotifyAppFocusChange(bool bGaining);
  virtual void ShowOSMouse(bool show);
  virtual bool Minimize();
  virtual bool Restore();
  virtual bool Hide();
  virtual bool Show(bool raise = true);
  virtual void OnMove(int x, int y);

  virtual void EnableSystemScreenSaver(bool bEnable);
  virtual bool IsSystemScreenSaverEnabled();
  virtual void ResetOSScreensaver();
  virtual bool EnableFrameLimiter();

  virtual void EnableTextInput(bool bEnable);
  virtual bool IsTextInputEnabled();

  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  
  virtual int GetNumScreens();
  virtual int GetCurrentScreen();
  virtual double GetCurrentRefreshrate() { return m_refreshRate; }
  
  void        WindowChangedScreen();

  void        AnnounceOnLostDevice();
  void        AnnounceOnResetDevice();
  void        StartLostDeviceTimer();
  void        StopLostDeviceTimer();
  
  void* GetCGLContextObj();
  void* GetNSOpenGLContext();

  std::string GetClipboardText(void);

protected:
  void  HandlePossibleRefreshrateChange();
  void* CreateWindowedContext(void* shareCtx);
  void* CreateFullScreenContext(int screen_index, void* shareCtx);
  void  GetScreenResolution(int* w, int* h, double* fps, int screenIdx);
  void  EnableVSync(bool enable); 
  bool  SwitchToVideoMode(int width, int height, double refreshrate, int screenIdx);
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

  bool                         m_use_system_screensaver;
  bool                         m_can_display_switch;
  bool                         m_movedToOtherScreen;
  int                          m_lastDisplayNr;
  void                        *m_windowDidMove;
  void                        *m_windowDidReSize;
  void                        *m_windowChangedScreen;
  double                       m_refreshRate;

  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  CTimer                       m_lostDeviceTimer;
};

#endif
