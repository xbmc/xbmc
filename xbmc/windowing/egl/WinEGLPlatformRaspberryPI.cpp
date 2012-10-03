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

#define IS_WIDESCREEN(m) ( m == 3 || m == 7 || m == 9 || \
    m == 11 || m == 13 || m == 15 || m == 18 || m == 22 || \
    m == 24 || m == 26 || m == 28 || m == 30 || m == 36 || \
    m == 38 || m == 43 || m == 45 || m == 49 || m == 51 || \
    m == 53 || m == 55 || m == 57 || m == 59)

#define MAKEFLAGS(group, mode, interlace, mode3d) \
  ( ( (mode)<<24 ) | ( (group)<<16 ) | \
   ( (interlace) != 0 ? D3DPRESENTFLAG_INTERLACED : D3DPRESENTFLAG_PROGRESSIVE) | \
   ( ((group) == HDMI_RES_GROUP_CEA && IS_WIDESCREEN(mode) ) ? D3DPRESENTFLAG_WIDESCREEN : 0) | \
   ( (mode3d) != 0 ? D3DPRESENTFLAG_MODE3DSBS : 0 ))

#define GETFLAGS_INTERLACE(f)   ( ( (f) & D3DPRESENTFLAG_INTERLACED ) != 0)
#define GETFLAGS_WIDESCREEN(f)  ( ( (f) & D3DPRESENTFLAG_WIDESCREEN ) != 0)
#define GETFLAGS_GROUP(f)       ( (HDMI_RES_GROUP_T)( ((f) >> 16) & 0xff ))
#define GETFLAGS_MODE(f)        ( ( (f) >>24 ) & 0xff )
#define GETFLAGS_MODE3D(f)      ( ( (f) & D3DPRESENTFLAG_MODE3DSBS ) !=0 )

#define TV_MAX_SUPPORTED_MODES 60

CWinEGLPlatformRaspberryPI::CWinEGLPlatformRaspberryPI()
{
  m_surface                 = EGL_NO_SURFACE;
  m_context                 = EGL_NO_CONTEXT;
  m_display                 = EGL_NO_DISPLAY;

  m_dispman_element         = DISPMANX_NO_HANDLE;
  m_dispman_element2        = DISPMANX_NO_HANDLE;
  m_dispman_display         = DISPMANX_NO_HANDLE;

  m_fixedMode               = false;

  m_nativeWindow = (EGL_DISPMANX_WINDOW_T *)malloc(sizeof(EGL_DISPMANX_WINDOW_T));
  memset(m_nativeWindow, 0x0, sizeof(EGL_DISPMANX_WINDOW_T));

  m_initDesktopRes          = true;
}

CWinEGLPlatformRaspberryPI::~CWinEGLPlatformRaspberryPI()
{
  UninitializeDisplay();

  free(m_nativeWindow);

  if(m_DllBcmHost.IsLoaded())
    m_DllBcmHost.Unload();
}

