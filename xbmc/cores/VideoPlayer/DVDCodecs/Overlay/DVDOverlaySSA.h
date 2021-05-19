/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../DVDSubtitles/DVDSubtitlesLibass.h"
#include "DVDOverlay.h"

class CDVDOverlaySSA : public CDVDOverlay
{
public:
  explicit CDVDOverlaySSA(std::shared_ptr<CDVDSubtitlesLibass> libass)
    : CDVDOverlay(DVDOVERLAY_TYPE_SSA)
  {
    replace = true;
    m_libass = std::move(libass);
  }

  CDVDOverlaySSA(CDVDOverlaySSA& src)
    : CDVDOverlay(src)
    , m_libass(src.m_libass)
  {
  }

  ~CDVDOverlaySSA() override = default;

  CDVDOverlaySSA* Clone() override
  {
    return new CDVDOverlaySSA(*this);
  }

  /*!
   \brief Getter for the libass handler
   \return The libass handler.
   */
  std::shared_ptr<CDVDSubtitlesLibass> GetLibass() const { return m_libass; }

private:
  std::shared_ptr<CDVDSubtitlesLibass> m_libass;
};
