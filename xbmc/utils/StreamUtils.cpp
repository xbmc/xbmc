/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StreamUtils.h"

#include "guilib/LocalizeStrings.h"

#include <array>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/defs.h>
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
    if (profile == AV_PROFILE_DTS_HD_MA)
      codecName = "dtshd_ma";
    else if (profile == AV_PROFILE_DTS_HD_MA_X)
      codecName = "dtshd_ma_x";
    else if (profile == AV_PROFILE_DTS_HD_MA_X_IMAX)
      codecName = "dtshd_ma_x_imax";
    else if (profile == AV_PROFILE_DTS_HD_HRA)
      codecName = "dtshd_hra";
    else
      codecName = "dca";

    return codecName;
  }

  if (codecId == AV_CODEC_ID_AAC)
  {
    switch (profile)
    {
      case AV_PROFILE_AAC_LOW:
      case AV_PROFILE_MPEG2_AAC_LOW:
        codecName = "aac_lc";
        break;
      case AV_PROFILE_AAC_HE:
      case AV_PROFILE_MPEG2_AAC_HE:
        codecName = "he_aac";
        break;
      case AV_PROFILE_AAC_HE_V2:
        codecName = "he_aac_v2";
        break;
      case AV_PROFILE_AAC_SSR:
        codecName = "aac_ssr";
        break;
      case AV_PROFILE_AAC_LTP:
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

std::string StreamUtils::GetDefaultLayout(unsigned int channels)
{
  static constexpr std::array layouts{
      "0.0", // 0
      "1.0", // 1
      "2.0", // 2
      "2.1", // 3
      "4.0", // 4
      "5.0", // 5
      "5.1", // 6
      "6.1", // 7
      "7.1", // 8
      "", // 9
      "5.1.4", // 10
      "", // 11
      "7.1.4", // 12
      "", // 13
      "9.1.4", // 14
      "", // 15
      "9.1.6", // 16
  };

  if (channels < layouts.size())
    return layouts[channels];

  return {};
}

std::string StreamUtils::GetLayout(unsigned int channels)
{
  std::string layout{GetDefaultLayout(channels)};

  if (layout.empty())
  {
    layout = std::to_string(channels);
    layout.append(" ");
    layout.append(g_localizeStrings.Get(10127)); // "channels"
  }

  return layout;
}
