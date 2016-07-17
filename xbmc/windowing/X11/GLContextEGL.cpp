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

#if defined(HAVE_X11) && defined(HAS_EGL)

#ifdef HAS_GL
  // always define GL_GLEXT_PROTOTYPES before include gl headers
  #if !defined(GL_GLEXT_PROTOTYPES)
    #define GL_GLEXT_PROTOTYPES
  #endif
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/glext.h>
#elif HAS_GLES == 2
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
#endif

#include "GLContextEGL.h"
#include "utils/log.h"

#define EGL_NO_CONFIG (EGLConfig)0

CGLContextEGL::CGLContextEGL(Display *dpy) : CGLContext(dpy)
{
  m_extPrefix = "EGL_";
  m_eglDisplay = EGL_NO_DISPLAY;
  m_eglSurface = EGL_NO_SURFACE;
  m_eglContext = EGL_NO_CONTEXT;
  m_eglConfig = EGL_NO_CONFIG;
}

CGLContextEGL::~CGLContextEGL()
{
  Destroy();
}

bool CGLContextEGL::Refresh(bool force, int screen, Window glWindow, bool &newContext)
{
  // refresh context
  if (m_eglContext && !force)
  {
    if (m_eglSurface == EGL_NO_SURFACE)
    {
      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, glWindow, NULL);
      if (m_eglSurface == EGL_NO_SURFACE)
      {
        CLog::Log(LOGERROR, "failed to create EGL window surface %d\n", eglGetError());
        return false;
      }
    }

    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshEGLContext: refreshing context");
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    return true;
  }

  // create context
  bool retVal = false;

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

  XVisualInfo vMask;
  XVisualInfo *visuals;
  XVisualInfo *vInfo      = NULL;
  int availableVisuals    = 0;
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

    if (m_eglContext)
    {
      eglMakeCurrent(m_eglContext, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglDestroyContext(m_eglDisplay, m_eglContext);
      m_eglContext = EGL_NO_CONTEXT;

      if (m_eglSurface)
      {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
      }
      eglTerminate(m_eglDisplay);
      m_eglDisplay = EGL_NO_DISPLAY;
      XSync(m_dpy, FALSE);
      newContext = true;
    }

    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);
    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CLog::Log(LOGERROR, "failed to get egl display");
      return false;
    }
    if (!eglInitialize(m_eglDisplay, NULL, NULL))
    {
      CLog::Log(LOGERROR, "failed to initialize egl\n");
      return false;
    }

#if defined (HAS_GL)
    if (!eglBindAPI(EGL_OPENGL_API))
    {
      CLog::Log(LOGERROR, "failed to initialize egl");
      XFree(vInfo);
      return false;
    }
#endif

    if(m_eglConfig == EGL_NO_CONFIG)
    {
      m_eglConfig = getEGLConfig(m_eglDisplay, vInfo);
    }

    if (m_eglConfig == EGL_NO_CONFIG)
    {
      CLog::Log(LOGERROR, "failed to get eglconfig for visual id");
      XFree(vInfo);
      return false;
    }

    if (m_eglSurface == EGL_NO_SURFACE)
    {
      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, glWindow, NULL);
      if (m_eglSurface == EGL_NO_SURFACE)
      {
        CLog::Log(LOGERROR, "failed to create EGL window surface %d", eglGetError());
        XFree(vInfo);
        return false;
      }
    }

    EGLint contextAttributes[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);
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
    XFree(vInfo);
    retVal = true;
  }
  else
  {
    CLog::Log(LOGERROR, "EGL Error: vInfo is NULL!");
  }

  return retVal;
}

void CGLContextEGL::Destroy()
{
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_eglDisplay, m_eglContext);
    m_eglContext = EGL_NO_CONTEXT;
  }

  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }

  if (m_eglDisplay)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CGLContextEGL::Detach()
{
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }
}

bool CGLContextEGL::IsSuitableVisual(XVisualInfo *vInfo)
{
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

  return true;
}

EGLConfig CGLContextEGL::getEGLConfig(EGLDisplay eglDisplay, XVisualInfo *vInfo)
{
  EGLint attributes[] =
  {
    EGL_DEPTH_SIZE, 24,
    EGL_ALPHA_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_NONE
  };
  EGLint numConfigs;

  if (!eglChooseConfig(eglDisplay, attributes, NULL, 0, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of egl configs");
    return EGL_NO_CONFIG;
  }
  if (numConfigs == 0)
  {
    CLog::Log(LOGERROR, "No suitable egl configs found");
    return EGL_NO_CONFIG;
  }

  EGLConfig *eglConfigs;
  eglConfigs = (EGLConfig*)malloc(numConfigs * sizeof(EGLConfig));
  if (!eglConfigs)
  {
    CLog::Log(LOGERROR, "eglConfigs malloc failed");
    return EGL_NO_CONFIG;
  }
  EGLConfig eglConfig = EGL_NO_CONFIG;
  if (!eglChooseConfig(eglDisplay, attributes, eglConfigs, numConfigs, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query egl configs");
    goto Exit;
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

Exit:
  free(eglConfigs);
  return eglConfig;
}

void CGLContextEGL::SetVSync(bool enable)
{
  eglSwapInterval(m_eglDisplay, enable ? 1 : 0);
}

void CGLContextEGL::SwapBuffers()
{
  if ((m_eglDisplay == EGL_NO_DISPLAY) || (m_eglSurface == EGL_NO_SURFACE))
    return;

  eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

void CGLContextEGL::QueryExtensions()
{
  std::string extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
  m_extensions = std::string(" ") + extensions + " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:%s", m_extensions.c_str());
}

XVisualInfo* CGLContextEGL::GetVisual()
{
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

    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);
      if (m_eglDisplay == EGL_NO_DISPLAY)
      {
        CLog::Log(LOGERROR, "failed to get egl display\n");
	return NULL;
      }
      if (!eglInitialize(m_eglDisplay, NULL, NULL))
      {
	CLog::Log(LOGERROR, "failed to initialize egl display\n");
	return NULL;
      }
    }

    EGLint numConfigs;
    EGLConfig eglConfig = 0;
    if (!eglChooseConfig(m_eglDisplay, att, &eglConfig, 1, &numConfigs) || numConfigs == 0) {
      CLog::Log(LOGERROR, "Failed to choose a config %d\n", eglGetError());
    }
    m_eglConfig=eglConfig;

    XVisualInfo x11_visual_info_template;
    if (!eglGetConfigAttrib(m_eglDisplay, m_eglConfig, EGL_NATIVE_VISUAL_ID, (EGLint*)&x11_visual_info_template.visualid)) {
      CLog::Log(LOGERROR, "Failed to query native visual id\n");
    }
    int num_visuals;
    return XGetVisualInfo(m_dpy,
                        VisualIDMask,
			&x11_visual_info_template,
			&num_visuals);
}

#endif
