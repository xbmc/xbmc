/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef WINDOW_SYSTEM_WIN32_H
#define WINDOW_SYSTEM_WIN32_H

#include "WinSystem.h"

enum MonitorType
{
  MONITOR_TYPE_PRIMARY = 1,
  MONITOR_TYPE_SECONDARY = 2
};

#define MAX_MONITORS_NUM 2

struct MONITOR_DETAILS
{
  MonitorType type;
  int ScreenWidth;
  int ScreenHeight;
  RECT MonitorRC;
  int RefreshRate;
  int Bpp;
  HMONITOR hMonitor;
  char MonitorName[128];
  char CardName[128];
  char DeviceName[128];
};

class CWinSystemWin32 : public CWinSystemBase
{
public:
  CWinSystemWin32();
  virtual ~CWinSystemWin32();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual bool CenterWindow();
  virtual void NotifyAppFocusChange(bool bGaining);
  virtual int GetNumScreens() { return m_nMonitorsCount; };
  virtual void ShowOSMouse(bool show);

  virtual bool Minimize();
  virtual bool Restore();
  virtual bool Hide();
  virtual bool Show(bool raise = true);

   // OS System screensaver
  virtual void EnableSystemScreenSaver(bool bEnable);
  virtual bool IsSystemScreenSaverEnabled();

  // CWinSystemWin32
  HWND GetHwnd() { return m_hWnd; }

protected:
  bool ChangeRefreshRate(int screen, float refresh);
  virtual bool ResizeInternal(bool forceRefresh = false);
  virtual bool UpdateResolutionsInternal();
  virtual bool CreateBlankWindow();
  virtual bool BlankNonActiveMonitor(bool bBlank);
  const MONITOR_DETAILS &GetMonitor(int screen) const;
  /*!
   \brief Adds a resolution to the list of resolutions if we don't already have it
   \param res resolution to add.
   */
  void AddResolution(const RESOLUTION_INFO &res);

  HWND m_hWnd;
  HWND m_hBlankWindow;
  HDC m_hDC;
  HINSTANCE m_hInstance;
  HICON m_hIcon;
  MONITOR_DETAILS m_MonitorsInfo[MAX_MONITORS_NUM];
  int m_nMonitorsCount;
  int m_nPrimary;
  int m_nSecondary;
};

extern HWND g_hWnd;

#endif // WINDOW_SYSTEM_H

