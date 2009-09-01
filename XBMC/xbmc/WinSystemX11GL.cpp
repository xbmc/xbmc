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

#include "WinSystemX11GL.h"

CWinSystemX11GL g_Windowing;

CWinSystemX11GL::CWinSystemX11GL()
{
}

CWinSystemX11GL::~CWinSystemX11GL()
{
}

bool CWinSystemX11GL::PresentRenderImpl()
{        
  glXSwapBuffers(m_dpy, m_glWindow);
	  
  return true;
}

void CWinSystemX11GL::SetVSyncImpl(bool enable)
{

}

bool CWinSystemX11GL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemX11::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);  

  return true;
}

bool CWinSystemX11GL::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{
  CWinSystemX11::SetFullScreen(fullScreen, screen, width, height, blankOtherDisplays, alwaysOnTop);
  CRenderSystemGL::ResetRenderSystem(width, height);  
  
  return true;
}

#endif
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

#include "WinSystemX11GL.h"

CWinSystemX11GL g_Windowing;

CWinSystemX11GL::CWinSystemX11GL()
{
}

CWinSystemX11GL::~CWinSystemX11GL()
{
}

bool CWinSystemX11GL::PresentRenderImpl()
{        
  glXSwapBuffers(m_dpy, m_glWindow);
	  
  return true;
}

void CWinSystemX11GL::SetVSyncImpl(bool enable)
{

}

bool CWinSystemX11GL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemX11::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);  

  return true;
}

bool CWinSystemX11GL::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{
  CWinSystemX11::SetFullScreen(fullScreen, screen, width, height, blankOtherDisplays, alwaysOnTop);
  CRenderSystemGL::ResetRenderSystem(width, height);  
  
  return true;
}

#endif
