/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodecFFmpeg.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDStreamInfo.h"
#include "ServiceBroker.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoSettings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <memory>
#include <mutex>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/video_enc_params.h>
}

#ifndef TARGET_POSIX
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#else
#include <math.h>
#define RINT lrint
#endif

enum DecoderState
{
  STATE_NONE,
  STATE_SW_SINGLE,
  STATE_HW_SINGLE,
  STATE_HW_FAILED,
  STATE_SW_MULTI
};

enum EFilterFlags {
  FILTER_NONE                =  0x0,
  FILTER_DEINTERLACE_BWDIF   =  0x1,  //< use first deinterlace mode
  FILTER_DEINTERLACE_ANY     =  0xf,  //< use any deinterlace mode
  FILTER_DEINTERLACE_FLAGGED = 0x10,  //< only deinterlace flagged frames
  FILTER_DEINTERLACE_HALFED  = 0x20,  //< do half rate deinterlacing
  FILTER_ROTATE              = 0x40,  //< rotate image according to the codec hints
};

namespace
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
static inline void ConvertYUV10RowTo8(const uint16_t* src, uint8_t* dst, int count)
{
  int x = 0;
  for (; x <= count - 16; x += 16)
  {
    uint16x8_t in0 = vld1q_u16(src + x);
    uint16x8_t in1 = vld1q_u16(src + x + 8);
    uint8x8_t out0 = vshrn_n_u16(in0, 2);
    uint8x8_t out1 = vshrn_n_u16(in1, 2);
    vst1q_u8(dst + x, vcombine_u8(out0, out1));
  }
  for (; x < count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[x] >> 2);
  }
}

static inline void ConvertYUV12RowTo8(const uint16_t* src, uint8_t* dst, int count)
{
  int x = 0;
  for (; x <= count - 16; x += 16)
  {
    uint16x8_t in0 = vld1q_u16(src + x);
    uint16x8_t in1 = vld1q_u16(src + x + 8);
    uint8x8_t out0 = vshrn_n_u16(in0, 4);
    uint8x8_t out1 = vshrn_n_u16(in1, 4);
    vst1q_u8(dst + x, vcombine_u8(out0, out1));
  }
  for (; x < count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[x] >> 4);
  }
}

static inline void DownscaleYUV10RowTo8(const uint16_t* src, uint8_t* dst, int dst_count)
{
  int x = 0;
  for (; x <= dst_count - 16; x += 16)
  {
    uint16x8x2_t in0 = vld2q_u16(src + 2 * x);
    uint16x8x2_t in1 = vld2q_u16(src + 2 * x + 16);
    uint8x8_t out0 = vshrn_n_u16(in0.val[0], 2);
    uint8x8_t out1 = vshrn_n_u16(in1.val[0], 2);
    vst1q_u8(dst + x, vcombine_u8(out0, out1));
  }
  for (; x < dst_count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[2 * x] >> 2);
  }
}

static inline void DownscaleYUV12RowTo8(const uint16_t* src, uint8_t* dst, int dst_count)
{
  int x = 0;
  for (; x <= dst_count - 16; x += 16)
  {
    uint16x8x2_t in0 = vld2q_u16(src + 2 * x);
    uint16x8x2_t in1 = vld2q_u16(src + 2 * x + 16);
    uint8x8_t out0 = vshrn_n_u16(in0.val[0], 4);
    uint8x8_t out1 = vshrn_n_u16(in1.val[0], 4);
    vst1q_u8(dst + x, vcombine_u8(out0, out1));
  }
  for (; x < dst_count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[2 * x] >> 4);
  }
}

static inline void DownscaleYUV8RowTo8(const uint8_t* src, uint8_t* dst, int dst_count)
{
  int x = 0;
  for (; x <= dst_count - 16; x += 16)
  {
    uint8x16x2_t in = vld2q_u8(src + 2 * x);
    vst1q_u8(dst + x, in.val[0]);
  }
  for (; x < dst_count; x++)
  {
    dst[x] = src[2 * x];
  }
}
#else
static inline void ConvertYUV10RowTo8(const uint16_t* src, uint8_t* dst, int count)
{
  for (int x = 0; x < count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[x] >> 2);
  }
}

static inline void ConvertYUV12RowTo8(const uint16_t* src, uint8_t* dst, int count)
{
  for (int x = 0; x < count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[x] >> 4);
  }
}

static inline void DownscaleYUV10RowTo8(const uint16_t* src, uint8_t* dst, int dst_count)
{
  for (int x = 0; x < dst_count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[2 * x] >> 2);
  }
}

static inline void DownscaleYUV12RowTo8(const uint16_t* src, uint8_t* dst, int dst_count)
{
  for (int x = 0; x < dst_count; x++)
  {
    dst[x] = static_cast<uint8_t>(src[2 * x] >> 4);
  }
}

static inline void DownscaleYUV8RowTo8(const uint8_t* src, uint8_t* dst, int dst_count)
{
  for (int x = 0; x < dst_count; x++)
  {
    dst[x] = src[2 * x];
  }
}
#endif
} // namespace

//------------------------------------------------------------------------------
// Video Buffers
//------------------------------------------------------------------------------

class CVideoBufferFFmpeg : public CVideoBuffer
{
public:
  CVideoBufferFFmpeg(IVideoBufferPool &pool, int id);
  ~CVideoBufferFFmpeg() override;
  void GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES]) override;
  void GetStrides(int(&strides)[YuvImage::MAX_PLANES]) override;

  void SetRef(AVFrame *frame);
  void Unref();

protected:
  AVFrame* m_pFrame;
};

CVideoBufferFFmpeg::CVideoBufferFFmpeg(IVideoBufferPool &pool, int id)
: CVideoBuffer(id)
{
  m_pFrame = av_frame_alloc();
}

CVideoBufferFFmpeg::~CVideoBufferFFmpeg()
{
  av_frame_free(&m_pFrame);
}

void CVideoBufferFFmpeg::GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES])
{
  planes[0] = m_pFrame->data[0];
  planes[1] = m_pFrame->data[1];
  planes[2] = m_pFrame->data[2];
}

void CVideoBufferFFmpeg::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  strides[0] = m_pFrame->linesize[0];
  strides[1] = m_pFrame->linesize[1];
  strides[2] = m_pFrame->linesize[2];
}

void CVideoBufferFFmpeg::SetRef(AVFrame *frame)
{
  av_frame_unref(m_pFrame);
  av_frame_move_ref(m_pFrame, frame);
  m_pixFormat = (AVPixelFormat)m_pFrame->format;
}

void CVideoBufferFFmpeg::Unref()
{
  av_frame_unref(m_pFrame);
}

//------------------------------------------------------------------------------

class CVideoBufferPoolFFmpeg : public IVideoBufferPool
{
public:
  ~CVideoBufferPoolFFmpeg() override;
  void Return(int id) override;
  CVideoBuffer* Get() override;

protected:
  CCriticalSection m_critSection;
  std::vector<CVideoBufferFFmpeg*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;
};

CVideoBufferPoolFFmpeg::~CVideoBufferPoolFFmpeg()
{
  for (auto buf : m_all)
  {
    delete buf;
  }
}

