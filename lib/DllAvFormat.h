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
  virtual int url_feof(ByteIOContext *s)=0;
  virtual AVMetadataTag *av_metadata_get(AVMetadata *m, const char *key, const AVMetadataTag *prev, int flags)=0;
  virtual void av_close_input_file(AVFormatContext *s)=0;
  virtual void av_close_input_stream(AVFormatContext *s)=0;
  virtual int av_read_frame(AVFormatContext *s, AVPacket *pkt)=0;
  virtual void av_read_frame_flush(AVFormatContext *s)=0;
  virtual int av_read_play(AVFormatContext *s)=0;
  virtual int av_read_pause(AVFormatContext *s)=0;
  virtual int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)=0;
#if (!defined USE_EXTERNAL_FFMPEG)
  virtual int av_find_stream_info_dont_call(AVFormatContext *ic)=0;
#endif
  virtual int av_open_input_file(AVFormatContext **ic_ptr, const char *filename, AVInputFormat *fmt, int buf_size, AVFormatParameters *ap)=0;
  virtual void url_set_interrupt_cb(URLInterruptCB *interrupt_cb)=0;
  virtual int av_open_input_stream(AVFormatContext **ic_ptr, ByteIOContext *pb, const char *filename, AVInputFormat *fmt, AVFormatParameters *ap)=0;
  virtual int init_put_byte(ByteIOContext *s, unsigned char *buffer, int buffer_size, int write_flag, void *opaque, 
                            int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                            int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                            offset_t (*seek)(void *opaque, offset_t offset, int whence))=0;
  virtual AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened)=0;
  virtual AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max)=0;
  virtual int av_probe_input_buffer(ByteIOContext *pb, AVInputFormat **fmt, const char *filename, void *logctx, unsigned int offset, unsigned int max_probe_size)=0;
  virtual void dump_format(AVFormatContext *ic, int index, const char *url, int is_output)=0;
  virtual int url_fdopen(ByteIOContext **s, URLContext *h)=0;
  virtual int url_fopen(ByteIOContext **s, const char *filename, int flags)=0;
  virtual int url_fclose(ByteIOContext *s)=0;
  virtual int url_open_dyn_buf(ByteIOContext **s)=0;
  virtual int url_close_dyn_buf(ByteIOContext *s, uint8_t **pbuffer)=0;
  virtual offset_t url_fseek(ByteIOContext *s, offset_t offset, int whence)=0;
  virtual int get_buffer(ByteIOContext *s, unsigned char *buf, int size)=0;
  virtual int get_partial_buffer(ByteIOContext *s, unsigned char *buf, int size)=0;
  virtual void put_byte(ByteIOContext *s, int b)=0;
  virtual void put_buffer(ByteIOContext *s, const unsigned char *buf, int size)=0;
  virtual void put_be24(ByteIOContext *s, unsigned int val)=0;
  virtual void put_be32(ByteIOContext *s, unsigned int val)=0;
  virtual void put_be16(ByteIOContext *s, unsigned int val)=0;
  virtual AVFormatContext *avformat_alloc_context(void)=0;
  virtual AVStream *av_new_stream(AVFormatContext *s, int id)=0;
  virtual AVOutputFormat *av_guess_format(const char *short_name, const char *filename, const char *mime_type)=0;
  virtual int av_set_parameters(AVFormatContext *s, AVFormatParameters *ap)=0;
  virtual ByteIOContext *av_alloc_put_byte(unsigned char *buffer, int buffer_size, int write_flag, void *opaque,
                                           int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                                           int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                                           offset_t (*seek)(void *opaque, offset_t offset, int whence))=0;
  virtual int av_write_header (AVFormatContext *s)=0;
  virtual int av_write_trailer(AVFormatContext *s)=0;
  virtual int av_write_frame  (AVFormatContext *s, AVPacket *pkt)=0;
  virtual int av_metadata_set2(AVMetadata **pm, const char *key, const char *value, int flags)=0;
};

