#pragma once
#include "../../DynamicDll.h"

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
#include "avformat.h"
}


class DllAvFormatInterface
{
public:
  virtual ~DllAvFormatInterface() {}
  virtual void av_register_all(void)=0;
  virtual AVInputFormat *av_find_input_format(const char *short_name)=0;
  virtual int url_feof(ByteIOContext *s)=0;
  virtual void av_close_input_file(AVFormatContext *s)=0;
  virtual int av_read_frame(AVFormatContext *s, AVPacket *pkt)=0;
  virtual int av_read_play(AVFormatContext *s)=0;
  virtual int av_read_pause(AVFormatContext *s)=0;
  virtual int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)=0;
  virtual int av_find_stream_info(AVFormatContext *ic)=0;
  virtual int av_open_input_file(AVFormatContext **ic_ptr, const char *filename, AVInputFormat *fmt, int buf_size, AVFormatParameters *ap)=0;
  virtual void url_set_interrupt_cb(URLInterruptCB *interrupt_cb)=0;
  virtual int av_dup_packet(AVPacket *pkt)=0;
  virtual int av_open_input_stream(AVFormatContext **ic_ptr, ByteIOContext *pb, const char *filename, AVInputFormat *fmt, AVFormatParameters *ap)=0;
  virtual int init_put_byte(ByteIOContext *s, unsigned char *buffer, int buffer_size, int write_flag, void *opaque, 
                            int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                            int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                            offset_t (*seek)(void *opaque, offset_t offset, int whence))=0;
  virtual AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened)=0;
  virtual void dump_format(AVFormatContext *ic, int index, const char *url, int is_output)=0;
  virtual void av_destruct_packet_nofree(AVPacket *pkt)=0;
  virtual int url_fdopen(ByteIOContext **s, URLContext *h)=0;
  virtual int url_fopen(ByteIOContext **s, const char *filename, int flags)=0;
  virtual int url_fclose(ByteIOContext *s)=0;
  virtual offset_t url_fseek(ByteIOContext *s, offset_t offset, int whence)=0;
  virtual void av_read_frame_flush(AVFormatContext *s)=0;
  virtual int get_buffer(ByteIOContext *s, unsigned char *buf, int size)=0;
};

class DllAvFormat : public DllDynamic, DllAvFormatInterface
{

#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllAvFormat, Q:\\system\\players\\dvdplayer\\avformat-51.dll)
#else
  DECLARE_DLL_WRAPPER(DllAvFormat, Q:\\system\\players\\dvdplayer\\avformat-51-i486-linux.so)
#endif

  LOAD_SYMBOLS()

  DEFINE_METHOD0(void, av_register_all)
  DEFINE_METHOD1(AVInputFormat*, av_find_input_format, (const char *p1))
  DEFINE_METHOD1(int, url_feof, (ByteIOContext *p1))
  DEFINE_METHOD1(void, av_close_input_file, (AVFormatContext *p1))
  DEFINE_METHOD1(int, av_read_play, (AVFormatContext *p1))
  DEFINE_METHOD1(int, av_read_pause, (AVFormatContext *p1))
#ifndef _LINUX
  DEFINE_FUNC_ALIGNED2(int, __cdecl, av_read_frame, AVFormatContext *, AVPacket *)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, av_seek_frame, AVFormatContext*, int, int64_t, int)
  DEFINE_FUNC_ALIGNED1(int, __cdecl, av_find_stream_info, AVFormatContext*)
  DEFINE_FUNC_ALIGNED5(int, __cdecl, av_open_input_file, AVFormatContext**, const char *, AVInputFormat *, int, AVFormatParameters *)
  DEFINE_FUNC_ALIGNED5(int,__cdecl, av_open_input_stream, AVFormatContext **, ByteIOContext *, const char *, AVInputFormat *, AVFormatParameters *)
  DEFINE_FUNC_ALIGNED2(AVInputFormat*, __cdecl, av_probe_input_format, AVProbeData*, int)
  DEFINE_FUNC_ALIGNED1(void, __cdecl, av_read_frame_flush, AVFormatContext*)
  DEFINE_FUNC_ALIGNED3(int, __cdecl, get_buffer, ByteIOContext*, unsigned char *, int)
