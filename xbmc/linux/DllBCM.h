#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#if defined(TARGET_RASPBERRY_PI)

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable:4244)
#endif

extern "C" {
#include <bcm_host.h>
}

#include "DynamicDll.h"
#include "utils/log.h"

////////////////////////////////////////////////////////////////////////////////////////////

class DllBcmHostInterface
{
public:
  virtual ~DllBcmHostInterface() {}

  virtual void bcm_host_init() = 0;
  virtual void bcm_host_deinit() = 0;
  virtual int32_t graphics_get_display_size( const uint16_t display_number, uint32_t *width, uint32_t *height) = 0;
  virtual int vc_tv_hdmi_power_on_best(uint32_t width, uint32_t height, uint32_t frame_rate,
                                       HDMI_INTERLACED_T scan_mode, EDID_MODE_MATCH_FLAG_T match_flags) = 0;
  virtual int vc_tv_hdmi_power_on_best_3d(uint32_t width, uint32_t height, uint32_t frame_rate,
                                       HDMI_INTERLACED_T scan_mode, EDID_MODE_MATCH_FLAG_T match_flags) = 0;

  virtual int vc_tv_hdmi_get_supported_modes(HDMI_RES_GROUP_T group, TV_SUPPORTED_MODE_T *supported_modes,
                                             uint32_t max_supported_modes, HDMI_RES_GROUP_T *preferred_group,
                                             uint32_t *preferred_mode) = 0;
  virtual int vc_tv_hdmi_power_on_explicit(HDMI_MODE_T mode, HDMI_RES_GROUP_T group, uint32_t code) = 0;
  virtual int vc_tv_get_state(TV_GET_STATE_RESP_T *tvstate) = 0;
  virtual int vc_tv_show_info(uint32_t show) = 0;
  virtual int vc_gencmd(char *response, int maxlen, const char *string) = 0;
  virtual void vc_tv_register_callback(TVSERVICE_CALLBACK_T callback, void *callback_data) = 0;
  virtual void vc_tv_unregister_callback(TVSERVICE_CALLBACK_T callback) = 0;
  virtual void vc_cec_register_callback(CECSERVICE_CALLBACK_T callback, void *callback_data) = 0;
  //virtual void vc_cec_unregister_callback(CECSERVICE_CALLBACK_T callback) = 0;
  virtual DISPMANX_DISPLAY_HANDLE_T vc_dispmanx_display_open( uint32_t device ) = 0;
  virtual DISPMANX_UPDATE_HANDLE_T vc_dispmanx_update_start( int32_t priority ) = 0;
  virtual DISPMANX_ELEMENT_HANDLE_T vc_dispmanx_element_add ( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display,
                                                              int32_t layer, const VC_RECT_T *dest_rect, DISPMANX_RESOURCE_HANDLE_T src,
                                                              const VC_RECT_T *src_rect, DISPMANX_PROTECTION_T protection,
                                                              VC_DISPMANX_ALPHA_T *alpha,
                                                              DISPMANX_CLAMP_T *clamp, DISPMANX_TRANSFORM_T transform ) = 0;
  virtual int vc_dispmanx_update_submit_sync( DISPMANX_UPDATE_HANDLE_T update ) = 0;
  virtual int vc_dispmanx_element_remove( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element ) = 0;
  virtual int vc_dispmanx_display_close( DISPMANX_DISPLAY_HANDLE_T display ) = 0;
  virtual int vc_dispmanx_display_get_info( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_MODEINFO_T * pinfo ) = 0;
  virtual int vc_dispmanx_display_set_background( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display,
                                                  uint8_t red, uint8_t green, uint8_t blue ) = 0;
  virtual int vc_tv_hdmi_audio_supported(uint32_t audio_format, uint32_t num_channels,
                                                  EDID_AudioSampleRate fs, uint32_t bitrate) = 0;
};

