/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#if defined(HAS_GLX) || (defined(HAS_EGL) && defined(HAVE_X11))

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
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include "CompileInfo.h"
#include <X11/Xatom.h>

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#include "../WinEventsX11.h"
#include "input/MouseStat.h"

using namespace std;

#define EGL_NO_CONFIG (EGLConfig)0

CWinSystemX11::CWinSystemX11() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_X11;
#if defined(HAS_GLX)
  m_glContext = NULL;
#endif
#if defined(HAS_EGL)
  m_eglContext = EGL_NO_CONTEXT;
  m_eglDisplay = EGL_NO_DISPLAY;
#endif
  m_dpy = NULL;
  m_glWindow = 0;
  m_mainWindow = 0;
  m_bWasFullScreenBeforeMinimize = false;
  m_minimized = false;
  m_bIgnoreNextFocusMessage = false;
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

#if defined(HAS_GLX)
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
    // crashes when shutting down via cec
//    XCloseDisplay(m_dpy);
  }
#endif

#if defined(HAS_EGL)
  if (m_eglDisplay)
  {
    if (m_eglContext != EGL_NO_CONTEXT)
    {
      eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      m_eglContext = EGL_NO_CONTEXT;
    }
  }
#endif
  // m_SDLSurface is free()'d by SDL_Quit().

  return true;
}

bool CWinSystemX11::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if(!SetFullScreen(fullScreen, res, false))
    return false;

  m_bWindowCreated = true;
  return true;
}

bool CWinSystemX11::DestroyWindow()
{
  if (!m_mainWindow)
    return true;

#if defined(HAS_GLX)
  if (m_glContext)
  {
    glFinish();
    glXMakeCurrent(m_dpy, None, NULL);
  }
#endif
#if defined(HAS_EGL)
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
#endif

  if (m_invisibleCursor)
  {
    XUndefineCursor(m_dpy, m_mainWindow);
    XFreeCursor(m_dpy, m_invisibleCursor);
    m_invisibleCursor = 0;
  }

  CWinEventsX11Imp::Quit();

  XUnmapWindow(m_dpy, m_mainWindow);
  XDestroyWindow(m_dpy, m_glWindow);
  XDestroyWindow(m_dpy, m_mainWindow);
  m_glWindow = 0;
  m_mainWindow = 0;

  if (m_icon)
    XFreePixmap(m_dpy, m_icon);

  return true;
}

bool CWinSystemX11::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_userOutput = CSettings::Get().GetString("videoscreen.monitor");
  XOutput *out = NULL;
  if (m_userOutput.compare("Default") != 0)
  {
    out = g_xrandr.GetOutput(m_userOutput);
    if (out)
    {
      XMode mode = g_xrandr.GetCurrentMode(m_userOutput);
      if (!mode.isCurrent)
      {
        out = NULL;
      }
    }
  }
  if (!out)
  {
    std::vector<XOutput> outputs = g_xrandr.GetModes();
    if (outputs.size() > 0)
    {
      m_userOutput = outputs[0].name;
    }
  }

  if(m_nWidth  == newWidth &&
     m_nHeight == newHeight &&
     m_userOutput.compare(m_currentOutput) == 0)
  {
    UpdateCrtc();
    return true;
  }

  if (!SetWindow(newWidth, newHeight, false, m_userOutput))
  {
    return false;
  }

  m_nWidth  = newWidth;
  m_nHeight = newHeight;
  m_bFullScreen = false;
  m_currentOutput = m_userOutput;

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
  if (!currmode.name.empty())
  {
    // flip h/w when rotated
    if (m_bIsRotated)
    {
      int w = mode.w;
      mode.w = mode.h;
      mode.h = w;
    }

    // only call xrandr if mode changes
    if (m_mainWindow)
    {
      if (currmode.w != mode.w || currmode.h != mode.h ||
          currmode.hz != mode.hz || currmode.id != mode.id)
      {
        CLog::Log(LOGNOTICE, "CWinSystemX11::SetFullScreen - calling xrandr");

        // remember last position of mouse
        Window root_return, child_return;
        int root_x_return, root_y_return;
        int win_x_return, win_y_return;
        unsigned int mask_return;
        bool isInWin = XQueryPointer(m_dpy, m_mainWindow, &root_return, &child_return,
                                     &root_x_return, &root_y_return,
                                     &win_x_return, &win_y_return,
                                     &mask_return);

        if (isInWin)
        {
          m_MouseX = win_x_return;
          m_MouseY = win_y_return;
        }
        else
        {
          m_MouseX = -1;
          m_MouseY = -1;
        }

        OnLostDevice();
        m_bIsInternalXrr = true;
        g_xrandr.SetMode(out, mode);
        return true;
      }
    }
  }
