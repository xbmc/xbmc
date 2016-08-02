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

#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include "DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "utils/CPUInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/VideoSettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
#include <memory>

#ifndef TARGET_POSIX
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#else
#include <math.h>
#define RINT lrint
#endif

#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFormats.h"

#ifdef HAVE_LIBVDPAU
#include "VDPAU.h"
#endif
#ifdef HAS_DX
#include "DXVA.h"
#endif
#ifdef HAVE_LIBVA
#include "VAAPI.h"
#endif
#ifdef TARGET_DARWIN
#include "VTB.h"
#endif
#ifdef HAS_MMAL
#include "MMALFFmpeg.h"
#endif
#include "utils/StringUtils.h"

extern "C" {
#include "libavutil/opt.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/pixdesc.h"
}

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
  FILTER_DEINTERLACE_YADIF   =  0x1,  //< use first deinterlace mode
  FILTER_DEINTERLACE_ANY     =  0xf,  //< use any deinterlace mode
  FILTER_DEINTERLACE_FLAGGED = 0x10,  //< only deinterlace flagged frames
  FILTER_DEINTERLACE_HALFED  = 0x20,  //< do half rate deinterlacing
  FILTER_ROTATE              = 0x40,  //< rotate image according to the codec hints
};

enum AVPixelFormat CDVDVideoCodecFFmpeg::GetFormat( struct AVCodecContext * avctx, const AVPixelFormat * fmt)
{
  CDVDVideoCodecFFmpeg* ctx  = (CDVDVideoCodecFFmpeg*)avctx->opaque;

  const char* pixFmtName = av_get_pix_fmt_name(*fmt);

  // if frame threading is enabled hw accel is not allowed
  if(ctx->m_decoderState != STATE_HW_SINGLE)
  {
    AVPixelFormat defaultFmt = avcodec_default_get_format(avctx, fmt);
    pixFmtName = av_get_pix_fmt_name(defaultFmt);
    ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
    return defaultFmt;
  }

  // fix an ffmpeg issue here, it calls us with an invalid profile
  // then a 2nd call with a valid one
  if (avctx->codec_id == AV_CODEC_ID_VC1 && avctx->profile == FF_PROFILE_UNKNOWN)
  {
    return avcodec_default_get_format(avctx, fmt);
  }

  // hardware decoder de-selected, restore standard ffmpeg
  if (ctx->GetHardware())
  {
    ctx->SetHardware(NULL);
    avctx->get_buffer2 = avcodec_default_get_buffer2;
    avctx->slice_flags = 0;
    avctx->hwaccel_context = 0;
  }

  const AVPixelFormat * cur = fmt;
  while(*cur != AV_PIX_FMT_NONE)
  {
    pixFmtName = av_get_pix_fmt_name(*cur);

#ifdef HAVE_LIBVDPAU
    if(VDPAU::CDecoder::IsVDPAUFormat(*cur) && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVDPAU))
    {
      CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::GetFormat - Creating VDPAU(%ix%i)", avctx->width, avctx->height);
      VDPAU::CDecoder* vdp = new VDPAU::CDecoder(ctx->m_processInfo);
      if(vdp->Open(avctx, ctx->m_pCodecContext, *cur, ctx->m_uSurfacesCount))
      {
        ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
        ctx->SetHardware(vdp);
        return *cur;
      }
      else
        vdp->Release();
    }
#endif
#ifdef HAS_DX
  if(DXVA::CDecoder::Supports(*cur) && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDXVA2) &&
     !ctx->m_hints.dvd && !ctx->m_hints.stills)
  {
    CLog::Log(LOGNOTICE, "CDVDVideoCodecFFmpeg::GetFormat - Creating DXVA(%ix%i)", avctx->width, avctx->height);
    DXVA::CDecoder* dec = new DXVA::CDecoder(ctx->m_processInfo);
    if(dec->Open(avctx, ctx->m_pCodecContext, *cur, ctx->m_uSurfacesCount))
    {
      ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
      ctx->SetHardware(dec);
      return *cur;
    }
    else
      dec->Release();
  }
