/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "../../../../utils/Win32Exception.h"
#if defined(_LINUX) || defined(_WIN32)
#include "utils/CPUInfo.h"
#endif
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "boost/shared_ptr.hpp"
#include "threads/Atomics.h"

#ifndef _LINUX
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#else
#include <math.h>
#define RINT lrint
#endif

#include "cores/VideoRenderers/RenderManager.h"

#ifdef HAVE_LIBVDPAU
#include "VDPAU.h"
#endif
#ifdef HAS_DX
#include "DXVA.h"
#endif
#ifdef HAVE_LIBVA
#include "VAAPI.h"
#endif

using namespace boost;

enum PixelFormat CDVDVideoCodecFFmpeg::GetFormat( struct AVCodecContext * avctx
                                                , const PixelFormat * fmt )
{
  CDVDVideoCodecFFmpeg* ctx  = (CDVDVideoCodecFFmpeg*)avctx->opaque;

  if(!ctx->IsHardwareAllowed())
    return ctx->m_dllAvCodec.avcodec_default_get_format(avctx, fmt);

  const PixelFormat * cur = fmt;
  while(*cur != PIX_FMT_NONE)
  {
#ifdef HAVE_LIBVDPAU
    if(CVDPAU::IsVDPAUFormat(*cur) && g_guiSettings.GetBool("videoplayer.usevdpau"))
    {
      if(ctx->GetHardware())
        return *cur;
        
      CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::GetFormat - Creating VDPAU(%ix%i)", avctx->width, avctx->height);
      CVDPAU* vdp = new CVDPAU();
      if(vdp->Open(avctx, *cur))
      {
        ctx->SetHardware(vdp);
        return *cur;
      }
      else
        vdp->Release();
    }
#endif
#ifdef HAS_DX
  if(DXVA::CDecoder::Supports(*cur) && g_guiSettings.GetBool("videoplayer.usedxva2"))
  {
    DXVA::CDecoder* dec = new DXVA::CDecoder();
    if(dec->Open(avctx, *cur, ctx->m_uSurfacesCount))
    {
      ctx->SetHardware(dec);
      return *cur;
    }
    else
      dec->Release();
  }
#endif
#ifdef HAVE_LIBVA
    if(*cur == PIX_FMT_VAAPI_VLD && g_guiSettings.GetBool("videoplayer.usevaapi"))
    {
      VAAPI::CDecoder* dec = new VAAPI::CDecoder();
      if(dec->Open(avctx, *cur))
      {
        ctx->SetHardware(dec);
        return *cur;
      }
      else
        dec->Release();
    }
#endif
    cur++;
  }
  return ctx->m_dllAvCodec.avcodec_default_get_format(avctx, fmt);
}

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg() : CDVDVideoCodec()
{
  m_pCodecContext = NULL;
  m_pConvertFrame = NULL;
  m_pFrame = NULL;
  m_pFilterGraph  = NULL;
  m_pFilterIn     = NULL;
  m_pFilterOut    = NULL;
  m_pFilterLink   = NULL;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

  m_uSurfacesCount = 0;

  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
  m_bSoftware = false;
  m_pHardware = NULL;
  m_iLastKeyframe = 0;
  m_dts = DVD_NOPTS_VALUE;
  m_started = false;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AVCodec* pCodec;

  if(!m_dllAvUtil.Load()
  || !m_dllAvCodec.Load()
  || !m_dllSwScale.Load()
  || !m_dllAvFilter.Load()
  ) return false;

  m_dllAvCodec.avcodec_register_all();
  m_dllAvFilter.avfilter_register_all();

  m_bSoftware     = hints.software;
  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();

  pCodec = NULL;

  if (hints.codec == CODEC_ID_H264)
  {
    switch(hints.profile)
    {
      case FF_PROFILE_H264_HIGH_10:
      case FF_PROFILE_H264_HIGH_10_INTRA:
      case FF_PROFILE_H264_HIGH_422:
      case FF_PROFILE_H264_HIGH_422_INTRA:
      case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      case FF_PROFILE_H264_HIGH_444_INTRA:
      case FF_PROFILE_H264_CAVLC_444:
      m_bSoftware = true;
      break;
    }
  }

#ifdef HAVE_LIBVDPAU
  if(g_guiSettings.GetBool("videoplayer.usevdpau") && !m_bSoftware)
  {
    while((pCodec = m_dllAvCodec.av_codec_next(pCodec)))
    {
      if(pCodec->id == hints.codec
      && pCodec->capabilities & CODEC_CAP_HWACCEL_VDPAU)
      {
        if ((pCodec->id == CODEC_ID_MPEG4) && !g_advancedSettings.m_videoAllowMpeg4VDPAU)
          continue;

        CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Creating VDPAU(%ix%i, %d)",hints.width, hints.height, hints.codec);
        CVDPAU* vdp = new CVDPAU();
        m_pCodecContext->codec_id = hints.codec;
        m_pCodecContext->width    = hints.width;
        m_pCodecContext->height   = hints.height;
        m_pCodecContext->coded_width   = hints.width;
        m_pCodecContext->coded_height  = hints.height;
        if(vdp->Open(m_pCodecContext, pCodec->pix_fmts ? pCodec->pix_fmts[0] : PIX_FMT_NONE))
        {
          m_pHardware = vdp;
          m_pCodecContext->codec_id = CODEC_ID_NONE; // ffmpeg will complain if this has been set
          break;
        }
        m_pCodecContext->codec_id = CODEC_ID_NONE; // ffmpeg will complain if this has been set
        CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Failed to get VDPAU device");
        vdp->Release();
      }
    }
  }
#endif

  if(pCodec == NULL)
    pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);

  if(pCodec == NULL)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Using codec: %s",pCodec->long_name ? pCodec->long_name : pCodec->name);

  m_pCodecContext->opaque = (void*)this;
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->get_format = GetFormat;
  m_pCodecContext->codec_tag = hints.codec_tag;

