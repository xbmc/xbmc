/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "utils/StringUtils.h"

void CDemuxStreamTeletext::GetStreamInfo(std::string& strInfo)
{
  strInfo = "Teletext Data Stream";
}

void CDemuxStreamRadioRDS::GetStreamInfo(std::string& strInfo)
{
  strInfo = "Radio Data Stream (RDS)";
}

void CDemuxStreamAudio::GetStreamType(std::string& strInfo)
{
  switch (codec)
  {
  case AV_CODEC_ID_AC3:
    strInfo = "AC3 ";
    break;
  case AV_CODEC_ID_DTS:
  {
    switch (profile)
    {
#ifdef FF_PROFILE_DTS_HD_MA
    case FF_PROFILE_DTS_HD_MA:
      strInfo = "DTS-HD MA ";
      break;
    case FF_PROFILE_DTS_HD_HRA:
      strInfo = "DTS-HD HRA ";
      break;
#endif
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
    strInfo = "Dolby TrueHD ";
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
    strInfo = "VORBIS ";
    break;
  case AV_CODEC_ID_PCM_BLURAY:
  case AV_CODEC_ID_PCM_DVD:
    strInfo = "PCM ";
    break;
  }

  // if the channel layout couldn't be determined, guess it
  if (m_channelLayout.empty())
  {
    switch (iChannels)
    {
    case 1:
      strInfo = "Mono";
      break;
    case 2:
      strInfo = "Stereo";
      break;
    case 6:
      strInfo = "5.1";
      break;
    case 7:
      strInfo = "6.1";
      break;
    case 8:
      strInfo = "7.1";
      break;
    default:
    {
      if (iChannels != 0)
      {
        strInfo += StringUtils::Format("%d-chs", iChannels);
      }
      break;
    }
    }
  }
  else
  {
    strInfo += m_channelLayout;
  }
}

int CDVDDemux::GetNrOfAudioStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_AUDIO) iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfVideoStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_VIDEO) iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfSubtitleStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_SUBTITLE) iCounter++;
  }

  return iCounter;
}

int CDVDDemux::GetNrOfTeletextStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_TELETEXT) iCounter++;
  }

  return iCounter;
}

const int CDVDDemux::GetNrOfRadioRDSStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_RADIO_RDS) iCounter++;
  }

  return iCounter;
}

CDemuxStreamAudio* CDVDDemux::GetStreamFromAudioId(int iAudioIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_AUDIO) counter++;
    if (iAudioIndex == counter)
      return (CDemuxStreamAudio*)pStream;
  }
  return NULL;
}

CDemuxStreamVideo* CDVDDemux::GetStreamFromVideoId(int iVideoIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_VIDEO) counter++;
    if (iVideoIndex == counter)
      return (CDemuxStreamVideo*)pStream;
  }
  return NULL;
}

CDemuxStreamSubtitle* CDVDDemux::GetStreamFromSubtitleId(int iSubtitleIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_SUBTITLE) counter++;
    if (iSubtitleIndex == counter)
      return (CDemuxStreamSubtitle*)pStream;
  }
  return NULL;
}

CDemuxStreamTeletext* CDVDDemux::GetStreamFromTeletextId(int iTeletextIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_TELETEXT) counter++;
    if (iTeletextIndex == counter)
      return (CDemuxStreamTeletext*)pStream;
  }
  return NULL;
}

const CDemuxStreamRadioRDS* CDVDDemux::GetStreamFromRadioRDSId(int iRadioRDSIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_RADIO_RDS) counter++;
    if (iRadioRDSIndex == counter)
      return (CDemuxStreamRadioRDS*)pStream;
  }
  return NULL;
}

void CDemuxStream::GetStreamName( std::string& strInfo )
{
  strInfo = "";
}

AVDiscard CDemuxStream::GetDiscard()
{
  return AVDISCARD_NONE;
}

void CDemuxStream::SetDiscard(AVDiscard discard)
{
  return;
}

