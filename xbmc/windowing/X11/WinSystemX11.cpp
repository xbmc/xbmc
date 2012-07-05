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
#include "settings/Setting.h"
#include "guilib/GraphicContext.h"
#include "guilib/Texture.h"
#include "guilib/DispResource.h"
#include "utils/log.h"
#include "XRandR.h"
#include <vector>
#include "threads/SingleLock.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/TimeUtils.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include <X11/Xatom.h>

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
  m_bIsInternalXrr = false;

  XSetErrorHandler(XErrorHandler);
}

CWinSystemX11::~CWinSystemX11()
{
}

bool CWinSystemX11::InitWindowSystem()
{
  if ((m_dpy = XOpenDisplay(NULL)))
  {
    bool ret = CWinSystemBase::InitWindowSystem();
    return ret;
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
    // i have seen core dumps on ATI if the display is not closed here
    XCloseDisplay(m_dpy);
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
  {
    glFinish();
    glXMakeCurrent(m_dpy, None, NULL);
  }

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

  if (!SetWindow(newWidth, newHeight, false, CSettings::Get().GetString("videoscreen.monitor")))
  {
    return false;
  }

  m_nWidth  = newWidth;
  m_nHeight = newHeight;
  m_bFullScreen = false;
  m_currentOutput = CSettings::Get().GetString("videoscreen.monitor");

  return false;
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
 
  XMode   currmode = g_xrandr.GetCurrentMode(out.name);

  // flip h/w when rotated
  if (m_bIsRotated)
  {
    int w = mode.w;
    mode.w = mode.h;
    mode.h = w;
  }

  // only call xrandr if mode changes
  if (currmode.w != mode.w || currmode.h != mode.h ||
      currmode.hz != mode.hz || currmode.id != mode.id)
  {
    CLog::Log(LOGNOTICE, "CWinSystemX11::SetFullScreen - calling xrandr");
    OnLostDevice();
    m_bIsInternalXrr = true;
    g_xrandr.SetMode(out, mode);
    return true;
  }
#endif

  if (!SetWindow(res.iWidth, res.iHeight, fullScreen, CSettings::Get().GetString("videoscreen.monitor")))
    return false;

  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;
  m_currentOutput = CSettings::Get().GetString("videoscreen.monitor");

  return true;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

#if defined(HAS_XRANDR)
  CStdString currentMonitor;
  int numScreens = XScreenCount(m_dpy);
  g_xrandr.SetNumScreens(numScreens);
  if(g_xrandr.Query(true))
  {
    currentMonitor = CSettings::Get().GetString("videoscreen.monitor");
    // check if the monitor is connected
    XOutput *out = g_xrandr.GetOutput(currentMonitor);
    if (!out)
    {
      // choose first output
      currentMonitor = g_xrandr.GetModes()[0].name;
      out = g_xrandr.GetOutput(currentMonitor);
      CSettings::Get().SetString("videoscreen.monitor", currentMonitor);
    }
    XMode mode = g_xrandr.GetCurrentMode(currentMonitor);
    m_bIsRotated = out->isRotated;
    if (!m_bIsRotated)
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), out->screen, mode.w, mode.h, mode.hz);
    else
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), out->screen, mode.h, mode.w, mode.hz);
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strId     = mode.id;
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput = currentMonitor;
  }
  else
#endif
  {
    int x11screen = m_nScreen;
    int w = DisplayWidth(m_dpy, x11screen);
    int h = DisplayHeight(m_dpy, x11screen);
    UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, w, h, 0.0);
  }