#endif
#ifdef HAVE_LIBVA
    // mpeg4 vaapi decoding is disabled
    if(*cur == AV_PIX_FMT_VAAPI_VLD && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVAAPI))
    {
      VAAPI::CDecoder* dec = new VAAPI::CDecoder(ctx->m_processInfo);
      if(dec->Open(avctx, ctx->m_pCodecContext, *cur, ctx->m_uSurfacesCount) == true)
      {
        ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
        ctx->SetHardware(dec);
        return *cur;
      }
      else
        dec->Release();
    }
#endif

#ifdef TARGET_DARWIN
    if (*cur == AV_PIX_FMT_VIDEOTOOLBOX && CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVTB))
    {
      VTB::CDecoder* dec = new VTB::CDecoder(ctx->m_processInfo);
      if(dec->Open(avctx, ctx->m_pCodecContext, *cur, ctx->m_uSurfacesCount))
      {
        ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
        ctx->SetHardware(dec);
        return *cur;
      }
      else
        dec->Release();
    }
#endif

#ifdef HAS_MMAL
    if (*cur == AV_PIX_FMT_YUV420P)
    {
      MMAL::CDecoder* dec = new MMAL::CDecoder();
      if(dec->Open(avctx, ctx->m_pCodecContext, *cur, ctx->m_uSurfacesCount))
      {
        ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
        ctx->SetHardware(dec);
        return *cur;
      }
      else
        dec->Release();
    }
#endif
    cur++;
  }

  ctx->m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
  ctx->m_decoderState = STATE_HW_FAILED;
  return avcodec_default_get_format(avctx, fmt);
}

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg(CProcessInfo &processInfo) : CDVDVideoCodec(processInfo)
{
  m_pCodecContext = nullptr;
  m_pFrame = nullptr;
  m_pDecodedFrame = nullptr;
  m_pFilterGraph = nullptr;
  m_pFilterIn = nullptr;
  m_pFilterOut = nullptr;
  m_pFilterFrame = nullptr;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

  m_uSurfacesCount = 0;

  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
  m_iOrientation = 0;
  m_decoderState = STATE_NONE;
  m_pHardware = nullptr;
  m_iLastKeyframe = 0;
  m_dts = DVD_NOPTS_VALUE;
  m_started = false;
  m_decoderPts = DVD_NOPTS_VALUE;
  m_codecControlFlags = 0;
  m_requestSkipDeint = false;
  m_skippedDeint = 0;
  m_droppedFrames = 0;
  m_interlaced = false;
  m_DAR = 1.0;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_hints = hints;
  m_options = options;

  AVCodec* pCodec;

  m_iOrientation = hints.orientation;

  for(std::vector<ERenderFormat>::iterator it = options.m_formats.begin(); it != options.m_formats.end(); ++it)
  {
    m_formats.push_back((AVPixelFormat)CDVDCodecUtils::PixfmtFromEFormat(*it));
    if(*it == RENDER_FMT_YUV420P)
      m_formats.push_back(AV_PIX_FMT_YUVJ420P);
  }
  m_formats.push_back(AV_PIX_FMT_NONE); /* always add none to get a terminated list in ffmpeg world */

  pCodec = avcodec_find_decoder(hints.codec);

  if(pCodec == NULL)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Using codec: %s",pCodec->long_name ? pCodec->long_name : pCodec->name);

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
    return false;

  m_pCodecContext->opaque = (void*)this;
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->get_format = GetFormat;
  m_pCodecContext->codec_tag = hints.codec_tag;

  // setup threading model
  if (!hints.software)
  {
    bool tryhw = false;
#ifdef HAVE_LIBVDPAU
    if(CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVDPAU))
      tryhw = true;
#endif
#ifdef HAVE_LIBVA
    if(CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVAAPI))
      tryhw = true;
#endif
#ifdef HAS_DX
    if(CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDXVA2))
      tryhw = true;
#endif
#ifdef TARGET_DARWIN
    if(CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVTB))
      tryhw = true;
#endif
#ifdef HAS_MMAL
    tryhw = true;