#if defined(__APPLE__) && defined(__arm__)
  // ffmpeg with enabled neon will crash and burn if this is enabled
  m_pCodecContext->flags &= CODEC_FLAG_EMU_EDGE;
#else
  if (pCodec->id != CODEC_ID_H264 && pCodec->capabilities & CODEC_CAP_DR1
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,69,0)
      && pCodec->id != CODEC_ID_VP8
#endif
     )
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;
#endif

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  // set acceleration
  m_pCodecContext->dsp_mask = 0;//FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;

  // advanced setting override for skip loop filter (see avcodec.h for valid options)
  // TODO: allow per video setting?
  if (g_advancedSettings.m_iSkipLoopFilter != 0)
  {
    m_pCodecContext->skip_loop_filter = (AVDiscard)g_advancedSettings.m_iSkipLoopFilter;
  }

  // set any special options
  for(CDVDCodecOptions::iterator it = options.begin(); it != options.end(); it++)
  {
    if (it->m_name == "surfaces")
      m_uSurfacesCount = std::atoi(it->m_value.c_str());
    else
      m_dllAvUtil.av_set_string3(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str(), 0, NULL);
  }

  int num_threads = std::min(8 /*MAX_THREADS*/, g_cpuInfo.getCPUCount());
  if( num_threads > 1 && !hints.software && m_pHardware == NULL // thumbnail extraction fails when run threaded
  && ( pCodec->id == CODEC_ID_H264
    || pCodec->id == CODEC_ID_MPEG4 ))
    m_dllAvCodec.avcodec_thread_init(m_pCodecContext, num_threads);

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    return false;
  }

  m_pFrame = m_dllAvCodec.avcodec_alloc_frame();
  if (!m_pFrame) return false;

  if(pCodec->name)
    m_name = CStdString("ff-") + pCodec->name;
  else
    m_name = "ffmpeg";

  if(m_pHardware)
    m_name += "-" + m_pHardware->Name();

  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  if (m_pFrame) m_dllAvUtil.av_free(m_pFrame);
  m_pFrame = NULL;

  if (m_pConvertFrame)
  {
    m_dllAvCodec.avpicture_free(m_pConvertFrame);
    m_dllAvUtil.av_free(m_pConvertFrame);
  }
  m_pConvertFrame = NULL;

  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    if (m_pCodecContext->extradata)
    {
      m_dllAvUtil.av_free(m_pCodecContext->extradata);
      m_pCodecContext->extradata = NULL;
      m_pCodecContext->extradata_size = 0;
    }
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  SAFE_RELEASE(m_pHardware);

  FilterClose();

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllAvFilter.Unload();
}

