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

#if defined(HAS_GLX)

#include "video/videosync/VideoSyncGLX.h"
#include <sstream>
#include <X11/extensions/Xrandr.h>
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include <string>

using namespace std;

Display* CVideoSyncGLX::m_Dpy = NULL;

void CVideoSyncGLX::OnLostDevice()
{
  if (!m_displayLost)
  {
    m_displayLost = true;
    m_lostEvent.Wait();
  }
}

void CVideoSyncGLX::OnResetDevice()
{
  m_displayReset = true;
}

bool CVideoSyncGLX::Setup(PUPDATECLOCK func)
{
  CSingleLock lock(g_graphicsContext);

  m_glXWaitVideoSyncSGI = NULL;
  m_glXGetVideoSyncSGI = NULL;
  m_vInfo = NULL;
  m_Window = 0;
  m_Context = NULL;
  UpdateClock = func;

  int singleBufferAttributes[] = {
    GLX_RGBA,
    GLX_RED_SIZE,      0,
    GLX_GREEN_SIZE,    0,
    GLX_BLUE_SIZE,     0,
    None
  };

  int ReturnV, SwaMask;
  unsigned int GlxTest;
  XSetWindowAttributes Swa;

  m_vInfo = NULL;
  m_Context = NULL;
  m_Window = 0;

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up GLX");

  g_Windowing.Register(this);

  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();

  if (!m_Dpy)
  {
    m_Dpy = XOpenDisplay(NULL);
    if (!m_Dpy)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to open display");
      return false;
    }
  }

  if (!glXQueryExtension(m_Dpy, NULL, NULL))
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: X server does not support GLX");
    return false;
  }

  bool          ExtensionFound = false;
  istringstream Extensions(glXQueryExtensionsString(m_Dpy, g_Windowing.GetCurrentScreen()));
  string        ExtensionStr;

  while (!ExtensionFound)
  {
    Extensions >> ExtensionStr;
    if (Extensions.fail())
      break;

    if (ExtensionStr == "GLX_SGI_video_sync")
      ExtensionFound = true;
  }

  if (!ExtensionFound)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: X server does not support GLX_SGI_video_sync");
    return false;
  }

  m_vInfo = glXChooseVisual(m_Dpy, g_Windowing.GetCurrentScreen(), singleBufferAttributes);
  if (!m_vInfo)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXChooseVisual returned NULL");
    return false;
  }

  Swa.border_pixel = 0;
  Swa.event_mask = StructureNotifyMask;
  Swa.colormap = XCreateColormap(m_Dpy, g_Windowing.GetWindow(), m_vInfo->visual, AllocNone );
  SwaMask = CWBorderPixel | CWColormap | CWEventMask;

  m_Window = XCreateWindow(m_Dpy, g_Windowing.GetWindow(), 0, 0, 256, 256, 0,
                           m_vInfo->depth, InputOutput, m_vInfo->visual, SwaMask, &Swa);

  m_Context = glXCreateContext(m_Dpy, m_vInfo, NULL, True);
  if (!m_Context)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXCreateContext returned NULL");
    return false;
  }

  ReturnV = glXMakeCurrent(m_Dpy, m_Window, m_Context);
  if (ReturnV != True)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
    return false;
  }

  m_glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  if (!m_glXWaitVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI not found");
    return false;
  }

  ReturnV = m_glXWaitVideoSyncSGI(2, 0, &GlxTest);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
    return false;
  }

  m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!m_glXGetVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI not found");
    return false;
  }

  ReturnV = m_glXGetVideoSyncSGI(&GlxTest);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI returned %i", ReturnV);
    return false;
  }

  return true;
}

void CVideoSyncGLX::Run(volatile bool& stop)
{
  unsigned int  PrevVblankCount;
  unsigned int  VblankCount;
  int           ReturnV;
  bool          IsReset = false;
  int64_t       Now;

  //get the current vblank counter
  m_glXGetVideoSyncSGI(&VblankCount);
  PrevVblankCount = VblankCount;

  while(!stop && !m_displayLost && !m_displayReset)
  {
    //wait for the next vblank
    ReturnV = m_glXWaitVideoSyncSGI(2, (VblankCount + 1) % 2, &VblankCount);
    m_glXGetVideoSyncSGI(&VblankCount); //the vblank count returned by glXWaitVideoSyncSGI is not always correct
    Now = CurrentHostCounter();         //get the timestamp of this vblank

    if(ReturnV)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
      return;
    }

    if (VblankCount > PrevVblankCount)
    {
      UpdateClock((int)(VblankCount - PrevVblankCount), Now);
      IsReset = false;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Vblank counter has reset");

      //only try reattaching once
      if (IsReset)
        return;

      //because of a bug in the nvidia driver, glXWaitVideoSyncSGI breaks when the vblank counter resets
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detaching glX context");
      ReturnV = glXMakeCurrent(m_Dpy, None, NULL);
      if (ReturnV != True)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
        return;
      }

      //sleep here so we don't busy spin when this constantly happens, for example when the display went to sleep
      Sleep(1000);

      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Attaching glX context");
      ReturnV = glXMakeCurrent(m_Dpy, m_Window, m_Context);
      if (ReturnV != True)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
        return;
      }

      m_glXGetVideoSyncSGI(&VblankCount);

      IsReset = true;
    }
    PrevVblankCount = VblankCount;
  }
  m_lostEvent.Set();
  while(!stop && m_displayLost && !m_displayReset)
  {
    Sleep(10);
  }
}

void CVideoSyncGLX::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up GLX");

  {
    CSingleLock lock(g_graphicsContext);

    if (m_vInfo)
    {
      XFree(m_vInfo);
      m_vInfo = NULL;
    }
    if (m_Context)
    {
      glXMakeCurrent(m_Dpy, None, NULL);
      glXDestroyContext(m_Dpy, m_Context);
      m_Context = NULL;
    }
    if (m_Window)
    {
      XDestroyWindow(m_Dpy, m_Window);
      m_Window = 0;
    }
  }

  m_lostEvent.Set();
  g_Windowing.Unregister(this);
}

float CVideoSyncGLX::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  return m_fps;
}

#endif
