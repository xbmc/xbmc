/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StreamParser.h"

#include "M2TSParser.h"
#include "PlaylistStructure.h"
#include "filesystem/DiscDirectoryHelper.h"

#include <map>
#include <span>
#include <vector>

#include <libbluray/bluray.h>

namespace XFILE
{
namespace
{
VideoStreamInfo PopulateVideoStreamInfo(const StreamInformation& stream,
                                        const TSVideoStreamInfo* bsvi)
{
  VideoStreamInfo vsi{};
  vsi.valid = true;

  if (bsvi)
  {
    vsi.height = static_cast<int>(bsvi->height);
    vsi.width = static_cast<int>(bsvi->width);

    if (bsvi->dolbyVision)
      vsi.hdrType = StreamHdrType::HDR_TYPE_DOLBYVISION;
    else if (bsvi->hdr10 || bsvi->hdr10Plus)
      vsi.hdrType = StreamHdrType::HDR_TYPE_HDR10;
    else
      vsi.hdrType = StreamHdrType::HDR_TYPE_NONE;

    switch (bsvi->streamType)
    {
      using enum ENCODING_TYPE;
      case VIDEO_MPEG2:
        vsi.codecName = "mpeg2";
        break;
      case VIDEO_VC1:
        vsi.codecName = "vc1";
        break;
      case VIDEO_H264:
      case VIDEO_H264_MVC:
        vsi.codecName = "h264";
        break;
      case VIDEO_HEVC:
        vsi.codecName = "hevc";
        break;
      default:
        break;
    }

    if (bsvi->is3d)
      vsi.stereoMode = "left_right"; // Only mode supported
  }
  else
  {
    switch (stream.format)
    {
      case BLURAY_VIDEO_FORMAT_480I:
      case BLURAY_VIDEO_FORMAT_480P:
        vsi.height = 480;
        vsi.width = 640; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_576I:
      case BLURAY_VIDEO_FORMAT_576P:
        vsi.height = 576;
        vsi.width = 720; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_720P:
        vsi.height = 720;
        vsi.width = 1280; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_1080I:
      case BLURAY_VIDEO_FORMAT_1080P:
        vsi.height = 1080;
        vsi.width = 1920; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_2160P:
        vsi.height = 2160;
        vsi.width = 3840; // Guess but never displayed
        break;
      default:
        vsi.height = 0;
        vsi.width = 0;
        break;
    }

    switch (stream.coding)
    {
      using enum ENCODING_TYPE;
      case VIDEO_MPEG2:
        vsi.codecName = "mpeg2";
        break;
      case VIDEO_VC1:
        vsi.codecName = "vc1";
        break;
      case VIDEO_H264:
        vsi.codecName = "h264";
        break;
      case VIDEO_HEVC:
        vsi.codecName = "hevc";
        break;
      default:
        vsi.codecName = "";
        break;
    }

    vsi.hdrType = StreamHdrType::HDR_TYPE_NONE; // Not stored in BLURAY_TITLE_INFO
  }

  switch (stream.aspect)
  {
    using enum ASPECT_RATIO;
    case RATIO_4_3:
      vsi.videoAspectRatio = 4.0f / 3.0f;
      break;
    case RATIO_16_9:
      vsi.videoAspectRatio = 16.0f / 9.0f;
      break;
    default:
      vsi.videoAspectRatio = 0.0f;
      break;
  }

  return vsi;
}

AudioStreamInfo PopulateAudioStreamInfo(const StreamInformation& stream,
                                        const TSAudioStreamInfo* bsai)
{
  AudioStreamInfo asi;
  asi.valid = true;

  if (bsai)
  {
    asi.channels = bsai->channels > 8
                       ? 8
                       : static_cast<int>(bsai->channels); // Limit to max 7.1 for display purposes

    switch (bsai->streamType)
    {
      using enum ENCODING_TYPE;
      case AUDIO_AC3:
        asi.codecName = "ac3";
        break;
      case AUDIO_AC3PLUS:
      case AUDIO_AC3PLUS_SECONDARY:
      {
        if (bsai->isAtmos)
          asi.codecName = "eac3_ddp_atmos";
        else
          asi.codecName = "eac3";
        break;
      }
      case AUDIO_LPCM:
        asi.codecName = "pcm_bluray";
        break;
      case AUDIO_DTS:
        asi.codecName = "dts";
        break;
      case AUDIO_DTSHD:
      case AUDIO_DTSHD_SECONDARY:
      {
        if (bsai->isXLL)
          asi.codecName = "dtshd_hra";
        else
          asi.codecName = "dca";
        break;
      }
      case AUDIO_DTSHD_MASTER:
      {
        if (bsai->isXLLXIMAX)
          asi.codecName = "dtshd_ma_x_imax";
        else if (bsai->isXLLX)
          asi.codecName = "dtshd_ma_x";
        else
          asi.codecName = "dtshd_ma";
        break;
      }
      case AUDIO_TRUHD:
      {
        if (bsai->isAtmos)
          asi.codecName = "truehd_atmos";
        else
          asi.codecName = "truehd";
        break;
      }
      default:
        asi.codecName = "";
        break;
    }
  }
  else
  {
    asi.channels = 0; // Only basic mono/stereo/multichannel is stored in BLURAY_TITLE_INFO

    switch (stream.coding)
    {
      using enum ENCODING_TYPE;
      case AUDIO_AC3:
        asi.codecName = "ac3";
        break;
      case AUDIO_AC3PLUS:
      case AUDIO_AC3PLUS_SECONDARY:
        asi.codecName = "eac3";
        break;
      case AUDIO_LPCM:
        asi.codecName = "pcm";
        break;
      case AUDIO_DTS:
        asi.codecName = "dts";
        break;
      case AUDIO_DTSHD:
      case AUDIO_DTSHD_SECONDARY:
        asi.codecName = "dtshd";
        break;
      case AUDIO_DTSHD_MASTER:
        asi.codecName = "dtshd_ma";
        break;
      case AUDIO_TRUHD:
        asi.codecName = "truehd";
        break;
      default:
        asi.codecName = "";
        break;
    }
  }

  asi.language = stream.language;

  return asi;
}
} // namespace

void CStreamParser::ConvertBlurayPlaylistInformation(const BlurayPlaylistInformation& b,
                                                     PlaylistInformation& p,
                                                     const StreamMap& s)
{
  // Parse BlurayPlaylistInformation (from MPLS) and stream information (from M2TS) into PlaylistInformation
  p.clear();
  p.playlist = b.playlist;
  p.duration = b.duration;
  p.chapters.reserve(b.chapters.size());
  for (const ChapterInformation& chapter : b.chapters)
    p.chapters.emplace_back(chapter.start);
  p.clips.reserve(b.clips.size());
  for (const ClipInformation& clip : b.clips)
  {
    p.clips.emplace_back(clip.clip);
    p.clipDuration[clip.clip] = clip.duration;
  }
  if (!b.clips.empty() && !b.clips[0].programs.empty())
  {
    for (const StreamInformation& stream : b.clips[0].programs[0].streams)
    {
      // Find stream in StreamMap to get accurate details
      const auto bs{s.find(stream.packetIdentifier)};
      switch (stream.coding)
      {
        using enum ENCODING_TYPE;
        case VIDEO_MPEG2:
        case VIDEO_VC1:
        case VIDEO_H264:
        case VIDEO_H264_MVC:
        case VIDEO_HEVC:
          p.videoStreams.emplace_back(PopulateVideoStreamInfo(
              stream,
              bs != s.end() ? dynamic_cast<TSVideoStreamInfo*>(bs->second.get()) : nullptr));
          break;
        case AUDIO_LPCM:
        case AUDIO_AC3:
        case AUDIO_DTS:
        case AUDIO_TRUHD:
        case AUDIO_AC3PLUS:
        case AUDIO_DTSHD:
        case AUDIO_DTSHD_MASTER:
        case AUDIO_AC3PLUS_SECONDARY:
        case AUDIO_DTSHD_SECONDARY:
          p.audioStreams.emplace_back(PopulateAudioStreamInfo(
              stream,
              bs != s.end() ? dynamic_cast<TSAudioStreamInfo*>(bs->second.get()) : nullptr));
          break;
        case SUB_PG:
        case SUB_TEXT:
        {
          SubtitleStreamInfo ssi;
          ssi.valid = true;
          ssi.language = stream.language;

          p.pgStreams.emplace_back(std::move(ssi));
          break;
        }
        case SUB_IG:
        default:
          break;
      }
    }
  }
}
} // namespace XFILE
