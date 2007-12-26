/*!
\file Surface.cpp
\brief 
*/

#include "include.h"
#include "Surface.h"
#include <string>

using namespace Surface;
#ifdef HAS_SDL_OPENGL
#include <SDL/SDL_syswm.h>
#endif

#ifdef HAS_GLX
Display* CSurface::s_dpy = 0;

static Bool WaitForNotify(Display *dpy, XEvent *event, XPointer arg) 
{
  return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}
Bool    (*_glXGetSyncValuesOML)(Display* dpy, GLXDrawable drawable, int64_t* ust, int64_t* msc, int64_t* sbc);
int64_t (*_glXSwapBuffersMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor,int64_t remainder);

#endif

bool CSurface::b_glewInit = 0;
std::string CSurface::s_glVendor = "";
std::string CSurface::s_glRenderer = "";

#ifdef HAS_SDL_OPENGL
int (*_glXGetVideoSyncSGI)(unsigned int*) = 0;
int (*_glXWaitVideoSyncSGI)(int, int, unsigned int*) = 0;
int (*_glXSwapIntervalSGI)(int) = 0;
int (*_glXSwapIntervalMESA)(int) = 0;
bool (APIENTRY *_wglSwapIntervalEXT)(GLint) = 0;
#endif

#ifdef HAS_SDL
CSurface::CSurface(int width, int height, bool doublebuffer, CSurface* shared,
                   CSurface* window, SDL_Surface* parent, bool fullscreen,
                   bool pixmap, bool pbuffer, int antialias) 
{
  CLog::Log(LOGDEBUG, "Constructing surface");
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
  m_iGLMajVer = 0;
  m_iGLMinVer = 0;

#ifdef HAS_GLX
  m_glWindow = 0;
  m_parentWindow = 0;
  m_glContext = 0;
  m_glPBuffer = 0;
  m_glPixmap = 0;

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
  
  if (window) 
  {
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
    MakePixmap();
    return;
  }
  
  if (doublebuffer) 
  {
    if (antialias)
    {
      fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributesAA, &num);
      if (!fbConfigs)
      {
        CLog::Log(LOGERROR, "GLX Error: No Multisample buffers available, FSAA disabled");
        fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
      }
    }
    else
    {    
      fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
    }
  } 
  else 
  {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);
  }
  if (fbConfigs==NULL) 
  {
    CLog::Log(LOGERROR, "GLX Error: No compatible framebuffers found");
    return;
  }

  vInfo = glXGetVisualFromFBConfig(s_dpy, fbConfigs[0]);

  // if no window is specified, create a window
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
    } else  {
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
    if (!m_glWindow) 
    {
      XFree(fbConfigs);
      CLog::Log(LOGERROR, "GLX Error: Could not create window");
      return;
    }
  }

  // still no window? then we got a problem  
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
  if (mapWindow) 
  {
    XEvent event;
    XMapWindow(s_dpy, m_glWindow);
    XIfEvent(s_dpy, &event, WaitForNotify, (XPointer)m_glWindow);
  }
  if (m_glContext) 
  {
    glXMakeCurrent(s_dpy, m_glWindow, m_glContext);
    if (!b_glewInit)
    {
      if (glewInit()!=GLEW_OK)
      {
        CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
      }
      else
      {
        b_glewInit = true;
        if (s_glVendor.length()==0)
        {
          s_glVendor = (const char*)glGetString(GL_VENDOR);
          s_glRenderer = (const char*)glGetString(GL_RENDERER);
          CLog::Log(LOGINFO, "GL: OpenGL Vendor String: %s", s_glVendor.c_str());
        }
      }
    }
    m_bOK = true;
  }
  else
  {
    CLog::Log(LOGERROR, "GLX Error: Could not create context");
  }