#endif

  if (!SetWindow(res.iWidth, res.iHeight, fullScreen, m_userOutput))
    return false;

  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;
  m_currentOutput = m_userOutput;

  return true;
}

void CWinSystemX11::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

#if defined(HAS_XRANDR)
  int numScreens = XScreenCount(m_dpy);
  g_xrandr.SetNumScreens(numScreens);

  bool switchOnOff = CSettings::Get().GetBool("videoscreen.blankdisplays");
  m_userOutput = CSettings::Get().GetString("videoscreen.monitor");
  if (m_userOutput.compare("Default") == 0)
    switchOnOff = false;

  if(g_xrandr.Query(true, !switchOnOff))
  {
    XOutput *out = NULL;
    if (m_userOutput.compare("Default") != 0)
    {
      out = g_xrandr.GetOutput(m_userOutput);
      if (out)
      {
        XMode mode = g_xrandr.GetCurrentMode(m_userOutput);
        if (!mode.isCurrent && !switchOnOff)
        {
          out = NULL;
        }
      }
    }
    if (!out)
    {
      m_userOutput = g_xrandr.GetModes()[0].name;
      out = g_xrandr.GetOutput(m_userOutput);
    }

    if (switchOnOff)
    {
      // switch on output
      g_xrandr.TurnOnOutput(m_userOutput);

      // switch off other outputs
      std::vector<XOutput> outputs = g_xrandr.GetModes();
      for (size_t i=0; i<outputs.size(); i++)
      {
        if (StringUtils::EqualsNoCase(outputs[i].name, m_userOutput))
          continue;
        g_xrandr.TurnOffOutput(outputs[i].name);
      }
    }

    XMode mode = g_xrandr.GetCurrentMode(m_userOutput);
    if (mode.id.empty())
      mode = g_xrandr.GetPreferredMode(m_userOutput);
    m_bIsRotated = out->isRotated;
    if (!m_bIsRotated)
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.w, mode.h, mode.hz);
    else
      UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, mode.h, mode.w, mode.hz);
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strId     = mode.id;
    CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP).strOutput = m_userOutput;
  }
  else
#endif
  {
    m_userOutput = "No Output";
    m_nScreen = DefaultScreen(m_dpy);
    int w = DisplayWidth(m_dpy, m_nScreen);
    int h = DisplayHeight(m_dpy, m_nScreen);
    UpdateDesktopResolution(CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP), 0, w, h, 0.0);
  }

