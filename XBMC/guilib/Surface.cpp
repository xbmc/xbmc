/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*!
\file Surface.cpp
\brief
*/
#include "include.h"
#include "Surface.h"
#ifdef __APPLE__
#include "CocoaUtils.h"
#endif
#include <string>
#include "Settings.h"

using namespace Surface;

#ifdef HAS_SDL_OPENGL
#include <SDL/SDL_syswm.h>
#include "XBVideoConfig.h"
#endif

#ifdef HAS_GLX
Display* CSurface::s_dpy = 0;

static Bool WaitForNotify(Display *dpy, XEvent *event, XPointer arg)
{
  return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}
static Bool    (*_glXGetSyncValuesOML)(Display* dpy, GLXDrawable drawable, int64_t* ust, int64_t* msc, int64_t* sbc);
static int64_t (*_glXSwapBuffersMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor,int64_t remainder);

#endif

static __int64 abs(__int64 a)
{
  if(a < 0) return -a;
  else      return  a;
}

bool CSurface::b_glewInit = 0;
std::string CSurface::s_glVendor = "";
std::string CSurface::s_glRenderer = "";
int         CSurface::s_glMajVer = 0;
int         CSurface::s_glMinVer = 0;

#include "GraphicContext.h"

#ifdef _WIN32
static BOOL (APIENTRY *_wglSwapIntervalEXT)(GLint) = 0;
static int (APIENTRY *_wglGetSwapIntervalEXT)() = 0;
#elif defined(HAS_SDL_OPENGL) && !defined(__APPLE__)
static int (*_glXGetVideoSyncSGI)(unsigned int*) = 0;
static int (*_glXWaitVideoSyncSGI)(int, int, unsigned int*) = 0;
static int (*_glXSwapIntervalSGI)(int) = 0;
static int (*_glXSwapIntervalMESA)(int) = 0;
#endif

