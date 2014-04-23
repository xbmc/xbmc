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
  #include <libavfilter/avfilter.h>
  #include <libavfilter/avfiltergraph.h>
  #include <libavfilter/buffersink.h>
#if LIBAVFILTER_VERSION_MICRO >= 100 && LIBAVFILTER_VERSION_INT < AV_VERSION_INT(3,43,100)
  #include <libavfilter/avcodec.h>
#endif
  #include <libavfilter/buffersrc.h>
#else
  #include "libavfilter/avfilter.h"
  #include "libavfilter/avfiltergraph.h"
  #include "libavfilter/buffersink.h"
#if LIBAVFILTER_VERSION_MICRO >= 100 && LIBAVFILTER_VERSION_INT < AV_VERSION_INT(3,43,100)
  #include "libavfilter/avcodec.h"
#endif
  #include "libavfilter/buffersrc.h"
#endif
}

#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(3,43,100)
#define LIBAVFILTER_AVFRAME_BASED
#endif

#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(3,78,100)
  #define HAVE_AVFILTER_GRAPH_PARSE_PTR
#endif

#include "threads/SingleLock.h"

class DllAvFilterInterface
{
public:
  virtual ~DllAvFilterInterface() {}
  virtual void avfilter_free(AVFilterContext *filter)=0;
  virtual void avfilter_graph_free(AVFilterGraph **graph)=0;
  virtual int avfilter_graph_create_filter(AVFilterContext **filt_ctx, AVFilter *filt, const char *name, const char *args, void *opaque, AVFilterGraph *graph_ctx)=0;
  virtual AVFilter *avfilter_get_by_name(const char *name)=0;
  virtual AVFilterGraph *avfilter_graph_alloc(void)=0;
  virtual AVFilterInOut *avfilter_inout_alloc()=0;
  virtual void avfilter_inout_free(AVFilterInOut **inout)=0;
#if defined(HAVE_AVFILTER_GRAPH_PARSE_PTR)
  virtual int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)=0;
#else
  virtual int avfilter_graph_parse(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)=0;
#endif
  virtual int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx)=0;
#if defined(LIBAVFILTER_AVFRAME_BASED)
  virtual int av_buffersrc_add_frame(AVFilterContext *buffer_filter, AVFrame *frame)=0;
#else
  virtual int av_buffersrc_add_frame(AVFilterContext *buffer_filter, AVFrame *frame, int flags)=0;
  virtual void avfilter_unref_buffer(AVFilterBufferRef *ref)=0;
#endif
  virtual int avfilter_link(AVFilterContext *src, unsigned srcpad, AVFilterContext *dst, unsigned dstpad)=0;
#if defined(LIBAVFILTER_AVFRAME_BASED)
  virtual int av_buffersink_get_frame(AVFilterContext *ctx, AVFrame *frame) = 0;
#else
  virtual int av_buffersink_get_buffer_ref(AVFilterContext *buffer_sink, AVFilterBufferRef **bufref, int flags)=0;
#endif
  virtual AVBufferSinkParams *av_buffersink_params_alloc()=0;
#if !defined(LIBAVFILTER_AVFRAME_BASED)
  virtual int av_buffersink_poll_frame(AVFilterContext *ctx)=0;
#endif
};

#if (defined USE_EXTERNAL_FFMPEG) || (defined TARGET_DARWIN) || (defined USE_STATIC_FFMPEG)
// Use direct mapping
class DllAvFilter : public DllDynamic, DllAvFilterInterface
{
public:
  virtual ~DllAvFilter() {}
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
#if defined(HAVE_AVFILTER_GRAPH_PARSE_PTR)
  virtual int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avfilter_graph_parse_ptr(graph, filters, inputs, outputs, log_ctx);
  }
#else
  virtual int avfilter_graph_parse(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avfilter_graph_parse(graph, filters, inputs, outputs, log_ctx);
  }
#endif
  virtual int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx)
  {
    return ::avfilter_graph_config(graphctx, log_ctx);
  }
#if defined(LIBAVFILTER_AVFRAME_BASED)
  virtual int av_buffersrc_add_frame(AVFilterContext *buffer_filter, AVFrame* frame) { return ::av_buffersrc_add_frame(buffer_filter, frame); }
#else
  virtual int av_buffersrc_add_frame(AVFilterContext *buffer_filter, AVFrame* frame, int flags) { return ::av_buffersrc_add_frame(buffer_filter, frame, flags); }
  virtual void avfilter_unref_buffer(AVFilterBufferRef *ref) { ::avfilter_unref_buffer(ref); }
#endif
  virtual int avfilter_link(AVFilterContext *src, unsigned srcpad, AVFilterContext *dst, unsigned dstpad) { return ::avfilter_link(src, srcpad, dst, dstpad); }
#if defined(LIBAVFILTER_AVFRAME_BASED)
  virtual int av_buffersink_get_frame(AVFilterContext *ctx, AVFrame *frame) { return ::av_buffersink_get_frame(ctx, frame); }
#else
  virtual int av_buffersink_get_buffer_ref(AVFilterContext *buffer_sink, AVFilterBufferRef **bufref, int flags) { return ::av_buffersink_get_buffer_ref(buffer_sink, bufref, flags); }
#endif
  virtual AVBufferSinkParams *av_buffersink_params_alloc() { return ::av_buffersink_params_alloc(); }
#if !defined(LIBAVFILTER_AVFRAME_BASED)
  virtual int av_buffersink_poll_frame(AVFilterContext *ctx) { return ::av_buffersink_poll_frame(ctx); }
