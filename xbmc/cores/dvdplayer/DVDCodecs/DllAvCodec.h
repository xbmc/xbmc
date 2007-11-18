
#pragma once
#include "../../../DynamicDll.h"

extern "C" {
#define HAVE_MMX
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#pragma warning(disable:4244)
#include "../ffmpeg/avcodec.h"
}

class DllAvCodecInterface
{
public:
  virtual void avcodec_flush_buffers(AVCodecContext *avctx)=0;
  virtual int avcodec_open(AVCodecContext *avctx, AVCodec *codec)=0;
  virtual AVCodec *avcodec_find_decoder(enum CodecID id)=0;
  virtual int avcodec_close(AVCodecContext *avctx)=0;
  virtual AVFrame *avcodec_alloc_frame(void)=0;
  virtual int avpicture_fill(AVPicture *picture, uint8_t *ptr, int pix_fmt, int width, int height)=0;
  virtual int avcodec_decode_video(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, uint8_t *buf, int buf_size)=0;
  virtual int avcodec_decode_audio(AVCodecContext *avctx, int16_t *samples, int *frame_size_ptr, uint8_t *buf, int buf_size)=0;
  virtual int avcodec_decode_subtitle(AVCodecContext *avctx, AVSubtitle *sub, int *got_sub_ptr, const uint8_t *buf, int buf_size)=0;
  virtual int img_convert(AVPicture *dst, int dst_pix_fmt, const AVPicture *src, int pix_fmt, int width, int height)=0;
  virtual int avpicture_get_size(int pix_fmt, int width, int height)=0;
  virtual AVCodecContext *avcodec_alloc_context(void)=0;
  virtual void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode)=0;
  virtual void avcodec_get_context_defaults(AVCodecContext *s)=0;
  virtual AVCodecParserContext *av_parser_init(int codec_id)=0;
  virtual int av_parser_parse(AVCodecParserContext *s,AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size, 
                    const uint8_t *buf, int buf_size,
                    int64_t pts, int64_t dts)=0;
  virtual void av_parser_close(AVCodecParserContext *s)=0;
  virtual void avpicture_free(AVPicture *picture)=0;
  virtual int avpicture_alloc(AVPicture *picture, int pix_fmt, int width, int height)=0;
  virtual ImgReSampleContext *img_resample_init(int output_width, int output_height, int input_width, int input_height)=0;
  virtual void img_resample(ImgReSampleContext *s, AVPicture *output, const AVPicture *input)=0;
  virtual void img_resample_close(ImgReSampleContext *s)=0;
  virtual AVOption *av_set_string(void *obj, const char *name, const char *val)=0;
  virtual int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic)=0;
  virtual void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic)=0;
};

class DllAvCodec : public DllDynamic, DllAvCodecInterface
{
  DECLARE_DLL_WRAPPER(DllAvCodec, Q:\\system\\players\\dvdplayer\\avcodec-51.dll)
  DEFINE_FUNC_ALIGNED1(void, __cdecl, avcodec_flush_buffers, AVCodecContext*)
  DEFINE_FUNC_ALIGNED2(int, __cdecl, avcodec_open, AVCodecContext*, AVCodec *)
  DEFINE_METHOD1(AVCodec*, avcodec_find_decoder, (enum CodecID p1))
  DEFINE_METHOD1(int, avcodec_close, (AVCodecContext *p1))
  DEFINE_METHOD0(AVFrame*, avcodec_alloc_frame)
  DEFINE_METHOD5(int, avpicture_fill, (AVPicture *p1, uint8_t *p2, int p3, int p4, int p5))
  DEFINE_FUNC_ALIGNED5(int, __cdecl, avcodec_decode_video, AVCodecContext*, AVFrame*, int*, uint8_t*, int)
  DEFINE_FUNC_ALIGNED5(int, __cdecl, avcodec_decode_audio, AVCodecContext*, int16_t*, int*, uint8_t*, int)
  DEFINE_FUNC_ALIGNED5(int, __cdecl, avcodec_decode_subtitle, AVCodecContext*, AVSubtitle*, int*, const uint8_t *, int)
  DEFINE_FUNC_ALIGNED6(int, __cdecl, img_convert, AVPicture*, int, const AVPicture*, int, int, int)
  DEFINE_METHOD3(int, avpicture_get_size, (int p1, int p2, int p3))
  DEFINE_FUNC_ALIGNED0(AVCodecContext*, __cdecl, avcodec_alloc_context)
  DEFINE_METHOD4(void, avcodec_string, (char *p1, int p2, AVCodecContext *p3, int p4))
  DEFINE_METHOD1(void, avcodec_get_context_defaults, (AVCodecContext *p1))
  DEFINE_FUNC_ALIGNED1(AVCodecParserContext*, __cdecl, av_parser_init, int)
  DEFINE_FUNC_ALIGNED8(int, __cdecl, av_parser_parse, AVCodecParserContext*,AVCodecContext*, uint8_t**, int*, 
                       const uint8_t*, int, int64_t, int64_t)
  DEFINE_METHOD1(void, av_parser_close, (AVCodecParserContext *p1))
  DEFINE_METHOD1(void, avpicture_free, (AVPicture *p1))
  DEFINE_METHOD4(int, avpicture_alloc, (AVPicture *p1, int p2, int p3, int p4))
  DEFINE_METHOD4(ImgReSampleContext*, img_resample_init, (int p1, int p2, int p3, int p4))
  DEFINE_FUNC_ALIGNED3(void, __cdecl, img_resample, ImgReSampleContext*, AVPicture*, const AVPicture*)
  DEFINE_METHOD1(void, img_resample_close, (ImgReSampleContext *p1))
  DEFINE_METHOD3(AVOption*, av_set_string, (void *p1, const char *p2, const char *p3))
  DEFINE_METHOD2(int, avcodec_default_get_buffer, (AVCodecContext *p1, AVFrame *p2))
  DEFINE_METHOD2(void, avcodec_default_release_buffer, (AVCodecContext *p1, AVFrame *p2));
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(avcodec_flush_buffers)
    RESOLVE_METHOD(avcodec_open)
    RESOLVE_METHOD(avcodec_find_decoder)
    RESOLVE_METHOD(avcodec_close)
    RESOLVE_METHOD(avcodec_alloc_frame)
    RESOLVE_METHOD(avpicture_fill)
    RESOLVE_METHOD(avcodec_decode_video)
    RESOLVE_METHOD(avcodec_decode_audio)
    RESOLVE_METHOD(avcodec_decode_subtitle)
    RESOLVE_METHOD(img_convert)
    RESOLVE_METHOD(avpicture_get_size)
    RESOLVE_METHOD(avcodec_alloc_context)
    RESOLVE_METHOD(avcodec_string)
    RESOLVE_METHOD(avcodec_get_context_defaults)
    RESOLVE_METHOD(av_parser_init)
    RESOLVE_METHOD(av_parser_parse)
    RESOLVE_METHOD(av_parser_close)
    RESOLVE_METHOD(avpicture_free)
    RESOLVE_METHOD(avpicture_alloc)
    RESOLVE_METHOD(img_resample_init)
    RESOLVE_METHOD(img_resample)
    RESOLVE_METHOD(img_resample_close)
    RESOLVE_METHOD(av_set_string)
    RESOLVE_METHOD(avcodec_default_get_buffer)
    RESOLVE_METHOD(avcodec_default_release_buffer)
  END_METHOD_RESOLVE()
};