#if defined(HAS_XRANDR)

  // erase previous stored modes
  CDisplaySettings::Get().ClearCustomResolutions();

  CLog::Log(LOGINFO, "Available videomodes (xrandr):");

  XOutput *out = g_xrandr.GetOutput(currentMonitor);
  string modename = "";

  if (out != NULL)
  {
    vector<XMode>::iterator modeiter;
    CLog::Log(LOGINFO, "Output '%s' has %"PRIdS" modes", out->name.c_str(), out->modes.size());

    for (modeiter = out->modes.begin() ; modeiter!=out->modes.end() ; modeiter++)
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
      if (mode.h>0 && mode.w>0 && out->hmm>0 && out->wmm>0)
        res.fPixelRatio = ((float)out->wmm/(float)mode.w) / (((float)out->hmm/(float)mode.h));
      else
        res.fPixelRatio = 1.0f;

      CLog::Log(LOGINFO, "Pixel Ratio: %f", res.fPixelRatio);

      res.strMode.Format("%s: %s @ %.2fHz", out->name.c_str(), mode.name.c_str(), mode.hz);
      res.strOutput    = out->name;
      res.strId        = mode.id;
      res.iSubtitles   = (int)(0.965*mode.h);
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
  CDisplaySettings::Get().ApplyCalibrations();
#endif
}

bool CWinSystemX11::HasCalibration(const RESOLUTION_INFO &resInfo)
{
  XOutput *out = g_xrandr.GetOutput(m_currentOutput);

  // keep calibrations done on a not connected output
  if (!out->name.Equals(resInfo.strOutput))
    return true;

  // keep calibrations not updated with resolution data
  if (resInfo.iWidth == 0)
    return true;

  float fPixRatio;
  if (resInfo.iHeight>0 && resInfo.iWidth>0 && out->hmm>0 && out->wmm>0)
    fPixRatio = ((float)out->wmm/(float)resInfo.iWidth) / (((float)out->hmm/(float)resInfo.iHeight));
  else
    fPixRatio = 1.0f;

  if (resInfo.Overscan.left != 0)
    return true;
  if (resInfo.Overscan.top != 0)
    return true;
  if (resInfo.Overscan.right != resInfo.iWidth)
    return true;
  if (resInfo.Overscan.bottom != resInfo.iHeight)
    return true;
  if (resInfo.fPixelRatio != fPixRatio)
    return true;
  if (resInfo.iSubtitles != (int)(0.965*resInfo.iHeight))
    return true;

  return false;
}

void CWinSystemX11::GetConnectedOutputs(std::vector<CStdString> *outputs)
{
  vector<XOutput> outs;
  outs = g_xrandr.GetModes();
  for(unsigned int i=0; i<outs.size(); ++i)
  {
    outputs->push_back(outs[i].name);
  }
}

bool CWinSystemX11::IsCurrentOutput(CStdString output)
{
  return m_currentOutput.Equals(output);
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
  vMask.screen = m_nScreen;
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
      XSync(m_dpy, FALSE);
      m_newGlContext = true;
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

void CWinSystemX11::EnableSystemScreenSaver(bool bEnable)
{
  if (!m_dpy)
    return;

  if (bEnable)
    XForceScreenSaver(m_dpy, ScreenSaverActive);
  else
  {
    Window root_return, child_return;
    int root_x_return, root_y_return;
    int win_x_return, win_y_return;
    unsigned int mask_return;
    bool isInWin = XQueryPointer(m_dpy, RootWindow(m_dpy, m_nScreen), &root_return, &child_return,
                                 &root_x_return, &root_y_return,
                                 &win_x_return, &win_y_return,
                                 &mask_return);

    XWarpPointer(m_dpy, None, RootWindow(m_dpy, m_nScreen), 0, 0, 0, 0, root_x_return+300, root_y_return+300);
    XSync(m_dpy, FALSE);
    XWarpPointer(m_dpy, None, RootWindow(m_dpy, m_nScreen), 0, 0, 0, 0, 0, 0);
    XSync(m_dpy, FALSE);
    XWarpPointer(m_dpy, None, RootWindow(m_dpy, m_nScreen), 0, 0, 0, 0, root_x_return, root_y_return);
    XSync(m_dpy, FALSE);
  }
}

void CWinSystemX11::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !m_bFullScreen)
  {
    g_graphicsContext.ToggleFullScreenRoot();

    m_bWasFullScreenBeforeMinimize = false;
  }
  m_minimized = !bActivated;
}

void CWinSystemX11::NotifyAppFocusChange(bool bGaining)
{
  if (bGaining && m_bWasFullScreenBeforeMinimize && !m_bIgnoreNextFocusMessage &&
      !m_bFullScreen)
  {
    m_bWasFullScreenBeforeMinimize = false;
    g_graphicsContext.ToggleFullScreenRoot();
    m_minimized = false;
  }
  if (!bGaining)
    m_bIgnoreNextFocusMessage = false;
}

