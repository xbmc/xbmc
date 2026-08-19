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
class CRenderContext;

class CRenderBufferPoolDMAOpenGLES : public CRenderBufferPoolDMA
{
public:
  explicit CRenderBufferPoolDMAOpenGLES(CRenderContext& context);
  ~CRenderBufferPoolDMAOpenGLES() override = default;

protected:
  // Implementation of CBaseRenderBufferPool
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;

private:
  CRenderContext& m_context;
};
} // namespace RETRO
} // namespace KODI