#ifdef HAS_SDL
CSurface::CSurface(int width, int height, bool doublebuffer, CSurface* shared,
                   CSurface* window, SDL_Surface* parent, bool fullscreen,
                   bool pixmap, bool pbuffer, int antialias)
{
  CLog::Log(LOGDEBUG, "Constructing surface %dx%d, shared=%p, fullscreen=%d\n", width, height, (void *)shared, fullscreen);
  m_bOK = false;
  m_iWidth = width;
  m_iHeight = height;
  m_bDoublebuffer = doublebuffer;
  m_iRedSize = 8;
  m_iGreenSize = 8;
  m_iBlueSize = 8;
  m_iAlphaSize = 8;
  m_bFullscreen = fullscreen;
  m_pShared = shared;
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
#ifdef HAVE_LIBVDPAU
  m_glPixmapTexture = 0;
#endif
#ifdef __APPLE__
  m_glContext = 0;
#endif

#ifdef _WIN32
  m_glDC = NULL;
  m_glContext = NULL;
  m_bCoversScreen = false;
  m_iOnTop = ONTOP_AUTO;

  timeBeginPeriod(1);
#endif

#ifdef HAS_GLX
  m_glWindow = 0;
  m_parentWindow = 0;
  m_glContext = 0;
  m_glPBuffer = 0;
  m_glPixmap = 0;
  m_Pixmap = 0;

  GLXFBConfig *fbConfigs = 0;
  bool mapWindow = false;
  XVisualInfo *vInfo = NULL;
  int num = 0;

  int singleVisAttributes[] =
    {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_RED_SIZE, m_iRedSize,
      GLX_GREEN_SIZE, m_iGreenSize,
      GLX_BLUE_SIZE, m_iBlueSize,
      GLX_ALPHA_SIZE, m_iAlphaSize,
      GLX_DEPTH_SIZE, 8,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      None
    };

  int doubleVisAttributes[] =
    {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_RED_SIZE, m_iRedSize,
      GLX_GREEN_SIZE, m_iGreenSize,
      GLX_BLUE_SIZE, m_iBlueSize,
      GLX_ALPHA_SIZE, m_iAlphaSize,
      GLX_DEPTH_SIZE, 8,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_DOUBLEBUFFER, True,
      None
    };

  int doubleVisAttributesAA[] =
    {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_RED_SIZE, m_iRedSize,
      GLX_GREEN_SIZE, m_iGreenSize,
      GLX_BLUE_SIZE, m_iBlueSize,
      GLX_ALPHA_SIZE, m_iAlphaSize,
      GLX_DEPTH_SIZE, 8,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_DOUBLEBUFFER, True,
      GLX_SAMPLE_BUFFERS, 1,
      None
    };

  // are we using an existing window to create the context?
  if (window)
  {
    // obtain the GLX window associated with the CSurface object
    m_glWindow = window->GetWindow();
    CLog::Log(LOGINFO, "GLX Info: Using destination window");
  } else {
    m_glWindow = 0;
    CLog::Log(LOGINFO, "GLX Info: NOT Using destination window");
  }

  if (!s_dpy)
  {
    // try to open root display
    s_dpy = XOpenDisplay(0);
    if (!s_dpy)
    {
      CLog::Log(LOGERROR, "GLX Error: No Display found");
      return;
    }
  }

  if (pbuffer)
  {
    MakePBuffer();
    return;
  }

  if (pixmap)
  {
    MakePixmap(width,height);
    return;
  }

  if (doublebuffer)
  {
    if (antialias)
    {
      // query compatible framebuffers based on double buffered AA attributes
      fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributesAA, &num);
      if (!fbConfigs)
      {
        CLog::Log(LOGWARNING, "GLX: No Multisample buffers available, FSAA disabled");
        fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
      }
    }
    else
    {
      // query compatible framebuffers based on double buffered attributes
      fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
    }
  }
  else
  {
    // query compatible framebuffers based on single buffered attributes (not used currently)
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);
  }
  if (fbConfigs==NULL)
  {
    CLog::Log(LOGERROR, "GLX Error: No compatible framebuffers found");
    return;
  }

  // obtain the xvisual from the first compatible framebuffer
  vInfo = glXGetVisualFromFBConfig(s_dpy, fbConfigs[0]);
  if (!vInfo) {
  CLog::Log(LOGERROR, "GLX Error: vInfo is NULL!");
  return;
  }

  // if no window is specified, create a window because a GL context needs to be
  // associated to a window
  if (!m_glWindow)
  {
    XSetWindowAttributes  swa;
    int swaMask;
    Window p;

    m_SDLSurface = parent;

    // check if a parent was passed
    if (parent)
    {
      SDL_SysWMinfo info;

      SDL_VERSION(&info.version);
      SDL_GetWMInfo(&info);
      m_parentWindow = p = info.info.x11.window;
      swaMask = 0;
      CLog::Log(LOGINFO, "GLX Info: Using parent window");
    }
    else
    {
      // create a window with the desktop as the parent
      p = RootWindow(s_dpy, vInfo->screen);
      swaMask = CWBorderPixel;
    }
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    swa.colormap = XCreateColormap(s_dpy, p, vInfo->visual, AllocNone);
    swaMask |= (CWColormap | CWEventMask);
    m_glWindow = XCreateWindow(s_dpy, p, 0, 0, m_iWidth, m_iHeight,
                               0, vInfo->depth, InputOutput, vInfo->visual,
                               swaMask, &swa );
    XSync(s_dpy, False);
    mapWindow = true;

    // success?
    if (!m_glWindow)
    {
      XFree(fbConfigs);
      CLog::Log(LOGERROR, "GLX Error: Could not create window");
      return;
    }
  }

  // now create the actual context
  if (shared)
  {
    CLog::Log(LOGINFO, "GLX Info: Creating shared context");
    m_glContext = glXCreateContext(s_dpy, vInfo, shared->GetContext(), True);
  } else {
    CLog::Log(LOGINFO, "GLX Info: Creating unshared context");
    m_glContext = glXCreateContext(s_dpy, vInfo, NULL, True);
  }
  XFree(fbConfigs);
  XFree(vInfo);

  // map the window and wait for notification that it was successful (X-specific)
  if (mapWindow)
  {
    XEvent event;
    XMapWindow(s_dpy, m_glWindow);
    XIfEvent(s_dpy, &event, WaitForNotify, (XPointer)m_glWindow);
  }

  // success?
  if (m_glContext)
  {
    // make this context current
    glXMakeCurrent(s_dpy, m_glWindow, m_glContext);

    // initialize glew only once (b_glewInit is static)
    if (!b_glewInit)
    {
      if (glewInit()!=GLEW_OK)
      {
        CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
      }
      else
      {
        b_glewInit = true;
      }
    }
    m_bOK = true;
  }
  else
  {
    CLog::Log(LOGERROR, "GLX Error: Could not create context");
  }
#elif defined(HAS_SDL_OPENGL)
#ifdef __APPLE__
  // We only want to call SDL_SetVideoMode if it's not shared, otherwise we'll create a new window.
  if (shared == 0)
  {
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   m_iRedSize);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, m_iGreenSize);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  m_iBlueSize);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, m_iAlphaSize);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, m_bDoublebuffer ? 1:0);
#ifdef __APPLE__
    // Enable vertical sync to avoid any tearing.
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    // We always want to use a shared context as we jump around in resolution,
    // otherwise we lose all our textures. However, contexts do not share correctly
    // between fullscreen and non-fullscreen.
    //
    shared = g_graphicsContext.getScreenSurface();

    // If we're coming from or going to fullscreen do NOT share.
    if (g_graphicsContext.getScreenSurface() != 0 &&
        (fullscreen == false && g_graphicsContext.getScreenSurface()->m_bFullscreen == true ||
         fullscreen == true  && g_graphicsContext.getScreenSurface()->m_bFullscreen == false))
    {
      shared =0;
    }

    // Make sure we DON'T call with SDL_FULLSCREEN, because we need a view!
    m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 0, SDL_OPENGL);

    // the context SDL creates isn't full screen compatible, so we create new one
    m_glContext = Cocoa_GL_ReplaceSDLWindowContext();
