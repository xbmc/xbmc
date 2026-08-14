/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoStreamSelect.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/LangCodeExpander.h"

#include <algorithm>

namespace KODI::VIDEO
{
namespace
{
struct SortComparerStreamVideo
{
  bool operator()(const VideoStreamInfoExt& a, const VideoStreamInfoExt& b)
  {
    if (a.language != b.language)
    {
      return a.language < b.language;
    }
    if (a.codecName != b.codecName)
    {
      return a.codecName < b.codecName;
    }
    if (a.hdrType != b.hdrType)
    {
      return a.hdrType < b.hdrType;
    }
    if (a.fps != b.fps)
    {
      return a.fps < b.fps;
    }
    if (a.height != b.height)
    {
      return a.height < b.height;
    }
    if (a.width != b.width)
    {
      return a.width < b.width;
    }
    return a.bitrate < b.bitrate;
  }
};

struct SortComparerStreamAudio
{
  bool operator()(const AudioStreamInfoExt& a, const AudioStreamInfoExt& b)
  {
    if (a.languageDesc != b.languageDesc)
    {
      return a.languageDesc < b.languageDesc;
    }
    if (a.isOriginal != b.isOriginal)
    {
      return a.isOriginal < b.isOriginal;
    }
    if (a.isHearingImpaired != b.isHearingImpaired)
    {
      return a.isHearingImpaired < b.isHearingImpaired;
    }
    if (a.isVisualImpaired != b.isVisualImpaired)
    {
      return a.isVisualImpaired < b.isVisualImpaired;
    }
    if (a.isForced != b.isForced)
    {
      return a.isForced < b.isForced;
    }
    if (a.channels != b.channels)
    {
      return a.channels < b.channels;
    }
    if (a.bitrate != b.bitrate)
    {
      return a.bitrate < b.bitrate;
    }
    if (a.samplerate != b.samplerate)
    {
      return a.samplerate < b.samplerate;
    }
    return a.codecName < b.codecName;
  }
};

struct SortComparerStreamSubtitle
{
  bool operator()(const SubtitleStreamInfoExt& a, const SubtitleStreamInfoExt& b)
  {
    if (a.isExternal != b.isExternal)
    {
      return a.isExternal > b.isExternal;
    }
    if (a.languageDesc != b.languageDesc)
    {
      return a.languageDesc < b.languageDesc;
    }
    if (a.isOriginal != b.isOriginal)
    {
      return a.isOriginal < b.isOriginal;
    }
    if (a.isHearingImpaired != b.isHearingImpaired)
    {
      return a.isHearingImpaired < b.isHearingImpaired;
    }
    if (a.isVisualImpaired != b.isVisualImpaired)
    {
      return a.isVisualImpaired < b.isVisualImpaired;
    }
    if (a.isForced != b.isForced)
    {
      return a.isForced < b.isForced;
    }
    return a.codecName < b.codecName;
  }
};
} // namespace

VideoStreamInfoExt::VideoStreamInfoExt(int id, const VideoStreamInfo& info) : VideoStreamInfo(info)
{
  streamId = id;
  isDefault = info.flags & StreamFlags::FLAG_DEFAULT;
  isForced = info.flags & StreamFlags::FLAG_FORCED;
  isHearingImpaired = info.flags & StreamFlags::FLAG_HEARING_IMPAIRED;
  isVisualImpaired = info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED;

  fps = static_cast<float>(info.fpsRate);
  if (fps > 0.0f && info.fpsScale > 0)
    fps /= info.fpsScale;
}

AudioStreamInfoExt::AudioStreamInfoExt(int id, const AudioStreamInfo& info) : AudioStreamInfo(info)
{
  streamId = id;

  if (!g_LangCodeExpander.Lookup(info.language, languageDesc))
    languageDesc =
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13205); // Unknown

  isDefault = info.flags & StreamFlags::FLAG_DEFAULT;
  isForced = info.flags & StreamFlags::FLAG_FORCED;
  isHearingImpaired = info.flags & StreamFlags::FLAG_HEARING_IMPAIRED;
  isVisualImpaired = info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED;
  isOriginal = info.flags & StreamFlags::FLAG_ORIGINAL;
}

SubtitleStreamInfoExt::SubtitleStreamInfoExt(int id, const SubtitleStreamInfo& info)
  : SubtitleStreamInfo(info)
{
  streamId = id;

  if (!g_LangCodeExpander.Lookup(info.language, languageDesc))
    languageDesc =
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13205); // Unknown

  isDefault = info.flags & StreamFlags::FLAG_DEFAULT;
  isForced = info.flags & StreamFlags::FLAG_FORCED;
  isHearingImpaired = info.flags & StreamFlags::FLAG_HEARING_IMPAIRED;
  isVisualImpaired = info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED;
  isOriginal = info.flags & StreamFlags::FLAG_ORIGINAL;
}

std::vector<VideoStreamInfoExt> CVideoStreamSelect::GetVideoStreams()
{
  auto& components = CServiceBroker::GetAppComponents();
  auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const int streamCount = appPlayer->GetVideoStreamCount();

  std::vector<VideoStreamInfoExt> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    VideoStreamInfo info;
    appPlayer->GetVideoStreamInfo(i, info);
    streams.emplace_back(i, info);
  }

  std::sort(streams.begin(), streams.end(), SortComparerStreamVideo());

  return streams;
}

std::vector<AudioStreamInfoExt> CVideoStreamSelect::GetAudioStreams()
{
  auto& components = CServiceBroker::GetAppComponents();
  auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const int streamCount = appPlayer->GetAudioStreamCount();

  std::vector<AudioStreamInfoExt> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    AudioStreamInfo info;
    appPlayer->GetAudioStreamInfo(i, info);
    streams.emplace_back(i, info);
  }

  std::sort(streams.begin(), streams.end(), SortComparerStreamAudio());

  return streams;
}

std::vector<SubtitleStreamInfoExt> CVideoStreamSelect::GetSubtitleStreams()
{
  auto& components = CServiceBroker::GetAppComponents();
  auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const int streamCount = appPlayer->GetSubtitleCount();

  std::vector<SubtitleStreamInfoExt> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    SubtitleStreamInfo info;
    appPlayer->GetSubtitleStreamInfo(i, info);
    streams.emplace_back(i, info);
  }

  std::sort(streams.begin(), streams.end(), SortComparerStreamSubtitle());

  return streams;
}
} // namespace KODI::VIDEO