#elif defined(HAS_SDL_OPENGL)
  int options = SDL_OPENGL | (fullscreen?SDL_FULLSCREEN:0);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   m_iRedSize);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, m_iGreenSize);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  m_iBlueSize);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, m_iAlphaSize);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, m_bDoublebuffer?1:0);
  m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 0, options);
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
            if (s_glVendor.length()==0)
            {
              s_glVendor = (const char*)glGetString(GL_VENDOR);
              s_glRenderer = (const char*)glGetString(GL_RENDERER);
              CLog::Log(LOGINFO, "GL: OpenGL Vendor String: %s", s_glVendor.c_str());
            }
    }
  }
#else
  int options = SDL_HWSURFACE | SDL_DOUBLEBUF;
  if (fullscreen) 
  {
    options |= SDL_FULLSCREEN;
  }
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
  } else {
    CLog::Log(LOGINFO, "GLX Error: Could not create PBuffer");
    status = false;
  }
  XFree(fbConfigs);
  return status;
}

bool CSurface::MakePixmap()
{
// FIXME: this needs to be implemented
  return false;
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
  if (m_glWindow && !IsShared()) 
  {
    CLog::Log(LOGINFO, "GLX: Destroying Window");
    glXDestroyWindow(s_dpy, m_glWindow);
  }
#else
  if (IsValid() && m_SDLSurface) 
  {
    CLog::Log(LOGINFO, "Freeing surface");
    SDL_FreeSurface(m_SDLSurface);
  }
#endif
}

void CSurface::EnableVSync(bool enable)
{
#ifdef HAS_SDL_OPENGL
  if (m_bVSync==enable)
    return;

  if (enable)
  {
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");

#ifdef __GNUC__
#warning using vsync on nvidia always true
#endif
    // the following setenv will currently have no effect on rendering. it should be set before screen setup.
    // workaround needed.
#ifdef _LINUX
    if (setenv("__GL_SYNC_TO_VBLANK","1",true) != 0)
      CLog::Log(LOGERROR,"GL: failed to set vsync env variable!");
#endif
  }
  else
  {
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");
#ifdef _LINUX
    if (unsetenv("__GL_SYNC_TO_VBLANK") != 0)
      CLog::Log(LOGERROR,"GL: failed to unset vsync env variable!");
#endif
  }

  // Nvidia cards: See Appendix E. of NVidia Linux Driver Set README
  CStdString strVendor(s_glVendor);
  strVendor.ToLower();
  bool bNVidia = (strVendor.find("nvidia") != std::string::npos);
  if (!bNVidia)
  {
    switch(m_iVSyncMode)
    {
    case 1:
      if (_glXSwapIntervalSGI)
        _glXSwapIntervalSGI(0);
      break;
      
    case 2:
      if (_glXSwapIntervalMESA)
        _glXSwapIntervalMESA(0);
      break;

    case 3:
      if (_wglSwapIntervalEXT)
        _wglSwapIntervalEXT(0);
      break;
    }
  }

  m_iVSyncMode = 0;
  m_bVSync=enable;
  if (bNVidia)
    return;
  if (IsValid() && enable)
  {
#ifdef HAS_GLX
    // Obtain function pointers
    if (!_glXGetSyncValuesOML)
      _glXGetSyncValuesOML = (Bool (*)(Display*, GLXDrawable, int64_t*, int64_t*, int64_t*))glXGetProcAddress((const GLubyte*)"glXGetSyncValuesOML");
    if (!_glXSwapBuffersMscOML)
      _glXSwapBuffersMscOML = (int64_t (*)(Display*, GLXDrawable, int64_t, int64_t, int64_t))glXGetProcAddress((const GLubyte*)"glXSwapBuffersMscOML");
    if (!_glXWaitVideoSyncSGI)
    {
      _glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
    }
    if (!_glXGetVideoSyncSGI)
    {
      _glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
    }
    if (!_glXSwapIntervalSGI)
    {
      _glXSwapIntervalSGI = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
    }
    if (!_glXSwapIntervalMESA)
    {
      _glXSwapIntervalMESA = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
    }      
#elif defined (_WIN32)
    if (!_wglSwapIntervalEXT)
    {
      _wglSwapIntervalEXT = (bool (APIENTRY *)(GLint))wglGetProcAddress("wglSwapIntervalEXT");
    }
#endif
    if (_glXGetSyncValuesOML && _glXSwapBuffersMscOML && !m_iVSyncMode)
    {
      int64_t ust, msc, sbc;
      if(_glXGetSyncValuesOML(s_dpy, m_glWindow, &ust, &msc, &sbc))
        m_iVSyncMode = 5;
      else
        CLog::Log(LOGWARNING, "%s - _glXGetSyncValuesOML failed", __FUNCTION__);
    }
    if (_glXWaitVideoSyncSGI && _glXGetVideoSyncSGI  && !m_iVSyncMode)
    {
      unsigned int count;
      if(_glXGetVideoSyncSGI(&count) == 0)
        m_iVSyncMode = 4;
      else
        CLog::Log(LOGWARNING, "%s - glXGetVideoSyncSGI failed, glcontext probably not direct", __FUNCTION__);
    }
    if (_glXSwapIntervalSGI && !m_iVSyncMode)
    {
      if(_glXSwapIntervalSGI(1) == 0)
        m_iVSyncMode = 1;
      else
        CLog::Log(LOGWARNING, "%s - glXSwapIntervalSGI failed", __FUNCTION__);
    }
    if (_glXSwapIntervalMESA && !m_iVSyncMode)
    {
      if(_glXSwapIntervalMESA(1) == 0)
        m_iVSyncMode = 2;
      else
        CLog::Log(LOGWARNING, "%s - _glXSwapIntervalMESA failed", __FUNCTION__);
    }
    if (_wglSwapIntervalEXT && !m_iVSyncMode)
    {
      if(_wglSwapIntervalEXT(1))
        m_iVSyncMode = 3;
      else
        CLog::Log(LOGWARNING, "%s - _wglSwapIntervalEXT failed", __FUNCTION__);
    }

    if(!m_iVSyncMode)
    {
      m_iVSyncMode = 0;
      m_bVSync = false;
      OutputDebugString("Vertical Blank Syncing unsupported\n");
      CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
    }
    else
      CLog::Log(LOGINFO, "%s - Selected vsync mode %d", __FUNCTION__, m_iVSyncMode);
  }
#endif
}

