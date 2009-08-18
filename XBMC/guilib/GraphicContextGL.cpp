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

#include "include.h"
#include "GraphicContextGL.h"
#include "GUIFontManager.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "Settings.h"
#include "GUISettings.h"
#include "XBVideoConfig.h"
#include "TextureManager.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/Application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GUIAudioManager.h"

#ifdef HAS_GL

#define D3D_CLEAR_STENCIL 0x0l

#ifdef HAS_GL
#define GLVALIDATE  { CSingleLock locker(*this); ValidateSurface(); }
#endif
//#include "SurfaceGL.h"
#include "SkinInfo.h"
using namespace Surface;
#ifdef HAS_GLX
#include <X11/extensions/Xinerama.h>
#elif defined (__APPLE__)
#include "CocoaInterface.h"
#endif
#ifdef HAS_XRANDR
#include "XRandR.h"
#endif

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif


using namespace std;

extern bool g_fullScreen;

CGraphicContextGL g_graphicsContext;

CGraphicContextGL::CGraphicContextGL(void)
{
  m_maxTextureSize = 2048;
}

CGraphicContextGL::~CGraphicContextGL(void)
{
  
}

void CGraphicContextGL::GetRenderVersion(int& maj, int& min)
{
  // elis
  return; 

  if (s_RenderMajVer==0)
  {
    const char* ver = (const char*)glGetString(GL_VERSION);
    if (ver != 0)
      sscanf(ver, "%d.%d", &s_RenderMajVer, &s_RenderMinVer);
  }

  if (s_RenderVendor.length()==0)
  {
    s_RenderVendor   = (const char*)glGetString(GL_VENDOR);
    s_RenderRenderer = (const char*)glGetString(GL_RENDERER);
  }
#ifdef HAS_GLX
  if (s_RenderxExt.length()==0)
  {
    s_RenderxExt  = " ";
    s_RenderxExt += (const char*)glXQueryExtensionsString(s_dpy, DefaultScreen(s_dpy));
    s_RenderxExt += " ";
  }
#endif
  maj = s_RenderMajVer;
  min = s_RenderMinVer;
}

CRect CGraphicContextGL::GetRenderViewPort()
{
  GLVALIDATE;
  GLint viewport[4];
  glGetIntegerv(GL_SCISSOR_BOX, viewport);

  CRect rect;
  rect.x1 = (int)viewport[0];
  rect.y2 = m_iScreenHeight - viewport[1];       // opengl uses bottomleft as origin
  rect.y1 = rect.y2 - viewport[3];
  rect.x2 = (int)viewport[0] + viewport[2];

  return rect;
}

void CGraphicContextGL::SetRendrViewPort(CRect& viewPort)
{
  GLint glviewport[4];

  glviewport[0] = viewPort.x1;
  glviewport[1] = m_iScreenHeight - viewPort.y2; // opengl uses bottomleft as origin
  glviewport[2] = viewPort.x2 - viewPort.x1;
  glviewport[3] = viewPort.y2 - viewPort.y1;
  glScissor(glviewport[0], glviewport[1], glviewport[2], glviewport[3]);
  glViewport(glviewport[0], glviewport[1], glviewport[2], glviewport[3]);
  VerifyGLState();
}

