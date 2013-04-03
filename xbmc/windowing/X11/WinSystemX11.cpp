/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "WinSystemX11.h"
#include "settings/Settings.h"
#include "pictures/DllImageLib.h"
#include "guilib/DispResource.h"
#include "utils/log.h"
#include "XRandR.h"
#include <vector>
#include "threads/SingleLock.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/TimeUtils.h"
#include "Application.h"

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#include "../WinEvents.h"

using namespace std;

static bool SetOutputMode(RESOLUTION_INFO& res)
{
#if defined(HAS_XRANDR)
  XOutput out;
  XMode   mode = g_xrandr.GetCurrentMode(res.iInternal, res.strOutput);

  /* mode matches so we are done */
  if(res.strId == mode.id)
    return false;

  /* set up mode */
  out.name = res.strOutput;
  out.screen = res.iInternal;
  mode.w   = res.iWidth;
  mode.h   = res.iHeight;
  mode.hz  = res.fRefreshRate;
  mode.id  = res.strId;
  g_xrandr.SetMode(out, mode);
  return true;
#endif
}

CWinSystemX11::CWinSystemX11() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_X11;
  m_glContext = NULL;
  m_dpy = NULL;
  m_visual = NULL;
  m_glWindow = 0;
  m_wmWindow = 0;
  m_bWasFullScreenBeforeMinimize = false;
  m_minimized = false;
  m_dpyLostTime = 0;
  m_invisibleCursor = 0;
  m_outputName  = "";
  m_outputIndex = 0;
  m_wm_fullscreen = false;
  m_wm_controlled = false;

  XSetErrorHandler(XErrorHandler);
}

CWinSystemX11::~CWinSystemX11()
{
}

bool CWinSystemX11::InitWindowSystem()
{
  if ((m_dpy = XOpenDisplay(NULL)))
  {
    if(!CWinSystemBase::InitWindowSystem())
      return false;

    UpdateResolutions();
    return true;
  }
  else
    CLog::Log(LOGERROR, "GLX Error: No Display found");

  return false;
}

bool CWinSystemX11::DestroyWindowSystem()
{
  //restore videomode on exit
  if (m_bFullScreen)
    SetOutputMode(g_settings.m_ResInfo[DesktopResolution(m_outputIndex)]);

  if (m_visual)
  {
    XFree(m_visual);
    m_visual = NULL;
  }

  if (m_dpy)
  {
    XCloseDisplay(m_dpy);
    m_dpy = NULL;
  }

  return true;
}

/**
 * @brief Allocate a cursor that won't be visible on the window
 */
static Cursor AllocateInvisibleCursor(Display* dpy, Window wnd)
{
  Pixmap bitmap;
  XColor black = {0};
  Cursor cursor;
  static char data[] = { 0,0,0,0,0,0,0,0 };

  bitmap = XCreateBitmapFromData(dpy, wnd, data, 8, 8);
  cursor = XCreatePixmapCursor(dpy, bitmap, bitmap,
                               &black, &black, 0, 0);
  XFreePixmap(dpy, bitmap);
  return cursor;
}

