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
  virtual ~CGUIInfoProvider() = default;

  void UpdateAVInfo(const AudioStreamInfo& audioInfo, const VideoStreamInfo& videoInfo) override
  { m_audioInfo = audioInfo, m_videoInfo = videoInfo; }

protected:
  VideoStreamInfo m_videoInfo;
  AudioStreamInfo m_audioInfo;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
