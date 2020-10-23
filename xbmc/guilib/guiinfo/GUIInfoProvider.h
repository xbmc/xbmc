/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "guilib/guiinfo/IGUIInfoProvider.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CGUIInfoProvider : public IGUIInfoProvider
{
public:
  CGUIInfoProvider() = default;
  ~CGUIInfoProvider() override = default;

  bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const CGUIInfo& info,
                        std::string* fallback) override
  {
    return false;
  }

  void UpdateAVInfo(const AudioStreamInfo& audioInfo, const VideoStreamInfo& videoInfo, const SubtitleStreamInfo& subtitleInfo) override
  { m_audioInfo = audioInfo, m_videoInfo = videoInfo, m_subtitleInfo = subtitleInfo; }

protected:
  VideoStreamInfo m_videoInfo;
  AudioStreamInfo m_audioInfo;
  SubtitleStreamInfo m_subtitleInfo;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
