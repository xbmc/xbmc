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
#include "pictures/DllImageLib.h"
#include "utils/log.h"
#include "XRandR.h"
#include <vector>
#include "threads/SingleLock.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/TimeUtils.h"

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#include "../WinEvents.h"

using namespace std;

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
#if defined(HAS_XRANDR)
  //restore videomode on exit
  if (m_bFullScreen)
    g_xrandr.RestoreState();
#endif

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


bool CWinSystemX11::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if (m_wmWindow)
  {
    OnLostDevice();
    DestroyWindow();
  }

  /* higher layer should have set m_visual */
  if(m_visual == NULL)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateNewWindow - no visual setup");
    return false;
  }

  XSetWindowAttributes swa = {0};

  swa.override_redirect = False;
  swa.border_pixel      = fullScreen ? 0 : 5;

  if(m_visual->visual == DefaultVisual(m_dpy, m_visual->screen))
      swa.background_pixel = BlackPixel(m_dpy, m_visual->screen);

  swa.colormap   = XCreateColormap(m_dpy, RootWindow(m_dpy, m_visual->screen), m_visual->visual, AllocNone);
  swa.event_mask = FocusChangeMask | KeyPressMask | KeyReleaseMask |
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                   PropertyChangeMask | StructureNotifyMask | KeymapStateMask |
                   EnterWindowMask | LeaveWindowMask | ExposureMask;

  m_wmWindow = XCreateWindow(m_dpy, RootWindow(m_dpy, m_visual->screen),
                  0, 0,
                  res.iWidth, res.iHeight,
                  0, m_visual->depth,
                  InputOutput, m_visual->visual,
                  CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask,
                  &swa);

  if (fullScreen)
  {
    Atom fs = XInternAtom(m_dpy, "_NET_WM_STATE_FULLSCREEN", False);
    XChangeProperty(m_dpy, m_wmWindow, XInternAtom(m_dpy, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (unsigned char *) &fs, 1);
  }
  else
    XDeleteProperty(m_dpy, m_wmWindow, XInternAtom(m_dpy, "_NET_WM_STATE", False));

  m_invisibleCursor = AllocateInvisibleCursor(m_dpy, m_wmWindow);
  XDefineCursor(m_dpy, m_wmWindow, m_invisibleCursor);

  m_icon = AllocateIconPixmap(m_dpy, m_wmWindow);

  XWMHints wm_hints;
  XTextProperty windowName, iconName;
  const char* title = "XBMC Media Center";

  XStringListToTextProperty((char**)&title, 1, &windowName);
  XStringListToTextProperty((char**)&title, 1, &iconName);
  wm_hints.initial_state = NormalState;
  wm_hints.input         = True;
  wm_hints.icon_pixmap   = m_icon;
  wm_hints.flags         = StateHint | IconPixmapHint | InputHint;

  XSetWMProperties(m_dpy, m_wmWindow, &windowName, &iconName,
                        NULL, 0, NULL, &wm_hints,
                        NULL);

  // register interest in the delete window message
  Atom wmDeleteMessage = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(m_dpy, m_wmWindow, &wmDeleteMessage, 1);

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

bool CWinSystemX11::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{

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


  XSetWindowAttributes attr = {0};
  if(fullScreen)
    attr.border_pixel = 0;
  else
    attr.border_pixel = 5;
  XChangeWindowAttributes(m_dpy, m_wmWindow, CWBorderPixel, &attr);

  int x = 0, y = 0;

  XWindowAttributes attr2;
  XGetWindowAttributes(m_dpy, m_wmWindow, &attr2);

  Atom m_NET_WM_STATE                = XInternAtom(m_dpy, "_NET_WM_STATE", False);
  Atom m_NET_WM_STATE_FULLSCREEN     = XInternAtom(m_dpy, "_NET_WM_STATE_FULLSCREEN", True);
  Atom m_NET_WM_STATE_MAXIMIZED_VERT = XInternAtom(m_dpy, "_NET_WM_STATE_MAXIMIZED_VERT", True);
  Atom m_NET_WM_STATE_MAXIMIZED_HORZ = XInternAtom(m_dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", True);

  if(m_NET_WM_STATE_FULLSCREEN)
  {
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
        e.xclient.data.l[2] = m_NET_WM_STATE_MAXIMIZED_VERT;
        e.xclient.data.l[3] = m_NET_WM_STATE_MAXIMIZED_HORZ;
      }

      XSendEvent(m_dpy, RootWindow(m_dpy, m_visual->screen), False,
                 SubstructureNotifyMask | SubstructureRedirectMask, &e);
    }
    XSync(m_dpy, False);
  }

  if(fullScreen)
    XMoveResizeWindow(m_dpy, m_wmWindow, x, y, res.iWidth, res.iHeight);
  else
    XResizeWindow(m_dpy, m_wmWindow, res.iWidth, res.iHeight);
  XSync(m_dpy, False);

  RefreshWindowState();

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
    UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.w, mode.h, mode.hz);
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
      CDisplaySettings::Get().AddResolutionInfo(res);
    }
  }
#endif

}

void CWinSystemX11::RefreshWindowState()
{
  XWindowAttributes attr;
  Window child;
  XGetWindowAttributes(m_dpy, m_wmWindow, &attr);
  XTranslateCoordinates(m_dpy, m_wmWindow, attr.root, -attr.border_width, -attr.border_width, &attr.x, &attr.y, &child);

  m_nWidth    = attr.width;
  m_nHeight   = attr.height;
  m_nLeft     = attr.x;
  m_nTop      = attr.y;
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
    OnResetDevice();
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

void CWinSystemX11::SetGrabMode(const CSetting *setting /*= NULL*/)
{
  bool enabled;
  if (setting)
    enabled = ((CSettingBool*)setting)->GetValue();
  else
    enabled = CSettings::Get().GetBool("input.enablesystemkeys");
    
  if (m_SDLSurface && m_SDLSurface->flags & SDL_FULLSCREEN)
  {
    if (enabled)
    {
      //SDL will always call XGrabPointer and XGrabKeyboard when in fullscreen
      //so temporarily zero the SDL_FULLSCREEN flag, then turn off SDL grab mode
      //this will make SDL call XUnGrabPointer and XUnGrabKeyboard
      m_SDLSurface->flags &= ~SDL_FULLSCREEN;
      SDL_WM_GrabInput(SDL_GRAB_OFF);
      m_SDLSurface->flags |= SDL_FULLSCREEN;
    }
    else
    {
      //turn off key grabbing, which will actually make SDL turn it on when in fullscreen
      SDL_WM_GrabInput(SDL_GRAB_OFF);
    }
  }
}

void CWinSystemX11::OnSettingChanged(const CSetting *setting)
{
  if (setting->GetId() == "input.enablesystemkeys")
    SetGrabMode(setting);
}

#endif
