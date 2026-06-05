/*
 *  Copyright (C) 2006-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlay.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitlesLibass.h"

#include <memory>

class CDVDOverlayLibass : public CDVDOverlay
{
public:
  explicit CDVDOverlayLibass(const std::shared_ptr<CDVDSubtitlesLibass>& libass,
                             DVDOverlayType type)
    : CDVDOverlay(type), m_libass(libass)
  {
  }

  CDVDOverlayLibass(const CDVDOverlayLibass& src)
    : CDVDOverlay(src),
      m_pendingChange(src.m_pendingChange),
      m_libass(src.m_libass)
  {
  }

  ~CDVDOverlayLibass() override = default;

  /*!
  \brief Getter for Libass handler
  \return The Libass handler.
  */
  std::shared_ptr<CDVDSubtitlesLibass> GetLibassHandler() const { return m_libass; }

  // libass change-since-last-walk-consumed flag. Set non-zero by
  // OVERLAY::CRenderer::PrepareOverlays when libass detect_change reports
  // non-zero; consumed and cleared by ConvertLibass when a fresh COverlay
  // is created. Lives on the persistent overlay so it survives the
  // per-frame SElement recycling driven by RenderManager AddOverlay/Release.
  int m_pendingChange{0};

private:
  std::shared_ptr<CDVDSubtitlesLibass> m_libass;
};