#else
    // We use the RESIZABLE flag or else the SDL wndproc won't let us ResizeSurface(), the
    // first sizing will replace the borderstyle to prevent allowing abritrary resizes
    int options = SDL_OPENGL | SDL_RESIZABLE | (fullscreen?SDL_NOFRAME:0);
    m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 0, options);
    m_glDC = wglGetCurrentDC();
    m_glContext = wglGetCurrentContext();
#endif
    if (m_SDLSurface)
    {
      m_bOK = true;
    }

    if (!b_glewInit)
    {
      if (glewInit()!=GLEW_OK)
      {
        CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
      }
      else
      {
        b_glewInit = true;
      }
    }

#ifdef __APPLE__
  }
  else
  {
    // Take the shared context.
    m_glContext = shared->m_glContext;
    MakeCurrent();
    m_bOK = true;
  }

#endif

#else
  int options = SDL_HWSURFACE | SDL_DOUBLEBUF;
  if (fullscreen)
  {
    options |= SDL_FULLSCREEN;
  }
  s_glVendor   = "Unknown";
  s_glRenderer = "Unknown";
  m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 32, options);
  if (m_SDLSurface)
  {
    m_bOK = true;
  }
#endif
#if defined(HAS_SDL_OPENGL)
  int temp;
  GetGLVersion(temp, temp);
#endif
  LogGraphicsInfo();
}
#endif

#ifdef HAS_GLX
bool CSurface::MakePBuffer()
{
  int num=0;
  GLXFBConfig *fbConfigs=NULL;
  XVisualInfo *visInfo=NULL;

  bool status = false;
  int singleVisAttributes[] =
    {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_RED_SIZE, m_iRedSize,
      GLX_GREEN_SIZE, m_iGreenSize,
      GLX_BLUE_SIZE, m_iBlueSize,
      GLX_ALPHA_SIZE, m_iAlphaSize,
      GLX_DEPTH_SIZE, 8,
      GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
      None
    };

  int doubleVisAttributes[] =
    {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_RED_SIZE, m_iRedSize,
      GLX_GREEN_SIZE, m_iGreenSize,
      GLX_BLUE_SIZE, m_iBlueSize,
      GLX_ALPHA_SIZE, m_iAlphaSize,
      GLX_DEPTH_SIZE, 8,
      GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
      GLX_DOUBLEBUFFER, True,
      None
    };

  int pbufferAttributes[] =
    {
      GLX_PBUFFER_WIDTH, m_iWidth,
      GLX_PBUFFER_HEIGHT, m_iHeight,
      GLX_LARGEST_PBUFFER, True,
      None
    };

  if (m_bDoublebuffer)
  {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
  } else {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);
  }
  if (fbConfigs==NULL)
  {
    CLog::Log(LOGERROR, "GLX Error: MakePBuffer: No compatible framebuffers found");
    XFree(fbConfigs);
    return status;
  }
  m_glPBuffer = glXCreatePbuffer(s_dpy, fbConfigs[0], pbufferAttributes);

  if (m_glPBuffer)
  {
    CLog::Log(LOGINFO, "GLX: Created PBuffer context");
    visInfo = glXGetVisualFromFBConfig(s_dpy, fbConfigs[0]);
    if (!visInfo)
    {
      CLog::Log(LOGINFO, "GLX Error: Could not obtain X Visual Info");
      return false;
    }
    if (m_pShared)
    {
      CLog::Log(LOGINFO, "GLX: Creating shared PBuffer context");
      m_glContext = glXCreateContext(s_dpy, visInfo, m_pShared->GetContext(), True);
    } else {
      CLog::Log(LOGINFO, "GLX: Creating unshared PBuffer context");
      m_glContext = glXCreateContext(s_dpy, visInfo, NULL, True);
    }
    XFree(visInfo);
    if (glXMakeCurrent(s_dpy, m_glPBuffer, m_glContext))
    {
      CLog::Log(LOGINFO, "GL: Initialised PBuffer");
      if (!b_glewInit)
      {
        if (glewInit()!=GLEW_OK)
        {
          CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
        } else {
          b_glewInit = true;
          if (s_glVendor.length()==0)
          {
            s_glVendor = (const char*)glGetString(GL_VENDOR);
            CLog::Log(LOGINFO, "GL: OpenGL Vendor String: %s", s_glVendor.c_str());
          }
        }
      }
      m_bOK = status = true;
    } else {
      CLog::Log(LOGINFO, "GLX Error: Could not make PBuffer current");
      status = false;
    }
  }
  else
  {
    CLog::Log(LOGINFO, "GLX Error: Could not create PBuffer");
    status = false;
  }
  XFree(fbConfigs);
  return status;
}

