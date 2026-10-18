/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HdrOutput.h"

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"

#include <algorithm>

extern "C"
{
#include <libavutil/rational.h>
}

namespace
{
// Chromaticities are given in units of 1/50000, as the HDR infoframe carries them
constexpr int CHROMATICITY_SCALE = 50000;

AVRational Chromaticity(double value)
{
  return av_make_q(static_cast<int>(value * CHROMATICITY_SCALE + 0.5), CHROMATICITY_SCALE);
}
} // namespace

unsigned int KODI::RETRO::GetHdrPeak(float displayMaxLuminance)
{
  if (displayMaxLuminance <= 0.0f)
    return HDR_DEFAULT_PEAK;

  return std::max(static_cast<unsigned int>(displayMaxLuminance + 0.5f), HDR_REFERENCE_WHITE);
}

void KODI::RETRO::GetHdrOutputPicture(VideoPicture& picture,
                                      unsigned int paperWhite,
                                      unsigned int peak)
{
  picture.Reset();

  picture.color_primaries = AVCOL_PRI_BT2020;
  picture.color_transfer = AVCOL_TRC_SMPTE2084;
  picture.color_space = AVCOL_SPC_BT2020_NCL;
  picture.color_range = 1;
  picture.colorBits = 10;
  picture.hdrType = StreamHdrType::HDR_TYPE_HDR10;

  // A game has no mastering display, so it is described as one with the BT.2020
  // primaries that reaches the peak
  AVMasteringDisplayMetadata& display = picture.displayMetadata;
  display = {};
  display.display_primaries[0][0] = Chromaticity(0.708);
  display.display_primaries[0][1] = Chromaticity(0.292);
  display.display_primaries[1][0] = Chromaticity(0.170);
  display.display_primaries[1][1] = Chromaticity(0.797);
  display.display_primaries[2][0] = Chromaticity(0.131);
  display.display_primaries[2][1] = Chromaticity(0.046);
  display.white_point[0] = Chromaticity(0.3127);
  display.white_point[1] = Chromaticity(0.3290);
  display.min_luminance = av_make_q(1, 10000);
  display.max_luminance = av_make_q(static_cast<int>(peak), 1);
  display.has_primaries = 1;
  display.has_luminance = 1;
  picture.hasDisplayMetadata = true;

  picture.lightMetadata.MaxCLL = peak;
  picture.lightMetadata.MaxFALL = std::min(paperWhite, peak);
  picture.hasLightMetadata = true;
}
