#pragma once

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

#include "DynamicDll.h"
#ifdef _WIN32
#include <lame.h>
#else
#include "lame/lame.h"
#endif

class DllLameEncInterface
{
public:
  virtual void id3tag_init(lame_global_flags* gfp)=0;
  virtual int id3tag_set_genre(lame_global_flags* gfp, const char* genre)=0;
  virtual void id3tag_set_title(lame_global_flags* gfp, const char* title)=0;
  virtual void id3tag_set_artist(lame_global_flags* gfp, const char* artist)=0;
  virtual void id3tag_set_album(lame_global_flags* gfp, const char* album)=0;
  virtual void id3tag_set_year(lame_global_flags* gfp, const char* year)=0;
  virtual void id3tag_set_comment(lame_global_flags* gfp, const char* comment)=0;
  virtual void id3tag_set_track(lame_global_flags* gfp, const char* track)=0;
  virtual int id3tag_set_textinfo_latin1(lame_global_flags* gfp, const char* field, const char* text)=0;
  virtual int id3tag_set_textinfo_utf16(lame_global_flags* gfp, const char* field, const unsigned short* text)=0;
  virtual int id3tag_set_textinfo_ucs2(lame_global_flags* gfp, const char* field, const unsigned short* text)=0;
  virtual lame_global_flags* lame_init()=0;
  virtual int lame_init_params(lame_global_flags *gfp)=0;
  virtual int lame_set_in_samplerate(lame_global_flags *gfp, int arg)=0;
  virtual int lame_set_brate(lame_global_flags *gfp, int arg)=0;
  virtual int lame_set_preset(lame_global_flags *gfp, int arg)=0;
  virtual int lame_set_asm_optimizations(lame_global_flags* gfp, int arg1, int arg2)=0;
  virtual int lame_encode_buffer_interleaved(lame_global_flags* gfp, short int pcm[], int num_samples, unsigned char* mp3buf, int mp3buf_size)=0;
  virtual int lame_close(lame_global_flags* gfp)=0;
  virtual void lame_mp3_tags_fid(lame_global_flags* gfp, FILE* fid)=0;
  virtual int lame_encode_flush(lame_global_flags* gfp, unsigned char* mp3buf, int size)=0;
  virtual ~DllLameEncInterface() {}
};

class DllLameEnc : public DllDynamic, DllLameEncInterface
{
  DECLARE_DLL_WRAPPER(DllLameEnc, DLL_PATH_LAME_ENC)
  DEFINE_METHOD1(void, id3tag_init, (lame_global_flags* p1));
  DEFINE_METHOD2(int, id3tag_set_genre, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD2(void, id3tag_set_title, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD2(void, id3tag_set_artist, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD2(void, id3tag_set_album, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD2(void, id3tag_set_year, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD2(void, id3tag_set_comment, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD2(void, id3tag_set_track, (lame_global_flags* p1, const char* p2))
  DEFINE_METHOD3(int, id3tag_set_textinfo_latin1, (lame_global_flags* p1, const char* p2, const char* p3))
  DEFINE_METHOD3(int, id3tag_set_textinfo_utf16, (lame_global_flags* p1, const char* p2, const unsigned short *p3))
  DEFINE_METHOD3(int, id3tag_set_textinfo_ucs2, (lame_global_flags* p1, const char* p2, const unsigned short *p3))
  DEFINE_METHOD0(lame_global_flags*, lame_init)
  DEFINE_METHOD1(int, lame_init_params, (lame_global_flags *p1))
  DEFINE_METHOD2(int, lame_set_in_samplerate, (lame_global_flags *p1, int p2))
  DEFINE_METHOD2(int, lame_set_brate, (lame_global_flags *p1, int p2))
  DEFINE_METHOD2(int, lame_set_preset, (lame_global_flags *p1, int p2))
  DEFINE_METHOD3(int, lame_set_asm_optimizations, (lame_global_flags* p1, int p2, int p3))
  DEFINE_METHOD5(int, lame_encode_buffer_interleaved, (lame_global_flags* p1, short int p2[], int p3, unsigned char* p4, int p5))
  DEFINE_METHOD1(int, lame_close, (lame_global_flags* p1))
  DEFINE_METHOD2(void, lame_mp3_tags_fid, (lame_global_flags* p1, FILE* p2))
  DEFINE_METHOD3(int, lame_encode_flush, (lame_global_flags* p1, unsigned char* p2, int p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(id3tag_init)
    RESOLVE_METHOD(id3tag_set_genre)
    RESOLVE_METHOD(id3tag_set_title)
    RESOLVE_METHOD(id3tag_set_artist)
    RESOLVE_METHOD(id3tag_set_album)
    RESOLVE_METHOD(id3tag_set_year)
    RESOLVE_METHOD(id3tag_set_comment)
    RESOLVE_METHOD(id3tag_set_track)
    RESOLVE_METHOD(id3tag_set_textinfo_latin1)
    RESOLVE_METHOD(id3tag_set_textinfo_utf16)
    RESOLVE_METHOD(id3tag_set_textinfo_ucs2)
    RESOLVE_METHOD(lame_init)
    RESOLVE_METHOD(lame_init_params)
    RESOLVE_METHOD(lame_set_in_samplerate)
    RESOLVE_METHOD(lame_set_brate)
    RESOLVE_METHOD(lame_set_preset)
    RESOLVE_METHOD(lame_set_asm_optimizations)
    RESOLVE_METHOD(lame_encode_buffer_interleaved)
    RESOLVE_METHOD(lame_close)
    RESOLVE_METHOD(lame_mp3_tags_fid)
    RESOLVE_METHOD(lame_encode_flush)
  END_METHOD_RESOLVE()
};