#if (defined USE_EXTERNAL_FFMPEG)

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
  virtual void av_register_all_dont_call() { *(int* )0x0 = 0; } 
  virtual AVInputFormat *av_find_input_format(const char *short_name) { return ::av_find_input_format(short_name); }
  virtual int url_feof(ByteIOContext *s) { return ::url_feof(s); }
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,31,0)
  // API added on: 2009-03-01
  virtual AVMetadataTag *av_metadata_get(AVMetadata *m, const char *key, const AVMetadataTag *prev, int flags){ return ::av_metadata_get(m, key, prev, flags); }
#else
  virtual AVMetadataTag *av_metadata_get(AVMetadata *m, const char *key, const AVMetadataTag *prev, int flags){ return NULL; }
#endif
  virtual void av_close_input_file(AVFormatContext *s) { ::av_close_input_file(s); }
  virtual void av_close_input_stream(AVFormatContext *s) { ::av_close_input_stream(s); }
  virtual int av_read_frame(AVFormatContext *s, AVPacket *pkt) { return ::av_read_frame(s, pkt); }
  virtual void av_read_frame_flush(AVFormatContext *s) { ::av_read_frame_flush(s); }
  virtual int av_read_play(AVFormatContext *s) { return ::av_read_play(s); }
  virtual int av_read_pause(AVFormatContext *s) { return ::av_read_pause(s); }
  virtual int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags) { return ::av_seek_frame(s, stream_index, timestamp, flags); }
  virtual int av_find_stream_info(AVFormatContext *ic)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::av_find_stream_info(ic);
  }
  virtual int av_open_input_file(AVFormatContext **ic_ptr, const char *filename, AVInputFormat *fmt, int buf_size, AVFormatParameters *ap) { return ::av_open_input_file(ic_ptr, filename, fmt, buf_size, ap); }
  virtual void url_set_interrupt_cb(URLInterruptCB *interrupt_cb) { ::url_set_interrupt_cb(interrupt_cb); }
  virtual int av_open_input_stream(AVFormatContext **ic_ptr, ByteIOContext *pb, const char *filename, AVInputFormat *fmt, AVFormatParameters *ap) { return ::av_open_input_stream(ic_ptr, pb, filename, fmt, ap); }
  virtual int init_put_byte(ByteIOContext *s, unsigned char *buffer, int buffer_size, int write_flag, void *opaque, 
                            int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                            int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                            offset_t (*seek)(void *opaque, offset_t offset, int whence)) { return ::init_put_byte(s, buffer, buffer_size, write_flag, opaque, read_packet, write_packet, seek); }
  virtual AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened) {return ::av_probe_input_format(pd, is_opened); }
  virtual AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max) {*score_max = 100; return ::av_probe_input_format(pd, is_opened); } // Use av_probe_input_format, this is not exported by ffmpeg's headers
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,98,0)
  // API added on: 2010-02-08
  virtual int av_probe_input_buffer(ByteIOContext *pb, AVInputFormat **fmt, const char *filename, void *logctx, unsigned int offset, unsigned int max_probe_size) { return ::av_probe_input_buffer(pb, fmt, filename, logctx, offset, max_probe_size); }
#else
  virtual int av_probe_input_buffer(ByteIOContext *pb, AVInputFormat **fmt, const char *filename, void *logctx, unsigned int offset, unsigned int max_probe_size) { return -1; }