static Pixmap AllocateIconPixmap(Display* dpy, Window w)
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
  unsigned i,j;
  unsigned char *buf;
  uint32_t *newBuf = 0;
  size_t numNewBufBytes;

  // Get visual Info
  XGetWindowAttributes(dpy, w, &wndattribs);
  visInfo.visualid = wndattribs.visual->visualid;
  int nvisuals = 0;
  XVisualInfo* visuals = XGetVisualInfo(dpy, VisualIDMask, &visInfo, &nvisuals);
  if (nvisuals != 1)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - could not find visual");
    return None;
  }
  visInfo = visuals[0];
  XFree(visuals);

  depth = visInfo.depth;
  vis = visInfo.visual;

  if (depth < 15)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - no suitable depth");
    return None;
  }

  rRatio = vis->red_mask / 255.0;
  gRatio = vis->green_mask / 255.0;
  bRatio = vis->blue_mask / 255.0;

  DllImageLib dll;
  if (!dll.Load())
    return false;

  ImageInfo image;
  memset(&image, 0, sizeof(image));

  if(!dll.LoadImage("special://xbmc/media/icon.png", 256, 256, &image))
  {
    CLog::Log(LOGERROR, "AllocateIconPixmap to load icon");
    return false;
  }

  buf = image.texture;
  int pitch = ((image.width + 1)* 3 / 4) * 4;


  if (depth>=24)
    numNewBufBytes = 4 * image.width * image.height;
  else
    numNewBufBytes = 2 * image.width * image.height;

  newBuf = (uint32_t*)malloc(numNewBufBytes);
  if (!newBuf)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - malloc failed");
    dll.ReleaseImage(&image);
    return None;
  }

  for (i=0; i<image.height;++i)
  {
    for (j=0; j<image.width;++j)
    {
      unsigned int pos = (image.height-i-1)*pitch+j*3;
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

  dll.ReleaseImage(&image);

  img = XCreateImage(dpy, vis, depth,ZPixmap, 0, (char *)newBuf,
                     image.width, image.height,
                     (depth>=24)?32:16, 0);
  if (!img)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - could not create image");
    free(newBuf);
    return None;
  }
  if (!XInitImage(img))
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - init image failed");
    XDestroyImage(img);
    return None;
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
  Pixmap icon = XCreatePixmap(dpy, w, img->width, img->height, depth);
  GC gc = XCreateGC(dpy, w, 0, NULL);
  XPutImage(dpy, icon, gc, img, 0, 0, 0, 0, img->width, img->height);
  XFreeGC(dpy, gc);
  XDestroyImage(img); // this also frees newBuf

  return icon;
}

void CWinSystemX11::ProbeWindowManager()
{
  int res, format;
  Atom *items, type;
  unsigned long bytes_after, nitems;
  unsigned char *prop_return;
  int wm_window_id;

  m_wm            = false;
  m_wm_name       = "";
  m_wm_fullscreen = false;

  res = XGetWindowProperty(m_dpy, XRootWindow(m_dpy, m_visual->screen), m_NET_SUPPORTING_WM_CHECK,
                        0, 16384, False, AnyPropertyType, &type, &format,
                        &nitems, &bytes_after, &prop_return);
  if(res != Success || nitems == 0 || prop_return == NULL)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::ProbeWindowManager - No window manager found (NET_SUPPORTING_WM_CHECK not set)");
    return;
  }

  wm_window_id = *(int *)prop_return;
  XFree(prop_return);
  prop_return = NULL;

  res = XGetWindowProperty(m_dpy, wm_window_id, m_NET_WM_NAME,
                        0, 16384, False, AnyPropertyType, &type, &format,
                        &nitems, &bytes_after, &prop_return);
  if(res != Success || nitems == 0 || prop_return == NULL)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::ProbeWindowManager - No window manager found (NET_WM_NAME not set on wm window)");
    return;
  }
  m_wm_name = (char *)prop_return;
  XFree(prop_return);
  prop_return = NULL;

  res = XGetWindowProperty(m_dpy, XRootWindow(m_dpy, m_visual->screen), m_NET_SUPPORTED,
                        0, 16384, False, AnyPropertyType, &type, &format,
                        &nitems, &bytes_after, &prop_return);
  if(res != Success || nitems == 0 || prop_return == NULL)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::ProbeWindowManager - No window manager found (NET_SUPPORTING_WM_CHECK not set)");
    return;
  }
  items = (Atom *)prop_return;
  m_wm = true;
  for(unsigned long i = 0; i < nitems; i++)
  {
    if(items[i] == m_NET_WM_STATE_FULLSCREEN)
      m_wm_fullscreen = true;
  }
  XFree(prop_return);
  prop_return = NULL;

  CLog::Log(LOGDEBUG, "CWinSystemX11::ProbeWindowManager - Window manager (%s) detected%s",
        m_wm_name.c_str(),
        m_wm_fullscreen ? ", can fullscreen" : "");

  if(m_wm_fullscreen && g_application.IsStandAlone())
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::ProbeWindowManager - Disable window manager fullscreen due to standalone");
    m_wm_fullscreen = false;
  }
}


