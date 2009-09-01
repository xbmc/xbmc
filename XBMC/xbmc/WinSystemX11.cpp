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

#include <SDL/SDL_syswm.h>
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
  m_dpy = XOpenDisplay(0);
  if (!m_dpy)
  {
	  CLog::Log(LOGERROR, "GLX Error: No Display found");
	  return false;
  }
	
  SDL_EnableUNICODE(1);
  // set repeat to 10ms to ensure repeat time < frame time
  // so that hold times can be reliably detected
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);

  if (!CWinSystemBase::InitWindowSystem())
    return false;
   
  return true;
}

bool CWinSystemX11::DestroyWindowSystem()
{  
  // TODO

  return true;
}

bool CWinSystemX11::CreateNewWindow(const CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  
  // Enable vertical sync to avoid any tearing.
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);  
  
  m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, SDL_OPENGL);
  if (!m_SDLSurface)
  {
    return false;
  }
  
  RefreshGlxContext();
  
  CTexture iconTexture;
  iconTexture.LoadFromFile("special://xbmc/media/icon.png");
  SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture.GetPixels(), iconTexture.GetWidth(), iconTexture.GetHeight(), iconTexture.GetBPP(), iconTexture.GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
    
  SDL_WM_SetCaption("XBMC Media Center", NULL);
  
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

  int options = SDL_OPENGL;
  if (m_bFullScreen)
  {
    options |= SDL_FULLSCREEN;
  }
  
  m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, options);
  if (!m_SDLSurface)
  {
    return false;
  }
  
  RefreshGlxContext();
  
  return true;
}

bool CWinSystemX11::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{  
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;
  
  ResizeWindow(m_nWidth, m_nHeight, -1, -1);
  
  return true;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();
  
  int x11screen = DefaultScreen(m_dpy);
  int w = DisplayWidth(m_dpy, x11screen);
  int h = DisplayHeight(m_dpy, x11screen);
	
  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, 0.0f);
}

bool CWinSystemX11::RefreshGlxContext()
{
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  SDL_GetWMInfo(&info);
  m_glWindow = info.info.x11.window;
  
  GLXFBConfig *fbConfigs = 0;
  XVisualInfo *vInfo = NULL;
  int num = 0;

  int doubleVisAttributes[] =
	{
	  GLX_RENDER_TYPE, GLX_RGBA_BIT,
	  GLX_RED_SIZE, 8,
	  GLX_GREEN_SIZE, 8,
	  GLX_BLUE_SIZE, 8,
	  GLX_ALPHA_SIZE, 8,
	  GLX_DEPTH_SIZE, 8,
	  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	  GLX_DOUBLEBUFFER, True,
	  None
	};

  // query compatible framebuffers based on double buffered attributes
  fbConfigs = glXChooseFBConfig(m_dpy, DefaultScreen(m_dpy), doubleVisAttributes, &num);
  if (fbConfigs == NULL)
  {
	CLog::Log(LOGERROR, "GLX Error: No compatible framebuffers found");
	return false;
  }

  for (int i = 0; i < num; i++)
  {
	// obtain the xvisual from the first compatible framebuffer
	vInfo = glXGetVisualFromFBConfig(m_dpy, fbConfigs[i]);
	if (vInfo->depth == 24)
	{
	  CLog::Log(LOGNOTICE, "Using fbConfig[%i]",i);
	  break;
	}
  }

  if (!vInfo) 
  {
	CLog::Log(LOGERROR, "GLX Error: vInfo is NULL!");
	return false;
  }

  m_glContext = glXCreateContext(m_dpy, vInfo, NULL, True);

  XFree(fbConfigs);
  XFree(vInfo);

  // success?
  if (!m_glContext)
  {
	CLog::Log(LOGERROR, "GLX Error: Could not create context");
	return false;
  }

  // make this context current
  glXMakeCurrent(m_dpy, m_glWindow, m_glContext);
  
  return true;
}

#endif
