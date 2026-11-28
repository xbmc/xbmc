/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecFFmpeg.h"

#include "DVDOverlayImage.h"
#include "DVDStreamInfo.h"
#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace
{
enum class BitmapStereoLayout
{
  MONO,
  LEFT_RIGHT,
  TOP_BOTTOM,
};

struct Bounds
{
  int x1{0};
  int y1{0};
  int x2{0};
  int y2{0};

  bool IsEmpty() const { return x1 >= x2 || y1 >= y2; }
  int Width() const { return x2 - x1; }
  int Height() const { return y2 - y1; }
};

BitmapStereoLayout GetBitmapStereoLayout(std::string_view stereoMode)
{
  if (stereoMode == "left_right" || stereoMode == "right_left")
    return BitmapStereoLayout::LEFT_RIGHT;
  if (stereoMode == "top_bottom" || stereoMode == "bottom_top")
    return BitmapStereoLayout::TOP_BOTTOM;
  return BitmapStereoLayout::MONO;
}

uint32_t GetPixel(const CDVDOverlayImage& bitmap, int x, int y)
{
  const auto index = bitmap.pixels[(y - bitmap.y) * bitmap.linesize + x - bitmap.x];
  const uint32_t color = bitmap.palette[index];
  return ((color >> PIXEL_ASHIFT) & 0xff) != 0 ? color : 0;
}

Bounds GetVisibleBounds(const CDVDOverlayImage& bitmap, const Bounds& region)
{
  const int x1 = std::max(bitmap.x, region.x1);
  const int y1 = std::max(bitmap.y, region.y1);
  const int x2 = std::min(bitmap.x + bitmap.width, region.x2);
  const int y2 = std::min(bitmap.y + bitmap.height, region.y2);
  if (x1 >= x2 || y1 >= y2)
    return {};

  Bounds visible{x2, y2, x1, y1};
  for (int y = y1; y < y2; ++y)
  {
    for (int x = x1; x < x2; ++x)
    {
      if (GetPixel(bitmap, x, y) == 0)
        continue;

      visible.x1 = std::min(visible.x1, x);
      visible.y1 = std::min(visible.y1, y);
      visible.x2 = std::max(visible.x2, x + 1);
      visible.y2 = std::max(visible.y2, y + 1);
    }
  }
  return visible;
}

bool HaveCompatiblePositions(const Bounds& first,
                             const Bounds& second,
                             const Bounds& firstRegion,
                             const Bounds& secondRegion)
{
  const int firstX = first.x1 - firstRegion.x1;
  const int firstY = first.y1 - firstRegion.y1;
  const int secondX = second.x1 - secondRegion.x1;
  const int secondY = second.y1 - secondRegion.y1;
  const int maxHorizontalShift = std::max(2, firstRegion.Width() / 8);
  const int maxVerticalShift = std::max(2, firstRegion.Height() / 100);

  return std::abs(firstX - secondX) <= maxHorizontalShift &&
         std::abs(firstY - secondY) <= maxVerticalShift;
}

bool IsStereoBitmap(const CDVDOverlayImage& bitmap, BitmapStereoLayout layout)
{
  if (bitmap.linesize < bitmap.width || bitmap.width <= 0 || bitmap.height <= 0 ||
      bitmap.source_width <= 0 || bitmap.source_height <= 0 || bitmap.palette.empty())
    return false;

  Bounds first;
  Bounds second;
  if (layout == BitmapStereoLayout::LEFT_RIGHT)
  {
    if (bitmap.source_width % 2 != 0)
      return false;

    const int split = bitmap.source_width / 2;
    if (bitmap.x >= split || bitmap.x + bitmap.width <= split)
      return false;

    first = {0, 0, split, bitmap.source_height};
    second = {split, 0, bitmap.source_width, bitmap.source_height};
  }
  else if (layout == BitmapStereoLayout::TOP_BOTTOM)
  {
    if (bitmap.source_height % 2 != 0)
      return false;

    const int split = bitmap.source_height / 2;
    if (bitmap.y >= split || bitmap.y + bitmap.height <= split)
      return false;

    first = {0, 0, bitmap.source_width, split};
    second = {0, split, bitmap.source_width, bitmap.source_height};
  }
  else
    return false;

  const Bounds firstRegion = first;
  const Bounds secondRegion = second;
  first = GetVisibleBounds(bitmap, firstRegion);
  second = GetVisibleBounds(bitmap, secondRegion);
  if (first.IsEmpty() || second.IsEmpty() || first.Width() != second.Width() ||
      first.Height() != second.Height() ||
      !HaveCompatiblePositions(first, second, firstRegion, secondRegion))
    return false;

  std::size_t comparedPixels = 0;
  std::size_t mismatchedPixels = 0;
  for (int y = 0; y < first.Height(); ++y)
  {
    for (int x = 0; x < first.Width(); ++x)
    {
      const uint32_t firstPixel = GetPixel(bitmap, first.x1 + x, first.y1 + y);
      const uint32_t secondPixel = GetPixel(bitmap, second.x1 + x, second.y1 + y);
      if (firstPixel == 0 && secondPixel == 0)
        continue;

      ++comparedPixels;
      if (firstPixel != secondPixel)
        ++mismatchedPixels;
    }
  }

  return comparedPixels > 0 && mismatchedPixels * 100 <= comparedPixels;
}

std::shared_ptr<CDVDOverlayImage> CropStereoView(const CDVDOverlayImage& source,
                                                 BitmapStereoLayout layout,
                                                 DVDOverlayStereoView view)
{
  int regionX{0};
  int regionY{0};
  int regionWidth{source.source_width};
  int regionHeight{source.source_height};

  if (layout == BitmapStereoLayout::LEFT_RIGHT && source.source_width % 2 == 0)
  {
    regionWidth /= 2;
    if (view == DVDOverlayStereoView::RIGHT)
      regionX = regionWidth;
  }
  else if (layout == BitmapStereoLayout::TOP_BOTTOM && source.source_height % 2 == 0)
  {
    regionHeight /= 2;
    if (view == DVDOverlayStereoView::RIGHT)
      regionY = regionHeight;
  }
  else
    return nullptr;

  const int cropX = std::max(source.x, regionX);
  const int cropY = std::max(source.y, regionY);
  const int cropRight = std::min(source.x + source.width, regionX + regionWidth);
  const int cropBottom = std::min(source.y + source.height, regionY + regionHeight);
  if (cropX >= cropRight || cropY >= cropBottom)
    return nullptr;

  auto cropped = std::make_shared<CDVDOverlayImage>(source, cropX, cropY, cropRight - cropX,
                                                    cropBottom - cropY);
  cropped->x = cropX - regionX;
  cropped->y = cropY - regionY;
  cropped->source_width = regionWidth;
  cropped->source_height = regionHeight;
  cropped->m_stereoView = view;
  return cropped;
}
} // unnamed namespace

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
  m_pCodecContext->colorspace = hints.colorSpace;
  m_pCodecContext->color_primaries = hints.colorPrimaries;
  m_pCodecContext->color_trc = hints.colorTransferCharacteristic;
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

  if (pCodec->name != nullptr)
    SetName("ff-" + std::string(pCodec->name));

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
  m_pendingOverlay.reset();

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
  m_pendingOverlay.reset();

  avcodec_flush_buffers(m_pCodecContext);
}