#if defined(HAS_XRANDR)

  // erase previous stored modes
  CDisplaySettings::Get().ClearCustomResolutions();

  CLog::Log(LOGINFO, "Available videomodes (xrandr):");

  XOutput *out = g_xrandr.GetOutput(m_userOutput);
  if (out != NULL)
  {
    vector<XMode>::iterator modeiter;
    CLog::Log(LOGINFO, "Output '%s' has %" PRIdS" modes", out->name.c_str(), out->modes.size());

    for (modeiter = out->modes.begin() ; modeiter!=out->modes.end() ; modeiter++)
    {
      XMode mode = *modeiter;
      CLog::Log(LOGINFO, "ID:%s Name:%s Refresh:%f Width:%d Height:%d",
                mode.id.c_str(), mode.name.c_str(), mode.hz, mode.w, mode.h);
      RESOLUTION_INFO res;
      res.iScreen = 0; // not used by X11
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

      res.strMode      = StringUtils::Format("%s: %s @ %.2fHz", out->name.c_str(), mode.name.c_str(), mode.hz);
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
  if (!StringUtils::EqualsNoCase(out->name, resInfo.strOutput))
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

void CWinSystemX11::GetConnectedOutputs(std::vector<std::string> *outputs)
{
  vector<XOutput> outs;
  g_xrandr.Query(true);
  outs = g_xrandr.GetModes();
  outputs->push_back("Default");
  for(unsigned int i=0; i<outs.size(); ++i)
  {
    outputs->push_back(outs[i].name);
  }
}

bool CWinSystemX11::IsCurrentOutput(std::string output)
{
  return (StringUtils::EqualsNoCase(output, "Default")) || (m_currentOutput.compare(output.c_str()) == 0);
}

#if defined(HAS_EGL)
EGLConfig getEGLConfig(EGLDisplay eglDisplay, XVisualInfo *vInfo)
{
  EGLint attributes[] = {
    EGL_NONE
  };
  EGLint numConfigs;
  // TODO make dynamic
  EGLConfig eglConfigs[1024];
  EGLConfig eglConfig = EGL_NO_CONFIG;
  if (!eglChooseConfig(eglDisplay, attributes, eglConfigs, 1024, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query egl configs");
    return EGL_NO_CONFIG;
  }
  for (EGLint i = 0;i < numConfigs;++i)
  {
    EGLint value;
    if (!eglGetConfigAttrib(eglDisplay, eglConfigs[i], EGL_NATIVE_VISUAL_ID, &value))
    {
      CLog::Log(LOGERROR, "Failed to query EGL_NATIVE_VISUAL_ID for egl config.");
      break;
    }
    if (value == (EGLint)vInfo->visualid) {
      eglConfig = eglConfigs[i];
      break;
    }
  }

  return eglConfig;
}

#endif

bool CWinSystemX11::IsSuitableVisual(XVisualInfo *vInfo)
{
#if defined(HAS_GLX)
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
#endif

#if defined(HAS_EGL)
  EGLConfig config = getEGLConfig(m_eglDisplay, vInfo);
  if (config == EGL_NO_CONFIG)
  {
    CLog::Log(LOGERROR, "Failed to determine egl config for visual info");
    return false;
  }
  EGLint value;

  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_RED_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_GREEN_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_BLUE_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_ALPHA_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_DEPTH_SIZE, &value) || value < 24)
    return false;
 
#endif
  return true;
}

bool CWinSystemX11::RefreshGlxContext(bool force)
{
  bool retVal = false;

#if defined(HAS_GLX)
  if (m_glContext && !force)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshGlxContext: refreshing context");
    glXMakeCurrent(m_dpy, None, NULL);
    glXMakeCurrent(m_dpy, m_glWindow, m_glContext);
    return true;
  }
#endif

#if defined(HAS_EGL)
  if (m_eglContext && !force)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshGlxContext: refreshing context");
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    return true;
  }