#endif
  virtual void dump_format(AVFormatContext *ic, int index, const char *url, int is_output) { ::dump_format(ic, index, url, is_output); }
  virtual int url_fdopen(ByteIOContext **s, URLContext *h) { return ::url_fdopen(s, h); }
  virtual int url_fopen(ByteIOContext **s, const char *filename, int flags) { return ::url_fopen(s, filename, flags); }
  virtual int url_fclose(ByteIOContext *s) { return ::url_fclose(s); }
  virtual int url_open_dyn_buf(ByteIOContext **s) { return ::url_open_dyn_buf(s); }
  virtual int url_close_dyn_buf(ByteIOContext *s, uint8_t **pbuffer) { return ::url_close_dyn_buf(s, pbuffer); }
  virtual offset_t url_fseek(ByteIOContext *s, offset_t offset, int whence) { return ::url_fseek(s, offset, whence); }
  virtual int get_buffer(ByteIOContext *s, unsigned char *buf, int size) { return ::get_buffer(s, buf, size); }
  virtual int get_partial_buffer(ByteIOContext *s, unsigned char *buf, int size) { return ::get_partial_buffer(s, buf, size); }
  virtual void put_byte(ByteIOContext *s, int b) { ::put_byte(s, b); }
  virtual void put_buffer(ByteIOContext *s, const unsigned char *buf, int size) { ::put_buffer(s, buf, size); }
  virtual void put_be24(ByteIOContext *s, unsigned int val) { ::put_be24(s, val); }
  virtual void put_be32(ByteIOContext *s, unsigned int val) { ::put_be32(s, val); }
  virtual void put_be16(ByteIOContext *s, unsigned int val) { ::put_be16(s, val); }
  virtual AVFormatContext *avformat_alloc_context() { return ::avformat_alloc_context(); }
  virtual AVStream *av_new_stream(AVFormatContext *s, int id) { return ::av_new_stream(s, id); }
#if LIBAVFORMAT_VERSION_INT < (52<<16 | 45<<8)
  virtual AVOutputFormat *av_guess_format(const char *short_name, const char *filename, const char *mime_type) { return ::guess_format(short_name, filename, mime_type); }
#else
  virtual AVOutputFormat *av_guess_format(const char *short_name, const char *filename, const char *mime_type) { return ::av_guess_format(short_name, filename, mime_type); }
#endif
  virtual int av_set_parameters(AVFormatContext *s, AVFormatParameters *ap) { return ::av_set_parameters(s, ap); }
  virtual ByteIOContext *av_alloc_put_byte(unsigned char *buffer, int buffer_size, int write_flag, void *opaque,
                                           int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                                           int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                                           offset_t (*seek)(void *opaque, offset_t offset, int whence)) { return ::av_alloc_put_byte(buffer, buffer_size, write_flag, opaque, read_packet, write_packet, seek); }
  virtual int av_write_header (AVFormatContext *s) { return ::av_write_header (s); }
  virtual int av_write_trailer(AVFormatContext *s) { return ::av_write_trailer(s); }
  virtual int av_write_frame  (AVFormatContext *s, AVPacket *pkt) { return ::av_write_frame(s, pkt); }
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,43,0)
  // API added on: 2009-12-13
  virtual int av_metadata_set2(AVMetadata **pm, const char *key, const char *value, int flags) { return ::av_metadata_set2(pm, key, value, flags); }
#elif LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,31,0)
  // API added on: 2009-03-01
  virtual int av_metadata_set2(AVMetadata **pm, const char *key, const char *value, int flags) { return ::av_metadata_set(pm, key, value); }
#else
  virtual int av_metadata_set2(AVMetadata **pm, const char *key, const char *value, int flags) { return -1; }
