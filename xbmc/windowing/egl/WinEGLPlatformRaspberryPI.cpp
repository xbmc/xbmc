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

#include "system.h"

#if defined(TARGET_RASPBERRY_PI)

#include "system_gl.h"

#ifdef HAS_EGL

#include "WinEGLPlatformRaspberryPI.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "xbmc/cores/VideoRenderers/RenderManager.h"
#include "settings/Settings.h"

#include <string>

#ifndef __VIDEOCORE4__
#define __VIDEOCORE4__
#endif

#define __VCCOREVER__ 0x04000000

#define IS_WIDESCREEN(m) (m==3||m==7||m==9||m==11||m==13||m==15||m==18||m==22||m==24||m==26||m==28||m==30||m==36||m==38||m==43||m==45||m==49||m==51||m==53||m==55||m==57||m==59)

#define MAKEFLAGS(group, mode, interlace, mode3d) (((mode)<<24)|((group)<<16)|((interlace)!=0?D3DPRESENTFLAG_INTERLACED:D3DPRESENTFLAG_PROGRESSIVE)| \
   (((group)==HDMI_RES_GROUP_CEA && IS_WIDESCREEN(mode))?D3DPRESENTFLAG_WIDESCREEN:0)|((mode3d)!=0?D3DPRESENTFLAG_MODE3DSBS:0))
#define GETFLAGS_INTERLACE(f) (((f)&D3DPRESENTFLAG_INTERLACED)!=0)
#define GETFLAGS_WIDESCREEN(f) (((f)&D3DPRESENTFLAG_WIDESCREEN)!=0)
#define GETFLAGS_GROUP(f) ((HDMI_RES_GROUP_T)(((f)>>16)&0xff))
#define GETFLAGS_MODE(f) (((f)>>24)&0xff)
#define GETFLAGS_MODE3D(f) (((f)&D3DPRESENTFLAG_MODE3DSBS)!=0)

CWinEGLPlatformRaspberryPI::CWinEGLPlatformRaspberryPI()
{
  m_surface = EGL_NO_SURFACE;
  m_context = EGL_NO_CONTEXT;
  m_display = EGL_NO_DISPLAY;

  m_nativeWindow = NULL;

  m_desktopRes.iScreen = 0;
  m_desktopRes.iWidth  = 1280;
  m_desktopRes.iHeight = 720;
  //m_desktopRes.iScreenWidth  = 1280;
  //m_desktopRes.iScreenHeight = 720;
  m_desktopRes.fRefreshRate = 60.0f;
  m_desktopRes.bFullScreen = true;
  m_desktopRes.iSubtitles = (int)(0.965 * 720);
  m_desktopRes.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
  m_desktopRes.fPixelRatio = 1.0f;
  m_desktopRes.strMode = "720p 16:9";
  m_sdMode = false;
}

CWinEGLPlatformRaspberryPI::~CWinEGLPlatformRaspberryPI()
{
  DestroyWindow();
}

EGLNativeWindowType CWinEGLPlatformRaspberryPI::InitWindowSystem(EGLNativeDisplayType nativeDisplay, int width, int height, int bpp)
{
  m_nativeDisplay = nativeDisplay;
  m_width = width;
  m_height = height;

  m_dispman_element = DISPMANX_NO_HANDLE;
  m_dispman_element2 = DISPMANX_NO_HANDLE;
  m_dispman_display = DISPMANX_NO_HANDLE;

  if (!m_DllBcmHost.Load())
    return NULL;

  memset(&m_tv_state, 0, sizeof(TV_GET_STATE_RESP_T));
  m_DllBcmHost.vc_tv_get_state(&m_tv_state);

  m_nativeWindow  = (EGL_DISPMANX_WINDOW_T*)calloc(1, sizeof(EGL_DISPMANX_WINDOW_T));

  return (EGLNativeWindowType)m_nativeWindow;
}

