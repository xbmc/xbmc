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
#include "utils/LanguageTag.h"
#include "utils/log.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <vector>

#include <fmt/format.h>
#include <libbluray/bluray.h>

using namespace KODI::UTILS;

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
          asi.codecName = "dts";
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

  asi.language = CLanguageTag::Parse(stream.language);

  return asi;
}

// Add one elementary stream to the playlist, refined by the M2TS analysis in s where it has been
// done (s is empty when stream details were deferred).
void AddStream(const StreamInformation& stream,
               const StreamMap& s,
               unsigned int playlist,
               PlaylistInformation& p)
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
          stream, bs != s.end() ? dynamic_cast<TSVideoStreamInfo*>(bs->second.get()) : nullptr));
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
    {
      const auto* bsai{bs != s.end() ? dynamic_cast<TSAudioStreamInfo*>(bs->second.get())
                                     : nullptr};
      if (!bsai && !s.empty())
        CLog::LogF(LOGDEBUG,
                   "Playlist {} - no parsed stream information for audio PID 0x{} coding 0x{} "
                   "- {} - channel count will be unknown",
                   playlist, fmt::format("{:04x}", stream.packetIdentifier),
                   fmt::format("{:02x}", static_cast<int>(stream.coding)),
                   bs == s.end() ? "packet identifier not present in stream map"
                                 : "stream in map is not an audio stream");

      p.audioStreams.emplace_back(PopulateAudioStreamInfo(stream, bsai));
      break;
    }
    case SUB_PG:
    case SUB_TEXT:
    {
      SubtitleStreamInfo ssi;
      ssi.valid = true;
      ssi.language = CLanguageTag::Parse(stream.language);

      p.pgStreams.emplace_back(std::move(ssi));
      break;
    }
    case SUB_IG:
    default:
      break;
  }
}
} // namespace

void CStreamParser::ConvertBlurayPlaylistInformation(const BlurayPlaylistInformation& b,
                                                     PlaylistInformation& p,
                                                     const StreamMap& s,
                                                     StreamDetails streamDetails)
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

  if (streamDetails == StreamDetails::DEFER)
  {
    // Neither the .clpi nor the m2ts has been read, so describe the streams from the play item's
    // stream number table. That gives the coding and language of every stream the playlist
    // exposes, which is what telling playlists apart and listing their languages needs - only the
    // details the m2ts carries (channel counts, resolutions) are missing.
    if (const PlayItemInformation * playItem{GetLongestPlayItem(b)}; playItem)
    {
      for (const auto* streams : {&playItem->videoStreams, &playItem->audioStreams,
                                  &playItem->presentationGraphicStreams})
      {
        for (const StreamInformation& stream : *streams)
          AddStream(stream, s, b.playlist, p);
      }
    }
    return;
  }

  // Stream information must come from the same clip the M2TS analysis used (see
  // CM2TSParser::GetStreams), otherwise the packet identifiers will not correspond and no parsed
  // details will be found for some (or all) streams
  const ClipInformation* streamClip{nullptr};
  if (const ClipInformation * playItemClip{GetLongestPlayItemClip(b)}; playItemClip)
  {
    if (const auto it{std::ranges::find(b.clips, playItemClip->clip, &ClipInformation::clip)};
        it != b.clips.end())
      streamClip = &*it;
    else
      CLog::LogFC(LOGDEBUG, LOGBLURAY,
                  "Playlist {} - no clip information for clip {} - falling back to first clip",
                  b.playlist, playItemClip->clip);
  }
  if (!streamClip && !b.clips.empty())
    streamClip = &b.clips[0];

  if (streamClip && !streamClip->programs.empty())
  {
    for (const StreamInformation& stream : streamClip->programs[0].streams)
      AddStream(stream, s, b.playlist, p);
  }
}
} // namespace XFILE
