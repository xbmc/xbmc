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

#if defined(HAVE_X11) && defined(HAS_GLES)

#include "WinSystemX11GLES.h"
#include "utils/log.h"
#include <SDL/SDL_syswm.h>
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include "windowing/X11/XRandR.h"
#include <vector>

using namespace std;

// Comment out one of the following defines to select the colourspace to use
//#define RGBA8888
#define RGB565

#if defined(RGBA8888)
#define RSIZE	8
#define GSIZE	8
#define BSIZE	8
#define ASIZE	8
#define DEPTH	8
#define BPP		32
#elif defined(RGB565)
#define RSIZE	5
#define GSIZE	6
#define BSIZE	5
#define ASIZE	0
#define DEPTH	16
#define BPP		16
#endif

static int configAttributes[] =
{
  EGL_RED_SIZE,			RSIZE,
  EGL_GREEN_SIZE,		GSIZE,
  EGL_BLUE_SIZE,		BSIZE,
  EGL_ALPHA_SIZE,		ASIZE,
  EGL_DEPTH_SIZE,		DEPTH,
  EGL_SAMPLES,			0,        // AntiAiliasing
#if HAS_GLES == 2
  EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
#endif
  EGL_NONE
};

static int contextAttributes[] =
{
  EGL_CONTEXT_CLIENT_VERSION,	HAS_GLES,
  EGL_NONE
};

CWinSystemX11GLES::CWinSystemX11GLES() : CWinSystemBase()
, m_eglOMXContext(0)
{
  m_eWindowSystem = WINDOW_SYSTEM_EGL;
  m_bWasFullScreenBeforeMinimize = false;

  m_SDLSurface = NULL;
  m_eglDisplay = NULL;
  m_eglContext = NULL;
  m_eglSurface = NULL;
  m_eglWindow  = NULL;
  m_wmWindow   = NULL;
  m_dpy        = NULL;
  
  m_iVSyncErrors = 0;
}

CWinSystemX11GLES::~CWinSystemX11GLES()
{
}

bool CWinSystemX11GLES::InitWindowSystem()
{
  EGLBoolean val = false;
  EGLint maj, min;
  if ((m_dpy = XOpenDisplay(NULL)) &&
      (m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy)) &&
      (val = eglInitialize(m_eglDisplay, &maj, &min)))
  {
	SDL_EnableUNICODE(1);
	// set repeat to 10ms to ensure repeat time < frame time
	// so that hold times can be reliably detected
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   RSIZE);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, GSIZE);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  BSIZE);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, ASIZE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	return CWinSystemBase::InitWindowSystem();
  }
  else
    CLog::Log(LOGERROR, "EGL Error: No Display found! dpy:%p egl:%p init:%d", m_dpy, m_eglDisplay, val);

  return false;
}

bool CWinSystemX11GLES::DestroyWindowSystem()
{
  if (m_eglContext)
  {
    eglDestroyContext(m_eglDisplay, m_eglContext);
    m_eglContext = 0;
  }

  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = NULL;
  }

  if (m_eglDisplay)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = 0;
  }

  if (m_dpy)
  {
    XCloseDisplay(m_dpy);
    m_dpy = NULL;
  }

  return true;
}

bool CWinSystemX11GLES::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if(!SetFullScreen(fullScreen, res, false))
	return false;

  CTexture iconTexture;
  iconTexture.LoadFromFile("special://xbmc/media/icon.png");

  SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture.GetPixels(), iconTexture.GetWidth(), iconTexture.GetHeight(), BPP, iconTexture.GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
  SDL_WM_SetCaption("XBMC Media Center", NULL);

  m_bWindowCreated = true;

  m_eglext  = " ";
  m_eglext += eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
  m_eglext += " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:%s", m_eglext.c_str());

  return true;
}

bool CWinSystemX11GLES::DestroyWindow()
{
  return true;
}

bool CWinSystemX11GLES::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (m_nWidth != newWidth || m_nHeight != newHeight)
  {
    m_nWidth  = newWidth;
    m_nHeight = newHeight;

#if (HAS_GLES == 2)
    int options = 0;
#else
    int options = SDL_OPENGL;
#endif
    if (m_bFullScreen)
      options |= SDL_FULLSCREEN;
    else
      options |= SDL_RESIZABLE;

    if ((m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, options)))
    {
      RefreshEGLContext();
    }
  }

  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, false, 0);

  return true;
}

