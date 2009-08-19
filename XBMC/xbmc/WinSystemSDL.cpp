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

#include "stdafx.h"
#include "WinSystemSDL.h"
#include "WinEvents.h"

#ifdef _LINUX

CWinSystem g_WinSystem;

CWinSystemSDL::CWinSystemSDL()
: CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_SDL;
}

CWinSystemSDL::~CWinSystemSDL()
{
  Destroy();
};

bool CWinSystemSDL::Create(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;
  
  return true;
}

bool CWinSystemSDL::Destroy()
{
  return true;
}

bool CWinSystemSDL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  return true;
}

bool CWinSystemSDL::SetFullScreen(bool fullScreen, int width, int height)
{
  return true;
}

bool CWinSystemSDL::Resize()
{
  return true;
}

#endif

