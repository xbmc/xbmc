/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoStreamSelectHelper.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LangCodeExpander.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace
{
constexpr int STREAM_ID_DISABLE = -2; // Stream id referred to item that disable the stream
constexpr int STREAM_ID_NONE = -1; // Stream id referred to item for "none" stream

// \brief Make a FileItem entry with "Disable" label, allow to disable a stream
const std::shared_ptr<CFileItem> MakeFileItemDisable(bool isSelected)
{
  const auto fileItem{std::make_shared<CFileItem>(g_localizeStrings.Get(24021))};
  fileItem->Select(isSelected);
  fileItem->SetProperty("stream.id", STREAM_ID_DISABLE);
  return fileItem;
}

// \brief Make a FileItem entry with "None" label, signal an empty list
const std::shared_ptr<CFileItem> MakeFileItemNone()
{
  const auto fileItem{std::make_shared<CFileItem>(g_localizeStrings.Get(231))};
  fileItem->SetProperty("stream.id", STREAM_ID_NONE);
  return fileItem;
}

std::shared_ptr<const CFileItem> OpenSelectDialog(CGUIDialogSelect& dialog,
                                                  int headingId,
                                                  const CFileItemList& itemsToDisplay)
{
  dialog.Reset();
  dialog.SetHeading(headingId);
  dialog.SetUseDetails(true);
  dialog.SetMultiSelection(false);
  dialog.SetItems(itemsToDisplay);

  dialog.Open();

  if (dialog.IsConfirmed())
    return dialog.GetSelectedFileItem();

  return {};
}

std::string ConvertFpsToString(float value)
{
  if (value == 0.0f)
    return "";

  std::string str{StringUtils::Format("{:.3f}", value)};
  // Keep numbers after the comma only if they are not 0
  const size_t zeroPos = str.find_last_not_of("0");
  if (zeroPos != std::string::npos)
    str.erase(zeroPos + 1);

  if (str.back() == '.')
    str.pop_back();

  return str;
}

struct VideoStreamInfoExt : VideoStreamInfo
{
  VideoStreamInfoExt(int id, const VideoStreamInfo& info) : VideoStreamInfo(info)
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
  AudioStreamInfoExt(int id, const AudioStreamInfo& info) : AudioStreamInfo(info)
  {
    streamId = id;

    if (!g_LangCodeExpander.Lookup(info.language, languageDesc))
      languageDesc = g_localizeStrings.Get(13205); // Unknown

    isDefault = info.flags & StreamFlags::FLAG_DEFAULT;
    isForced = info.flags & StreamFlags::FLAG_FORCED;
    isHearingImpaired = info.flags & StreamFlags::FLAG_HEARING_IMPAIRED;
    isVisualImpaired = info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED;
    isOriginal = info.flags & StreamFlags::FLAG_ORIGINAL;
  }

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
  SubtitleStreamInfoExt(int id, const SubtitleStreamInfo& info) : SubtitleStreamInfo(info)
  {
    streamId = id;

    if (!g_LangCodeExpander.Lookup(info.language, languageDesc))
      languageDesc = g_localizeStrings.Get(13205); // Unknown

    isDefault = info.flags & StreamFlags::FLAG_DEFAULT;
    isForced = info.flags & StreamFlags::FLAG_FORCED;
    isHearingImpaired = info.flags & StreamFlags::FLAG_HEARING_IMPAIRED;
    isVisualImpaired = info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED;
    isOriginal = info.flags & StreamFlags::FLAG_ORIGINAL;
  }

  int streamId{0};
  std::string languageDesc;
  bool isDefault{false};
  bool isForced{false};
  bool isHearingImpaired{false};
  bool isVisualImpaired{false};
  bool isOriginal{false};
};

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

bool SupportsAudioFeature(IPlayerAudioCaps feature, const std::vector<IPlayerAudioCaps>& caps)
{
  for (IPlayerAudioCaps cap : caps)
  {
    if (cap == feature || cap == IPlayerAudioCaps::ALL)
      return true;
  }

  return false;
}

bool SupportsSubtitleFeature(IPlayerSubtitleCaps feature,
                             const std::vector<IPlayerSubtitleCaps>& caps)
{
  for (IPlayerSubtitleCaps cap : caps)
  {
    if (cap == feature || cap == IPlayerSubtitleCaps::ALL)
      return true;
  }
  return false;
}

} // unnamed namespace

