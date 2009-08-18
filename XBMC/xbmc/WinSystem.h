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

#include "StdString.h"
#include "WinEvents.h"

typedef enum _WindowSystemType
{
  WINDOW_SYSTEM_WIN32,
  WINDOW_SYSTEM_SDL
} WindowSystemType;

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();
  WindowSystemType GetWinSystem() { return m_eWindowSystem; }


  virtual bool Create(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction){return false;}
  virtual bool Destroy(){ return false; }
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop){return false;}
  virtual bool SetFullScreen(bool fullScreen, int width, int height){return false;}
  virtual bool MoveWindow(int topLeft, int topRight){return false;}
  virtual bool CenterWindow(){return false;}

  virtual bool IsCreated(){ return false; }

  virtual bool IsFullScreen() { return m_bFullScreen; }
  virtual unsigned int GetWidth() { return m_nWidth; }
  virtual unsigned int GetHeight() { return m_nHeight; }
  virtual unsigned int GetDisplayFreq() { return m_nDisplayFreq; }

protected:
  // resize window based on dimensions, positions and full screen flag
  virtual bool Resize(){ return false; }

  WindowSystemType m_eWindowSystem;
  unsigned  int m_nWidth;
  unsigned m_nHeight;
  unsigned m_nTop;
  unsigned m_nLeft;
  bool m_bFullScreen;
  bool m_bCreated;
  unsigned int m_nDisplayFreq;
};

#endif // WINDOW_SYSTEM_H

#ifdef _WIN32
#include "WinSystemWin32.h"
#define CWinSystem CWinSystemWin32
#endif

#ifdef _LINUX
#include "WinSystemSDL.h"
#define CWinSystem CWinSystemSDL
#endif

