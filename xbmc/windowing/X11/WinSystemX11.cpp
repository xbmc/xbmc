/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef HAS_GLX

#include <SDL/SDL_syswm.h>
#include "WinSystemX11.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include "guilib/DispResource.h"
#include "utils/log.h"
#include "XRandR.h"
#include <vector>
#include "threads/SingleLock.h"
#include <X11/Xlib.h>
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/TimeUtils.h"

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

using namespace std;

CWinSystemX11::CWinSystemX11() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_X11;
  m_glContext = NULL;
  m_SDLSurface = NULL;
  m_dpy = NULL;
  m_glWindow = 0;
  m_wmWindow = 0;
  m_bWasFullScreenBeforeMinimize = false;
  m_minimized = false;
  m_dpyLostTime = 0;

  XSetErrorHandler(XErrorHandler);
}

CWinSystemX11::~CWinSystemX11()
{
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
#if defined(HAS_XRANDR)
  //restore videomode on exit
  if (m_bFullScreen)
    g_xrandr.RestoreState();
#endif

  if (m_dpy)
  {
    if (m_glContext)
    {
      glXMakeCurrent(m_dpy, None, NULL);
      glXDestroyContext(m_dpy, m_glContext);
    }

    m_glContext = 0;

    //we don't call XCloseDisplay() here, since ati keeps a pointer to our m_dpy
    //so instead we just let m_dpy die on exit
  }

  // m_SDLSurface is free()'d by SDL_Quit().

  return true;
}

bool CWinSystemX11::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  RESOLUTION_INFO& desktop = g_settings.m_ResInfo[RES_DESKTOP];

  if (fullScreen &&
      (res.iWidth != desktop.iWidth || res.iHeight != desktop.iHeight ||
       res.fRefreshRate != desktop.fRefreshRate || res.iScreen != desktop.iScreen))
  {
    //on the first call to SDL_SetVideoMode, SDL stores the current displaymode
    //SDL restores the displaymode on SDL_QUIT(), if we change the displaymode
    //before the first call to SDL_SetVideoMode, SDL changes the displaymode back
    //to the wrong mode on exit

    CLog::Log(LOGINFO, "CWinSystemX11::CreateNewWindow initializing to desktop resolution first");
    if (!SetFullScreen(true, desktop, false))
      return false;
  }

  if(!SetFullScreen(fullScreen, res, false))
    return false;

  CBaseTexture* iconTexture = CTexture::LoadFromFile("special://xbmc/media/icon.png");

  if (iconTexture)
    SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture->GetPixels(), iconTexture->GetWidth(), iconTexture->GetHeight(), 32, iconTexture->GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
  SDL_WM_SetCaption("XBMC Media Center", NULL);
  delete iconTexture;

  // register XRandR Events
#if defined(HAS_XRANDR)
  int iReturn;
  XRRQueryExtension(m_dpy, &m_RREventBase, &iReturn);
  XRRSelectInput(m_dpy, m_wmWindow, RRScreenChangeNotifyMask);
#endif

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
  XOutput out;
  XMode mode;
  out.name = res.strOutput;
  mode.w   = res.iWidth;
  mode.h   = res.iHeight;
  mode.hz  = res.fRefreshRate;
  mode.id  = res.strId;
 
  if(m_bFullScreen)
  {
    OnLostDevice();
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
  else
#endif
  {
    int x11screen = DefaultScreen(m_dpy);
    int w = DisplayWidth(m_dpy, x11screen);
    int h = DisplayHeight(m_dpy, x11screen);
    UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, 0.0);
  }


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
      res.iScreenWidth  = mode.w;
      res.iScreenHeight = mode.h;
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
      res.bFullScreen  = true;

      if (mode.h > 0 && ((float)mode.w / (float)mode.h >= 1.59))
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
      CLog::Log(LOGWARNING, "Failed to get VisualInfo of SDL visual 0x%x", (unsigned) vMask.visualid);
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
    {
      glXMakeCurrent(m_dpy, None, NULL);
      glXDestroyContext(m_dpy, m_glContext);
    }

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

void CWinSystemX11::ResetOSScreensaver()
{
  if (m_bFullScreen)
  {
    //disallow the screensaver when we're fullscreen by periodically calling XResetScreenSaver(),
    //normally SDL does this but we disable that in CApplication::Create()
    //for some reason setting a 0 timeout with XSetScreenSaver doesn't work with gnome
    if (!m_screensaverReset.IsRunning() || m_screensaverReset.GetElapsedSeconds() > 5.0f)
    {
      m_screensaverReset.StartZero();
      XResetScreenSaver(m_dpy);
      //need to flush the output buffer, since we don't check for events on m_dpy
      XFlush(m_dpy);
    }
  }
  else
  {
    m_screensaverReset.Stop();
  }
}

void CWinSystemX11::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();

  m_minimized = !bActivated;
}
bool CWinSystemX11::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    g_graphicsContext.ToggleFullScreenRoot();

  SDL_WM_IconifyWindow();
  m_minimized = true;
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
  m_minimized = false;
  return true;
}

void CWinSystemX11::CheckDisplayEvents()
{
#if defined(HAS_XRANDR)
  bool bGotEvent(false);
  bool bTimeout(false);
  XEvent Event;
  while (XCheckTypedEvent(m_dpy, m_RREventBase + RRScreenChangeNotify, &Event))
  {
    if (Event.type == m_RREventBase + RRScreenChangeNotify)
    {
      CLog::Log(LOGDEBUG, "%s: Received RandR event %i", __FUNCTION__, Event.type);
      bGotEvent = true;
    }
    XRRUpdateConfiguration(&Event);
  }

  // check fail safe timer
  if (m_dpyLostTime && CurrentHostCounter() - m_dpyLostTime > (uint64_t)3 * CurrentHostFrequency())
  {
    CLog::Log(LOGERROR, "%s - no display event after 3 seconds", __FUNCTION__);
    bTimeout = true;
  }

  if (bGotEvent || bTimeout)
  {
    CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);

    CSingleLock lock(m_resourceSection);

    // tell any shared resources
    for (vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnResetDevice();

    // reset fail safe timer
    m_dpyLostTime = 0;
  }
#endif
}

void CWinSystemX11::OnLostDevice()
{
  CLog::Log(LOGDEBUG, "%s - notify display change event", __FUNCTION__);

  // make sure renderer has no invalid references
  g_renderManager.Flush();

  { CSingleLock lock(m_resourceSection);
    for (vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnLostDevice();
  }

  // fail safe timer
  m_dpyLostTime = CurrentHostCounter();
}

void CWinSystemX11::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemX11::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_resourceSection);
  vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

int CWinSystemX11::XErrorHandler(Display* dpy, XErrorEvent* error)
{
  char buf[1024];
  XGetErrorText(error->display, error->error_code, buf, sizeof(buf));
  CLog::Log(LOGERROR, "CWinSystemX11::XErrorHandler: %s, type:%i, serial:%lu, error_code:%i, request_code:%i minor_code:%i",
            buf, error->type, error->serial, (int)error->error_code, (int)error->request_code, (int)error->minor_code);

  return 0;
}

bool CWinSystemX11::EnableFrameLimiter()
{
  return m_minimized;
}

#endif
