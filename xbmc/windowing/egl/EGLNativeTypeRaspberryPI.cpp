/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include <EGL/egl.h>
#include "EGLNativeTypeRaspberryPI.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "linux/DllBCM.h"

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

//#ifndef DEBUG_PRINT
//#define DEBUG_PRINT 1
//#endif

#if defined(DEBUG_PRINT)
# define DLOG(fmt, args...) printf(fmt, ##args)
#else
# define DLOG(fmt, args...)
#endif


CEGLNativeTypeRaspberryPI::CEGLNativeTypeRaspberryPI()
{
#if defined(TARGET_RASPBERRY_PI)
  m_DllBcmHost    = NULL;
  m_nativeWindow  = NULL;
#endif
}

CEGLNativeTypeRaspberryPI::~CEGLNativeTypeRaspberryPI()
{
#if defined(TARGET_RASPBERRY_PI)
  delete m_DllBcmHost;
  if(m_nativeWindow)
    free(m_nativeWindow);
#endif
} 

bool CEGLNativeTypeRaspberryPI::CheckCompatibility()
{
#if defined(TARGET_RASPBERRY_PI)
  DLOG("CEGLNativeTypeRaspberryPI::CheckCompatibility\n");
  return true;
#else
  return false;
#endif
}

void CEGLNativeTypeRaspberryPI::Initialize()
{
#if defined(TARGET_RASPBERRY_PI)
  m_DllBcmHost              = NULL;
  m_dispman_element         = DISPMANX_NO_HANDLE;
  m_dispman_element2        = DISPMANX_NO_HANDLE;
  m_dispman_display         = DISPMANX_NO_HANDLE;

  m_width                   = 1280;
  m_height                  = 720;
  m_initDesktopRes          = true;

  m_DllBcmHost = new DllBcmHost;
  m_DllBcmHost->Load();
#endif
}

void CEGLNativeTypeRaspberryPI::Destroy()
{
#if defined(TARGET_RASPBERRY_PI)
  if(m_DllBcmHost && m_DllBcmHost->IsLoaded())
    m_DllBcmHost->Unload();
  delete m_DllBcmHost;
  m_DllBcmHost = NULL;
#endif
}

bool CEGLNativeTypeRaspberryPI::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeRaspberryPI::CreateNativeWindow()
{
#if defined(TARGET_RASPBERRY_PI)
  if(!m_nativeWindow)
    m_nativeWindow = (EGLNativeWindowType) calloc(1,sizeof( EGL_DISPMANX_WINDOW_T));
  DLOG("CEGLNativeTypeRaspberryPI::CEGLNativeTypeRaspberryPI\n");
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeRaspberryPI::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeRaspberryPI::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
  DLOG("CEGLNativeTypeRaspberryPI::GetNativeWindow\n");
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}  

bool CEGLNativeTypeRaspberryPI::DestroyNativeDisplay()
{
  DLOG("CEGLNativeTypeRaspberryPI::DestroyNativeDisplay\n");
  return true;
}

bool CEGLNativeTypeRaspberryPI::DestroyNativeWindow()
{
#if defined(TARGET_RASPBERRY_PI)
  DestroyDispmaxWindow();
  free(m_nativeWindow);
  m_nativeWindow = NULL;
  DLOG("CEGLNativeTypeRaspberryPI::DestroyNativeWindow\n");
  return true;
#else
  return false;
#endif  
}

