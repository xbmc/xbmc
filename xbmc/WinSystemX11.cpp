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

#include "system.h"

#ifdef HAS_GLX

#include <SDL/SDL_syswm.h>
#include "WinSystemX11.h"
#include "SpecialProtocol.h"
#include "Settings.h"
#include "Texture.h"
#include "utils/log.h"
#include "linux/XRandR.h"
#include <vector>

using namespace std;

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

bool CWinSystemX11::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if(!SetFullScreen(fullScreen, res, false))
    return false;

  CTexture iconTexture;
  iconTexture.LoadFromFile("special://xbmc/media/icon.png");

  SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture.GetPixels(), iconTexture.GetWidth(), iconTexture.GetHeight(), 32, iconTexture.GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
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
  if(m_nWidth  == newWidth
  && m_nHeight == newHeight)
    return true;

  m_nWidth  = newWidth;
  m_nHeight = newHeight;

  int options = SDL_OPENGL;
  if (m_bFullScreen)
    options |= SDL_FULLSCREEN;
  else
    options |= SDL_RESIZABLE;

  if ((m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, options)))
  {
    RefreshGlxContext();
    return true;
  }

  return false;
}

bool CWinSystemX11::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

#if defined(HAS_XRANDR)

  if(m_bFullScreen)
  {
    XOutput out;
    XMode mode;
    out.name = res.strOutput;
    mode.w   = res.iWidth;
    mode.h   = res.iHeight;
    mode.hz  = res.fRefreshRate;
    mode.id  = res.strId;
    g_xrandr.SetMode(out, mode);
  }
  else
    g_xrandr.RestoreState();

#endif

  int options = SDL_OPENGL;
  if (m_bFullScreen)
    options |= SDL_FULLSCREEN;
  else
    options |= SDL_RESIZABLE;

  if ((m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, options)))
  {
    if ((m_SDLSurface->flags & SDL_OPENGL) != SDL_OPENGL)
      CLog::Log(LOGERROR, "CWinSystemX11::SetFullScreen SDL_OPENGL not set, SDL_GetError:%s", SDL_GetError());

    RefreshGlxContext();
    return true;
  }

  return false;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();


#if defined(HAS_XRANDR)
  if(g_xrandr.Query())
  {
    XOutput out  = g_xrandr.GetCurrentOutput();
    XMode   mode = g_xrandr.GetCurrentMode(out.name);
    UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, mode.w, mode.h, mode.hz);
    g_settings.m_ResInfo[RES_DESKTOP].strId     = mode.id;
    g_settings.m_ResInfo[RES_DESKTOP].strOutput = out.name;
  }
#else
  {
    int x11screen = DefaultScreen(m_dpy);
    int w = DisplayWidth(m_dpy, x11screen);
    int h = DisplayHeight(m_dpy, x11screen);
    UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, 0.0);
  }
#endif


#if defined(HAS_XRANDR)

  CLog::Log(LOGINFO, "Available videomodes (xrandr):");
  vector<XOutput>::iterator outiter;
  vector<XOutput> outs;
  outs = g_xrandr.GetModes();
  CLog::Log(LOGINFO, "Number of connected outputs: %"PRIdS"", outs.size());
  string modename = "";

  for (outiter = outs.begin() ; outiter != outs.end() ; outiter++)
  {
    XOutput out = *outiter;
    vector<XMode>::iterator modeiter;
    CLog::Log(LOGINFO, "Output '%s' has %"PRIdS" modes", out.name.c_str(), out.modes.size());

    for (modeiter = out.modes.begin() ; modeiter!=out.modes.end() ; modeiter++)
    {
      XMode mode = *modeiter;
      CLog::Log(LOGINFO, "ID:%s Name:%s Refresh:%f Width:%d Height:%d",
                mode.id.c_str(), mode.name.c_str(), mode.hz, mode.w, mode.h);
      RESOLUTION_INFO res;
      res.iWidth  = mode.w;
      res.iHeight = mode.h;
      if (mode.h>0 && mode.w>0 && out.hmm>0 && out.wmm>0)
        res.fPixelRatio = ((float)out.wmm/(float)mode.w) / (((float)out.hmm/(float)mode.h));
      else
        res.fPixelRatio = 1.0f;

      CLog::Log(LOGINFO, "Pixel Ratio: %f", res.fPixelRatio);

      res.strMode.Format("%s: %s @ %.2fHz", out.name.c_str(), mode.name.c_str(), mode.hz);
      res.strOutput    = out.name;
      res.strId        = mode.id;
      res.iSubtitles   = (int)(0.95*mode.h);
      res.fRefreshRate = mode.hz;

      if ((float)mode.w / (float)mode.h >= 1.59)
        res.dwFlags = D3DPRESENTFLAG_WIDESCREEN;
      else
        res.dwFlags = 0;

      g_graphicsContext.ResetOverscan(res);
      g_settings.m_ResInfo.push_back(res);
    }
  }
