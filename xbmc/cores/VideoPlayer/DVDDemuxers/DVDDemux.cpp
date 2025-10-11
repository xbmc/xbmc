/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemux.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"

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
    strInfo = g_localizeStrings.Get(13205); // "Unknown"

  strInfo.append(" ");
  strInfo.append(StreamUtils::GetLayout(iChannels));

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
