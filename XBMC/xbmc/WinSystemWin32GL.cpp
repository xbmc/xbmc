/*!
\file Surface.h
\brief
*/



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
#include "WinSystemWin32GL.h"
#include "WIN32Util.h"

#ifdef HAS_GL

#pragma comment (lib,"opengl32.lib")
#pragma comment (lib,"glu32.lib")
#pragma comment (lib,"../../xbmc/lib/libglew/glew32.lib") 


CWinSystemWin32GL g_Windowing;


CWinSystemWin32GL::CWinSystemWin32GL()
{

}

CWinSystemWin32GL::~CWinSystemWin32GL()
{

}

bool CWinSystemWin32GL::InitRenderSystem()
{
  if(m_hWnd == NULL || m_hDC == NULL)
    return false;

  PIXELFORMATDESCRIPTOR pfd;
  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 16;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;

  // if these fail, wglCreateContext will also quietly fail
  int format;
  if ((format = ChoosePixelFormat(m_hDC, &pfd)) != 0)
    SetPixelFormat(m_hDC, format, &pfd);
  else
    return false;

  m_hglrc = wglCreateContext(m_hDC);
  if (!m_hglrc)
    return false;

  wglMakeCurrent(m_hDC, m_hglrc);

  if(!CRenderSystemGL::InitRenderSystem())
    return false;

  CWIN32Util::CheckGLVersion();

  return true;
}

void CWinSystemWin32GL::SetVSyncImpl(bool enable)
{

}

bool CWinSystemWin32GL::PresentRenderImpl()
{
  if(!m_bWindowCreated || !m_bRenderCreated)
    return false;

  SwapBuffers(m_hDC);

  return true;
}

bool CWinSystemWin32GL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin32::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);  

  return true;
}

bool CWinSystemWin32GL::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{
  CWinSystemWin32::SetFullScreen(fullScreen, screen, width, height, blankOtherDisplays, alwaysOnTop);
  CRenderSystemGL::ResetRenderSystem(width, height);  

  return true;
}

#endif // HAS_GL