bool CWinEGLPlatformRaspberryPI::SetDisplayResolution(RESOLUTION_INFO& res)
{
  EGL_DISPMANX_WINDOW_T *nativeWindow = (EGL_DISPMANX_WINDOW_T *)m_nativeWindow;

  DestroyWindow();

  bool bFound = false;

  RESOLUTION_INFO resSearch;

  int best_score = 0;

  for (size_t i = 0; i < m_res.size(); i++)
  {
    if(m_res[i].iWidth == res.iWidth && m_res[i].iHeight == res.iHeight && m_res[i].fRefreshRate == res.fRefreshRate)
    {
      int score = 0;

      /* prefere progressive over interlaced */
      if(!GETFLAGS_INTERLACE(m_res[i].dwFlags))
        score = 1;

      if(score >= best_score)
      {
        resSearch = m_res[i];
        bFound = true;
      }
    }
  }

  if(bFound && !m_sdMode)
  {
    sem_init(&m_tv_synced, 0, 0);
    m_DllBcmHost.vc_tv_register_callback(CallbackTvServiceCallback, this);
  
    int success = m_DllBcmHost.vc_tv_hdmi_power_on_explicit(HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), GETFLAGS_MODE(resSearch.dwFlags));

    if (success == 0) 
    {
      CLog::Log(LOGNOTICE, "CWinEGLPlatformRaspberryPI::SetDisplayResolution set HDMI mode (%d,%d,%d)=%d\n", 
                          GETFLAGS_MODE3D(resSearch.dwFlags) ? HDMI_MODE_3D:HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), 
                          GETFLAGS_MODE(resSearch.dwFlags), success);
      sem_wait(&m_tv_synced);
    } 
    else 
    {
      CLog::Log(LOGERROR, "CWinEGLPlatformRaspberryPI::SetDisplayResolution failed to set HDMI mode (%d,%d,%d)=%d\n", 
                          GETFLAGS_MODE3D(resSearch.dwFlags) ? HDMI_MODE_3D:HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), 
                          GETFLAGS_MODE(resSearch.dwFlags), success);
    }
    m_DllBcmHost.vc_tv_unregister_callback(CallbackTvServiceCallback);
    sem_destroy(&m_tv_synced);
  }

  m_dispman_display = m_DllBcmHost.vc_dispmanx_display_open(0);

  m_width   = res.iWidth;
  m_height  = res.iHeight;
  //m_bFullScreen = fullScreen;
  m_fb_width  = res.iWidth;
  m_fb_height = res.iHeight;

  m_fb_bpp    = 8;

  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = res.iWidth;
  dst_rect.height = res.iHeight;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = m_fb_width << 16;
  src_rect.height = m_fb_height << 16;

  VC_DISPMANX_ALPHA_T alpha;
  memset(&alpha, 0x0, sizeof(VC_DISPMANX_ALPHA_T));
  alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE;

  DISPMANX_CLAMP_T clamp;
  memset(&clamp, 0x0, sizeof(DISPMANX_CLAMP_T));

  DISPMANX_TRANSFORM_T transform = DISPMANX_NO_ROTATE;
  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost.vc_dispmanx_update_start(0);
  CLog::Log(LOGDEBUG, "CWinEGLPlatformRaspberryPI::SetDisplayResolution %dx%d->%dx%d\n", m_fb_width, m_fb_height, dst_rect.width, dst_rect.height);

  // width < height => half SBS
  if (src_rect.width < src_rect.height)
  {
    // right side
    /*
    dst_rect.x = m_width;
    dst_rect.width = m_width;
    */
    dst_rect.x = res.iWidth;
    dst_rect.width >>= dst_rect.width - dst_rect.x;
    m_dispman_element2 = m_DllBcmHost.vc_dispmanx_element_add(dispman_update,
      m_dispman_display,
      1,                              // layer
      &dst_rect,
      (DISPMANX_RESOURCE_HANDLE_T)0,  // src
      &src_rect,
      DISPMANX_PROTECTION_NONE,
      //(VC_DISPMANX_ALPHA_T*)0,        // alpha
      &alpha,
      //(DISPMANX_CLAMP_T*)0,           // clamp
      &clamp,
      //(DISPMANX_TRANSFORM_T)0);       // transform
      transform);       // transform
      assert(m_dispman_element2 != DISPMANX_NO_HANDLE);
      assert(m_dispman_element2 != (unsigned)DISPMANX_INVALID);
    // left side - fall through
    /*
    dst_rect.x = 0;
    dst_rect.width = m_width;
    */
    dst_rect.x = 0;
    dst_rect.width = res.iWidth - dst_rect.x;
  }
  m_dispman_element = m_DllBcmHost.vc_dispmanx_element_add(dispman_update,
    m_dispman_display,
    1,                              // layer
    &dst_rect,
    (DISPMANX_RESOURCE_HANDLE_T)0,  // src
    &src_rect,
    DISPMANX_PROTECTION_NONE,
    //(VC_DISPMANX_ALPHA_T*)0,        // alpha
    &alpha,
    //(DISPMANX_CLAMP_T*)0,           // clamp
    &clamp,
    //(DISPMANX_TRANSFORM_T)0);       // transform
    transform);       // transform
    assert(m_dispman_element != DISPMANX_NO_HANDLE);
    assert(m_dispman_element != (unsigned)DISPMANX_INVALID);

  memset(nativeWindow, 0, sizeof(EGL_DISPMANX_WINDOW_T));
  nativeWindow->element = m_dispman_element;
  nativeWindow->width   = m_fb_width;
  nativeWindow->height  = m_fb_height;
  m_DllBcmHost.vc_dispmanx_display_set_background(dispman_update, m_dispman_display, 0x00, 0x00, 0x00);
  m_DllBcmHost.vc_dispmanx_update_submit_sync(dispman_update);

  CLog::Log(LOGDEBUG, "CWinEGLPlatformRaspberryPI::SetDisplayResolution(%dx%d) (%dx%d)\n", nativeWindow->width, nativeWindow->height, m_width, m_height);

  if (!setConfiguration())
  {
    free(m_nativeWindow);
    m_nativeWindow = NULL;
    return false;
  }

  return true;
}