bool CSurface::MakePixmap(int width, int height)
{
  int num=0;
  GLXFBConfig *fbConfigs=NULL;
  int fbConfigIndex = 0;
  XVisualInfo *visInfo=NULL;

  bool status = false;
  int singleVisAttributes[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, m_iRedSize,
    GLX_GREEN_SIZE, m_iGreenSize,
    GLX_BLUE_SIZE, m_iBlueSize,
    GLX_ALPHA_SIZE, m_iAlphaSize,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
    GLX_X_RENDERABLE, True, // Added by Rob
    None
  };

  int doubleVisAttributes[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, m_iRedSize,
    GLX_GREEN_SIZE, m_iGreenSize,
    GLX_BLUE_SIZE, m_iBlueSize,
    GLX_ALPHA_SIZE, m_iAlphaSize,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
    GLX_DOUBLEBUFFER, True,
    GLX_Y_INVERTED_EXT, True,
    GLX_X_RENDERABLE, True, // Added by Rob
    None
  };

  int pixmapAttribs[] = {
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
    None
  };

  if (m_bDoublebuffer)
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
  else
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);

  // Get our window attribs.
  XWindowAttributes wndattribs;
  XGetWindowAttributes(s_dpy, DefaultRootWindow(s_dpy), &wndattribs); // returns a status but I don't know what success is

  CLog::Log(LOGDEBUG, "Found %d fbconfigs.", num);
  fbConfigIndex = 0;

  if (fbConfigs==NULL) 
  {
    CLog::Log(LOGERROR, "GLX Error: MakePixmap: No compatible framebuffers found");
    XFree(fbConfigs);
    return status;
  }
  CLog::Log(LOGDEBUG, "Using fbconfig index %d.", fbConfigIndex);
  m_Pixmap = XCreatePixmap(s_dpy,
                           DefaultRootWindow(s_dpy),
                           width,
                           height,
                           wndattribs.depth);
  if (!m_Pixmap)
  {
    CLog::Log(LOGERROR, "GLX Error: MakePixmap: Unable to create XPixmap");
    XFree(fbConfigs);
    return status;
  }
  m_glPixmap = glXCreatePixmap(s_dpy, fbConfigs[fbConfigIndex], m_Pixmap, pixmapAttribs);

  if (m_glPixmap)
  {
    CLog::Log(LOGINFO, "GLX: Created Pixmap context");
    visInfo = glXGetVisualFromFBConfig(s_dpy, fbConfigs[fbConfigIndex]);
    if (!visInfo)
    {
      CLog::Log(LOGINFO, "GLX Error: Could not obtain X Visual Info for pixmap");
      return false;
    }
    if (m_pShared)
    {
      CLog::Log(LOGINFO, "GLX: Creating shared Pixmap context");
      m_glContext = glXCreateContext(s_dpy, visInfo, m_pShared->GetContext(), True);
    } 
    else
    {
      CLog::Log(LOGINFO, "GLX: Creating unshared Pixmap context");
      m_glContext = glXCreateContext(s_dpy, visInfo, NULL, True);
    }
    XFree(visInfo);
    if (glXMakeCurrent(s_dpy, m_glPixmap, m_glContext))
    {
      CLog::Log(LOGINFO, "GL: Initialised Pixmap");
      if (!b_glewInit)
      {
        if (glewInit()!=GLEW_OK)
          CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
        else 
        {
          b_glewInit = true;
          if (s_glVendor.length()==0)
          {
            s_glVendor = (const char*)glGetString(GL_VENDOR);
            CLog::Log(LOGINFO, "GL: OpenGL Vendor String: %s", s_glVendor.c_str());
          }
        }
      }

      GLenum glErr;
      if (!m_glPixmapTexture)
      {
        glGenTextures (1, &m_glPixmapTexture);
        glErr = glGetError();
        if ((glErr == GL_INVALID_VALUE) | (glErr == GL_INVALID_OPERATION))
        {
          CLog::Log(LOGINFO, "glGenTextures returned an error!");
          status = false;
        }
      }
    } 
    else
    {
      CLog::Log(LOGINFO, "GLX Error: Could not make Pixmap current");
      status = false;
    }
  }
  else
  {
    CLog::Log(LOGINFO, "GLX Error: Could not create Pixmap");
    status = false;
  }
  XFree(fbConfigs);
  return status;
}
#endif

CSurface::~CSurface()
{
#ifdef HAS_GLX
  if (m_glContext && !IsShared())
  {
    CLog::Log(LOGINFO, "GLX: Destroying OpenGL Context");
    glXDestroyContext(s_dpy, m_glContext);
  }
  if (m_glPBuffer)
  {
    CLog::Log(LOGINFO, "GLX: Destroying PBuffer");
    glXDestroyPbuffer(s_dpy, m_glPBuffer);
  }
  if (m_glPixmap)
  {
    CLog::Log(LOGINFO, "GLX: Destroying glPixmap");
    glXDestroyGLXPixmap(s_dpy, m_glPixmap);
    m_glPixmap = NULL;
  }
  if (m_Pixmap)
  {
    CLog::Log(LOGINFO, "GLX: Destroying XPixmap");
    XFreePixmap(s_dpy, m_Pixmap);
    m_Pixmap = NULL;
  }
  if (m_glWindow && !IsShared())
  {
    CLog::Log(LOGINFO, "GLX: Destroying Window");
    glXDestroyWindow(s_dpy, m_glWindow);
  }
#else
  if (IsValid() && m_SDLSurface
#if defined(__APPLE__ ) || defined(_WIN32)
      && !IsShared()
#endif
     )
  {
    CLog::Log(LOGINFO, "Freeing surface");
    SDL_FreeSurface(m_SDLSurface);
  }

#ifdef __APPLE__
  if (m_glContext && !IsShared())
  {
    CLog::Log(LOGINFO, "Surface: Whacking context 0x%08lx", m_glContext);
    Cocoa_GL_ReleaseContext(m_glContext);
  }
#endif

#ifdef _WIN32
  timeEndPeriod(1);
#endif
#endif
}

