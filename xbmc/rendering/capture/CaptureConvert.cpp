/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureConvert.h"

#include "rendering/capture/CaptureTypes.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <vector>

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
#include <libswscale/swscale.h>
}

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

namespace
{

// Limited-range RGB expanded to full on a copy: swscale forces JPEG range on RGB input.
std::vector<uint8_t> ExpandRange(const CaptureResult& result)
{
  std::vector<uint8_t> out(static_cast<size_t>(result.stride) * result.height);
  std::memcpy(out.data(), result.pixels.get(), out.size());

  if (result.bitDepth > 8)
  {
    // 16-bit samples: limited range 16..235 scaled by 257
    constexpr int lo = 16 * 257;
    constexpr int hi = 235 * 257;
    uint16_t* samples = reinterpret_cast<uint16_t*>(out.data());
    const size_t count = out.size() / 2;
    for (size_t i = 0; i < count; i++)
    {
      if (i % 4 == 3)
        continue; // alpha
      const int64_t v = static_cast<int64_t>(samples[i] - lo) * 65535 / (hi - lo);
      samples[i] = static_cast<uint16_t>(std::clamp<int64_t>(v, 0, 65535));
    }
  }
  else
  {
    constexpr int lo = 16;
    constexpr int hi = 235;
    for (size_t i = 0; i < out.size(); i++)
    {
      if (i % 4 == 3)
        continue; // alpha
      const int v = (static_cast<int>(out[i]) - lo) * 255 / (hi - lo);
      out[i] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
  }
  return out;
}

// Color-managed conversion honoring the capture's tags: PQ/HLG BT.2020 input
// tonemapped to 8-bit sRGB BT.709, matching what the thumbnail extractor does.
bool ConvertColorManaged(const CaptureResult& result,
                         unsigned int width,
                         unsigned int height,
                         uint8_t* buffer)
{
  const AVPixelFormat srcFormat = result.bitDepth > 8 ? AV_PIX_FMT_RGBA64LE : AV_PIX_FMT_BGRA;

  // limited-range input must be pre-expanded; the expanded copy outlives sws
  std::vector<uint8_t> expanded;
  const uint8_t* srcData = result.pixels.get();
  if (result.color.range == AVCOL_RANGE_MPEG)
  {
    expanded = ExpandRange(result);
    srcData = expanded.data();
  }

  bool converted = false;

#if LIBSWSCALE_BUILD >= AV_VERSION_INT(9, 0, 100)
  {
    AVFrame* srcFrame = av_frame_alloc();
    AVFrame* dstFrame = av_frame_alloc();
    SwsContext* sws = sws_alloc_context();
    if (srcFrame && dstFrame && sws)
    {
      srcFrame->width = static_cast<int>(result.width);
      srcFrame->height = static_cast<int>(result.height);
      srcFrame->format = srcFormat;
      srcFrame->data[0] = const_cast<uint8_t*>(srcData);
      srcFrame->linesize[0] = static_cast<int>(result.stride);
      srcFrame->colorspace = AVCOL_SPC_RGB;
      srcFrame->color_range = AVCOL_RANGE_JPEG;
      srcFrame->color_primaries = static_cast<AVColorPrimaries>(result.color.primaries);
      srcFrame->color_trc = static_cast<AVColorTransferCharacteristic>(result.color.transfer);

      // the source peak: without it swscale assumes the full PQ range and crushes
      if (result.hasDisplayMetadata)
      {
        AVMasteringDisplayMetadata* mdm = av_mastering_display_metadata_create_side_data(srcFrame);
        if (mdm)
          *mdm = result.displayMetadata;
      }
      if (result.hasLightMetadata)
      {
        AVContentLightMetadata* clm = av_content_light_metadata_create_side_data(srcFrame);
        if (clm)
          *clm = result.lightMetadata;
      }

      dstFrame->width = static_cast<int>(width);
      dstFrame->height = static_cast<int>(height);
      dstFrame->format = AV_PIX_FMT_BGRA;
      dstFrame->data[0] = buffer;
      dstFrame->linesize[0] = static_cast<int>(width * 4);
      dstFrame->colorspace = AVCOL_SPC_RGB;
      dstFrame->color_range = AVCOL_RANGE_JPEG;
      dstFrame->color_primaries = AVCOL_PRI_BT709;
      dstFrame->color_trc = AVCOL_TRC_BT709;

      sws->flags = SWS_BILINEAR;
      // stated explicitly: the default intent is relative colorimetric, which clips
      sws->intent = SWS_INTENT_PERCEPTUAL;

      const int res = sws_scale_frame(sws, dstFrame, srcFrame);
      if (res < 0)
        CLog::LogF(LOGWARNING, "sws_scale_frame failed ({}), using legacy conversion", res);
      else
        converted = true;
    }
    sws_free_context(&sws);
    av_frame_free(&srcFrame);
    av_frame_free(&dstFrame);
  }
#endif

  if (!converted)
  {
    // pre-CMS swscale cannot tonemap: plain-convert so the image at least has
    // the right size and depth; the output coding stays baked in
    SwsContext* context = sws_getContext(static_cast<int>(result.width),
                                         static_cast<int>(result.height), srcFormat,
                                         static_cast<int>(width), static_cast<int>(height),
                                         AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!context)
      return false;

    const uint8_t* src[] = {srcData, nullptr, nullptr, nullptr};
    const int srcStride[] = {static_cast<int>(result.stride), 0, 0, 0};
    uint8_t* dst[] = {buffer, nullptr, nullptr, nullptr};
    const int dstStride[] = {static_cast<int>(width * 4), 0, 0, 0};
    sws_scale(context, src, srcStride, 0, static_cast<int>(result.height), dst, dstStride);
    sws_freeContext(context);
    converted = true;
  }

  return converted;
}

} // namespace

bool CaptureCopyBGRA8(const CaptureResult& result,
                      unsigned int width,
                      unsigned int height,
                      uint8_t* buffer)
{
  if (!result.pixels || result.width == 0 || result.height == 0 || width == 0 || height == 0)
    return false;

  if (result.width == width && result.height == height)
  {
    // matching size: stride-aware row copy, the tap already delivers BGRA
    for (unsigned int y = 0; y < height; y++)
      std::memcpy(buffer + y * width * 4, result.pixels.get() + y * result.stride, width * 4);
    return true;
  }

  // size mismatch: native-size copies from platforms without a scaling blit
  SwsContext* context = sws_getContext(static_cast<int>(result.width),
                                       static_cast<int>(result.height), AV_PIX_FMT_BGRA,
                                       static_cast<int>(width), static_cast<int>(height),
                                       AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!context)
    return false;

  uint8_t* src[] = {result.pixels.get(), nullptr, nullptr, nullptr};
  const int srcStride[] = {static_cast<int>(result.stride), 0, 0, 0};
  uint8_t* dst[] = {buffer, nullptr, nullptr, nullptr};
  const int dstStride[] = {static_cast<int>(width * 4), 0, 0, 0};
  sws_scale(context, src, srcStride, 0, static_cast<int>(result.height), dst, dstStride);
  sws_freeContext(context);
  return true;
}

bool CaptureToBGRA(const CaptureResult& result,
                   unsigned int width,
                   unsigned int height,
                   uint8_t* buffer)
{
  if (!result.pixels || result.width == 0 || result.height == 0 || width == 0 || height == 0)
    return false;

  // tonemap by tags, not depth: an 8-bit PQ capture still needs the treatment
  const bool hdr = result.color.transfer == AVCOL_TRC_SMPTE2084 ||
                   result.color.transfer == AVCOL_TRC_ARIB_STD_B67;

  if (result.bitDepth > 8 || hdr)
    return ConvertColorManaged(result, width, height, buffer);

  return CaptureCopyBGRA8(result, width, height, buffer);
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
