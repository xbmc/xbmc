/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicCodecInfoFFmpeg.h"

#include "cores/FFmpeg.h"
#include "filesystem/FFmpegVfsContext.h"

using namespace XFILE;

bool CMusicCodecInfoFFmpeg::GetMusicCodecInfo(AVFormatContext* fctx, musicCodecInfo& codec_info)
{
  if (!fctx)
    return false;

  const AVCodec* decoder = nullptr;
  bool haveInfo = false;
  AVStream* st = nullptr;
  int streamIndex = -1;
  // Look for the default audio stream first
  for (unsigned int i = 0; i < fctx->nb_streams; ++i)
  {
    if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      if (fctx->streams[i]->disposition & AV_DISPOSITION_DEFAULT)
      {
        streamIndex = i;
        break; // Found the default audio stream, no need to check further
      }
    }
  }
  if (streamIndex == -1)
  {
    for (unsigned int i = 0; i < fctx->nb_streams; ++i)
    {
      if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
      {
        streamIndex = i;
        break; // Found the first audio stream
      }
    }
  }

  if (streamIndex != -1)
  {
    st = fctx->streams[streamIndex];
    decoder = avcodec_find_decoder(st->codecpar->codec_id);
    if (decoder)
    {
      std::string codec_name = "unknown";

      codec_name = avcodec_get_name(st->codecpar->codec_id);
      int par_profile = st->codecpar->profile;
      if (st->codecpar->codec_id == AV_CODEC_ID_DTS)
      {
        switch (par_profile)
        {
          case AV_PROFILE_DTS_HD_MA_X:
            codec_name = "dtshd_ma_x";
            break;
          case AV_PROFILE_DTS_HD_MA_X_IMAX:
            codec_name = "dtshd_ma_x_imax";
            break;
          case AV_PROFILE_DTS_ES:
            codec_name = "dts_es";
            break;
          case AV_PROFILE_DTS_96_24:
            codec_name = "dts_96_24";
            break;
          case AV_PROFILE_DTS_HD_HRA:
            codec_name = "dtshd_hra";
            break;
          case AV_PROFILE_DTS_EXPRESS:
            codec_name = "dts_express";
            break;
          case AV_PROFILE_DTS_HD_MA:
            codec_name = "dtshd_ma";
            break;
          default:
            codec_name = "dca";
            break;
        }
      }
      if (st->codecpar->codec_id == AV_CODEC_ID_EAC3 && par_profile == AV_PROFILE_EAC3_DDP_ATMOS)
        codec_name = "eac3_ddp_atmos";

      if (st->codecpar->codec_id == AV_CODEC_ID_TRUEHD && par_profile == AV_PROFILE_TRUEHD_ATMOS)
        codec_name = "truehd_atmos";
      codec_info.codecName = codec_name;
      codec_info.bitRate = static_cast<int>(st->codecpar->bit_rate / 1000);
      codec_info.channels = st->codecpar->ch_layout.nb_channels;
      codec_info.bitsPerSample = (st->codecpar->bits_per_coded_sample != 0)
                                     ? st->codecpar->bits_per_coded_sample
                                     : st->codecpar->bits_per_raw_sample;
      codec_info.sampleRate = st->codecpar->sample_rate;
      // st->duration is in st->time_base units; rescale to whole seconds.
      // Fall back to the container duration when the stream value is unset.
      if (st->duration != AV_NOPTS_VALUE)
        codec_info.duration =
            static_cast<int>(av_rescale_q(st->duration, st->time_base, AVRational{1, 1}));
      else if (fctx->duration != AV_NOPTS_VALUE)
        codec_info.duration = static_cast<int>(fctx->duration / AV_TIME_BASE);
      else
        codec_info.duration = 0;
      haveInfo = true;
    }
  }

  return haveInfo;
}

bool CMusicCodecInfoFFmpeg::GetMusicCodecInfo(const std::string& strFileName,
                                              musicCodecInfo& codec_info)
{
  CFFmpegVfsContext demux;
  // Nothing but codec info is wanted here, and there is none to be had without the streams.
  if (!demux.Open(strFileName) || !demux.HasStreamInfo())
    return false;

  return GetMusicCodecInfo(demux.FormatContext(), codec_info);
}