void CSurface::EnableVSync(bool enable)
{
#ifdef HAS_SDL_OPENGL
#ifndef __APPLE__
  if (m_bVSync==enable && m_bVsyncInit == true)
    return;
#endif

#ifdef __APPLE__
  if (enable == true && m_bVSync == false)
  {
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
    Cocoa_GL_EnableVSync(true);
  }
  else if (enable == false && m_bVSync == true)
  {
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");
    Cocoa_GL_EnableVSync(false);
  }
  m_bVSync = enable;
  return;
#else
  if (enable)
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");

  // Nvidia cards: See Appendix E. of NVidia Linux Driver Set README
  CStdString strVendor(s_glVendor), strRenderer(s_glRenderer);
  strVendor.ToLower();
  strRenderer.ToLower();

  m_iVSyncMode = 0;
  m_iSwapRate = 0;
  m_bVSync=enable;

#ifdef HAS_GLX
  // Obtain function pointers
  if (!_glXGetSyncValuesOML)
    _glXGetSyncValuesOML = (Bool (*)(Display*, GLXDrawable, int64_t*, int64_t*, int64_t*))glXGetProcAddress((const GLubyte*)"glXGetSyncValuesOML");
  if (!_glXSwapBuffersMscOML)
    _glXSwapBuffersMscOML = (int64_t (*)(Display*, GLXDrawable, int64_t, int64_t, int64_t))glXGetProcAddress((const GLubyte*)"glXSwapBuffersMscOML");
  if (!_glXWaitVideoSyncSGI)
    _glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  if (!_glXGetVideoSyncSGI)
    _glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!_glXSwapIntervalSGI)
    _glXSwapIntervalSGI = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
  if (!_glXSwapIntervalMESA)
    _glXSwapIntervalMESA = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
#elif defined (_WIN32)
  if (!_wglSwapIntervalEXT)
    _wglSwapIntervalEXT = (BOOL (APIENTRY *)(GLint))wglGetProcAddress("wglSwapIntervalEXT");
  if (!_wglGetSwapIntervalEXT )
    _wglGetSwapIntervalEXT = (int (APIENTRY *)())wglGetProcAddress("wglGetSwapIntervalEXT");
#endif

  m_bVsyncInit = true;

#ifdef HAS_GLX
  if (_glXSwapIntervalSGI)
    _glXSwapIntervalSGI(0);
  if (_glXSwapIntervalMESA)
    _glXSwapIntervalMESA(0);
#elif defined (_WIN32)
  if (_wglSwapIntervalEXT)
    _wglSwapIntervalEXT(0);
#endif

  if (IsValid() && enable)
  {
#ifdef HAS_GLX
    // now let's see if we have some system to do specific vsync handling
    if (_glXGetSyncValuesOML && _glXSwapBuffersMscOML && !m_iVSyncMode)
    {
      int64_t ust, msc, sbc;
      if(_glXGetSyncValuesOML(s_dpy, m_glWindow, &ust, &msc, &sbc))
        m_iVSyncMode = 5;
      else
        CLog::Log(LOGWARNING, "%s - glXGetSyncValuesOML failed", __FUNCTION__);
    }
    if (_glXWaitVideoSyncSGI && _glXGetVideoSyncSGI && !m_iVSyncMode && strVendor.find("nvidia") == std::string::npos)
    {
      unsigned int count;
      if(_glXGetVideoSyncSGI(&count) == 0)
          m_iVSyncMode = 3;
      else
        CLog::Log(LOGWARNING, "%s - glXGetVideoSyncSGI failed, glcontext probably not direct", __FUNCTION__);
    }
#endif

#ifdef HAS_GLX
    if (_glXSwapIntervalSGI && !m_iVSyncMode)
    {
      if(_glXSwapIntervalSGI(1) == 0)
        m_iVSyncMode = 2;
      else
        CLog::Log(LOGWARNING, "%s - glXSwapIntervalSGI failed", __FUNCTION__);
    }
    if (_glXSwapIntervalMESA && !m_iVSyncMode)
    {
      if(_glXSwapIntervalMESA(1) == 0)
        m_iVSyncMode = 2;
      else
        CLog::Log(LOGWARNING, "%s - glXSwapIntervalMESA failed", __FUNCTION__);
    }
#elif defined (_WIN32)
    if (_wglSwapIntervalEXT && !m_iVSyncMode)
    {
      if(_wglSwapIntervalEXT(1))
      {
        if(_wglGetSwapIntervalEXT() == 1)
          m_iVSyncMode = 2;
        else
          CLog::Log(LOGWARNING, "%s - wglGetSwapIntervalEXT didn't return the set swap interval", __FUNCTION__);
      }
      else
        CLog::Log(LOGWARNING, "%s - wglSwapIntervalEXT failed", __FUNCTION__);
    }
#endif

    if(g_advancedSettings.m_ForcedSwapTime != 0.0)
    {
      /* some hardware busy wait on swap/glfinish, so we must manually sleep to avoid 100% cpu */
      double rate = g_graphicsContext.GetFPS();
      if (rate <= 0.0 || rate > 1000.0)
      {
        CLog::Log(LOGWARNING, "Unable to determine a valid horizontal refresh rate, vsync workaround disabled %.2g", rate);
        m_iSwapRate = 0;
      }
      else
      {
        __int64 freq;
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        m_iSwapRate   = (__int64)((double)freq / rate);
        m_iSwapTime   = (__int64)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
        m_iSwapStamp  = 0;
        CLog::Log(LOGINFO, "GL: Using artificial vsync sleep with rate %f", rate);
        if(!m_iVSyncMode)
          m_iVSyncMode = 1;
      }
    }

    if(!m_iVSyncMode)
      CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
    else
      CLog::Log(LOGINFO, "GL: Selected vsync mode %d", m_iVSyncMode);
  }