CVideoBuffer* CVideoBufferPoolFFmpeg::Get()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CVideoBufferFFmpeg *buf = nullptr;
  if (!m_free.empty())
  {
    int idx = m_free.front();
    m_free.pop_front();
    m_used.push_back(idx);
    buf = m_all[idx];
  }
  else
  {
    int id = m_all.size();
    buf = new CVideoBufferFFmpeg(*this, id);
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolFFmpeg::Return(int id)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_all[id]->Unref();
  auto it = m_used.begin();
  while (it != m_used.end())
  {
    if (*it == id)
    {
      m_used.erase(it);
      break;
    }
    else
      ++it;
  }
  m_free.push_back(id);
}

//------------------------------------------------------------------------------
// main class
//------------------------------------------------------------------------------

CDVDVideoCodecFFmpeg::CDropControl::CDropControl()
{
  Reset(true);
}

void CDVDVideoCodecFFmpeg::CDropControl::Reset(bool init)
{
  m_lastPTS = AV_NOPTS_VALUE;

  if (init || m_state != VALID)
  {
    m_count = 0;
    m_diffPTS = 0;
    m_state = INIT;
  }
}

void CDVDVideoCodecFFmpeg::CDropControl::Process(int64_t pts, bool drop)
{
  if (m_state == INIT)
  {
    if (pts != AV_NOPTS_VALUE && m_lastPTS != AV_NOPTS_VALUE)
    {
      m_diffPTS += pts - m_lastPTS;
      m_count++;
    }
    if (m_count > 10)
    {
      m_diffPTS = m_diffPTS / m_count;
      if (m_diffPTS > 0)
      {
        CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg::CDropControl: calculated diff time: {}",
                  m_diffPTS);
        m_state = CDropControl::VALID;
        m_count = 0;
      }
    }
  }
  else if (m_state == VALID && !drop)
  {
    if (std::abs(pts - m_lastPTS - m_diffPTS) > m_diffPTS * 0.2)
    {
      m_count++;
      if (m_count > 5)
      {
        CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg::CDropControl: lost diff");
        Reset(true);
      }
    }
    else
      m_count = 0;
  }
  m_lastPTS = pts;
}

enum AVPixelFormat CDVDVideoCodecFFmpeg::GetFormat(struct AVCodecContext * avctx, const AVPixelFormat * fmt)
{
  ICallbackHWAccel *cb = static_cast<ICallbackHWAccel*>(avctx->opaque);
  CDVDVideoCodecFFmpeg* ctx  = dynamic_cast<CDVDVideoCodecFFmpeg*>(cb);

  const char* pixFmtName = av_get_pix_fmt_name(*fmt);

  ctx->m_processInfo.SetVideoDimensions(avctx->coded_width, avctx->coded_height);

  // if frame threading is enabled hw accel is not allowed
  // 2nd condition:
  // fix an ffmpeg issue here, it calls us with an invalid profile
  // then a 2nd call with a valid one
  if(ctx->m_decoderState != STATE_HW_SINGLE ||
     (avctx->codec_id == AV_CODEC_ID_VC1 && avctx->profile == FF_PROFILE_UNKNOWN))
  {
    AVPixelFormat defaultFmt = avcodec_default_get_format(avctx, fmt);
    pixFmtName = av_get_pix_fmt_name(defaultFmt);
    ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
    ctx->m_processInfo.SetSwDeinterlacingMethods();
    return defaultFmt;
  }

  // hardware decoder de-selected, restore standard ffmpeg
  if (ctx->HasHardware())
  {
    ctx->SetHardware(nullptr);
    avctx->get_buffer2 = avcodec_default_get_buffer2;
    avctx->slice_flags = 0;
    av_buffer_unref(&avctx->hw_frames_ctx);
  }

  const AVPixelFormat * cur = fmt;
  while (*cur != AV_PIX_FMT_NONE)
  {
    pixFmtName = av_get_pix_fmt_name(*cur);

    auto hwaccels = CDVDFactoryCodec::GetHWAccels();
    for (auto &hwaccel : hwaccels)
    {
      IHardwareDecoder *pDecoder(CDVDFactoryCodec::CreateVideoCodecHWAccel(hwaccel, ctx->m_hints,
                                                                           ctx->m_processInfo, *cur));
      if (pDecoder)
      {
        if (pDecoder->Open(avctx, ctx->m_pCodecContext, *cur))
        {
          ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
          ctx->SetHardware(pDecoder);
          return *cur;
        }
        pDecoder->Release();
      }
    }
    cur++;
  }

  ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
  ctx->m_decoderState = STATE_HW_FAILED;
  return avcodec_default_get_format(avctx, fmt);
}

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg(CProcessInfo& processInfo)
  : CDVDVideoCodec(processInfo),
    m_videoBufferPool(std::make_shared<CVideoBufferPoolFFmpeg>()),
    m_postProc(processInfo)
{
  m_decoderState = STATE_NONE;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (hints.cryptoSession)
  {
    CLog::Log(LOGERROR,"CDVDVideoCodecFFmpeg::Open() CryptoSessions unsupported!");
    return false;
  }

  m_hints = hints;
  m_options = options;

  const AVCodec* pCodec = nullptr;

  m_iOrientation = hints.orientation;

  m_formats.clear();
  m_formats = m_processInfo.GetPixFormats();
  m_formats.push_back(AV_PIX_FMT_NONE); /* always add none to get a terminated list in ffmpeg world */
  m_processInfo.SetSwDeinterlacingMethods();
  m_processInfo.SetVideoInterlaced(false);

  // libdav1d av1 sw decoding is implemented as a separate decoder
  // in ffmpeg which is always found first when calling `avcodec_find_decoder`.
  // To get hwaccels we look for decoders registered for `av1` (unless sw decoding is enforced).
  // The decoder state check is needed to succesfully fallback to sw decoding if
  // necessary (on retry).
  if (hints.codec == AV_CODEC_ID_AV1 && m_decoderState != STATE_HW_FAILED &&
      !(hints.codecOptions & CODEC_FORCE_SOFTWARE))
    pCodec = avcodec_find_decoder_by_name("av1");

  if (!pCodec)
    pCodec = avcodec_find_decoder(hints.codec);

  if(pCodec == NULL)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg::Open() Unable to find codec {}", hints.codec);
    return false;
  }

  CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg::Open() Using codec: {}",
            pCodec->long_name ? pCodec->long_name : pCodec->name);

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
    return false;

  m_pCodecContext->opaque = static_cast<ICallbackHWAccel*>(this);
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->get_format = GetFormat;
  m_pCodecContext->codec_tag = hints.codec_tag;

#if LIBAVCODEC_VERSION_MAJOR >= 60
  m_pCodecContext->flags = AV_CODEC_FLAG_COPY_OPAQUE;
