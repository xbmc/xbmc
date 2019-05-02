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
};
