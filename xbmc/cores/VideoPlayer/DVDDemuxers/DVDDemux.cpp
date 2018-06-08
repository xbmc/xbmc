/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
