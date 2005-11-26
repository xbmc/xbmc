
#pragma once
#include "..\..\..\DynamicDll.h"

#include "..\ffmpeg\ffmpeg.h"
//#include "..\ffmpeg\avcodec.h"

class DllAvCodecInterface
{
public:
  virtual void avcodec_flush_buffers(AVCodecContext *avctx)=0;
  virtual int avcodec_thread_init(AVCodecContext *s, int thread_count)=0;
  virtual int avcodec_open(AVCodecContext *avctx, AVCodec *codec)=0;
  virtual AVCodec *avcodec_find_decoder(enum CodecID id)=0;
  virtual int avcodec_close(AVCodecContext *avctx)=0;
  virtual AVFrame *avcodec_alloc_frame(void)=0;
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, int pix_fmt, int width, int height)=0;
  virtual int avcodec_decode_video(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, uint8_t *buf, int buf_size)=0;
  virtual int avcodec_decode_audio(AVCodecContext *avctx, int16_t *samples, int *frame_size_ptr, uint8_t *buf, int buf_size)=0;
  virtual void av_log_set_callback(void (*)(void*, int, const char*, va_list))=0;
  virtual int img_convert(AVPicture *dst, int dst_pix_fmt, const AVPicture *src, int pix_fmt, int width, int height)=0;
  virtual int avpicture_get_size(int pix_fmt, int width, int height)=0;
  virtual void *av_malloc(unsigned int size)=0;
  virtual void *av_mallocz(unsigned int size)=0;
  virtual void *av_realloc(void *ptr, unsigned int size)=0;
  virtual void av_free(void *ptr)=0;
  virtual void av_freep(void *ptr)=0;
  virtual AVCodecContext *avcodec_alloc_context(void)=0;
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode)=0;
  virtual void avcodec_get_context_defaults(AVCodecContext *s)=0;
  virtual AVCodecParserContext *av_parser_init(int codec_id)=0;
  virtual int av_parser_parse(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size, 
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts)=0;
  virtual void av_parser_close(AVCodecParserContext *s)=0;
  virtual void av_log_set_level(int)=0;
  virtual AVCodec *avcodec_find_encoder(enum CodecID id)=0;
  virtual int avcodec_encode_video(AVCodecContext *avctx, uint8_t *buf, int buf_size, const AVFrame *pict)=0;
  virtual void avpicture_free(AVPicture *picture)=0;
  virtual int avpicture_alloc(AVPicture *picture, int pix_fmt, int width, int height)=0;
  virtual ImgReSampleContext *img_resample_init(int output_width, int output_height, int input_width, int input_height)=0;
  virtual void img_resample(ImgReSampleContext *s, AVPicture *output, const AVPicture *input)=0;
  virtual void img_resample_close(ImgReSampleContext *s)=0;
};