/*
void CGraphicContextGL::UpdateRenderingScreenResolution(RESOLUTION& res, RESOLUTION& lastRes)
{
  int options = SDL_RESIZABLE;
  if (g_advancedSettings.m_fullScreen) options |= SDL_FULLSCREEN;

  // Create a bare root window so that SDL Input handling works
#ifdef HAS_GLX
  static SDL_Surface* rootWindow = NULL;
  if (!rootWindow)
  {
#ifdef HAS_XRANDR
    XOutput out;
    XMode mode;
    out.name = g_settings.m_ResInfo[res].strOutput;
    mode.w = g_settings.m_ResInfo[res].iWidth;
    mode.h = g_settings.m_ResInfo[res].iHeight;
    mode.hz = g_settings.m_ResInfo[res].fRefreshRate;
    g_audioManager.Stop();
    g_xrandr.SetMode(out, mode);
    g_renderManager.Recover();
    SDL_ShowCursor(SDL_ENABLE);
#endif

    rootWindow = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0,  options);
    // attach a GLX surface to the root window
    m_screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, 0, 0, rootWindow, (bool)g_advancedSettings.m_fullScreen);
    if (g_videoConfig.GetVSyncMode()==VSYNC_ALWAYS)
      m_screenSurface->EnableVSync();

    if (g_advancedSettings.m_fullScreen)
    {
      SetFullScreenRoot(true);
    }
  }
  else
  {
    if (!g_advancedSettings.m_fullScreen)
    {
      rootWindow = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0,  options);
      m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
    }
    else
    {
      SetFullScreenRoot(true);
    }
  }

#elif defined(__APPLE__) || defined(_WIN32PC)
  // Allow for fullscreen.
  bool needsResize = (m_screenSurface != 0);
#if defined(_WIN32PC)
  // Always resize even the first time because we need to change the attributes on the SDL window and center
  needsResize = true;
#endif
  if (!m_screenSurface)
    m_screenSurface = new CSurfaceGL(m_iScreenWidth, m_iScreenHeight, true, 0, 0, 0, g_advancedSettings.m_fullScreen);

  if (g_advancedSettings.m_fullScreen)
  {
    // SetFullScreenRoot will call m_screenSurface->ResizeSurface
    needsResize = false;
    SetFullScreenRoot(true);
  }
#ifndef _WIN32PC
  else if (lastRes>=DESKTOP )
#else
  else if (m_bFullScreenRoot)
#endif
  {
    // SetFullScreenRoot will call m_screenSurface->ResizeSurface
    needsResize = false;
    SetFullScreenRoot(false);
  }

  if (needsResize)
    m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
#endif

#if defined(_WIN32PC) && !defined(__APPLE__)
  if (!g_guiSettings.GetBool("videoplayer.adjustrefreshrate"))
  {
    //get the display frequency
    DEVMODE devmode;
    ZeroMemory(&devmode, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    if(devmode.dmDisplayFrequency == 59 || devmode.dmDisplayFrequency == 29 || devmode.dmDisplayFrequency == 23)
      g_settings.m_ResInfo[res].fRefreshRate = (float)(devmode.dmDisplayFrequency + 1) / 1.001f;
    else
      g_settings.m_ResInfo[res].fRefreshRate = (float)(devmode.dmDisplayFrequency);
  }
  else
    if(g_settings.m_ResInfo[res].iSubtitles > g_settings.m_ResInfo[res].iHeight)
      g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * g_settings.m_ResInfo[res].iHeight);
#endif

  SDL_WM_SetCaption("XBMC Media Center", NULL);

  {
    CSingleLock aLock(m_surfaceLock);
    m_surfaces[SDL_ThreadID()] = m_screenSurface;
  }

  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

  {
    GLint width = 256;
    glGetError(); // reset any previous GL errors

    // max out at 2^(8+8)
    for (int i = 0 ; i<8 ; i++)
    {
      glTexImage2D(GL_PROXY_TEXTURE_2D, 0, 4, width, width, 0, GL_BGRA,
        GL_UNSIGNED_BYTE, NULL);
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
        &width);

      // GMA950 on OS X sets error instead
      if (width==0 || (glGetError()!=GL_NO_ERROR) )
        break;
      m_maxTextureSize = width;
      width *= 2;
      if (width > 65536) // have an upper limit in case driver acts stupid
      {
        CLog::Log(LOGERROR, "GL: Could not determine maximum texture width, falling back to 2048");
        m_maxTextureSize = 2048;
        break;
      }
    }
    CLog::Log(LOGINFO, "GL: Maximum texture width: %d", m_maxTextureSize);
  }

  glViewport(0, 0, m_iScreenWidth, m_iScreenHeight);
  glScissor(0, 0, m_iScreenWidth, m_iScreenHeight);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0.0f, m_iScreenWidth-1, m_iScreenHeight-1, 0.0f, -1.0f, 1.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);
  VerifyGLState();
}
*/

