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
#include "DllAvUtil.h"
#include "utils/log.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __GNUC__
#pragma warning(disable:4244)
#endif

#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVCODEC_AVCODEC_H)
    #include <libavcodec/avcodec.h>
    #if (defined HAVE_LIBAVCODEC_OPT_H)
      #include <libavcodec/opt.h>
    #endif
    #if (defined AVPACKET_IN_AVFORMAT)
      #include <libavformat/avformat.h>
    #endif
  #elif (defined HAVE_FFMPEG_AVCODEC_H)
    #include <ffmpeg/avcodec.h>
    #include <ffmpeg/opt.h>
    #if (defined AVPACKET_IN_AVFORMAT)
      #include <ffmpeg/avformat.h>
    #endif
  #endif
#else
  #include "libavcodec/avcodec.h"
#endif
}

#include "threads/SingleLock.h"

class DllAvCodecInterface
{
public:
  virtual ~DllAvCodecInterface() {}
  virtual void avcodec_register_all(void)=0;
  virtual void avcodec_flush_buffers(AVCodecContext *avctx)=0;
  virtual int avcodec_open2_dont_call(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options)=0;
  virtual AVCodec *avcodec_find_decoder(enum CodecID id)=0;
  virtual AVCodec *avcodec_find_encoder(enum CodecID id)=0;
  virtual int avcodec_close_dont_call(AVCodecContext *avctx)=0;
  virtual AVFrame *avcodec_alloc_frame(void)=0;
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, PixelFormat pix_fmt, int width, int height)=0;
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_encode_audio(AVCodecContext *avctx, uint8_t *buf, int buf_size, const short *samples)=0;
  virtual int avpicture_get_size(PixelFormat pix_fmt, int width, int height)=0;
  virtual AVCodecContext *avcodec_alloc_context3(AVCodec *codec)=0;
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode)=0;
  virtual void avcodec_get_context_defaults3(AVCodecContext *s, AVCodec *codec)=0;
  virtual AVCodecParserContext *av_parser_init(int codec_id)=0;
  virtual int av_parser_parse2(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size,
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts, int64_t pos)=0;
  virtual void av_parser_close(AVCodecParserContext *s)=0;
  virtual AVBitStreamFilterContext *av_bitstream_filter_init(const char *name)=0;
  virtual int av_bitstream_filter_filter(AVBitStreamFilterContext *bsfc,
    AVCodecContext *avctx, const char *args,
    uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *buf, int buf_size, int keyframe) =0;
  virtual void av_bitstream_filter_close(AVBitStreamFilterContext *bsfc) =0;
  virtual void avpicture_free(AVPicture *picture)=0;
  virtual void av_free_packet(AVPacket *pkt)=0;
  virtual int avpicture_alloc(AVPicture *picture, PixelFormat pix_fmt, int width, int height)=0;
  virtual enum PixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum PixelFormat *fmt)=0;
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual AVCodec *av_codec_next(AVCodec *c)=0;
  virtual int av_dup_packet(AVPacket *pkt)=0;
  virtual void av_init_packet(AVPacket *pkt)=0;
};

#if (defined USE_EXTERNAL_FFMPEG) || (defined TARGET_DARWIN)