#endif

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllAvFormat: Using libavformat system library");
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
  DEFINE_METHOD1(int, url_feof, (ByteIOContext *p1))
  DEFINE_METHOD4(AVMetadataTag*, av_metadata_get, (AVMetadata *p1, const char *p2, const AVMetadataTag *p3, int p4))
  DEFINE_METHOD1(void, av_close_input_file, (AVFormatContext *p1))
  DEFINE_METHOD1(void, av_close_input_stream, (AVFormatContext *p1))
  DEFINE_METHOD1(int, av_read_play, (AVFormatContext *p1))
  DEFINE_METHOD1(int, av_read_pause, (AVFormatContext *p1))
  DEFINE_METHOD1(void, av_read_frame_flush, (AVFormatContext *p1))
  DEFINE_FUNC_ALIGNED2(int, __cdecl, av_read_frame, AVFormatContext *, AVPacket *)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, av_seek_frame, AVFormatContext*, int, int64_t, int)
  DEFINE_FUNC_ALIGNED1(int, __cdecl, av_find_stream_info_dont_call, AVFormatContext*)
  DEFINE_FUNC_ALIGNED5(int, __cdecl, av_open_input_file, AVFormatContext**, const char *, AVInputFormat *, int, AVFormatParameters *)
  DEFINE_FUNC_ALIGNED5(int,__cdecl, av_open_input_stream, AVFormatContext **, ByteIOContext *, const char *, AVInputFormat *, AVFormatParameters *)
  DEFINE_FUNC_ALIGNED2(AVInputFormat*, __cdecl, av_probe_input_format, AVProbeData*, int)
  DEFINE_FUNC_ALIGNED3(AVInputFormat*, __cdecl, av_probe_input_format2, AVProbeData*, int, int*)
  DEFINE_FUNC_ALIGNED6(int, __cdecl, av_probe_input_buffer, ByteIOContext *, AVInputFormat **, const char *, void *, unsigned int, unsigned int)
  DEFINE_FUNC_ALIGNED3(int, __cdecl, get_buffer, ByteIOContext*, unsigned char *, int)
  DEFINE_FUNC_ALIGNED3(int, __cdecl, get_partial_buffer, ByteIOContext*, unsigned char *, int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, put_byte, ByteIOContext*, int)
  DEFINE_FUNC_ALIGNED3(void, __cdecl, put_buffer, ByteIOContext*, const unsigned char *, int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, put_be24, ByteIOContext*, unsigned int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, put_be32, ByteIOContext*, unsigned int)
  DEFINE_FUNC_ALIGNED2(void, __cdecl, put_be16, ByteIOContext*, unsigned int)
  DEFINE_METHOD1(void, url_set_interrupt_cb, (URLInterruptCB *p1))
  DEFINE_METHOD8(int, init_put_byte, (ByteIOContext *p1, unsigned char *p2, int p3, int p4, void *p5, 
                  int (*p6)(void *opaque, uint8_t *buf, int buf_size),
                  int (*p7)(void *opaque, uint8_t *buf, int buf_size),
                  offset_t (*p8)(void *opaque, offset_t offset, int whence)))
  DEFINE_METHOD4(void, dump_format, (AVFormatContext *p1, int p2, const char *p3, int p4))
  DEFINE_METHOD2(int, url_fdopen, (ByteIOContext **p1, URLContext *p2))
  DEFINE_METHOD3(int, url_fopen, (ByteIOContext **p1, const char *p2, int p3))
  DEFINE_METHOD1(int, url_fclose, (ByteIOContext *p1))
  DEFINE_METHOD1(int, url_open_dyn_buf, (ByteIOContext **p1))
  DEFINE_METHOD2(int, url_close_dyn_buf, (ByteIOContext *p1, uint8_t **p2))
  DEFINE_METHOD3(offset_t, url_fseek, (ByteIOContext *p1, offset_t p2, int p3))
  DEFINE_METHOD0(AVFormatContext *, avformat_alloc_context)
  DEFINE_METHOD2(AVStream *, av_new_stream, (AVFormatContext *p1, int p2))
#if LIBAVFORMAT_VERSION_INT < (52<<16 | 45<<8)
  DEFINE_METHOD3(AVOutputFormat *, guess_format, (const char *p1, const char *p2, const char *p3))
#else
  DEFINE_METHOD3(AVOutputFormat *, av_guess_format, (const char *p1, const char *p2, const char *p3))
