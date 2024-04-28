/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DebugInfo.h"
#include "OverlayRenderer.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoPlayer/DVDSubtitles/SubtitlesAdapter.h"

#include <atomic>
#include <memory>
#include <string>

class CDebugRenderer
{
public:
  CDebugRenderer();
  virtual ~CDebugRenderer();
  void Initialize();
  void Dispose();
  void SetInfo(DEBUG_INFO_PLAYER& info);
  void SetInfo(DEBUG_INFO_VIDEO& video, DEBUG_INFO_RENDER& render);
  void Render(CRect& src, CRect& dst, CRect& view);
  void Flush();

protected:
  class CRenderer : public OVERLAY::CRenderer
  {
  public:
    CRenderer();
    void Render(int idx, float depth = 1.0f) override;
    void CreateSubtitlesStyle();

  private:
    // Implementation of Observer
    void Notify(const Observable& obs, const ObservableMessage msg) override{};

    std::shared_ptr<struct KODI::SUBTITLES::STYLE::style> m_debugOverlayStyle;
  };

  CRenderer m_overlayRenderer;

private:
  CSubtitlesAdapter* m_adapter{nullptr};
  std::atomic_bool m_isInitialized{false};
  std::shared_ptr<CDVDOverlay> m_overlay;
};
