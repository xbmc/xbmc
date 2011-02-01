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
    #if (defined AVPACKET_IN_AVFORMAT)
      #include <libavformat/avformat.h>
    #endif
  #elif (defined HAVE_FFMPEG_AVCODEC_H)
    #include <ffmpeg/avcodec.h>
    #if (defined AVPACKET_IN_AVFORMAT)
      #include <ffmpeg/avformat.h>
    #endif
  #endif

  /* From non-public audioconvert.h */
  int64_t avcodec_guess_channel_layout(int nb_channels, enum CodecID codec_id, const char *fmt_name);
  struct AVAudioConvert;
  typedef struct AVAudioConvert AVAudioConvert;
  AVAudioConvert *av_audio_convert_alloc(enum AVSampleFormat out_fmt, int out_channels,
                                         enum AVSampleFormat in_fmt, int in_channels,
                                         const float *matrix, int flags);
  void av_audio_convert_free(AVAudioConvert *ctx);
  int av_audio_convert(AVAudioConvert *ctx,
                             void * const out[6], const int out_stride[6],
                       const void * const  in[6], const int  in_stride[6], int len);
#else
  #include "libavcodec/avcodec.h"
  #include "libavcodec/audioconvert.h"
#endif
}

/* Some convenience macros introduced at this particular revision of libavcodec.
 */
#if LIBAVCODEC_VERSION_INT < (52<<16 | 25<<8)
#define CH_LAYOUT_5POINT0_BACK      (CH_LAYOUT_SURROUND|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_5POINT1_BACK      (CH_LAYOUT_5POINT0_BACK|CH_LOW_FREQUENCY)
#undef CH_LAYOUT_7POINT1_WIDE
#define CH_LAYOUT_7POINT1_WIDE      (CH_LAYOUT_5POINT1_BACK|\
                                           CH_FRONT_LEFT_OF_CENTER|CH_FRONT_RIGHT_OF_CENTER)
#endif

#include "threads/SingleLock.h"

class DllAvCodecInterface
{
public:
  virtual ~DllAvCodecInterface() {}
  virtual void avcodec_register_all(void)=0;
  virtual void avcodec_flush_buffers(AVCodecContext *avctx)=0;
  virtual int avcodec_open_dont_call(AVCodecContext *avctx, AVCodec *codec)=0;
  virtual AVCodec *avcodec_find_decoder(enum CodecID id)=0;
  virtual AVCodec *avcodec_find_encoder(enum CodecID id)=0;
  virtual int avcodec_close_dont_call(AVCodecContext *avctx)=0;
  virtual AVFrame *avcodec_alloc_frame(void)=0;
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, PixelFormat pix_fmt, int width, int height)=0;
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_decode_audio3(AVCodecContext *avctx, int16_t *samples, int *frame_size_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt)=0;
  virtual int avcodec_encode_audio(AVCodecContext *avctx, uint8_t *buf, int buf_size, const short *samples)=0;
  virtual int avpicture_get_size(PixelFormat pix_fmt, int width, int height)=0;
  virtual AVCodecContext *avcodec_alloc_context(void)=0;
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode)=0;
  virtual void avcodec_get_context_defaults(AVCodecContext *s)=0;
  virtual AVCodecParserContext *av_parser_init(int codec_id)=0;
  virtual int av_parser_parse(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size,
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts)=0;
  virtual void av_parser_close(AVCodecParserContext *s)=0;
  virtual void avpicture_free(AVPicture *picture)=0;
  virtual void av_free_packet(AVPacket *pkt)=0;
  virtual int avpicture_alloc(AVPicture *picture, PixelFormat pix_fmt, int width, int height)=0;
  virtual AVOption *av_set_string(void *obj, const char *name, const char *val)=0;
  virtual enum PixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum PixelFormat *fmt)=0;
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual int avcodec_thread_init(AVCodecContext *s, int thread_count)=0;
  virtual AVCodec *av_codec_next(AVCodec *c)=0;
  virtual int av_get_bits_per_sample_format(enum SampleFormat sample_fmt)=0;
  virtual AVAudioConvert *av_audio_convert_alloc(enum SampleFormat out_fmt, int out_channels,
                                                 enum SampleFormat in_fmt , int in_channels,
                                                 const float *matrix      , int flags)=0;
  virtual void av_audio_convert_free(AVAudioConvert *ctx)=0;
  virtual int av_audio_convert(AVAudioConvert *ctx,
                                     void * const out[6], const int out_stride[6],
                               const void * const  in[6], const int  in_stride[6], int len)=0;
  virtual int av_dup_packet(AVPacket *pkt)=0;
  virtual void av_init_packet(AVPacket *pkt)=0;
  virtual int64_t avcodec_guess_channel_layout(int nb_channels, enum CodecID codec_id, const char *fmt_name)=0;
};

