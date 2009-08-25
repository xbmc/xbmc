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

#ifndef WINDOW_SYSTEM_H
#define WINDOW_SYSTEM_H

#pragma once

#include <stdio.h>
#include <vector>
#include "StdString.h"
#include "WinEvents.h"
#include "WinSystem.h"

using namespace std;

enum RESOLUTION {
  INVALID = -1,
  HDTV_1080i = 0,
  HDTV_720p = 1,
  HDTV_480p_4x3 = 2,
  HDTV_480p_16x9 = 3,
  NTSC_4x3 = 4,
  NTSC_16x9 = 5,
  PAL_4x3 = 6,
  PAL_16x9 = 7,
  PAL60_4x3 = 8,
  PAL60_16x9 = 9,
  AUTORES = 10,
  WINDOW = 11,
  DESKTOP = 12,
  CUSTOM = 13
};

enum VSYNC {
  VSYNC_DISABLED = 0,
  VSYNC_VIDEO = 1,
  VSYNC_ALWAYS = 2,
  VSYNC_DRIVER = 3
};

enum DISPLAYS
{
  PRIMARY_MONITOR = 0,
  SECONDARY_MONITOR = 1
};

struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
};

struct RESOLUTION_INFO
{
  OVERSCAN Overscan;
  bool bFullScreen;
  DISPLAYS iScreen;
  int iWidth;
  int iHeight;
  int iSubtitles;
  DWORD dwFlags;
  float fPixelRatio;
  float fRefreshRate;
  char strMode[48];
  char strOutput[32];
  char strId[16];
};

typedef std::vector<RESOLUTION_INFO> ResVector;

typedef enum _WindowSystemType
{
  WINDOW_SYSTEM_WIN32,
  WINDOW_SYSTEM_OSX,
  WINDOW_SYSTEM_X11,
  WINDOW_SYSTEM_SDL
} WindowSystemType;

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();
  WindowSystemType GetWinSystem() { return m_eWindowSystem; }

  // windowing interfaces
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem() = 0;
  virtual bool CreateNewWindow(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction) = 0;
  virtual bool DestroyWindow() = 0;
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) = 0;
  virtual bool SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop) = 0;
  virtual bool MoveWindow(int topLeft, int topRight){return false;}
  virtual bool CenterWindow(){return false;}
  virtual bool IsCreated(){ return m_bWindowCreated; }

  // resolution interfaces
  virtual bool IsFullScreen() { return m_bFullScreen; }
  virtual unsigned int GetWidth() { return m_nWidth; }
  virtual unsigned int GetHeight() { return m_nHeight; }
  virtual void GetResolutions(ResVector& vec);
  virtual void GetDesktopRes(RESOLUTION_INFO& desktopRes) = 0;
  virtual bool IsValidResolution(RESOLUTION_INFO res);
  
protected:
  // resize window based on dimensions, positions and full screen flag
  virtual bool Resize(){ return false; }
  virtual void UpdateResolutions();
  virtual void AddNewResolution(RESOLUTION_INFO newRes);
  
  WindowSystemType m_eWindowSystem;
  unsigned int m_nWidth;
  unsigned int m_nHeight;
  unsigned int m_nTop;
  unsigned int m_nLeft;
  bool m_bFullScreen;
  bool m_bWindowCreated;

  RESOLUTION_INFO m_DesktopRes;
  int m_nCurrentResolution;
  ResVector m_VecResInfo;
};


#endif // WINDOW_SYSTEM_H
