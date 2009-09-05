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
#include "utils/log.h"

static int doubleVisAttributes[] =
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

CWinSystemX11::CWinSystemX11() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_X11;
  m_glContext = NULL;
  m_SDLSurface = NULL;
  m_dpy = NULL;
}

CWinSystemX11::~CWinSystemX11()
{
  DestroyWindowSystem();
}

bool CWinSystemX11::InitWindowSystem()
{
  if ((m_dpy = XOpenDisplay(NULL)))
  {

    SDL_EnableUNICODE(1);
    // set repeat to 10ms to ensure repeat time < frame time
    // so that hold times can be reliably detected
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    return CWinSystemBase::InitWindowSystem();
  }
  else
    CLog::Log(LOGERROR, "GLX Error: No Display found");
  
  return false;
}

bool CWinSystemX11::DestroyWindowSystem()
{
  if (m_dpy)
  {
    if (m_glContext)
      glXDestroyContext(m_dpy, m_glContext);
    XCloseDisplay(m_dpy);
  }

  // m_SDLSurface is free()'d by SDL_Quit().

  return true;
}

bool CWinSystemX11::CreateNewWindow(const CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;

  if ((m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, SDL_OPENGL | (m_bFullScreen ? 0 : SDL_RESIZABLE))))
  {
    RefreshGlxContext();

    CTexture iconTexture;
    iconTexture.LoadFromFile("special://xbmc/media/icon.png");
    
    SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture.GetPixels(), iconTexture.GetWidth(), iconTexture.GetHeight(), iconTexture.GetBPP(), iconTexture.GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
    SDL_WM_SetCaption("XBMC Media Center", NULL);

    m_bWindowCreated = true;
    return true;
  }

  return false;
}

bool CWinSystemX11::DestroyWindow()
{
  return true;
}
    
bool CWinSystemX11::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  int options = SDL_OPENGL | (m_bFullScreen ? 0 : SDL_RESIZABLE);
;

  m_nWidth = newWidth;
  m_nHeight = newHeight;
  
  if (m_bFullScreen)
    options |= SDL_FULLSCREEN;

  if ((m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, options)))
  {
    RefreshGlxContext();
    return true;
  }
  
  return false;
}

bool CWinSystemX11::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{  
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;

  return ResizeWindow(m_nWidth, m_nHeight, -1, -1);
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
  bool retVal = false;
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (SDL_GetWMInfo(&info) > 0)
  {
    GLXFBConfig *fbConfigs  = NULL;
    XVisualInfo *vInfo      = NULL;
    int availableFBs        = 0;
    m_glWindow = info.info.x11.window;

    // query compatible framebuffers based on double buffered attributes
    if ((fbConfigs = glXChooseFBConfig(m_dpy, DefaultScreen(m_dpy), doubleVisAttributes, &availableFBs)))
    {
      for (int i = 0; i < availableFBs; i++)
      {
        // obtain the xvisual from the first compatible framebuffer
        vInfo = glXGetVisualFromFBConfig(m_dpy, fbConfigs[i]);
        if (vInfo)
        {
          if (vInfo->depth == 24)
          {
            CLog::Log(LOGNOTICE, "Using fbConfig[%i]",i);
            break;
          }
          XFree(vInfo);
          vInfo = NULL;
        }
      }

      if (vInfo) 
      {
        if (m_glContext)
          glXDestroyContext(m_dpy, m_glContext);

        if ((m_glContext = glXCreateContext(m_dpy, vInfo, NULL, True)))
        {
          // make this context current
          glXMakeCurrent(m_dpy, m_glWindow, m_glContext);
          retVal = true;
        }
        else
          CLog::Log(LOGERROR, "GLX Error: Could not create context");
        XFree(vInfo);
      }
      else
        CLog::Log(LOGERROR, "GLX Error: vInfo is NULL!");
      XFree(fbConfigs);
    }
    else
      CLog::Log(LOGERROR, "GLX Error: No compatible framebuffers found");
  }
  else
    CLog::Log(LOGERROR, "Failed to get window manager info from SDL");

  return retVal;
}

#endif