bool CWinSystemX11::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  int x = 0
    , y = 0
    , w = res.iWidth
    , h = res.iHeight;

  if (m_wmWindow)
  {
    OnLostDevice();
    DestroyWindow();
  }

  /* update available atoms */
  m_NET_SUPPORTING_WM_CHECK     = XInternAtom(m_dpy, "_NET_SUPPORTING_WM_CHECK", False);
  m_NET_WM_STATE                = XInternAtom(m_dpy, "_NET_WM_STATE", False);
  m_NET_WM_STATE_FULLSCREEN     = XInternAtom(m_dpy, "_NET_WM_STATE_FULLSCREEN", False);
  m_NET_WM_STATE_MAXIMIZED_VERT = XInternAtom(m_dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  m_NET_WM_STATE_MAXIMIZED_HORZ = XInternAtom(m_dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  m_NET_SUPPORTED               = XInternAtom(m_dpy, "_NET_SUPPORTED", False);
  m_NET_WM_STATE_FULLSCREEN     = XInternAtom(m_dpy, "_NET_WM_STATE_FULLSCREEN", False);
  m_NET_SUPPORTING_WM_CHECK     = XInternAtom(m_dpy, "_NET_SUPPORTING_WM_CHECK", False);
  m_NET_WM_NAME                 = XInternAtom(m_dpy, "_NET_WM_NAME", False);
  m_WM_DELETE_WINDOW            = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);

  /* higher layer should have set m_visual */
  if(m_visual == NULL)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateNewWindow - no visual setup");
    return false;
  }

  /* figure out what the window manager support */
  ProbeWindowManager();

  /* make sure our requsted output has the right resolution */
  SetResolution(res, fullScreen);

  XSetWindowAttributes swa = {0};

#if defined(HAS_XRANDR)
  XOutput out = g_xrandr.GetOutput(m_visual->screen, res.strOutput);
  if(out.isConnected)
  {
    x = out.x;
    y = out.y;
  }
#endif

  if(m_wm_fullscreen || fullScreen == false)
  {
    /* center in display */
    RESOLUTION_INFO& win = g_settings.m_ResInfo[RES_WINDOW];
    w  = win.iWidth;
    h  = win.iHeight;
    x += res.iWidth   / 2 - w / 2;
    y += res.iHeight  / 2 - w / 2;

    swa.override_redirect = 0;
    swa.border_pixel      = 5;
  }
  else
  {
    swa.override_redirect = 1;
    swa.border_pixel      = 0;
  }

  if(m_visual->visual == DefaultVisual(m_dpy, m_visual->screen))
      swa.background_pixel = BlackPixel(m_dpy, m_visual->screen);

  swa.colormap   = XCreateColormap(m_dpy, RootWindow(m_dpy, m_visual->screen), m_visual->visual, AllocNone);
  swa.event_mask = FocusChangeMask | KeyPressMask | KeyReleaseMask |
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                   PropertyChangeMask | StructureNotifyMask | KeymapStateMask |
                   EnterWindowMask | LeaveWindowMask | ExposureMask;

  m_wmWindow = XCreateWindow(m_dpy, RootWindow(m_dpy, m_visual->screen),
                  x, y,
                  w, h,
                  0, m_visual->depth,
                  InputOutput, m_visual->visual,
                  CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask,
                  &swa);

  if(m_wm_fullscreen && fullScreen)
      XChangeProperty(m_dpy, m_wmWindow, m_NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &m_NET_WM_STATE_FULLSCREEN, 1);

  m_invisibleCursor = AllocateInvisibleCursor(m_dpy, m_wmWindow);
  XDefineCursor(m_dpy, m_wmWindow, m_invisibleCursor);

  m_icon = AllocateIconPixmap(m_dpy, m_wmWindow);

  XWMHints* wm_hints = XAllocWMHints();
  XTextProperty windowName, iconName;
  const char* title = "XBMC Media Center";

  XStringListToTextProperty((char**)&title, 1, &windowName);
  XStringListToTextProperty((char**)&title, 1, &iconName);
  wm_hints->initial_state = NormalState;
  wm_hints->input         = True;
  wm_hints->icon_pixmap   = m_icon;
  wm_hints->flags         = StateHint | IconPixmapHint | InputHint;

  XSetWMProperties(m_dpy, m_wmWindow, &windowName, &iconName,
                        NULL, 0, NULL, wm_hints,
                        NULL);

  XFree(wm_hints);

  // register interest in the delete window message
  XSetWMProtocols(m_dpy, m_wmWindow, &m_WM_DELETE_WINDOW, 1);

  XMapRaised(m_dpy, m_wmWindow);
  XSync(m_dpy, True);

  RefreshWindowState();

  //init X11 events
  CWinEvents::Init(m_dpy, m_wmWindow);

  m_bWindowCreated = true;
  return true;
}