#endif
#endif
}

void CSurface::Flip()
{
  if (m_bOK && m_bDoublebuffer)
  {
#ifdef _WIN32
    int priority;
    DWORD_PTR affinity;
#endif
    if (m_iVSyncMode && m_iSwapRate != 0)
    {
#ifdef HAS_SDL_OPENGL
      glFlush();
#endif
#ifdef _WIN32
      priority = GetThreadPriority(GetCurrentThread());
      affinity = SetThreadAffinityMask(GetCurrentThread(), 1);
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

      __int64 curr, diff, freq;
      QueryPerformanceCounter((LARGE_INTEGER*)&curr);
      QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

      if(m_iSwapStamp == 0)
        m_iSwapStamp = curr;

      /* calculate our next swap timestamp */
      diff = curr - m_iSwapStamp;
      diff = m_iSwapRate - diff % m_iSwapRate;
      m_iSwapStamp = curr + diff;

      /* sleep as close as we can before, assume 1ms precision of sleep *
       * this should always awake so that we are guaranteed the given   *
       * m_iSwapTime to do our swap                                     */
      diff = (diff - m_iSwapTime) * 1000 / freq;
      if(diff > 0)
        Sleep((DWORD)diff);
    }

#ifdef HAS_GLX
    if (m_iVSyncMode == 3)
    {
      glXWaitGL();
      unsigned int before, after;
      if(_glXGetVideoSyncSGI(&before) != 0)
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

      glXSwapBuffers(s_dpy, m_glWindow);

      if(_glXGetVideoSyncSGI(&after) != 0)
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

      if(after == before)
      {
        CLog::Log(LOGINFO, "GL: retrace count didn't change after buffer swap, switching to vsync mode 4");
        m_iVSyncMode = 4;
      }
    }
    else if (m_iVSyncMode == 4)
    {
      glXWaitGL();
      unsigned int vCount;
      if(_glXGetVideoSyncSGI(&vCount) == 0)
        _glXWaitVideoSyncSGI(2, (vCount+1)%2, &vCount);
      else
      {
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);
        EnableVSync(true);
      }
      glXSwapBuffers(s_dpy, m_glWindow);
    }
    else if (m_iVSyncMode == 5)
    {
      int64_t ust, msc, sbc;
      if(_glXGetSyncValuesOML(s_dpy, m_glWindow, &ust, &msc, &sbc))
        _glXSwapBuffersMscOML(s_dpy, m_glWindow, msc, 0, 0);
      else
      {
        CLog::Log(LOGERROR, "%s - glXSwapBuffersMscOML - Failed to get current retrace count", __FUNCTION__);
        EnableVSync(true);
      }
      CLog::Log(LOGINFO, "%s - ust:%"PRId64", msc:%"PRId64", sbc:%"PRId64"", __FUNCTION__, ust, msc, sbc);
    }
    else
      glXSwapBuffers(s_dpy, m_glWindow);
#elif defined(__APPLE__)
    Cocoa_GL_SwapBuffers(m_glContext);
#elif defined(HAS_SDL_OPENGL)
    SDL_GL_SwapBuffers();
#else
    SDL_Flip(m_SDLSurface);
#endif

    if (m_iVSyncMode && m_iSwapRate != 0)
    {
      __int64 curr, diff;
      QueryPerformanceCounter((LARGE_INTEGER*)&curr);

#ifdef _WIN32
      SetThreadPriority(GetCurrentThread(), priority);
      SetThreadAffinityMask(GetCurrentThread(), affinity);
#endif

      diff = curr - m_iSwapStamp;
      m_iSwapStamp = curr;
      if (abs(diff - m_iSwapRate) < abs(diff))
        CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
    }
  }