// calback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);

class DllAvUtilInterface
{
public:
#if LIBAVUTIL_VERSION_INT < (50<<16)
  virtual void av_log_set_callback(void (*)(void*, int, const char*, va_list))=0;
#endif
  virtual void *av_malloc(unsigned int size)=0;
  virtual void *av_mallocz(unsigned int size)=0;
  virtual void *av_realloc(void *ptr, unsigned int size)=0;
  virtual void av_free(void *ptr)=0;
  virtual void av_freep(void *ptr)=0;
  virtual int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding)=0;
};

class DllAvUtilBase : public DllDynamic, DllAvUtilInterface
{
  DECLARE_DLL_WRAPPER(DllAvUtilBase, Q:\\system\\players\\dvdplayer\\avutil-49.dll)
#if LIBAVUTIL_VERSION_INT < (50<<16)
  DEFINE_METHOD1(void, av_log_set_callback, (void (*p1)(void*, int, const char*, va_list)))
#else
  m_dll->ResolveExport("av_vlog", (void**)&av_vlog) &&
#endif
  DEFINE_METHOD1(void*, av_malloc, (unsigned int p1))
  DEFINE_METHOD1(void*, av_mallocz, (unsigned int p1))
  DEFINE_METHOD2(void*, av_realloc, (void *p1, unsigned int p2))
  DEFINE_METHOD1(void, av_free, (void *p1))
  DEFINE_METHOD1(void, av_freep, (void *p1))
  DEFINE_METHOD4(int64_t, av_rescale_rnd, (int64_t p1, int64_t p2, int64_t p3, enum AVRounding p4));
  public:
    void (*av_vlog)(void*, int, const char*, va_list);
  BEGIN_METHOD_RESOLVE()
#if LIBAVUTIL_VERSION_INT < (50<<16)
    RESOLVE_METHOD(av_log_set_callback)
#else
    m_dll->ResolveExport("av_vlog", (void**)&av_vlog) &&
#endif

    RESOLVE_METHOD(av_malloc)
    RESOLVE_METHOD(av_mallocz)
    RESOLVE_METHOD(av_realloc)
    RESOLVE_METHOD(av_free)
    RESOLVE_METHOD(av_freep)
    RESOLVE_METHOD(av_rescale_rnd)
  END_METHOD_RESOLVE()
};

class DllAvUtil : public DllAvUtilBase
{
public:
  virtual bool Load()
  {
    if( DllAvUtilBase::Load() )
    {
#if LIBAVUTIL_VERSION_INT < (50<<16)
      DllAvUtilBase::av_log_set_callback(ff_avutil_log);
#else
      DllAvUtilBase::av_vlog = ff_avutil_log;
#endif
      return true;
    }
    return false;
  }
};