void KODI::VIDEO::GUILIB::OpenDialogSelectVideoStream()
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_VIDEO_STREAM)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_VIDEO_STREAM dialog instance");
    return;
  }

  auto& components = CServiceBroker::GetAppComponents();
  auto appPlayer = components.GetComponent<CApplicationPlayer>();
  const int streamCount = appPlayer->GetVideoStreamCount();
  const int selectedId = appPlayer->GetVideoStream();

  std::vector<VideoStreamInfoExt> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    VideoStreamInfo info;
    appPlayer->GetVideoStreamInfo(i, info);
    streams.emplace_back(i, info);
  }

  // Sort streams
  std::sort(streams.begin(), streams.end(), SortComparerStreamVideo());

  // Convert streams to FileItem's
  CFileItemList itemsToDisplay;
  itemsToDisplay.Reserve(streams.size());

  for (const VideoStreamInfoExt& info : streams)
  {
    const auto fileItem = std::make_shared<CFileItem>(info.name);
    fileItem->SetProperty("stream.id", info.streamId);
    fileItem->SetProperty("stream.description", info.name);
    fileItem->SetProperty("stream.codec", info.codecName);

    std::string languageDesc;
    g_LangCodeExpander.Lookup(info.language, languageDesc);
    fileItem->SetProperty("stream.language", languageDesc);

    fileItem->SetProperty("stream.resolution",
                          std::to_string(info.width) + "x" + std::to_string(info.height));
    fileItem->SetProperty("stream.bitrate",
                          static_cast<int>(std::lrint(static_cast<double>(info.bitrate) / 1000.0)));
    fileItem->SetProperty("stream.fps", ConvertFpsToString(info.fps));

    fileItem->SetProperty("stream.is3d", !info.stereoMode.empty() && info.stereoMode != "mono");
    fileItem->SetProperty("stream.stereomode", info.stereoMode);
    fileItem->SetProperty("stream.hdrtype", CStreamDetails::HdrTypeToString(info.hdrType));

    fileItem->SetProperty("stream.isdefault", info.isDefault);
    fileItem->SetProperty("stream.isforced", info.isForced);
    fileItem->SetProperty("stream.ishearingimpaired", info.isHearingImpaired);
    fileItem->SetProperty("stream.isvisualimpaired", info.isVisualImpaired);
    if (selectedId == info.streamId)
      fileItem->Select(true);

    itemsToDisplay.Add(fileItem);
  }

  if (itemsToDisplay.IsEmpty())
    itemsToDisplay.Add(MakeFileItemNone());

  const auto selectedItem = OpenSelectDialog(*dialog, 38031, itemsToDisplay);
  if (selectedItem)
  {
    const int id = selectedItem->GetProperty("stream.id").asInteger32(STREAM_ID_NONE);

    if (id != STREAM_ID_NONE)
      appPlayer->SetVideoStream(id);
  }
}

void KODI::VIDEO::GUILIB::OpenDialogSelectAudioStream()
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_AUDIO_STREAM)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_AUDIO_STREAM dialog instance");
    return;
  }

  auto& components = CServiceBroker::GetAppComponents();
  auto appPlayer = components.GetComponent<CApplicationPlayer>();

  std::vector<IPlayerAudioCaps> caps;
  appPlayer->GetAudioCapabilities(caps);
  if (!SupportsAudioFeature(IPlayerAudioCaps::SELECT_STREAM, caps))
    return;

  const int streamCount = appPlayer->GetAudioStreamCount();
  const int selectedId = appPlayer->GetAudioStream();

  std::vector<AudioStreamInfoExt> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    AudioStreamInfo info;
    appPlayer->GetAudioStreamInfo(i, info);
    streams.emplace_back(i, info);
  }

  // Sort streams
  std::sort(streams.begin(), streams.end(), SortComparerStreamAudio());

  // Convert streams to FileItem's
  CFileItemList itemsToDisplay;
  itemsToDisplay.Reserve(streams.size());

  for (const AudioStreamInfoExt& info : streams)
  {
    CFileItemPtr fileItem = std::make_shared<CFileItem>(info.languageDesc);
    fileItem->SetProperty("stream.id", info.streamId);
    fileItem->SetProperty("stream.description", info.name);
    fileItem->SetProperty("stream.codec", info.codecName);
    fileItem->SetProperty("stream.codecdesc", info.codecDesc);
    fileItem->SetProperty("stream.channels", info.channels);

    fileItem->SetProperty("stream.isdefault", info.isDefault);
    fileItem->SetProperty("stream.isforced", info.isForced);
    fileItem->SetProperty("stream.ishearingimpaired", info.isHearingImpaired);
    fileItem->SetProperty("stream.isvisualimpaired", info.isVisualImpaired);
    fileItem->SetProperty("stream.isoriginal", info.isOriginal);
    if (selectedId == info.streamId)
      fileItem->Select(true);

    itemsToDisplay.Add(fileItem);
  }

  if (itemsToDisplay.IsEmpty())
    itemsToDisplay.Add(MakeFileItemNone());

  const auto selectedItem = OpenSelectDialog(*dialog, 460, itemsToDisplay);
  if (selectedItem)
  {
    const int id = selectedItem->GetProperty("stream.id").asInteger32(STREAM_ID_NONE);

    if (id != STREAM_ID_NONE)
      appPlayer->SetAudioStream(id);
  }
}