void CWinSystemX11::NotifyMouseCoverage(bool covered)
{
  if (!m_bFullScreen)
    return;

  if (covered)
  {
    int result = -1;
    while (result != GrabSuccess && result != AlreadyGrabbed)
    {
      result = XGrabPointer(m_dpy, m_glWindow, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      XbmcThreads::ThreadSleep(100);
    }
    XGrabKeyboard(m_dpy, m_glWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);
  }
  else
  {
    XUngrabKeyboard(m_dpy, CurrentTime);
    XUngrabPointer(m_dpy, CurrentTime);
  }
}

bool CWinSystemX11::Minimize()
{
  m_bWasFullScreenBeforeMinimize = m_bFullScreen;
  if (m_bWasFullScreenBeforeMinimize)
  {
    m_bIgnoreNextFocusMessage = true;
    g_graphicsContext.ToggleFullScreenRoot();
  }

  XIconifyWindow(m_dpy, m_glWindow, m_nScreen);

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
  m_windowDirty = true;

  // if external event update resolutions
  if (!m_bIsInternalXrr)
  {
    UpdateResolutions();
  }
  else if (!g_xrandr.Query(true))
  {
    CLog::Log(LOGERROR, "WinSystemX11::RefreshWindow - failed to query xrandr");
    return;
  }
  m_bIsInternalXrr = false;

  CStdString currentOutput = CSettings::Get().GetString("videoscreen.monitor");
  XOutput *out = g_xrandr.GetOutput(currentOutput);
  XMode   mode = g_xrandr.GetCurrentMode(currentOutput);

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
    i = RES_DESKTOP;
  }

  if (g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.SetVideoResolution((RESOLUTION)i, true);
  else
    g_graphicsContext.SetVideoResolution(RES_WINDOW, true);

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

bool CWinSystemX11::SetWindow(int width, int height, bool fullscreen, const CStdString &output)
{
  bool changeWindow = false;
  bool changeSize = false;
  bool mouseActive = false;
  float mouseX, mouseY;

  if (m_glWindow && ((m_bFullScreen != fullscreen) || !m_currentOutput.Equals(output) || m_windowDirty))
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
    OnLostDevice();
    DestroyWindow();
    m_windowDirty = true;
  }

  // create main window
  if (!m_glWindow)
  {
    EnableSystemScreenSaver(false);

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

    XOutput *out = g_xrandr.GetOutput(output);
    if (!out)
      out = g_xrandr.GetOutput(m_currentOutput);
    m_nScreen = out->screen;
    vi = glXChooseVisual(m_dpy, m_nScreen, att);
    cmap = XCreateColormap(m_dpy, RootWindow(m_dpy, vi->screen), vi->visual, AllocNone);

    int def_vis = (vi->visual == DefaultVisual(m_dpy, vi->screen));
    swa.override_redirect = False;
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
                    out->x, out->y, width, height, 0, vi->depth,
                    InputOutput, vi->visual,
                    mask, &swa);

    if (fullscreen)
    {
      Atom fs = XInternAtom(m_dpy, "_NET_WM_STATE_FULLSCREEN", True);
      XChangeProperty(m_dpy, m_glWindow, XInternAtom(m_dpy, "_NET_WM_STATE", True), XA_ATOM, 32, PropModeReplace, (unsigned char *) &fs, 1);
    }

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
      while (result != GrabSuccess && result != AlreadyGrabbed)
      {
        result = XGrabPointer(m_dpy, m_glWindow, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
        XbmcThreads::ThreadSleep(100);
      }
      XGrabKeyboard(m_dpy, m_glWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    }

    CDirtyRegionList dr;
    RefreshGlxContext();
    XSync(m_dpy, FALSE);
    g_graphicsContext.Clear(0);
    g_graphicsContext.Flip(dr);
    g_Windowing.ResetVSync();
    m_windowDirty = false;

    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnResetDevice();
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