// Use direct layer
class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
public:
  static CCriticalSection m_critSection;

  virtual ~DllAvCodec() {}
  virtual void avcodec_register_all()
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    ::avcodec_register_all();
  }
  virtual void avcodec_flush_buffers(AVCodecContext *avctx) { ::avcodec_flush_buffers(avctx); }
  virtual int avcodec_open2(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avcodec_open2(avctx, codec, options);
  }
  virtual int avcodec_open2_dont_call(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options) { *(volatile int *)0x0 = 0; return 0; }
  virtual int avcodec_close_dont_call(AVCodecContext *avctx) { *(volatile int *)0x0 = 0; return 0; }
  virtual AVCodec *avcodec_find_decoder(enum CodecID id) { return ::avcodec_find_decoder(id); }
  virtual AVCodec *avcodec_find_encoder(enum CodecID id) { return ::avcodec_find_encoder(id); }
  virtual int avcodec_close(AVCodecContext *avctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avcodec_close(avctx);
  }
  virtual AVFrame *avcodec_alloc_frame() { return ::avcodec_alloc_frame(); }
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, PixelFormat pix_fmt, int width, int height) { return ::avpicture_fill(picture, ptr, pix_fmt, width, height); }
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt) { return ::avcodec_decode_video2(avctx, picture, got_picture_ptr, avpkt); }
  virtual int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, AVPacket *avpkt) { return ::avcodec_decode_audio4(avctx, frame, got_frame_ptr, avpkt); }
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt) { return ::avcodec_decode_subtitle2(avctx, sub, got_sub_ptr, avpkt); }
  virtual int avcodec_encode_audio(AVCodecContext *avctx, uint8_t *buf, int buf_size, const short *samples) { return ::avcodec_encode_audio(avctx, buf, buf_size, samples); }
  virtual int avpicture_get_size(PixelFormat pix_fmt, int width, int height) { return ::avpicture_get_size(pix_fmt, width, height); }
  virtual AVCodecContext *avcodec_alloc_context3(AVCodec *codec) { return ::avcodec_alloc_context3(codec); }
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode) { ::avcodec_string(buf, buf_size, enc, encode); }
  virtual void avcodec_get_context_defaults3(AVCodecContext *s, AVCodec *codec) { ::avcodec_get_context_defaults3(s, codec); }

  virtual AVCodecParserContext *av_parser_init(int codec_id) { return ::av_parser_init(codec_id); }
  virtual int av_parser_parse2(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size,
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts, int64_t pos)
  {
    return ::av_parser_parse2(s, avctx, poutbuf, poutbuf_size, buf, buf_size, pts, dts, pos);
  }
  virtual void av_parser_close(AVCodecParserContext *s) { ::av_parser_close(s); }

  virtual AVBitStreamFilterContext *av_bitstream_filter_init(const char *name) { return ::av_bitstream_filter_init(name); }
  virtual int av_bitstream_filter_filter(AVBitStreamFilterContext *bsfc,
    AVCodecContext *avctx, const char *args,
    uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *buf, int buf_size, int keyframe) { return ::av_bitstream_filter_filter(bsfc, avctx, args, poutbuf, poutbuf_size, buf, buf_size, keyframe); }
  virtual void av_bitstream_filter_close(AVBitStreamFilterContext *bsfc) { ::av_bitstream_filter_close(bsfc); }

  virtual void avpicture_free(AVPicture *picture) { ::avpicture_free(picture); }
  virtual void av_free_packet(AVPacket *pkt) { ::av_free_packet(pkt); }
  virtual int avpicture_alloc(AVPicture *picture, PixelFormat pix_fmt, int width, int height) { return ::avpicture_alloc(picture, pix_fmt, width, height); }
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic) { return ::avcodec_default_get_buffer(s, pic); }
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic) { ::avcodec_default_release_buffer(s, pic); }
  virtual enum PixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum PixelFormat *fmt) { return ::avcodec_default_get_format(s, fmt); }
  virtual AVCodec *av_codec_next(AVCodec *c) { return ::av_codec_next(c); }

  virtual int av_dup_packet(AVPacket *pkt) { return ::av_dup_packet(pkt); }
  virtual void av_init_packet(AVPacket *pkt) { return ::av_init_packet(pkt); }

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if !defined(TARGET_DARWIN)
    CLog::Log(LOGDEBUG, "DllAvCodec: Using libavcodec system library");