bool CWinEGLPlatformRaspberryPI::ClampToGUIDisplayLimits(int &width, int &height)
{
  /*
  width   = m_tv_state.width;
  height  = m_tv_state.height;
  */
  return true;
}

bool CWinEGLPlatformRaspberryPI::ProbeDisplayResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  resolutions.clear();
  
  GetSupportedModes(HDMI_RES_GROUP_CEA, resolutions);
  GetSupportedModes(HDMI_RES_GROUP_DMT, resolutions);
  GetSupportedModes(HDMI_RES_GROUP_CEA_3D, resolutions);

  if(resolutions.size() == 0)
  {
    m_sdMode = true;

    TV_GET_STATE_RESP_T tv;
    m_DllBcmHost.vc_tv_get_state(&tv);

    RESOLUTION_INFO res;
    CLog::Log(LOGNOTICE, "%dx%d@%d %s:%x\n", tv.width, tv.height, tv.frame_rate, tv.scan_mode?"I":"");

    res.iScreen = 0;
    res.bFullScreen = true;
    res.iSubtitles = (int)(0.965 * tv.height);
    res.dwFlags = tv.scan_mode ? D3DPRESENTFLAG_INTERLACED : D3DPRESENTFLAG_PROGRESSIVE;
    res.fRefreshRate = (float)tv.frame_rate;
    res.fPixelRatio = 1.0f;
    res.iWidth = tv.width;
    res.iHeight = tv.height;
    //res.iScreenWidth = tv.width;
    //res.iScreenHeight = tv.height;
    res.strMode.Format("%dx%d", tv.width, tv.height);
    if ((float)tv.frame_rate > 1)
        res.strMode.Format("%s @ %.2f%s - Full Screen", res.strMode, (float)tv.frame_rate, res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

    CStdString resolution;
    resolutions.push_back(res);
    m_desktopRes = res;
    m_res.push_back(res);
  }

  return true;
}