bool CWinEGLPlatformRaspberryPI::SetDisplayResolution(RESOLUTION_INFO& res)
{
  bool bFound = false;

  DestroyDispmaxWindow();

  RESOLUTION_INFO resSearch;

  int best_score = 0;

  for (size_t i = 0; i < m_res.size(); i++)
  {
    if(m_res[i].iScreenWidth == res.iScreenWidth && m_res[i].iScreenHeight == res.iScreenHeight && m_res[i].fRefreshRate == res.fRefreshRate)
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

  if(bFound && !m_fixedMode)
  {
    sem_init(&m_tv_synced, 0, 0);
    m_DllBcmHost.vc_tv_register_callback(CallbackTvServiceCallback, this);
  
    int success = m_DllBcmHost.vc_tv_hdmi_power_on_explicit(HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), GETFLAGS_MODE(resSearch.dwFlags));

    if (success == 0) 
    {
      CLog::Log(LOGDEBUG, "EGL set HDMI mode (%d,%d,%d)=%d\n", 
                          GETFLAGS_MODE3D(resSearch.dwFlags) ? HDMI_MODE_3D:HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), 
                          GETFLAGS_MODE(resSearch.dwFlags), success);
      sem_wait(&m_tv_synced);
    } 
    else 
    {
      CLog::Log(LOGERROR, "EGL failed to set HDMI mode (%d,%d,%d)=%d\n", 
                          GETFLAGS_MODE3D(resSearch.dwFlags) ? HDMI_MODE_3D:HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), 
                          GETFLAGS_MODE(resSearch.dwFlags), success);
    }
    m_DllBcmHost.vc_tv_unregister_callback(CallbackTvServiceCallback);
    sem_destroy(&m_tv_synced);

    m_desktopRes = resSearch; 
  }

  m_dispman_display = m_DllBcmHost.vc_dispmanx_display_open(0);

  m_width   = res.iWidth;
  m_height  = res.iHeight;

  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  dst_rect.x      = 0;
  dst_rect.y      = 0;
  dst_rect.width  = res.iScreenWidth;
  dst_rect.height = res.iScreenHeight;

  src_rect.x      = 0;
  src_rect.y      = 0;
  src_rect.width  = m_width << 16;
  src_rect.height = m_height << 16;

  VC_DISPMANX_ALPHA_T alpha;
  memset(&alpha, 0x0, sizeof(VC_DISPMANX_ALPHA_T));
  alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE;

  DISPMANX_CLAMP_T clamp;
  memset(&clamp, 0x0, sizeof(DISPMANX_CLAMP_T));

  DISPMANX_TRANSFORM_T transform = DISPMANX_NO_ROTATE;
  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost.vc_dispmanx_update_start(0);

  CLog::Log(LOGDEBUG, "EGL set resolution %dx%d -> %dx%d @ %.2f fps\n", 
      m_width, m_height, dst_rect.width, dst_rect.height, bFound ? resSearch.fRefreshRate : res.fRefreshRate);

  // The trick for SBS is that we stick two dispman elements together 
  // and the PI firmware knows that we are in SBS mode and it renders the gui in SBS
  if(bFound && (resSearch.dwFlags & D3DPRESENTFLAG_MODE3DSBS))
  {
    // right side
    dst_rect.x = res.iScreenWidth;
    dst_rect.width = res.iScreenWidth;

    m_dispman_element2 = m_DllBcmHost.vc_dispmanx_element_add(dispman_update,
      m_dispman_display,
      1,                              // layer
      &dst_rect,
      (DISPMANX_RESOURCE_HANDLE_T)0,  // src
      &src_rect,
      DISPMANX_PROTECTION_NONE,
      &alpha,                         // alpha
      &clamp,                         // clamp
      transform);                     // transform
      assert(m_dispman_element2 != DISPMANX_NO_HANDLE);
      assert(m_dispman_element2 != (unsigned)DISPMANX_INVALID);

    // left side - fall through
    dst_rect.x = 0;
    dst_rect.width = res.iScreenWidth;
  }

  m_dispman_element = m_DllBcmHost.vc_dispmanx_element_add(dispman_update,
    m_dispman_display,
    1,                              // layer
    &dst_rect,
    (DISPMANX_RESOURCE_HANDLE_T)0,  // src
    &src_rect,
    DISPMANX_PROTECTION_NONE,
    &alpha,                         //alphe
    &clamp,                         //clamp
    transform);                     // transform

  assert(m_dispman_element != DISPMANX_NO_HANDLE);
  assert(m_dispman_element != (unsigned)DISPMANX_INVALID);

  memset(m_nativeWindow, 0, sizeof(EGL_DISPMANX_WINDOW_T));

  m_nativeWindow->element = m_dispman_element;
  m_nativeWindow->width   = m_width;
  m_nativeWindow->height  = m_height;

  m_DllBcmHost.vc_dispmanx_display_set_background(dispman_update, m_dispman_display, 0x00, 0x00, 0x00);
  m_DllBcmHost.vc_dispmanx_update_submit_sync(dispman_update);

  return true;
}

