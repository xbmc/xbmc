/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "WinSystemX11.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "guilib/GraphicContext.h"
#include "guilib/Texture.h"
#include "guilib/DispResource.h"
#include "utils/log.h"
#include "XRandR.h"
#include <vector>
#include "threads/SingleLock.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/TimeUtils.h"
#include "utils/StringUtils.h"

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#include "../WinEventsX11.h"
#include "input/MouseStat.h"

using namespace std;

CWinSystemX11::CWinSystemX11() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_X11;
  m_glContext = NULL;
  m_dpy = NULL;
  m_glWindow = 0;
  m_bWasFullScreenBeforeMinimize = false;
  m_minimized = false;
  m_bIgnoreNextFocusMessage = false;
  m_dpyLostTime = 0;
  m_invisibleCursor = 0;

  XSetErrorHandler(XErrorHandler);
}

CWinSystemX11::~CWinSystemX11()
{
}

bool CWinSystemX11::InitWindowSystem()
{
  if ((m_dpy = XOpenDisplay(NULL)))
  {
    return CWinSystemBase::InitWindowSystem();
  }
  else
    CLog::Log(LOGERROR, "GLX Error: No Display found");

  return false;
}

bool CWinSystemX11::DestroyWindowSystem()
{
#if defined(HAS_XRANDR)
  //restore desktop resolution on exit
  if (m_bFullScreen)
  {
    XOutput out;
    XMode mode;
    out.name = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput;
    mode.w   = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).iWidth;
    mode.h   = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).iHeight;
    mode.hz  = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).fRefreshRate;
    mode.id  = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strId;
    g_xrandr.SetMode(out, mode);
  }
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
  if(!SetFullScreen(fullScreen, res, false))
    return false;

  m_bWindowCreated = true;
  return true;
}

bool CWinSystemX11::DestroyWindow()
{
  if (!m_glWindow)
    return true;

  if (m_glContext)
    glXMakeCurrent(m_dpy, None, NULL);

  if (m_invisibleCursor)
  {
    XUndefineCursor(m_dpy, m_glWindow);
    XFreeCursor(m_dpy, m_invisibleCursor);
    m_invisibleCursor = 0;
  }

  CWinEventsX11Imp::Quit();

  XUnmapWindow(m_dpy, m_glWindow);
  XSync(m_dpy,TRUE);
  XUngrabKeyboard(m_dpy, CurrentTime);
  XUngrabPointer(m_dpy, CurrentTime);
  XDestroyWindow(m_dpy, m_glWindow);
  m_glWindow = 0;

  if (m_icon)
    XFreePixmap(m_dpy, m_icon);

  return true;
}

bool CWinSystemX11::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if(m_nWidth  == newWidth
  && m_nHeight == newHeight)
    return true;

  if (!SetWindow(newWidth, newHeight, false))
  {
    return false;
  }

  RefreshGlxContext();
  m_nWidth  = newWidth;
  m_nHeight = newHeight;
  m_bFullScreen = false;

  return false;
}

void CWinSystemX11::RefreshWindow()
{
  if (!g_xrandr.Query(true))
  {
    CLog::Log(LOGERROR, "WinSystemX11::RefreshWindow - failed to query xrandr");
    return;
  }
  XOutput out  = g_xrandr.GetCurrentOutput();
  XMode   mode = g_xrandr.GetCurrentMode(out.name);

  RotateResolutions();

  // only overwrite desktop resolution, if we are not in fullscreen mode
  if (!g_graphicsContext.IsFullScreenVideo())
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshWindow - store desktop resolution, width: %d, height: %d, hz: %2.2f", mode.w, mode.h, mode.hz);
    if (!out.isRotated)
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.w, mode.h, mode.hz);
    else
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.h, mode.w, mode.hz);
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strId     = mode.id;
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput = out.name;
  }

  RESOLUTION_INFO res;
  unsigned int i;
  bool found(false);
  for (i = RES_DESKTOP; i < CDisplaySettings::Get().ResolutionInfoSize(); ++i)
  {
    if (CDisplaySettings::Get().GetResolutionInfo(i).strId == mode.id)
    {
      found = true;
      break;
    }
  }

  if (!found)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::RefreshWindow - could not find resolution");
    return;
  }

  if (g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.SetVideoResolution((RESOLUTION)i, true);
  else
    g_graphicsContext.SetVideoResolution(RES_WINDOW, true);
}