bool CWinSystemX11::DestroyWindow()
{
  if (m_glContext)
  {
    glFinish();
    glXMakeCurrent(m_dpy, None, NULL);
  }

  if (m_invisibleCursor)
  {
    XUndefineCursor(m_dpy, m_wmWindow);
    XFreeCursor(m_dpy, m_invisibleCursor);
    m_invisibleCursor = 0;
  }

  CWinEvents::Quit();

  if(m_wmWindow)
  {
    XUnmapWindow(m_dpy, m_wmWindow);
    XSync(m_dpy, True);
    XDestroyWindow(m_dpy, m_wmWindow);
    m_wmWindow = 0;
  }

  if (m_icon)
  {
    XFreePixmap(m_dpy, m_icon);
    m_icon = None;
  }

  return true;
}

bool CWinSystemX11::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  RefreshWindowState();

  if(newLeft < 0) newLeft = m_nLeft;
  if(newTop  < 0) newTop  = m_nTop;

  /* check if we are already correct */
  if(m_nWidth  != newWidth
  && m_nHeight != newHeight
  && m_nLeft   != newLeft
  && m_nTop    != newTop)
  {
    XMoveResizeWindow(m_dpy, m_wmWindow, newLeft, newTop, newWidth, newHeight);
    XSync(m_dpy, False);
    RefreshWindowState();
  }

  return true;
}

bool CWinSystemX11::SetResolution(RESOLUTION_INFO& res, bool fullScreen)
{
  bool changed = false;
  /* if we switched outputs or went to desktop, restore old resolution */
  if(m_bFullScreen && (m_outputName != res.strOutput || !fullScreen))
    changed |= SetOutputMode(g_settings.m_ResInfo[DesktopResolution(m_outputIndex)]);

  /* setup wanted mode on wanted display */
  if(fullScreen)
    changed |= SetOutputMode(res);

  if(changed)
  {
    CLog::Log(LOGNOTICE, "CWinSystemX11::SetResolution - modes changed, reset device");
    OnLostDevice();
    XSync(m_dpy, False);
  }
  return changed;
}

bool CWinSystemX11::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  if(res.iInternal != m_visual->screen)
  {
    CreateNewWindow("", fullScreen, res, NULL);
    m_bFullScreen = fullScreen;
    m_outputName  = res.strOutput;
    m_outputIndex = res.iScreen;
    return true;
  }

  SetResolution(res, fullScreen);

  int x = 0, y = 0;
#if defined(HAS_XRANDR)
  XOutput out = g_xrandr.GetOutput(res.iInternal, res.strOutput);
  if(out.isConnected)
  {
    x = out.x;
    y = out.y;
  }
