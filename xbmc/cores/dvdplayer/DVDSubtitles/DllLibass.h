#pragma once

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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
extern "C" {
#if (defined USE_EXTERNAL_LIBASS)
  #include <ass/ass.h>
#else
  #include "../../../lib/libass/libass/ass.h"
#endif
}
#include "DynamicDll.h"
#include "utils/log.h"

class DllLibassInterface
{
public:
  virtual ~DllLibassInterface() {}
  virtual void ass_set_extract_fonts(ass_library_t* priv, int extract)=0;
  virtual void ass_set_fonts_dir(ass_library_t* priv, const char* fonts_dir)=0;
  virtual ass_library_t* ass_library_init(void)=0;
  virtual ass_renderer_t* ass_renderer_init(ass_library_t* library)=0;
  virtual void ass_set_frame_size(ass_renderer_t* priv, int w, int h)=0;
  virtual void ass_set_margins(ass_renderer_t* priv, int t, int b, int l, int r)=0;
  virtual void ass_set_use_margins(ass_renderer_t* priv, int use)=0;
  virtual void ass_set_font_scale(ass_renderer_t* priv, double font_scale)=0;
  virtual ass_image_t* ass_render_frame(ass_renderer_t *priv, ass_track_t* track, long long now, int* detect_change)=0;
  virtual ass_track_t* ass_new_track(ass_library_t*)=0;
  virtual ass_track_t* ass_read_file(ass_library_t* library, char* fname, char* codepage)=0;
  virtual void ass_free_track(ass_track_t* track)=0;
  virtual void ass_set_fonts(ass_renderer_t *priv, const char *default_font, const char *default_family, int fc, const char *config, int update) = 0;
  virtual void ass_set_style_overrides(ass_library_t* priv, char** list)=0;
  virtual void ass_library_done(ass_library_t* library)=0;
  virtual void ass_renderer_done(ass_renderer_t* renderer)=0;
  virtual void ass_process_chunk(ass_track_t* track, char *data, int size, long long timecode, long long duration)=0;
  virtual void ass_process_codec_private(ass_track_t* track, char *data, int size)=0;
  virtual void ass_set_message_cb(ass_library_t *priv
                                , void (*msg_cb)(int level, const char *fmt, va_list args, void *data)
                                , void *data)=0;
};

#if (defined USE_EXTERNAL_LIBASS)