void CWinEGLPlatformRaspberryPI::DestroyWindowSystem(EGLNativeWindowType native_window)
{
  DestroyWindow();

  EGLBoolean eglStatus;
  if (m_context != EGL_NO_CONTEXT)
  {
    eglStatus = eglDestroyContext(m_display, m_context);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL context");
    m_context = EGL_NO_CONTEXT;
  }

  if (m_surface != EGL_NO_SURFACE)
  {
    eglStatus = eglDestroySurface(m_display, m_surface);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL surface");
    m_surface = EGL_NO_SURFACE;
  }

  if (m_display != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglStatus = eglTerminate(m_display);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error terminating EGL");
    m_display = EGL_NO_DISPLAY;
  }

  free(m_nativeWindow);
  m_nativeWindow = NULL;

  if(m_DllBcmHost.IsLoaded())
    m_DllBcmHost.Unload();
}

bool CWinEGLPlatformRaspberryPI::setConfiguration()
{
  EGLBoolean eglStatus;
  EGLint     configCount;
  EGLConfig* configList = NULL;

  m_display = eglGetDisplay(m_nativeDisplay);
  if (m_display == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "EGL failed to obtain display");
    return false;
  }

  if (!eglInitialize(m_display, 0, 0))
  {
    CLog::Log(LOGERROR, "EGL failed to initialize");
    return false;
  }

  EGLint configAttrs[] = {
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_DEPTH_SIZE,      16,
        EGL_SAMPLE_BUFFERS,  1,
        EGL_NONE
  };

  // Find out how many configurations suit our needs  
  eglStatus = eglChooseConfig(m_display, configAttrs, NULL, 0, &configCount);
  if (!eglStatus || !configCount)
  {
    CLog::Log(LOGERROR, "EGL failed to return any matching configurations: %d", eglStatus);
    return false;
  }

  // Allocate room for the list of matching configurations
  configList = (EGLConfig*)malloc(configCount * sizeof(EGLConfig));
  if (!configList)
  {
    CLog::Log(LOGERROR, "kdMalloc failure obtaining configuration list");
    return false;
  }

  // Obtain the configuration list from EGL
  eglStatus = eglChooseConfig(m_display, configAttrs,
                                configList, configCount, &configCount);
  if (!eglStatus || !configCount)
  {
    CLog::Log(LOGERROR, "EGL failed to populate configuration list: %d", eglStatus);
    return false;
  }

  // Select an EGL configuration that matches the native window
  m_config = configList[0];

  if (m_surface != EGL_NO_SURFACE)
  {
    ReleaseSurface();
  }

  m_surface = eglCreateWindowSurface(m_display, m_config, m_nativeWindow, NULL);
  if (!m_surface)
  {
    CLog::Log(LOGERROR, "EGL couldn't create window surface");
    return false;
  }

  // Let's get the current width and height
  EGLint width, height;
  if (!eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width) || !eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height) ||
      width <= 0 || height <= 0)
  {
    CLog::Log(LOGERROR, "EGL couldn't provide the surface's width and/or height");
    return false;
  }

  m_width = width;
  m_height = height;

  free(configList);

  return true;
}

