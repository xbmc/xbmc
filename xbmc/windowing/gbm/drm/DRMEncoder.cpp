/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMEncoder.h"

#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <string>

using namespace KODI::WINDOWING::GBM;

CDRMEncoder::CDRMEncoder(int fd, uint32_t encoder)
  : CDRMObject(fd), m_encoder(drmModeGetEncoder(m_fd, encoder))
{
  if (!m_encoder)
    throw std::runtime_error("drmModeGetEncoder failed: " + std::string{strerror(errno)});
}

std::vector<CDRMCrtc*> CDRMEncoder::GetPossibleCrtcs(
    const std::vector<std::unique_ptr<CDRMCrtc>>& crtcs, CDRMCrtc* preferred) const
{
  std::vector<CDRMCrtc*> possible;

  for (auto& crtc : crtcs)
  {
    // Check bitmask
    if (!(m_encoder->possible_crtcs & (1 << crtc->GetOffset())))
      continue;

    // Use raw pointer for the result
    if (preferred && preferred->GetId() == crtc->GetId())
      possible.insert(possible.begin(), crtc.get()); // Put preferred at front
    else
      possible.push_back(crtc.get());
  }

  return possible;
}