#if (defined USE_EXTERNAL_LIBBCM_HOST)
class DllBcmHost : public DllDynamic, DllBcmHostInterface
{
public:
  virtual void bcm_host_init()
    { return ::bcm_host_init(); };
  virtual void bcm_host_deinit()
    { return ::bcm_host_deinit(); };
  virtual int32_t graphics_get_display_size( const uint16_t display_number, uint32_t *width, uint32_t *height)
    { return ::graphics_get_display_size(display_number, width, height); };
  virtual int vc_tv_hdmi_power_on_best(uint32_t width, uint32_t height, uint32_t frame_rate,
                                       HDMI_INTERLACED_T scan_mode, EDID_MODE_MATCH_FLAG_T match_flags)
    { return ::vc_tv_hdmi_power_on_best(width, height, frame_rate, scan_mode, match_flags); };
  virtual int vc_tv_hdmi_power_on_best_3d(uint32_t width, uint32_t height, uint32_t frame_rate,
                                       HDMI_INTERLACED_T scan_mode, EDID_MODE_MATCH_FLAG_T match_flags)
    { return ::vc_tv_hdmi_power_on_best_3d(width, height, frame_rate, scan_mode, match_flags); };
  virtual int vc_tv_hdmi_get_supported_modes(HDMI_RES_GROUP_T group, TV_SUPPORTED_MODE_T *supported_modes,
                                             uint32_t max_supported_modes, HDMI_RES_GROUP_T *preferred_group,
                                             uint32_t *preferred_mode)
    { return ::vc_tv_hdmi_get_supported_modes(group, supported_modes, max_supported_modes, preferred_group, preferred_mode); };
  virtual int vc_tv_hdmi_power_on_explicit(HDMI_MODE_T mode, HDMI_RES_GROUP_T group, uint32_t code)
    { return ::vc_tv_hdmi_power_on_explicit(mode, group, code); };
  virtual int vc_tv_get_state(TV_GET_STATE_RESP_T *tvstate)
    { return ::vc_tv_get_state(tvstate); };
  virtual int vc_tv_show_info(uint32_t show)
    { return ::vc_tv_show_info(show); };
  virtual int vc_gencmd(char *response, int maxlen, const char *string)
    { return ::vc_gencmd(response, maxlen, string); };
  virtual void vc_tv_register_callback(TVSERVICE_CALLBACK_T callback, void *callback_data)
    { ::vc_tv_register_callback(callback, callback_data); };
  virtual void vc_tv_unregister_callback(TVSERVICE_CALLBACK_T callback)
    { ::vc_tv_unregister_callback(callback); };
  virtual void vc_cec_register_callback(CECSERVICE_CALLBACK_T callback, void *callback_data)
    { ::vc_cec_register_callback(callback, callback_data); };
  //virtual void vc_cec_unregister_callback(CECSERVICE_CALLBACK_T callback)
  //  { ::vc_cec_unregister_callback(callback); };
  virtual DISPMANX_DISPLAY_HANDLE_T vc_dispmanx_display_open( uint32_t device )
     { return ::vc_dispmanx_display_open(device); };
  virtual DISPMANX_UPDATE_HANDLE_T vc_dispmanx_update_start( int32_t priority )
    { return ::vc_dispmanx_update_start(priority); };
  virtual DISPMANX_ELEMENT_HANDLE_T vc_dispmanx_element_add ( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display,
                                                                              int32_t layer, const VC_RECT_T *dest_rect, DISPMANX_RESOURCE_HANDLE_T src,
                                                                              const VC_RECT_T *src_rect, DISPMANX_PROTECTION_T protection,
                                                                              VC_DISPMANX_ALPHA_T *alpha,
                                                                              DISPMANX_CLAMP_T *clamp, DISPMANX_TRANSFORM_T transform )
    { return ::vc_dispmanx_element_add(update, display, layer, dest_rect, src, src_rect, protection, alpha, clamp, transform); };
  virtual int vc_dispmanx_update_submit_sync( DISPMANX_UPDATE_HANDLE_T update )
    { return ::vc_dispmanx_update_submit_sync(update); };
  virtual int vc_dispmanx_element_remove( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element )
    { return ::vc_dispmanx_element_remove(update, element); };
  virtual int vc_dispmanx_display_close( DISPMANX_DISPLAY_HANDLE_T display )
    { return ::vc_dispmanx_display_close(display); };
  virtual int vc_dispmanx_display_get_info( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_MODEINFO_T *pinfo )
    { return ::vc_dispmanx_display_get_info(display, pinfo); };
  virtual int vc_dispmanx_display_set_background( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display,
                                                  uint8_t red, uint8_t green, uint8_t blue )
    { return ::vc_dispmanx_display_set_background(update, display, red, green, blue); };
  virtual int vc_tv_hdmi_audio_supported(uint32_t audio_format, uint32_t num_channels,
                                                  EDID_AudioSampleRate fs, uint32_t bitrate)
  { return ::vc_tv_hdmi_audio_supported(audio_format, num_channels, fs, bitrate); };
  virtual bool ResolveExports() 
    { return true; }
  virtual bool Load() 
  {
    CLog::Log(LOGDEBUG, "DllBcm: Using omx system library");
    return true;
  }
  virtual void Unload() {}
};
#else
class DllBcmHost : public DllDynamic, DllBcmHostInterface
{
  DECLARE_DLL_WRAPPER(DllBcmHost, "/opt/vc/lib/libbcm_host.so")

  DEFINE_METHOD0(void,    bcm_host_init)
  DEFINE_METHOD0(void,    bcm_host_deinit)
  DEFINE_METHOD3(int32_t, graphics_get_display_size, (const uint16_t p1, uint32_t *p2, uint32_t *p3))
  DEFINE_METHOD5(int,     vc_tv_hdmi_power_on_best, (uint32_t p1, uint32_t p2, uint32_t p3,
                                                     HDMI_INTERLACED_T p4, EDID_MODE_MATCH_FLAG_T p5))
  DEFINE_METHOD5(int,     vc_tv_hdmi_power_on_best_3d, (uint32_t p1, uint32_t p2, uint32_t p3,
                                                     HDMI_INTERLACED_T p4, EDID_MODE_MATCH_FLAG_T p5))
  DEFINE_METHOD5(int,     vc_tv_hdmi_get_supported_modes, (HDMI_RES_GROUP_T p1, TV_SUPPORTED_MODE_T *p2,
                                                           uint32_t p3, HDMI_RES_GROUP_T *p4, uint32_t *p5))
  DEFINE_METHOD3(int,     vc_tv_hdmi_power_on_explicit, (HDMI_MODE_T p1, HDMI_RES_GROUP_T p2, uint32_t p3))
  DEFINE_METHOD1(int,     vc_tv_get_state, (TV_GET_STATE_RESP_T *p1))
  DEFINE_METHOD1(int,    vc_tv_show_info, (uint32_t p1))
  DEFINE_METHOD3(int,    vc_gencmd, (char *p1, int p2, const char *p3))