#endif
    if (tryhw && m_decoderState == STATE_NONE)
    {
      m_decoderState = STATE_HW_SINGLE;
    }
    else
    {
      int num_threads = g_cpuInfo.getCPUCount() * 3 / 2;
      num_threads = std::max(1, std::min(num_threads, 16));
      m_pCodecContext->thread_count = num_threads;
      m_pCodecContext->thread_safe_callbacks = 1;
      m_decoderState = STATE_SW_MULTI;
      CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg - open frame threaded with %d threads", num_threads);
    }
  }
  else
    m_decoderState = STATE_SW_SINGLE;

#if defined(TARGET_DARWIN_IOS)
  // ffmpeg with enabled neon will crash and burn if this is enabled
  m_pCodecContext->flags &= CODEC_FLAG_EMU_EDGE;
#else
  if (pCodec->id != AV_CODEC_ID_H264 && pCodec->capabilities & CODEC_CAP_DR1
      && pCodec->id != AV_CODEC_ID_VP8
     )
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;
#endif

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  // advanced setting override for skip loop filter (see avcodec.h for valid options)
  //! @todo allow per video setting?
  if (g_advancedSettings.m_iSkipLoopFilter != 0)
  {
    m_pCodecContext->skip_loop_filter = (AVDiscard)g_advancedSettings.m_iSkipLoopFilter;
  }

  // set any special options
  for(std::vector<CDVDCodecOption>::iterator it = options.m_keys.begin(); it != options.m_keys.end(); ++it)
  {
    if (it->m_name == "surfaces")
      m_uSurfacesCount = atoi(it->m_value.c_str());
    else
      av_opt_set(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str(), 0);
  }

  // If non-zero, the decoded audio and video frames returned from avcodec_decode_video2() are reference-counted and are valid indefinitely.
  // Without this frames will get (deep) copied when deinterlace is set to automatic, but file is not deinterlaced.
  m_pCodecContext->refcounted_frames = 1;

  if (avcodec_open2(m_pCodecContext, pCodec, nullptr) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

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

  m_processInfo.SetVideoDimensions(m_pCodecContext->coded_width, m_pCodecContext->coded_height);
  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  av_frame_free(&m_pFrame);
  av_frame_free(&m_pDecodedFrame);
  av_frame_free(&m_pFilterFrame);
  avcodec_free_context(&m_pCodecContext);
  SAFE_RELEASE(m_pHardware);

  FilterClose();
}

void CDVDVideoCodecFFmpeg::SetDropState(bool bDrop)
{
  if( m_pCodecContext )
  {
    if (bDrop && m_pHardware && m_pHardware->CanSkipDeint())
    {
      m_requestSkipDeint = true;
      bDrop = false;
    }
    else
      m_requestSkipDeint = false;

    // i don't know exactly how high this should be set
    // couldn't find any good docs on it. think it varies
    // from codec to codec on what it does

    //  2 seem to be to high.. it causes video to be ruined on following images
    if( bDrop )
    {
      m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      m_pCodecContext->skip_idct = AVDISCARD_NONREF;
      m_pCodecContext->skip_loop_filter = AVDISCARD_NONREF;
    }
    else
    {
      m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_loop_filter = AVDISCARD_DEFAULT;
    }
  }
}

void CDVDVideoCodecFFmpeg::SetFilters()
{
  // ask codec to do deinterlacing if possible
  EINTERLACEMETHOD mInt = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod;

  if (mInt != VS_INTERLACEMETHOD_DEINTERLACE && mInt != VS_INTERLACEMETHOD_DEINTERLACE_HALF)
    mInt = m_processInfo.GetFallbackDeintMethod();

  unsigned int filters = 0;

  if (mInt != VS_INTERLACEMETHOD_NONE)
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

  if (filters & FILTER_DEINTERLACE_YADIF)
  {
    if (filters & FILTER_DEINTERLACE_HALFED)
      m_filters_next = "yadif=0:-1";
    else
      m_filters_next = "yadif=1:-1";

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

  CLog::Log(LOGDEBUG, "CDVDVideoCodecFFmpeg - Updated codec: %s", m_name.c_str());
}

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}