#endif

  // setup threading model
  if (!(hints.codecOptions & CODEC_FORCE_SOFTWARE) && !CDVDFactoryCodec::GetHWAccels().empty())
  {
    if (m_decoderState == STATE_NONE)
    {
      m_decoderState = STATE_HW_SINGLE;
    }
    else
    {
      int num_threads = CServiceBroker::GetCPUInfo()->GetCPUCount();
      num_threads = std::max(1, std::min(num_threads, 8));
      m_pCodecContext->thread_count = num_threads;
      m_pCodecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
      m_decoderState = STATE_SW_MULTI;
      CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg - open frame threaded with {} threads",
                num_threads);
    }
  }
  else
  {
    int num_threads = CServiceBroker::GetCPUInfo()->GetCPUCount();
    num_threads = std::max(1, std::min(num_threads, 8));
#if defined(TARGET_WEBOS)
    // Utilize all 4 cores on webOS quad-core SoC for maximum software decoding throughput
    num_threads = std::min(num_threads, 4);
#endif
    m_pCodecContext->thread_count = num_threads;
    m_pCodecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
    m_decoderState = STATE_SW_MULTI;
    CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg - open software decoder with {} threads",
              num_threads);
  }

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;
  m_pCodecContext->bits_per_raw_sample = hints.bitdepth;

  if (hints.extradata)
  {
    m_pCodecContext->extradata =
        (uint8_t*)av_mallocz(hints.extradata.GetSize() + AV_INPUT_BUFFER_PADDING_SIZE);
    if (m_pCodecContext->extradata)
    {
      m_pCodecContext->extradata_size = hints.extradata.GetSize();
      memcpy(m_pCodecContext->extradata, hints.extradata.GetData(), hints.extradata.GetSize());
    }
  }

  // advanced setting override for skip loop filter (see avcodec.h for valid options)
  //! @todo allow per video setting?
  int iSkipLoopFilter = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iSkipLoopFilter;
  if (iSkipLoopFilter != 0)
  {
    m_pCodecContext->skip_loop_filter = static_cast<AVDiscard>(iSkipLoopFilter);
  }
#if defined(TARGET_WEBOS)
  m_pCodecContext->flags2 |= AV_CODEC_FLAG2_FAST;
  if (hints.width >= 2560 || hints.height >= 1440)
  {
    if (m_pCodecContext->skip_loop_filter == AVDISCARD_DEFAULT)
      m_pCodecContext->skip_loop_filter = AVDISCARD_ALL;
    CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg - 4K/QHD webOS optimizations enabled (skip_loop_filter=all, fast)");
  }
#endif
  m_defaultSkipLoopFilter = m_pCodecContext->skip_loop_filter;

  AVDictionary* codecOptions = nullptr;
#if defined(TARGET_WEBOS)
  if (pCodec->id == AV_CODEC_ID_HEVC)
  {
    av_dict_set(&codecOptions, "skip_sao", "1", 0);
    CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg - webOS HEVC optimization: skip_sao=1 enabled");
  }
