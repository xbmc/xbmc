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

#include "DynamicDll.h"

#ifdef __cplusplus
extern "C"
{
#endif
  #include "../../../lib/libass/libass/ass.h"
#ifdef __cplusplus
}
#endif


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
  virtual int  ass_set_fonts(ass_renderer_t* priv, const char* default_font, const char* default_family)=0;
  virtual void ass_set_style_overrides(ass_library_t* priv, char** list)=0;
  virtual void ass_library_done(ass_library_t* library)=0;
  virtual void ass_renderer_done(ass_renderer_t* renderer)=0;
  virtual void ass_process_chunk(ass_track_t* track, char *data, int size, long long timecode, long long duration)=0;
  virtual void ass_process_codec_private(ass_track_t* track, char *data, int size)=0;
};

class DllLibass : public DllDynamic, DllLibassInterface
{
#ifdef __APPLE__
  DECLARE_DLL_WRAPPER(DllLibass, Q:\\system\\players\\dvdplayer\\libass-osx.so)
#elif !defined(_LINUX)
  DECLARE_DLL_WRAPPER(DllLibass, Q:\\system\\players\\dvdplayer\\libass.dll)
#else
  DECLARE_DLL_WRAPPER(DllLibass, Q:\\system\\players\\dvdplayer\\libass-i486-linux.so)
#endif
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
  DEFINE_METHOD3(int,  ass_set_fonts, (ass_renderer_t * p1, const char * p2, const char * p3))
  DEFINE_METHOD2(void, ass_set_style_overrides, (ass_library_t* p1, char** p2))
  DEFINE_METHOD1(void, ass_library_done, (ass_library_t* p1))
  DEFINE_METHOD1(void, ass_renderer_done, (ass_renderer_t* p1))
  DEFINE_METHOD5(void, ass_process_chunk, (ass_track_t* p1, char* p2, int p3, long long p4, long long p5))
  DEFINE_METHOD3(void, ass_process_codec_private, (ass_track_t* p1, char* p2, int p3))
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
  END_METHOD_RESOLVE()
};