bool CWinEGLPlatformRaspberryPI::ClampToGUIDisplayLimits(int &width, int &height)
{
  const int max_width = 1280, max_height = 720;
  float ar = (float)width/(float)height;
  // bigger than maximum, so need to clamp
  if (width > max_width || height > max_height) {
    // wider than max, so clamp width first
    if (ar > (float)max_width/(float)max_height)
    {
      width = max_width;
      height = (float)max_width / ar + 0.5f;
    // taller than max, so clamp height first
    } else {
      height = max_height;
      width = (float)max_height * ar + 0.5f;
    }
    return true;
  }
  return false;
}

bool CWinEGLPlatformRaspberryPI::ProbeDisplayResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  resolutions.clear();
  m_res.clear();
  
  m_fixedMode               = false;

  /* read initial desktop resolution before probe resolutions.
   * probing will replace the desktop resolution when it finds the same one.
   * we raplace it because probing will generate more detailed 
   * resolution flags we don't get with vc_tv_get_state.
   */

  if(m_initDesktopRes)
  {
    TV_GET_STATE_RESP_T tv_state;

    // get current display settings state
    memset(&tv_state, 0, sizeof(TV_GET_STATE_RESP_T));
    m_DllBcmHost.vc_tv_get_state(&tv_state);

    m_desktopRes.iScreen      = 0;
    m_desktopRes.bFullScreen  = true;
    m_desktopRes.iSubtitles   = (int)(0.965 * tv_state.height);
    m_desktopRes.iWidth       = tv_state.width;
    m_desktopRes.iHeight      = tv_state.height;
    m_desktopRes.iScreenWidth = tv_state.width;
    m_desktopRes.iScreenHeight= tv_state.height;
    m_desktopRes.dwFlags      = tv_state.scan_mode ? D3DPRESENTFLAG_INTERLACED : D3DPRESENTFLAG_PROGRESSIVE;
    m_desktopRes.fRefreshRate = (float)tv_state.frame_rate;
    m_desktopRes.strMode.Format("%dx%d", tv_state.width, tv_state.height);
    if((float)tv_state.frame_rate > 1)
    {
        m_desktopRes.strMode.Format("%s @ %.2f%s - Full Screen", m_desktopRes.strMode, (float)tv_state.frame_rate, 
            m_desktopRes.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
    }
    m_initDesktopRes = false;

    CLog::Log(LOGDEBUG, "EGL initial desktop resolution %s\n", m_desktopRes.strMode.c_str());
  }

  GetSupportedModes(HDMI_RES_GROUP_CEA, resolutions);
  GetSupportedModes(HDMI_RES_GROUP_DMT, resolutions);
  GetSupportedModes(HDMI_RES_GROUP_CEA_3D, resolutions);

  if(resolutions.size() == 0)
  {
    TV_GET_STATE_RESP_T tv;
    m_DllBcmHost.vc_tv_get_state(&tv);

    RESOLUTION_INFO res;
    CLog::Log(LOGDEBUG, "EGL probe resolution %dx%d@%f %s:%x\n",
        m_desktopRes.iWidth, m_desktopRes.iHeight, m_desktopRes.fRefreshRate,
        m_desktopRes.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "p");

    m_res.push_back(m_desktopRes);
    resolutions.push_back(m_desktopRes);
  }

  if(resolutions.size() < 2)
    m_fixedMode = true;

  return true;
}

EGLNativeWindowType CWinEGLPlatformRaspberryPI::InitWindowSystem(EGLNativeDisplayType nativeDisplay, int width, int height, int bpp)
{
  m_nativeDisplay = nativeDisplay;
  m_width         = width;
  m_height        = height;

  m_DllBcmHost.Load();

  return getNativeWindow();
}

void CWinEGLPlatformRaspberryPI::DestroyWindowSystem(EGLNativeWindowType native_window)
{
  UninitializeDisplay();
}