#endif

  // set any special options
  for(std::vector<CDVDCodecOption>::iterator it = options.m_keys.begin(); it != options.m_keys.end(); ++it)
  {
    av_opt_set(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str(), 0);
    av_dict_set(&codecOptions, it->m_name.c_str(), it->m_value.c_str(), 0);
  }

  int openRet = avcodec_open2(m_pCodecContext, pCodec, &codecOptions);
  av_dict_free(&codecOptions);
  if (openRet < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

#if defined(TARGET_WEBOS)
  if (pCodec->id == AV_CODEC_ID_HEVC && m_pCodecContext->priv_data)
  {
    av_opt_set(m_pCodecContext->priv_data, "skip_sao", "1", 0);
  }
#endif

  m_pFrame = av_frame_alloc();
  if (!m_pFrame)
  {
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

  m_pDecodedFrame = av_frame_alloc();
  if (!m_pDecodedFrame)
  {
    av_frame_free(&m_pFrame);
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

  m_pFilterFrame = av_frame_alloc();
  if (!m_pFilterFrame)
  {
    av_frame_free(&m_pFrame);
    av_frame_free(&m_pDecodedFrame);
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

  UpdateName();
  const char* pixFmtName = av_get_pix_fmt_name(m_pCodecContext->pix_fmt);
  m_processInfo.SetVideoDimensions(m_pCodecContext->coded_width, m_pCodecContext->coded_height);
  m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");

  m_dropCtrl.Reset(true);
  m_eof = false;
  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  av_frame_free(&m_pFrame);
  av_frame_free(&m_pDecodedFrame);
  av_frame_free(&m_pFilterFrame);
  avcodec_free_context(&m_pCodecContext);

  if (m_pHardware)
  {
    m_pHardware->Release();
    m_pHardware = nullptr;
  }

  FilterClose();

  if (m_pConversionBufferPool)
  {
    av_buffer_pool_uninit(&m_pConversionBufferPool);
    m_pConversionBufferPool = nullptr;
  }
  m_conversionPoolWidth = 0;
  m_conversionPoolHeight = 0;
}

void CDVDVideoCodecFFmpeg::SetFilters()
{
  m_filters_next.clear();

  // 4K and QHD content is strictly progressive. Never run deinterlacing filters on 4K.
  if (m_pCodecContext->width >= 2560 || m_pCodecContext->height >= 1440 ||
      m_hints.width >= 2560 || m_hints.height >= 1440)
  {
    m_interlaced = false;
    return;
  }

  // ask codec to do deinterlacing if possible
  EINTERLACEMETHOD mInt = m_processInfo.GetVideoSettings().m_InterlaceMethod;

  if (!m_processInfo.Supports(mInt))
    mInt = m_processInfo.GetFallbackDeintMethod();

  unsigned int filters = 0;

  if (mInt != VS_INTERLACEMETHOD_NONE && m_interlaced)
  {
    if (mInt == VS_INTERLACEMETHOD_DEINTERLACE)
      filters = FILTER_DEINTERLACE_ANY;
    else if (mInt == VS_INTERLACEMETHOD_DEINTERLACE_HALF)
      filters = FILTER_DEINTERLACE_ANY | FILTER_DEINTERLACE_HALFED;

    if (filters)
      filters |= FILTER_DEINTERLACE_FLAGGED;
  }

  if (m_codecControlFlags & DVD_CODEC_CTRL_ROTATE)
    filters |= FILTER_ROTATE;

  m_filters_next.clear();

  if (filters & FILTER_ROTATE)
  {
    switch(m_iOrientation)
    {
      case 90:
        m_filters_next += "transpose=1";
        break;
      case 180:
        m_filters_next += "vflip,hflip";
        break;
      case 270:
        m_filters_next += "transpose=2";
        break;
      default:
        break;
      }
  }

  if (filters & FILTER_DEINTERLACE_BWDIF)
  {
    if (filters & FILTER_DEINTERLACE_HALFED)
      m_filters_next = "bwdif=0:-1";
    else
      m_filters_next = "bwdif=1:-1";

    if (filters & FILTER_DEINTERLACE_FLAGGED)
      m_filters_next += ":1";
  }
}

void CDVDVideoCodecFFmpeg::UpdateName()
{
  if(m_pCodecContext->codec->name)
    m_name = std::string("ff-") + m_pCodecContext->codec->name;
  else
    m_name = "ffmpeg";

  if(m_pHardware)
    m_name += "-" + m_pHardware->Name();

  m_processInfo.SetVideoDecoderName(m_name, m_pHardware ? true : false);

  CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg - Updated codec: {}", m_name);
}

#if LIBAVCODEC_VERSION_MAJOR < 60
union pts_union
{
  double pts_d;
  int64_t pts_i;
};

static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}
#endif

bool CDVDVideoCodecFFmpeg::AddData(const DemuxPacket &packet)
{
  if (!m_pCodecContext)
    return true;

  if (!packet.pData)
    return true;

  if (m_eof)
  {
    Reset();
  }

  if (packet.recoveryPoint)
    m_started = true;

  m_dts = packet.dts;

#if LIBAVCODEC_VERSION_MAJOR < 60
  m_pCodecContext->reordered_opaque = pts_dtoi(packet.pts);
#endif

  AVPacket* avpkt = av_packet_alloc();
  if (!avpkt)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  avpkt->data = packet.pData;
  avpkt->size = packet.iSize;
  avpkt->dts = (packet.dts == DVD_NOPTS_VALUE)
                   ? AV_NOPTS_VALUE
                   : static_cast<int64_t>(packet.dts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt->pts = (packet.pts == DVD_NOPTS_VALUE)
                   ? AV_NOPTS_VALUE
                   : static_cast<int64_t>(packet.pts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt->side_data = static_cast<AVPacketSideData*>(packet.pSideData);
  avpkt->side_data_elems = packet.iSideDataElems;

  int ret = avcodec_send_packet(m_pCodecContext, avpkt);

  //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
  // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
  // and storing our own AVPacket. This will require some extensive changes.
  av_buffer_unref(&avpkt->buf);
  av_free(avpkt);

  // try again
  if (ret == AVERROR(EAGAIN))
  {
    return false;
  }
  // error
  else if (ret)
  {
    // handle VC_NOBUFFER error for hw accel
    if (m_pHardware)
    {
      int result = m_pHardware->Check(m_pCodecContext);
      if (result == VC_NOBUFFER)
      {
        return false;
      }
    }
  }

  m_iLastKeyframe++;
  // put a limit on convergence count to avoid huge mem usage on streams without keyframes
  if (m_iLastKeyframe > 300)
    m_iLastKeyframe = 300;

  m_startedInput = true;

  return true;
}

CDVDVideoCodec::VCReturn CDVDVideoCodecFFmpeg::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_startedInput)
  {
    return VC_BUFFER;
  }
  else if (m_eof)
  {
    return VC_EOF;
  }

  // handle hw accelerators first, they may have frames ready
  if (m_pHardware)
  {
    int flags = m_codecControlFlags;
    flags &= ~DVD_CODEC_CTRL_DRAIN;
    m_pHardware->SetCodecControl(flags);
    CDVDVideoCodec::VCReturn ret = m_pHardware->Decode(m_pCodecContext, nullptr);
    if (ret == VC_PICTURE)
    {
      if (m_pHardware->GetPicture(m_pCodecContext, pVideoPicture))
        return VC_PICTURE;
      else
        return VC_ERROR;
    }
    else if (ret == VC_BUFFER)
      ;
    else
      return ret;
  }
  else if (m_pFilterGraph && !m_filterEof)
  {
    CDVDVideoCodec::VCReturn ret = FilterProcess(nullptr);
    if (ret == VC_PICTURE)
    {
      if (!SetPictureParams(pVideoPicture))
        return VC_ERROR;
      return VC_PICTURE;
    }
    else if (ret == VC_BUFFER)
      ;
    else
      return ret;
  }

  // process ffmpeg
  if (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)
  {
    AVPacket* avpkt = av_packet_alloc();
    if (!avpkt)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::{} - av_packet_alloc failed: {}", __FUNCTION__,
                strerror(errno));
      return VC_ERROR;
    }
    avpkt->data = nullptr;
    avpkt->size = 0;
    avpkt->dts = AV_NOPTS_VALUE;
    avpkt->pts = AV_NOPTS_VALUE;
    avcodec_send_packet(m_pCodecContext, avpkt);

    av_packet_free(&avpkt);
  }

  int ret = avcodec_receive_frame(m_pCodecContext, m_pDecodedFrame);

  if (m_decoderState == STATE_HW_FAILED && !m_pHardware)
    return VC_REOPEN;

  if(m_iLastKeyframe < m_pCodecContext->has_b_frames + 2)
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;

  if (ret == AVERROR_EOF)
  {
    // next drain hw accel or filter
    if (m_pHardware)
    {
      int flags = m_codecControlFlags;
      flags |= DVD_CODEC_CTRL_DRAIN;
      m_pHardware->SetCodecControl(flags);
      int ret = m_pHardware->Decode(m_pCodecContext, nullptr);
      if (ret == VC_PICTURE)
      {
        if (m_pHardware->GetPicture(m_pCodecContext, pVideoPicture))
          return VC_PICTURE;
        else
          return VC_ERROR;
      }
      else
      {
        m_eof = true;
        CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg::GetPicture - eof hw accel");
        return VC_EOF;
      }
    }
    else if (m_pFilterGraph && !m_filterEof)
    {
      int ret = FilterProcess(nullptr);
      if (ret == VC_PICTURE)
      {
        if (!SetPictureParams(pVideoPicture))
          return VC_ERROR;
        else
          return VC_PICTURE;
      }
      else
      {
        m_eof = true;
        CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg::GetPicture - eof filter");
        return VC_EOF;
      }
    }
    else
    {
      m_eof = true;
      CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg::GetPicture - eof");
      return VC_EOF;
    }
  }
  else if (ret == AVERROR(EAGAIN))
  {
    return VC_BUFFER;
  }
  else if (ret)
  {
    CLog::Log(LOGERROR, "{} - avcodec_receive_frame returned failure", __FUNCTION__);
    return VC_ERROR;
  }

  // here we got a frame
  int64_t framePTS = m_pDecodedFrame->best_effort_timestamp;

  if (m_pCodecContext->skip_frame > AVDISCARD_DEFAULT)
  {
    if (m_dropCtrl.m_state == CDropControl::VALID &&
        m_dropCtrl.m_lastPTS != AV_NOPTS_VALUE &&
        framePTS != AV_NOPTS_VALUE &&
        framePTS > (m_dropCtrl.m_lastPTS + m_dropCtrl.m_diffPTS * 1.5))
    {
      m_droppedFrames++;
      if (m_interlaced)
        m_droppedFrames++;
    }
  }
  m_dropCtrl.Process(framePTS, m_pCodecContext->skip_frame > AVDISCARD_DEFAULT);

  if (m_pDecodedFrame->key_frame)
  {
    m_started = true;
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;
  }
#if defined(TARGET_WEBOS)
  if (m_pDecodedFrame->width >= 2560 || m_pDecodedFrame->height >= 1440)
    m_interlaced = false;
  else if (m_pDecodedFrame->interlaced_frame)
    m_interlaced = true;
  else
    m_interlaced = false;
#else
  if (m_pDecodedFrame->interlaced_frame)
    m_interlaced = true;
  else
    m_interlaced = false;
#endif

  if (!m_processInfo.GetVideoInterlaced() && m_interlaced)
    m_processInfo.SetVideoInterlaced(m_interlaced);

#if defined(TARGET_WEBOS)
  if (m_pDecodedFrame->width >= 2560 || m_pDecodedFrame->height >= 1440)
  {
    if (m_defaultSkipLoopFilter == AVDISCARD_DEFAULT)
    {
      m_defaultSkipLoopFilter = AVDISCARD_ALL;
      m_pCodecContext->skip_loop_filter = AVDISCARD_ALL;
      CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg - 4K/QHD detected from decoded frame ({}x{}), skip_loop_filter=all enabled",
                m_pDecodedFrame->width, m_pDecodedFrame->height);
    }
    if (m_pCodecContext->priv_data)
    {
      av_opt_set(m_pCodecContext->priv_data, "skip_sao", "1", 0);
    }
  }