#endif
    return true;
  }
  virtual void Unload() {}
};
#else
class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
  DECLARE_DLL_WRAPPER(DllAvCodec, DLL_PATH_LIBAVCODEC)
  DEFINE_FUNC_ALIGNED1(void, __cdecl, avcodec_flush_buffers, AVCodecContext*)
  DEFINE_FUNC_ALIGNED3(int, __cdecl, avcodec_open2_dont_call, AVCodecContext*, AVCodec *, AVDictionary **)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_video2, AVCodecContext*, AVFrame*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_audio4, AVCodecContext*, AVFrame*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_subtitle2, AVCodecContext*, AVSubtitle*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_encode_audio, AVCodecContext*, uint8_t*, int, const short*)
  DEFINE_FUNC_ALIGNED1(AVCodecContext*, __cdecl, avcodec_alloc_context3, AVCodec *)
  DEFINE_FUNC_ALIGNED1(AVCodecParserContext*, __cdecl, av_parser_init, int)
  DEFINE_FUNC_ALIGNED9(int, __cdecl, av_parser_parse2, AVCodecParserContext*,AVCodecContext*, uint8_t**, int*, const uint8_t*, int, int64_t, int64_t, int64_t)
  DEFINE_METHOD1(int, av_dup_packet, (AVPacket *p1))
  DEFINE_METHOD1(void, av_init_packet, (AVPacket *p1))

  LOAD_SYMBOLS();

  DEFINE_METHOD0(void, avcodec_register_all_dont_call)
  DEFINE_METHOD1(AVCodec*, avcodec_find_decoder, (enum CodecID p1))
  DEFINE_METHOD1(AVCodec*, avcodec_find_encoder, (enum CodecID p1))
  DEFINE_METHOD1(int, avcodec_close_dont_call, (AVCodecContext *p1))
  DEFINE_METHOD0(AVFrame*, avcodec_alloc_frame)
  DEFINE_METHOD5(int, avpicture_fill, (AVPicture *p1, uint8_t *p2, PixelFormat p3, int p4, int p5))
  DEFINE_METHOD3(int, avpicture_get_size, (PixelFormat p1, int p2, int p3))
  DEFINE_METHOD4(void, avcodec_string, (char *p1, int p2, AVCodecContext *p3, int p4))
  DEFINE_METHOD2(void, avcodec_get_context_defaults3, (AVCodecContext *p1, AVCodec *p2))
  DEFINE_METHOD1(void, av_parser_close, (AVCodecParserContext *p1))
  DEFINE_METHOD1(void, avpicture_free, (AVPicture *p1))
  DEFINE_METHOD1(AVBitStreamFilterContext*, av_bitstream_filter_init, (const char *p1))
  DEFINE_METHOD8(int, av_bitstream_filter_filter, (AVBitStreamFilterContext* p1, AVCodecContext* p2, const char* p3, uint8_t** p4, int* p5, const uint8_t* p6, int p7, int p8))
  DEFINE_METHOD1(void, av_bitstream_filter_close, (AVBitStreamFilterContext *p1))
  DEFINE_METHOD1(void, av_free_packet, (AVPacket *p1))
  DEFINE_METHOD4(int, avpicture_alloc, (AVPicture *p1, PixelFormat p2, int p3, int p4))
  DEFINE_METHOD2(int, avcodec_default_get_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD2(void, avcodec_default_release_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD2(enum PixelFormat, avcodec_default_get_format, (struct AVCodecContext *p1, const enum PixelFormat *p2))

  DEFINE_METHOD1(AVCodec*, av_codec_next, (AVCodec *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(avcodec_flush_buffers)
    RESOLVE_METHOD_RENAME(avcodec_open2,avcodec_open2_dont_call)
    RESOLVE_METHOD_RENAME(avcodec_close,avcodec_close_dont_call)
    RESOLVE_METHOD(avcodec_find_decoder)
    RESOLVE_METHOD(avcodec_find_encoder)
    RESOLVE_METHOD(avcodec_alloc_frame)
    RESOLVE_METHOD_RENAME(avcodec_register_all, avcodec_register_all_dont_call)
    RESOLVE_METHOD(avpicture_fill)
    RESOLVE_METHOD(avcodec_decode_video2)
    RESOLVE_METHOD(avcodec_decode_audio4)
    RESOLVE_METHOD(avcodec_decode_subtitle2)
    RESOLVE_METHOD(avcodec_encode_audio)
    RESOLVE_METHOD(avpicture_get_size)
    RESOLVE_METHOD(avcodec_alloc_context3)
    RESOLVE_METHOD(avcodec_string)
    RESOLVE_METHOD(avcodec_get_context_defaults3)
    RESOLVE_METHOD(av_parser_init)
    RESOLVE_METHOD(av_parser_parse2)
    RESOLVE_METHOD(av_parser_close)
    RESOLVE_METHOD(av_bitstream_filter_init)
    RESOLVE_METHOD(av_bitstream_filter_filter)
    RESOLVE_METHOD(av_bitstream_filter_close)
    RESOLVE_METHOD(avpicture_free)
    RESOLVE_METHOD(avpicture_alloc)
    RESOLVE_METHOD(av_free_packet)
    RESOLVE_METHOD(avcodec_default_get_buffer)
    RESOLVE_METHOD(avcodec_default_release_buffer)
    RESOLVE_METHOD(avcodec_default_get_format)
    RESOLVE_METHOD(av_codec_next)
    RESOLVE_METHOD(av_dup_packet)
    RESOLVE_METHOD(av_init_packet)
  END_METHOD_RESOLVE()

  /* dependencies of libavcodec */
  DllAvUtil m_dllAvUtil;
  // DllAvUtil loaded implicitely by m_dllAvCore

public:
    static CCriticalSection m_critSection;
    int avcodec_open2(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options)
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      return avcodec_open2_dont_call(avctx,codec, options);
    }
    int avcodec_close(AVCodecContext *avctx)
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      return avcodec_close_dont_call(avctx);
    }
    void avcodec_register_all()
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      avcodec_register_all_dont_call();
    }
    virtual bool Load()
    {
      if (!m_dllAvUtil.Load())
	return false;
      return DllDynamic::Load();
    }
};

#endif
