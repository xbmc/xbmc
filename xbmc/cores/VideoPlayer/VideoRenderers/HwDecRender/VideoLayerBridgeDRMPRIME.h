/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/DmaBufIdentityCache.h"
#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "utils/Geometry.h"
#include "windowing/gbm/VideoLayerBridge.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <drm_mode.h>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{
class CDRMAtomic;
class CDRMPlane;
}
} // namespace WINDOWING
} // namespace KODI

class CVideoBufferDRMPRIME;

class CVideoLayerBridgeDRMPRIME : public KODI::WINDOWING::GBM::CVideoLayerBridge
{
public:
  CVideoLayerBridgeDRMPRIME(std::shared_ptr<KODI::WINDOWING::GBM::CDRMAtomic> drm);
  ~CVideoLayerBridgeDRMPRIME() override;
  void Disable() override;

  virtual void Configure(CVideoBufferDRMPRIME* buffer);
  virtual void SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect);
  virtual void UpdateVideoPlane();

  //! \brief Whether the video plane refused to show pictures of this size
  bool Refuses(uint32_t width, uint32_t height) const;

protected:
  std::shared_ptr<KODI::WINDOWING::GBM::CDRMAtomic> m_DRM;

private:
  void Acquire(CVideoBufferDRMPRIME* buffer, uint32_t fbId);
  void Release(CVideoBufferDRMPRIME* buffer);
  bool PrepareBuffer(CVideoBufferDRMPRIME* buffer);
  //! \brief Convert the buffer's descriptor to a framebuffer; 0 on failure.
  uint32_t CreateFramebuffer(CVideoBufferDRMPRIME* buffer);

  static constexpr size_t MAX_FB_CACHE = 32;

  DRMPRIME::CDmaBufIdentityCache m_fbCache{MAX_FB_CACHE, "bridge-fb"};
  CVideoBufferDRMPRIME* m_buffer = nullptr;
  CVideoBufferDRMPRIME* m_prev_buffer = nullptr;
  uint32_t m_fb_id{0};
  uint32_t m_prev_fb_id{0};

  using Properties = std::vector<std::pair<const char*, uint64_t>>;

  void AddPlaneProperties(KODI::WINDOWING::GBM::CDRMPlane* plane);
  bool PlaneAccepts(KODI::WINDOWING::GBM::CDRMPlane* plane, const Properties& properties);

  CRect m_destRect;
  std::optional<std::pair<std::pair<uint32_t, uint32_t>, CRect>> m_tested;
  bool m_accepted{false};

  std::optional<std::pair<uint32_t, uint32_t>> m_refused;
};