int CDVDVideoCodecFFmpeg::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  int iGotPicture = 0, len = 0;

  if (!m_pCodecContext)
    return VC_ERROR;

  if (pData)
    m_iLastKeyframe++;

  if (m_pHardware)
  {
    int result;
    if (pData || (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN))
    {
      result = m_pHardware->Check(m_pCodecContext);
      result &= ~VC_NOBUFFER;
    }
    else
    {
      result = m_pHardware->Decode(m_pCodecContext, nullptr);
    }

    if (result)
      return result;
  }

  if (!m_pHardware && pData)
    SetFilters();

  if (m_pFilterGraph && !m_filterEof)
  {
    int result = 0;
    if (pData == NULL)
      result = FilterProcess(nullptr);
    if (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)
    {
      result &= VC_PICTURE;
    }
    if (result)
      return result;
  }

  m_dts = dts;
  m_pCodecContext->reordered_opaque = pts_dtoi(pts);

  AVPacket avpkt;
  av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  avpkt.dts = (dts == DVD_NOPTS_VALUE) ? AV_NOPTS_VALUE : dts / DVD_TIME_BASE * AV_TIME_BASE;
  avpkt.pts = (pts == DVD_NOPTS_VALUE) ? AV_NOPTS_VALUE : pts / DVD_TIME_BASE * AV_TIME_BASE;

  /* We lie, but this flag is only used by pngdec.c.
   * Setting it correctly would allow CorePNG decoding. */
  avpkt.flags = AV_PKT_FLAG_KEY;
  len = avcodec_decode_video2(m_pCodecContext, m_pDecodedFrame, &iGotPicture, &avpkt);

  if (m_decoderState == STATE_HW_FAILED && !m_pHardware)
    return VC_REOPEN;

  if(m_iLastKeyframe < m_pCodecContext->has_b_frames + 2)
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;

  if (len < 0)
  {
    if(m_pHardware)
    {
      int result = m_pHardware->Check(m_pCodecContext);
      if (result & VC_NOBUFFER)
      {
        result = m_pHardware->Decode(m_pCodecContext, NULL);
        return result;
      }
    }
    CLog::Log(LOGERROR, "%s - avcodec_decode_video returned failure", __FUNCTION__);
    return VC_ERROR;
  }

  if (!iGotPicture)
  {
    if (pData && m_pCodecContext->skip_frame > AVDISCARD_DEFAULT)
    {
      m_droppedFrames++;
      if (m_interlaced)
        m_droppedFrames++;
    }

    if (m_pHardware && (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN))
    {
      int result;
      result = m_pHardware->Decode(m_pCodecContext, NULL);
      return result;
    }
    else
      return VC_BUFFER;
  }

  if (m_pDecodedFrame->key_frame)
  {
    m_started = true;
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;
  }

  if (m_pDecodedFrame->interlaced_frame)
    m_interlaced = true;
  else
    m_interlaced = false;

  /* put a limit on convergence count to avoid huge mem usage on streams without keyframes */
  if (m_iLastKeyframe > 300)
    m_iLastKeyframe = 300;

  /* h264 doesn't always have keyframes + won't output before first keyframe anyway */
  if(m_pCodecContext->codec_id == AV_CODEC_ID_H264 ||
     m_pCodecContext->codec_id == AV_CODEC_ID_SVQ3)
    m_started = true;

  if (m_pHardware == nullptr)
  {
    bool need_scale = std::find( m_formats.begin()
                               , m_formats.end()
                               , m_pCodecContext->pix_fmt) == m_formats.end();

    bool need_reopen  = false;
    if (m_filters != m_filters_next)
      need_reopen = true;

    if (!m_filters_next.empty() && m_filterEof)
      need_reopen = true;

    if (m_pFilterIn)
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
  }

  int result;
  if (m_pHardware)
  {
    av_frame_unref(m_pFrame);
    av_frame_move_ref(m_pFrame, m_pDecodedFrame);
    result = m_pHardware->Decode(m_pCodecContext, m_pFrame);
  }
  else if (m_pFilterGraph && !m_filterEof)
  {
    result = FilterProcess(m_pDecodedFrame);
  }
  else
  {
    av_frame_unref(m_pFrame);
    av_frame_move_ref(m_pFrame, m_pDecodedFrame);
    result = VC_PICTURE | VC_BUFFER;
  }

  if (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)
    result &= ~VC_BUFFER;

  if (result & VC_FLUSHED)
    Reset();

  return result;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  m_started = false;
  m_interlaced = false;
  m_decoderPts = DVD_NOPTS_VALUE;
  m_skippedDeint = 0;
  m_droppedFrames = 0;
  m_iLastKeyframe = m_pCodecContext->has_b_frames;
  avcodec_flush_buffers(m_pCodecContext);

  if (m_pHardware)
    m_pHardware->Reset();

  m_filters = "";
  FilterClose();
}