#endif

  if (!m_started)
  {
    int frames = 300;
    if (m_dropCtrl.m_state == CDropControl::VALID)
      frames = static_cast<int>(6000000 / m_dropCtrl.m_diffPTS);
    if (m_iLastKeyframe >= frames && m_pDecodedFrame->pict_type == AV_PICTURE_TYPE_I)
    {
      m_started = true;
    }
    else
    {
      av_frame_unref(m_pDecodedFrame);
      return VC_BUFFER;
    }
  }

  // push the frame to hw decoder for further processing
  if (m_pHardware)
  {
    av_frame_unref(m_pFrame);
    av_frame_move_ref(m_pFrame, m_pDecodedFrame);
    CDVDVideoCodec::VCReturn ret = m_pHardware->Decode(m_pCodecContext, m_pFrame);
    if (ret == VC_FLUSHED)
    {
      Reset();
      return ret;
    }
    else if (ret == VC_FATAL)
    {
      m_decoderState = STATE_HW_FAILED;
      return VC_REOPEN;
    }
    else if (ret == VC_PICTURE)
    {
      if (m_pHardware->GetPicture(m_pCodecContext, pVideoPicture))
        return VC_PICTURE;
      else
        return VC_ERROR;
    }

    return ret;
  }
  // process filters for sw decoding
  else
  {
    SetFilters();

    // Fast direct 10-bit & 12-bit YUV to 8-bit YUV conversion, plus 4K downscale (bypasses libavfilter entirely)
    const bool is10or12Bit = (m_pDecodedFrame->format == AV_PIX_FMT_YUV420P10LE ||
                              m_pDecodedFrame->format == AV_PIX_FMT_YUV420P10 ||
                              m_pDecodedFrame->format == AV_PIX_FMT_YUV420P12LE ||
                              m_pDecodedFrame->format == AV_PIX_FMT_YUV420P12);
#if defined(TARGET_WEBOS)
    const bool is4K8Bit = (m_pDecodedFrame->format == AV_PIX_FMT_YUV420P &&
                           (m_pDecodedFrame->width >= 2560 || m_pDecodedFrame->height >= 1440));
#else
    const bool is4K8Bit = false;
#endif

    if (m_filters_next.empty() && (is10or12Bit || is4K8Bit))
    {
      if (m_pFilterGraph)
        FilterClose();

      const int w = m_pDecodedFrame->width;
      const int h = m_pDecodedFrame->height;

#if defined(TARGET_WEBOS)
      const bool downscale4K = (w >= 2560 || h >= 1440);
#else
      const bool downscale4K = false;
#endif

      const int out_w = downscale4K ? ((w / 2) & ~1) : w;
      const int out_h = downscale4K ? ((h / 2) & ~1) : h;

      // Reinitialize buffer pool if dimensions changed
      if (!m_pConversionBufferPool || m_conversionPoolWidth != out_w || m_conversionPoolHeight != out_h)
      {
        if (m_pConversionBufferPool)
          av_buffer_pool_uninit(&m_pConversionBufferPool);

        int bufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, out_w, out_h, 32);
        if (bufSize > 0)
        {
          m_pConversionBufferPool = av_buffer_pool_init(bufSize, nullptr);
          m_conversionPoolWidth = out_w;
          m_conversionPoolHeight = out_h;
          CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg - initialized conversion buffer pool for {}x{} (src {}x{}, bufSize={})",
                    out_w, out_h, w, h, bufSize);
        }
      }

      AVBufferRef* buf = m_pConversionBufferPool ? av_buffer_pool_get(m_pConversionBufferPool) : nullptr;
      if (buf)
      {
        av_frame_unref(m_pFrame);
        av_frame_copy_props(m_pFrame, m_pDecodedFrame);
        m_pFrame->format = AV_PIX_FMT_YUV420P;
        m_pFrame->width = out_w;
        m_pFrame->height = out_h;

        av_image_fill_arrays(m_pFrame->data, m_pFrame->linesize, buf->data, AV_PIX_FMT_YUV420P, out_w, out_h, 32);
        m_pFrame->buf[0] = buf;

        const bool is12Bit = (m_pDecodedFrame->format == AV_PIX_FMT_YUV420P12LE ||
                              m_pDecodedFrame->format == AV_PIX_FMT_YUV420P12);
        const bool is8Bit = (m_pDecodedFrame->format == AV_PIX_FMT_YUV420P);

        if (downscale4K)
        {
          if (is8Bit)
          {
            // 8-bit Y plane
            for (int y = 0; y < out_h; y++)
            {
              const uint8_t* src_y = m_pDecodedFrame->data[0] + (y * 2) * m_pDecodedFrame->linesize[0];
              uint8_t* dst_y = m_pFrame->data[0] + y * m_pFrame->linesize[0];
              DownscaleYUV8RowTo8(src_y, dst_y, out_w);
            }

            const int uv_w = (out_w + 1) / 2;
            const int uv_h = (out_h + 1) / 2;
            for (int y = 0; y < uv_h; y++)
            {
              const uint8_t* src_u = m_pDecodedFrame->data[1] + (y * 2) * m_pDecodedFrame->linesize[1];
              uint8_t* dst_u = m_pFrame->data[1] + y * m_pFrame->linesize[1];
              DownscaleYUV8RowTo8(src_u, dst_u, uv_w);
            }

            for (int y = 0; y < uv_h; y++)
            {
              const uint8_t* src_v = m_pDecodedFrame->data[2] + (y * 2) * m_pDecodedFrame->linesize[2];
              uint8_t* dst_v = m_pFrame->data[2] + y * m_pFrame->linesize[2];
              DownscaleYUV8RowTo8(src_v, dst_v, uv_w);
            }
          }
          else
          {
            // 10-bit / 12-bit Y plane
            for (int y = 0; y < out_h; y++)
            {
              const uint16_t* src_y = reinterpret_cast<const uint16_t*>(m_pDecodedFrame->data[0] + (y * 2) * m_pDecodedFrame->linesize[0]);
              uint8_t* dst_y = m_pFrame->data[0] + y * m_pFrame->linesize[0];
              if (is12Bit)
                DownscaleYUV12RowTo8(src_y, dst_y, out_w);
              else
                DownscaleYUV10RowTo8(src_y, dst_y, out_w);
            }

            const int uv_w = (out_w + 1) / 2;
            const int uv_h = (out_h + 1) / 2;
            for (int y = 0; y < uv_h; y++)
            {
              const uint16_t* src_u = reinterpret_cast<const uint16_t*>(m_pDecodedFrame->data[1] + (y * 2) * m_pDecodedFrame->linesize[1]);
              uint8_t* dst_u = m_pFrame->data[1] + y * m_pFrame->linesize[1];
              if (is12Bit)
                DownscaleYUV12RowTo8(src_u, dst_u, uv_w);
              else
                DownscaleYUV10RowTo8(src_u, dst_u, uv_w);
            }

            for (int y = 0; y < uv_h; y++)
            {
              const uint16_t* src_v = reinterpret_cast<const uint16_t*>(m_pDecodedFrame->data[2] + (y * 2) * m_pDecodedFrame->linesize[2]);
              uint8_t* dst_v = m_pFrame->data[2] + y * m_pFrame->linesize[2];
              if (is12Bit)
                DownscaleYUV12RowTo8(src_v, dst_v, uv_w);
              else
                DownscaleYUV10RowTo8(src_v, dst_v, uv_w);
            }
          }
        }
        else
        {
          // 1:1 conversion for 10-bit/12-bit (<= 1080p)
          for (int y = 0; y < h; y++)
          {
            const uint16_t* src_y = reinterpret_cast<const uint16_t*>(m_pDecodedFrame->data[0] + y * m_pDecodedFrame->linesize[0]);
            uint8_t* dst_y = m_pFrame->data[0] + y * m_pFrame->linesize[0];
            if (is12Bit)
              ConvertYUV12RowTo8(src_y, dst_y, w);
            else
              ConvertYUV10RowTo8(src_y, dst_y, w);
          }

          const int uv_w = (w + 1) / 2;
          const int uv_h = (h + 1) / 2;
          for (int y = 0; y < uv_h; y++)
          {
            const uint16_t* src_u = reinterpret_cast<const uint16_t*>(m_pDecodedFrame->data[1] + y * m_pDecodedFrame->linesize[1]);
            uint8_t* dst_u = m_pFrame->data[1] + y * m_pFrame->linesize[1];
            if (is12Bit)
              ConvertYUV12RowTo8(src_u, dst_u, uv_w);
            else
              ConvertYUV10RowTo8(src_u, dst_u, uv_w);
          }

          for (int y = 0; y < uv_h; y++)
          {
            const uint16_t* src_v = reinterpret_cast<const uint16_t*>(m_pDecodedFrame->data[2] + y * m_pDecodedFrame->linesize[2]);
            uint8_t* dst_v = m_pFrame->data[2] + y * m_pFrame->linesize[2];
            if (is12Bit)
              ConvertYUV12RowTo8(src_v, dst_v, uv_w);
            else
              ConvertYUV10RowTo8(src_v, dst_v, uv_w);
          }
        }

        av_frame_unref(m_pDecodedFrame);

        if (!SetPictureParams(pVideoPicture))
          return VC_ERROR;
        else
        {
          pVideoPicture->pixelFormat = AV_PIX_FMT_YUV420P;
          pVideoPicture->colorBits = 8;
          return VC_PICTURE;
        }
      }
      else
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::GetPicture - failed to get buffer from conversion pool");
        return VC_ERROR;
      }
    }

    bool need_scale = std::find(m_formats.begin(),
                                m_formats.end(),
                                m_pCodecContext->pix_fmt) == m_formats.end();

    bool need_reopen = false;
    if (m_filters != m_filters_next)
      need_reopen = true;

    if (!m_filters_next.empty() && m_filterEof)
      need_reopen = true;

    if (m_pFilterIn && m_pFilterIn->outputs && m_pFilterIn->nb_outputs > 0 && m_pFilterIn->outputs[0])
    {
      if (m_pFilterIn->outputs[0]->format != m_pCodecContext->pix_fmt ||
          m_pFilterIn->outputs[0]->w != m_pCodecContext->width ||
          m_pFilterIn->outputs[0]->h != m_pCodecContext->height)
        need_reopen = true;
    }

    // try to setup new filters
    if (need_reopen || (need_scale && m_pFilterGraph == nullptr))
    {
      m_filters = m_filters_next;

      if (FilterOpen(m_filters, need_scale) < 0)
        FilterClose();
    }

    if (m_pFilterGraph && !m_filterEof)
    {
      CDVDVideoCodec::VCReturn ret = FilterProcess(m_pDecodedFrame);
      if (ret != VC_PICTURE)
        return VC_NONE;
    }
    else
    {
      av_frame_unref(m_pFrame);
      av_frame_move_ref(m_pFrame, m_pDecodedFrame);
    }

    if (!SetPictureParams(pVideoPicture))
      return VC_ERROR;
    else
      return VC_PICTURE;
  }

  return VC_NONE;
}