#if (defined USE_EXTERNAL_FFMPEG)

extern "C" { AVOption* av_set_string(void *obj, const char *name, const char *val); }

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
  virtual int avcodec_open(AVCodecContext *avctx, AVCodec *codec)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avcodec_open(avctx, codec);
  }
  virtual int avcodec_open_dont_call(AVCodecContext *avctx, AVCodec *codec) { *(int *)0x0 = 0; return 0; }
  virtual int avcodec_close_dont_call(AVCodecContext *avctx) { *(int *)0x0 = 0; return 0; }
  virtual AVCodec *avcodec_find_decoder(enum CodecID id) { return ::avcodec_find_decoder(id); }
  virtual AVCodec *avcodec_find_encoder(enum CodecID id) { return ::avcodec_find_encoder(id); }
  virtual int avcodec_close(AVCodecContext *avctx)
  {
    CSingleLock lock(DllAvCodec::m_critSection);
    return ::avcodec_close(avctx);
  }
  virtual AVFrame *avcodec_alloc_frame() { return ::avcodec_alloc_frame(); }
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, PixelFormat pix_fmt, int width, int height) { return ::avpicture_fill(picture, ptr, pix_fmt, width, height); }
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
  // API added on: 2009-04-07
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt) { return ::avcodec_decode_video2(avctx, picture, got_picture_ptr, avpkt); }
  virtual int avcodec_decode_audio3(AVCodecContext *avctx, int16_t *samples, int *frame_size_ptr, AVPacket *avpkt) { return ::avcodec_decode_audio3(avctx, samples, frame_size_ptr, avpkt); }
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt) { return ::avcodec_decode_subtitle2(avctx, sub, got_sub_ptr, avpkt); }
#else
  virtual int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, AVPacket *avpkt) { return ::avcodec_decode_video(avctx, picture, got_picture_ptr, avpkt->data, avpkt->size); }
  virtual int avcodec_decode_audio3(AVCodecContext *avctx, int16_t *samples, int *frame_size_ptr, AVPacket *avpkt) { return ::avcodec_decode_audio2(avctx, samples, frame_size_ptr, avpkt->data, avpkt->size); }
  virtual int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, AVPacket *avpkt) { return ::avcodec_decode_subtitle(avctx, sub, got_sub_ptr, avpkt->data, avpkt->size); }
#endif
  virtual int avcodec_encode_audio(AVCodecContext *avctx, uint8_t *buf, int buf_size, const short *samples) { return ::avcodec_encode_audio(avctx, buf, buf_size, samples); }
  virtual int avpicture_get_size(PixelFormat pix_fmt, int width, int height) { return ::avpicture_get_size(pix_fmt, width, height); }
  virtual AVCodecContext *avcodec_alloc_context() { return ::avcodec_alloc_context(); }
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode) { ::avcodec_string(buf, buf_size, enc, encode); }
  virtual void avcodec_get_context_defaults(AVCodecContext *s) { ::avcodec_get_context_defaults(s); }

  virtual AVCodecParserContext *av_parser_init(int codec_id) { return ::av_parser_init(codec_id); }
  virtual int av_parser_parse(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size,
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts) { return ::av_parser_parse(s, avctx, poutbuf, poutbuf_size, buf, buf_size, pts, dts); }
  virtual void av_parser_close(AVCodecParserContext *s) { ::av_parser_close(s); }

  virtual void avpicture_free(AVPicture *picture) { ::avpicture_free(picture); }
  virtual void av_free_packet(AVPacket *pkt) { ::av_free_packet(pkt); }
  virtual int avpicture_alloc(AVPicture *picture, PixelFormat pix_fmt, int width, int height) { return ::avpicture_alloc(picture, pix_fmt, width, height); }
  virtual AVOption *av_set_string(void *obj, const char *name, const char *val) { return ::av_set_string(obj, name, val); }
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic) { return ::avcodec_default_get_buffer(s, pic); }
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic) { ::avcodec_default_release_buffer(s, pic); }
  virtual enum PixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum PixelFormat *fmt) { return ::avcodec_default_get_format(s, fmt); }
  virtual int avcodec_thread_init(AVCodecContext *s, int thread_count) { return ::avcodec_thread_init(s, thread_count); }
  virtual AVCodec *av_codec_next(AVCodec *c) { return ::av_codec_next(c); }
  virtual int av_get_bits_per_sample_format(enum SampleFormat sample_fmt)
          { return ::av_get_bits_per_sample_format(sample_fmt); }
  virtual AVAudioConvert *av_audio_convert_alloc(enum SampleFormat out_fmt, int out_channels,
                                                 enum SampleFormat in_fmt , int in_channels,
                                                 const float *matrix      , int flags)
          { return ::av_audio_convert_alloc(out_fmt, out_channels, in_fmt, in_channels, matrix, flags); }
  virtual void av_audio_convert_free(AVAudioConvert *ctx)
          { ::av_audio_convert_free(ctx); }

  virtual int av_audio_convert(AVAudioConvert *ctx,
                                     void * const out[6], const int out_stride[6],
                               const void * const  in[6], const int  in_stride[6], int len)
          { return ::av_audio_convert(ctx, out, out_stride, in, in_stride, len); }

  virtual int av_dup_packet(AVPacket *pkt) { return ::av_dup_packet(pkt); }
  virtual void av_init_packet(AVPacket *pkt) { return ::av_init_packet(pkt); }
  virtual int64_t avcodec_guess_channel_layout(int nb_channels, enum CodecID codec_id, const char *fmt_name) { return ::avcodec_guess_channel_layout(nb_channels, codec_id, fmt_name); }

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllAvCodec: Using libavcodec system library");
    return true;
  }
  virtual void Unload() {}
};
#else
class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
  DECLARE_DLL_WRAPPER(DllAvCodec, DLL_PATH_LIBAVCODEC)
