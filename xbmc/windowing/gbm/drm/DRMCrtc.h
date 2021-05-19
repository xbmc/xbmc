/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMObject.h"

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CDRMCrtc : public CDRMObject
{
public:
  explicit CDRMCrtc(int fd, uint32_t crtc);
  CDRMCrtc(const CDRMCrtc&) = delete;
  CDRMCrtc& operator=(const CDRMCrtc&) = delete;
  ~CDRMCrtc() = default;

  uint32_t GetCrtcId() const { return m_crtc->crtc_id; }
  uint32_t GetBufferId() const { return m_crtc->buffer_id; }
  uint32_t GetX() const { return m_crtc->x; }
  uint32_t GetY() const { return m_crtc->y; }
  drmModeModeInfoPtr GetMode() const { return &m_crtc->mode; }
  bool GetModeValid() const { return m_crtc->mode_valid != 0; }

private:
  struct DrmModeCrtcDeleter
  {
    void operator()(drmModeCrtc* p) { drmModeFreeCrtc(p); }
  };

  std::unique_ptr<drmModeCrtc, DrmModeCrtcDeleter> m_crtc;
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
