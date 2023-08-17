/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemux.h"

#include "utils/StringUtils.h"

std::string CDemuxStreamAudio::GetStreamType()
{
  std::string strInfo;
  switch (codec)
  {
    //! @todo: With ffmpeg >= 6.1 add new AC4 codec
    case AV_CODEC_ID_AC3:
      strInfo = "AC3 ";
      break;
    case AV_CODEC_ID_EAC3:
    {
      //! @todo: With ffmpeg >= 6.1 add new atmos profile case
      // "JOC" its EAC3 Atmos underlying profile, there is no standard codec name string
      if (StringUtils::Contains(codecName, "JOC"))
        strInfo = "DD+ ATMOS ";
      else
        strInfo = "DD+ ";
      break;
    }
    case AV_CODEC_ID_DTS:
    {
      //! @todo: With ffmpeg >= 6.1 add new DTSX profile cases
      switch (profile)
      {
        case FF_PROFILE_DTS_96_24:
          strInfo = "DTS 96/24 ";
          break;
        case FF_PROFILE_DTS_ES:
          strInfo = "DTS ES ";
          break;
        case FF_PROFILE_DTS_EXPRESS:
          strInfo = "DTS EXPRESS ";
          break;
        case FF_PROFILE_DTS_HD_MA:
          strInfo = "DTS-HD MA ";
          break;
        case FF_PROFILE_DTS_HD_HRA:
          strInfo = "DTS-HD HRA ";
          break;
        default:
          strInfo = "DTS ";
          break;
      }
      break;
    }
    case AV_CODEC_ID_MP2:
      strInfo = "MP2 ";
      break;
    case AV_CODEC_ID_MP3:
      strInfo = "MP3 ";
      break;
    case AV_CODEC_ID_TRUEHD:
      strInfo = "TrueHD ";
      break;
    case AV_CODEC_ID_AAC:
    {
      switch (profile)
      {
        case FF_PROFILE_AAC_LOW:
        case FF_PROFILE_MPEG2_AAC_LOW:
          strInfo = "AAC-LC ";
          break;
        case FF_PROFILE_AAC_HE:
        case FF_PROFILE_MPEG2_AAC_HE:
          strInfo = "HE-AAC ";
          break;
        case FF_PROFILE_AAC_HE_V2:
          strInfo = "HE-AACv2 ";
          break;
        case FF_PROFILE_AAC_SSR:
          strInfo = "AAC-SSR ";
          break;
        case FF_PROFILE_AAC_LTP:
          strInfo = "AAC-LTP ";
          break;
        default:
        {
          // Try check by codec full string according to RFC 6381
          if (codecName == "mp4a.40.2" || codecName == "mp4a.40.17")
            strInfo = "AAC-LC ";
          else if (codecName == "mp4a.40.3")
            strInfo = "AAC-SSR ";
          else if (codecName == "mp4a.40.4" || codecName == "mp4a.40.19")
            strInfo = "AAC-LTP ";
          else if (codecName == "mp4a.40.5")
            strInfo = "HE-AAC ";
          else if (codecName == "mp4a.40.29")
            strInfo = "HE-AACv2 ";
          else
            strInfo = "AAC ";
          break;
        }
      }
      break;
    }
    case AV_CODEC_ID_ALAC:
      strInfo = "ALAC ";
      break;
    case AV_CODEC_ID_FLAC:
      strInfo = "FLAC ";
      break;
    case AV_CODEC_ID_OPUS:
      strInfo = "Opus ";
      break;
    case AV_CODEC_ID_VORBIS:
      strInfo = "Vorbis ";
      break;
    case AV_CODEC_ID_PCM_BLURAY:
    case AV_CODEC_ID_PCM_DVD:
      strInfo = "PCM ";
      break;
    default:
      strInfo = "";
      break;
  }

  strInfo += m_channelLayoutName;

  return strInfo;
}

int CDVDDemux::GetNrOfStreams(StreamType streamType)
{
  int iCounter = 0;

  for (auto pStream : GetStreams())
  {
    if (pStream && pStream->type == streamType)
      iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfSubtitleStreams()
{
  return GetNrOfStreams(STREAM_SUBTITLE);
}

std::string CDemuxStream::GetStreamName()
{
  return name;
}
