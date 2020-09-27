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

class CDRMEncoder : public CDRMObject
{
public:
  explicit CDRMEncoder(int fd, uint32_t encoder);
  CDRMEncoder(const CDRMEncoder&) = delete;
  CDRMEncoder& operator=(const CDRMEncoder&) = delete;
  ~CDRMEncoder() = default;

  uint32_t GetEncoderId() const { return m_encoder->encoder_id; }
  uint32_t GetCrtcId() const { return m_encoder->crtc_id; }
  uint32_t GetPossibleCrtcs() const { return m_encoder->possible_crtcs; }

private:
  struct DrmModeEncoderDeleter
  {
    void operator()(drmModeEncoder* p) { drmModeFreeEncoder(p); }
  };

  std::unique_ptr<drmModeEncoder, DrmModeEncoderDeleter> m_encoder;
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
