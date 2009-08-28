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

#ifdef HAS_GLX

#include <SDL/SDL_image.h>
#include "WinSystemX11.h"
#include "SpecialProtocol.h"
#include "Settings.h"
#include "Texture.h"


CWinSystemX11::CWinSystemX11() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_X11;
  m_glContext = 0;
}

CWinSystemX11::~CWinSystemX11()
{
  DestroyWindowSystem();
};

bool CWinSystemX11::InitWindowSystem()
{
  if (!CWinSystemBase::InitWindowSystem())
    return false;
  
  return true;
}

bool CWinSystemX11::DestroyWindowSystem()
{  
  // TODO

  return true;
}

bool CWinSystemX11::CreateNewWindow(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;

  // TODO
  
  m_bWindowCreated = true;

  return true;
}

bool CWinSystemX11::DestroyWindow()
{
  return true;
}
    
bool CWinSystemX11::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;

  // TODO
  
  return true;
}

bool CWinSystemX11::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{  
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;
  
  // TODO
  
  return true;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();
  
  // TODO 
}

#endif
