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
#include "DllAvCodec.h"

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
  #if (defined HAVE_LIBAVFORMAT_AVFORMAT_H)
    #include <libavformat/avformat.h>
  #else
    #include <ffmpeg/avformat.h>
  #endif
  /* av_read_frame_flush() is defined for us in lib/xbmc-dll-symbols/DllAvFormat.c */
  void av_read_frame_flush(AVFormatContext *s);
#else
  #include "libavformat/avformat.h"
#endif
}

/* Flag introduced without a version bump */
#ifndef AVSEEK_FORCE
#define AVSEEK_FORCE 0x20000
#endif

typedef int64_t offset_t;

class DllAvFormatInterface
{
public:
  virtual ~DllAvFormatInterface() {}
  virtual void av_register_all_dont_call(void)=0;
  virtual AVInputFormat *av_find_input_format(const char *short_name)=0;
  virtual int url_feof(AVIOContext *s)=0;
  virtual void avformat_close_input(AVFormatContext **s)=0;
  virtual int av_read_frame(AVFormatContext *s, AVPacket *pkt)=0;
  virtual void av_read_frame_flush(AVFormatContext *s)=0;
  virtual int av_read_play(AVFormatContext *s)=0;
  virtual int av_read_pause(AVFormatContext *s)=0;
  virtual int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)=0;
#if (!defined USE_EXTERNAL_FFMPEG) && (!defined TARGET_DARWIN)
  virtual int avformat_find_stream_info_dont_call(AVFormatContext *ic, AVDictionary **options)=0;
#endif
  virtual int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options)=0;
  virtual AVIOContext *avio_alloc_context(unsigned char *buffer, int buffer_size, int write_flag, void *opaque,
                            int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                            int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                            offset_t (*seek)(void *opaque, offset_t offset, int whence))=0;
  virtual AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened)=0;
  virtual AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max)=0;
  virtual int av_probe_input_buffer(AVIOContext *pb, AVInputFormat **fmt, const char *filename, void *logctx, unsigned int offset, unsigned int max_probe_size)=0;
  virtual void av_dump_format(AVFormatContext *ic, int index, const char *url, int is_output)=0;
  virtual int avio_open(AVIOContext **s, const char *filename, int flags)=0;
  virtual int avio_close(AVIOContext *s)=0;
  virtual int avio_open_dyn_buf(AVIOContext **s)=0;
  virtual int avio_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer)=0;
  virtual offset_t avio_seek(AVIOContext *s, offset_t offset, int whence)=0;
  virtual int avio_read(AVIOContext *s, unsigned char *buf, int size)=0;
  virtual void avio_w8(AVIOContext *s, int b)=0;
  virtual void avio_write(AVIOContext *s, const unsigned char *buf, int size)=0;
  virtual void avio_wb24(AVIOContext *s, unsigned int val)=0;
  virtual void avio_wb32(AVIOContext *s, unsigned int val)=0;
  virtual void avio_wb16(AVIOContext *s, unsigned int val)=0;
  virtual AVFormatContext *avformat_alloc_context(void)=0;
  virtual AVStream *avformat_new_stream(AVFormatContext *s, AVCodec *c)=0;
  virtual AVOutputFormat *av_guess_format(const char *short_name, const char *filename, const char *mime_type)=0;
  virtual int avformat_write_header (AVFormatContext *s, AVDictionary **options)=0;
  virtual int av_write_trailer(AVFormatContext *s)=0;
  virtual int av_write_frame  (AVFormatContext *s, AVPacket *pkt)=0;
  virtual int av_find_default_stream_index(AVFormatContext *s)=0;
};

#if (defined USE_EXTERNAL_FFMPEG) || (defined TARGET_DARWIN) 

