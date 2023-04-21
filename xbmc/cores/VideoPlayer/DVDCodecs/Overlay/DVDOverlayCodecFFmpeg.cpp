/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecFFmpeg.h"

#include "DVDOverlayImage.h"
#include "DVDStreamInfo.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

CDVDOverlayCodecFFmpeg::CDVDOverlayCodecFFmpeg() : CDVDOverlayCodec("FFmpeg Subtitle Decoder")
{
  m_pCodecContext = NULL;
  m_SubtitleIndex = -1;
  m_width         = 0;
  m_height        = 0;
  m_StartTime     = 0.0;
  m_StopTime      = 0.0;
  memset(&m_Subtitle, 0, sizeof(m_Subtitle));
}

CDVDOverlayCodecFFmpeg::~CDVDOverlayCodecFFmpeg()
{
  avsubtitle_free(&m_Subtitle);
  avcodec_free_context(&m_pCodecContext);
}

bool CDVDOverlayCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{

  // decoding of this kind of subs does not work reliable
  if (hints.codec == AV_CODEC_ID_EIA_608)
    return false;

  const AVCodec* pCodec = avcodec_find_decoder(hints.codec);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG, "{} - Unable to find codec {}", __FUNCTION__, hints.codec);
    return false;
  }

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
    return false;

  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->codec_tag = hints.codec_tag;
  m_pCodecContext->time_base.num = 1;
  m_pCodecContext->time_base.den = DVD_TIME_BASE;
  m_pCodecContext->pkt_timebase.num = 1;
  m_pCodecContext->pkt_timebase.den = DVD_TIME_BASE;

  if (hints.extradata)
  {
    m_pCodecContext->extradata_size = hints.extradata.GetSize();
    m_pCodecContext->extradata =
        (uint8_t*)av_mallocz(hints.extradata.GetSize() + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata.GetData(), hints.extradata.GetSize());

    // start parsing of extra data - create a copy to be safe and make it zero-terminating to avoid access violations!
    unsigned int parse_extrasize = hints.extradata.GetSize();
    char* parse_extra = new char[parse_extrasize + 1];
    memcpy(parse_extra, hints.extradata.GetData(), parse_extrasize);
    parse_extra[parse_extrasize] = '\0';

    // assume that the extra data is formatted as a concatenation of lines ('\n' terminated)
    char *ptr = parse_extra;
    do // read line by line
    {
      if (!strncmp(ptr, "size:", 5))
      {
        int width = 0, height = 0;
        sscanf(ptr, "size: %dx%d", &width, &height);
        if (width > 0 && height > 0)
        {
          m_pCodecContext->width = width;
          m_pCodecContext->height = height;
          CLog::Log(LOGDEBUG, "{} - parsed extradata: size: {} x {}", __FUNCTION__, width, height);
        }
      }
      /*
      // leaving commented code: these items don't work yet... but they may be meaningful
      if (!strncmp(ptr, "palette:", 8))
        if (sscanf(ptr, "palette: %x, %x, %x, %x, %x, %x, %x, %x,"
                                " %x, %x, %x, %x, %x, %x, %x, %x", ...
      if (!StringUtils::CompareNoCase(ptr, "forced subs: on", 15))
        forced_subs_only = 1;
      */
      // if tried all possibilities, then read newline char and move to next line
      ptr = strchr(ptr, '\n');
      if (ptr != NULL) ptr++;
    }
    while (ptr != NULL && ptr <= parse_extra + parse_extrasize);

    delete[] parse_extra;
  }

  if (avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

  return true;
}

