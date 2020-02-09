/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPIUtils.h"

#include "ServiceBroker.h"
#include "guilib/StereoscopicsManager.h"
#include "guilib/gui3d.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "platform/linux/DllBCM.h"
#include "platform/linux/RBP.h"

#include <cassert>
#include <math.h>

#ifndef __VIDEOCORE4__
#define __VIDEOCORE4__
#endif

#define __VCCOREVER__ 0x04000000

#define IS_WIDESCREEN(m) ( m == 3 || m == 7 || m == 9 || \
    m == 11 || m == 13 || m == 15 || m == 18 || m == 22 || \
    m == 24 || m == 26 || m == 28 || m == 30 || m == 36 || \
    m == 38 || m == 43 || m == 45 || m == 49 || m == 51 || \
    m == 53 || m == 55 || m == 57 || m == 59)

#define MAKEFLAGS(group, mode, interlace) \
  ( ( (mode)<<24 ) | ( (group)<<16 ) | \
   ( (interlace) != 0 ? D3DPRESENTFLAG_INTERLACED : D3DPRESENTFLAG_PROGRESSIVE) | \
   ( ((group) == HDMI_RES_GROUP_CEA && IS_WIDESCREEN(mode) ) ? D3DPRESENTFLAG_WIDESCREEN : 0) )

#define GETFLAGS_GROUP(f)       ( (HDMI_RES_GROUP_T)( ((f) >> 16) & 0xff ))
#define GETFLAGS_MODE(f)        ( ( (f) >>24 ) & 0xff )

static void SetResolutionString(RESOLUTION_INFO &res);
static SDTV_ASPECT_T get_sdtv_aspect_from_display_aspect(float display_aspect);

CRPIUtils::CRPIUtils()
{
  m_DllBcmHost = new DllBcmHost;
  m_DllBcmHost->Load();

  m_dispman_element = DISPMANX_NO_HANDLE;
  m_dispman_display = DISPMANX_NO_HANDLE;

  m_height = 1280;
  m_width = 720;
  m_screen_width = 1280;
  m_screen_height = 720;
  m_shown = false;

  m_initDesktopRes = true;
}

CRPIUtils::~CRPIUtils()
{
  if(m_DllBcmHost && m_DllBcmHost->IsLoaded())
  {
    m_DllBcmHost->Unload();
  }

  delete m_DllBcmHost;
  m_DllBcmHost = NULL;
}

bool CRPIUtils::GetNativeResolution(RESOLUTION_INFO *res) const
{
  *res = m_desktopRes;

  return true;
}

int CRPIUtils::FindMatchingResolution(const RESOLUTION_INFO &res, const std::vector<RESOLUTION_INFO> &resolutions, bool desktop)
{
  uint32_t mask = desktop ? D3DPRESENTFLAG_MODEMASK : D3DPRESENTFLAG_MODE3DSBS|D3DPRESENTFLAG_MODE3DTB;
  for (int i = 0; i < (int)resolutions.size(); i++)
  {
    if(resolutions[i].iScreenWidth == res.iScreenWidth && resolutions[i].iScreenHeight == res.iScreenHeight && resolutions[i].fRefreshRate == res.fRefreshRate &&
      (resolutions[i].dwFlags & mask) == (res.dwFlags & mask))
    {
       return i;
    }
  }
  return -1;
}

int CRPIUtils::AddUniqueResolution(RESOLUTION_INFO &res, std::vector<RESOLUTION_INFO> &resolutions, bool desktop /* = false */)
{
  SetResolutionString(res);
  int i = FindMatchingResolution(res, resolutions, desktop);
  if (i>=0)
  {  // don't replace a progressive resolution with an interlaced one of same resolution
    if (!(res.dwFlags & D3DPRESENTFLAG_INTERLACED))
      resolutions[i] = res;
  }
  else
  {
     resolutions.push_back(res);
  }
  return i;
}