#endif


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
#if defined(HAS_GLX)
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
#endif
#if defined(HAS_EGL)
    if (m_eglContext)
    {
      eglMakeCurrent(m_eglContext, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglDestroyContext(m_eglDisplay, m_eglContext);
      m_eglContext = EGL_NO_CONTEXT;
      eglDestroySurface(m_eglDisplay, m_eglSurface);
      m_eglSurface = EGL_NO_SURFACE;
      eglTerminate(m_eglDisplay);
      m_eglDisplay = EGL_NO_DISPLAY;
      XSync(m_dpy, FALSE);
      m_newGlContext = true;
    }

    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);
    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CLog::Log(LOGERROR, "failed to get egl display\n");
      return false;
    }
    if (!eglInitialize(m_eglDisplay, NULL, NULL))
    {
      CLog::Log(LOGERROR, "failed to initialize egl\n");
      return false;
    }

    EGLConfig eglConfig = getEGLConfig(m_eglDisplay, vInfo);

    if (eglConfig == EGL_NO_CONFIG)
    {
      CLog::Log(LOGERROR, "failed to get eglconfig for visual id\n");
      return false;
    }

    if (m_eglSurface == EGL_NO_SURFACE)
    {
      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, eglConfig, m_glWindow, NULL);
      if (m_eglSurface == EGL_NO_SURFACE)
      {
        CLog::Log(LOGERROR, "failed to create EGL window surface %d\n", eglGetError());
        return false;
      }
    }

    GLint contextAttributes[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttributes);
    if (m_eglContext == EGL_NO_CONTEXT)
    {
      CLog::Log(LOGERROR, "failed to create EGL context\n");
      return false;
    }

    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
    {
      CLog::Log(LOGERROR, "Failed to make context current %p %p %p\n", m_eglDisplay, m_eglSurface, m_eglContext);
      return false;
    }
#endif
    XFree(vInfo);
  }
  else
  {
    CLog::Log(LOGERROR, "EGL/GLX Error: vInfo is NULL!");
  }

  return retVal;
}

