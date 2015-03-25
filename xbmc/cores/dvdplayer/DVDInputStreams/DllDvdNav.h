#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

extern "C" {
#define DVDNAV_COMPILE
 #include <stdint.h>

 #include "dvdnav/dvdnav.h"

 #ifndef WIN32
 #define WIN32
 #endif // WIN32

 #ifndef HAVE_CONFIG_H
 #define HAVE_CONFIG_H
 #endif

 #include "dvdnav/dvdnav_internal.h"
 #include "dvdnav/vm.h"
 #include "dvdnav/dvd_types.h"

 #ifdef WIN32 // WIN32INCLUDES
 #undef HAVE_CONFIG_H
 #endif
}
#include "DynamicDll.h"

class DllDvdNavInterface
{
public:
  virtual ~DllDvdNavInterface() {}
  virtual dvdnav_status_t dvdnav_open(dvdnav_t **dest, const char *path)=0;
  virtual dvdnav_status_t dvdnav_close(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_reset(dvdnav_t *self)=0;
  virtual const char* dvdnav_err_to_string(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_set_readahead_flag(dvdnav_t *self, int32_t read_ahead_flag)=0;
  virtual dvdnav_status_t dvdnav_set_PGC_positioning_flag(dvdnav_t *self, int32_t pgc_based_flag)=0;
  virtual dvdnav_status_t dvdnav_get_next_cache_block(dvdnav_t *self, uint8_t **buf, int32_t *event, int32_t *len)=0;
  virtual dvdnav_status_t dvdnav_free_cache_block(dvdnav_t *self, unsigned char *buf)=0;
  virtual dvdnav_status_t dvdnav_still_skip(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_wait_skip(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_stop(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_button_select(dvdnav_t *self, pci_t *pci, int32_t button)=0;
  virtual dvdnav_status_t dvdnav_button_activate(dvdnav_t *self, pci_t *pci)=0;
  virtual dvdnav_status_t dvdnav_upper_button_select(dvdnav_t *self, pci_t *pci)=0;
  virtual dvdnav_status_t dvdnav_lower_button_select(dvdnav_t *self, pci_t *pci)=0;
  virtual dvdnav_status_t dvdnav_right_button_select(dvdnav_t *self, pci_t *pci)=0;
  virtual dvdnav_status_t dvdnav_left_button_select(dvdnav_t *self, pci_t *pci)=0;
  virtual dvdnav_status_t dvdnav_sector_search(dvdnav_t *self, uint64_t offset, int32_t origin)=0;
  virtual pci_t* dvdnav_get_current_nav_pci(dvdnav_t *self)=0;
  virtual dsi_t* dvdnav_get_current_nav_dsi(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_get_position(dvdnav_t *self, uint32_t *pos, uint32_t *len)=0;
  virtual dvdnav_status_t dvdnav_current_title_info(dvdnav_t *self, int32_t *title, int32_t *part)=0;
  virtual dvdnav_status_t dvdnav_spu_language_select(dvdnav_t *self, char *code)=0;
  virtual dvdnav_status_t dvdnav_audio_language_select(dvdnav_t *self, char *code)=0;
  virtual dvdnav_status_t dvdnav_menu_language_select(dvdnav_t *self, char *code)=0;
  virtual int8_t dvdnav_is_domain_vts(dvdnav_t *self)=0;
  virtual int8_t dvdnav_get_active_spu_stream(dvdnav_t *self)=0;
  virtual int8_t dvdnav_get_spu_logical_stream(dvdnav_t *self, uint8_t subp_num)=0;
  virtual uint16_t dvdnav_spu_stream_to_lang(dvdnav_t *self, uint8_t stream)=0;
  virtual dvdnav_status_t dvdnav_get_current_highlight(dvdnav_t *self, int32_t *button)=0;
  virtual dvdnav_status_t dvdnav_menu_call(dvdnav_t *self, DVDMenuID_t menu)=0;
  virtual dvdnav_status_t dvdnav_prev_pg_search(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_next_pg_search(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_get_highlight_area(pci_t *nav_pci , int32_t button, int32_t mode, dvdnav_highlight_area_t *highlight)=0;
  virtual dvdnav_status_t dvdnav_go_up(dvdnav_t *self)=0;
  virtual int8_t dvdnav_get_active_audio_stream(dvdnav_t *self)=0;
  virtual uint16_t dvdnav_audio_stream_to_lang(dvdnav_t *self, uint8_t stream)=0;
  virtual vm_t* dvdnav_get_vm(dvdnav_t *self)=0;
  virtual int dvdnav_get_button_info(dvdnav_t* self, int alpha[2][4], int color[2][4])=0;
  virtual int8_t dvdnav_get_audio_logical_stream(dvdnav_t *self, uint8_t audio_num)=0;
  virtual dvdnav_status_t dvdnav_set_region_mask(dvdnav_t *self, int32_t region_mask)=0;
  virtual uint8_t dvdnav_get_video_aspect(dvdnav_t *self)=0;
  virtual uint8_t dvdnav_get_video_scale_permission(dvdnav_t *self)=0;
  virtual dvdnav_status_t dvdnav_get_number_of_titles(dvdnav_t *self, int32_t *titles)=0;
  virtual dvdnav_status_t dvdnav_get_number_of_parts(dvdnav_t *self, int32_t title, int32_t *parts)=0;
  virtual dvdnav_status_t dvdnav_title_play(dvdnav_t *self, int32_t title)=0;
  virtual dvdnav_status_t dvdnav_part_play(dvdnav_t *self, int32_t title, int32_t part)=0;
  virtual dvdnav_status_t dvdnav_get_audio_attr(dvdnav_t * self, int32_t streamid, audio_attr_t* audio_attributes)=0;
  virtual dvdnav_status_t dvdnav_get_spu_attr(dvdnav_t * self, int32_t streamid, subp_attr_t* stitle_attributes)=0;
  virtual dvdnav_status_t dvdnav_time_search(dvdnav_t * self, uint64_t timepos)=0;
  virtual int64_t dvdnav_convert_time(dvd_time_t *time)=0;
  virtual dvdnav_status_t dvdnav_get_state(dvdnav_t *self, dvd_state_t *save_state)=0;
  virtual dvdnav_status_t dvdnav_set_state(dvdnav_t *self, dvd_state_t *save_state)=0;
  virtual dvdnav_status_t dvdnav_get_angle_info(dvdnav_t *self, int32_t *current_angle,int32_t *number_of_angles)=0;
  virtual dvdnav_status_t dvdnav_mouse_activate(dvdnav_t *self, pci_t *pci, int32_t x, int32_t y)=0;
  virtual dvdnav_status_t dvdnav_mouse_select(dvdnav_t *self, pci_t *pci, int32_t x, int32_t y)=0;
  virtual dvdnav_status_t dvdnav_get_title_string(dvdnav_t *self, const char **title_str)=0;
  virtual dvdnav_status_t dvdnav_get_serial_string(dvdnav_t *self, const char **serial_str)=0;
  virtual uint32_t dvdnav_describe_title_chapters(dvdnav_t* self, uint32_t title, uint64_t** times, uint64_t* duration)=0;
  virtual void dvdnav_free(void* pdata) = 0;
};

#if (defined USE_STATIC_LIBDVDNAV)
#error "Use of static libdvdnav is currently unsupported."

class DllDvdNav : public DllDynamic, DllDvdNavInterface
{
public:
    virtual ~DllDvdNav() {}
    virtual dvdnav_status_t dvdnav_open(dvdnav_t **dest, const char *path)
        { return ::dvdnav_open(dest, path); }
    virtual dvdnav_status_t dvdnav_close(dvdnav_t *self)
        { return ::dvdnav_close(self); }
    virtual dvdnav_status_t dvdnav_reset(dvdnav_t *self)
        { return ::dvdnav_reset(self); }
    virtual const char* dvdnav_err_to_string(dvdnav_t *self)
        { return ::dvdnav_err_to_string(self); }
    virtual dvdnav_status_t dvdnav_set_readahead_flag(dvdnav_t *self, int32_t read_ahead_flag)
        { return ::dvdnav_set_readahead_flag(self, read_ahead_flag); }
    virtual dvdnav_status_t dvdnav_set_PGC_positioning_flag(dvdnav_t *self, int32_t pgc_based_flag)
        { return ::dvdnav_set_PGC_positioning_flag(self, pgc_based_flag); }
    virtual dvdnav_status_t dvdnav_get_next_cache_block(dvdnav_t *self, uint8_t **buf, int32_t *event, int32_t *len)
        { return ::dvdnav_get_next_cache_block(self, buf, event, len); }
    virtual dvdnav_status_t dvdnav_free_cache_block(dvdnav_t *self, unsigned char *buf)
        { return ::dvdnav_free_cache_block(self, buf); }
    virtual dvdnav_status_t dvdnav_still_skip(dvdnav_t *self)
        { return ::dvdnav_still_skip(self); }
    virtual dvdnav_status_t dvdnav_wait_skip(dvdnav_t *self)
        { return ::dvdnav_wait_skip(self); }
    virtual dvdnav_status_t dvdnav_stop(dvdnav_t *self)
        { return ::dvdnav_stop(self); }
    virtual dvdnav_status_t dvdnav_button_select(dvdnav_t *self, pci_t *pci, int32_t button)
        { return ::dvdnav_button_select(self, pci, button); }
    virtual dvdnav_status_t dvdnav_button_activate(dvdnav_t *self, pci_t *pci)
        { return ::dvdnav_button_activate(self, pci); }
    virtual dvdnav_status_t dvdnav_upper_button_select(dvdnav_t *self, pci_t *pci)
        { return ::dvdnav_upper_button_select(self, pci); }
    virtual dvdnav_status_t dvdnav_lower_button_select(dvdnav_t *self, pci_t *pci)
        { return ::dvdnav_lower_button_select(self, pci); }
    virtual dvdnav_status_t dvdnav_right_button_select(dvdnav_t *self, pci_t *pci)
        { return ::dvdnav_right_button_select(self, pci); }
    virtual dvdnav_status_t dvdnav_left_button_select(dvdnav_t *self, pci_t *pci)
        { return ::dvdnav_left_button_select(self, pci); }
    virtual dvdnav_status_t dvdnav_sector_search(dvdnav_t *self, uint64_t offset, int32_t origin)
        { return ::dvdnav_sector_search(self, offset, origin); }
    virtual pci_t* dvdnav_get_current_nav_pci(dvdnav_t *self)
        { return ::dvdnav_get_current_nav_pci(self); }
    virtual dsi_t* dvdnav_get_current_nav_dsi(dvdnav_t *self)
        { return ::dvdnav_get_current_nav_dsi(self); }
    virtual dvdnav_status_t dvdnav_get_position(dvdnav_t *self, uint32_t *pos, uint32_t *len)
        { return ::dvdnav_get_position(self, pos, len); }
    virtual dvdnav_status_t dvdnav_current_title_info(dvdnav_t *self, int32_t *title, int32_t *part)
        { return ::dvdnav_current_title_info(self, title, part); }
    virtual dvdnav_status_t dvdnav_spu_language_select(dvdnav_t *self, char *code)
        { return ::dvdnav_spu_language_select(self, code); }
    virtual dvdnav_status_t dvdnav_audio_language_select(dvdnav_t *self, char *code)
        { return ::dvdnav_audio_language_select(self, code); }
    virtual dvdnav_status_t dvdnav_menu_language_select(dvdnav_t *self, char *code)
        { return ::dvdnav_menu_language_select(self, code); }
    virtual int8_t dvdnav_is_domain_vts(dvdnav_t *self)
        { return ::dvdnav_is_domain_vts(self); }
    virtual int8_t dvdnav_get_active_spu_stream(dvdnav_t *self)
        { return ::dvdnav_get_active_spu_stream(self); }
    virtual int8_t dvdnav_get_spu_logical_stream(dvdnav_t *self, uint8_t subp_num)
        { return ::dvdnav_get_spu_logical_stream(self, subp_num); }
    virtual uint16_t dvdnav_spu_stream_to_lang(dvdnav_t *self, uint8_t stream)
        { return ::dvdnav_spu_stream_to_lang(self, stream); }
    virtual dvdnav_status_t dvdnav_get_current_highlight(dvdnav_t *self, int32_t *button)
        { return ::dvdnav_get_current_highlight(self, button); }
    virtual dvdnav_status_t dvdnav_menu_call(dvdnav_t *self, DVDMenuID_t menu)
        { return ::dvdnav_menu_call(self, menu); }
    virtual dvdnav_status_t dvdnav_prev_pg_search(dvdnav_t *self)
        { return ::dvdnav_prev_pg_search(self); }
    virtual dvdnav_status_t dvdnav_next_pg_search(dvdnav_t *self)
        { return ::dvdnav_next_pg_search(self); }
    virtual dvdnav_status_t dvdnav_get_highlight_area(pci_t *nav_pci , int32_t button, int32_t mode, dvdnav_highlight_area_t *highlight)
        { return ::dvdnav_get_highlight_area(nav_pci, button, mode, highlight); }
    virtual dvdnav_status_t dvdnav_go_up(dvdnav_t *self)
        { return ::dvdnav_go_up(self); }
    virtual int8_t dvdnav_get_active_audio_stream(dvdnav_t *self)
        { return ::dvdnav_get_active_audio_stream(self); }
    virtual uint16_t dvdnav_audio_stream_to_lang(dvdnav_t *self, uint8_t stream)
        { return ::dvdnav_audio_stream_to_lang(self, stream); }
    virtual vm_t* dvdnav_get_vm(dvdnav_t *self)
        { return ::dvdnav_get_vm(self); }
    virtual int dvdnav_get_button_info(dvdnav_t* self, int alpha[2][4], int color[2][4])
        { return ::dvdnav_get_button_info(self, alpha, color); }
    virtual int8_t dvdnav_get_audio_logical_stream(dvdnav_t *self, uint8_t audio_num)
        { return ::dvdnav_get_audio_logical_stream(self, audio_num); }
    virtual dvdnav_status_t dvdnav_set_region_mask(dvdnav_t *self, int32_t region_mask)
        { return ::dvdnav_set_region_mask(self, region_mask); }
    virtual uint8_t dvdnav_get_video_aspect(dvdnav_t *self)
        { return ::dvdnav_get_video_aspect(self); }
    virtual uint8_t dvdnav_get_video_scale_permission(dvdnav_t *self)
        { return ::dvdnav_get_video_scale_permission(self); }
    virtual dvdnav_status_t dvdnav_get_number_of_titles(dvdnav_t *self, int32_t *titles)
        { return ::dvdnav_get_number_of_titles(self, titles); }
    virtual dvdnav_status_t dvdnav_get_number_of_parts(dvdnav_t *self, int32_t title, int32_t *parts)
        { return ::dvdnav_get_number_of_parts(self, title, parts); }
    virtual dvdnav_status_t dvdnav_title_play(dvdnav_t *self, int32_t title)
        { return ::dvdnav_title_play(self, title); }
    virtual dvdnav_status_t dvdnav_part_play(dvdnav_t *self, int32_t title, int32_t part)
        { return ::dvdnav_part_play(self, title, part); }
    virtual dvdnav_status_t dvdnav_get_audio_attr(dvdnav_t * self, int32_t streamid, audio_attr_t* audio_attributes)
        { return ::dvdnav_get_audio_attr(self, streamid, audio_attributes); }
    virtual dvdnav_status_t dvdnav_get_spu_attr(dvdnav_t * self, int32_t streamid, subp_attr_t* stitle_attributes)
        { return ::dvdnav_get_spu_attr(self, streamid, stitle_attributes); }
    virtual dvdnav_status_t dvdnav_time_search(dvdnav_t * self, uint64_t timepos)
        { return ::dvdnav_time_search(self, timepos); }
    virtual int64_t dvdnav_convert_time(dvd_time_t *time)
        { return ::dvdnav_convert_time(time); }
    virtual dvdnav_status_t dvdnav_get_state(dvdnav_t *self, dvd_state_t *save_state)
        { return ::dvdnav_get_state(self, save_state); }
    virtual dvdnav_status_t dvdnav_set_state(dvdnav_t *self, dvd_state_t *save_state)
        { return ::dvdnav_set_state(self, save_state); }
    virtual dvdnav_status_t dvdnav_get_angle_info(dvdnav_t *self, int32_t *current_angle,int32_t *number_of_angles)
        { return ::dvdnav_get_angle_info(self, current_angle, number_of_angles); }
    virtual dvdnav_status_t dvdnav_mouse_activate(dvdnav_t *self, pci_t *pci, int32_t x, int32_t y)
        { return ::dvdnav_mouse_activate(self, pci, x, y); }
    virtual dvdnav_status_t dvdnav_mouse_select(dvdnav_t *self, pci_t *pci, int32_t x, int32_t y)
        { return ::dvdnav_mouse_select(self, pci, x, y); }
    virtual dvdnav_status_t dvdnav_get_title_string(dvdnav_t *self, const char **title_str)
        { return ::dvdnav_get_title_string(self, title_str); }
    virtual dvdnav_status_t dvdnav_get_serial_string(dvdnav_t *self, const char **serial_str)
        { return ::dvdnav_get_serial_string(self, serial_str); }
    virtual uint32_t dvdnav_describe_title_chapters(dvdnav_t* self, uint32_t title, uint64_t** times, uint64_t* duration)
        { return ::dvdnav_describe_title_chapters(self, title, times, duration); }
    virtual void dvdnav_free(void* data)
        { return ::dvdnav_free(data); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() { return true; }
    virtual void Unload() {}
};

#else

class DllDvdNav : public DllDynamic, DllDvdNavInterface
{
  DECLARE_DLL_WRAPPER(DllDvdNav, DLL_PATH_LIBDVDNAV)

  DEFINE_METHOD2(dvdnav_status_t, dvdnav_open, (dvdnav_t **p1, const char *p2))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_close, (dvdnav_t *p1))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_reset, (dvdnav_t *p1))
  DEFINE_METHOD1(const char*, dvdnav_err_to_string, (dvdnav_t *p1))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_set_readahead_flag, (dvdnav_t *p1, int32_t p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_set_PGC_positioning_flag, (dvdnav_t *p1, int32_t p2))
  DEFINE_METHOD4(dvdnav_status_t, dvdnav_get_next_cache_block, (dvdnav_t *p1, uint8_t **p2, int32_t *p3, int32_t *p4))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_free_cache_block, (dvdnav_t *p1, unsigned char *p2))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_still_skip, (dvdnav_t *p1))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_wait_skip, (dvdnav_t *p1))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_stop, (dvdnav_t *p1))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_button_select, (dvdnav_t *p1, pci_t *p2, int32_t p3))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_button_activate,(dvdnav_t *p1, pci_t *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_upper_button_select, (dvdnav_t *p1, pci_t *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_lower_button_select, (dvdnav_t *p1, pci_t *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_right_button_select, (dvdnav_t *p1, pci_t *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_left_button_select, (dvdnav_t *p1, pci_t *p2))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_sector_search, (dvdnav_t *p1, uint64_t p2, int32_t p3))
  DEFINE_METHOD1(pci_t*, dvdnav_get_current_nav_pci, (dvdnav_t *p1))
  DEFINE_METHOD1(dsi_t*, dvdnav_get_current_nav_dsi, (dvdnav_t *p1))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_get_position, (dvdnav_t *p1, uint32_t *p2, uint32_t *p3))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_current_title_info, (dvdnav_t *p1, int32_t *p2, int32_t *p3))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_spu_language_select, (dvdnav_t *p1, char *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_audio_language_select, (dvdnav_t *p1, char *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_menu_language_select, (dvdnav_t *p1, char *p2))
  DEFINE_METHOD1(int8_t, dvdnav_is_domain_vts, (dvdnav_t *p1))
  DEFINE_METHOD1(int8_t, dvdnav_get_active_spu_stream, (dvdnav_t *p1))
  DEFINE_METHOD2(int8_t, dvdnav_get_spu_logical_stream, (dvdnav_t *p1, uint8_t p2))
  DEFINE_METHOD2(uint16_t, dvdnav_spu_stream_to_lang, (dvdnav_t *p1, uint8_t p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_get_current_highlight, (dvdnav_t *p1, int32_t *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_menu_call, (dvdnav_t *p1, DVDMenuID_t p2))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_prev_pg_search, (dvdnav_t *p1))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_next_pg_search, (dvdnav_t *p1))
  DEFINE_METHOD4(dvdnav_status_t, dvdnav_get_highlight_area, (pci_t *p1, int32_t p2, int32_t p3, dvdnav_highlight_area_t *p4))
  DEFINE_METHOD1(dvdnav_status_t, dvdnav_go_up, (dvdnav_t *p1))
  DEFINE_METHOD1(int8_t, dvdnav_get_active_audio_stream, (dvdnav_t *p1))
  DEFINE_METHOD2(uint16_t, dvdnav_audio_stream_to_lang, (dvdnav_t *p1, uint8_t p2))
  DEFINE_METHOD1(vm_t*, dvdnav_get_vm, (dvdnav_t *p1))
  DEFINE_METHOD3(int, dvdnav_get_button_info, (dvdnav_t* p1, int p2[2][4], int p3[2][4]))
  DEFINE_METHOD2(int8_t, dvdnav_get_audio_logical_stream, (dvdnav_t *p1, uint8_t p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_set_region_mask, (dvdnav_t *p1, int32_t p2))
  DEFINE_METHOD1(uint8_t, dvdnav_get_video_aspect, (dvdnav_t *p1))
  DEFINE_METHOD1(uint8_t, dvdnav_get_video_scale_permission, (dvdnav_t *p1))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_get_number_of_titles, (dvdnav_t *p1, int32_t *p2))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_get_number_of_parts, (dvdnav_t *p1, int32_t p2, int32_t *p3))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_title_play, (dvdnav_t *p1, int32_t p2))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_part_play, (dvdnav_t *p1, int32_t p2, int32_t p3))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_get_audio_attr, (dvdnav_t * p1, int32_t p2, audio_attr_t* p3))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_get_spu_attr, (dvdnav_t * p1, int32_t p2, subp_attr_t* p3))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_time_search, (dvdnav_t * p1, uint64_t p2))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_jump_to_sector_by_time, (dvdnav_t * p1, uint64_t p2, int32_t p3))
  DEFINE_METHOD1(int64_t, dvdnav_convert_time, (dvd_time_t *p1))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_get_state, (dvdnav_t *p1, dvd_state_t *p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_set_state, (dvdnav_t *p1, dvd_state_t *p2))
  DEFINE_METHOD3(dvdnav_status_t, dvdnav_get_angle_info, (dvdnav_t *p1, int32_t *p2,int32_t *p3))
  DEFINE_METHOD4(dvdnav_status_t, dvdnav_mouse_activate, (dvdnav_t *p1, pci_t *p2, int32_t p3, int32_t p4))
  DEFINE_METHOD4(dvdnav_status_t, dvdnav_mouse_select, (dvdnav_t *p1, pci_t *p2, int32_t p3, int32_t p4))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_get_title_string, (dvdnav_t *p1, const char **p2))
  DEFINE_METHOD2(dvdnav_status_t, dvdnav_get_serial_string, (dvdnav_t *p1, const char **p2))
  DEFINE_METHOD4(uint32_t, dvdnav_describe_title_chapters, (dvdnav_t* p1, uint32_t p2, uint64_t** p3, uint64_t* p4))
  DEFINE_METHOD1(void, dvdnav_free, (void *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(dvdnav_open)
    RESOLVE_METHOD(dvdnav_close)
    RESOLVE_METHOD(dvdnav_reset)
    RESOLVE_METHOD(dvdnav_err_to_string)
    RESOLVE_METHOD(dvdnav_set_readahead_flag)
    RESOLVE_METHOD(dvdnav_set_PGC_positioning_flag)
    RESOLVE_METHOD(dvdnav_get_next_cache_block)
    RESOLVE_METHOD(dvdnav_free_cache_block)
    RESOLVE_METHOD(dvdnav_still_skip)
    RESOLVE_METHOD(dvdnav_wait_skip)
    RESOLVE_METHOD(dvdnav_stop)
    RESOLVE_METHOD(dvdnav_button_select)
    RESOLVE_METHOD(dvdnav_button_activate)
    RESOLVE_METHOD(dvdnav_upper_button_select)
    RESOLVE_METHOD(dvdnav_lower_button_select)
    RESOLVE_METHOD(dvdnav_right_button_select)
    RESOLVE_METHOD(dvdnav_left_button_select)
    RESOLVE_METHOD(dvdnav_sector_search)
    RESOLVE_METHOD(dvdnav_get_current_nav_pci)
    RESOLVE_METHOD(dvdnav_get_current_nav_dsi)
    RESOLVE_METHOD(dvdnav_get_position)
    RESOLVE_METHOD(dvdnav_current_title_info)
    RESOLVE_METHOD(dvdnav_spu_language_select)
    RESOLVE_METHOD(dvdnav_audio_language_select)
    RESOLVE_METHOD(dvdnav_menu_language_select)
    RESOLVE_METHOD(dvdnav_is_domain_vts)
    RESOLVE_METHOD(dvdnav_get_active_spu_stream)
    RESOLVE_METHOD(dvdnav_get_spu_logical_stream)
    RESOLVE_METHOD(dvdnav_spu_stream_to_lang)
    RESOLVE_METHOD(dvdnav_get_current_highlight)
    RESOLVE_METHOD(dvdnav_menu_call)
    RESOLVE_METHOD(dvdnav_prev_pg_search)
    RESOLVE_METHOD(dvdnav_next_pg_search)
    RESOLVE_METHOD(dvdnav_get_highlight_area)
    RESOLVE_METHOD(dvdnav_go_up)
    RESOLVE_METHOD(dvdnav_get_active_audio_stream)
    RESOLVE_METHOD(dvdnav_audio_stream_to_lang)
    RESOLVE_METHOD(dvdnav_get_vm)
    RESOLVE_METHOD(dvdnav_get_button_info)
    RESOLVE_METHOD(dvdnav_get_audio_logical_stream)
    RESOLVE_METHOD(dvdnav_set_region_mask)
    RESOLVE_METHOD(dvdnav_get_video_aspect)
    RESOLVE_METHOD(dvdnav_get_video_scale_permission)
    RESOLVE_METHOD(dvdnav_get_number_of_titles)
    RESOLVE_METHOD(dvdnav_get_number_of_parts)
    RESOLVE_METHOD(dvdnav_title_play)
    RESOLVE_METHOD(dvdnav_part_play)
    RESOLVE_METHOD(dvdnav_get_audio_attr)
    RESOLVE_METHOD(dvdnav_get_spu_attr)
    RESOLVE_METHOD(dvdnav_time_search)
    RESOLVE_METHOD(dvdnav_jump_to_sector_by_time)
    RESOLVE_METHOD(dvdnav_convert_time)
    RESOLVE_METHOD(dvdnav_get_state)
    RESOLVE_METHOD(dvdnav_set_state)
    RESOLVE_METHOD(dvdnav_get_angle_info)
    RESOLVE_METHOD(dvdnav_mouse_activate)
    RESOLVE_METHOD(dvdnav_mouse_select)
    RESOLVE_METHOD(dvdnav_get_title_string)
    RESOLVE_METHOD(dvdnav_get_serial_string)
    RESOLVE_METHOD(dvdnav_describe_title_chapters)
    RESOLVE_METHOD(dvdnav_free)
END_METHOD_RESOLVE()
};

#endif