bool CRPIUtils::SetNativeResolution(const RESOLUTION_INFO res, EGLSurface m_nativeWindow)
{
  if(!m_DllBcmHost || !m_nativeWindow)
    return false;

  DestroyDispmanxWindow();

  RENDER_STEREO_MODE stereo_mode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
  if(GETFLAGS_GROUP(res.dwFlags) && GETFLAGS_MODE(res.dwFlags))
  {
    uint32_t mode3d = HDMI_3D_FORMAT_NONE;
    sem_init(&m_tv_synced, 0, 0);
    m_DllBcmHost->vc_tv_register_callback(CallbackTvServiceCallback, this);

    if (stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL || stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    {
      /* inform TV of any 3D settings. Note this property just applies to next hdmi mode change, so no need to call for 2D modes */
      HDMI_PROPERTY_PARAM_T property;
      property.property = HDMI_PROPERTY_3D_STRUCTURE;
      const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      if (settings->GetBool(CSettings::SETTING_VIDEOSCREEN_FRAMEPACKING) &&
          settings->GetBool(CSettings::SETTING_VIDEOPLAYER_SUPPORTMVC) && res.fRefreshRate <= 30.0f)
        property.param1 = HDMI_3D_FORMAT_FRAME_PACKING;
      else if (stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
        property.param1 = HDMI_3D_FORMAT_SBS_HALF;
      else if (stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
        property.param1 = HDMI_3D_FORMAT_TB_HALF;
      else
        property.param1 = HDMI_3D_FORMAT_NONE;
      property.param2 = 0;
      mode3d = property.param1;
      vc_tv_hdmi_set_property(&property);
    }

    HDMI_PROPERTY_PARAM_T property;
    property.property = HDMI_PROPERTY_PIXEL_CLOCK_TYPE;
    // if we are closer to ntsc version of framerate, let gpu know
    int   iFrameRate  = (int)(res.fRefreshRate + 0.5f);
    if (fabsf(res.fRefreshRate * (1001.0f / 1000.0f) - iFrameRate) < fabsf(res.fRefreshRate - iFrameRate))
      property.param1 = HDMI_PIXEL_CLOCK_TYPE_NTSC;
    else
      property.param1 = HDMI_PIXEL_CLOCK_TYPE_PAL;
    property.param2 = 0;
    vc_tv_hdmi_set_property(&property);

    int success = m_DllBcmHost->vc_tv_hdmi_power_on_explicit_new(HDMI_MODE_HDMI, GETFLAGS_GROUP(res.dwFlags), GETFLAGS_MODE(res.dwFlags));

    if (success == 0)
    {
      CLog::Log(LOGDEBUG, "EGL set HDMI mode (%d,%d)=%d %s%s",
                          GETFLAGS_GROUP(res.dwFlags), GETFLAGS_MODE(res.dwFlags), success,
                          CStereoscopicsManager::ConvertGuiStereoModeToString(stereo_mode),
                          mode3d==HDMI_3D_FORMAT_FRAME_PACKING ? " FP" : mode3d==HDMI_3D_FORMAT_SBS_HALF ? " SBS" : mode3d==HDMI_3D_FORMAT_TB_HALF ? " TB" : "");

      sem_wait(&m_tv_synced);
    }
    else
    {
      CLog::Log(LOGERROR, "EGL failed to set HDMI mode (%d,%d)=%d %s%s",
                          GETFLAGS_GROUP(res.dwFlags), GETFLAGS_MODE(res.dwFlags), success,
                          CStereoscopicsManager::ConvertGuiStereoModeToString(stereo_mode),
                          mode3d==HDMI_3D_FORMAT_FRAME_PACKING ? " FP" : mode3d==HDMI_3D_FORMAT_SBS_HALF ? " SBS" : mode3d==HDMI_3D_FORMAT_TB_HALF ? " TB" : "");
    }
    m_DllBcmHost->vc_tv_unregister_callback(CallbackTvServiceCallback);
    sem_destroy(&m_tv_synced);

    m_desktopRes = res;
  }
  else if(!GETFLAGS_GROUP(res.dwFlags) && GETFLAGS_MODE(res.dwFlags))
  {
    sem_init(&m_tv_synced, 0, 0);
    m_DllBcmHost->vc_tv_register_callback(CallbackTvServiceCallback, this);

    SDTV_OPTIONS_T options;
    options.aspect = get_sdtv_aspect_from_display_aspect((float)res.iScreenWidth / (float)res.iScreenHeight);

    int success = m_DllBcmHost->vc_tv_sdtv_power_on((SDTV_MODE_T)GETFLAGS_MODE(res.dwFlags), &options);

    if (success == 0)
    {
      CLog::Log(LOGDEBUG, "EGL set SDTV mode (%d,%d)=%d",
                          GETFLAGS_GROUP(res.dwFlags), GETFLAGS_MODE(res.dwFlags), success);

      sem_wait(&m_tv_synced);
    }
    else
    {
      CLog::Log(LOGERROR, "EGL failed to set SDTV mode (%d,%d)=%d",
                          GETFLAGS_GROUP(res.dwFlags), GETFLAGS_MODE(res.dwFlags), success);
    }
    m_DllBcmHost->vc_tv_unregister_callback(CallbackTvServiceCallback);
    sem_destroy(&m_tv_synced);

    m_desktopRes = res;
  }

  m_dispman_display = g_RBP.OpenDisplay(0);

  m_width   = res.iWidth;
  m_height  = res.iHeight;

  m_screen_width   = res.iScreenWidth;
  m_screen_height  = res.iScreenHeight;

  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  dst_rect.x      = 0;
  dst_rect.y      = 0;
  dst_rect.width  = m_screen_width;
  dst_rect.height = m_screen_height;

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

  if (stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    transform = DISPMANX_STEREOSCOPIC_SBS;
  else if (stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
    transform = DISPMANX_STEREOSCOPIC_TB;
  else
    transform = DISPMANX_STEREOSCOPIC_MONO;

  CLog::Log(LOGDEBUG, "EGL set resolution %dx%d -> %dx%d @ %.2f fps (%d,%d) flags:%x aspect:%.2f",
      m_width, m_height, dst_rect.width, dst_rect.height, res.fRefreshRate, GETFLAGS_GROUP(res.dwFlags), GETFLAGS_MODE(res.dwFlags), (int)res.dwFlags, res.fPixelRatio);

  m_dispman_element = m_DllBcmHost->vc_dispmanx_element_add(dispman_update,
    m_dispman_display,
    1,                              // layer
    &dst_rect,
    (DISPMANX_RESOURCE_HANDLE_T)0,  // src
    &src_rect,
    DISPMANX_PROTECTION_NONE,
    &alpha,                         //alpha
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
  m_shown = true;

  return true;
}

static float get_display_aspect_ratio(HDMI_ASPECT_T aspect)
{
  float display_aspect;
  switch (aspect) {
    case HDMI_ASPECT_4_3:   display_aspect = 4.0/3.0;   break;
    case HDMI_ASPECT_14_9:  display_aspect = 14.0/9.0;  break;
    case HDMI_ASPECT_16_9:  display_aspect = 16.0/9.0;  break;
    case HDMI_ASPECT_5_4:   display_aspect = 5.0/4.0;   break;
    case HDMI_ASPECT_16_10: display_aspect = 16.0/10.0; break;
    case HDMI_ASPECT_15_9:  display_aspect = 15.0/9.0;  break;
    case HDMI_ASPECT_64_27: display_aspect = 64.0/27.0; break;
    default:                display_aspect = 16.0/9.0;  break;
  }
  return display_aspect;
}

static float get_display_aspect_ratio(SDTV_ASPECT_T aspect)
{
  float display_aspect;
  switch (aspect) {
    case SDTV_ASPECT_4_3:  display_aspect = 4.0/3.0;  break;
    case SDTV_ASPECT_14_9: display_aspect = 14.0/9.0; break;
    case SDTV_ASPECT_16_9: display_aspect = 16.0/9.0; break;
    default:               display_aspect = 4.0/3.0;  break;
  }
  return display_aspect;
}

static bool ClampToGUIDisplayLimits(int &width, int &height)
{
  float max_height = (float)g_RBP.GetGUIResolutionLimit();
  float default_ar = 16.0f/9.0f;
  if (max_height < 540.0f || max_height > 1080.0f)
    max_height = 1080.0f;

  float ar = (float)width/(float)height;
  float max_width = max_height * default_ar;
  // bigger than maximum, so need to clamp
  if (width > max_width || height > max_height) {
    // wider than max, so clamp width first
    if (ar > default_ar)
    {
      width = max_width;
      height = max_width / ar + 0.5f;
    // taller than max, so clamp height first
    } else {
      height = max_height;
      width = max_height * ar + 0.5f;
    }
    return true;
  }

  return false;
}

static void SetResolutionString(RESOLUTION_INFO &res)
{
  int gui_width  = res.iScreenWidth;
  int gui_height = res.iScreenHeight;

  ClampToGUIDisplayLimits(gui_width, gui_height);

  res.iWidth = gui_width;
  res.iHeight = gui_height;

  res.strMode = StringUtils::Format("%dx%d (%dx%d) @ %.2f%s - Full Screen", res.iScreenWidth, res.iScreenHeight, res.iWidth, res.iHeight, res.fRefreshRate,
    res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
}

static SDTV_ASPECT_T get_sdtv_aspect_from_display_aspect(float display_aspect)
{
  SDTV_ASPECT_T aspect;
  const float delta = 1e-3;
  if(fabs(get_display_aspect_ratio(SDTV_ASPECT_16_9) - display_aspect) < delta)
  {
    aspect = SDTV_ASPECT_16_9;
  }
  else if(fabs(get_display_aspect_ratio(SDTV_ASPECT_14_9) - display_aspect) < delta)
  {
    aspect = SDTV_ASPECT_14_9;
  }
  else
  {
    aspect = SDTV_ASPECT_4_3;
  }
  return aspect;
}

bool CRPIUtils::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  resolutions.clear();

  if(!m_DllBcmHost)
    return false;

  /* read initial desktop resolution before probe resolutions.
   * probing will replace the desktop resolution when it finds the same one.
   * we replace it because probing will generate more detailed
   * resolution flags we don't get with vc_tv_get_state.
   */

  if(m_initDesktopRes)
  {
    TV_DISPLAY_STATE_T tv_state;

    // get current display settings state
    memset(&tv_state, 0, sizeof(TV_DISPLAY_STATE_T));
    m_DllBcmHost->vc_tv_get_display_state(&tv_state);

    if ((tv_state.state & ( VC_HDMI_HDMI | VC_HDMI_DVI )) != 0) // hdtv
    {
      m_desktopRes.bFullScreen  = true;
      m_desktopRes.iWidth       = tv_state.display.hdmi.width;
      m_desktopRes.iHeight      = tv_state.display.hdmi.height;
      m_desktopRes.iScreenWidth = tv_state.display.hdmi.width;
      m_desktopRes.iScreenHeight= tv_state.display.hdmi.height;
      m_desktopRes.dwFlags      = MAKEFLAGS(tv_state.display.hdmi.group, tv_state.display.hdmi.mode, tv_state.display.hdmi.scan_mode);
      m_desktopRes.fPixelRatio  = tv_state.display.hdmi.display_options.aspect == 0 ? 1.0f : get_display_aspect_ratio((HDMI_ASPECT_T)tv_state.display.hdmi.display_options.aspect) / ((float)m_desktopRes.iScreenWidth / (float)m_desktopRes.iScreenHeight);
      HDMI_PROPERTY_PARAM_T property;
      property.property = HDMI_PROPERTY_PIXEL_CLOCK_TYPE;
      vc_tv_hdmi_get_property(&property);
      m_desktopRes.fRefreshRate = property.param1 == HDMI_PIXEL_CLOCK_TYPE_NTSC ? tv_state.display.hdmi.frame_rate * (1000.0f/1001.0f) : tv_state.display.hdmi.frame_rate;
    }
    else if ((tv_state.state & ( VC_SDTV_NTSC | VC_SDTV_PAL )) != 0) // sdtv
    {
      m_desktopRes.bFullScreen  = true;
      m_desktopRes.iWidth       = tv_state.display.sdtv.width;
      m_desktopRes.iHeight      = tv_state.display.sdtv.height;
      m_desktopRes.iScreenWidth = tv_state.display.sdtv.width;
      m_desktopRes.iScreenHeight= tv_state.display.sdtv.height;
      m_desktopRes.dwFlags      = MAKEFLAGS(HDMI_RES_GROUP_INVALID, tv_state.display.sdtv.mode, 1);
      m_desktopRes.fRefreshRate = (float)tv_state.display.sdtv.frame_rate;
      m_desktopRes.fPixelRatio  = tv_state.display.hdmi.display_options.aspect == 0 ? 1.0f : get_display_aspect_ratio((SDTV_ASPECT_T)tv_state.display.sdtv.display_options.aspect) / ((float)m_desktopRes.iScreenWidth / (float)m_desktopRes.iScreenHeight);
    }
    else if ((tv_state.state & VC_LCD_ATTACHED_DEFAULT) != 0) // lcd
    {
      m_desktopRes.bFullScreen  = true;
      m_desktopRes.iWidth       = tv_state.display.sdtv.width;
      m_desktopRes.iHeight      = tv_state.display.sdtv.height;
      m_desktopRes.iScreenWidth = tv_state.display.sdtv.width;
      m_desktopRes.iScreenHeight= tv_state.display.sdtv.height;
      m_desktopRes.dwFlags      = MAKEFLAGS(HDMI_RES_GROUP_INVALID, 0, 0);
      m_desktopRes.fRefreshRate = (float)tv_state.display.sdtv.frame_rate;
      m_desktopRes.fPixelRatio  = tv_state.display.hdmi.display_options.aspect == 0 ? 1.0f : get_display_aspect_ratio((SDTV_ASPECT_T)tv_state.display.sdtv.display_options.aspect) / ((float)m_desktopRes.iScreenWidth / (float)m_desktopRes.iScreenHeight);
    }

    SetResolutionString(m_desktopRes);

    m_initDesktopRes = false;

    m_desktopRes.iSubtitles   = (int)(0.965 * m_desktopRes.iHeight);

    CLog::Log(LOGDEBUG, "EGL initial desktop resolution %s (%.2f)", m_desktopRes.strMode.c_str(), m_desktopRes.fPixelRatio);
  }

  if(GETFLAGS_GROUP(m_desktopRes.dwFlags) && GETFLAGS_MODE(m_desktopRes.dwFlags))
  {
    GetSupportedModes(HDMI_RES_GROUP_DMT, resolutions);
    GetSupportedModes(HDMI_RES_GROUP_CEA, resolutions);
  }
  {
    AddUniqueResolution(m_desktopRes, resolutions, true);
    CLog::Log(LOGDEBUG, "EGL probe resolution %s:%x", m_desktopRes.strMode.c_str(), m_desktopRes.dwFlags);
  }

  return true;
}

void CRPIUtils::DestroyDispmanxWindow()
{
  if(!m_DllBcmHost)
    return;

  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost->vc_dispmanx_update_start(0);

  if (m_dispman_element != DISPMANX_NO_HANDLE)
  {
    m_DllBcmHost->vc_dispmanx_element_remove(dispman_update, m_dispman_element);
    m_dispman_element = DISPMANX_NO_HANDLE;
  }
  m_DllBcmHost->vc_dispmanx_update_submit_sync(dispman_update);

  if (m_dispman_display != DISPMANX_NO_HANDLE)
  {
    g_RBP.CloseDisplay(m_dispman_display);
    m_dispman_display = DISPMANX_NO_HANDLE;
  }
}

void CRPIUtils::SetVisible(bool enable)
{
  if(!m_DllBcmHost || m_shown == enable)
    return;

  CLog::Log(LOGDEBUG, "CRPIUtils::EnableDispmanxWindow(%d)", enable);

  DISPMANX_UPDATE_HANDLE_T dispman_update = m_DllBcmHost->vc_dispmanx_update_start(0);

  if (m_dispman_element != DISPMANX_NO_HANDLE)
  {
    VC_RECT_T dst_rect;
    if (enable)
    {
      dst_rect.x      = 0;
      dst_rect.y      = 0;
      dst_rect.width  = m_screen_width;
      dst_rect.height = m_screen_height;
    }
    else
    {
      dst_rect.x      = m_screen_width;
      dst_rect.y      = m_screen_height;
      dst_rect.width  = m_screen_width;
      dst_rect.height = m_screen_height;
    }
    m_shown = enable;
    m_DllBcmHost->vc_dispmanx_element_change_attributes(dispman_update, m_dispman_element,
        (1<<2), 0, 0, &dst_rect, nullptr, 0, DISPMANX_NO_ROTATE);
  }
  m_DllBcmHost->vc_dispmanx_update_submit(dispman_update, nullptr, nullptr);
}

void CRPIUtils::GetSupportedModes(HDMI_RES_GROUP_T group, std::vector<RESOLUTION_INFO> &resolutions)
{
  if(!m_DllBcmHost)
    return;

  //Supported HDMI CEA/DMT resolutions, preferred resolution will be returned
  int32_t num_modes = 0;
  HDMI_RES_GROUP_T prefer_group;
  uint32_t prefer_mode;
  int i;
  TV_SUPPORTED_MODE_NEW_T *supported_modes = NULL;
  // query the number of modes first
  int max_supported_modes = m_DllBcmHost->vc_tv_hdmi_get_supported_modes_new(group, NULL, 0, &prefer_group, &prefer_mode);

  if (max_supported_modes > 0)
    supported_modes = new TV_SUPPORTED_MODE_NEW_T[max_supported_modes];

  if (supported_modes)
  {
    num_modes = m_DllBcmHost->vc_tv_hdmi_get_supported_modes_new(group,
        supported_modes, max_supported_modes, &prefer_group, &prefer_mode);

    CLog::Log(LOGDEBUG, "EGL get supported modes (%d) = %d, prefer_group=%x, prefer_mode=%x",
        group, num_modes, prefer_group, prefer_mode);
  }

  if (num_modes > 0 && prefer_group != HDMI_RES_GROUP_INVALID)
  {
    TV_SUPPORTED_MODE_NEW_T *tv = supported_modes;
    for (i=0; i < num_modes; i++, tv++)
    {
      RESOLUTION_INFO res;

      res.bFullScreen   = true;
      res.dwFlags       = MAKEFLAGS(group, tv->code, tv->scan_mode);
      res.fRefreshRate  = (float)tv->frame_rate;
      res.iWidth        = tv->width;
      res.iHeight       = tv->height;
      res.iScreenWidth  = tv->width;
      res.iScreenHeight = tv->height;
      res.fPixelRatio   = get_display_aspect_ratio((HDMI_ASPECT_T)tv->aspect_ratio) / ((float)res.iScreenWidth / (float)res.iScreenHeight);
      res.iSubtitles    = (int)(0.965 * res.iHeight);

      if (!m_desktopRes.dwFlags && prefer_group == group && prefer_mode == tv->code)
        m_desktopRes = res;

      AddUniqueResolution(res, resolutions);
      CLog::Log(LOGDEBUG, "EGL mode %d: %s (%.2f) %s%s:%x", i, res.strMode, res.fPixelRatio,
          tv->native ? "N" : "", tv->scan_mode ? "I" : "", int(tv->code));

      if (tv->frame_rate == 24 || tv->frame_rate == 30 || tv->frame_rate == 48 || tv->frame_rate == 60 || tv->frame_rate == 72)
      {
        RESOLUTION_INFO res2 = res;
        res2.fRefreshRate  = (float)tv->frame_rate * (1000.0f/1001.0f);
        AddUniqueResolution(res2, resolutions);
      }
    }
  }
  if (supported_modes)
    delete [] supported_modes;
}

void CRPIUtils::TvServiceCallback(uint32_t reason, uint32_t param1, uint32_t param2)
{
  CLog::Log(LOGDEBUG, "EGL tv_service_callback (%d,%d,%d)", reason, param1, param2);
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

void CRPIUtils::CallbackTvServiceCallback(void *userdata, uint32_t reason, uint32_t param1, uint32_t param2)
{
   CRPIUtils *callback = static_cast<CRPIUtils*>(userdata);
   callback->TvServiceCallback(reason, param1, param2);
}
