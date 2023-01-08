/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlayCodec.h"
#include "DVDStreamInfo.h"
#include "DVDSubtitles/SubtitlesAdapter.h"
#include "cores/VideoPlayer/DVDSubtitles/webvtt/WebVTTISOHandler.h"

#include <vector>

class CDVDOverlay;

class COverlayCodecWebVTT : public CDVDOverlayCodec, private CSubtitlesAdapter
{
public:
  COverlayCodecWebVTT();
  ~COverlayCodecWebVTT() override = default;
  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  OverlayMessage Decode(DemuxPacket* pPacket) override;
  void Reset() override;
  void Flush() override;
  std::shared_ptr<CDVDOverlay> GetOverlay() override;

private:
  std::shared_ptr<CDVDOverlay> m_pOverlay;
  CWebVTTISOHandler m_webvttHandler;
  bool m_isISOFormat{false};
  bool m_allowFlush{true};
  std::vector<int> m_previousSubIds;
};