#else
  DEFINE_METHOD2(int, av_read_frame, (AVFormatContext *p1, AVPacket *p2))
  DEFINE_METHOD4(int, av_seek_frame, (AVFormatContext *p1, int p2, int64_t p3, int p4))
  DEFINE_METHOD1(int, av_find_stream_info, (AVFormatContext *p1))
  DEFINE_METHOD5(int, av_open_input_file, (AVFormatContext **p1, const char *p2, AVInputFormat *p3, int p4, AVFormatParameters *p5))
  DEFINE_METHOD5(int, av_open_input_stream, (AVFormatContext **p1, ByteIOContext *p2, const char *p3, AVInputFormat *p4, AVFormatParameters *p5))
  DEFINE_METHOD2(AVInputFormat*, av_probe_input_format, (AVProbeData* p1 , int p2))
  DEFINE_METHOD1(void, av_read_frame_flush, (AVFormatContext* p1))
  DEFINE_METHOD3(int, get_buffer, (ByteIOContext* p1, unsigned char *p2, int p3))
#endif
  DEFINE_METHOD1(void, url_set_interrupt_cb, (URLInterruptCB *p1))
  DEFINE_METHOD1(int, av_dup_packet, (AVPacket *p1))
  DEFINE_METHOD8(int, init_put_byte, (ByteIOContext *p1, unsigned char *p2, int p3, int p4, void *p5, 
                  int (*p6)(void *opaque, uint8_t *buf, int buf_size),
                  int (*p7)(void *opaque, uint8_t *buf, int buf_size),
                  offset_t (*p8)(void *opaque, offset_t offset, int whence)))
  DEFINE_METHOD4(void, dump_format, (AVFormatContext *p1, int p2, const char *p3, int p4))
  DEFINE_METHOD1(void, av_destruct_packet_nofree, (AVPacket *p1))
  DEFINE_METHOD2(int, url_fdopen, (ByteIOContext **p1, URLContext *p2))
  DEFINE_METHOD3(int, url_fopen, (ByteIOContext **p1, const char *p2, int p3))
  DEFINE_METHOD1(int, url_fclose, (ByteIOContext *p1))
  DEFINE_METHOD3(offset_t, url_fseek, (ByteIOContext *p1, offset_t p2, int p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(av_register_all)
    RESOLVE_METHOD(av_find_input_format)
    RESOLVE_METHOD(url_feof)
    RESOLVE_METHOD(av_close_input_file)
    RESOLVE_METHOD(av_read_frame)
    RESOLVE_METHOD(av_read_play)
    RESOLVE_METHOD(av_read_pause)
    RESOLVE_METHOD(av_seek_frame)
    RESOLVE_METHOD(av_find_stream_info)
    RESOLVE_METHOD(av_open_input_file)
    RESOLVE_METHOD(url_set_interrupt_cb)
    RESOLVE_METHOD(av_dup_packet)
    RESOLVE_METHOD(av_open_input_stream)
    RESOLVE_METHOD(init_put_byte)
    RESOLVE_METHOD(av_probe_input_format)
    RESOLVE_METHOD(dump_format)
    RESOLVE_METHOD(av_destruct_packet_nofree)
    RESOLVE_METHOD(url_fdopen)
    RESOLVE_METHOD(url_fopen)
    RESOLVE_METHOD(url_fclose)
    RESOLVE_METHOD(url_fseek)
    RESOLVE_METHOD(av_read_frame_flush)
    RESOLVE_METHOD(get_buffer)
  END_METHOD_RESOLVE()
};