void CSurface::Flip() 
{
  if (m_bOK && m_bDoublebuffer) 
  {
#ifdef HAS_GLX
    if (m_iVSyncMode == 4)
    {
      glFinish();
      unsigned int vCount;
      if(_glXGetVideoSyncSGI(&vCount) == 0)
        _glXWaitVideoSyncSGI(2, (vCount+1)%2, &vCount);
      else
      {
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Filed to get current retrace count", __FUNCTION__);
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
      CLog::Log(LOGINFO, "%s - ust:%lld, msc:%lld, sbc:%lld", __FUNCTION__, ust, msc, sbc);
    }
    else
      glXSwapBuffers(s_dpy, m_glWindow);
#elif defined(HAS_SDL_OPENGL)
    SDL_GL_SwapBuffers();
#else
    SDL_Flip(m_SDLSurface);
#endif
  } else {
    OutputDebugString("Surface Error: Could not flip surface.");
  }
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
  return false;
}

void CSurface::ReleaseContext()
{
#ifdef HAS_GLX
  {
    CLog::Log(LOGINFO, "GL: ReleaseContext");
    glXMakeCurrent(s_dpy, None, NULL);
  }
#endif
}

bool CSurface::ResizeSurface(int newWidth, int newHeight)
{
#ifdef HAS_GLX
  if (m_parentWindow)
  {
    XResizeWindow(s_dpy, m_parentWindow, newWidth, newHeight);
    XResizeWindow(s_dpy, m_glWindow, newWidth, newHeight);
    glXWaitX();
  }
#endif
  return false;
}

#ifdef HAS_SDL_OPENGL
void CSurface::GetGLVersion(int& maj, int& min)
{
  if (m_iGLMajVer==0)
  {
    const char* ver = (const char*)glGetString(GL_VERSION);
    sscanf(ver, "%d.%d", &m_iGLMajVer, &m_iGLMinVer);
  }
  maj = m_iGLMajVer;
  min = m_iGLMinVer;
}
#endif