#ifdef HAS_SDL_OPENGL
  else
  {
    glFlush();
  }
#endif
}

bool CSurface::MakeCurrent()
{
#ifdef HAS_GLX
  if (m_glWindow)
  {
    //attempt up to 10 times
    int i = 0;
    while (i<=10 && !glXMakeCurrent(s_dpy, m_glWindow, m_glContext))
    {
      Sleep(5);
      i++;
    }
    if (i==10)
      return false;
    return true;
    //return (bool)glXMakeCurrent(s_dpy, m_glWindow, m_glContext);
  }
  else if (m_glPBuffer)
  {
    return (bool)glXMakeCurrent(s_dpy, m_glPBuffer, m_glContext);
  }
#endif

#ifdef __APPLE__
  //if (m_glContext)
  if (m_pShared)
  {
    // Use the shared context, because ours might not be up to date (e.g. if
    // a transition from windowed to full-screen took place).
    //
    m_pShared->MakeCurrent();
  }
  else if (m_glContext)
  {
    Cocoa_GL_MakeCurrentContext(m_glContext);
    return true;
  }
#endif

#ifdef _WIN32
  if (IsShared())
    return m_pShared->MakeCurrent();
  if (m_glContext)
  {
    if(wglGetCurrentContext() == m_glContext)
      return true;
    else
      return (wglMakeCurrent(m_glDC, m_glContext) == TRUE);
    }
#endif
  return false;
}

void CSurface::RefreshCurrentContext()
{
#ifdef __APPLE__
  m_glContext = Cocoa_GL_GetCurrentContext();
#endif
}

void CSurface::ReleaseContext()
{
#ifdef HAS_GLX
  {
    CLog::Log(LOGINFO, "GL: ReleaseContext");
    glXMakeCurrent(s_dpy, None, NULL);
  }
#endif
#ifdef __APPLE__
  // Nothing?
#endif
#ifdef _WIN32
  if (IsShared())
    m_pShared->ReleaseContext();
  else if (m_glContext)
    wglMakeCurrent(NULL, NULL);
#endif
}

bool CSurface::ResizeSurface(int newWidth, int newHeight)
{
  CLog::Log(LOGDEBUG, "Asking to resize surface to %d x %d", newWidth, newHeight);
#ifdef HAS_GLX
  if (m_parentWindow)
  {
    XLockDisplay(s_dpy);
    XResizeWindow(s_dpy, m_parentWindow, newWidth, newHeight);
    XResizeWindow(s_dpy, m_glWindow, newWidth, newHeight);
    glXWaitX();
    XUnlockDisplay(s_dpy);
  }
#endif
#ifdef __APPLE__
  m_glContext = Cocoa_GL_ResizeWindow(m_glContext, newWidth, newHeight);
  // If we've resized, we likely lose the vsync settings so get it back.
  if (m_bVSync)
  {
    Cocoa_GL_EnableVSync(m_bVSync);
  }
#endif
#ifdef _WIN32
  SDL_SysWMinfo sysInfo;
  SDL_VERSION(&sysInfo.version);
  if (SDL_GetWMInfo(&sysInfo))
  {
    RECT rBounds;
    HMONITOR hMonitor;
    MONITORINFO mi;
    HWND hwnd = sysInfo.window;

    // Start by getting the current window rect and centering the new window on
    // the monitor that window is on
    GetWindowRect(hwnd, &rBounds);
    hMonitor = MonitorFromRect(&rBounds, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    rBounds.left = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - newWidth) / 2;
    rBounds.top = mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top - newHeight) / 2;
    rBounds.right = rBounds.left + newWidth;
    rBounds.bottom = rBounds.top + newHeight;

    // if this covers the screen area top to bottom, remove the window borders and caption bar
    m_bCoversScreen = (mi.rcMonitor.top + newHeight == mi.rcMonitor.bottom);
    CLog::Log(LOGDEBUG, "New size %s the screen", (m_bCoversScreen ? "covers" : "does not cover"));

    DWORD styleOut, styleIn;
    DWORD swpOptions = SWP_NOCOPYBITS | SWP_SHOWWINDOW;
    HWND hInsertAfter;

    styleIn = styleOut = GetWindowLong(hwnd, GWL_STYLE);
    // We basically want 2 styles, one that is our maximized borderless
    // and one with a caption and non-resizable frame
    if (m_bCoversScreen)
    {
      styleOut = WS_VISIBLE | WS_CLIPSIBLINGS | WS_POPUP;
      LockSetForegroundWindow(LSFW_LOCK);
    }
    else
    {
      styleOut = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
      LockSetForegroundWindow(LSFW_UNLOCK);
    }

    if (IsOnTop())
    {
      hInsertAfter = HWND_TOPMOST;
    }
    else
    {
      hInsertAfter = HWND_NOTOPMOST;
    }

    if (styleIn != styleOut)
    {
      SetWindowLong(hwnd, GWL_STYLE, styleOut);
      // if the style is changing, we are either adding or removing a frame so need to notify
      swpOptions |= SWP_FRAMECHANGED;
    }

    // Now adjust the size of the window so that the client rect is the requested size
    AdjustWindowRectEx(&rBounds, styleOut, false, 0); // there is never a menu

    // finally, move and resize the window
    SetWindowPos(hwnd, hInsertAfter, rBounds.left, rBounds.top,
      rBounds.right - rBounds.left, rBounds.bottom - rBounds.top,
      swpOptions);

    SetForegroundWindow(hwnd);

    SDL_SetWidthHeight(newWidth, newHeight);

    return true;
  }
