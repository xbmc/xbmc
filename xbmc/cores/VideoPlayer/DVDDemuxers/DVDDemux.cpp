/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemux.h"

#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/Map.h"
#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"

#include <string_view>

std::string CDemuxStreamAudio::GetStreamType() const
{
  std::string strInfo;
  switch (codec)
  {
    case AV_CODEC_ID_AC3:
      strInfo = "AC3";
      break;
    case AV_CODEC_ID_AC4:
      strInfo = "AC4";
      break;
    case AV_CODEC_ID_EAC3:
    {
      if (profile == AV_PROFILE_EAC3_DDP_ATMOS)
        strInfo = "DD+ ATMOS";
      else
        strInfo = "DD+";
      break;
    }
    case AV_CODEC_ID_DTS:
    {
      switch (profile)
      {
        case AV_PROFILE_DTS_96_24:
          strInfo = "DTS 96/24";
          break;
        case AV_PROFILE_DTS_ES:
          strInfo = "DTS ES";
          break;
        case AV_PROFILE_DTS_EXPRESS:
          strInfo = "DTS EXPRESS";
          break;
        case AV_PROFILE_DTS_HD_MA:
          strInfo = "DTS-HD MA";
          break;
        case AV_PROFILE_DTS_HD_HRA:
          strInfo = "DTS-HD HRA";
          break;
        case AV_PROFILE_DTS_HD_MA_X:
          strInfo = "DTS-HD MA X";
          break;
        case AV_PROFILE_DTS_HD_MA_X_IMAX:
          strInfo = "DTS-HD MA X (IMAX)";
          break;
        default:
          strInfo = "DTS";
          break;
      }
      break;
    }
    case AV_CODEC_ID_MP2:
      strInfo = "MP2";
      break;
    case AV_CODEC_ID_MP3:
      strInfo = "MP3";
      break;
    case AV_CODEC_ID_TRUEHD:
      if (profile == AV_PROFILE_TRUEHD_ATMOS)
        strInfo = "TrueHD ATMOS";
      else
        strInfo = "TrueHD";
      break;
    case AV_CODEC_ID_AAC:
    {
      switch (profile)
      {
        case AV_PROFILE_AAC_LOW:
        case AV_PROFILE_MPEG2_AAC_LOW:
          strInfo = "AAC-LC";
          break;
        case AV_PROFILE_AAC_HE:
        case AV_PROFILE_MPEG2_AAC_HE:
          strInfo = "HE-AAC";
          break;
        case AV_PROFILE_AAC_HE_V2:
          strInfo = "HE-AACv2";
          break;
        case AV_PROFILE_AAC_SSR:
          strInfo = "AAC-SSR";
          break;
        case AV_PROFILE_AAC_LTP:
          strInfo = "AAC-LTP";
          break;
        default:
        {
          // Try check by codec full string according to RFC 6381
          if (codecName == "mp4a.40.2" || codecName == "mp4a.40.17")
            strInfo = "AAC-LC";
          else if (codecName == "mp4a.40.3")
            strInfo = "AAC-SSR";
          else if (codecName == "mp4a.40.4" || codecName == "mp4a.40.19")
            strInfo = "AAC-LTP";
          else if (codecName == "mp4a.40.5")
            strInfo = "HE-AAC";
          else if (codecName == "mp4a.40.29")
            strInfo = "HE-AACv2";
          else
            strInfo = "AAC";
          break;
        }
      }
      break;
    }
    case AV_CODEC_ID_ALAC:
      strInfo = "ALAC";
      break;
    case AV_CODEC_ID_FLAC:
      strInfo = "FLAC";
      break;
    case AV_CODEC_ID_OPUS:
      strInfo = "Opus";
      break;
    case AV_CODEC_ID_VORBIS:
      strInfo = "Vorbis";
      break;
    default:
      break;
  }

  if (codec >= AV_CODEC_ID_PCM_S16LE && codec <= AV_CODEC_ID_PCM_SGA)
    strInfo = "PCM";

  if (strInfo.empty())
    strInfo = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13205); // "Unknown"

  strInfo.append(" ");
  strInfo.append(StreamUtils::GetLayout(iChannels));

  return strInfo;
}

constexpr auto subtitleTypes = make_map<int, std::string_view>({
    {AV_CODEC_ID_DVD_SUBTITLE, "VobSub"},
    {AV_CODEC_ID_DVB_SUBTITLE, "DVB-SUB"},
    {AV_CODEC_ID_SSA, "SSA"},
    {AV_CODEC_ID_MOV_TEXT, "TTXT"}, // mov-text, timed text
    {AV_CODEC_ID_TEXT, "Text"},
    {AV_CODEC_ID_XSUB, "XSUB"},
    {AV_CODEC_ID_HDMV_PGS_SUBTITLE, "PGS"},
    {AV_CODEC_ID_DVB_TELETEXT, "DVB-TXT"},
    {AV_CODEC_ID_SRT, "SubRip"},
    {AV_CODEC_ID_MICRODVD, "MicroDVD"},
    {AV_CODEC_ID_SAMI, "SAMI"},
    {AV_CODEC_ID_REALTEXT, "RealText"},
    {AV_CODEC_ID_SUBRIP, "SubRip"},
    {AV_CODEC_ID_WEBVTT, "WebVTT"},
    {AV_CODEC_ID_MPL2, "MPL2"},
    {AV_CODEC_ID_VPLAYER, "VPlayer"},
    {AV_CODEC_ID_ASS, "ASS"},
    {AV_CODEC_ID_TTML, "TTML"},
});

std::string CDemuxStreamSubtitle::GetStreamType() const
{
  std::string strInfo;

  if (auto it = subtitleTypes.find(codec); it != subtitleTypes.cend())
    strInfo = it->second;

  return strInfo;
}

int CDVDDemux::GetNrOfStreams(StreamType streamType) const
{
  int iCounter = 0;

  for (auto pStream : GetStreams())
  {
    if (pStream && pStream->type == streamType)
      iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfSubtitleStreams() const
{
  return GetNrOfStreams(StreamType::SUBTITLE);
}

std::string CDemuxStream::GetStreamName()
{
  return name;
}