std::shared_ptr<CDVDOverlay> CDVDOverlayCodecFFmpeg::GetOverlay()
{
  if (m_pendingOverlay)
  {
    return std::exchange(m_pendingOverlay, nullptr);
  }

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
    overlay->bForced = (rect.flags & AV_SUBTITLE_FLAG_FORCED);
    //! @todo for now, set hint for PGS+PQ for UHD Bluray, update for DVB+HLG if needed
    overlay->m_isHDROverlay = m_pCodecContext->codec_id == AV_CODEC_ID_HDMV_PGS_SUBTITLE &&
                              m_pCodecContext->color_trc == AVCOL_TRC_SMPTE2084;
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

    const BitmapStereoLayout layout =
        GetBitmapStereoLayout(CServiceBroker::GetDataCacheCore().GetVideoStereoMode());
    const RenderStereoMode renderMode =
        CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
    if (renderMode != RenderStereoMode::OFF && renderMode != RenderStereoMode::HARDWAREBASED &&
        layout != BitmapStereoLayout::MONO && IsStereoBitmap(*overlay, layout))
    {
      auto left = CropStereoView(*overlay, layout, DVDOverlayStereoView::LEFT);
      auto right = CropStereoView(*overlay, layout, DVDOverlayStereoView::RIGHT);
      if (left && right)
      {
        if (!m_loggedStereoSplit)
        {
          m_loggedStereoSplit = true;
          CLog::Log(LOGDEBUG, "{} - splitting {} packed stereoscopic subtitle", __FUNCTION__,
                    layout == BitmapStereoLayout::LEFT_RIGHT ? "side-by-side" : "top-and-bottom");
        }
        m_pendingOverlay = std::move(right);
        m_SubtitleIndex++;
        return left;
      }
    }
    m_SubtitleIndex++;

    return overlay;
  }

  return nullptr;
}
