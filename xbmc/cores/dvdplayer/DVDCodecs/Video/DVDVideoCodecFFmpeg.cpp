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
    if(dec->Open(avctx, *cur))
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


CDVDVideoCodecFFmpeg::IHardwareDecoder*  CDVDVideoCodecFFmpeg::IHardwareDecoder::Acquire()
{
  InterlockedIncrement(&m_references);
  return this;
}

long CDVDVideoCodecFFmpeg::IHardwareDecoder::Release()
{
  long count = InterlockedDecrement(&m_references);
  ASSERT(count >= 0);
  if (count == 0) delete this;
  return count;
}

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg() : CDVDVideoCodec()
{
  m_pCodecContext = NULL;
  m_pConvertFrame = NULL;
  m_pFrame = NULL;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

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

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load()) return false;

  m_dllAvCodec.avcodec_register_all();

  m_bSoftware     = hints.software;
  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();

  pCodec = NULL;

#ifdef HAVE_LIBVDPAU
  if(g_guiSettings.GetBool("videoplayer.usevdpau") && !m_bSoftware)
  {
    while((pCodec = m_dllAvCodec.av_codec_next(pCodec)))
    {
      if(pCodec->id == hints.codec
      && pCodec->capabilities & CODEC_CAP_HWACCEL_VDPAU)
      {
        if ((pCodec->id == CODEC_ID_MPEG4 || pCodec->id == CODEC_ID_XVID) && !g_advancedSettings.m_videoAllowMpeg4VDPAU)
          continue;

        CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Creating VDPAU(%ix%i, %d)",hints.width, hints.height, hints.codec);
        CVDPAU* vdp = new CVDPAU();
        m_pCodecContext->codec_id = hints.codec;
        m_pCodecContext->width    = hints.width;
        m_pCodecContext->height   = hints.height;
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
    m_dllAvCodec.av_set_string(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str());
  }

#if defined(__APPLE__) && defined(__arm__)
  m_dllAvCodec.avcodec_thread_init(m_pCodecContext, 1);
#elif defined(_LINUX) || defined(_WIN32)
  int num_threads = std::min(8 /*MAX_THREADS*/, g_cpuInfo.getCPUCount());
  if( num_threads > 1 && !hints.software && m_pHardware == NULL // thumbnail extraction fails when run threaded
  && ( pCodec->id == CODEC_ID_H264
    || pCodec->id == CODEC_ID_MPEG4 ))
    m_dllAvCodec.avcodec_thread_init(m_pCodecContext, num_threads);
#endif

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

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
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
      // TODO: 'hurry_up' has been deprecated in favor of the skip_* variables
      // Use those instead.

      m_pCodecContext->hurry_up = 1;
      //m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      //m_pCodecContext->skip_idct = AVDISCARD_NONREF;
      //m_pCodecContext->skip_loop_filter = AVDISCARD_NONREF;
    }
    else
    {
      m_pCodecContext->hurry_up = 0;
      //m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      //m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
      //m_pCodecContext->skip_loop_filter = AVDISCARD_DEFAULT;
    }
  }
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

  if (len != iSize && !m_pCodecContext->hurry_up)
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

  int result;
  if(m_pHardware)
    result = m_pHardware->Decode(m_pCodecContext, m_pFrame);
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
    delete[] m_pConvertFrame->data[0];
    m_dllAvUtil.av_free(m_pConvertFrame);
    m_pConvertFrame = NULL;
  }
}

bool CDVDVideoCodecFFmpeg::GetPictureCommon(DVDVideoPicture* pDvdVideoPicture)
{
  GetVideoAspect(m_pCodecContext, pDvdVideoPicture->iDisplayWidth, pDvdVideoPicture->iDisplayHeight);

  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < m_pCodecContext->width
                                   && m_pCodecContext->coded_width  > m_pCodecContext->width  - 10)
    pDvdVideoPicture->iWidth = m_pCodecContext->coded_width;
  else
    pDvdVideoPicture->iWidth = m_pCodecContext->width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < m_pCodecContext->height
                                   && m_pCodecContext->coded_height > m_pCodecContext->height - 10)
    pDvdVideoPicture->iHeight = m_pCodecContext->coded_height;
  else
    pDvdVideoPicture->iHeight = m_pCodecContext->height;

  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if (!m_pFrame)
    return false;

  pDvdVideoPicture->iRepeatPicture = 0.5 * m_pFrame->repeat_pict;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
  if(m_pCodecContext->pix_fmt == PIX_FMT_YUVJ420P)
    pDvdVideoPicture->color_range = 1;

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

  return true;
}

/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecFFmpeg::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
  double aspect_ratio;

  /* XXX: use variable in the frame */
  if (pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
  else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;

  if (aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  iHeight = pCodecContext->height;
  iWidth = ((int)RINT(pCodecContext->height * aspect_ratio)) & -3;
  if (iWidth > (unsigned int)pCodecContext->width)
  {
    iWidth = pCodecContext->width;
    iHeight = ((int)RINT(pCodecContext->width / aspect_ratio)) & -3;
  }
}

unsigned CDVDVideoCodecFFmpeg::GetConvergeCount()
{
  if(m_pHardware)
    return m_iLastKeyframe;
  else
    return 0;
}