#ifndef _LINUX
  DEFINE_FUNC_ALIGNED1(void, __cdecl, avcodec_flush_buffers, AVCodecContext*)
  DEFINE_FUNC_ALIGNED2(int, __cdecl, avcodec_open_dont_call, AVCodecContext*, AVCodec *)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_video2, AVCodecContext*, AVFrame*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_audio3, AVCodecContext*, int16_t*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_decode_subtitle2, AVCodecContext*, AVSubtitle*, int*, AVPacket*)
  DEFINE_FUNC_ALIGNED4(int, __cdecl, avcodec_encode_audio, AVCodecContext*, uint8_t*, int, const short*)
  DEFINE_FUNC_ALIGNED0(AVCodecContext*, __cdecl, avcodec_alloc_context)
  DEFINE_FUNC_ALIGNED1(AVCodecParserContext*, __cdecl, av_parser_init, int)
  DEFINE_FUNC_ALIGNED8(int, __cdecl, av_parser_parse, AVCodecParserContext*,AVCodecContext*, uint8_t**, int*, const uint8_t*, int, int64_t, int64_t)
#else
  DEFINE_METHOD1(void, avcodec_flush_buffers, (AVCodecContext* p1))
  DEFINE_METHOD2(int, avcodec_open_dont_call, (AVCodecContext* p1, AVCodec *p2))
  DEFINE_METHOD4(int, avcodec_decode_video2, (AVCodecContext* p1, AVFrame *p2, int *p3, AVPacket *p4))
  DEFINE_METHOD4(int, avcodec_decode_audio3, (AVCodecContext* p1, int16_t *p2, int *p3, AVPacket *p4))
  DEFINE_METHOD4(int, avcodec_decode_subtitle2, (AVCodecContext* p1, AVSubtitle *p2, int *p3, AVPacket *p4))
  DEFINE_METHOD4(int, avcodec_encode_audio, (AVCodecContext* p1, uint8_t *p2, int p3, const short *p4))
  DEFINE_METHOD0(AVCodecContext*, avcodec_alloc_context)
  DEFINE_METHOD1(AVCodecParserContext*, av_parser_init, (int p1))
  DEFINE_METHOD8(int, av_parser_parse, (AVCodecParserContext* p1, AVCodecContext* p2, uint8_t** p3, int* p4, const uint8_t* p5, int p6, int64_t p7, int64_t p8))