// Use direct mapping
class DllAvFormat : public DllDynamic, DllAvFormatInterface
{
public:
  virtual ~DllAvFormat() {}
  virtual void av_register_all() 
  { 
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::av_register_all();
  } 
  virtual void av_register_all_dont_call() { *(volatile int* )0x0 = 0; } 
  virtual AVInputFormat *av_find_input_format(const char *short_name) { return ::av_find_input_format(short_name); }
  virtual int url_feof(AVIOContext *s) { return ::url_feof(s); }
  virtual void avformat_close_input(AVFormatContext **s) { ::avformat_close_input(s); }
  virtual int av_read_frame(AVFormatContext *s, AVPacket *pkt) { return ::av_read_frame(s, pkt); }
  virtual void av_read_frame_flush(AVFormatContext *s) { ::av_read_frame_flush(s); }
  virtual int av_read_play(AVFormatContext *s) { return ::av_read_play(s); }
  virtual int av_read_pause(AVFormatContext *s) { return ::av_read_pause(s); }
  virtual int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags) { return ::av_seek_frame(s, stream_index, timestamp, flags); }
  virtual int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avformat_find_stream_info(ic, options);
  }
  virtual int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options)
  { return ::avformat_open_input(ps, filename, fmt, options); }
  virtual AVIOContext *avio_alloc_context(unsigned char *buffer, int buffer_size, int write_flag, void *opaque,
                            int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                            int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                            offset_t (*seek)(void *opaque, offset_t offset, int whence)) { return ::avio_alloc_context(buffer, buffer_size, write_flag, opaque, read_packet, write_packet, seek); }
  virtual AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened) {return ::av_probe_input_format(pd, is_opened); }
  virtual AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max) {*score_max = 100; return ::av_probe_input_format(pd, is_opened); } // Use av_probe_input_format, this is not exported by ffmpeg's headers
  virtual int av_probe_input_buffer(AVIOContext *pb, AVInputFormat **fmt, const char *filename, void *logctx, unsigned int offset, unsigned int max_probe_size) { return ::av_probe_input_buffer(pb, fmt, filename, logctx, offset, max_probe_size); }
  virtual void av_dump_format(AVFormatContext *ic, int index, const char *url, int is_output) { ::av_dump_format(ic, index, url, is_output); }
  virtual int avio_open(AVIOContext **s, const char *filename, int flags) { return ::avio_open(s, filename, flags); }
  virtual int avio_close(AVIOContext *s) { return ::avio_close(s); }
  virtual int avio_open_dyn_buf(AVIOContext **s) { return ::avio_open_dyn_buf(s); }
  virtual int avio_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer) { return ::avio_close_dyn_buf(s, pbuffer); }
  virtual offset_t avio_seek(AVIOContext *s, offset_t offset, int whence) { return ::avio_seek(s, offset, whence); }
  virtual int avio_read(AVIOContext *s, unsigned char *buf, int size) { return ::avio_read(s, buf, size); }
  virtual void avio_w8(AVIOContext *s, int b) { ::avio_w8(s, b); }
  virtual void avio_write(AVIOContext *s, const unsigned char *buf, int size) { ::avio_write(s, buf, size); }
  virtual void avio_wb24(AVIOContext *s, unsigned int val) { ::avio_wb24(s, val); }
  virtual void avio_wb32(AVIOContext *s, unsigned int val) { ::avio_wb32(s, val); }
  virtual void avio_wb16(AVIOContext *s, unsigned int val) { ::avio_wb16(s, val); }
  virtual AVFormatContext *avformat_alloc_context() { return ::avformat_alloc_context(); }
  virtual AVStream *avformat_new_stream(AVFormatContext *s, AVCodec *c) { return ::avformat_new_stream(s, c); }
  virtual AVOutputFormat *av_guess_format(const char *short_name, const char *filename, const char *mime_type) { return ::av_guess_format(short_name, filename, mime_type); }
  virtual int avformat_write_header (AVFormatContext *s, AVDictionary **options) { return ::avformat_write_header (s, options); }
  virtual int av_write_trailer(AVFormatContext *s) { return ::av_write_trailer(s); }
  virtual int av_write_frame  (AVFormatContext *s, AVPacket *pkt) { return ::av_write_frame(s, pkt); }
  virtual int av_find_default_stream_index(AVFormatContext *s) { return ::av_find_default_stream_index(s); }

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if !defined(TARGET_DARWIN)
    CLog::Log(LOGDEBUG, "DllAvFormat: Using libavformat system library");
#endif
    return true;
  }
  virtual void Unload() {}
};

#else

class DllAvFormat : public DllDynamic, DllAvFormatInterface
{
  DECLARE_DLL_WRAPPER(DllAvFormat, DLL_PATH_LIBAVFORMAT)

  LOAD_SYMBOLS()

