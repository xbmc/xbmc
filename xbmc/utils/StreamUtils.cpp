/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StreamUtils.h"

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