OverlayMessage CDVDOverlayCodecFFmpeg::Decode(DemuxPacket* pPacket)
{
  if (!m_pCodecContext || !pPacket)
    return OverlayMessage::OC_ERROR;

  int gotsub = 0, len = 0;

  avsubtitle_free(&m_Subtitle);

  AVPacket* avpkt = av_packet_alloc();
  if (!avpkt)
  {
    CLog::Log(LOGERROR, "CDVDOverlayCodecFFmpeg::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return OverlayMessage::OC_ERROR;
  }

  avpkt->data = pPacket->pData;
  avpkt->size = pPacket->iSize;
  avpkt->pts = pPacket->pts == DVD_NOPTS_VALUE ? AV_NOPTS_VALUE : (int64_t)pPacket->pts;
  avpkt->dts = pPacket->dts == DVD_NOPTS_VALUE ? AV_NOPTS_VALUE : (int64_t)pPacket->dts;

  len = avcodec_decode_subtitle2(m_pCodecContext, &m_Subtitle, &gotsub, avpkt);

  int size = avpkt->size;

  av_packet_free(&avpkt);

  if (len < 0)
  {
    CLog::Log(LOGERROR, "{} - avcodec_decode_subtitle returned failure", __FUNCTION__);
    Flush();
    return OverlayMessage::OC_ERROR;
  }

  if (len != size)
    CLog::Log(LOGWARNING, "{} - avcodec_decode_subtitle didn't consume the full packet",
              __FUNCTION__);

  if (!gotsub)
    return OverlayMessage::OC_BUFFER;

  double pts_offset = 0.0;

  if (m_pCodecContext->codec_id == AV_CODEC_ID_HDMV_PGS_SUBTITLE && m_Subtitle.format == 0)
  {
    // for pgs subtitles the packet pts of the end_segments are wrong
    // instead use the subtitle pts to calc the offset here
    // see http://git.videolan.org/?p=ffmpeg.git;a=commit;h=2939e258f9d1fff89b3b68536beb931b54611585

    if (m_Subtitle.pts != AV_NOPTS_VALUE && pPacket->pts != DVD_NOPTS_VALUE)
    {
      pts_offset = m_Subtitle.pts - pPacket->pts ;
    }
  }

  m_StartTime   = DVD_MSEC_TO_TIME(m_Subtitle.start_display_time);
  m_StopTime    = DVD_MSEC_TO_TIME(m_Subtitle.end_display_time);

  //adapt start and stop time to our packet pts
  CDVDOverlayCodec::GetAbsoluteTimes(m_StartTime, m_StopTime, pPacket);

  m_StartTime += pts_offset;
  if (m_StopTime > 0)
    m_StopTime += pts_offset;

  m_SubtitleIndex = 0;

  return OverlayMessage::OC_OVERLAY;
}

void CDVDOverlayCodecFFmpeg::Reset()
{
  Flush();
}

void CDVDOverlayCodecFFmpeg::Flush()
{
  avsubtitle_free(&m_Subtitle);
  m_SubtitleIndex = -1;

  avcodec_flush_buffers(m_pCodecContext);
}

std::shared_ptr<CDVDOverlay> CDVDOverlayCodecFFmpeg::GetOverlay()
{
  if(m_SubtitleIndex<0)
    return nullptr;

  if(m_Subtitle.num_rects == 0 && m_SubtitleIndex == 0)
  {
    // we must add an empty overlay to replace the previous one
    auto o = std::make_shared<CDVDOverlay>(DVDOVERLAY_TYPE_NONE);
    o->iPTSStartTime = m_StartTime;
    o->iPTSStopTime  = 0;
    o->replace  = true;
    m_SubtitleIndex++;
    return o;
  }

  if(m_Subtitle.format == 0)
  {
    if(m_SubtitleIndex >= (int)m_Subtitle.num_rects)
      return nullptr;

    if(m_Subtitle.rects[m_SubtitleIndex] == NULL)
      return nullptr;

    AVSubtitleRect rect = *m_Subtitle.rects[m_SubtitleIndex];
    if (rect.data[0] == NULL)
      return nullptr;

    m_height = m_pCodecContext->height;
    m_width  = m_pCodecContext->width;

    if (m_pCodecContext->codec_id == AV_CODEC_ID_DVB_SUBTITLE)
    {
      // ETSI EN 300 743 V1.3.1
      // 5.3.1
      // Absence of a DDS in a stream implies that the stream is coded in accordance with EN 300 743 (V1.2.1) [5] and that a
      // display width of 720 pixels and a display height of 576 lines may be assumed.
      if (!m_height && !m_width)
      {
        m_width = 720;
        m_height = 576;
      }
    }

    RENDER_STEREO_MODE render_stereo_mode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
    if (render_stereo_mode != RENDER_STEREO_MODE_OFF &&
        render_stereo_mode != RENDER_STEREO_MODE_HARDWAREBASED)
    {
      if (rect.h > m_height / 2)
      {
        m_height /= 2;
        rect.h /= 2;
      }
      else if (rect.w > m_width / 2)
      {
        m_width /= 2;
        rect.w /= 2;
      }
    }

    auto overlay = std::make_shared<CDVDOverlayImage>();

    overlay->iPTSStartTime = m_StartTime;
    overlay->iPTSStopTime = m_StopTime;
    overlay->replace = true;
    overlay->linesize = rect.w;
    overlay->pixels.resize(rect.w * rect.h);
    overlay->palette.resize(rect.nb_colors);
    overlay->x = rect.x;
    overlay->y = rect.y;
    overlay->width = rect.w;
    overlay->height = rect.h;
    overlay->bForced = rect.flags != 0;
    overlay->source_width = m_width;
    overlay->source_height = m_height;

    uint8_t* s = rect.data[0];
    uint8_t* t = overlay->pixels.data();

    for (int i = 0; i < rect.h; i++)
    {
      memcpy(t, s, rect.w);
      s += rect.linesize[0];
      t += overlay->linesize;
    }

    for (int i = 0; i < rect.nb_colors; i++)
      overlay->palette[i] = Endian_SwapLE32(((uint32_t *)rect.data[1])[i]);

    m_SubtitleIndex++;

    return overlay;
  }

  return nullptr;
}
