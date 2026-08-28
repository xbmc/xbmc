/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StreamUtils.h"

#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"

#include <algorithm>
#include <array>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/defs.h>
}

int StreamUtils::GetCodecPriority(const std::string &codec)
{
  /*
   * Every name StreamUtils::GetCodecName() and the bluray stream parser can produce is ranked, so
   * that 0 means a codec genuinely nobody knows rather than a common one nobody listed. See
   * CompareAudioQuality(), which orders codec before channel count for surround streams and so
   * relies on the ranks being meaningful.
   *
   * Priority   Codecs
   * 13         truehd_atmos, dtshd_ma_x_imax, dtshd_ma_x — lossless + objects
   * 12         truehd, dtshd_ma, mlp - lossless, can be bit-streamed (offload processing)
   * 11         flac, alac, ape, wavpack, pcm_bluray and any LPCM of 24 bit or better - lossless. pcm_bluray carries no bit depth in its name and can be 16, 20 or 24 bit,
   *            but a primary disc track is 24 and nothing records the real depth, so it is ranked
   *            here rather than guessed downwards. flac, alac and wavpack carry no depth either
   * 10         dtshd_hra - lossy, high resolution
   * 9          dtshd - DTS-HD of an undetermined flavour, so the poorest one it could be
   * 8          eac3_ddp_atmos - lossy + objects
   * 7          pcm, pcm_dvd, and any LPCM of 16 bit - lossless, or with nothing known about it
   * 6          ac4, dts_es, dts_96_24 - Dolby AC-4, and DTS carrying extended surround or
   *            96kHz/24-bit, which at the bitrates they are shipped at outweigh Dolby Digital Plus
   * 5          eac3, opus - the efficient modern lossy codecs
   * 4          dts
   * 3          aac_lc, aac, aac_latm, aac_ltp, aac_ssr, vorbis, wmapro. A bare aac is ranked with aac_lc
   *            rather than at the family floor, as that is overwhelmingly what it turns out to be
   * 2          ac3, he_aac, he_aac_v2 - the HE profiles signal low bitrate content
   * 1          mp3, mp2, mp1, wmav2, dts_express, and 8 bit LPCM - oldest, built for low bitrate
   *            secondary audio, or genuinely poor
   * 0          anything else - genuinely unknown
   */
  if (codec == "truehd_atmos") // Dolby TrueHD with Atmos
    return 13;
  if (codec == "dtshd_ma_x_imax") // DTS:X IMAX Enhanced
    return 13;
  if (codec == "dtshd_ma_x") // DTS:X
    return 13;
  if (codec == "truehd") // Dolby TrueHD
    return 12;
  if (codec == "dtshd_ma") // DTS-HD Master Audio (previously known as DTS++)
    return 12;
  if (codec == "mlp") // Meridian Lossless Packing, Dolby TrueHD's predecessor
    return 12;
  if (codec == "flac") // Lossless FLAC
    return 11;
  if (codec == "alac") // Apple Lossless
    return 11;
  if (codec == "ape") // Monkey's Audio
    return 11;
  if (codec == "wavpack") // WavPack, lossless outside its hybrid mode
    return 11;
  if (codec == "pcm_bluray") // Uncompressed LPCM from a disc, taken to be 24 bit - see above
    return 11;
  if (codec == "dtshd_hra") // DTS-HD High Resolution Audio
    return 10;
  if (codec == "dtshd") // DTS-HD, flavour undetermined - a better one would have been detected
    return 9;
  if (codec == "eac3_ddp_atmos") // Dolby Digital Plus with Atmos
    return 8;
  if (codec == "pcm_dvd") // Uncompressed LPCM from a DVD, usually 16 bit
    return 7;
  if (codec == "pcm") // LPCM with no parsed substream information, so an unknown channel count
    return 7;
  if (codec == "ac4") // Dolby AC-4
    return 6;
  if (codec == "dts_es") // DTS Extended Surround
    return 6;
  if (codec == "dts_96_24") // DTS 96kHz/24-bit
    return 6;
  if (codec == "eac3") // Dolby Digital Plus
    return 5;
  if (codec == "opus") // Opus
    return 5;
  if (codec == "dts") // DTS
    return 4;
  if (codec == "aac_lc") // AAC Low Complexity
    return 3;
  if (codec == "aac_latm") // AAC Low Complexity in LATM framing, as broadcast streams carry it
    return 3;
  if (codec == "aac") // AAC of an unstated profile, which is Low Complexity in all but name
    return 3;
  if (codec == "aac_ltp") // AAC Long Term Prediction
    return 3;
  if (codec == "aac_ssr") // AAC Scalable Sample Rate
    return 3;
  if (codec == "vorbis") // Ogg Vorbis
    return 3;
  if (codec == "wmapro") // Windows Media Audio Professional
    return 3;
  if (codec == "ac3") // Dolby Digital
    return 2;
  if (codec == "he_aac") // AAC High Efficiency
    return 2;
  if (codec == "he_aac_v2") // AAC High Efficiency v2
    return 2;
  if (codec == "mp3") // MPEG-1 Audio Layer III
    return 1;
  if (codec == "mp2") // MPEG-1 Audio Layer II
    return 1;
  if (codec == "mp1") // MPEG-1 Audio Layer I
    return 1;
  if (codec == "wmav2") // Windows Media Audio 2
    return 1;
  if (codec == "dts_express") // DTS Express, a low bitrate secondary audio format
    return 1;
  // CDemuxStreamAudio::GetStreamType() recognises every ffmpeg PCM codec as one range, so the
  // family is matched on prefix rather than spelled out. Byte order and planarity say nothing
  // about quality - pcm_s24be is the same audio as pcm_s24le - and new spellings appear from time
  // to time, so only the bit depth is read.
  if (codec.starts_with("pcm_"))
  {
    for (const auto* prefix : {"pcm_s24", "pcm_u24", "pcm_s32", "pcm_u32", "pcm_s64", "pcm_u64",
                               "pcm_f24", "pcm_f32", "pcm_f64"})
    {
      if (codec.starts_with(prefix))
        return 11;
    }

    for (const auto* prefix : {"pcm_s16", "pcm_u16", "pcm_f16"})
    {
      if (codec.starts_with(prefix))
        return 7;
    }

    // 8 bit, or companded down to 8 bit, which is the one LPCM form that is genuinely poor
    for (const auto* name : {"pcm_s8", "pcm_u8", "pcm_alaw", "pcm_mulaw"})
    {
      if (codec == name)
        return 1;
    }
  }

  return 0;
}