class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
  DECLARE_DLL_WRAPPER(DllAvCodec, Q:\\system\\players\\dvdplayer\\avcodec.dll)
  DEFINE_METHOD1(void, avcodec_flush_buffers, (AVCodecContext *p1))
  DEFINE_METHOD2(int, avcodec_thread_init, (AVCodecContext *p1, int p2))
  DEFINE_METHOD2(int, avcodec_open, (AVCodecContext *p1, AVCodec *p2))
  DEFINE_METHOD1(AVCodec*, avcodec_find_decoder, (enum CodecID p1))
  DEFINE_METHOD1(int, avcodec_close, (AVCodecContext *p1))
  DEFINE_METHOD0(AVFrame*, avcodec_alloc_frame)
  DEFINE_METHOD5(int, avpicture_fill, (AVPicture *p1, uint8_t *p2, int p3, int p4, int p5))
  DEFINE_METHOD5(int, avcodec_decode_video, (AVCodecContext *p1, AVFrame *p2, int *p3, uint8_t *p4, int p5))
  DEFINE_METHOD5(int, avcodec_decode_audio, (AVCodecContext *p1, int16_t *p2, int *p3, uint8_t *p4, int p5))
  DEFINE_METHOD1(void, av_log_set_callback, (void (*p1)(void*, int, const char*, va_list)))
  DEFINE_METHOD6(int, img_convert, (AVPicture *p1, int p2, const AVPicture *p3, int p4, int p5, int p6))
  DEFINE_METHOD3(int, avpicture_get_size, (int p1, int p2, int p3))
  DEFINE_METHOD1(void*, av_malloc, (unsigned int p1))
  DEFINE_METHOD1(void*, av_mallocz, (unsigned int p1))
  DEFINE_METHOD2(void*, av_realloc, (void *p1, unsigned int p2))
  DEFINE_METHOD1(void, av_free, (void *p1))
  DEFINE_METHOD1(void, av_freep, (void *p1))
  DEFINE_METHOD0(AVCodecContext*, avcodec_alloc_context)
  DEFINE_METHOD4(void, avcodec_string, (char *p1, int p2, AVCodecContext *p3, int p4))
  DEFINE_METHOD1(void, avcodec_get_context_defaults, (AVCodecContext *p1))
  DEFINE_METHOD1(AVCodecParserContext*, av_parser_init, (int p1))
  DEFINE_METHOD8(int, av_parser_parse, (AVCodecParserContext *p1,AVCodecContext *p2, uint8_t **p3, int *p4, 
                    const uint8_t *p5, int p6,
                    int64_t p7, int64_t p8))
  DEFINE_METHOD1(void, av_parser_close, (AVCodecParserContext *p1))
  DEFINE_METHOD1(void, av_log_set_level, (int p1))
  DEFINE_METHOD1(AVCodec*, avcodec_find_encoder, (enum CodecID p1))
  DEFINE_METHOD4(int, avcodec_encode_video, (AVCodecContext *p1, uint8_t *p2, int p3, const AVFrame *p4))
  DEFINE_METHOD1(void, avpicture_free, (AVPicture *p1))
  DEFINE_METHOD4(int, avpicture_alloc, (AVPicture *p1, int p2, int p3, int p4))
  DEFINE_METHOD4(ImgReSampleContext*, img_resample_init, (int p1, int p2, int p3, int p4))
  DEFINE_METHOD3(void, img_resample, (ImgReSampleContext *p1, AVPicture *p2, const AVPicture *p3))
  DEFINE_METHOD1(void, img_resample_close, (ImgReSampleContext *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(avcodec_flush_buffers)
    RESOLVE_METHOD(avcodec_thread_init)
    RESOLVE_METHOD(avcodec_open)
    RESOLVE_METHOD(avcodec_find_decoder)
    RESOLVE_METHOD(avcodec_close)
    RESOLVE_METHOD(avcodec_alloc_frame)
    RESOLVE_METHOD(avpicture_fill)
    RESOLVE_METHOD(avcodec_decode_video)
    RESOLVE_METHOD(avcodec_decode_audio)
    RESOLVE_METHOD(av_log_set_callback)
    RESOLVE_METHOD(img_convert)
    RESOLVE_METHOD(avpicture_get_size)
    RESOLVE_METHOD(av_malloc)
    RESOLVE_METHOD(av_mallocz)
    RESOLVE_METHOD(av_realloc)
    RESOLVE_METHOD(av_free)
    RESOLVE_METHOD(av_freep)
    RESOLVE_METHOD(avcodec_alloc_context)
    RESOLVE_METHOD(avcodec_string)
    RESOLVE_METHOD(avcodec_get_context_defaults)
    RESOLVE_METHOD(av_parser_init)
    RESOLVE_METHOD(av_parser_parse)
    RESOLVE_METHOD(av_parser_close)
    RESOLVE_METHOD(av_log_set_level)
    RESOLVE_METHOD(avcodec_find_encoder)
    RESOLVE_METHOD(avcodec_encode_video)
    RESOLVE_METHOD(avpicture_free)
    RESOLVE_METHOD(avpicture_alloc)
    RESOLVE_METHOD(img_resample_init)
    RESOLVE_METHOD(img_resample)
    RESOLVE_METHOD(img_resample_close)
  END_METHOD_RESOLVE()
};
