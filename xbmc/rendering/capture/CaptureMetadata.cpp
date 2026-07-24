/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureMetadata.h"

#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

ImageColorMetadata GetOutputColorMetadata(CWinSystemBase& winSystem)
{
  // Tag color as the display interpreted it. PQ/HLG from the winsystem HDR
  // state; SDR by CTA-861 mode inference (HD/UHD = BT.709, SD = SMPTE-170M).
  using KODI::UTILS::Colorimetry;
  using KODI::UTILS::Eotf;
  ImageColorMetadata color;
  color.primaries = AVCOL_PRI_BT709;
  const Colorimetry colorimetry = winSystem.GetColorimetry();
  if (colorimetry == Colorimetry::BT2020_RGB || colorimetry == Colorimetry::BT2020_YCC ||
      colorimetry == Colorimetry::BT2020_CYCC)
    color.primaries = AVCOL_PRI_BT2020;
  const Eotf eotf = winSystem.GetEotf();
  if (eotf == Eotf::PQ)
    color.transfer = AVCOL_TRC_SMPTE2084;
  else if (eotf == Eotf::HLG)
    color.transfer = AVCOL_TRC_ARIB_STD_B67;
  //! @todo tag AVCOL_TRC_BT2020_12 once a >10-bit output surface exists:
  //! 16-bit unorm AR48/AB48 (DRM_FORMAT_ARGB16161616/ABGR16161616) or
  //! 16-bit float AR4H/AB4H (DRM_FORMAT_ARGB16161616F/ABGR16161616F)
  else if (color.primaries == AVCOL_PRI_BT2020)
    color.transfer = AVCOL_TRC_BT2020_10;
  else if (winSystem.GetGfxContext().GetWidth() > 1024 ||
           winSystem.GetGfxContext().GetHeight() >= 600)
    color.transfer = AVCOL_TRC_BT709;
  else
  {
    color.primaries = AVCOL_PRI_SMPTE170M;
    color.transfer = AVCOL_TRC_SMPTE170M;
  }

  // Display output is limited-range whenever videoscreen.limitedrange is on.
  color.range = winSystem.UseLimitedColor() ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;
  return color;
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
