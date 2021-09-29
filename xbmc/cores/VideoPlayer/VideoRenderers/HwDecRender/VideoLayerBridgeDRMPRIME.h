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

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{
class CDRMAtomic;
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

protected:
  std::shared_ptr<KODI::WINDOWING::GBM::CDRMAtomic> m_DRM;

private:
  void Acquire(CVideoBufferDRMPRIME* buffer);
  void Release(CVideoBufferDRMPRIME* buffer);
  bool Map(CVideoBufferDRMPRIME* buffer);
  void Unmap(CVideoBufferDRMPRIME* buffer);

  CVideoBufferDRMPRIME* m_buffer = nullptr;
  CVideoBufferDRMPRIME* m_prev_buffer = nullptr;
};
