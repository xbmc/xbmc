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

#include "system_gl.h"

#if defined(HAVE_X11) && defined(HAS_GL)

#include <GL/glx.h>
#include "GLContextGLX.h"
#include "utils/log.h"

CGLContextGLX::CGLContextGLX(Display *dpy) : CGLContext(dpy)
{
  m_extPrefix = "GLX_";
  m_glxWindow = 0;
  m_glxContext = 0;
  m_vsyncMode = 0;
}

bool CGLContextGLX::Refresh(bool force, int screen, Window glWindow, bool &newContext)
{
  bool retVal = false;
  m_glxWindow = glWindow;
  m_nScreen = screen;

  // refresh context
  if (m_glxContext && !force)
  {
    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshGlxContext: refreshing context");
    glXMakeCurrent(m_dpy, None, NULL);
    glXMakeCurrent(m_dpy, glWindow, m_glxContext);
    return true;
  }

  // create context

  XVisualInfo vMask;
  XVisualInfo *visuals;
  XVisualInfo *vInfo = NULL;
  int availableVisuals = 0;
  vMask.screen = screen;
  XWindowAttributes winAttr;

  /* Assume a depth of 24 in case the below calls to XGetWindowAttributes()
     or XGetVisualInfo() fail. That shouldn't happen unless something is
     fatally wrong, but lets prepare for everything. */
  vMask.depth = 24;

  if (XGetWindowAttributes(m_dpy, glWindow, &winAttr))
  {
    vMask.visualid = XVisualIDFromVisual(winAttr.visual);
    vInfo = XGetVisualInfo(m_dpy, VisualScreenMask | VisualIDMask, &vMask, &availableVisuals);
    if (!vInfo)
      CLog::Log(LOGWARNING, "Failed to get VisualInfo of visual 0x%x", (unsigned) vMask.visualid);
    else if(!IsSuitableVisual(vInfo))
    {
      CLog::Log(LOGWARNING, "Visual 0x%x of the window is not suitable, looking for another one...",
                (unsigned) vInfo->visualid);
      vMask.depth = vInfo->depth;
      XFree(vInfo);
      vInfo = NULL;
    }
  }
  else
    CLog::Log(LOGWARNING, "Failed to get window attributes");

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
    if (m_glxContext)
    {
      glXMakeCurrent(m_dpy, None, NULL);
      glXDestroyContext(m_dpy, m_glxContext);
      XSync(m_dpy, FALSE);
    }

    if ((m_glxContext = glXCreateContext(m_dpy, vInfo, NULL, True)))
    {
      // make this context current
      glXMakeCurrent(m_dpy, glWindow, m_glxContext);
      retVal = true;
    }
    else
      CLog::Log(LOGERROR, "GLX Error: Could not create context");

    XFree(vInfo);
  }
  else
  {
    CLog::Log(LOGERROR, "GLX Error: vInfo is NULL!");
  }

  return retVal;
}

void CGLContextGLX::Destroy()
{
  glXMakeCurrent(m_dpy, None, NULL);
  glXDestroyContext(m_dpy, m_glxContext);
  m_glxContext = 0;
}

void CGLContextGLX::Detach()
{
  glXMakeCurrent(m_dpy, None, NULL);
}

bool CGLContextGLX::IsSuitableVisual(XVisualInfo *vInfo)
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

void CGLContextGLX::SetVSync(bool enable)
{
  // turn of current setting first
  if(m_glXSwapIntervalEXT)
    m_glXSwapIntervalEXT(m_dpy, m_glxWindow, 0);
  else if(m_glXSwapIntervalMESA)
    m_glXSwapIntervalMESA(0);

  m_iVSyncErrors = 0;

  if(!enable)
    return;

  if (m_glXSwapIntervalEXT)
  {
    m_glXSwapIntervalEXT(m_dpy, m_glxWindow, 1);
    m_vsyncMode = 6;
  }
  if (m_glXSwapIntervalMESA)
  {
    if(m_glXSwapIntervalMESA(1) == 0)
      m_vsyncMode = 2;
    else
      CLog::Log(LOGWARNING, "%s - glXSwapIntervalMESA failed", __FUNCTION__);
  }
  if (m_glXWaitVideoSyncSGI && m_glXGetVideoSyncSGI && !m_vsyncMode)
  {
    unsigned int count;
    if(m_glXGetVideoSyncSGI(&count) == 0)
      m_vsyncMode = 3;
    else
      CLog::Log(LOGWARNING, "%s - glXGetVideoSyncSGI failed, glcontext probably not direct", __FUNCTION__);
  }
}

void CGLContextGLX::SwapBuffers()
{
  if (m_vsyncMode == 3)
  {
    glFinish();
    unsigned int before = 0, after = 0;
    if (m_glXGetVideoSyncSGI(&before) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    glXSwapBuffers(m_dpy, m_glxWindow);
    glFinish();

    if(m_glXGetVideoSyncSGI(&after) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    if (after == before)
      m_iVSyncErrors = 1;
    else
      m_iVSyncErrors--;

    if (m_iVSyncErrors > 0)
    {
      CLog::Log(LOGINFO, "GL: retrace count didn't change after buffer swap, switching to vsync mode 4");
      m_iVSyncErrors = 0;
      m_vsyncMode = 4;
    }

    if (m_iVSyncErrors < -200)
    {
      CLog::Log(LOGINFO, "GL: retrace count change for %d consecutive buffer swap, switching to vsync mode 2", -m_iVSyncErrors);
      m_iVSyncErrors = 0;
      m_vsyncMode = 2;
    }
  }
  else if (m_vsyncMode == 4)
  {
    glFinish();
    unsigned int before = 0, swap = 0, after = 0;
    if (m_glXGetVideoSyncSGI(&before) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    if(m_glXWaitVideoSyncSGI(2, (before+1)%2, &swap) != 0)
      CLog::Log(LOGERROR, "%s - glXWaitVideoSyncSGI - Returned error", __FUNCTION__);

    glXSwapBuffers(m_dpy, m_glxWindow);
    glFinish();

    if (m_glXGetVideoSyncSGI(&after) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    if (after == before)
      CLog::Log(LOGERROR, "%s - glXWaitVideoSyncSGI - Woke up early", __FUNCTION__);

    if (after > before + 1)
      m_iVSyncErrors++;
    else
      m_iVSyncErrors = 0;

    if (m_iVSyncErrors > 30)
    {
      CLog::Log(LOGINFO, "GL: retrace count seems to be changing due to the swapbuffers call, switching to vsync mode 3");
      m_vsyncMode = 3;
      m_iVSyncErrors = 0;
    }
  }
  else
    glXSwapBuffers(m_dpy, m_glxWindow);
}

void CGLContextGLX::QueryExtensions()
{
  m_extensions  = " ";
  m_extensions += (const char*)glXQueryExtensionsString(m_dpy, m_nScreen);
  m_extensions += " ";

  CLog::Log(LOGDEBUG, "GLX_EXTENSIONS:%s", m_extensions.c_str());

  if (IsExtSupported("GLX_SGI_video_sync"))
    m_glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  else
    m_glXWaitVideoSyncSGI = NULL;

  if (IsExtSupported("GLX_SGI_video_sync"))
    m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  else
    m_glXGetVideoSyncSGI = NULL;

  if (IsExtSupported("GLX_MESA_swap_control"))
    m_glXSwapIntervalMESA = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
  else
    m_glXSwapIntervalMESA = NULL;

  if (IsExtSupported("GLX_EXT_swap_control"))
    m_glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
  else
    m_glXSwapIntervalEXT = NULL;
}

#endif
