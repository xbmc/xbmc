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

#include "WinEvents.h"

enum RESOLUTION {
  RES_INVALID = -1,
  RES_HDTV_1080i = 0,
  RES_HDTV_720p = 1,
  RES_HDTV_480p_4x3 = 2,
  RES_HDTV_480p_16x9 = 3,
  RES_NTSC_4x3 = 4,
  RES_NTSC_16x9 = 5,
  RES_PAL_4x3 = 6,
  RES_PAL_16x9 = 7,
  RES_PAL60_4x3 = 8,
  RES_PAL60_16x9 = 9,
  RES_AUTORES = 10,
  RES_WINDOW = 11,
  RES_DESKTOP = 12,
  RES_CUSTOM = 13
};

enum VSYNC {
  VSYNC_DISABLED = 0,
  VSYNC_VIDEO = 1,
  VSYNC_ALWAYS = 2,
  VSYNC_DRIVER = 3
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
  int iScreen;
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
  virtual bool DestroyWindowSystem(){ return false; }
  virtual bool CreateNewWindow(CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction) = 0;
  virtual bool DestroyWindow(){ return false; }
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) = 0;
  virtual bool SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop) = 0;
  virtual bool MoveWindow(int topLeft, int topRight){return false;}
  virtual bool CenterWindow(){return false;}
  virtual bool IsCreated(){ return m_bWindowCreated; }

  // resolution interfaces
  unsigned int GetWidth() { return m_nWidth; }
  unsigned int GetHeight() { return m_nHeight; }
  bool IsFullScreen() { return m_bFullScreen; } 

  virtual void UpdateResolutions();
  
protected:
  void UpdateDesktopResolution(RESOLUTION_INFO& newRes, int screen, int width, int height, float refreshRate);
  
  WindowSystemType m_eWindowSystem;
  unsigned int m_nWidth;
  unsigned int m_nHeight;
  unsigned int m_nTop;
  unsigned int m_nLeft;
  bool m_bWindowCreated;
  bool m_bFullScreen;
  int m_nScreen;
  bool m_bBlankOtherDisplay;
};


#endif // WINDOW_SYSTEM_H