class DllLibass : public DllDynamic, DllLibassInterface
{
public:
    virtual ~DllLibass() {}
    virtual void ass_set_extract_fonts(ass_library_t* priv, int extract)
        { return ::ass_set_extract_fonts(priv, extract); }
    virtual void ass_set_fonts_dir(ass_library_t* priv, const char* fonts_dir)
        { return ::ass_set_fonts_dir(priv, fonts_dir); }
    virtual ass_library_t* ass_library_init(void)
        { return ::ass_library_init(); }
    virtual ass_renderer_t* ass_renderer_init(ass_library_t* library)
        { return ::ass_renderer_init(library); }
    virtual void ass_set_frame_size(ass_renderer_t* priv, int w, int h)
        { return ::ass_set_frame_size(priv, w, h); }
    virtual void ass_set_margins(ass_renderer_t* priv, int t, int b, int l, int r)
        { return ::ass_set_margins(priv, t, b, l, r); }
    virtual void ass_set_use_margins(ass_renderer_t* priv, int use)
        { return ::ass_set_use_margins(priv, use); }
    virtual void ass_set_font_scale(ass_renderer_t* priv, double font_scale)
        { return ::ass_set_font_scale(priv, font_scale); }
    virtual ass_image_t* ass_render_frame(ass_renderer_t *priv, ass_track_t* track, long long now, int* detect_change)
        { return ::ass_render_frame(priv, track, now, detect_change); }
    virtual ass_track_t* ass_new_track(ass_library_t* library)
        { return ::ass_new_track(library); }
    virtual ass_track_t* ass_read_file(ass_library_t* library, char* fname, char* codepage)
        { return ::ass_read_file(library, fname, codepage); }
    virtual void ass_free_track(ass_track_t* track)
        { return ::ass_free_track(track); }
    virtual void ass_set_fonts(ass_renderer_t *priv, const char *default_font, const char *default_family, int fc, const char *config, int update)
        { return ::ass_set_fonts(priv, default_font, default_family, fc, config, update); }
    virtual void ass_set_style_overrides(ass_library_t* priv, char** list)
        { return ::ass_set_style_overrides(priv, list); }
    virtual void ass_library_done(ass_library_t* library)
        { return ::ass_library_done(library); }
    virtual void ass_renderer_done(ass_renderer_t* renderer)
        { return ::ass_renderer_done(renderer); }
    virtual void ass_process_chunk(ass_track_t* track, char *data, int size, long long timecode, long long duration)
        { return ::ass_process_chunk(track, data, size, timecode, duration); }
    virtual void ass_process_codec_private(ass_track_t* track, char *data, int size)
        { return ::ass_process_codec_private(track, data, size); }
    virtual void ass_set_message_cb(ass_library_t *priv
                                   , void (*msg_cb)(int level, const char *fmt, va_list args, void *data)
                                   , void *data)
        { return ::ass_set_message_cb(priv, msg_cb, data); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() {
        CLog::Log(LOGDEBUG, "DllLibass: Using libass system library");
        return true;
    }
    virtual void Unload() {}
};

#else

class DllLibass : public DllDynamic, DllLibassInterface
{
  DECLARE_DLL_WRAPPER(DllLibass, DLL_PATH_LIBASS)
  DEFINE_METHOD2(void, ass_set_extract_fonts, (ass_library_t * p1, int p2))
  DEFINE_METHOD2(void, ass_set_fonts_dir, (ass_library_t * p1, const char * p2))
  DEFINE_METHOD0(ass_library_t *, ass_library_init)
  DEFINE_METHOD1(ass_renderer_t *, ass_renderer_init, (ass_library_t * p1))
  DEFINE_METHOD3(void, ass_set_frame_size, (ass_renderer_t * p1, int p2, int p3))
  DEFINE_METHOD5(void, ass_set_margins, (ass_renderer_t * p1, int p2, int p3, int p4, int p5))
  DEFINE_METHOD2(void, ass_set_use_margins, (ass_renderer_t * p1, int p2))
  DEFINE_METHOD2(void, ass_set_font_scale, (ass_renderer_t * p1, double p2))
  DEFINE_METHOD4(ass_image_t *, ass_render_frame, (ass_renderer_t * p1, ass_track_t * p2, long long p3, int * p4))
  DEFINE_METHOD1(ass_track_t *, ass_new_track, (ass_library_t * p1))
  DEFINE_METHOD3(ass_track_t *, ass_read_file, (ass_library_t * p1, char * p2, char * p3))
  DEFINE_METHOD1(void, ass_free_track, (ass_track_t * p1))
  DEFINE_METHOD6(void, ass_set_fonts, (ass_renderer_t* p1, const char* p2, const char* p3, int p4, const char* p5, int p6))
  DEFINE_METHOD2(void, ass_set_style_overrides, (ass_library_t* p1, char** p2))
  DEFINE_METHOD1(void, ass_library_done, (ass_library_t* p1))
  DEFINE_METHOD1(void, ass_renderer_done, (ass_renderer_t* p1))
  DEFINE_METHOD5(void, ass_process_chunk, (ass_track_t* p1, char* p2, int p3, long long p4, long long p5))
  DEFINE_METHOD3(void, ass_process_codec_private, (ass_track_t* p1, char* p2, int p3))
  DEFINE_METHOD3(void, ass_set_message_cb, (ass_library_t* p1, void (*p2)(int level, const char *fmt, va_list args, void *data), void* p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(ass_set_extract_fonts)
    RESOLVE_METHOD(ass_set_fonts_dir)
    RESOLVE_METHOD(ass_library_init)
    RESOLVE_METHOD(ass_renderer_init)
    RESOLVE_METHOD(ass_set_frame_size)
    RESOLVE_METHOD(ass_set_margins)
    RESOLVE_METHOD(ass_set_use_margins)
    RESOLVE_METHOD(ass_set_font_scale)
    RESOLVE_METHOD(ass_render_frame)
    RESOLVE_METHOD(ass_new_track)
    RESOLVE_METHOD(ass_read_file)
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

#endif