bool CWinEGLPlatformRaspberryPI::BindSurface()
{
  EGLBoolean eglStatus;

  if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE || m_config == NULL)
  {
    CLog::Log(LOGNOTICE, "EGL not configured correctly. Let's try to do that now...");
    if (!setConfiguration())
    {
      CLog::Log(LOGERROR, "EGL not configured correctly to create a surface");
      return false;
    }
  }

  eglStatus = eglBindAPI(EGL_OPENGL_ES_API);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "EGL failed to bind API: %d", eglStatus);
    return false;
  }

  EGLint contextAttrs[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  // Create an EGL context
  if (m_context == EGL_NO_CONTEXT)
  {
    m_context = eglCreateContext(m_display, m_config, NULL, contextAttrs);
    if (!m_context)
    {
      CLog::Log(LOGERROR, "EGL couldn't create context");
      return false;
    }
  }

  // Make the context and surface current to this thread for rendering
  eglStatus = eglMakeCurrent(m_display, m_surface, m_surface, m_context);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "EGL couldn't make context/surface current: %d", eglStatus);
    return false;
  }

  eglSwapInterval(m_display, 0);

  // For EGL backend, it needs to clear all the back buffers of the window
  // surface before drawing anything, otherwise the image will be blinking
  // heavily.  The default eglWindowSurface has 3 gdl surfaces as the back
  // buffer, that's why glClear should be called 3 times.
  glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(m_display, m_surface);

  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(m_display, m_surface);

  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(m_display, m_surface);

  m_eglext  = " ";
  m_eglext += eglQueryString(m_display, EGL_EXTENSIONS);
  m_eglext += " ";
  CLog::Log(LOGDEBUG, "EGL extensions:%s", m_eglext.c_str());

  // setup for vsync disabled
  eglSwapInterval(m_display, 0);

  CLog::Log(LOGNOTICE, "EGL window and context creation complete");

  return true;
}

bool CWinEGLPlatformRaspberryPI::DestroyWindow()
{
  CLog::Log(LOGDEBUG, "CWinEGLPlatformRaspberryPI::DestroyWindow()\n");

  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost.vc_dispmanx_update_start(0);

  if (m_dispman_element != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost.vc_dispmanx_element_remove(dispman_update, m_dispman_element);
    m_dispman_element = DISPMANX_NO_HANDLE;
  }
  if (m_dispman_element2 != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost.vc_dispmanx_element_remove(dispman_update, m_dispman_element2);
    m_dispman_element2 = DISPMANX_NO_HANDLE;
  }
  m_DllBcmHost.vc_dispmanx_update_submit_sync(dispman_update);

  if (m_dispman_display != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost.vc_dispmanx_display_close(m_dispman_display);
    m_dispman_display = DISPMANX_NO_HANDLE;
  }

  return true;
}

bool CWinEGLPlatformRaspberryPI::ShowWindow(bool show)
{
  return true;
}

bool CWinEGLPlatformRaspberryPI::ReleaseSurface()
{
  EGLBoolean eglStatus;

  if (m_surface == EGL_NO_SURFACE)
  {
    return true;
  }

  eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  eglStatus = eglDestroySurface(m_display, m_surface);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "Error destroying EGL surface");
    return false;
  }

  m_surface = EGL_NO_SURFACE;

  return true;
}

void CWinEGLPlatformRaspberryPI::SwapBuffers()
{
  eglSwapBuffers(m_display, m_surface);
}

bool CWinEGLPlatformRaspberryPI::SetVSync(bool enable)
{
  // depending how buffers are setup, eglSwapInterval
  // might fail so let caller decide if this is an error.
  return eglSwapInterval(m_display, enable ? 1 : 0);
}

bool CWinEGLPlatformRaspberryPI::IsExtSupported(const char* extension)
{
  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != std::string::npos;
}

EGLDisplay CWinEGLPlatformRaspberryPI::GetEGLDisplay()
{
  return m_display;
}

EGLSurface CWinEGLPlatformRaspberryPI::GetEGLSurface()
{
  return m_surface;
}

EGLContext CWinEGLPlatformRaspberryPI::GetEGLContext()
{
  return m_context;
}