bool CWinSystemX11::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{

#if defined(HAS_XRANDR)
  XOutput out;
  XMode mode;

  if (fullScreen)
  {
    out.name = res.strOutput;
    mode.w   = res.iWidth;
    mode.h   = res.iHeight;
    mode.hz  = res.fRefreshRate;
    mode.id  = res.strId;
  }
  else
  {
    out.name = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput;
    mode.w   = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).iWidth;
    mode.h   = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).iHeight;
    mode.hz  = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).fRefreshRate;
    mode.id  = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strId;
  }
 
  XOutput currout  = g_xrandr.GetCurrentOutput();
  XMode   currmode = g_xrandr.GetCurrentMode(currout.name);

  // flip h/w when rotated
  if (m_bIsRotated)
  {
    int w = mode.w;
    mode.w = mode.h;
    mode.h = w;
  }

  // only call xrandr if mode changes
  if (currout.name != out.name || currmode.w != mode.w || currmode.h != mode.h ||
      currmode.hz != mode.hz || currmode.id != mode.id)
  {
    CLog::Log(LOGNOTICE, "CWinSystemX11::SetFullScreen - calling xrandr");
    OnLostDevice();
    g_xrandr.SetMode(out, mode);
  }
#endif

  if (!SetWindow(res.iWidth, res.iHeight, fullScreen))
    return false;

  RefreshGlxContext();

  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  return true;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();


#if defined(HAS_XRANDR)
  if(g_xrandr.Query())
  {
    XOutput out  = g_xrandr.GetCurrentOutput();
    XMode   mode = g_xrandr.GetCurrentMode(out.name);
    m_bIsRotated = out.isRotated;
    if (!m_bIsRotated)
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.w, mode.h, mode.hz);
    else
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.h, mode.w, mode.hz);
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strId     = mode.id;
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput = out.name;
  }
  else
#endif
  {
    int x11screen = DefaultScreen(m_dpy);
    int w = DisplayWidth(m_dpy, x11screen);
    int h = DisplayHeight(m_dpy, x11screen);
    UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, w, h, 0.0);
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
      if (!m_bIsRotated)
      {
        res.iWidth  = mode.w;
        res.iHeight = mode.h;
      }
      else
      {
        res.iWidth  = mode.h;
        res.iHeight = mode.w;
      }
      if (mode.h>0 && mode.w>0 && out.hmm>0 && out.wmm>0)
        res.fPixelRatio = ((float)out.wmm/(float)mode.w) / (((float)out.hmm/(float)mode.h));
      else
        res.fPixelRatio = 1.0f;

      CLog::Log(LOGINFO, "Pixel Ratio: %f", res.fPixelRatio);

      res.strMode      = StringUtils::Format("%s: %s @ %.2fHz", out.name.c_str(), mode.name.c_str(), mode.hz);
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
      CDisplaySettings::Get().AddResolutionInfo(res);
    }
  }
#endif

}

void CWinSystemX11::RotateResolutions()
{
#if defined(HAS_XRANDR)
  XOutput out  = g_xrandr.GetCurrentOutput();
  if (out.isRotated == m_bIsRotated)
    return;

  for (unsigned int i = 0; i < CDisplaySettings::Get().ResolutionInfoSize(); ++i)
  {
    int width = CDisplaySettings::Get().GetResolutionInfo(i).iWidth;
    CDisplaySettings::Get().GetResolutionInfo(i).iWidth = CDisplaySettings::Get().GetResolutionInfo(i).iHeight;
    CDisplaySettings::Get().GetResolutionInfo(i).iHeight = width;
  }
  // update desktop resolution
//  int h = g_settings.m_ResInfo[RES_DESKTOP].iHeight;
//  int w = g_settings.m_ResInfo[RES_DESKTOP].iWidth;
//  float hz = g_settings.m_ResInfo[RES_DESKTOP].fRefreshRate;
//  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, hz);

  m_bIsRotated = out.isRotated;

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

  if (m_glContext)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshGlxContext: refreshing context");
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
  if (show)
    XUndefineCursor(m_dpy,m_glWindow);
  else if (m_invisibleCursor)
    XDefineCursor(m_dpy,m_glWindow, m_invisibleCursor);
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

void CWinSystemX11::NotifyAppFocusChange(bool bGaining)
{
  if (bGaining && m_bWasFullScreenBeforeMinimize && !m_bIgnoreNextFocusMessage &&
      !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();
  if (!bGaining)
    m_bIgnoreNextFocusMessage = false;
}

bool CWinSystemX11::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
  {
    m_bIgnoreNextFocusMessage = true;
    g_graphicsContext.ToggleFullScreenRoot();
  }

  XIconifyWindow(m_dpy, m_glWindow, DefaultScreen(m_dpy));

  m_minimized = true;
  return true;
}
bool CWinSystemX11::Restore()
{
  return false;
}
bool CWinSystemX11::Hide()
{
  XUnmapWindow(m_dpy, m_glWindow);
  XSync(m_dpy, False);
  return true;
}
bool CWinSystemX11::Show(bool raise)
{
  XMapWindow(m_dpy, m_glWindow);
  XSync(m_dpy, False);
  m_minimized = false;
  return true;
}

void CWinSystemX11::CheckDisplayEvents()
{
#if defined(HAS_XRANDR) && defined(HAS_SDL_VIDEO_X11)
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
    NotifyXRREvent();

    // reset fail safe timer
    m_dpyLostTime = 0;
  }
#endif
}

