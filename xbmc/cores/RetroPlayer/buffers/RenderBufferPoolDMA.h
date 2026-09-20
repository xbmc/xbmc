/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRenderBuffer.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"

namespace KODI
{
namespace RETRO
{
/**
 * @brief Common DMA buffer-pool configuration that converts AVPixelFormat to
 *        DRM_FORMAT_* for use by API-specific CRenderBufferDMA subclasses.
 *
 */
class CRenderBufferPoolDMA : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolDMA() = default;
  ~CRenderBufferPoolDMA() override = default;

  // Implementation of IRenderBufferPool via CBaseRenderBufferPool
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

protected:
  // Implementation of CBaseRenderBufferPool
  bool ConfigureInternal() override;

  int GetFourcc() const { return m_fourcc; }

private:
  // Configuration parameters
  int m_fourcc = 0;
};
} // namespace RETRO
} // namespace KODI