bool CWinSystemX11GLES::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
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

#if (HAS_GLES == 2)
    int options = 0;
#else
    int options = SDL_OPENGL;
#endif
  if (m_bFullScreen)
    options |= SDL_FULLSCREEN;
  else
    options |= SDL_RESIZABLE;

  if ((m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, options)))
  {
    RefreshEGLContext();
  }

  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  
  return true;
}

void CWinSystemX11GLES::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

#if defined(HAS_XRANDR)
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

bool CWinSystemX11GLES::IsExtSupported(const char* extension)
{
  if(strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGLES::IsExtSupported(extension);

  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != string::npos;
}

bool CWinSystemX11GLES::RefreshEGLContext()
{
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (SDL_GetWMInfo(&info) <= 0)
  {
    CLog::Log(LOGERROR, "Failed to get window manager info from SDL");
    return false;
  }

  if (!m_eglDisplay)
  {
    CLog::Log(LOGERROR, "EGL: No valid display!");
    return false;
  }

  if ((m_eglWindow == info.info.x11.window) && m_eglSurface && m_eglContext)
  {
    CLog::Log(LOGWARNING, "EGL: Same window as before, refreshing context");
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglMakeCurrent(m_eglDisplay, m_eglSurface,   m_eglSurface,   m_eglContext);
    return true;
  }

  m_eglWindow = info.info.x11.window;
  m_wmWindow  = info.info.x11.wmwindow;

  EGLConfig eglConfig = NULL;
  EGLint num;

  if (eglChooseConfig(m_eglDisplay, configAttributes, &eglConfig, 1, &num) == EGL_FALSE)
  {
    CLog::Log(LOGERROR, "EGL Error: No compatible configs found");
    return false;
  }

  if (m_eglContext)
    eglDestroyContext(m_eglDisplay, m_eglContext);

  if ((m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttributes)) == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not create context");
    return false;
  }

  if ((m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, m_eglContext, contextAttributes)) == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not create OMX context");
    return false;
  }

  if (m_eglSurface)
    eglDestroySurface(m_eglDisplay, m_eglSurface);

  if ((m_eglSurface = eglCreateWindowSurface(m_eglDisplay, eglConfig, (EGLNativeWindowType)m_eglWindow, NULL)) == EGL_NO_SURFACE)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not create surface");
    return false;
  }

  if (eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) == EGL_FALSE)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not make context current");
    return false;
  }

  CLog::Log(LOGDEBUG, "RefreshEGLContext Succeeded! Format:A%d|R%d|G%d|B%d|BPP%d", ASIZE, RSIZE, GSIZE, BSIZE, DEPTH);
  return true;
}

bool CWinSystemX11GLES::PresentRenderImpl(const CDirtyRegionList &dirty)
{
//  glFinish();	// Needed???
  eglSwapBuffers(m_eglDisplay, m_eglSurface);

  return true;
}

void CWinSystemX11GLES::SetVSyncImpl(bool enable)
{
  if (eglSwapInterval(m_eglDisplay, enable ? 1 : 0) == EGL_FALSE)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not set vsync");
  }
}

void CWinSystemX11GLES::ShowOSMouse(bool show)
{
  SDL_ShowCursor(show ? 1 : 0);
  // On BB have show the cursor, otherwise it hangs! (FIXME verify it if fixed)
  //SDL_ShowCursor(1);
}

void CWinSystemX11GLES::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();
}

bool CWinSystemX11GLES::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    g_graphicsContext.ToggleFullScreenRoot();

  SDL_WM_IconifyWindow();
  return true;
}

bool CWinSystemX11GLES::Restore()
{
  return false;
}

bool CWinSystemX11GLES::Hide()
{
  XUnmapWindow(m_dpy, m_wmWindow);
  XSync(m_dpy, False);
  return true;
}

bool CWinSystemX11GLES::Show(bool raise)
{
  XMapWindow(m_dpy, m_wmWindow);
  XSync(m_dpy, False);
  return true;
}

EGLContext CWinSystemX11GLES::GetEGLContext() const
{
  return m_eglContext;
}

EGLDisplay CWinSystemX11GLES::GetEGLDisplay() const
{
  return m_eglDisplay;
}

bool CWinSystemX11GLES::makeOMXCurrent()
{
  return true;
}


#endif
