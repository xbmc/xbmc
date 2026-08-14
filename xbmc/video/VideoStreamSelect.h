/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"

#include <string>
#include <vector>

class CApplicationPlayer;

namespace KODI::VIDEO
{
struct VideoStreamInfoExt : VideoStreamInfo
{
  VideoStreamInfoExt(int id, const VideoStreamInfo& info);

  int streamId{0};
  std::string languageDesc;
  bool isDefault{false};
  bool isForced{false};
  bool isHearingImpaired{false};
  bool isVisualImpaired{false};
  float fps{0.0f};
};

struct AudioStreamInfoExt : AudioStreamInfo
{
  AudioStreamInfoExt(int id, const AudioStreamInfo& info);

  int streamId{0};
  std::string languageDesc;
  bool isDefault{false};
  bool isForced{false};
  bool isHearingImpaired{false};
  bool isVisualImpaired{false};
  bool isOriginal{false};
};

struct SubtitleStreamInfoExt : SubtitleStreamInfo
{
  SubtitleStreamInfoExt(int id, const SubtitleStreamInfo& info);

  int streamId{0};
  std::string languageDesc;
  bool isDefault{false};
  bool isForced{false};
  bool isHearingImpaired{false};
  bool isVisualImpaired{false};
  bool isOriginal{false};
};

class CVideoStreamSelect
{
private:
  CVideoStreamSelect() = delete;
  ~CVideoStreamSelect() = delete;

public:
  enum class TrackOrder
  {
    MEDIA = 0, // explicit IDs because persisted in settings.
    SORTED = 1,
  };

  static std::vector<VideoStreamInfoExt> GetVideoStreams(const CApplicationPlayer* appPlayer);
  static std::vector<AudioStreamInfoExt> GetAudioStreams(const CApplicationPlayer* appPlayer);
  static std::vector<SubtitleStreamInfoExt> GetSubtitleStreams(const CApplicationPlayer* appPlayer);

  static void OrderVideoStreams(std::vector<VideoStreamInfoExt>& streams, TrackOrder order);
  static void OrderAudioStreams(std::vector<AudioStreamInfoExt>& streams, TrackOrder order);
  static void OrderSubtitleStreams(std::vector<SubtitleStreamInfoExt>& streams, TrackOrder order);
};
} // namespace KODI::VIDEO