void CGraphicContextGL::Clear()
{
  GLVALIDATE;
  glClear(GL_COLOR_BUFFER_BIT);
}

void CGraphicContextGL::CaptureStateBlock()
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  if (glActiveTextureARB)
    glActiveTextureARB(GL_TEXTURE0_ARB);
  glDisable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glColor3f(1.0, 1.0, 1.0);
}

void CGraphicContextGL::ApplyStateBlock()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  if (glActiveTextureARB)
    glActiveTextureARB(GL_TEXTURE0_ARB);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);
}

void CGraphicContextGL::UpdateCameraPosition(const CPoint &camera)
{
  // NOTE: This routine is currently called (twice) every time there is a <camera>
  //       tag in the skin.  It actually only has to be called before we render
  //       something, so another option is to just save the camera coordinates
  //       and then have a routine called before every draw that checks whether
  //       the camera has changed, and if so, changes it.  Similarly, it could set
  //       the world transform at that point as well (or even combine world + view
  //       to cut down on one setting)

  // and calculate the offset from the screen center
  CPoint offset = camera - CPoint(m_iScreenWidth*0.5f, m_iScreenHeight*0.5f);


  // grab the viewport dimensions and location
  GLint viewport[4];
  BeginPaint();
  glGetIntegerv(GL_VIEWPORT, viewport);

  float w = (float)viewport[2]*0.5f;
  float h = (float)viewport[3]*0.5f;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-(viewport[0] + w + offset.x), +(viewport[1] + h + offset.y), 0);
  gluLookAt(0.0, 0.0, -2.0*h, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  glMatrixMode(GL_MODELVIEW);
  EndPaint();
}

bool CGraphicContextGL::ValidateSurface(CSurface* dest)
{
  /* elis
  CSingleLock aLock(m_surfaceLock);
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  iter = m_surfaces.find(tid);
  if (iter==m_surfaces.end()) {
#if defined(HAS_GLX) || defined(__APPLE__) || defined(_WIN32PC)
    if (dest==NULL)
    {
      CLog::Log(LOGDEBUG, "GL: Sharing screen surface for thread %u", tid);
      CSurfaceGL* surface = new CSurfaceGL(m_screenSurface);
      if (!surface->MakeCurrent())
      {
        CLog::Log(LOGERROR, "GL: Error making context current");
        delete surface;
        return false;
      }
      m_surfaces[tid] = surface;
      return true;
    }
    else
    {
      m_surfaces[tid] = dest;
      dest->MakeCurrent();
    }
#else
    CLog::Log(LOGDEBUG, "Creating surface for thread %ul", tid);
    CSurface* surface = InitializeSurface();
    if (surface)
    {
      m_surfaces[tid] = surface;
      return true;
    } else {
      CLog::Log(LOGERROR, "Did not get surface for thread %ul", tid);
      return false;
    }
#endif
  } else {
    (iter->second)->MakeCurrent();
  }
  */
  return true;
}

CSurface* CGraphicContextGL::InitializeSurface()
{
  /* elis
  CSurfaceGL* screenSurface = NULL;
  Lock();

  screenSurface = new CSurfaceGL(m_iScreenWidth, m_iScreenHeight, true, m_screenSurface, m_screenSurface);
  if (!screenSurface || !screenSurface->IsValid())
  {
    CLog::Log(LOGERROR, "Surface creation error");
    delete screenSurface;
    Unlock();
    return NULL;
  }
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

  glViewport(0, 0, m_iScreenWidth, m_iScreenHeight);
  glScissor(0, 0, m_iScreenWidth, m_iScreenHeight);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0.0f, m_iScreenWidth-1, m_iScreenHeight-1, 0.0f, -1.0f, 1.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);

  Unlock();
  return screenSurface;
  */
  return NULL;
}