#endif


  if(!fullScreen)
  {
    x = m_nLeft;
    y = m_nTop;
  }

  XWindowAttributes attr2;
  XGetWindowAttributes(m_dpy, m_wmWindow, &attr2);

  if(m_wm_fullscreen)
  {
    /* if on other screen, we must move it first, otherwise
     * the window manager will fullscreen it on the wrong
     * window */
    if(fullScreen && GetCurrentScreen() != res.iScreen)
      XMoveWindow(m_dpy, m_wmWindow, x, y);

    if(attr2.map_state == IsUnmapped)
    {
      if(fullScreen)
        XChangeProperty(m_dpy, m_wmWindow, m_NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &m_NET_WM_STATE_FULLSCREEN, 1);
      else
        XDeleteProperty(m_dpy, m_wmWindow, m_NET_WM_STATE);
    }
    else
    {
      XEvent e = {0};
      e.xany.type = ClientMessage;
      e.xclient.message_type = m_NET_WM_STATE;
      e.xclient.display = m_dpy;
      e.xclient.window  = m_wmWindow;
      e.xclient.format  = 32;
      if(fullScreen)
      {
        e.xclient.data.l[0] = 1; /* _NET_WM_STATE_ADD    */
        e.xclient.data.l[1] = m_NET_WM_STATE_FULLSCREEN;
      }
      else
      {
        e.xclient.data.l[0] = 0; /* _NET_WM_STATE_REMOVE */
        e.xclient.data.l[1] = m_NET_WM_STATE_FULLSCREEN;
      }

      XSendEvent(m_dpy, RootWindow(m_dpy, m_visual->screen), False,
                 SubstructureNotifyMask | SubstructureRedirectMask, &e);
    }

  }
  else
  {
    XSetWindowAttributes attr = {0};
    if(fullScreen)
      attr.override_redirect = 1;
    else
      attr.override_redirect = 0;

    /* if override_redirect changes, we need to notify
     * WM by reparenting the window to root */
    if(attr.override_redirect != attr2.override_redirect)
    {
      XUnmapWindow           (m_dpy, m_wmWindow);
      XChangeWindowAttributes(m_dpy, m_wmWindow, CWOverrideRedirect, &attr);
      XReparentWindow        (m_dpy, m_wmWindow, RootWindow(m_dpy, res.iInternal), x, y);
      XGetWindowAttributes   (m_dpy, m_wmWindow, &attr2);
    }

    XWindowChanges cw;
    cw.x = x;
    cw.y = y;
    if(fullScreen)
      cw.border_width = 0;
    else
      cw.border_width = 5;
    cw.width  = res.iWidth;
    cw.height = res.iHeight;

    XConfigureWindow(m_dpy, m_wmWindow, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &cw);
  }

  XMapRaised(m_dpy, m_wmWindow);
  XSync(m_dpy, False);

  m_bFullScreen = fullScreen;
  m_outputName  = res.strOutput;
  m_outputIndex = res.iScreen;

  RefreshWindowState();
  return true;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  // erase previous stored modes
  if (g_settings.m_ResInfo.size() > (unsigned)RES_DESKTOP)
  {
    g_settings.m_ResInfo.erase(g_settings.m_ResInfo.begin()+RES_DESKTOP
                             , g_settings.m_ResInfo.end());
  }

#if defined(HAS_XRANDR)
  // add desktop modes
  g_xrandr.Query(XScreenCount(m_dpy), true);
  std::vector<XOutput> outs = g_xrandr.GetModes();
  if(outs.size() > 0)
  {
    for(unsigned i = 0; i < outs.size(); ++i)
    {
      XOutput out  = outs[i];
      XMode   mode = g_xrandr.GetCurrentMode(out.screen, out.name);
      RESOLUTION_INFO res;

      UpdateDesktopResolution(res, i, mode.w, mode.h, mode.hz);
      res.strId     = mode.id;
      res.strOutput = out.name;
      res.iInternal = out.screen;
      g_settings.m_ResInfo.push_back(res);
    }
  }
  else
#endif
  {
    RESOLUTION_INFO res;
    int x11screen = DefaultScreen(m_dpy);
    int w = DisplayWidth(m_dpy, x11screen);
    int h = DisplayHeight(m_dpy, x11screen);
    res.iInternal = x11screen;
    UpdateDesktopResolution(res, 0, w, h, 0.0);
    g_settings.m_ResInfo.push_back(res);
  }


