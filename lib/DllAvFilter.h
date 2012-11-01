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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "DllAvCodec.h"
#include "DllAvFormat.h"
#include "DllSwResample.h"
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
  #if (defined HAVE_LIBAVFILTER_AVFILTER_H)
    #include <libavfilter/avfiltergraph.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/avcodec.h>
  #elif (defined HAVE_FFMPEG_AVFILTER_H)
    #include <ffmpeg/avfiltergraph.h>
    #include <ffmpeg/buffersink.h>
    #include <ffmpeg/avcodec.h>
  #endif
#else
  #include "libavfilter/avfiltergraph.h"
  #include "libavfilter/buffersink.h"
  #include "libavfilter/avcodec.h"
#endif
}

#include "threads/SingleLock.h"

class DllAvFilterInterface
{
public:
  virtual ~DllAvFilterInterface() {}
  virtual int avfilter_open(AVFilterContext **filter_ctx, AVFilter *filter, const char *inst_name)=0;
  virtual void avfilter_free(AVFilterContext *filter)=0;
  virtual void avfilter_graph_free(AVFilterGraph **graph)=0;
  virtual int avfilter_graph_create_filter(AVFilterContext **filt_ctx, AVFilter *filt, const char *name, const char *args, void *opaque, AVFilterGraph *graph_ctx)=0;
  virtual AVFilter *avfilter_get_by_name(const char *name)=0;
  virtual AVFilterGraph *avfilter_graph_alloc(void)=0;
  virtual AVFilterInOut *avfilter_inout_alloc()=0;
  virtual void avfilter_inout_free(AVFilterInOut **inout)=0;
  virtual int avfilter_graph_parse(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)=0;
  virtual int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx)=0;
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(3,0,0)
  virtual int av_vsrc_buffer_add_frame(AVFilterContext *buffer_filter, AVFrame *frame, int flags)=0;
#else
  virtual int av_buffersrc_add_frame(AVFilterContext *buffer_filter, AVFrame *frame, int flags)=0;
#endif
  virtual void avfilter_unref_buffer(AVFilterBufferRef *ref)=0;
  virtual int avfilter_link(AVFilterContext *src, unsigned srcpad, AVFilterContext *dst, unsigned dstpad)=0;
  virtual int av_buffersink_get_buffer_ref(AVFilterContext *buffer_sink, AVFilterBufferRef **bufref, int flags)=0;
  virtual AVBufferSinkParams *av_buffersink_params_alloc()=0;
  virtual int av_buffersink_poll_frame(AVFilterContext *ctx)=0;
};

#if (defined USE_EXTERNAL_FFMPEG) || (defined TARGET_DARWIN)
// Use direct mapping
class DllAvFilter : public DllDynamic, DllAvFilterInterface
{
public:
  virtual ~DllAvFilter() {}
  virtual int avfilter_open(AVFilterContext **filter_ctx, AVFilter *filter, const char *inst_name)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avfilter_open(filter_ctx, filter, inst_name);
  }
  virtual void avfilter_free(AVFilterContext *filter)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    ::avfilter_free(filter);
  }
  virtual void avfilter_graph_free(AVFilterGraph **graph)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    ::avfilter_graph_free(graph);
  }
  void avfilter_register_all()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    ::avfilter_register_all();
  }
  virtual int avfilter_graph_create_filter(AVFilterContext **filt_ctx, AVFilter *filt, const char *name, const char *args, void *opaque, AVFilterGraph *graph_ctx) { return ::avfilter_graph_create_filter(filt_ctx, filt, name, args, opaque, graph_ctx); }
  virtual AVFilter *avfilter_get_by_name(const char *name) { return ::avfilter_get_by_name(name); }
  virtual AVFilterGraph *avfilter_graph_alloc() { return ::avfilter_graph_alloc(); }
  virtual AVFilterInOut *avfilter_inout_alloc()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avfilter_inout_alloc();
  }
  virtual void avfilter_inout_free(AVFilterInOut **inout)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    ::avfilter_inout_free(inout);
  }
  virtual int avfilter_graph_parse(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avfilter_graph_parse(graph, filters, inputs, outputs, log_ctx);
  }
  virtual int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx)
  {
    return ::avfilter_graph_config(graphctx, log_ctx);
  }
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(3,0,0)
  virtual int av_vsrc_buffer_add_frame(AVFilterContext *buffer_filter, AVFrame *frame, int flags) { return ::av_vsrc_buffer_add_frame(buffer_filter, frame, flags); }
#else
  virtual int av_buffersrc_add_frame(AVFilterContext *buffer_filter, AVFrame* frame, int flags) { return ::av_buffersrc_add_frame(buffer_filter, frame, flags); }
#endif
  virtual void avfilter_unref_buffer(AVFilterBufferRef *ref) { ::avfilter_unref_buffer(ref); }
  virtual int avfilter_link(AVFilterContext *src, unsigned srcpad, AVFilterContext *dst, unsigned dstpad) { return ::avfilter_link(src, srcpad, dst, dstpad); }
  virtual int av_buffersink_get_buffer_ref(AVFilterContext *buffer_sink, AVFilterBufferRef **bufref, int flags) { return ::av_buffersink_get_buffer_ref(buffer_sink, bufref, flags); }
  virtual AVBufferSinkParams *av_buffersink_params_alloc() { return ::av_buffersink_params_alloc(); }
  virtual int av_buffersink_poll_frame(AVFilterContext *ctx) { return ::av_buffersink_poll_frame(ctx); }
  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if !defined(TARGET_DARWIN)
    CLog::Log(LOGDEBUG, "DllAvFilter: Using libavfilter system library");