void CGraphicContextGL::ReleaseCurrentContext(Surface::CSurface* ctx)
{
  /* elis
  if (ctx)
  {
    Lock();
    ctx->ReleaseContext();
    Unlock();
    return;
  }
  Lock();
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  CSingleLock aLock(m_surfaceLock);
  iter = m_surfaces.find(tid);
  if (iter==m_surfaces.end())
  {
    m_screenSurface->ReleaseContext();
    Unlock();
    return;
  }
  (iter->second)->ReleaseContext();
  Unlock();
  */
}

void CGraphicContextGL::DeleteThreadContext() {
  /* elis
  CSingleLock aLock(m_surfaceLock);
  // FIXME?: DeleteThreadContext get called from different threads and
  // produces an acces_violation from time to time when doing the find
  // on an empty m_surfaces
  if(m_surfaces.empty())
    return;
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  iter = m_surfaces.find(tid);
  if (iter!=m_surfaces.end())
    m_surfaces.erase(iter);
    */
}

void CGraphicContextGL::AcquireCurrentContext(Surface::CSurface* ctx)
{
  /* eli
  if (ctx)
  {
    Lock();
    if (!ctx->MakeCurrent())
    {
      CLog::Log(LOGERROR, "Error making context current");
    }
    Unlock();
    return;
  }
  Lock();
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  CSingleLock aLock(m_surfaceLock);
  iter = m_surfaces.find(tid);
  if (iter==m_surfaces.end())
  {
    Unlock();
    return;
  }
  if (!(iter->second)->MakeCurrent())
  {
    CLog::Log(LOGERROR, "Error making context current");
  }
  Unlock();
  */
}

void CGraphicContextGL::BeginPaint(CSurface *dest, bool lock)
{
  if (lock) Lock();
  ValidateSurface(dest);
  VerifyGLState();
}

void CGraphicContextGL::EndPaint(CSurface *dest, bool lock)
{
  if (lock) Unlock();
  VerifyGLState();
}