  DEFINE_METHOD2(void,    vc_tv_register_callback, (TVSERVICE_CALLBACK_T p1, void *p2))
  DEFINE_METHOD1(void,    vc_tv_unregister_callback, (TVSERVICE_CALLBACK_T p1))

  DEFINE_METHOD2(void,    vc_cec_register_callback, (CECSERVICE_CALLBACK_T p1, void *p2))
  //DEFINE_METHOD1(void,    vc_cec_unregister_callback, (CECSERVICE_CALLBACK_T p1))
  DEFINE_METHOD1(DISPMANX_DISPLAY_HANDLE_T, vc_dispmanx_display_open, (uint32_t p1 ))
  DEFINE_METHOD1(DISPMANX_UPDATE_HANDLE_T,  vc_dispmanx_update_start, (int32_t p1 ))
  DEFINE_METHOD10(DISPMANX_ELEMENT_HANDLE_T, vc_dispmanx_element_add, (DISPMANX_UPDATE_HANDLE_T p1, DISPMANX_DISPLAY_HANDLE_T p2,
                                                                       int32_t p3, const VC_RECT_T *p4, DISPMANX_RESOURCE_HANDLE_T p5,
                                                                       const VC_RECT_T *p6, DISPMANX_PROTECTION_T p7,
                                                                       VC_DISPMANX_ALPHA_T *p8,
                                                                       DISPMANX_CLAMP_T *p9, DISPMANX_TRANSFORM_T p10 ))
  DEFINE_METHOD1(int, vc_dispmanx_update_submit_sync, (DISPMANX_UPDATE_HANDLE_T p1))
  DEFINE_METHOD2(int, vc_dispmanx_element_remove, (DISPMANX_UPDATE_HANDLE_T p1, DISPMANX_ELEMENT_HANDLE_T p2))
  DEFINE_METHOD1(int, vc_dispmanx_display_close, (DISPMANX_DISPLAY_HANDLE_T p1))
  DEFINE_METHOD2(int, vc_dispmanx_display_get_info, (DISPMANX_DISPLAY_HANDLE_T p1, DISPMANX_MODEINFO_T *p2))
  DEFINE_METHOD5(int, vc_dispmanx_display_set_background, ( DISPMANX_UPDATE_HANDLE_T p1, DISPMANX_DISPLAY_HANDLE_T p2,
                                                            uint8_t p3, uint8_t p4, uint8_t p5 ))
  DEFINE_METHOD4(int, vc_tv_hdmi_audio_supported, (uint32_t p1, uint32_t p2, EDID_AudioSampleRate p3, uint32_t p4))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(bcm_host_init)
    RESOLVE_METHOD(bcm_host_deinit)
    RESOLVE_METHOD(graphics_get_display_size)
    RESOLVE_METHOD(vc_tv_hdmi_power_on_best)
    RESOLVE_METHOD(vc_tv_hdmi_power_on_best_3d)
    RESOLVE_METHOD(vc_tv_hdmi_get_supported_modes)
    RESOLVE_METHOD(vc_tv_hdmi_power_on_explicit)
    RESOLVE_METHOD(vc_tv_get_state)
    RESOLVE_METHOD(vc_tv_show_info)
    RESOLVE_METHOD(vc_gencmd)
    RESOLVE_METHOD(vc_tv_register_callback)
    RESOLVE_METHOD(vc_tv_unregister_callback)
    RESOLVE_METHOD(vc_cec_register_callback)
    //RESOLVE_METHOD(vc_cec_unregister_callback)
    RESOLVE_METHOD(vc_dispmanx_display_open)
    RESOLVE_METHOD(vc_dispmanx_update_start)
    RESOLVE_METHOD(vc_dispmanx_element_add)
    RESOLVE_METHOD(vc_dispmanx_update_submit_sync)
    RESOLVE_METHOD(vc_dispmanx_element_remove)
    RESOLVE_METHOD(vc_dispmanx_display_close)
    RESOLVE_METHOD(vc_dispmanx_display_get_info)
    RESOLVE_METHOD(vc_dispmanx_display_set_background)
    RESOLVE_METHOD(vc_tv_hdmi_audio_supported)
  END_METHOD_RESOLVE()

public:
  virtual bool Load()
  {
    return DllDynamic::Load();
  }
};
#endif

#endif