bool CWinEGLPlatformRaspberryPI::InitializeDisplay()
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
    CLog::Log(LOGERROR, "EGL failed to return any matching configurations: %d error : 0x%08x", eglStatus, eglGetError());
    return false;
  }

  // Allocate room for the list of matching configurations
  configList = (EGLConfig*)malloc(configCount * sizeof(EGLConfig));
  if (!configList)
  {
    CLog::Log(LOGERROR, "EGL malloc failure obtaining configuration list");
    return false;
  }

  // Obtain the configuration list from EGL
  eglStatus = eglChooseConfig(m_display, configAttrs,
                                configList, configCount, &configCount);
  if (!eglStatus || !configCount)
  {
    CLog::Log(LOGERROR, "EGL failed to populate configuration list: %d error : 0x%08x", eglStatus, eglGetError());
    return false;
  }

  // Select an EGL configuration that matches the native window
  m_config = configList[0];

  if (m_surface != EGL_NO_SURFACE)
    ReleaseSurface();

  free(configList);

  return true;
}

bool CWinEGLPlatformRaspberryPI::UninitializeDisplay()
{
  EGLBoolean eglStatus;

  // Recreate a new rendering context doesn't work on the PI.
  // We have to keep the first created context alive until we exit.
  if (m_context != EGL_NO_CONTEXT)
  {
    eglStatus = eglDestroyContext(m_display, m_context);
    if (!eglStatus)
      CLog::Log(LOGERROR, "EGL destroy context error : 0x%08x", eglGetError());
    m_context = EGL_NO_CONTEXT;
  }

  DestroyWindow();

  DestroyDispmaxWindow();

  if (m_display != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglStatus = eglTerminate(m_display);
    if (!eglStatus)
      CLog::Log(LOGERROR, "EGL terminate error 0x%08x", eglGetError());
    m_display = EGL_NO_DISPLAY;
  }

  return true;
}

bool CWinEGLPlatformRaspberryPI::CreateWindow()
{
  if (m_display == EGL_NO_DISPLAY || m_config == NULL)
  {
    if (!InitializeDisplay())
      return false;
  }

  if (m_surface != EGL_NO_SURFACE)
    return true;

  m_nativeWindow = (EGL_DISPMANX_WINDOW_T *)getNativeWindow();

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

  return true;
}

void CWinEGLPlatformRaspberryPI::DestroyDispmaxWindow()
{
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
}

bool CWinEGLPlatformRaspberryPI::DestroyWindow()
{
  EGLBoolean eglStatus;

  ReleaseSurface();

  if (m_surface == EGL_NO_SURFACE)
    return true;

  eglStatus = eglDestroySurface(m_display, m_surface);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "EGL destroy surface error : 0x%08x", eglGetError());
    return false;
  }

  m_surface = EGL_NO_SURFACE;
  m_width = 0;
  m_height = 0;

  return true;
}

bool CWinEGLPlatformRaspberryPI::BindSurface()
{
  EGLBoolean eglStatus;

  if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE || m_config == NULL)
  {
    CLog::Log(LOGINFO, "EGL not configured correctly. Let's try to do that now...");
    if (!CreateWindow())
    {
      CLog::Log(LOGERROR, "EGL not configured correctly to create a surface");
      return false;
    }
  }

  eglStatus = eglBindAPI(EGL_OPENGL_ES_API);
  if (!eglStatus)
  {
    CLog::Log(LOGERROR, "EGL failed to bind API: %d error 0x%08x", eglStatus, eglGetError());
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
    m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, contextAttrs);
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
    CLog::Log(LOGERROR, "EGL couldn't make context/surface current: %d error : 0x%08x", eglStatus, eglGetError());
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

  CLog::Log(LOGINFO, "EGL window and context creation complete");

  return true;
}

bool CWinEGLPlatformRaspberryPI::ReleaseSurface()
{
  EGLBoolean eglStatus;

  if (m_display != EGL_NO_DISPLAY)
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  // Recreate a new rendering context doesn't work on the PI.
  // We have to keep the first created context alive until we exit.
  /*
  if (m_context != EGL_NO_CONTEXT)
  {
    eglStatus = eglDestroyContext(m_display, m_context);
    if (!eglStatus)
      CLog::Log(LOGERROR, "Error destroying EGL context 0x%08x", eglGetError());
    m_context = EGL_NO_CONTEXT;
  }
  */

  return true;
}

