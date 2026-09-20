/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoStreamSelect.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/guiinfo/GUIInfoUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <functional>

namespace KODI::VIDEO
{
namespace
{
struct SortComparerStreamVideo
{
  bool operator()(const VideoStreamInfoExt& a, const VideoStreamInfoExt& b)
  {
    if (a.languageDesc != b.languageDesc)
    {
      return a.languageDesc < b.languageDesc;
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

  languageDesc = KODI::GUILIB::GUIINFO::CGUIInfoUtils::FormatLanguage(info.language);

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

  languageDesc = KODI::GUILIB::GUIINFO::CGUIInfoUtils::FormatLanguage(info.language);

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

  languageDesc = KODI::GUILIB::GUIINFO::CGUIInfoUtils::FormatLanguage(info.language);

  isDefault = info.flags & StreamFlags::FLAG_DEFAULT;
  isForced = info.flags & StreamFlags::FLAG_FORCED;
  isHearingImpaired = info.flags & StreamFlags::FLAG_HEARING_IMPAIRED;
  isVisualImpaired = info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED;
  isOriginal = info.flags & StreamFlags::FLAG_ORIGINAL;
}

namespace
{
// This attempts to distinguish videos files, with may have streams muxed in a meaningful order,
// from videos where stream order is more random and always benefits from reordering, for example
// internet streaming protocols.
bool IsTrackOrderMeaningful()
{
  //! @todo improve accuracy with detection in VideoPlayer

  const auto item = g_application.CurrentFileItemPtr();

  if (!item)
    return true;

  const std::string dynPath = item->GetDynPath();

  // Detect file-based all protocols discretly since the macro test "IsNetworkFilesystem" interprets
  // http as always file-based, which misidentifies masqueraded streaming
  if (URIUtils::IsHD(dynPath) || URIUtils::IsSmb(dynPath) || URIUtils::IsNfs(dynPath) ||
      URIUtils::IsFTP(dynPath) || URIUtils::IsProtocol(dynPath, "sftp") ||
      URIUtils::IsProtocol(dynPath, "ssh") || URIUtils::IsDAV(dynPath))
    return true;

  const std::string path = item->GetPath();
  if (URIUtils::IsUPnP(path))
    return true;

  return false;
}

CVideoStreamSelect::TrackOrder GetTrackOrder()
{
  if (!IsTrackOrderMeaningful())
    return CVideoStreamSelect::TrackOrder::SORTED;

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  return static_cast<CVideoStreamSelect::TrackOrder>(
      settings->GetInt(CSettings::SETTING_VIDEOPLAYER_FILESTREAMDISPLAYORDER));
}

// Concepts to safeguard the template
template<typename F, typename Player>
concept StreamCountGetter = requires(F f, Player* p) {
  { std::invoke(f, p) } -> std::convertible_to<int>;
};

template<typename F, typename Player, typename InfoT>
concept StreamInfoGetter = requires(F f, Player* p, int i, InfoT& info) {
  { std::invoke(f, p, i, info) } -> std::same_as<void>;
};

template<typename F, typename InfoExtT, typename Order>
concept StreamOrderer = requires(F f, std::vector<InfoExtT>& i, Order o) {
  { std::invoke(f, i, o) } -> std::same_as<void>;
};

template<typename StreamInfoExtT,
         typename StreamInfoT,
         typename GetCountFn,
         typename GetInfoFn,
         typename OrderFn>
  requires StreamCountGetter<GetCountFn, CApplicationPlayer> &&
           StreamInfoGetter<GetInfoFn, CApplicationPlayer, StreamInfoT> &&
           StreamOrderer<OrderFn, StreamInfoExtT, CVideoStreamSelect::TrackOrder>
std::vector<StreamInfoExtT> GetStreams(const CApplicationPlayer* appPlayer,
                                       GetCountFn getCount,
                                       GetInfoFn getInfo,
                                       OrderFn orderer)
{
  if (appPlayer == nullptr)
    return {};

  const int streamCount = std::invoke(getCount, appPlayer);
  std::vector<StreamInfoExtT> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    StreamInfoT info;
    std::invoke(getInfo, appPlayer, i, info);
    streams.emplace_back(i, info);
  }

  // Sort the streams
  std::invoke(orderer, streams, GetTrackOrder());

  return streams;
}
} // namespace

std::vector<VideoStreamInfoExt> CVideoStreamSelect::GetVideoStreams(
    const CApplicationPlayer* appPlayer)
{
  return GetStreams<VideoStreamInfoExt, VideoStreamInfo>(
      appPlayer, &CApplicationPlayer::GetVideoStreamCount, &CApplicationPlayer::GetVideoStreamInfo,
      OrderVideoStreams);
}

std::vector<AudioStreamInfoExt> CVideoStreamSelect::GetAudioStreams(
    const CApplicationPlayer* appPlayer)
{
  return GetStreams<AudioStreamInfoExt, AudioStreamInfo>(
      appPlayer, &CApplicationPlayer::GetAudioStreamCount, &CApplicationPlayer::GetAudioStreamInfo,
      OrderAudioStreams);
}

std::vector<SubtitleStreamInfoExt> CVideoStreamSelect::GetSubtitleStreams(
    const CApplicationPlayer* appPlayer)
{
  return GetStreams<SubtitleStreamInfoExt, SubtitleStreamInfo>(
      appPlayer, &CApplicationPlayer::GetSubtitleCount, &CApplicationPlayer::GetSubtitleStreamInfo,
      OrderSubtitleStreams);
}

void CVideoStreamSelect::OrderVideoStreams(std::vector<VideoStreamInfoExt>& streams,
                                           TrackOrder order)
{
  if (order == TrackOrder::SORTED)
    std::sort(streams.begin(), streams.end(), SortComparerStreamVideo());
}

void CVideoStreamSelect::OrderAudioStreams(std::vector<AudioStreamInfoExt>& streams,
                                           TrackOrder order)
{
  if (order == TrackOrder::SORTED)
    std::sort(streams.begin(), streams.end(), SortComparerStreamAudio());
}

void CVideoStreamSelect::OrderSubtitleStreams(std::vector<SubtitleStreamInfoExt>& streams,
                                              TrackOrder order)
{
  if (order == TrackOrder::SORTED)
    std::sort(streams.begin(), streams.end(), SortComparerStreamSubtitle());
}
} // namespace KODI::VIDEO
