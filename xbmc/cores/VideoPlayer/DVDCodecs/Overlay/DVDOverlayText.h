/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlayLibass.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitlesLibass.h"

#include <memory>

class CDVDOverlayText : public CDVDOverlayLibass
{
public:
  explicit CDVDOverlayText(const std::shared_ptr<CDVDSubtitlesLibass>& libass)
    : CDVDOverlayLibass(libass, DVDOVERLAY_TYPE_TEXT)
  {
    replace = true;
  }

  ~CDVDOverlayText() override = default;

  std::shared_ptr<CDVDOverlay> Clone() override { return std::make_shared<CDVDOverlayText>(*this); }

  void SetTextAlignEnabled(bool enable) override { m_enableTextAlign = enable; }
};