bool CDVDVideoCodecFFmpeg::SetPictureParams(VideoPicture* pVideoPicture)
{
  if (!GetPictureCommon(pVideoPicture))
    return false;

  pVideoPicture->iFlags |= m_pFrame->data[0] ? 0 : DVP_FLAG_DROPPED;

  if (pVideoPicture->videoBuffer)
    pVideoPicture->videoBuffer->Release();
  pVideoPicture->videoBuffer = nullptr;

  CVideoBufferFFmpeg *buffer = dynamic_cast<CVideoBufferFFmpeg*>(m_videoBufferPool->Get());
  buffer->SetRef(m_pFrame);
  pVideoPicture->videoBuffer = buffer;

  if (m_processInfo.GetVideoSettings().m_PostProcess)
  {
    m_postProc.SetType(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoPPFFmpegPostProc, false);
    m_postProc.Process(pVideoPicture);
  }

  return true;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  m_started = false;
  m_startedInput = false;
  m_interlaced = false;
  m_decoderPts = DVD_NOPTS_VALUE;
  m_skippedDeint = 0;
  m_droppedFrames = 0;
  m_eof = false;
  m_iLastKeyframe = m_pCodecContext->has_b_frames;
  avcodec_flush_buffers(m_pCodecContext);
  av_frame_unref(m_pFrame);

  if (m_pHardware)
    m_pHardware->Reset();

  m_filters = "";
  FilterClose();
  m_dropCtrl.Reset(false);
}

void CDVDVideoCodecFFmpeg::Reopen()
{
  Dispose();
  if (!Open(m_hints, m_options))
  {
    Dispose();
  }
}

bool CDVDVideoCodecFFmpeg::GetPictureCommon(VideoPicture* pVideoPicture)
{
  if (!m_pFrame)
    return false;

  pVideoPicture->iWidth = m_pFrame->width;
  pVideoPicture->iHeight = m_pFrame->height;

  /* crop of 10 pixels if demuxer asked it */
  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < (int)pVideoPicture->iWidth
                                   && m_pCodecContext->coded_width  > (int)pVideoPicture->iWidth  - 10)
    pVideoPicture->iWidth = m_pCodecContext->coded_width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < (int)pVideoPicture->iHeight
                                   && m_pCodecContext->coded_height > (int)pVideoPicture->iHeight - 10)
    pVideoPicture->iHeight = m_pCodecContext->coded_height;

  double aspect_ratio;

  /* use variable in the frame */
  AVRational pixel_aspect = m_pFrame->sample_aspect_ratio;

  if (pixel_aspect.num == 0)
    aspect_ratio = 0;
  else
    aspect_ratio = av_q2d(pixel_aspect) * pVideoPicture->iWidth / pVideoPicture->iHeight;

  if (aspect_ratio <= 0.0)
    aspect_ratio = static_cast<double>(pVideoPicture->iWidth) / pVideoPicture->iHeight;

  if (m_DAR != aspect_ratio)
  {
    m_DAR = aspect_ratio;
    m_processInfo.SetVideoDAR(static_cast<float>(m_DAR));
  }

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  pVideoPicture->iDisplayHeight = pVideoPicture->iHeight;
  pVideoPicture->iDisplayWidth  = ((int)RINT(pVideoPicture->iHeight * aspect_ratio)) & -3;
  if (pVideoPicture->iDisplayWidth > pVideoPicture->iWidth)
  {
    pVideoPicture->iDisplayWidth  = pVideoPicture->iWidth;
    pVideoPicture->iDisplayHeight = ((int)RINT(pVideoPicture->iWidth / aspect_ratio)) & -3;
  }


  pVideoPicture->pts = DVD_NOPTS_VALUE;

  AVDictionaryEntry * entry = av_dict_get(m_pFrame->metadata, "stereo_mode", NULL, 0);
  if(entry && entry->value)
  {
    pVideoPicture->stereoMode = (const char*)entry->value;
  }
  else
    pVideoPicture->stereoMode.clear();

  pVideoPicture->iRepeatPicture = 0.5 * m_pFrame->repeat_pict;
  pVideoPicture->iFlags = 0;