void CWinSystemX11::ShowOSMouse(bool show)
{
  if (show)
    XUndefineCursor(m_dpy,m_mainWindow);
  else if (m_invisibleCursor)
    XDefineCursor(m_dpy,m_mainWindow, m_invisibleCursor);
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
    XQueryPointer(m_dpy, RootWindow(m_dpy, m_nScreen), &root_return, &child_return,
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

bool CWinSystemX11::Minimize()
{
  m_bWasFullScreenBeforeMinimize = m_bFullScreen;
  if (m_bWasFullScreenBeforeMinimize)
  {
    m_bIgnoreNextFocusMessage = true;
    g_graphicsContext.ToggleFullScreenRoot();
  }

  XIconifyWindow(m_dpy, m_mainWindow, m_nScreen);

  m_minimized = true;
  return true;
}
bool CWinSystemX11::Restore()
{
  return false;
}
bool CWinSystemX11::Hide()
{
  XUnmapWindow(m_dpy, m_mainWindow);
  XFlush(m_dpy);
  return true;
}
bool CWinSystemX11::Show(bool raise)
{
  XMapWindow(m_dpy, m_mainWindow);
  XFlush(m_dpy);
  m_minimized = false;
  return true;
}

void CWinSystemX11::NotifyXRREvent()
{
  CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);

  CSingleLock lock(g_graphicsContext);

  if (!g_xrandr.Query(true))
  {
    CLog::Log(LOGERROR, "WinSystemX11::RefreshWindow - failed to query xrandr");
    return;
  }

  // if external event update resolutions
  if (!m_bIsInternalXrr)
  {
    UpdateResolutions();
  }

  RecreateWindow();
}

void CWinSystemX11::RecreateWindow()
{
  m_windowDirty = true;

  CSingleLock lock(g_graphicsContext);

  XOutput *out = g_xrandr.GetOutput(m_userOutput);
  XMode   mode = g_xrandr.GetCurrentMode(m_userOutput);

  if (out)
    CLog::Log(LOGDEBUG, "%s - current output: %s, mode: %s, refresh: %.3f", __FUNCTION__
             , out->name.c_str(), mode.id.c_str(), mode.hz);
  else
    CLog::Log(LOGWARNING, "%s - output name not set", __FUNCTION__);

  RESOLUTION_INFO res;
  unsigned int i;
  bool found(false);
  for (i = RES_DESKTOP; i < CDisplaySettings::Get().ResolutionInfoSize(); ++i)
  {
    res = CDisplaySettings::Get().GetResolutionInfo(i);
    if (StringUtils::EqualsNoCase(CDisplaySettings::Get().GetResolutionInfo(i).strId, mode.id))
    {
      found = true;
      break;
    }
  }

  if (!found)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::RecreateWindow - could not find resolution");
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

  CWinEventsX11Imp::SetXRRFailSafeTimer(3000);
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

bool CWinSystemX11::SetWindow(int width, int height, bool fullscreen, const std::string &output)
{
  bool changeWindow = false;
  bool changeSize = false;
  float mouseX = 0.5;
  float mouseY = 0.5;

  if (!m_mainWindow)
  {
    g_Mouse.SetActive(false);
  }

  if (m_mainWindow && ((m_bFullScreen != fullscreen) || m_currentOutput.compare(output) != 0 || m_windowDirty))
  {
    // set mouse to last known position
    // we can't trust values after an xrr event
    if (m_bIsInternalXrr && m_MouseX >= 0 && m_MouseY >= 0)
    {
      mouseX = (float)m_MouseX/m_nWidth;
      mouseY = (float)m_MouseY/m_nHeight;
    }
    else if (!m_windowDirty)
    {
      Window root_return, child_return;
      int root_x_return, root_y_return;
      int win_x_return, win_y_return;
      unsigned int mask_return;
      bool isInWin = XQueryPointer(m_dpy, m_mainWindow, &root_return, &child_return,
                                   &root_x_return, &root_y_return,
                                   &win_x_return, &win_y_return,
                                   &mask_return);

      if (isInWin)
      {
        mouseX = (float)win_x_return/m_nWidth;
        mouseY = (float)win_y_return/m_nHeight;
      }
    }

    g_Mouse.SetActive(false);
    OnLostDevice();
    DestroyWindow();
    m_windowDirty = true;
  }

  // create main window
  if (!m_mainWindow)
  {
    EnableSystemScreenSaver(false);

#if defined(HAS_GLX)
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
#endif
#if defined(HAS_EGL)
    GLint att[] =
    {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_BUFFER_SIZE, 32,
      EGL_DEPTH_SIZE, 24,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
    };
#endif

    Colormap cmap;
    XSetWindowAttributes swa;
    XVisualInfo *vi;
    int x0 = 0;
    int y0 = 0;

    XOutput *out = g_xrandr.GetOutput(output);
    if (!out)
      out = g_xrandr.GetOutput(m_currentOutput);
    if (out)
    {
      m_nScreen = out->screen;
      x0 = out->x;
      y0 = out->y;
    }

#if defined(HAS_GLX)
    vi = glXChooseVisual(m_dpy, m_nScreen, att);
#endif
#if defined(HAS_EGL)
    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);
      if (m_eglDisplay == EGL_NO_DISPLAY)
      {
        CLog::Log(LOGERROR, "failed to get egl display\n");
	return false;
      }
      if (!eglInitialize(m_eglDisplay, NULL, NULL))
      {
	CLog::Log(LOGERROR, "failed to initialize egl display\n");
	return false;
      }
    }

    EGLint numConfigs;
    EGLConfig eglConfig = 0;
    if (!eglChooseConfig(m_eglDisplay, att, &eglConfig, 1, &numConfigs) || numConfigs == 0) {
      CLog::Log(LOGERROR, "Failed to choose a config %d\n", eglGetError());
    }

    XVisualInfo x11_visual_info_template;
    if (!eglGetConfigAttrib(m_eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, (EGLint*)&x11_visual_info_template.visualid)) {
      CLog::Log(LOGERROR, "Failed to query native visual id\n");
    }
    int num_visuals;
    vi = XGetVisualInfo(m_dpy,
                        VisualIDMask,
			&x11_visual_info_template,
			&num_visuals);

#endif
    cmap = XCreateColormap(m_dpy, RootWindow(m_dpy, vi->screen), vi->visual, AllocNone);

    bool hasWM = HasWindowManager();

    int def_vis = (vi->visual == DefaultVisual(m_dpy, vi->screen));
    swa.override_redirect = hasWM ? False : True;
    swa.border_pixel = fullscreen ? 0 : 5;
    swa.background_pixel = def_vis ? BlackPixel(m_dpy, vi->screen) : 0;
    swa.colormap = cmap;
    swa.event_mask = FocusChangeMask | KeyPressMask | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     PropertyChangeMask | StructureNotifyMask | KeymapStateMask |
                     EnterWindowMask | LeaveWindowMask | ExposureMask;
    unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask;

    m_mainWindow = XCreateWindow(m_dpy, RootWindow(m_dpy, vi->screen),
                    x0, y0, width, height, 0, vi->depth,
                    InputOutput, vi->visual,
                    mask, &swa);

    swa.override_redirect = False;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWColormap | CWEventMask;

    m_glWindow = XCreateWindow(m_dpy, m_mainWindow,
                    0, 0, width, height, 0, vi->depth,
                    InputOutput, vi->visual,
                    mask, &swa);

    if (fullscreen && hasWM)
    {
      Atom fs = XInternAtom(m_dpy, "_NET_WM_STATE_FULLSCREEN", True);
      XChangeProperty(m_dpy, m_mainWindow, XInternAtom(m_dpy, "_NET_WM_STATE", True), XA_ATOM, 32, PropModeReplace, (unsigned char *) &fs, 1);
    }

    // define invisible cursor
    Pixmap bitmapNoData;
    XColor black;
    static char noData[] = { 0,0,0,0,0,0,0,0 };
    black.red = black.green = black.blue = 0;

    bitmapNoData = XCreateBitmapFromData(m_dpy, m_mainWindow, noData, 8, 8);
    m_invisibleCursor = XCreatePixmapCursor(m_dpy, bitmapNoData, bitmapNoData,
                                            &black, &black, 0, 0);
    XFreePixmap(m_dpy, bitmapNoData);
    XDefineCursor(m_dpy,m_mainWindow, m_invisibleCursor);

    //init X11 events
    CWinEventsX11Imp::Init(m_dpy, m_mainWindow);

    changeWindow = true;
    changeSize = true;

#if defined(HAS_EGL)
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, eglConfig, m_glWindow, NULL);
    if (m_eglSurface == EGL_NO_SURFACE)
    {
      CLog::Log(LOGERROR, "failed to create egl window surface\n");
      return false;
    }
#endif
  }

  if (!CWinEventsX11Imp::HasStructureChanged() && ((width != m_nWidth) || (height != m_nHeight)))
  {
    changeSize = true;
  }

  if (changeSize || changeWindow)
  {
    XResizeWindow(m_dpy, m_mainWindow, width, height);
  }

  if ((width != m_nWidth) || (height != m_nHeight) || changeWindow)
  {
    XResizeWindow(m_dpy, m_glWindow, width, height);
  }

  if (changeWindow)
  {
    m_icon = None;
    {
      CreateIconPixmap();
      XWMHints *wm_hints;
      XClassHint *class_hints;
      XTextProperty windowName, iconName;

      std::string titleString = CCompileInfo::GetAppName();
      std::string classString = titleString;
      char *title = (char*)titleString.c_str();

      XStringListToTextProperty(&title, 1, &windowName);
      XStringListToTextProperty(&title, 1, &iconName);

      wm_hints = XAllocWMHints();
      wm_hints->initial_state = NormalState;
      wm_hints->icon_pixmap = m_icon;
      wm_hints->flags = StateHint | IconPixmapHint;

      class_hints = XAllocClassHint();
      class_hints->res_class = (char*)classString.c_str();
      class_hints->res_name = (char*)classString.c_str();

      XSetWMProperties(m_dpy, m_mainWindow, &windowName, &iconName,
                            NULL, 0, NULL, wm_hints,
                            class_hints);
      XFree(class_hints);
      XFree(wm_hints);

      // register interest in the delete window message
      Atom wmDeleteMessage = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);
      XSetWMProtocols(m_dpy, m_mainWindow, &wmDeleteMessage, 1);
    }

    // placement of window may follow mouse
    XWarpPointer(m_dpy, None, m_mainWindow, 0, 0, 0, 0, mouseX*width, mouseY*height);

    XMapRaised(m_dpy, m_glWindow);
    XMapRaised(m_dpy, m_mainWindow);

    // discard events generated by creating the window, i.e. xrr events
    XSync(m_dpy, TRUE);

    CDirtyRegionList dr;
    RefreshGlxContext(m_currentOutput.compare(output) != 0);
    XSync(m_dpy, FALSE);
    g_graphicsContext.Clear(0);
    g_graphicsContext.Flip(dr);