void CGraphicContextGL::SetVideoResolution(RESOLUTION &res, BOOL NeedZ, bool forceClear /* = false */)
{
  RESOLUTION lastRes = m_Resolution;
  if (res == AUTORES)
  {
    res = g_videoConfig.GetBestMode();
  }

  if (!IsValidResolution(res))
  {
    // Choose a failsafe resolution that we can actually display
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    res = g_videoConfig.GetSafeMode();
  }
#ifndef _WIN32PC
  // FIXME: see if #5256 works also for Linux and Mac
  if (res>=DESKTOP || g_advancedSettings.m_startFullScreen)
  {
    g_advancedSettings.m_fullScreen = true;
    m_bFullScreenRoot = true;
    if (res!=m_Resolution)
    {
      if (m_Resolution != INVALID)
        g_settings.m_ResInfo[WINDOW] = g_settings.m_ResInfo[m_Resolution];
    }
  }
  else
  {
    g_advancedSettings.m_fullScreen = false;
    m_bFullScreenRoot = false;
#ifdef HAS_XRANDR
    g_audioManager.Stop();
    g_xrandr.RestoreState();
#endif
  }

  if (res==WINDOW)
  {
    g_advancedSettings.m_fullScreen = false;
    m_bFullScreenRoot = false;
  }

  if (res==WINDOW || (m_Resolution != res))
#else
  if ((m_Resolution != res) || (m_bFullScreenRoot != g_advancedSettings.m_fullScreen))
#endif
  {
#if defined(__APPLE__)
    // In going FullScreen, m_Resolution == DESKTOP but if using multiple displays
    // the display resolution will be wrong if the windowed display is moved to
    // a display with a different resolution. So we have to resort to
    // Hack, hack, hack. The basic problem is the resolution is not linked to the
    // display so we have to find which display we are going fs on, then search
    // through the m_ResInfo resolutions to find a matching "Full Screen"
    // descriptor, then use that index to setup m_Resolution as there are multiple
    // "DESKTOP" with multiple displays. If the strMode descriptor changes, this
    // will break but the resolution really need to be linked to a display index.
    if (m_bFullScreenRoot)
    {
      // going to fullscreen desktop but which display if multiple displays?
      // need to find the m_ResInfo index for that display.
      int screen_index = Cocoa_GetScreenIndex();
      char        test_string[256];

      if (screen_index == 1)
      {
        strcpy(test_string, "(Full Screen)");
      }
      else
      {
        sprintf(test_string, "(Full Screen #%d)", screen_index);
      }
      for (int i = (int)DESKTOP ; i< (CUSTOM+g_videoConfig.GetNumberOfResolutions()) ; i++)
      {
        if (strstr(g_settings.m_ResInfo[i].strMode, test_string) != 0)
        {
          res = (RESOLUTION)i;
          break;
        }
      }
    }
#endif
    Lock();
    m_iScreenWidth  = g_settings.m_ResInfo[res].iWidth;
    m_iScreenHeight = g_settings.m_ResInfo[res].iHeight;
    m_Resolution    = res;

#ifdef HAS_SDL_2D
    int options = SDL_HWSURFACE | SDL_DOUBLEBUF;
    if (g_advancedSettings.m_fullScreen) options |= SDL_FULLSCREEN;
    m_screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, 0, 0, 0, (bool)g_advancedSettings.m_fullScreen);
#elif defined(HAS_GL)
    /* elis
    int options = SDL_RESIZABLE;
    if (g_advancedSettings.m_fullScreen) options |= SDL_FULLSCREEN;
    */

    // Create a bare root window so that SDL Input handling works
#ifdef HAS_GLX
    static SDL_Surface* rootWindow = NULL;
    if (!rootWindow)
    {
#ifdef HAS_XRANDR
      XOutput out;
      XMode mode;
      out.name = g_settings.m_ResInfo[res].strOutput;
      mode.w = g_settings.m_ResInfo[res].iWidth;
      mode.h = g_settings.m_ResInfo[res].iHeight;
      mode.hz = g_settings.m_ResInfo[res].fRefreshRate;
      g_audioManager.Stop();
      g_xrandr.SetMode(out, mode);
      g_renderManager.Recover();
      SDL_ShowCursor(SDL_ENABLE);
#endif

      rootWindow = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0,  options);
      // attach a GLX surface to the root window
      m_screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, 0, 0, rootWindow, (bool)g_advancedSettings.m_fullScreen);
      if (g_videoConfig.GetVSyncMode()==VSYNC_ALWAYS)
        m_screenSurface->EnableVSync();

      if (g_advancedSettings.m_fullScreen)
      {
        SetFullScreenRoot(true);
      }
    }
    else
    {
      if (!g_advancedSettings.m_fullScreen)
      {
        rootWindow = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0,  options);
        m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
      }
      else
      {
        SetFullScreenRoot(true);
      }
    }

#elif defined(__APPLE__) || defined(_WIN32PC)
    // Allow for fullscreen.
    // elis bool needsResize = (m_screenSurface != 0);
#if defined(_WIN32PC)
    // Always resize even the first time because we need to change the attributes on the SDL window and center
   // elis  needsResize = true;
#endif
    /* elis
    if (!m_screenSurface)
      m_screenSurface = new CSurfaceGL(m_iScreenWidth, m_iScreenHeight, true, 0, 0, 0, g_advancedSettings.m_fullScreen);
      */

    if (g_advancedSettings.m_fullScreen)
    {
      // SetFullScreenRoot will call m_screenSurface->ResizeSurface
      // elis needsResize = false;
      SetFullScreenRoot(true);
    }
#ifndef _WIN32PC
    else if (lastRes>=DESKTOP )
#else
    else if (m_bFullScreenRoot)
#endif
    {
      // SetFullScreenRoot will call m_screenSurface->ResizeSurface
      // elis needsResize = false;
      SetFullScreenRoot(false);
    }
    /*
    if (needsResize)
      m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
      */
#endif