#endif
  DEFINE_METHOD2(int, av_set_parameters, (AVFormatContext *p1, AVFormatParameters *p2));
  DEFINE_METHOD7(ByteIOContext *, av_alloc_put_byte, (unsigned char *p1, int p2, int p3, void *p4,
                  int(*p5)(void *opaque, uint8_t *buf, int buf_size),
                  int(*p6)(void *opaque, uint8_t *buf, int buf_size),
                  offset_t(*p7)(void *opaque, offset_t offset, int whence)))
  DEFINE_METHOD1(int, av_write_header , (AVFormatContext *p1))
  DEFINE_METHOD1(int, av_write_trailer, (AVFormatContext *p1))
  DEFINE_METHOD2(int, av_write_frame  , (AVFormatContext *p1, AVPacket *p2))
  DEFINE_METHOD4(int, av_metadata_set2, (AVMetadata **p1, const char *p2, const char *p3, int p4));
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(av_register_all, av_register_all_dont_call)
    RESOLVE_METHOD(av_find_input_format)
    RESOLVE_METHOD(url_feof)
    RESOLVE_METHOD(av_metadata_get)
    RESOLVE_METHOD(av_close_input_file)
    RESOLVE_METHOD(av_close_input_stream)
    RESOLVE_METHOD(av_read_frame)
    RESOLVE_METHOD(av_read_play)
    RESOLVE_METHOD(av_read_pause)
    RESOLVE_METHOD_RENAME(ff_read_frame_flush, av_read_frame_flush)
    RESOLVE_METHOD(av_seek_frame)
    RESOLVE_METHOD_RENAME(av_find_stream_info, av_find_stream_info_dont_call)
    RESOLVE_METHOD(av_open_input_file)
    RESOLVE_METHOD(url_set_interrupt_cb)
    RESOLVE_METHOD(av_open_input_stream)
    RESOLVE_METHOD(init_put_byte)
    RESOLVE_METHOD(av_probe_input_format)
    RESOLVE_METHOD(av_probe_input_format2)
    RESOLVE_METHOD(av_probe_input_buffer)
    RESOLVE_METHOD(dump_format)
    RESOLVE_METHOD(url_fdopen)
    RESOLVE_METHOD(url_fopen)
    RESOLVE_METHOD(url_fclose)
    RESOLVE_METHOD(url_open_dyn_buf)
    RESOLVE_METHOD(url_close_dyn_buf)
    RESOLVE_METHOD(url_fseek)
    RESOLVE_METHOD(get_buffer)
    RESOLVE_METHOD(get_partial_buffer)
    RESOLVE_METHOD(put_byte)
    RESOLVE_METHOD(put_buffer)
    RESOLVE_METHOD(put_be24)
    RESOLVE_METHOD(put_be32)
    RESOLVE_METHOD(put_be16)
    RESOLVE_METHOD(avformat_alloc_context)
    RESOLVE_METHOD(av_new_stream)
#if LIBAVFORMAT_VERSION_INT < (52<<16 | 45<<8)
    RESOLVE_METHOD(guess_format)
#else
    RESOLVE_METHOD(av_guess_format)
#endif
    RESOLVE_METHOD(av_set_parameters)
    RESOLVE_METHOD(av_alloc_put_byte)
    RESOLVE_METHOD(av_write_header)
    RESOLVE_METHOD(av_write_trailer)
    RESOLVE_METHOD(av_write_frame)
    RESOLVE_METHOD(av_metadata_set2)
  END_METHOD_RESOLVE()

  /* dependencies of libavformat */
  DllAvCodec m_dllAvCodec;
  // DllAvCore loaded implicitely by m_dllAvCodec
  // DllAvUtil loaded implicitely by m_dllAvCodec

public:
  void av_register_all()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    av_register_all_dont_call();
  }
  int av_find_stream_info(AVFormatContext *ic)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return(av_find_stream_info_dont_call(ic));
  }

  virtual bool Load()
  {
    if (!m_dllAvCodec.Load())
      return false;
    return DllDynamic::Load();
  }
};

#endif