bool CWinEGLPlatformRaspberryPI::ShowWindow(bool show)
{
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

EGLNativeWindowType CWinEGLPlatformRaspberryPI::getNativeWindow()
{
  return (EGLNativeWindowType)m_nativeWindow;
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
  CLog::Log(LOGDEBUG, "EGL tv_service_callback (%d,%d,%d)\n", reason, param1, param2);
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
   CWinEGLPlatformRaspberryPI *callback = static_cast<CWinEGLPlatformRaspberryPI*>(userdata);
   callback->TvServiceCallback(reason, param1, param2);
}

void CWinEGLPlatformRaspberryPI::GetSupportedModes(HDMI_RES_GROUP_T group, std::vector<RESOLUTION_INFO> &resolutions)
{
  //Supported HDMI CEA/DMT resolutions, first one will be preferred resolution
  TV_SUPPORTED_MODE_T supported_modes[TV_MAX_SUPPORTED_MODES];
  int32_t num_modes;
  HDMI_RES_GROUP_T prefer_group;
  uint32_t prefer_mode;
  int i;

  num_modes = m_DllBcmHost.vc_tv_hdmi_get_supported_modes(group,
      supported_modes, TV_MAX_SUPPORTED_MODES, &prefer_group, &prefer_mode);

  CLog::Log(LOGDEBUG, "EGL get supported modes (%d) = %d, prefer_group=%x, prefer_mode=%x\n", 
      group, num_modes, prefer_group, prefer_mode);

  if (num_modes > 0 && prefer_group != HDMI_RES_GROUP_INVALID)
  {
    TV_SUPPORTED_MODE_T *tv = supported_modes;
    for (i=0; i < num_modes; i++, tv++)
    {
      // treat 3D modes as half-width SBS
      unsigned int width = (group == HDMI_RES_GROUP_CEA_3D) ? tv->width>>1 : tv->width;
      RESOLUTION_INFO res;
      CLog::Log(LOGDEBUG, "EGL mode %d: %dx%d@%d %s%s:%x\n", i, width, tv->height, tv->frame_rate, 
          tv->native ? "N" : "", tv->scan_mode ? "I" : "", tv->code);

      res.iScreen       = 0;
      res.bFullScreen   = true;
      res.iSubtitles    = (int)(0.965 * tv->height);
      res.dwFlags       = MAKEFLAGS(group, tv->code, tv->scan_mode, group==HDMI_RES_GROUP_CEA_3D);
      res.fRefreshRate  = (float)tv->frame_rate;
      res.fPixelRatio   = 1.0f;
      res.iWidth        = width;
      res.iHeight       = tv->height;
      res.iScreenWidth  = width;
      res.iScreenHeight = tv->height;
      res.strMode.Format("%dx%d", width, tv->height);
      if((float)tv->frame_rate > 1)
      {
        res.strMode.Format("%s @ %.2f%s - Full Screen", res.strMode, (float)tv->frame_rate, 
            res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
      }

      resolutions.push_back(res);

      /* replace initial desktop resolution with probed hdmi resolution */
      if(m_desktopRes.iWidth == res.iWidth && m_desktopRes.iHeight == res.iHeight &&
          m_desktopRes.iScreenWidth == res.iScreenWidth && m_desktopRes.iScreenHeight == res.iScreenHeight &&
          m_desktopRes.fRefreshRate == res.fRefreshRate)
      {
        m_desktopRes = res;
        CLog::Log(LOGDEBUG, "EGL desktop replacement resolution %dx%d@%d %s%s:%x\n", 
            width, tv->height, tv->frame_rate, tv->native ? "N" : "", tv->scan_mode ? "I" : "", tv->code);
      }

      m_res.push_back(res);
    }
  }
}

#endif

#endif