#if defined(_WIN32PC) && !defined(__APPLE__)
    if (!g_guiSettings.GetBool("videoplayer.adjustrefreshrate"))
    {
      //get the display frequency
      DEVMODE devmode;
      ZeroMemory(&devmode, sizeof(devmode));
      devmode.dmSize = sizeof(devmode);
      EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
      if(devmode.dmDisplayFrequency == 59 || devmode.dmDisplayFrequency == 29 || devmode.dmDisplayFrequency == 23)
        g_settings.m_ResInfo[res].fRefreshRate = (float)(devmode.dmDisplayFrequency + 1) / 1.001f;
      else
        g_settings.m_ResInfo[res].fRefreshRate = (float)(devmode.dmDisplayFrequency);
    }
    else
      if(g_settings.m_ResInfo[res].iSubtitles > g_settings.m_ResInfo[res].iHeight)
        g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * g_settings.m_ResInfo[res].iHeight);
#endif

    // elis SDL_WM_SetCaption("XBMC Media Center", NULL);

    {
      /* elis
      CSingleLock aLock(m_surfaceLock);
      m_surfaces[SDL_ThreadID()] = m_screenSurface;
      */
    }

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    {
      GLint width = 256;
      glGetError(); // reset any previous GL errors

      // max out at 2^(8+8)
      for (int i = 0 ; i<8 ; i++)
      {
        glTexImage2D(GL_PROXY_TEXTURE_2D, 0, 4, width, width, 0, GL_BGRA,
          GL_UNSIGNED_BYTE, NULL);
        glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
          &width);

        // GMA950 on OS X sets error instead
        if (width==0 || (glGetError()!=GL_NO_ERROR) )
          break;
        m_maxTextureSize = width;
        width *= 2;
        if (width > 65536) // have an upper limit in case driver acts stupid
        {
          CLog::Log(LOGERROR, "GL: Could not determine maximum texture width, falling back to 2048");
          m_maxTextureSize = 2048;
          break;
        }
      }
      CLog::Log(LOGINFO, "GL: Maximum texture width: %d", m_maxTextureSize);
    }

    glViewport(0, 0, m_iScreenWidth, m_iScreenHeight);
    glScissor(0, 0, m_iScreenWidth, m_iScreenHeight);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_SCISSOR_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, m_iScreenWidth-1, m_iScreenHeight-1, 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);          // Turn Blending On
    glDisable(GL_DEPTH_TEST);
    VerifyGLState();
#endif
    m_bWidescreen = (res == HDTV_1080i || res == HDTV_720p || res == PAL60_16x9 ||
      res == PAL_16x9 || res == NTSC_16x9);

    // set the mouse resolution
    if ((lastRes == -1) || (g_settings.m_ResInfo[lastRes].iWidth != g_settings.m_ResInfo[res].iWidth) || (g_settings.m_ResInfo[lastRes].iHeight != g_settings.m_ResInfo[res].iHeight))
    {
      g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
      g_fontManager.ReloadTTFFonts();
    }

    // restore vsync mode
    g_videoConfig.SetVSyncMode((VSYNC)g_guiSettings.GetInt("videoscreen.vsync"));

    SetFullScreenViewWindow(res);

    m_Resolution = res;
    Unlock();
  }
}