#endif
    return true;
  }
  virtual void Unload() {}
};
#else
class DllAvFilter : public DllDynamic, DllAvFilterInterface
{
  DECLARE_DLL_WRAPPER(DllAvFilter, DLL_PATH_LIBAVFILTER)

  LOAD_SYMBOLS()

  DEFINE_METHOD3(int, avfilter_open_dont_call, (AVFilterContext **p1, AVFilter *p2, const char *p3))
  DEFINE_METHOD1(void, avfilter_free_dont_call, (AVFilterContext *p1))
  DEFINE_METHOD1(void, avfilter_graph_free_dont_call, (AVFilterGraph **p1))
  DEFINE_METHOD0(void, avfilter_register_all_dont_call)
  DEFINE_METHOD6(int, avfilter_graph_create_filter, (AVFilterContext **p1, AVFilter *p2, const char *p3, const char *p4, void *p5, AVFilterGraph *p6))
  DEFINE_METHOD1(AVFilter*, avfilter_get_by_name, (const char *p1))
  DEFINE_METHOD0(AVFilterGraph*, avfilter_graph_alloc)
  DEFINE_METHOD0(AVFilterInOut*, avfilter_inout_alloc_dont_call)
  DEFINE_METHOD1(void, avfilter_inout_free_dont_call, (AVFilterInOut **p1))
  DEFINE_FUNC_ALIGNED5(int, __cdecl, avfilter_graph_parse_dont_call, AVFilterGraph *, const char *, AVFilterInOut **, AVFilterInOut **, void *)
  DEFINE_FUNC_ALIGNED2(int, __cdecl, avfilter_graph_config_dont_call, AVFilterGraph *, void *)
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(3,0,0)
  DEFINE_METHOD3(int, av_vsrc_buffer_add_frame, (AVFilterContext *p1, AVFrame *p2, int p3))
#else
  DEFINE_METHOD3(int, av_buffersrc_add_frame, (AVFilterContext *p1, AVFrame *p2, int p3))
#endif
  DEFINE_METHOD1(void, avfilter_unref_buffer, (AVFilterBufferRef *p1))
  DEFINE_METHOD4(int, avfilter_link, (AVFilterContext *p1, unsigned p2, AVFilterContext *p3, unsigned p4))
  DEFINE_FUNC_ALIGNED3(int                , __cdecl, av_buffersink_get_buffer_ref, AVFilterContext *, AVFilterBufferRef **, int);
  DEFINE_FUNC_ALIGNED0(AVBufferSinkParams*, __cdecl, av_buffersink_params_alloc);
  DEFINE_FUNC_ALIGNED1(int                , __cdecl, av_buffersink_poll_frame, AVFilterContext *);

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(avfilter_open, avfilter_open_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_free, avfilter_free_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_graph_free, avfilter_graph_free_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_register_all, avfilter_register_all_dont_call)
    RESOLVE_METHOD(avfilter_graph_create_filter)
    RESOLVE_METHOD(avfilter_get_by_name)
    RESOLVE_METHOD(avfilter_graph_alloc)
    RESOLVE_METHOD_RENAME(avfilter_inout_alloc, avfilter_inout_alloc_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_inout_free, avfilter_inout_free_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_graph_parse, avfilter_graph_parse_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_graph_config, avfilter_graph_config_dont_call)
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(3,0,0)
    RESOLVE_METHOD(av_vsrc_buffer_add_frame)
#else
    RESOLVE_METHOD(av_buffersrc_add_frame)
#endif
    RESOLVE_METHOD(avfilter_unref_buffer)
    RESOLVE_METHOD(avfilter_link)
    RESOLVE_METHOD(av_buffersink_get_buffer_ref)
    RESOLVE_METHOD(av_buffersink_params_alloc)
    RESOLVE_METHOD(av_buffersink_poll_frame)
  END_METHOD_RESOLVE()

  /* dependencies of libavfilter */
  DllAvUtil m_dllAvUtil;
  DllSwResample m_dllSwResample;
  DllAvFormat m_dllAvFormat;

public:
  int avfilter_open(AVFilterContext **filter_ctx, AVFilter *filter, const char *inst_name)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avfilter_open_dont_call(filter_ctx, filter, inst_name);
  }
  void avfilter_free(AVFilterContext *filter)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    avfilter_free_dont_call(filter);
  }
  void avfilter_graph_free(AVFilterGraph **graph)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    avfilter_graph_free_dont_call(graph);
  }
  void avfilter_register_all()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    avfilter_register_all_dont_call();
  }
  AVFilterInOut* avfilter_inout_alloc()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avfilter_inout_alloc_dont_call();
  }
  int avfilter_graph_parse(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avfilter_graph_parse_dont_call(graph, filters, inputs, outputs, log_ctx);
  }
  void avfilter_inout_free(AVFilterInOut **inout)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    avfilter_inout_free_dont_call(inout);
  }
  int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avfilter_graph_config_dont_call(graphctx, log_ctx);
  }
  virtual bool Load()
  {
    if (!m_dllAvUtil.Load())
      return false;
    if (!m_dllSwResample.Load())
      return false;
    if (!m_dllAvFormat.Load())
      return false;
    return DllDynamic::Load();
  }
};
#endif