  DEFINE_METHOD0(void, av_register_all_dont_call)
  DEFINE_METHOD1(AVInputFormat*, av_find_input_format, (const char *p1))
  DEFINE_METHOD1(int, url_feof, (AVIOContext *p1))
  DEFINE_METHOD1(void, avformat_close_input, (AVFormatContext **p1))
  DEFINE_METHOD1(int, av_read_play, (AVFormatContext *p1))
  DEFINE_METHOD1(int, av_read_pause, (AVFormatContext *p1))
  DEFINE_METHOD1(void, av_read_frame_flush, (AVFormatContext *p1))
  DEFINE_FUNC_ALIGNED2(int, __cdecl, av_read_frame, AVFormatContext *, AVPacket *)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, av_seek_frame, AVFormatContext*, int, int64_t, int)
  DEFINE_FUNC_ALIGNED2(int, __cdecl, avformat_find_stream_info_dont_call, AVFormatContext*, AVDictionary **)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avformat_open_input, AVFormatContext **, const char *, AVInputFormat *, AVDictionary **)
  DEFINE_FUNC_ALIGNED2(AVInputFormat*, __cdecl, av_probe_input_format, AVProbeData*, int)
  DEFINE_FUNC_ALIGNED3(AVInputFormat*, __cdecl, av_probe_input_format2, AVProbeData*, int, int*)
  DEFINE_FUNC_ALIGNED6(int, __cdecl, av_probe_input_buffer, AVIOContext *, AVInputFormat **, const char *, void *, unsigned int, unsigned int)
  DEFINE_FUNC_ALIGNED3(int, __cdecl, avio_read, AVIOContext*, unsigned char *, int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, avio_w8, AVIOContext*, int)
  DEFINE_FUNC_ALIGNED3(void, __cdecl, avio_write, AVIOContext*, const unsigned char *, int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, avio_wb24, AVIOContext*, unsigned int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, avio_wb32, AVIOContext*, unsigned int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, avio_wb16, AVIOContext*, unsigned int)
  DEFINE_METHOD7(AVIOContext *, avio_alloc_context, (unsigned char *p1, int p2, int p3, void *p4,
                  int (*p5)(void *opaque, uint8_t *buf, int buf_size),
                  int (*p6)(void *opaque, uint8_t *buf, int buf_size),
                  offset_t (*p7)(void *opaque, offset_t offset, int whence)))
  DEFINE_METHOD4(void, av_dump_format, (AVFormatContext *p1, int p2, const char *p3, int p4))
  DEFINE_METHOD3(int, avio_open, (AVIOContext **p1, const char *p2, int p3))
  DEFINE_METHOD1(int, avio_close, (AVIOContext *p1))
  DEFINE_METHOD1(int, avio_open_dyn_buf, (AVIOContext **p1))
  DEFINE_METHOD2(int, avio_close_dyn_buf, (AVIOContext *p1, uint8_t **p2))
  DEFINE_METHOD3(offset_t, avio_seek, (AVIOContext *p1, offset_t p2, int p3))
  DEFINE_METHOD0(AVFormatContext *, avformat_alloc_context)
  DEFINE_METHOD2(AVStream *, avformat_new_stream, (AVFormatContext *p1, AVCodec *p2))
  DEFINE_METHOD3(AVOutputFormat *, av_guess_format, (const char *p1, const char *p2, const char *p3))
  DEFINE_METHOD2(int, avformat_write_header , (AVFormatContext *p1, AVDictionary **p2))
  DEFINE_METHOD1(int, av_write_trailer, (AVFormatContext *p1))
  DEFINE_METHOD2(int, av_write_frame  , (AVFormatContext *p1, AVPacket *p2))
  DEFINE_METHOD1(int, av_find_default_stream_index, (AVFormatContext *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(av_register_all, av_register_all_dont_call)
    RESOLVE_METHOD(av_find_input_format)
    RESOLVE_METHOD(url_feof)
    RESOLVE_METHOD(avformat_close_input)
    RESOLVE_METHOD(av_read_frame)
    RESOLVE_METHOD(av_read_play)
    RESOLVE_METHOD(av_read_pause)
    RESOLVE_METHOD(av_read_frame_flush)
    RESOLVE_METHOD(av_seek_frame)
    RESOLVE_METHOD_RENAME(avformat_find_stream_info, avformat_find_stream_info_dont_call)
    RESOLVE_METHOD(avformat_open_input)
    RESOLVE_METHOD(avio_alloc_context)
    RESOLVE_METHOD(av_probe_input_format)
    RESOLVE_METHOD(av_probe_input_format2)
    RESOLVE_METHOD(av_probe_input_buffer)
    RESOLVE_METHOD(av_dump_format)
    RESOLVE_METHOD(avio_open)
    RESOLVE_METHOD(avio_close)
    RESOLVE_METHOD(avio_open_dyn_buf)
    RESOLVE_METHOD(avio_close_dyn_buf)
    RESOLVE_METHOD(avio_seek)
    RESOLVE_METHOD(avio_read)
    RESOLVE_METHOD(avio_w8)
    RESOLVE_METHOD(avio_write)
    RESOLVE_METHOD(avio_wb24)
    RESOLVE_METHOD(avio_wb32)
    RESOLVE_METHOD(avio_wb16)
    RESOLVE_METHOD(avformat_alloc_context)
    RESOLVE_METHOD(avformat_new_stream)
    RESOLVE_METHOD(av_guess_format)
    RESOLVE_METHOD(avformat_write_header)
    RESOLVE_METHOD(av_write_trailer)
    RESOLVE_METHOD(av_write_frame)
    RESOLVE_METHOD(av_find_default_stream_index)
  END_METHOD_RESOLVE()

  /* dependencies of libavformat */
  DllAvCodec m_dllAvCodec;
  // DllAvUtil loaded implicitely by m_dllAvCodec

public:
  void av_register_all()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    av_register_all_dont_call();
  }
  int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return avformat_find_stream_info_dont_call(ic, options);
  }

  virtual bool Load()
  {
    if (!m_dllAvCodec.Load())
      return false;
    return DllDynamic::Load();
  }
};

#endif