void CWinSystemX11::NotifyXRREvent()
{
  CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);
  RefreshWindow();

  CSingleLock lock(m_resourceSection);

  // tell any shared resources
  for (vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnResetDevice();

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

#if defined(HAS_SDL_VIDEO_X11)
  // fail safe timer
  m_dpyLostTime = CurrentHostCounter();
#else
  CWinEventsX11Imp::SetXRRFailSafeTimer(3000);
#endif
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

bool CWinSystemX11::SetWindow(int width, int height, bool fullscreen)
{
  bool changeWindow = false;
  bool changeSize = false;
  bool mouseActive = false;
  float mouseX, mouseY;

  if (m_glWindow && (m_bFullScreen != fullscreen))
  {
    mouseActive = g_Mouse.IsActive();
    if (mouseActive)
    {
      Window root_return, child_return;
      int root_x_return, root_y_return;
      int win_x_return, win_y_return;
      unsigned int mask_return;
      bool isInWin = XQueryPointer(m_dpy, m_glWindow, &root_return, &child_return,
                                   &root_x_return, &root_y_return,
                                   &win_x_return, &win_y_return,
                                   &mask_return);
      if (isInWin)
      {
        mouseX = (float)win_x_return/m_nWidth;
        mouseY = (float)win_y_return/m_nHeight;
        g_Mouse.SetActive(false);
      }
      else
        mouseActive = false;
    }
    DestroyWindow();
  }

  // create main window
  if (!m_glWindow)
  {
    GLint att[] =
    {
      GLX_RGBA,
      GLX_RED_SIZE, 8,
      GLX_GREEN_SIZE, 8,
      GLX_BLUE_SIZE, 8,
      GLX_ALPHA_SIZE, 8,
      GLX_DEPTH_SIZE, 24,
      GLX_DOUBLEBUFFER,
      None
    };
    Colormap cmap;
    XSetWindowAttributes swa;
    XVisualInfo *vi;

    vi = glXChooseVisual(m_dpy, DefaultScreen(m_dpy), att);
    cmap = XCreateColormap(m_dpy, RootWindow(m_dpy, vi->screen), vi->visual, AllocNone);

    int def_vis = (vi->visual == DefaultVisual(m_dpy, vi->screen));
    swa.override_redirect = fullscreen ? True : False;
    swa.border_pixel = fullscreen ? 0 : 5;
    swa.background_pixel = def_vis ? BlackPixel(m_dpy, vi->screen) : 0;
    swa.colormap = cmap;
    swa.background_pixel = def_vis ? BlackPixel(m_dpy, vi->screen) : 0;
    swa.event_mask = FocusChangeMask | KeyPressMask | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     PropertyChangeMask | StructureNotifyMask | KeymapStateMask |
                     EnterWindowMask | LeaveWindowMask | ExposureMask;
    unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask;

    m_glWindow = XCreateWindow(m_dpy, RootWindow(m_dpy, vi->screen),
                    0, 0, width, height, 0, vi->depth,
                    InputOutput, vi->visual,
                    mask, &swa);

    // define invisible cursor
    Pixmap bitmapNoData;
    XColor black;
    static char noData[] = { 0,0,0,0,0,0,0,0 };
    black.red = black.green = black.blue = 0;

    bitmapNoData = XCreateBitmapFromData(m_dpy, m_glWindow, noData, 8, 8);
    m_invisibleCursor = XCreatePixmapCursor(m_dpy, bitmapNoData, bitmapNoData,
                                            &black, &black, 0, 0);
    XFreePixmap(m_dpy, bitmapNoData);
    XDefineCursor(m_dpy,m_glWindow, m_invisibleCursor);

    //init X11 events
    CWinEventsX11Imp::Init(m_dpy, m_glWindow);

    changeWindow = true;
    changeSize = true;
  }

  if (!CWinEventsX11Imp::HasStructureChanged() && ((width != m_nWidth) || (height != m_nHeight)))
  {
    changeSize = true;
  }

  if (changeSize || changeWindow)
  {
    XResizeWindow(m_dpy, m_glWindow, width, height);
  }

  if (changeWindow)
  {
    m_icon = None;
    if (!fullscreen)
    {
      CreateIconPixmap();
      XWMHints wm_hints;
      XClassHint class_hints;
      XTextProperty windowName, iconName;
      std::string titleString = "XBMC Media Center";
      char *title = (char*)titleString.c_str();

      XStringListToTextProperty(&title, 1, &windowName);
      XStringListToTextProperty(&title, 1, &iconName);
      wm_hints.initial_state = NormalState;
      wm_hints.input = True;
      wm_hints.icon_pixmap = m_icon;
      wm_hints.flags = StateHint | IconPixmapHint | InputHint;

      XSetWMProperties(m_dpy, m_glWindow, &windowName, &iconName,
                            NULL, 0, NULL, &wm_hints,
                            NULL);

      // register interest in the delete window message
      Atom wmDeleteMessage = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);
      XSetWMProtocols(m_dpy, m_glWindow, &wmDeleteMessage, 1);
    }
    XMapRaised(m_dpy, m_glWindow);
    XSync(m_dpy,TRUE);

    if (changeWindow && mouseActive)
    {
      XWarpPointer(m_dpy, None, m_glWindow, 0, 0, 0, 0, mouseX*width, mouseY*height);
    }

    if (fullscreen)
    {
      int result = -1;
      while (result != GrabSuccess)
      {
        result = XGrabPointer(m_dpy, m_glWindow, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, m_glWindow, None, CurrentTime);
        XbmcThreads::ThreadSleep(100);
      }
      XGrabKeyboard(m_dpy, m_glWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);

    }
  }
  return true;
}