void KODI::VIDEO::GUILIB::OpenDialogSelectSubtitleStream()
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_SUBTITLE_STREAM)};
  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_SUBTITLE_STREAM dialog instance");
    return;
  }

  auto& components = CServiceBroker::GetAppComponents();
  auto appPlayer = components.GetComponent<CApplicationPlayer>();

  std::vector<IPlayerSubtitleCaps> caps;
  appPlayer->GetSubtitleCapabilities(caps);
  if (!SupportsSubtitleFeature(IPlayerSubtitleCaps::SELECT_STREAM, caps))
    return;

  const int streamCount = appPlayer->GetSubtitleCount();
  const int selectedId = appPlayer->GetSubtitle();
  const bool isSubtitleEnabled = appPlayer->GetSubtitleVisible();

  std::vector<SubtitleStreamInfoExt> streams;
  streams.reserve(streamCount);

  // Collect all streams
  for (int i = 0; i < streamCount; ++i)
  {
    SubtitleStreamInfo info;
    appPlayer->GetSubtitleStreamInfo(i, info);
    streams.emplace_back(i, info);
  }

  // Sort streams
  std::sort(streams.begin(), streams.end(), SortComparerStreamSubtitle());

  // Convert streams to FileItem's
  CFileItemList itemsToDisplay;
  itemsToDisplay.Reserve(streams.size() + 1);

  for (const SubtitleStreamInfoExt& info : streams)
  {
    CFileItemPtr fileItem = std::make_shared<CFileItem>(info.languageDesc);
    fileItem->SetProperty("stream.id", info.streamId);
    fileItem->SetProperty("stream.description", info.name);
    fileItem->SetProperty("stream.codec", info.codecName);

    fileItem->SetProperty("stream.isdefault", info.isDefault);
    fileItem->SetProperty("stream.isforced", info.isForced);
    fileItem->SetProperty("stream.isoriginal", info.isOriginal);
    fileItem->SetProperty("stream.ishearingimpaired", info.isHearingImpaired);
    fileItem->SetProperty("stream.isvisualimpaired", info.isVisualImpaired);
    fileItem->SetProperty("stream.isexternal", info.isExternal);
    if (selectedId == info.streamId && isSubtitleEnabled)
      fileItem->Select(true);

    itemsToDisplay.Add(fileItem);
  }

  if (itemsToDisplay.IsEmpty())
    itemsToDisplay.Add(MakeFileItemNone());
  else
    itemsToDisplay.AddFront(MakeFileItemDisable(!isSubtitleEnabled), 0);

  const auto selectedItem = OpenSelectDialog(*dialog, 462, itemsToDisplay);
  if (selectedItem)
  {
    const int id = selectedItem->GetProperty("stream.id").asInteger32(STREAM_ID_NONE);

    if (id == STREAM_ID_DISABLE)
    {
      appPlayer->SetSubtitleVisible(false);
    }
    else if (id != STREAM_ID_NONE)
    {
      appPlayer->SetSubtitle(id);

      if (!appPlayer->GetSubtitleVisible())
        appPlayer->SetSubtitleVisible(true);
    }
  }
}
