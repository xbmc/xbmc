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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
extern "C" {
  #include <ass/ass.h>
}
#include "DynamicDll.h"

#ifndef LIBASS_VERSION /* Legacy version. */
typedef struct ass_library_s ASS_Library;
typedef struct render_priv_s ASS_Renderer;
typedef ass_image_t ASS_Image;
typedef ass_track_t ASS_Track;
typedef ass_event_t ASS_Event;
#endif

class DllLibassInterface
{
public:
  virtual ~DllLibassInterface() {}
  virtual void ass_set_extract_fonts(ASS_Library* priv, int extract)=0;
  virtual void ass_set_fonts_dir(ASS_Library* priv, const char* fonts_dir)=0;
  virtual ASS_Library* ass_library_init(void)=0;
  virtual ASS_Renderer* ass_renderer_init(ASS_Library* library)=0;
  virtual void ass_set_frame_size(ASS_Renderer* priv, int w, int h)=0;
  virtual void ass_set_aspect_ratio(ASS_Renderer* priv, double dar, double sar)=0;
  virtual void ass_set_margins(ASS_Renderer* priv, int t, int b, int l, int r)=0;
  virtual void ass_set_use_margins(ASS_Renderer* priv, int use)=0;
  virtual void ass_set_line_position(ASS_Renderer* priv, double line_position)=0;
  virtual void ass_set_font_scale(ASS_Renderer* priv, double font_scale)=0;
  virtual ASS_Image* ass_render_frame(ASS_Renderer *priv, ASS_Track* track, long long now, int* detect_change)=0;
  virtual ASS_Track* ass_new_track(ASS_Library*)=0;
  virtual ASS_Track* ass_read_file(ASS_Library* library, char* fname, char* codepage)=0;
  virtual ASS_Track* ass_read_memory(ASS_Library* library, char* buf, size_t bufsize, char* codepage)=0;
  virtual void ass_free_track(ASS_Track* track)=0;
  virtual void ass_set_fonts(ASS_Renderer *priv, const char *default_font, const char *default_family, int fc, const char *config, int update) = 0;
  virtual void ass_set_style_overrides(ASS_Library* priv, char** list)=0;
  virtual void ass_library_done(ASS_Library* library)=0;
  virtual void ass_renderer_done(ASS_Renderer* renderer)=0;
  virtual void ass_process_chunk(ASS_Track* track, char *data, int size, long long timecode, long long duration)=0;
  virtual void ass_process_codec_private(ASS_Track* track, char *data, int size)=0;
  virtual void ass_set_message_cb(ASS_Library *priv
                                , void (*msg_cb)(int level, const char *fmt, va_list args, void *data)
                                , void *data)=0;
};

class DllLibass : public DllDynamic, DllLibassInterface
{
  DECLARE_DLL_WRAPPER(DllLibass, DLL_PATH_LIBASS)
  DEFINE_METHOD2(void, ass_set_extract_fonts, (ASS_Library * p1, int p2))
  DEFINE_METHOD2(void, ass_set_fonts_dir, (ASS_Library * p1, const char * p2))
  DEFINE_METHOD0(ASS_Library *, ass_library_init)
  DEFINE_METHOD1(ASS_Renderer *, ass_renderer_init, (ASS_Library * p1))
  DEFINE_METHOD3(void, ass_set_frame_size, (ASS_Renderer * p1, int p2, int p3))
  DEFINE_METHOD3(void, ass_set_aspect_ratio, (ASS_Renderer * p1, double p2, double p3))
  DEFINE_METHOD5(void, ass_set_margins, (ASS_Renderer * p1, int p2, int p3, int p4, int p5))
  DEFINE_METHOD2(void, ass_set_use_margins, (ASS_Renderer * p1, int p2))
  DEFINE_METHOD2(void, ass_set_line_position, (ASS_Renderer * p1, double p2))
  DEFINE_METHOD2(void, ass_set_font_scale, (ASS_Renderer * p1, double p2))
  DEFINE_METHOD4(ASS_Image *, ass_render_frame, (ASS_Renderer * p1, ASS_Track * p2, long long p3, int * p4))
  DEFINE_METHOD1(ASS_Track *, ass_new_track, (ASS_Library * p1))
  DEFINE_METHOD3(ASS_Track *, ass_read_file, (ASS_Library * p1, char * p2, char * p3))
  DEFINE_METHOD4(ASS_Track *, ass_read_memory, (ASS_Library * p1, char * p2, size_t p3, char * p4))
  DEFINE_METHOD1(void, ass_free_track, (ASS_Track * p1))
  DEFINE_METHOD6(void, ass_set_fonts, (ASS_Renderer* p1, const char* p2, const char* p3, int p4, const char* p5, int p6))
  DEFINE_METHOD2(void, ass_set_style_overrides, (ASS_Library* p1, char** p2))
  DEFINE_METHOD1(void, ass_library_done, (ASS_Library* p1))
  DEFINE_METHOD1(void, ass_renderer_done, (ASS_Renderer* p1))
  DEFINE_METHOD5(void, ass_process_chunk, (ASS_Track* p1, char* p2, int p3, long long p4, long long p5))
  DEFINE_METHOD3(void, ass_process_codec_private, (ASS_Track* p1, char* p2, int p3))
  DEFINE_METHOD3(void, ass_set_message_cb, (ASS_Library* p1, void (*p2)(int level, const char *fmt, va_list args, void *data), void* p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(ass_set_extract_fonts)
    RESOLVE_METHOD(ass_set_fonts_dir)
    RESOLVE_METHOD(ass_library_init)
    RESOLVE_METHOD(ass_renderer_init)
    RESOLVE_METHOD(ass_set_frame_size)
    RESOLVE_METHOD(ass_set_aspect_ratio)
    RESOLVE_METHOD(ass_set_margins)
    RESOLVE_METHOD(ass_set_use_margins)
    RESOLVE_METHOD(ass_set_line_position)
    RESOLVE_METHOD(ass_set_font_scale)
    RESOLVE_METHOD(ass_render_frame)
    RESOLVE_METHOD(ass_new_track)
    RESOLVE_METHOD(ass_read_file)
    RESOLVE_METHOD(ass_read_memory)
    RESOLVE_METHOD(ass_free_track)
    RESOLVE_METHOD(ass_set_fonts)
    RESOLVE_METHOD(ass_set_style_overrides)
    RESOLVE_METHOD(ass_library_done)
    RESOLVE_METHOD(ass_renderer_done)
    RESOLVE_METHOD(ass_process_chunk)
    RESOLVE_METHOD(ass_process_codec_private)
    RESOLVE_METHOD(ass_set_message_cb)
  END_METHOD_RESOLVE()
};
