#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "DynamicDll.h"
#include "utils/log.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __GNUC__
#pragma warning(disable:4244)
#endif
#if (defined USE_EXTERNAL_FFMPEG)
  #if HAVE_LIBSWRESAMPLE_SWRESAMPLE_H
    #include <libswresample/swresample.h>
  #elif HAVE_LIBAVRESAMPLE_AVRESAMPLE_H
    #include <libavresample/avresample.h>
    #include <libavutil/opt.h>
    #include <libavutil/samplefmt.h>
    #define SwrContext AVAudioResampleContext
  #else
    #error "Either libswresample or libavresample is needed!"
  #endif
#else
  #include "libswresample/swresample.h"
#endif
}

class DllSwResampleInterface
{
public:
  virtual ~DllSwResampleInterface() {}
  virtual struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx)=0;
  virtual int swr_init(struct SwrContext *s)=0;
  virtual void swr_free(struct SwrContext **s)=0;
  virtual int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count)=0;
};

#if (defined USE_EXTERNAL_FFMPEG) || (defined TARGET_DARWIN) 

#if HAVE_LIBSWRESAMPLE_SWRESAMPLE_H || (defined TARGET_DARWIN)
// Use direct mapping
class DllSwResample : public DllDynamic, DllSwResampleInterface
{
public:
  virtual ~DllSwResample() {}

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if !defined(TARGET_DARWIN)
    CLog::Log(LOGDEBUG, "DllAvFormat: Using libswresample system library");
#endif
    return true;
  }
  virtual void Unload() {}
  virtual struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx) { return ::swr_alloc_set_opts(s, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, log_offset, log_ctx); }
  virtual int swr_init(struct SwrContext *s) { return ::swr_init(s); }
  virtual void swr_free(struct SwrContext **s){ return ::swr_free(s); }
  virtual int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count){ return ::swr_convert(s, out, out_count, in, in_count); }
};
#else
// Wrap the same API through libavresample.
class DllSwResample : public DllDynamic, DllSwResampleInterface
{
public:
  virtual ~DllSwResample() {}

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if !defined(TARGET_DARWIN)
    CLog::Log(LOGDEBUG, "DllAvFormat: Using libavresample system library");
#endif
    return true;
  }
  virtual void Unload() {}
  virtual struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx) {
          AVAudioResampleContext *ret = ::avresample_alloc_context();
          av_opt_set_int(ret, "out_channel_layout", out_ch_layout  , 0);
          av_opt_set_int(ret, "out_sample_fmt"    , out_sample_fmt , 0);
          av_opt_set_int(ret, "out_sample_rate"   , out_sample_rate, 0);
          av_opt_set_int(ret, "in_channel_layout" , in_ch_layout   , 0);
          av_opt_set_int(ret, "in_sample_fmt"     , in_sample_fmt  , 0);
          av_opt_set_int(ret, "in_sample_rate"    , in_sample_rate , 0);
          return ret;
  }
  virtual int swr_init(struct SwrContext *s) { return ::avresample_open(s); }
  virtual void swr_free(struct SwrContext **s){ ::avresample_close(*s); *s = NULL; }
  virtual int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count){ return ::avresample_convert(s, (void**)out, 0, out_count, (void**)in, 0,in_count); }
};
#endif

#else

class DllSwResample : public DllDynamic, DllSwResampleInterface
{
  DECLARE_DLL_WRAPPER(DllSwResample, DLL_PATH_LIBSWRESAMPLE)

  LOAD_SYMBOLS()

  DEFINE_METHOD9(SwrContext*, swr_alloc_set_opts, (struct SwrContext *p1, int64_t p2, enum AVSampleFormat p3, int p4, int64_t p5, enum AVSampleFormat p6, int p7, int p8, void * p9));
  DEFINE_METHOD1(int, swr_init, (struct SwrContext *p1))
  DEFINE_METHOD1(void, swr_free, (struct SwrContext **p1))
  DEFINE_METHOD5(int, swr_convert, (struct SwrContext *p1, uint8_t **p2, int p3, const uint8_t **p4, int p5))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(swr_alloc_set_opts)
    RESOLVE_METHOD(swr_init)
    RESOLVE_METHOD(swr_free)
    RESOLVE_METHOD(swr_convert)
  END_METHOD_RESOLVE()

  /* dependencies of libavformat */
  DllAvUtil m_dllAvUtil;

public:

  virtual bool Load()
  {
    if (!m_dllAvUtil.Load())
      return false;
    return DllDynamic::Load();
  }
};

#endif