#endif
  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if !defined(TARGET_DARWIN) && !defined(USE_STATIC_FFMPEG)
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

  DEFINE_METHOD1(void, avfilter_free_dont_call, (AVFilterContext *p1))
  DEFINE_METHOD1(void, avfilter_graph_free_dont_call, (AVFilterGraph **p1))
  DEFINE_METHOD0(void, avfilter_register_all_dont_call)
  DEFINE_METHOD6(int, avfilter_graph_create_filter, (AVFilterContext **p1, AVFilter *p2, const char *p3, const char *p4, void *p5, AVFilterGraph *p6))
  DEFINE_METHOD1(AVFilter*, avfilter_get_by_name, (const char *p1))
  DEFINE_METHOD0(AVFilterGraph*, avfilter_graph_alloc)
  DEFINE_METHOD0(AVFilterInOut*, avfilter_inout_alloc_dont_call)
  DEFINE_METHOD1(void, avfilter_inout_free_dont_call, (AVFilterInOut **p1))
#if defined(HAVE_AVFILTER_GRAPH_PARSE_PTR)
  DEFINE_FUNC_ALIGNED5(int, __cdecl, avfilter_graph_parse_ptr_dont_call, AVFilterGraph *, const char *, AVFilterInOut **, AVFilterInOut **, void *)
#else
  DEFINE_FUNC_ALIGNED5(int, __cdecl, avfilter_graph_parse_dont_call, AVFilterGraph *, const char *, AVFilterInOut **, AVFilterInOut **, void *)
#endif
  DEFINE_FUNC_ALIGNED2(int, __cdecl, avfilter_graph_config_dont_call, AVFilterGraph *, void *)
#if defined(LIBAVFILTER_AVFRAME_BASED)
  DEFINE_METHOD2(int, av_buffersrc_add_frame, (AVFilterContext *p1, AVFrame *p2))
#else
  DEFINE_METHOD3(int, av_buffersrc_add_frame, (AVFilterContext *p1, AVFrame *p2, int p3))
  DEFINE_METHOD1(void, avfilter_unref_buffer, (AVFilterBufferRef *p1))
#endif
  DEFINE_METHOD4(int, avfilter_link, (AVFilterContext *p1, unsigned p2, AVFilterContext *p3, unsigned p4))
#if defined(LIBAVFILTER_AVFRAME_BASED)
  DEFINE_FUNC_ALIGNED2(int                , __cdecl, av_buffersink_get_frame, AVFilterContext *, AVFrame *);
#else
  DEFINE_FUNC_ALIGNED3(int                , __cdecl, av_buffersink_get_buffer_ref, AVFilterContext *, AVFilterBufferRef **, int);
#endif
  DEFINE_FUNC_ALIGNED0(AVBufferSinkParams*, __cdecl, av_buffersink_params_alloc);
#if !defined(LIBAVFILTER_AVFRAME_BASED)
  DEFINE_FUNC_ALIGNED1(int                , __cdecl, av_buffersink_poll_frame, AVFilterContext *);
#endif

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(avfilter_free, avfilter_free_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_graph_free, avfilter_graph_free_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_register_all, avfilter_register_all_dont_call)
    RESOLVE_METHOD(avfilter_graph_create_filter)
    RESOLVE_METHOD(avfilter_get_by_name)
    RESOLVE_METHOD(avfilter_graph_alloc)
    RESOLVE_METHOD_RENAME(avfilter_inout_alloc, avfilter_inout_alloc_dont_call)
    RESOLVE_METHOD_RENAME(avfilter_inout_free, avfilter_inout_free_dont_call)
#if defined(HAVE_AVFILTER_GRAPH_PARSE_PTR)
    RESOLVE_METHOD_RENAME(avfilter_graph_parse_ptr, avfilter_graph_parse_ptr_dont_call)
#else
    RESOLVE_METHOD_RENAME(avfilter_graph_parse, avfilter_graph_parse_dont_call)
#endif
    RESOLVE_METHOD_RENAME(avfilter_graph_config, avfilter_graph_config_dont_call)
    RESOLVE_METHOD(av_buffersrc_add_frame)
#if !defined(LIBAVFILTER_AVFRAME_BASED)
    RESOLVE_METHOD(avfilter_unref_buffer)
#endif
    RESOLVE_METHOD(avfilter_link)
#if defined(LIBAVFILTER_AVFRAME_BASED)
    RESOLVE_METHOD(av_buffersink_get_frame)
#else
    RESOLVE_METHOD(av_buffersink_get_buffer_ref)
#endif
    RESOLVE_METHOD(av_buffersink_params_alloc)
#if !defined(LIBAVFILTER_AVFRAME_BASED)
    RESOLVE_METHOD(av_buffersink_poll_frame)
#endif
  END_METHOD_RESOLVE()

  /* dependencies of libavfilter */
  DllAvUtil m_dllAvUtil;
  DllSwResample m_dllSwResample;
  DllAvFormat m_dllAvFormat;

public:
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
#if defined(HAVE_AVFILTER_GRAPH_PARSE_PTR)
  int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avfilter_graph_parse_ptr_dont_call(graph, filters, inputs, outputs, log_ctx);
  }
#else
  int avfilter_graph_parse(AVFilterGraph *graph, const char *filters, AVFilterInOut **inputs, AVFilterInOut **outputs, void *log_ctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avfilter_graph_parse_dont_call(graph, filters, inputs, outputs, log_ctx);
  }
#endif
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