#endif
  return false;
}


void CSurface::GetGLVersion(int& maj, int& min)
{
#ifdef HAS_SDL_OPENGL
  if (s_glMajVer==0)
  {
    const char* ver = (const char*)glGetString(GL_VERSION);
    if (ver != 0)
      sscanf(ver, "%d.%d", &s_glMajVer, &s_glMinVer);
  }

  if (s_glVendor.length()==0)
  {
    s_glVendor   = (const char*)glGetString(GL_VENDOR);
    s_glRenderer = (const char*)glGetString(GL_RENDERER);
  }
#endif
  maj = s_glMajVer;
  min = s_glMinVer;
}

// function should return the timestamp
// of the frame where a call to flip,
// earliest can land upon.
DWORD CSurface::GetNextSwap()
{
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    __int64 curr, freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&curr);
    DWORD timestamp = timeGetTime();

    curr  -= m_iSwapStamp;
    curr  %= m_iSwapRate;
    curr  -= m_iSwapRate;
    return timestamp - (int)(1000 * curr / freq);
  }
  return timeGetTime();
}

#ifdef _WIN32
void CSurface::SetOnTop(ONTOP onTop)
{
  m_iOnTop = onTop;
}

bool CSurface::IsOnTop() {
  return m_iOnTop == ONTOP_ALWAYS || (m_iOnTop == ONTOP_FULLSCREEN && m_bCoversScreen) ||
        (m_iOnTop == ONTOP_AUTO && m_bCoversScreen && (s_glVendor.find("Intel") != s_glVendor.npos));
}
#endif

void CSurface::NotifyAppFocusChange(bool bGaining)
{
  /* Notification from the Application that we are either becoming the foreground window or are losing focus */
#ifdef _WIN32
  /* Remove our TOPMOST status when we lose focus.  TOPMOST seems to be required for Intel vsync, it isn't
     like I actually want to be on top. Seems like a lot of work. */

  if (m_iOnTop != ONTOP_ALWAYS && IsOnTop())
  {
    CLog::Log(LOGDEBUG, "NotifyAppFocusChange: bGaining=%d, m_bCoverScreen=%d", bGaining, m_bCoversScreen);

    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);

    if (SDL_GetWMInfo(&sysInfo))
    {
      HWND hwnd = sysInfo.window;

      if (bGaining)
      {
        // For intel IGPs Vsync won't start working after we've lost focus and then regained it,
        // to kick-start it we hide then re-show the window; it's gonna look a bit ugly but it
        // seems to be the only option
        if (s_glVendor.find("Intel") != s_glVendor.npos)
        {
          CLog::Log(LOGDEBUG, "NotifyAppFocusChange: hiding XBMC window (to workaround Intel Vsync bug)");
          SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING);
        }
        CLog::Log(LOGDEBUG, "NotifyAppFocusChange: showing XBMC window TOPMOST");
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        SetForegroundWindow(hwnd);
        LockSetForegroundWindow(LSFW_LOCK);
      }
      else
      {
        /*
         * We can't just SetWindowPos(hwnd, hNewFore, ....) because (at least) on Vista focus may be lost to the
         * Alt-Tab window-list which is also TOPMOST, so going just below that in the z-order will leave us still
         * TOPMOST and so probably still above the window you eventually select from the window-list.
         * Checking whether hNewFore is TOPMOST and, if so, making ourselves just NOTOPMOST is almost good enough;
         * we should be NOTOPMOST when you select a window from the window-list and that will be raised above us.
         * However, if the timing is just right (i.e. wrong) then we lose focus to the window-list, see that it's
         * another TOPMOST window, at which time focus leaves the window-list and goes to the selected window
         * and then make ourselves NOTOPMOST and so raise ourselves above it.
         * We could just go straight to the bottom of the z-order but that'll mean we disappear as soon as the
         * window-list appears.
         * So, the best I can come up with is to go NOTOPMOST asap and then if hNewFore is NOTOPMOST go below that,
         * this should minimize the timespan in which the above timing issue can happen.
         */
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        CLog::Log(LOGDEBUG, "NotifyAppFocusChange: making XBMC window NOTOPMOST");

        HWND hNewFore = GetForegroundWindow();
        LONG newStyle = GetWindowLong(hNewFore, GWL_EXSTYLE);
        if (!(newStyle & WS_EX_TOPMOST))
        {
          CLog::Log(LOGDEBUG, "NotifyAppFocusChange: lowering XBMC window beneath new foreground window");
          SetWindowPos(hwnd, hNewFore, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        LockSetForegroundWindow(LSFW_UNLOCK);
      }
    }
  }
#endif
}