int StreamUtils::CompareAudioQuality(const std::string& codecA,
                                     int channelsA,
                                     const std::string& codecB,
                                     int channelsB)
{
  const int priorityA{GetCodecPriority(codecA)};
  const int priorityB{GetCodecPriority(codecB)};

  // A channel count of zero or less means unknown, and the sources disagree on which of those to
  // use, so normalise them to compare equal rather than ranking one unknown above another.
  const int knownChannelsA{std::max(0, channelsA)};
  const int knownChannelsB{std::max(0, channelsB)};

  // The comparison has to be a strict weak ordering, because VideoPlayer hands it to
  // std::stable_sort. Streams are therefore split into surround and not-surround, which each
  // stream decides for itself, and then ranked within that group on a fixed pair of keys. Nothing
  // here may depend on which two streams are being compared, or the ordering can cycle.
  const bool surroundA{knownChannelsA > 2};
  const bool surroundB{knownChannelsB > 2};
  if (surroundA != surroundB)
    return surroundA ? 1 : -1;

  if (surroundA)
  {
    // Beyond stereo the codec describes the stream better than the channel count does, so 5.1
    // TrueHD is a better listen than 7.1 AC3. Every codec the stream details can carry is ranked,
    // so a rank of 0 really is an unknown codec and belongs at the bottom.
    if (priorityA != priorityB)
      return priorityA > priorityB ? 1 : -1;

    // Codecs of equal rank are equally good, so the wider presentation wins
    if (knownChannelsA != knownChannelsB)
      return knownChannelsA > knownChannelsB ? 1 : -1;

    return 0;
  }

  // At or below stereo the step up towards surround outweighs any codec difference
  if (knownChannelsA != knownChannelsB)
    return knownChannelsA > knownChannelsB ? 1 : -1;

  // In case of a tie, revert to codec priority
  if (priorityA != priorityB)
    return priorityA > priorityB ? 1 : -1;

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
    else if (profile == AV_PROFILE_DTS_ES)
      codecName = "dts_es";
    else if (profile == AV_PROFILE_DTS_96_24)
      codecName = "dts_96_24";
    else if (profile == AV_PROFILE_DTS_EXPRESS)
      codecName = "dts_express";
    else
      codecName = "dts";

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

std::string StreamUtils::NormalizeAudioCodecName(const std::string& codec)
{
  // 'dca' (name of ffmpeg's DTS decoder) was previously used in error for 'dts'
  // Corrected in database schema v145
  if (codec == "dca")
    return "dts";

  return codec;
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
    layout.append(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(10127)); // "channels"
  }

  return layout;
}

bool StreamUtils::IsCodecSupportForcedOverlay(int codecId)
{
  return codecId == AV_CODEC_ID_DVD_SUBTITLE || codecId == AV_CODEC_ID_HDMV_PGS_SUBTITLE ||
         codecId == AV_CODEC_ID_DVB_SUBTITLE;
}