void CDVDVideoCodecFFmpeg::SetDropState(bool bDrop)
{
  if( m_pCodecContext )
  {
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

unsigned int CDVDVideoCodecFFmpeg::SetFilters(unsigned int flags)
{
  m_filters_next.Empty();

  if(m_pHardware)
    return 0;

  if(flags & FILTER_DEINTERLACE_YADIF)
  {
    if(flags & FILTER_DEINTERLACE_HALFED)
      m_filters_next = "yadif=0:-1";
    else
      m_filters_next = "yadif=1:-1";

    if(flags & FILTER_DEINTERLACE_FLAGGED)
      m_filters_next += ":1";

    flags &= ~FILTER_DEINTERLACE_ANY | FILTER_DEINTERLACE_YADIF;
  }

  return flags;
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

static double pts_itod(int64_t pts)
{
  pts_union u;
  u.pts_i = pts;
  return u.pts_d;
}

int CDVDVideoCodecFFmpeg::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  int iGotPicture = 0, len = 0;

  if (!m_pCodecContext)
    return VC_ERROR;

  if(pData)
    m_iLastKeyframe++;

  shared_ptr<CSingleLock> lock;
  if(m_pHardware)
  {
    CCriticalSection* section = m_pHardware->Section();
    if(section)
      lock = shared_ptr<CSingleLock>(new CSingleLock(*section));

    int result;
    if(pData)
      result = m_pHardware->Check(m_pCodecContext);
    else
      result = m_pHardware->Decode(m_pCodecContext, NULL);

    if(result)
      return result;
  }

  if(m_pFilterGraph)
  {
    int result = 0;
    if(pData == NULL)
      result = FilterProcess(NULL);
    if(result)
      return result;
  }

  m_dts = dts;
  m_pCodecContext->reordered_opaque = pts_dtoi(pts);

  AVPacket avpkt;
  m_dllAvCodec.av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  /* We lie, but this flag is only used by pngdec.c.
   * Setting it correctly would allow CorePNG decoding. */
  avpkt.flags = AV_PKT_FLAG_KEY;
  len = m_dllAvCodec.avcodec_decode_video2(m_pCodecContext, m_pFrame, &iGotPicture, &avpkt);

  if(m_iLastKeyframe < m_pCodecContext->has_b_frames + 1)
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 1;

  if (len < 0)
  {
    CLog::Log(LOGERROR, "%s - avcodec_decode_video returned failure", __FUNCTION__);
    return VC_ERROR;
  }

  if (len != iSize && m_pCodecContext->skip_frame != AVDISCARD_NONREF)
    CLog::Log(LOGWARNING, "%s - avcodec_decode_video didn't consume the full packet. size: %d, consumed: %d", __FUNCTION__, iSize, len);

  if (!iGotPicture)
    return VC_BUFFER;

  if(m_pFrame->key_frame)
  {
    m_started = true;
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 1;
  }

  if(m_pCodecContext->pix_fmt != PIX_FMT_YUV420P
  && m_pCodecContext->pix_fmt != PIX_FMT_YUVJ420P
  && m_pHardware == NULL)
  {
    if (!m_dllSwScale.IsLoaded() && !m_dllSwScale.Load())
        return VC_ERROR;

    if (!m_pConvertFrame)
    {
      // Allocate an AVFrame structure
      m_pConvertFrame = (AVPicture*)m_dllAvUtil.av_mallocz(sizeof(AVPicture));
      // Due to a bug in swsscale we need to allocate one extra line of data
      if(m_dllAvCodec.avpicture_alloc( m_pConvertFrame
                                     , PIX_FMT_YUV420P
                                     , m_pCodecContext->width
                                     , m_pCodecContext->height+1) < 0)
      {
        m_dllAvUtil.av_free(m_pConvertFrame);
        m_pConvertFrame = NULL;
        return VC_ERROR;
      }
    }

    // convert the picture
    struct SwsContext *context = m_dllSwScale.sws_getContext(m_pCodecContext->width, m_pCodecContext->height,
                                         m_pCodecContext->pix_fmt, m_pCodecContext->width, m_pCodecContext->height,
                                         PIX_FMT_YUV420P, SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);

    if(context == NULL)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::Decode - unable to obtain sws context for w:%i, h:%i, pixfmt: %i", m_pCodecContext->width, m_pCodecContext->height, m_pCodecContext->pix_fmt);
      return VC_ERROR;
    }

    m_dllSwScale.sws_scale(context
                          , m_pFrame->data
                          , m_pFrame->linesize
                          , 0
                          , m_pCodecContext->height
                          , m_pConvertFrame->data
                          , m_pConvertFrame->linesize);

    m_dllSwScale.sws_freeContext(context);
  }
  else
  {
    // no need to convert, just free any existing convert buffers
    if (m_pConvertFrame)
    {
      m_dllAvCodec.avpicture_free(m_pConvertFrame);
      m_dllAvUtil.av_free(m_pConvertFrame);
      m_pConvertFrame = NULL;
    }
  }

  // try to setup new filters
  if (!m_filters.Equals(m_filters_next))
  {
    m_filters = m_filters_next;
    if(FilterOpen(m_filters) < 0)
      FilterClose();
  }

  int result;
  if(m_pHardware)
    result = m_pHardware->Decode(m_pCodecContext, m_pFrame);
  else if(m_pFilterGraph)
    result = FilterProcess(m_pFrame);
  else
    result = VC_PICTURE | VC_BUFFER;

  if(result & VC_FLUSHED)
    Reset();

  return result;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  m_started = false;
  m_iLastKeyframe = m_pCodecContext->has_b_frames;
  m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

  if (m_pHardware)
    m_pHardware->Reset();

  if (m_pConvertFrame)
  {
    m_dllAvCodec.avpicture_free(m_pConvertFrame);
    m_dllAvUtil.av_free(m_pConvertFrame);
    m_pConvertFrame = NULL;
  }
  m_filters = "";
  FilterClose();
}

bool CDVDVideoCodecFFmpeg::GetPictureCommon(DVDVideoPicture* pDvdVideoPicture)
{
  pDvdVideoPicture->iWidth = m_pCodecContext->width;
  pDvdVideoPicture->iHeight = m_pCodecContext->height;

  if(m_pFilterLink)
  {
    pDvdVideoPicture->iWidth  = m_pFilterLink->cur_buf->video->w;
    pDvdVideoPicture->iHeight = m_pFilterLink->cur_buf->video->h;
  }

  /* crop of 10 pixels if demuxer asked it */
  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < (int)pDvdVideoPicture->iWidth
                                   && m_pCodecContext->coded_width  > (int)pDvdVideoPicture->iWidth  - 10)
    pDvdVideoPicture->iWidth = m_pCodecContext->coded_width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < (int)pDvdVideoPicture->iHeight
                                   && m_pCodecContext->coded_height > (int)pDvdVideoPicture->iHeight - 10)
    pDvdVideoPicture->iHeight = m_pCodecContext->coded_height;

  double aspect_ratio;

  /* use variable in the frame */
  AVRational pixel_aspect = m_pCodecContext->sample_aspect_ratio;
  if (m_pFilterLink)