void CDVDVideoCodecFFmpeg::Reopen()
{
  Dispose();
  if (!Open(m_hints, m_options))
  {
    Dispose();
  }
}

bool CDVDVideoCodecFFmpeg::GetPictureCommon(DVDVideoPicture* pDvdVideoPicture)
{
  if (!m_pFrame)
    return false;

  pDvdVideoPicture->iWidth = m_pFrame->width;
  pDvdVideoPicture->iHeight = m_pFrame->height;

  /* crop of 10 pixels if demuxer asked it */
  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < (int)pDvdVideoPicture->iWidth
                                   && m_pCodecContext->coded_width  > (int)pDvdVideoPicture->iWidth  - 10)
    pDvdVideoPicture->iWidth = m_pCodecContext->coded_width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < (int)pDvdVideoPicture->iHeight
                                   && m_pCodecContext->coded_height > (int)pDvdVideoPicture->iHeight - 10)
    pDvdVideoPicture->iHeight = m_pCodecContext->coded_height;

  double aspect_ratio;

  /* use variable in the frame */
  AVRational pixel_aspect = m_pFrame->sample_aspect_ratio;

  if (pixel_aspect.num == 0)
    aspect_ratio = 0;
  else
    aspect_ratio = av_q2d(pixel_aspect) * pDvdVideoPicture->iWidth / pDvdVideoPicture->iHeight;

  if (aspect_ratio <= 0.0)
    aspect_ratio = (float)pDvdVideoPicture->iWidth / (float)pDvdVideoPicture->iHeight;

  if (m_DAR != aspect_ratio)
  {
    m_DAR = aspect_ratio;
    m_processInfo.SetVideoDAR(m_DAR);
  }

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
  pDvdVideoPicture->iDisplayWidth  = ((int)RINT(pDvdVideoPicture->iHeight * aspect_ratio)) & -3;
  if (pDvdVideoPicture->iDisplayWidth > pDvdVideoPicture->iWidth)
  {
    pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
    pDvdVideoPicture->iDisplayHeight = ((int)RINT(pDvdVideoPicture->iWidth / aspect_ratio)) & -3;
  }


  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  AVDictionaryEntry * entry = av_dict_get(av_frame_get_metadata(m_pFrame), "stereo_mode", NULL, 0);
  if(entry && entry->value)
  {
    strncpy(pDvdVideoPicture->stereo_mode, (const char*)entry->value, sizeof(pDvdVideoPicture->stereo_mode));
    pDvdVideoPicture->stereo_mode[sizeof(pDvdVideoPicture->stereo_mode)-1] = '\0';
  }

  pDvdVideoPicture->iRepeatPicture = 0.5 * m_pFrame->repeat_pict;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;

  if (m_codecControlFlags & DVD_CODEC_CTRL_DROP)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  pDvdVideoPicture->chroma_position = m_pCodecContext->chroma_sample_location;
  pDvdVideoPicture->color_primaries = m_pCodecContext->color_primaries;
  pDvdVideoPicture->color_transfer = m_pCodecContext->color_trc;
  pDvdVideoPicture->color_matrix = m_pCodecContext->colorspace;
  if(m_pCodecContext->color_range == AVCOL_RANGE_JPEG
  || m_pCodecContext->pix_fmt     == AV_PIX_FMT_YUVJ420P)
    pDvdVideoPicture->color_range = 1;
  else
    pDvdVideoPicture->color_range = 0;

  int qscale_type;
  pDvdVideoPicture->qp_table = av_frame_get_qp_table(m_pFrame, &pDvdVideoPicture->qstride, &qscale_type);

  switch (qscale_type)
  {
  case FF_QSCALE_TYPE_MPEG1:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_MPEG1;
    break;
  case FF_QSCALE_TYPE_MPEG2:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_MPEG2;
    break;
  case FF_QSCALE_TYPE_H264:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_H264;
    break;
  default:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_UNKNOWN;
  }

  if (pDvdVideoPicture->iRepeatPicture)
    pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  else
    pDvdVideoPicture->dts = m_dts;

  m_dts = DVD_NOPTS_VALUE;

  int64_t bpts = av_frame_get_best_effort_timestamp(m_pFrame);
  if (bpts != AV_NOPTS_VALUE)
  {
    pDvdVideoPicture->pts = (double)bpts * DVD_TIME_BASE / AV_TIME_BASE;
    if (pDvdVideoPicture->pts == m_decoderPts)
    {
      pDvdVideoPicture->iRepeatPicture = -0.5;
      pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
      pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
    }
  }
  else
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if (pDvdVideoPicture->pts != DVD_NOPTS_VALUE)
    m_decoderPts = pDvdVideoPicture->pts;

  if (m_requestSkipDeint)
  {
    pDvdVideoPicture->iFlags |= DVD_CODEC_CTRL_SKIPDEINT;
    m_skippedDeint++;
  }

  m_requestSkipDeint = false;
  pDvdVideoPicture->iFlags |= m_codecControlFlags;

  if (!m_started)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  return true;
}

