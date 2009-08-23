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
  virtual bool SetFullScreen(bool fullScreen, int width, int height);
  virtual bool Destroy();
  virtual void UpdateResolutions();
  
  // CWinSystemWin32
  HWND GetHwnd() { return m_hWnd; }

protected:
  void GetDesktopRes(RESOLUTION_INFO& desktopRes);
  virtual bool Resize();

  HWND m_hWnd;
  HDC m_hDC;
  HINSTANCE m_hInstance; 
  HICON m_hIcon;
};

#endif // WINDOW_SYSTEM_H

