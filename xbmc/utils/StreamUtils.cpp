/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StreamUtils.h"

#include <array>
#include <bit>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int StreamUtils::GetCodecPriority(const std::string &codec)
{
  /*
   * Technically flac, truehd, and dtshd_ma are equivalently good as they're all lossless. However,
   * ffmpeg can't decode dtshd_ma losslessy yet.
   */
  if (codec == "flac") // Lossless FLAC
    return 7;
  if (codec == "truehd") // Dolby TrueHD
    return 6;
  if (codec == "dtshd_ma") // DTS-HD Master Audio (previously known as DTS++)
    return 5;
  if (codec == "dtshd_hra") // DTS-HD High Resolution Audio
    return 4;
  if (codec == "eac3") // Dolby Digital Plus
    return 3;
  if (codec == "dca") // DTS
    return 2;
  if (codec == "ac3") // Dolby Digital
    return 1;
  return 0;
}

std::string StreamUtils::GetCodecName(int codecId, int profile)
{
  std::string codecName;

  if (codecId == AV_CODEC_ID_DTS)
  {
    if (profile == FF_PROFILE_DTS_HD_MA)
      codecName = "dtshd_ma";
    else if (profile == FF_PROFILE_DTS_HD_MA_X)
      codecName = "dtshd_ma_x";
    else if (profile == FF_PROFILE_DTS_HD_MA_X_IMAX)
      codecName = "dtshd_ma_x_imax";
    else if (profile == FF_PROFILE_DTS_HD_HRA)
      codecName = "dtshd_hra";
    else
      codecName = "dca";

    return codecName;
  }

  if (codecId == AV_CODEC_ID_AAC)
  {
    switch (profile)
    {
      case FF_PROFILE_AAC_LOW:
      case FF_PROFILE_MPEG2_AAC_LOW:
        codecName = "aac_lc";
        break;
      case FF_PROFILE_AAC_HE:
      case FF_PROFILE_MPEG2_AAC_HE:
        codecName = "he_aac";
        break;
      case FF_PROFILE_AAC_HE_V2:
        codecName = "he_aac_v2";
        break;
      case FF_PROFILE_AAC_SSR:
        codecName = "aac_ssr";
        break;
      case FF_PROFILE_AAC_LTP:
        codecName = "aac_ltp";
        break;
      default:
        codecName = "aac";
    }
    return codecName;
  }

  if (codecId == AV_CODEC_ID_EAC3 && profile == AV_PROFILE_EAC3_DDP_ATMOS)
    return "eac3_ddp_atmos";

  if (codecId == AV_CODEC_ID_TRUEHD && profile == AV_PROFILE_TRUEHD_ATMOS)
    return "truehd_atmos";

  const AVCodec* codec = avcodec_find_decoder(static_cast<AVCodecID>(codecId));
  if (codec)
    codecName = avcodec_get_name(codec->id);

  return codecName;
}

std::string StreamUtils::GetLayoutXYZ(uint64_t mask)
{
  if (mask == 0)
    return {};

  constexpr uint64_t standardMask{
      AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT | AV_CH_FRONT_CENTER | AV_CH_BACK_LEFT |
      AV_CH_BACK_RIGHT | AV_CH_FRONT_LEFT_OF_CENTER | AV_CH_FRONT_RIGHT_OF_CENTER |
      AV_CH_BACK_CENTER | AV_CH_SIDE_LEFT | AV_CH_SIDE_RIGHT | AV_CH_STEREO_LEFT |
      AV_CH_STEREO_RIGHT | AV_CH_WIDE_LEFT | AV_CH_WIDE_RIGHT | AV_CH_SURROUND_DIRECT_LEFT |
      AV_CH_SURROUND_DIRECT_RIGHT | AV_CH_SIDE_SURROUND_LEFT | AV_CH_SIDE_SURROUND_RIGHT};
  constexpr uint64_t lfeMask{AV_CH_LOW_FREQUENCY | AV_CH_LOW_FREQUENCY_2};
  constexpr uint64_t topMask{AV_CH_TOP_CENTER | AV_CH_TOP_FRONT_LEFT | AV_CH_TOP_FRONT_CENTER |
                             AV_CH_TOP_FRONT_RIGHT | AV_CH_TOP_BACK_LEFT | AV_CH_TOP_BACK_CENTER |
                             AV_CH_TOP_BACK_RIGHT | AV_CH_TOP_SIDE_LEFT | AV_CH_TOP_SIDE_RIGHT |
                             AV_CH_TOP_SURROUND_LEFT | AV_CH_TOP_SURROUND_RIGHT};

  const int standardChannels{std::popcount(mask & standardMask)};
  const int lfeChannels{std::popcount(mask & lfeMask)};
  const int topChannels{std::popcount(mask & topMask)};

  std::string layout = std::to_string(standardChannels);
  layout.append(".");
  layout.append(std::to_string(lfeChannels));
  if (topChannels > 0)
  {
    layout.append(".");
    layout.append(std::to_string(topChannels));
  }
  return layout;
}

namespace
{
constexpr uint64_t UNKNOWN_MASK{0};

constexpr std::array<uint64_t, 15> DEFAULT_MASKS{
    UNKNOWN_MASK, // 0 > no default
    AV_CH_LAYOUT_MONO, // 1 > 1.0
    AV_CH_LAYOUT_STEREO, // 2 > 2.0
    AV_CH_LAYOUT_2POINT1, // 3 > 2.1
    AV_CH_LAYOUT_4POINT0, // 4 > 4.0, historical default of Estuary
    AV_CH_LAYOUT_QUAD | AV_CH_LOW_FREQUENCY, // 5 > 5.0, historical default of Estuary
    AV_CH_LAYOUT_5POINT1, // 6 > 5.1
    AV_CH_LAYOUT_6POINT1, // 7 > 6.1
    AV_CH_LAYOUT_7POINT1, // 8 > 7.1
    UNKNOWN_MASK, // 9 > no default
    AV_CH_LAYOUT_7POINT1 | AV_CH_FRONT_LEFT_OF_CENTER |
        AV_CH_FRONT_RIGHT_OF_CENTER, // 10 > 5.1.4, historical default of Estuary
    UNKNOWN_MASK, // 11 > no default
    AV_CH_LAYOUT_7POINT1POINT4_BACK, // 12 > 7.1.4
    UNKNOWN_MASK, // 13 > no default
    AV_CH_LAYOUT_9POINT1POINT4_BACK, // 14 > 9.1.4
};
} // namespace

uint64_t StreamUtils::GetDefaultMask(int channels)
{
  if (channels < DEFAULT_MASKS.size())
    return DEFAULT_MASKS[channels];

  return UNKNOWN_MASK;
}