#if defined(TARGET_WEBOS)
  const bool isOrig4K = (m_hints.width >= 2560 || m_hints.height >= 1440 ||
                         m_pCodecContext->width >= 2560 || m_pCodecContext->height >= 1440);
  if (!isOrig4K)
  {
    pVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
    pVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST : 0;
  }
#else
  pVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST : 0;
#endif

  if (m_codecControlFlags & DVD_CODEC_CTRL_DROP)
  {
    pVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  }

  pVideoPicture->pixelFormat = m_pCodecContext->sw_pix_fmt;

  pVideoPicture->chroma_position = m_pCodecContext->chroma_sample_location;
  pVideoPicture->color_primaries = m_pCodecContext->color_primaries == AVCOL_PRI_UNSPECIFIED ? m_hints.colorPrimaries : m_pCodecContext->color_primaries;
  pVideoPicture->m_originalColorPrimaries = pVideoPicture->color_primaries;
  pVideoPicture->color_transfer = m_pCodecContext->color_trc == AVCOL_TRC_UNSPECIFIED ? m_hints.colorTransferCharacteristic : m_pCodecContext->color_trc;
  pVideoPicture->color_space = m_pCodecContext->colorspace == AVCOL_SPC_UNSPECIFIED ? m_hints.colorSpace : m_pCodecContext->colorspace;
  pVideoPicture->colorBits = 8;

  // determine how number of bits of encoded video
  if (m_pCodecContext->pix_fmt == AV_PIX_FMT_YUV420P12)
    pVideoPicture->colorBits = 12;
  else if (m_pCodecContext->pix_fmt == AV_PIX_FMT_YUV420P10)
    pVideoPicture->colorBits = 10;
  else if (m_pCodecContext->codec_id == AV_CODEC_ID_HEVC &&
           m_pCodecContext->profile == FF_PROFILE_HEVC_MAIN_10)
    pVideoPicture->colorBits = 10;
  else if (m_pCodecContext->codec_id == AV_CODEC_ID_H264 &&
           (m_pCodecContext->profile == FF_PROFILE_H264_HIGH_10||
            m_pCodecContext->profile == FF_PROFILE_H264_HIGH_10_INTRA))
    pVideoPicture->colorBits = 10;
  else if ((m_pCodecContext->codec_id == AV_CODEC_ID_VP9 ||
            m_pCodecContext->codec_id == AV_CODEC_ID_AV1) &&
           m_pCodecContext->sw_pix_fmt == AV_PIX_FMT_YUV420P10)
    pVideoPicture->colorBits = 10;

  if (m_pCodecContext->color_range == AVCOL_RANGE_JPEG ||
    m_pCodecContext->pix_fmt == AV_PIX_FMT_YUVJ420P)
    pVideoPicture->color_range = 1;
  else
    pVideoPicture->color_range = m_hints.colorRange == AVCOL_RANGE_JPEG ? 1 : 0;

  //! @todo: ffmpeg doesn't seem like they know how they want to handle this.
  // av_frame_get_qp_table is deprecated but there doesn't seem to be a valid
  // replacement. the following is basically what av_frame_get_qp_table does
  // internally so we can avoid the deprecation warning however it may still
  // break in the future because some definitions are guarded and may be removed.

  pVideoPicture->qp_table = nullptr;
  pVideoPicture->qstride = 0;
  pVideoPicture->qscale_type = 0;

  pVideoPicture->hdrType = m_hints.hdrType;

  AVFrameSideData* sd;

  // https://github.com/FFmpeg/FFmpeg/blob/991d417692/doc/APIchanges#L18-L20
  sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_VIDEO_ENC_PARAMS);
  if (sd)
  {
    unsigned int mb_h = (m_pFrame->height + 15) / 16;
    unsigned int mb_w = (m_pFrame->width + 15) / 16;
    unsigned int nb_mb = mb_h * mb_w;
    unsigned int block_idx;

    auto par = reinterpret_cast<AVVideoEncParams*>(sd->data);
    if (par->type == AV_VIDEO_ENC_PARAMS_MPEG2 && (par->nb_blocks == 0 || par->nb_blocks == nb_mb))
    {
      pVideoPicture->qstride = mb_w;
      pVideoPicture->qscale_type = par->type;
      pVideoPicture->qp_table = static_cast<int8_t*>(av_malloc(nb_mb));
      for (block_idx = 0; block_idx < nb_mb; block_idx++)
      {
        AVVideoBlockParams* b = av_video_enc_params_block(par, block_idx);
        pVideoPicture->qp_table[block_idx] = par->qp + b->delta_qp;
      }
    }
  }

  pVideoPicture->pict_type = m_pFrame->pict_type;

  // metadata
  pVideoPicture->hasDisplayMetadata = false;
  pVideoPicture->hasLightMetadata = false;
  sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
  if (sd)
  {
    pVideoPicture->displayMetadata = *(AVMasteringDisplayMetadata *)sd->data;
    pVideoPicture->hasDisplayMetadata = true;
  }
  else if (m_hints.masteringMetadata)
  {
    pVideoPicture->displayMetadata = *m_hints.masteringMetadata.get();
    pVideoPicture->hasDisplayMetadata = true;
  }
  sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
  if (sd)
  {
    pVideoPicture->lightMetadata = *(AVContentLightMetadata *)sd->data;
    pVideoPicture->hasLightMetadata = true;
  }
  else if (m_hints.contentLightMetadata)
  {
    pVideoPicture->lightMetadata = *m_hints.contentLightMetadata.get();
    pVideoPicture->hasLightMetadata = true;
  }

  if (pVideoPicture->iRepeatPicture)
    pVideoPicture->dts = DVD_NOPTS_VALUE;
  else
    pVideoPicture->dts = m_dts;

  m_dts = DVD_NOPTS_VALUE;

  int64_t bpts = m_pFrame->best_effort_timestamp;
  if (bpts != AV_NOPTS_VALUE)
  {
    pVideoPicture->pts = (double)bpts * DVD_TIME_BASE / AV_TIME_BASE;
    if (pVideoPicture->pts == m_decoderPts)
    {
      pVideoPicture->iRepeatPicture = -0.5;
      pVideoPicture->pts = DVD_NOPTS_VALUE;
      pVideoPicture->dts = DVD_NOPTS_VALUE;
    }
  }
  else
    pVideoPicture->pts = DVD_NOPTS_VALUE;

  if (pVideoPicture->pts != DVD_NOPTS_VALUE)
    m_decoderPts = pVideoPicture->pts;

  if (m_requestSkipDeint)
  {
    pVideoPicture->iFlags |= DVD_CODEC_CTRL_SKIPDEINT;
    m_skippedDeint++;
  }

  m_requestSkipDeint = false;
  pVideoPicture->iFlags |= m_codecControlFlags;

  if (pVideoPicture->color_primaries == AVCOL_PRI_UNSPECIFIED)
  {
    if (pVideoPicture->iDisplayWidth > 1024 || pVideoPicture->iDisplayHeight >= 600)
      pVideoPicture->color_primaries = AVCOL_PRI_BT709;
    else
      pVideoPicture->color_primaries = AVCOL_PRI_BT470BG;
  }

  return true;
}