#if defined(HAS_XRANDR)

  CLog::Log(LOGINFO, "Available videomodes (xrandr):");
  vector<XOutput>::iterator outiter;
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
      res.iScreen      = outiter - outs.begin();
      res.iInternal    = out.screen;
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

int  CWinSystemX11::GetNumScreens()
{
#if defined(HAS_XRANDR)
  int count = g_xrandr.GetModes().size();
  if(count > 0)
    return count;
  else
    return 0;
#else
  return 1;
#endif
}

int  CWinSystemX11::GetCurrentScreen()
{

#if defined(HAS_XRANDR)
  std::vector<XOutput> outs = g_xrandr.GetModes();
  int best_index   = -1;
  int best_overlap =  0;
  for(unsigned i = 0; i < outs.size(); ++i)
  {
    XOutput out  = outs[i];
    if(out.screen != m_visual->screen)
      continue;

    int w = std::max(0, std::min(m_nLeft + m_nWidth , out.x + out.w) - std::max(m_nLeft, out.x));
    int h = std::max(0, std::min(m_nTop  + m_nHeight, out.y + out.h) - std::max(m_nTop , out.y));
    if(w*h > best_overlap)
      best_index = i;
  }

  if(best_index >= 0)
    return best_index;
#endif

  return m_outputIndex;
}

void CWinSystemX11::RefreshWindowState()
{
  XWindowAttributes attr;
  Window child;
  XGetWindowAttributes(m_dpy, m_wmWindow, &attr);
  XTranslateCoordinates(m_dpy, m_wmWindow, attr.root, -attr.border_width, -attr.border_width, &attr.x, &attr.y, &child);

  m_nWidth    = attr.width;
  m_nHeight   = attr.height;
  if(!m_bFullScreen)
  {
    m_nLeft     = attr.x;
    m_nTop      = attr.y;
  }

  m_wm_controlled = attr.override_redirect == 0 && m_wm_name;
}

void CWinSystemX11::ShowOSMouse(bool show)
{
  if (show)
    XUndefineCursor(m_dpy,m_wmWindow);
  else if (m_invisibleCursor)
    XDefineCursor(m_dpy,m_wmWindow, m_invisibleCursor);
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

void CWinSystemX11::NotifyAppFocusChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();

  m_minimized = !bActivated;
  if(bActivated)
    m_bWasFullScreenBeforeMinimize = false;
}

bool CWinSystemX11::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    g_graphicsContext.ToggleFullScreenRoot();

  m_minimized = true;
  XIconifyWindow(m_dpy, m_wmWindow, m_visual->screen);

  return true;
}

bool CWinSystemX11::Restore()
{
  return Show(true);
}

bool CWinSystemX11::Hide()
{
  XUnmapWindow(m_dpy, m_wmWindow);
  XSync(m_dpy, False);
  return true;
}

bool CWinSystemX11::Show(bool raise)
{
  if(raise)
    XMapRaised(m_dpy, m_wmWindow);
  else
    XMapWindow(m_dpy, m_wmWindow);

  XSync(m_dpy, False);
  m_minimized = false;
  return true;
}

void CWinSystemX11::CheckDisplayEvents()
{
  if (m_dpyLostTime && CurrentHostCounter() - m_dpyLostTime > (uint64_t)3 * CurrentHostFrequency())
  {
    CLog::Log(LOGERROR, "%s - no display event after 3 seconds", __FUNCTION__);
    OnResetDevice();
  }
}

void CWinSystemX11::NotifyXRREvent()
{
  CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);

#if defined(HAS_XRANDR)
  g_xrandr.Query(XScreenCount(m_dpy), true);
#endif
  OnResetDevice();
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


void CWinSystemX11::OnResetDevice()
{
  CSingleLock lock(m_resourceSection);

  // tell any shared resources
  for (vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnResetDevice();

  // reset fail safe timer
  m_dpyLostTime = 0;
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
