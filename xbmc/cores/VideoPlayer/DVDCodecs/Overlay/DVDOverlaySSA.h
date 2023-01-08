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

class CDVDOverlaySSA : public CDVDOverlayLibass
{
public:
  explicit CDVDOverlaySSA(const std::shared_ptr<CDVDSubtitlesLibass>& libass)
    : CDVDOverlayLibass(libass, DVDOVERLAY_TYPE_SSA)
  {
    replace = true;
  }

  ~CDVDOverlaySSA() override = default;

  std::shared_ptr<CDVDOverlay> Clone() override { return std::make_shared<CDVDOverlaySSA>(*this); }
};