bool CWinSystemX11::CreateIconPixmap()
{
  int depth;
  XImage *img = NULL;
  Visual *vis;
  XWindowAttributes wndattribs;
  XVisualInfo visInfo;
  double rRatio;
  double gRatio;
  double bRatio;
  int outIndex = 0;
  int i,j;
  int numBufBytes;
  unsigned char *buf;
  uint32_t *newBuf = 0;
  size_t numNewBufBytes;

  // Get visual Info
  XGetWindowAttributes(m_dpy, m_glWindow, &wndattribs);
  visInfo.visualid = wndattribs.visual->visualid;
  int nvisuals = 0;
  XVisualInfo* visuals = XGetVisualInfo(m_dpy, VisualIDMask, &visInfo, &nvisuals);
  if (nvisuals != 1)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - could not find visual");
    return false;
  }
  visInfo = visuals[0];
  XFree(visuals);

  depth = visInfo.depth;
  vis = visInfo.visual;

  if (depth < 15)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - no suitable depth");
    return false;
  }

  rRatio = vis->red_mask / 255.0;
  gRatio = vis->green_mask / 255.0;
  bRatio = vis->blue_mask / 255.0;

  CTexture iconTexture;
  iconTexture.LoadFromFile("special://xbmc/media/icon.png");
  buf = iconTexture.GetPixels();

  numBufBytes = iconTexture.GetWidth() * iconTexture.GetHeight() * 4;

  if (depth>=24)
    numNewBufBytes = (4 * (iconTexture.GetWidth() * iconTexture.GetHeight()));
  else
    numNewBufBytes = (2 * (iconTexture.GetWidth() * iconTexture.GetHeight()));

  newBuf = (uint32_t*)malloc(numNewBufBytes);
  if (!newBuf)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - malloc failed");
    return false;
  }

  for (i=0; i<iconTexture.GetHeight();++i)
  {
    for (j=0; j<iconTexture.GetWidth();++j)
    {
      unsigned int pos = i*iconTexture.GetPitch()+j*4;
      unsigned int r, g, b;
      r = (buf[pos+2] * rRatio);
      g = (buf[pos+1] * gRatio);
      b = (buf[pos+0] * bRatio);
      r &= vis->red_mask;
      g &= vis->green_mask;
      b &= vis->blue_mask;
      newBuf[outIndex] = r | g | b;
      ++outIndex;
    }
  }
  img = XCreateImage(m_dpy, vis, depth,ZPixmap, 0, (char *)newBuf,
                     iconTexture.GetWidth(), iconTexture.GetHeight(),
                     (depth>=24)?32:16, 0);
  if (!img)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - could not create image");
    free(newBuf);
    return false;
  }
  if (!XInitImage(img))
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - init image failed");
    XDestroyImage(img);
    return false;
  }

  // set byte order
  union
  {
    char c[sizeof(short)];
    short s;
  } order;
  order.s = 1;
  if ((1 == order.c[0]))
  {
    img->byte_order = LSBFirst;
  }
  else
  {
    img->byte_order = MSBFirst;
  }

  // create icon pixmap from image
  m_icon = XCreatePixmap(m_dpy, m_glWindow, img->width, img->height, depth);
  GC gc = XCreateGC(m_dpy, m_glWindow, 0, NULL);
  XPutImage(m_dpy, m_icon, gc, img, 0, 0, 0, 0, img->width, img->height);
  XFreeGC(m_dpy, gc);
  XDestroyImage(img); // this also frees newBuf

  return true;
}

#endif