int CDVDVideoCodecFFmpeg::FilterOpen(const std::string& filters, bool scale)
{
  int result;

  if (m_pFilterGraph)
    FilterClose();

  if (filters.empty() && !scale)
    return 0;

  if (m_pHardware)
  {
    CLog::Log(LOGWARNING, "CDVDVideoCodecFFmpeg::FilterOpen - skipped opening filters on hardware decode");
    return 0;
  }

  if (!(m_pFilterGraph = avfilter_graph_alloc()))
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - unable to alloc filter graph");
    return -1;
  }

  int num_threads = CServiceBroker::GetCPUInfo()->GetCPUCount();
  m_pFilterGraph->nb_threads = std::max(1, std::min(num_threads, 8));
  m_pFilterGraph->scale_sws_opts = av_strdup("flags=fast_bilinear");
  CLog::Log(LOGINFO, "CDVDVideoCodecFFmpeg::FilterOpen - configured filter graph with {} threads and fast_bilinear",
            m_pFilterGraph->nb_threads);

  const AVFilter* srcFilter = avfilter_get_by_name("buffer");
  const AVFilter* outFilter = avfilter_get_by_name("buffersink"); // should be last filter in the graph for now

  std::string args = StringUtils::Format(
      "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}", m_pCodecContext->width,
      m_pCodecContext->height, m_pCodecContext->pix_fmt,
      m_pCodecContext->time_base.num ? m_pCodecContext->time_base.num : 1,
      m_pCodecContext->time_base.num ? m_pCodecContext->time_base.den : 1,
      m_pCodecContext->sample_aspect_ratio.num != 0 ? m_pCodecContext->sample_aspect_ratio.num : 1,
      m_pCodecContext->sample_aspect_ratio.num != 0 ? m_pCodecContext->sample_aspect_ratio.den : 1);

  if ((result = avfilter_graph_create_filter(&m_pFilterIn, srcFilter, "src", args.c_str(), NULL, m_pFilterGraph)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_create_filter: src");
    return result;
  }

  if ((result = avfilter_graph_create_filter(&m_pFilterOut, outFilter, "out", NULL, NULL, m_pFilterGraph)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_create_filter: out");
    return result;
  }
  if ((result = av_opt_set_int_list(m_pFilterOut, "pix_fmts", &m_formats[0],  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - failed settings pix formats");
    return result;
  }

  if (!filters.empty())
  {
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs  = avfilter_inout_alloc();

    outputs->name = av_strdup("in");
    outputs->filter_ctx = m_pFilterIn;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = m_pFilterOut;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    result = avfilter_graph_parse_ptr(m_pFilterGraph, m_filters.c_str(), &inputs, &outputs, NULL);
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);

    if (result < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_parse");
      return result;
    }

    if (filters.compare(0,5,"bwdif") == 0)
    {
      m_processInfo.SetVideoDeintMethod(filters);
    }
    else
    {
      m_processInfo.SetVideoDeintMethod("none");
    }
  }
  else
  {
    if ((result = avfilter_link(m_pFilterIn, 0, m_pFilterOut, 0)) < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_link");
      return result;
    }

    m_processInfo.SetVideoDeintMethod("none");
  }

  if ((result = avfilter_graph_config(m_pFilterGraph,  nullptr)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_config");
    return result;
  }

  if (CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
  {
    char* graphDump = avfilter_graph_dump(m_pFilterGraph, nullptr);
    if (graphDump)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg::FilterOpen - Final filter graph:\n{}", graphDump);
      av_freep(&graphDump);
    }
  }

  m_filterEof = false;
  return result;
}

void CDVDVideoCodecFFmpeg::FilterClose()
{
  if (m_pFilterGraph)
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecFFmpeg::FilterClose - Freeing filter graph");
    avfilter_graph_free(&m_pFilterGraph);

    // Disposed by above code
    m_pFilterIn = nullptr;
    m_pFilterOut = nullptr;
  }
}

CDVDVideoCodec::VCReturn CDVDVideoCodecFFmpeg::FilterProcess(AVFrame* frame)
{
  int result;

  if (frame || (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN))
  {
    result = av_buffersrc_add_frame(m_pFilterIn, frame);
    if (result < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - av_buffersrc_add_frame");
      return VC_ERROR;
    }
  }

  result = av_buffersink_get_frame(m_pFilterOut, m_pFilterFrame);

  if (result  == AVERROR(EAGAIN))
    return VC_BUFFER;
  else if (result == AVERROR_EOF)
  {
    result = av_buffersink_get_frame(m_pFilterOut, m_pFilterFrame);
    m_filterEof = true;
    if (result < 0)
      return VC_BUFFER;
  }
  else if (result < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - av_buffersink_get_frame");
    return VC_ERROR;
  }

  av_frame_unref(m_pFrame);
  av_frame_move_ref(m_pFrame, m_pFilterFrame);

  return VC_PICTURE;
}

unsigned CDVDVideoCodecFFmpeg::GetConvergeCount()
{
  return m_iLastKeyframe;
}

unsigned CDVDVideoCodecFFmpeg::GetAllowedReferences()
{
  if(m_pHardware)
    return m_pHardware->GetAllowedReferences();
  else
    return 0;
}

bool CDVDVideoCodecFFmpeg::GetCodecStats(double &pts, int &droppedFrames, int &skippedPics)
{
  if (m_decoderPts != DVD_NOPTS_VALUE)
    pts = m_decoderPts;
  else
    pts = m_dts;

  if (m_droppedFrames)
    droppedFrames = m_droppedFrames;
  else
    droppedFrames = -1;
  m_droppedFrames = 0;

  if (m_skippedDeint)
    skippedPics = m_skippedDeint;
  else
    skippedPics = -1;
  m_skippedDeint = 0;

  return true;
}

void CDVDVideoCodecFFmpeg::SetCodecControl(int flags)
{
  m_codecControlFlags = flags;

  if (m_pCodecContext)
  {
    bool bDrop = (flags & (DVD_CODEC_CTRL_DROP_ANY | DVD_CODEC_CTRL_DROP)) != 0;
    if (bDrop && m_pHardware && m_pHardware->CanSkipDeint())
    {
      m_requestSkipDeint = true;
      bDrop = false;
    }
    else
      m_requestSkipDeint = false;

    if (bDrop)
    {
      m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      m_pCodecContext->skip_idct = AVDISCARD_NONREF;
      m_pCodecContext->skip_loop_filter = AVDISCARD_ALL;
    }
    else
    {
      m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_loop_filter = m_defaultSkipLoopFilter;
    }
  }

  if (m_pHardware)
    m_pHardware->SetCodecControl(flags);
}

void CDVDVideoCodecFFmpeg::SetHardware(IHardwareDecoder* hardware)
{
  if (m_pHardware)
    m_pHardware->Release();
  m_pHardware = hardware;
  UpdateName();
}

IHardwareDecoder* CDVDVideoCodecFFmpeg::GetHWAccel()
{
  return m_pHardware;
}

