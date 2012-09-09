#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#if defined(TARGET_DARWIN_OSX)

#include "windowing/WinSystem.h"
#include "threads/CriticalSection.h"

typedef struct SDL_Surface SDL_Surface;

class IDispResource;
class CWinEventsOSX;

class CWinSystemOSX : public CWinSystemBase
{
public:
  CWinSystemOSX();
  virtual ~CWinSystemOSX();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
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

  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  
  virtual int GetNumScreens();

  void CheckDisplayChanging(u_int32_t flags);
  
  void* GetCGLContextObj();

protected:
  void* CreateWindowedContext(void* shareCtx);
  void* CreateFullScreenContext(int screen_index, void* shareCtx);
  void  GetScreenResolution(int* w, int* h, double* fps, int screenIdx);
  void  EnableVSync(bool enable); 
  bool  SwitchToVideoMode(int width, int height, double refreshrate, int screenIdx);
  void  FillInVideoModes();
  bool  FlushBuffer(void);
  bool  IsObscured(void);

  void* m_glContext;
  static void* m_lastOwnedContext;
  SDL_Surface* m_SDLSurface;
  CWinEventsOSX *m_osx_events;
  bool                         m_obscured;
  unsigned int                 m_obscured_timecheck;

  bool                         m_use_system_screensaver;
  bool                         m_can_display_switch;
  void                        *m_windowDidMove;
  void                        *m_windowDidReSize;

  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
};

#endif
