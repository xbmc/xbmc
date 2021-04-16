/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMCrtc.h"

#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <string>

using namespace KODI::WINDOWING::GBM;

CDRMCrtc::CDRMCrtc(int fd, uint32_t crtc) : CDRMObject(fd), m_crtc(drmModeGetCrtc(m_fd, crtc))
{
  if (!m_crtc)
    throw std::runtime_error("drmModeGetCrtc failed: " + std::string{strerror(errno)});

  if (!GetProperties(m_crtc->crtc_id, DRM_MODE_OBJECT_CRTC))
    throw std::runtime_error("failed to get properties for crtc: " +
                             std::to_string(m_crtc->crtc_id));
}