void CGraphicContextGL::SetFullScreenRoot(bool fs)
{
#ifdef __APPLE__
  int blanking = g_guiSettings.GetInt("videoscreen.displayblanking");
  bool blankOtherDisplays = (blanking == BLANKING_ALL_DISPLAYS);
#endif

  if (fs)
  {
    // Code from this point on should be platform dependent. The Win32 version could
    // probably use GetSystemMetrics/EnumDisplayDevices/GetDeviceCaps to query current
    // resolution on the requested display no. and set 'width' and 'height'

    m_iFullScreenWidth = m_iScreenWidth;
    m_iFullScreenHeight = m_iScreenHeight;
#ifdef HAS_XRANDR
    XOutput out;
    XMode mode;
    RESOLUTION res = m_Resolution;
    out.name = g_settings.m_ResInfo[res].strOutput;
    mode.w = g_settings.m_ResInfo[res].iWidth;
    mode.h = g_settings.m_ResInfo[res].iHeight;
    mode.hz = g_settings.m_ResInfo[res].fRefreshRate;
    mode.id = g_settings.m_ResInfo[res].strId;
    g_audioManager.Stop();
    g_xrandr.SetMode(out, mode);
    SDL_ShowCursor(SDL_ENABLE);
#endif
#if defined(__APPLE__)
    Cocoa_GL_SetFullScreen(m_iFullScreenWidth, m_iFullScreenHeight, true, blankOtherDisplays,
      g_advancedSettings.m_osx_GLFullScreen, g_advancedSettings.m_alwaysOnTop);
#elif defined(_WIN32PC)
    DEVMODE settings;
    settings.dmSize = sizeof(settings);
    settings.dmDriverExtra = 0;
    settings.dmBitsPerPel = 32;
    settings.dmPelsWidth = m_iFullScreenWidth;
    settings.dmPelsHeight = m_iFullScreenHeight;
    settings.dmDisplayFrequency = (int)floorf(g_settings.m_ResInfo[m_Resolution].fRefreshRate);
    settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    if(settings.dmDisplayFrequency)
      settings.dmFields |= DM_DISPLAYFREQUENCY;
    if(ChangeDisplaySettings(&settings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
      CLog::Log(LOGERROR, "CGraphicContext::SetFullScreenRoot - failed to change resolution");
#else
    SDL_SetVideoMode(m_iFullScreenWidth, m_iFullScreenHeight, 0, SDL_FULLSCREEN);
#endif
    /* elis
    if (m_screenSurface)
    {
      m_screenSurface->RefreshCurrentContext();
      m_screenSurface->ResizeSurface(m_iFullScreenWidth, m_iFullScreenHeight);
    }
    */

    glViewport(0, 0, m_iFullScreenWidth, m_iFullScreenHeight);
    glScissor(0, 0, m_iFullScreenWidth, m_iFullScreenHeight);

    g_fontManager.ReloadTTFFonts();
    g_Mouse.SetResolution(m_iFullScreenWidth, m_iFullScreenHeight, 1, 1);
    g_renderManager.Recover();
  }
  else
  {
#ifdef __APPLE__
    Cocoa_GL_SetFullScreen(
      g_settings.m_ResInfo[m_Resolution].iWidth,
      g_settings.m_ResInfo[m_Resolution].iHeight,
      false, blankOtherDisplays, g_advancedSettings.m_osx_GLFullScreen, g_advancedSettings.m_alwaysOnTop);
#elif defined(_WIN32PC)
    ChangeDisplaySettings(NULL, 0);
#else
    SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0, SDL_RESIZABLE);
#endif
    /* elis
    if (m_screenSurface)
    {
      m_screenSurface->RefreshCurrentContext();
      m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
    }
    */

    glViewport(0, 0, m_iScreenWidth, m_iScreenHeight);
    glScissor(0, 0, m_iScreenWidth, m_iScreenHeight);

    g_fontManager.ReloadTTFFonts();
    g_Mouse.SetResolution(g_settings.m_ResInfo[m_Resolution].iWidth, g_settings.m_ResInfo[m_Resolution].iHeight, 1, 1);
    g_renderManager.Recover();
  }

  m_bFullScreenRoot = fs;
  g_advancedSettings.m_fullScreen = fs;
  SetFullScreenViewWindow(m_Resolution);
}


void CGraphicContextGL::Flip()
{
  // elis m_screenSurface->Flip();
}

void CGraphicContextGL::ApplyHardwareTransform()
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  GLfloat matrix[4][4];

  for(int i=0;i<3;i++)
    for(int j=0;j<4;j++)
      matrix[j][i] = m_finalTransform.m[i][j];

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;

  glMultMatrixf(&matrix[0][0]);
}

void CGraphicContextGL::RestoreHardwareTransform()
{
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

#endif