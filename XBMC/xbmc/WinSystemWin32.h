/*!
\file Surface.h
\brief
*/

#ifndef WINDOW_SYSTEM_WIN32_H
#define WINDOW_SYSTEM_WIN32_H

#pragma once

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
  virtual bool CreateNewWindow(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop);
  virtual void UpdateResolutions();
  virtual bool CenterWindow();
  
  // CWinSystemWin32
  HWND GetHwnd() { return m_hWnd; }

protected:
  virtual bool ResizeInternal();
  virtual bool UpdateResolutionsInternal();
  virtual bool CreateBlankWindow();
  virtual bool BlankNonActiveMonitor(bool bBlank);

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

