/*
 *  Copyright (C) 2017-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderBufferPoolDMA.h"

namespace KODI
{
namespace RETRO
{
class CRenderBufferPoolDMAOpenGLES : public CRenderBufferPoolDMA
{
public:
  CRenderBufferPoolDMAOpenGLES() = default;
  ~CRenderBufferPoolDMAOpenGLES() override = default;

protected:
  // Implementation of CBaseRenderBufferPool
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
};
} // namespace RETRO
} // namespace KODI