bool CDVDVideoCodecFFmpeg::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (m_pHardware)
    return m_pHardware->GetPicture(m_pCodecContext, m_pFrame, pDvdVideoPicture);

  if (!GetPictureCommon(pDvdVideoPicture))
    return false;

  for (int i = 0; i < 4; i++)
    pDvdVideoPicture->data[i] = m_pFrame->data[i];
  for (int i = 0; i < 4; i++)
    pDvdVideoPicture->iLineSize[i] = m_pFrame->linesize[i];

  pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED;
  pDvdVideoPicture->extended_format = 0;

  AVPixelFormat pix_fmt;
  pix_fmt = (AVPixelFormat)m_pFrame->format;

  pDvdVideoPicture->format = CDVDCodecUtils::EFormatFromPixfmt(pix_fmt);
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

  AVFilter* srcFilter = avfilter_get_by_name("buffer");
  AVFilter* outFilter = avfilter_get_by_name("buffersink"); // should be last filter in the graph for now

  std::string args = StringUtils::Format("%d:%d:%d:%d:%d:%d:%d",
                                        m_pCodecContext->width,
                                        m_pCodecContext->height,
                                        m_pCodecContext->pix_fmt,
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

    if ((result = avfilter_graph_parse_ptr(m_pFilterGraph, (const char*)m_filters.c_str(), &inputs, &outputs, NULL)) < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_parse");
      return result;
    }

    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);

    if (filters.compare(0,5,"yadif") == 0)
    {
      m_processInfo.SetVideoDeintMethod(filters);
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

  m_filterEof = false;
  return result;
}

void CDVDVideoCodecFFmpeg::FilterClose()
{
  if (m_pFilterGraph)
  {
    avfilter_graph_free(&m_pFilterGraph);

    // Disposed by above code
    m_pFilterIn = nullptr;
    m_pFilterOut = nullptr;
  }
}

int CDVDVideoCodecFFmpeg::FilterProcess(AVFrame* frame)
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
  if (m_pHardware)
    m_pHardware->SetCodecControl(flags);
}

void CDVDVideoCodecFFmpeg::SetHardware(IHardwareDecoder* hardware)
{
  SAFE_RELEASE(m_pHardware);
  m_pHardware = hardware;
  UpdateName();
}