#if defined(HAS_GLX)
    g_Windowing.ResetVSync();
#endif
    m_windowDirty = false;
    m_bIsInternalXrr = false;

// what's this???
#if defined(HAS_GLX)
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnResetDevice();
#endif
  }

  UpdateCrtc();

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
  unsigned int i,j;
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

  CBaseTexture *iconTexture = CBaseTexture::LoadFromFile("special://xbmc/media/icon256x256.png");

  if (!iconTexture)
    return false;

  buf = iconTexture->GetPixels();

  if (depth>=24)
    numNewBufBytes = (4 * (iconTexture->GetWidth() * iconTexture->GetHeight()));
  else
    numNewBufBytes = (2 * (iconTexture->GetWidth() * iconTexture->GetHeight()));

  newBuf = (uint32_t*)malloc(numNewBufBytes);
  if (!newBuf)
  {
    CLog::Log(LOGERROR, "CWinSystemX11::CreateIconPixmap - malloc failed");
    return false;
  }

  for (i=0; i<iconTexture->GetHeight();++i)
  {
    for (j=0; j<iconTexture->GetWidth();++j)
    {
      unsigned int pos = i*iconTexture->GetPitch()+j*4;
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
                     iconTexture->GetWidth(), iconTexture->GetHeight(),
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

  delete iconTexture;

  return true;
}

bool CWinSystemX11::HasWindowManager()
{
  Window wm_check;
  unsigned char *data;
  int status, real_format;
  Atom real_type, prop;
  unsigned long items_read, items_left;

  prop = XInternAtom(m_dpy, "_NET_SUPPORTING_WM_CHECK", True);
  if (prop == None)
    return false;
  status = XGetWindowProperty(m_dpy, DefaultRootWindow(m_dpy), prop,
                      0L, 1L, False, XA_WINDOW, &real_type, &real_format,
                      &items_read, &items_left, &data);
  if(status != Success || ! items_read)
  {
    if(status == Success)
      XFree(data);
    return false;
  }

  wm_check = ((Window*)data)[0];
  XFree(data);

  status = XGetWindowProperty(m_dpy, wm_check, prop,
                      0L, 1L, False, XA_WINDOW, &real_type, &real_format,
                      &items_read, &items_left, &data);

  if(status != Success || !items_read)
  {
    if(status == Success)
      XFree(data);
    return false;
  }

  if(wm_check != ((Window*)data)[0])
  {
    XFree(data);
    return false;
  }

  XFree(data);

  prop = XInternAtom(m_dpy, "_NET_WM_NAME", True);
  if (prop == None)
  {
    CLog::Log(LOGDEBUG,"Window Manager Name: ");
    return true;
  }

  status = XGetWindowProperty(m_dpy, wm_check, prop,
                        0L, (~0L), False, AnyPropertyType, &real_type, &real_format,
                        &items_read, &items_left, &data);

  if(status == Success && items_read)
  {
    CLog::Log(LOGDEBUG,"Window Manager Name: %s", data);
  }
  else
    CLog::Log(LOGDEBUG,"Window Manager Name: ");

  if(status == Success)
    XFree(data);

  return true;
}

void CWinSystemX11::UpdateCrtc()
{
  XWindowAttributes winattr;
  int posx, posy;
  Window child;
  XGetWindowAttributes(m_dpy, m_mainWindow, &winattr);
  XTranslateCoordinates(m_dpy, m_mainWindow, RootWindow(m_dpy, m_nScreen), winattr.x, winattr.y,
                        &posx, &posy, &child);

  m_crtc = g_xrandr.GetCrtc(posx+winattr.width/2, posy+winattr.height/2);
}

#endif
