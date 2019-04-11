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

// HDR structs copied from HDR patchset, should be removed before merge
struct hdr_metadata_infoframe {
	uint8_t eotf;
	uint8_t metadata_type;
	struct {
		uint16_t x, y;
		} display_primaries[3];
	struct {
		uint16_t x, y;
		} white_point;
	uint16_t max_display_mastering_luminance;
	uint16_t min_display_mastering_luminance;
	uint16_t max_cll;
	uint16_t max_fall;
};
struct hdr_output_metadata {
	uint32_t metadata_type;
	union {
		struct hdr_metadata_infoframe hdmi_metadata_type1;
	};
};

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{
  class CDRMUtils;
}
}
}

class IVideoBufferDRMPRIME;

class CVideoLayerBridgeDRMPRIME
  : public KODI::WINDOWING::GBM::CVideoLayerBridge
{
public:
  CVideoLayerBridgeDRMPRIME(std::shared_ptr<KODI::WINDOWING::GBM::CDRMUtils> drm);
  ~CVideoLayerBridgeDRMPRIME();
  void Disable() override;

  virtual void Configure(IVideoBufferDRMPRIME* buffer);
  virtual void SetVideoPlane(IVideoBufferDRMPRIME* buffer, const CRect& destRect);
  virtual void UpdateVideoPlane();

protected:
  std::shared_ptr<KODI::WINDOWING::GBM::CDRMUtils> m_DRM;

private:
  void Acquire(IVideoBufferDRMPRIME* buffer);
  void Release(IVideoBufferDRMPRIME* buffer);
  bool Map(IVideoBufferDRMPRIME* buffer);
  void Unmap(IVideoBufferDRMPRIME* buffer);

  IVideoBufferDRMPRIME* m_buffer = nullptr;
  IVideoBufferDRMPRIME* m_prev_buffer = nullptr;

  uint32_t m_hdr_blob_id = 0;
  struct hdr_output_metadata m_hdr_metadata = {0};
};