void CWinEGLPlatformRaspberryPI::TvServiceCallback(uint32_t reason, uint32_t param1, uint32_t param2)
{
  CLog::Log(LOGDEBUG, "tvservice_callback(%d,%d,%d)\n", reason, param1, param2);
  switch(reason)
  {
  case VC_HDMI_UNPLUGGED:
    break;
  case VC_HDMI_STANDBY:
    break;
  case VC_SDTV_NTSC:
  case VC_SDTV_PAL:
  case VC_HDMI_HDMI:
  case VC_HDMI_DVI:
    //Signal we are ready now
    sem_post(&m_tv_synced);
    break;
  default:
     break;
  }
}

void CWinEGLPlatformRaspberryPI::CallbackTvServiceCallback(void *userdata, uint32_t reason, uint32_t param1, uint32_t param2)
{
   CWinEGLPlatformRaspberryPI *omx = static_cast<CWinEGLPlatformRaspberryPI*>(userdata);
   omx->TvServiceCallback(reason, param1, param2);
}

void CWinEGLPlatformRaspberryPI::GetSupportedModes(HDMI_RES_GROUP_T group, std::vector<RESOLUTION_INFO> &resolutions)
{
  //Supported HDMI CEA/DMT resolutions, first one will be preferred resolution
  #define TV_MAX_SUPPORTED_MODES 60
  TV_SUPPORTED_MODE_T supported_modes[TV_MAX_SUPPORTED_MODES];
  int32_t num_modes;
  HDMI_RES_GROUP_T prefer_group;
  uint32_t prefer_mode;
  int i;

  num_modes = m_DllBcmHost.vc_tv_hdmi_get_supported_modes(group,
                                           supported_modes,
                                           TV_MAX_SUPPORTED_MODES,
                                           &prefer_group,
                                           &prefer_mode);
  CLog::Log(LOGNOTICE, "CWinEGLPlatformRaspberryPI::GetSupportedModes (%d) = %d, prefer_group=%x, prefer_mode=%x\n", group, num_modes, prefer_group, prefer_mode);

  if (num_modes > 0 && prefer_group != HDMI_RES_GROUP_INVALID)
  {
    TV_SUPPORTED_MODE_T *tv = supported_modes;
    for (i=0; i < num_modes; i++, tv++)
    {
      /* filter out interlaced modes */
      /*
      if(tv->scan_mode && group != HDMI_RES_GROUP_CEA_3D)
        continue;
      */

      // treat 3D modes as half-width SBS
      unsigned int width = group==HDMI_RES_GROUP_CEA_3D ? tv->width>>1:tv->width;
      RESOLUTION_INFO res;
      CLog::Log(LOGNOTICE, "%d: %dx%d@%d %s%s:%x\n", i, width, tv->height, tv->frame_rate, tv->native?"N":"", tv->scan_mode?"I":"", tv->code);

      res.iScreen = 0;
      res.bFullScreen = true;
      res.iSubtitles = (int)(0.965 * tv->height);
      res.dwFlags = MAKEFLAGS(group, tv->code, tv->scan_mode, group==HDMI_RES_GROUP_CEA_3D);
      res.fRefreshRate = (float)tv->frame_rate;
      res.fPixelRatio = 1.0f;
      res.iWidth = width;
      res.iHeight = tv->height;
      //res.iScreenWidth = width;
      //res.iScreenHeight = tv->height;
      res.strMode.Format("%dx%d", width, tv->height);
      if ((float)tv->frame_rate > 1)
        res.strMode.Format("%s @ %.2f%s - Full Screen", res.strMode, (float)tv->frame_rate, res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

      CStdString resolution;

      //resolution.Format("%dx%d%s%dHz", width, tv->height, tv->scan_mode ? "i" :"p", tv->frame_rate);
      //resolutions.push_back(resolution);

      resolutions.push_back(res);

      if(m_tv_state.width == width && m_tv_state.height == tv->height && 
         m_tv_state.scan_mode == tv->scan_mode && m_tv_state.frame_rate == tv->frame_rate)
        m_desktopRes = res;

      m_res.push_back(res);
    }
  }
}

#endif

#endif
