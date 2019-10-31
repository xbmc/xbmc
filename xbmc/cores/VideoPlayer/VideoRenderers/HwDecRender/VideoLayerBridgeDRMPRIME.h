/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "windowing/gbm/VideoLayerBridge.h"

#include <memory>

#include <drm_mode.h>

#ifndef HAVE_HDR_OUTPUT_METADATA
// HDR structs is copied from linux include/linux/hdmi.h
struct hdr_metadata_infoframe
{
  uint8_t eotf;
  uint8_t metadata_type;
  struct
  {
    uint16_t x, y;
  } display_primaries[3];
  struct
  {
    uint16_t x, y;
  } white_point;
  uint16_t max_display_mastering_luminance;
  uint16_t min_display_mastering_luminance;
  uint16_t max_cll;
  uint16_t max_fall;
};
struct hdr_output_metadata
{
  uint32_t metadata_type;
  union {
    struct hdr_metadata_infoframe hdmi_metadata_type1;
  };
};
#endif

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{
class CDRMUtils;
}
} // namespace WINDOWING
} // namespace KODI

class CVideoBufferDRMPRIME;

class CVideoLayerBridgeDRMPRIME : public KODI::WINDOWING::GBM::CVideoLayerBridge
{
public:
  CVideoLayerBridgeDRMPRIME(std::shared_ptr<KODI::WINDOWING::GBM::CDRMUtils> drm);
  ~CVideoLayerBridgeDRMPRIME() override;
  void Disable() override;

  virtual void Configure(CVideoBufferDRMPRIME* buffer);
  virtual void SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect);
  virtual void UpdateVideoPlane();

protected:
  std::shared_ptr<KODI::WINDOWING::GBM::CDRMUtils> m_DRM;

private:
  void Acquire(CVideoBufferDRMPRIME* buffer);
  void Release(CVideoBufferDRMPRIME* buffer);
  bool Map(CVideoBufferDRMPRIME* buffer);
  void Unmap(CVideoBufferDRMPRIME* buffer);

  CVideoBufferDRMPRIME* m_buffer = nullptr;
  CVideoBufferDRMPRIME* m_prev_buffer = nullptr;

  uint32_t m_hdr_blob_id = 0;
  struct hdr_output_metadata m_hdr_metadata = {};
};
