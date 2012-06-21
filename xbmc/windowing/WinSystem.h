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

#ifndef WINDOW_SYSTEM_BASE_H
#define WINDOW_SYSTEM_BASE_H

#include "WinEvents.h"
#include "guilib/Resolution.h"
#include <vector>

typedef enum _WindowSystemType
{
  WINDOW_SYSTEM_WIN32,
  WINDOW_SYSTEM_OSX,
  WINDOW_SYSTEM_IOS,
  WINDOW_SYSTEM_X11,
  WINDOW_SYSTEM_SDL,
  WINDOW_SYSTEM_EGL,
  WINDOW_SYSTEM_ANDROID
} WindowSystemType;

struct RESOLUTION_WHR
{
  int width;
  int height;
  int flags; //< only D3DPRESENTFLAG_MODEMASK flags
  int ResInfo_Index;
};

struct REFRESHRATE
{
  float RefreshRate;
  int   ResInfo_Index;
};

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();
  WindowSystemType GetWinSystem() { return m_eWindowSystem; }

  // windowing interfaces
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem(){ return false; }
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction) = 0;
  virtual bool DestroyWindow(){ return false; }
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) = 0;
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) = 0;
  virtual bool MoveWindow(int topLeft, int topRight){return false;}
  virtual bool CenterWindow(){return false;}
  virtual bool IsCreated(){ return m_bWindowCreated; }
  virtual void NotifyAppFocusChange(bool bGaining) {}
  virtual void NotifyAppActiveChange(bool bActivated) {}
  virtual void ShowOSMouse(bool show) {};
  virtual bool HasCursor(){ return true; }
  //some plattforms have api for gesture inertial scrolling - default to false and use the InertialScrollingHandler
  virtual bool HasInertialGestures(){ return false; }

  //does the output expect limited color range (ie 16-235)
  virtual bool UseLimitedColor();

  virtual bool Minimize() { return false; }
  virtual bool Restore() { return false; }
  virtual bool Hide() { return false; }
  virtual bool Show(bool raise = true) { return false; }

  // notifications
  virtual void OnMove(int x, int y) {}

  // OS System screensaver
  virtual void EnableSystemScreenSaver(bool bEnable) {};
  virtual bool IsSystemScreenSaverEnabled() {return false;}
  virtual void ResetOSScreensaver() {};
  virtual bool EnableFrameLimiter() {return false;};

  // resolution interfaces
  unsigned int GetWidth() { return m_nWidth; }
  unsigned int GetHeight() { return m_nHeight; }
  virtual int GetNumScreens() { return 0; }
  virtual int GetCurrentScreen() { return 0; }
  bool IsFullScreen() { return m_bFullScreen; }
  virtual void UpdateResolutions();
  void SetWindowResolution(int width, int height);
  int DesktopResolution(int screen);
  std::vector<RESOLUTION_WHR> ScreenResolutions(int screen, float refreshrate);
  std::vector<REFRESHRATE> RefreshRates(int screen, int width, int height, uint32_t dwFlags);
  REFRESHRATE DefaultRefreshRate(int screen, std::vector<REFRESHRATE> rates);
  virtual bool HasCalibration(const RESOLUTION_INFO &resInfo) { return true; };

  // text input interface
  virtual void EnableTextInput(bool bEnable) {}
  virtual bool IsTextInputEnabled() { return false; }

  std::string GetClipboardText(void);

protected:
  void UpdateDesktopResolution(RESOLUTION_INFO& newRes, int screen, int width, int height, float refreshRate, uint32_t dwFlags = 0);

  WindowSystemType  m_eWindowSystem;
  int               m_nWidth;
  int               m_nHeight;
  int               m_nTop;
  int               m_nLeft;
  bool              m_bWindowCreated;
  bool              m_bFullScreen;
  int               m_nScreen;
  bool              m_bBlankOtherDisplay;
  float             m_fRefreshRate;
};


#endif // WINDOW_SYSTEM_H