#endif
  DEFINE_METHOD1(int, av_dup_packet, (AVPacket *p1))
  DEFINE_METHOD1(void, av_init_packet, (AVPacket *p1))
  DEFINE_METHOD3(int64_t, avcodec_guess_channel_layout, (int p1, enum CodecID p2, const char *p3))

  LOAD_SYMBOLS();

  DEFINE_METHOD0(void, avcodec_register_all_dont_call)
  DEFINE_METHOD1(AVCodec*, avcodec_find_decoder, (enum CodecID p1))
  DEFINE_METHOD1(AVCodec*, avcodec_find_encoder, (enum CodecID p1))
  DEFINE_METHOD1(int, avcodec_close_dont_call, (AVCodecContext *p1))
  DEFINE_METHOD0(AVFrame*, avcodec_alloc_frame)
  DEFINE_METHOD5(int, avpicture_fill, (AVPicture *p1, uint8_t *p2, PixelFormat p3, int p4, int p5))
  DEFINE_METHOD3(int, avpicture_get_size, (PixelFormat p1, int p2, int p3))
  DEFINE_METHOD4(void, avcodec_string, (char *p1, int p2, AVCodecContext *p3, int p4))
  DEFINE_METHOD1(void, avcodec_get_context_defaults, (AVCodecContext *p1))
  DEFINE_METHOD1(void, av_parser_close, (AVCodecParserContext *p1))
  DEFINE_METHOD1(void, avpicture_free, (AVPicture *p1))
  DEFINE_METHOD1(void, av_free_packet, (AVPacket *p1))
  DEFINE_METHOD4(int, avpicture_alloc, (AVPicture *p1, PixelFormat p2, int p3, int p4))
  DEFINE_METHOD3(AVOption*, av_set_string, (void *p1, const char *p2, const char *p3))
  DEFINE_METHOD2(int, avcodec_default_get_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD2(void, avcodec_default_release_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD2(enum PixelFormat, avcodec_default_get_format, (struct AVCodecContext *p1, const enum PixelFormat *p2))

  DEFINE_METHOD2(int, avcodec_thread_init, (AVCodecContext *p1, int p2))
  DEFINE_METHOD1(AVCodec*, av_codec_next, (AVCodec *p1))
  DEFINE_METHOD1(int, av_get_bits_per_sample_format, (enum SampleFormat p1))
  DEFINE_METHOD6(AVAudioConvert*, av_audio_convert_alloc, (enum SampleFormat p1, int p2,
                                                           enum SampleFormat p3, int p4,
                                                           const float *p5, int p6))
  DEFINE_METHOD1(void, av_audio_convert_free, (AVAudioConvert *p1));
  DEFINE_METHOD6(int,  av_audio_convert,      (AVAudioConvert *p1,
                                                     void * const p2[6], const int p3[6],
                                               const void * const p4[6], const int p5[6], int p6))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(avcodec_flush_buffers)
    RESOLVE_METHOD_RENAME(avcodec_open,avcodec_open_dont_call)
    RESOLVE_METHOD_RENAME(avcodec_close,avcodec_close_dont_call)
    RESOLVE_METHOD(avcodec_find_decoder)
    RESOLVE_METHOD(avcodec_find_encoder)
    RESOLVE_METHOD(avcodec_alloc_frame)
    RESOLVE_METHOD_RENAME(avcodec_register_all, avcodec_register_all_dont_call)
    RESOLVE_METHOD(avpicture_fill)
    RESOLVE_METHOD(avcodec_decode_video2)
    RESOLVE_METHOD(avcodec_decode_audio3)
    RESOLVE_METHOD(avcodec_decode_subtitle2)
    RESOLVE_METHOD(avcodec_encode_audio)
    RESOLVE_METHOD(avpicture_get_size)
    RESOLVE_METHOD(avcodec_alloc_context)
    RESOLVE_METHOD(avcodec_string)
    RESOLVE_METHOD(avcodec_get_context_defaults)
    RESOLVE_METHOD(av_parser_init)
    RESOLVE_METHOD(av_parser_parse)
    RESOLVE_METHOD(av_parser_close)
    RESOLVE_METHOD(avpicture_free)
    RESOLVE_METHOD(avpicture_alloc)
    RESOLVE_METHOD(av_free_packet)
    RESOLVE_METHOD(av_set_string)
    RESOLVE_METHOD(avcodec_default_get_buffer)
    RESOLVE_METHOD(avcodec_default_release_buffer)
    RESOLVE_METHOD(avcodec_default_get_format)
    RESOLVE_METHOD(avcodec_thread_init)
    RESOLVE_METHOD(av_codec_next)
    RESOLVE_METHOD(av_get_bits_per_sample_format)
    RESOLVE_METHOD(av_audio_convert_alloc)
    RESOLVE_METHOD(av_audio_convert_free)
    RESOLVE_METHOD(av_audio_convert)
    RESOLVE_METHOD(av_dup_packet)
    RESOLVE_METHOD(av_init_packet)
    RESOLVE_METHOD(avcodec_guess_channel_layout)
  END_METHOD_RESOLVE()

  /* dependency of libavcodec */
  DllAvUtil m_dllAvUtil;

public:
    static CCriticalSection m_critSection;
    int avcodec_open(AVCodecContext *avctx, AVCodec *codec)
    {
      CSingleLock lock(DllAvCodec::m_critSection);
      return avcodec_open_dont_call(avctx,codec);
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
