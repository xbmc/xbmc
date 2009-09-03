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

#ifndef WINDOW_SYSTEM_BASE_H
#define WINDOW_SYSTEM_BASE_H

#include "WinEvents.h"
#include "Resolution.h"

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
  virtual bool CreateNewWindow(const CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction) = 0;
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