#endif

}

bool CWinSystemX11::IsSuitableVisual(XVisualInfo *vInfo)
{
  int value;
  if (glXGetConfig(m_dpy, vInfo, GLX_RGBA, &value) || !value)
    return false;
  if (glXGetConfig(m_dpy, vInfo, GLX_DOUBLEBUFFER, &value) || !value)
    return false;
  if (glXGetConfig(m_dpy, vInfo, GLX_RED_SIZE, &value) || value < 8)
    return false;
  if (glXGetConfig(m_dpy, vInfo, GLX_GREEN_SIZE, &value) || value < 8)
    return false;
  if (glXGetConfig(m_dpy, vInfo, GLX_BLUE_SIZE, &value) || value < 8)
    return false;
  if (glXGetConfig(m_dpy, vInfo, GLX_ALPHA_SIZE, &value) || value < 8)
    return false;
  if (glXGetConfig(m_dpy, vInfo, GLX_DEPTH_SIZE, &value) || value < 8)
    return false;
  return true;
}

bool CWinSystemX11::RefreshGlxContext()
{
  bool retVal = false;
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (SDL_GetWMInfo(&info) <= 0)
  {
    CLog::Log(LOGERROR, "Failed to get window manager info from SDL");
    return false;
  }

  if(m_glWindow == info.info.x11.window && m_glContext)
  {
    CLog::Log(LOGERROR, "GLX: Same window as before, refreshing context");
    glXMakeCurrent(m_dpy, None, NULL);
    glXMakeCurrent(m_dpy, m_glWindow, m_glContext);
    return true;
  }

  XVisualInfo vMask;
  XVisualInfo *visuals;
  XVisualInfo *vInfo      = NULL;
  int availableVisuals    = 0;
  vMask.screen = DefaultScreen(m_dpy);
  XWindowAttributes winAttr;
  m_glWindow = info.info.x11.window;
  m_wmWindow = info.info.x11.wmwindow;

  /* Assume a depth of 24 in case the below calls to XGetWindowAttributes()
     or XGetVisualInfo() fail. That shouldn't happen unless something is
     fatally wrong, but lets prepare for everything. */
  vMask.depth = 24;

  if (XGetWindowAttributes(m_dpy, m_glWindow, &winAttr))
  {
    vMask.visualid = XVisualIDFromVisual(winAttr.visual);
    vInfo = XGetVisualInfo(m_dpy, VisualScreenMask | VisualIDMask, &vMask, &availableVisuals);
    if (!vInfo)
      CLog::Log(LOGWARNING, "Failed to get VisualInfo of SDL visual 0x%x", (unsigned) vInfo->visualid);
    else if(!IsSuitableVisual(vInfo))
    {
      CLog::Log(LOGWARNING, "Visual 0x%x of the SDL window is not suitable, looking for another one...",
                (unsigned) vInfo->visualid);
      vMask.depth = vInfo->depth;
      XFree(vInfo);
      vInfo = NULL;
    }
  }
  else
    CLog::Log(LOGWARNING, "Failed to get SDL window attributes");

  /* As per glXMakeCurrent documentation, we have to use the same visual as
     m_glWindow. Since that was not suitable for use, we try to use another
     one with the same depth and hope that the used implementation is less
     strict than the documentation. */
  if (!vInfo)
  {
    visuals = XGetVisualInfo(m_dpy, VisualScreenMask | VisualDepthMask, &vMask, &availableVisuals);
    for (int i = 0; i < availableVisuals; i++)
    {
      if (IsSuitableVisual(&visuals[i]))
      {
        vMask.visualid = visuals[i].visualid;
        vInfo = XGetVisualInfo(m_dpy, VisualScreenMask | VisualIDMask, &vMask, &availableVisuals);
        break;
      }
    }
    XFree(visuals);
  }

  if (vInfo)
  {
    CLog::Log(LOGNOTICE, "Using visual 0x%x", (unsigned) vInfo->visualid);
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

  return retVal;
}

void CWinSystemX11::ShowOSMouse(bool show)
{
  SDL_ShowCursor(show ? 1 : 0);
}

void CWinSystemX11::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();
}
bool CWinSystemX11::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    g_graphicsContext.ToggleFullScreenRoot();

  SDL_WM_IconifyWindow();
  return true;
}
bool CWinSystemX11::Restore()
{
  return false;
}
bool CWinSystemX11::Hide()
{
  XUnmapWindow(m_dpy, m_wmWindow);
  XSync(m_dpy, False);
  return true;
}
bool CWinSystemX11::Show(bool raise)
{
  XMapWindow(m_dpy, m_wmWindow);
  XSync(m_dpy, False);
  return true;
}
#endif
