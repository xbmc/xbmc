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

  CDVDOverlayLibass(const CDVDOverlayLibass& src) : CDVDOverlay(src), m_libass(src.m_libass) {}

  ~CDVDOverlayLibass() override = default;

  /*!
  \brief Getter for Libass handler
  \return The Libass handler.
  */
  std::shared_ptr<CDVDSubtitlesLibass> GetLibassHandler() const { return m_libass; }

private:
  std::shared_ptr<CDVDSubtitlesLibass> m_libass;
};