#ifdef HAVE_AVFILTERBUFFERREFVIDEOPROPS_SAMPLE_ASPECT_RATIO
    pixel_aspect = m_pFilterLink->cur_buf->video->sample_aspect_ratio;
#else
    pixel_aspect = m_pFilterLink->cur_buf->video->pixel_aspect;
#endif

  if (pixel_aspect.num == 0)
    aspect_ratio = 0;
  else
    aspect_ratio = av_q2d(pixel_aspect) * pDvdVideoPicture->iWidth / pDvdVideoPicture->iHeight;

  if (aspect_ratio <= 0.0)
    aspect_ratio = (float)pDvdVideoPicture->iWidth / (float)pDvdVideoPicture->iHeight;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
  pDvdVideoPicture->iDisplayWidth  = ((int)RINT(pDvdVideoPicture->iHeight * aspect_ratio)) & -3;
  if (pDvdVideoPicture->iDisplayWidth > pDvdVideoPicture->iWidth)
  {
    pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
    pDvdVideoPicture->iDisplayHeight = ((int)RINT(pDvdVideoPicture->iWidth / aspect_ratio)) & -3;
  }


  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if (!m_pFrame)
    return false;

  pDvdVideoPicture->iRepeatPicture = 0.5 * m_pFrame->repeat_pict;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
  if(m_pCodecContext->pix_fmt == PIX_FMT_YUVJ420P)
    pDvdVideoPicture->color_range = 1;

  pDvdVideoPicture->chroma_position = m_pCodecContext->chroma_sample_location;
  pDvdVideoPicture->color_primaries = m_pCodecContext->color_primaries;
  pDvdVideoPicture->color_transfer = m_pCodecContext->color_trc;

  pDvdVideoPicture->qscale_table = m_pFrame->qscale_table;
  pDvdVideoPicture->qscale_stride = m_pFrame->qstride;

  switch (m_pFrame->qscale_type) {
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

  pDvdVideoPicture->dts = m_dts;
  m_dts = DVD_NOPTS_VALUE;
  if (m_pFrame->reordered_opaque)
    pDvdVideoPicture->pts = pts_itod(m_pFrame->reordered_opaque);
  else
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if(!m_started)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  return true;
}

