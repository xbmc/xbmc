/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemux.h"

std::string CDemuxStreamAudio::GetStreamType()
{
  std::string strInfo;
  switch (codec)
  {
  case AV_CODEC_ID_AC3:
    strInfo = "AC3 ";
    break;
  case AV_CODEC_ID_EAC3:
    strInfo = "DD+ ";
    break;
  case AV_CODEC_ID_DTS:
  {
    switch (profile)
    {
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
    strInfo = "AAC ";
    break;
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
    if (pStream && pStream->type == streamType) iCounter++;
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