bool CEGLNativeTypeRaspberryPI::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(TARGET_RASPBERRY_PI)
  *res = m_desktopRes;

  DLOG("CEGLNativeTypeRaspberryPI::GetNativeResolution %s\n", res->strMode.c_str());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeRaspberryPI::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(TARGET_RASPBERRY_PI)
  bool bFound = false;

  if(!m_DllBcmHost || !m_nativeWindow)
    return false;

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
    m_DllBcmHost->vc_tv_register_callback(CallbackTvServiceCallback, this);

    int success = m_DllBcmHost->vc_tv_hdmi_power_on_explicit(HDMI_MODE_HDMI, GETFLAGS_GROUP(resSearch.dwFlags), GETFLAGS_MODE(resSearch.dwFlags));

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
    m_DllBcmHost->vc_tv_unregister_callback(CallbackTvServiceCallback);
    sem_destroy(&m_tv_synced);

    m_desktopRes = resSearch;
  }

  m_dispman_display = m_DllBcmHost->vc_dispmanx_display_open(0);

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
  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost->vc_dispmanx_update_start(0);

  CLog::Log(LOGDEBUG, "EGL set resolution %dx%d -> %dx%d @ %.2f fps\n",
      m_width, m_height, dst_rect.width, dst_rect.height, bFound ? resSearch.fRefreshRate : res.fRefreshRate);

  // The trick for SBS is that we stick two dispman elements together 
  // and the PI firmware knows that we are in SBS mode and it renders the gui in SBS
  if(bFound && (resSearch.dwFlags & D3DPRESENTFLAG_MODE3DSBS))
  {
    // right side
    dst_rect.x = res.iScreenWidth;
    dst_rect.width = res.iScreenWidth;

    m_dispman_element2 = m_DllBcmHost->vc_dispmanx_element_add(dispman_update,
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

  m_dispman_element = m_DllBcmHost->vc_dispmanx_element_add(dispman_update,
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

  EGL_DISPMANX_WINDOW_T *nativeWindow = (EGL_DISPMANX_WINDOW_T *)m_nativeWindow;

  nativeWindow->element = m_dispman_element;
  nativeWindow->width   = m_width;
  nativeWindow->height  = m_height;

  m_DllBcmHost->vc_dispmanx_display_set_background(dispman_update, m_dispman_display, 0x00, 0x00, 0x00);
  m_DllBcmHost->vc_dispmanx_update_submit_sync(dispman_update);

  DLOG("CEGLNativeTypeRaspberryPI::SetNativeResolution\n");

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeRaspberryPI::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
#if defined(TARGET_RASPBERRY_PI)
  resolutions.clear();
  m_res.clear();

  if(!m_DllBcmHost)
    return false;

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
    m_DllBcmHost->vc_tv_get_state(&tv_state);

    m_desktopRes.iScreen      = 0;
    m_desktopRes.bFullScreen  = true;
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

    int gui_width  = m_desktopRes.iWidth;
    int gui_height = m_desktopRes.iHeight;

    ClampToGUIDisplayLimits(gui_width, gui_height);

    m_desktopRes.iWidth = gui_width;
    m_desktopRes.iHeight = gui_height;

    m_desktopRes.iSubtitles   = (int)(0.965 * m_desktopRes.iHeight);

    CLog::Log(LOGDEBUG, "EGL initial desktop resolution %s\n", m_desktopRes.strMode.c_str());
  }


  GetSupportedModes(HDMI_RES_GROUP_CEA, resolutions);
  GetSupportedModes(HDMI_RES_GROUP_DMT, resolutions);
  GetSupportedModes(HDMI_RES_GROUP_CEA_3D, resolutions);

  if(resolutions.size() == 0)
  {
    TV_GET_STATE_RESP_T tv;
    m_DllBcmHost->vc_tv_get_state(&tv);

    RESOLUTION_INFO res;
    CLog::Log(LOGDEBUG, "EGL probe resolution %dx%d@%f %s:%x\n",
        m_desktopRes.iWidth, m_desktopRes.iHeight, m_desktopRes.fRefreshRate,
        m_desktopRes.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "p");

    m_res.push_back(m_desktopRes);
    resolutions.push_back(m_desktopRes);
  }

  if(resolutions.size() < 2)
    m_fixedMode = true;

  DLOG("CEGLNativeTypeRaspberryPI::ProbeResolutions\n");
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeRaspberryPI::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  DLOG("CEGLNativeTypeRaspberryPI::GetPreferredResolution\n");
  return false;
}

bool CEGLNativeTypeRaspberryPI::ShowWindow(bool show)
{
  DLOG("CEGLNativeTypeRaspberryPI::ShowWindow\n");
  return false;
}

#if defined(TARGET_RASPBERRY_PI)
void CEGLNativeTypeRaspberryPI::DestroyDispmaxWindow()
{
  if(!m_DllBcmHost)
    return;

  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost->vc_dispmanx_update_start(0);

  if (m_dispman_element != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost->vc_dispmanx_element_remove(dispman_update, m_dispman_element);
    m_dispman_element = DISPMANX_NO_HANDLE;
  }
  if (m_dispman_element2 != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost->vc_dispmanx_element_remove(dispman_update, m_dispman_element2);
    m_dispman_element2 = DISPMANX_NO_HANDLE;
  }
  m_DllBcmHost->vc_dispmanx_update_submit_sync(dispman_update);

  if (m_dispman_display != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost->vc_dispmanx_display_close(m_dispman_display);
    m_dispman_display = DISPMANX_NO_HANDLE;
  }
  DLOG("CEGLNativeTypeRaspberryPI::DestroyDispmaxWindow\n");
}

void CEGLNativeTypeRaspberryPI::GetSupportedModes(HDMI_RES_GROUP_T group, std::vector<RESOLUTION_INFO> &resolutions)
{
  if(!m_DllBcmHost)
    return;

  //Supported HDMI CEA/DMT resolutions, first one will be preferred resolution
  TV_SUPPORTED_MODE_T supported_modes[TV_MAX_SUPPORTED_MODES];
  int32_t num_modes;
  HDMI_RES_GROUP_T prefer_group;
  uint32_t prefer_mode;
  int i;

  num_modes = m_DllBcmHost->vc_tv_hdmi_get_supported_modes(group,
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
        res.strMode.Format("%dx%d @ %.2f%s - Full Screen", res.iScreenWidth, res.iScreenHeight, res.fRefreshRate,
          res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
      }

      int gui_width  = res.iWidth;
      int gui_height = res.iHeight;

      ClampToGUIDisplayLimits(gui_width, gui_height);

      res.iWidth = gui_width;
      res.iHeight = gui_height;

      res.iSubtitles    = (int)(0.965 * res.iHeight);

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

void CEGLNativeTypeRaspberryPI::TvServiceCallback(uint32_t reason, uint32_t param1, uint32_t param2)
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

void CEGLNativeTypeRaspberryPI::CallbackTvServiceCallback(void *userdata, uint32_t reason, uint32_t param1, uint32_t param2)
{
   CEGLNativeTypeRaspberryPI *callback = static_cast<CEGLNativeTypeRaspberryPI*>(userdata);
   callback->TvServiceCallback(reason, param1, param2);
}

bool CEGLNativeTypeRaspberryPI::ClampToGUIDisplayLimits(int &width, int &height)
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

#endif