bool CDVDVideoCodecFFmpeg::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if(m_pHardware)
    return m_pHardware->GetPicture(m_pCodecContext, m_pFrame, pDvdVideoPicture);

  if(!GetPictureCommon(pDvdVideoPicture))
    return false;

  if(m_pConvertFrame)
  {
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->data[i]      = m_pConvertFrame->data[i];
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->iLineSize[i] = m_pConvertFrame->linesize[i];
  }
  else
  {
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->data[i]      = m_pFrame->data[i];
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->iLineSize[i] = m_pFrame->linesize[i];
  }

  pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;
  pDvdVideoPicture->extended_format = 0;

  return true;
}

int CDVDVideoCodecFFmpeg::FilterOpen(const CStdString& filters)
{
  int result;

  if (m_pFilterGraph)
    FilterClose();

  if (filters.IsEmpty())
    return 0;

  if (!(m_pFilterGraph = m_dllAvFilter.avfilter_graph_alloc()))
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - unable to alloc filter graph");
    return -1;
  }

  // CrHasher HACK (if an alternative becomes available use it!): In order to display the output
  // produced by a combination of filters we insert "nullsink" as the last filter and we use
  // its input pin as our output pin.
  //
  // input --> .. --> last_filter --> [in] nullsink [null]     [in] --> output
  //                                   |                        |
  //                                   |                        |
  //                                   +------------------------+
  //
  AVFilter* srcFilter = m_dllAvFilter.avfilter_get_by_name("buffer");
  AVFilter* outFilter = m_dllAvFilter.avfilter_get_by_name("nullsink"); // should be last filter in the graph for now

  CStdString args;

  args.Format("%d:%d:%d:%d:%d:%d:%d",
    m_pCodecContext->width,
    m_pCodecContext->height,
    m_pCodecContext->pix_fmt,
    m_pCodecContext->time_base.num,
    m_pCodecContext->time_base.den,
    m_pCodecContext->sample_aspect_ratio.num,
    m_pCodecContext->sample_aspect_ratio.den);

  if ((result = m_dllAvFilter.avfilter_graph_create_filter(&m_pFilterIn, srcFilter, "src", args, NULL, m_pFilterGraph)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_create_filter: src");
    return result;
  }

  if ((result = m_dllAvFilter.avfilter_graph_create_filter(&m_pFilterOut, outFilter, "out", NULL, NULL/*nullsink=>NULL*/, m_pFilterGraph)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_create_filter: out");
    return result;
  }

  if (!filters.empty())
  {
    AVFilterInOut* outputs = m_dllAvFilter.avfilter_inout_alloc();
    AVFilterInOut* inputs  = m_dllAvFilter.avfilter_inout_alloc();

    outputs->name    = m_dllAvUtil.av_strdup("in");
    outputs->filter_ctx = m_pFilterIn;
    outputs->pad_idx = 0;
    outputs->next    = NULL;

    inputs->name    = m_dllAvUtil.av_strdup("out");
    inputs->filter_ctx = m_pFilterOut;
    inputs->pad_idx = 0;
    inputs->next    = NULL;

    if ((result = m_dllAvFilter.avfilter_graph_parse(m_pFilterGraph, (const char*)m_filters.c_str(), &inputs, &outputs, NULL)) < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_parse");
      return result;
    }

    m_dllAvFilter.avfilter_inout_free(&outputs);
    m_dllAvFilter.avfilter_inout_free(&inputs);
  }
  else
  {
    if ((result = m_dllAvFilter.avfilter_link(m_pFilterIn, 0, m_pFilterOut, 0)) < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_link");
      return result;
    }
  }

  if ((result = m_dllAvFilter.avfilter_graph_config(m_pFilterGraph, NULL)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_config");
    return result;
  }

  return result;
}

void CDVDVideoCodecFFmpeg::FilterClose()
{
  if (m_pFilterGraph)
  {
    m_dllAvFilter.avfilter_graph_free(&m_pFilterGraph);

    // Disposed by above code
    m_pFilterIn   = NULL;
    m_pFilterOut  = NULL;
    m_pFilterLink = NULL;
  }
}

int CDVDVideoCodecFFmpeg::FilterProcess(AVFrame* frame)
{
  int result, frames;

  m_pFilterLink = m_pFilterOut->inputs[0];

  if (frame)
  {
#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(2,13,0)
    result = m_dllAvFilter.av_vsrc_buffer_add_frame(m_pFilterIn, frame, 0);
#elif LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(2,7,0)
    result = m_dllAvFilter.av_vsrc_buffer_add_frame(m_pFilterIn, frame);
#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,3,0)
    result = m_dllAvFilter.av_vsrc_buffer_add_frame(m_pFilterIn, frame, frame->pts);
#else
    result = m_dllAvFilter.av_vsrc_buffer_add_frame(m_pFilterIn, frame, frame->pts, m_pCodecContext->sample_aspect_ratio);
#endif

    if (result < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - av_vsrc_buffer_add_frame");
      return VC_ERROR;
    }
  }

  if ((frames = m_dllAvFilter.avfilter_poll_frame(m_pFilterLink)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - avfilter_poll_frame");
    return VC_ERROR;
  }

  if (frames > 0)
  {
    if (m_pFilterLink->cur_buf)
    {
      m_dllAvFilter.avfilter_unref_buffer(m_pFilterLink->cur_buf);
      m_pFilterLink->cur_buf = NULL;
    }

    if ((result = m_dllAvFilter.avfilter_request_frame(m_pFilterLink)) < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - avfilter_request_frame");
      return VC_ERROR;
    }

    if (!m_pFilterLink->cur_buf)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - cur_buf");
      return VC_ERROR;
    }

    if(frame == NULL)
      m_pFrame->reordered_opaque = 0;
    else
      m_pFrame->repeat_pict      = -(frames - 1);

    m_pFrame->interlaced_frame = m_pFilterLink->cur_buf->video->interlaced;
    m_pFrame->top_field_first  = m_pFilterLink->cur_buf->video->top_field_first;

    memcpy(m_pFrame->linesize, m_pFilterLink->cur_buf->linesize, 4*sizeof(int));
    memcpy(m_pFrame->data    , m_pFilterLink->cur_buf->data    , 4*sizeof(uint8_t*));

    if(frames > 1)
      return VC_PICTURE;
    else
      return VC_PICTURE | VC_BUFFER;
  }

  return VC_BUFFER;
}

unsigned CDVDVideoCodecFFmpeg::GetConvergeCount()
{
  if(m_pHardware)
    return m_iLastKeyframe;
  else
    return 0;
}
