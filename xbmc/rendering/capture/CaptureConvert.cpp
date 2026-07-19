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

#include <cstring>

extern "C"
{
#include <libswscale/swscale.h>
}

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

bool CaptureToBGRA(const CaptureResult& result,
                   unsigned int width,
                   unsigned int height,
                   uint8_t* buffer)
{
  if (!result.pixels || result.width == 0 || result.height == 0 || width == 0 || height == 0)
    return false;

  if (result.bitDepth > 8)
  {
    CLog::LogF(LOGERROR, "no conversion for {}-bit captures", result.bitDepth);
    return false;
  }

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

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
