/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BaseRenderBufferPool.h"
#include "IRenderBuffer.h"

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;
class CRenderVideoSettings;

class CRenderBufferPoolOpenGL : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolOpenGL() = default;
  ~CRenderBufferPoolOpenGL() override = default;

  // Implementation of IRenderBufferPool via CBaseRenderBufferPool
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;
  void ConfigureHardware(bool depth, bool stencil) override;

protected:
  // Implementation of CBaseRenderBufferPool
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
  bool ConfigureInternal() override;

private:
  // Configuration parameters
  GLuint m_pixelType = 0;
  GLuint m_internalFormat = 0;
  GLuint m_pixelFormat = 0;
  GLuint m_bpp = 0;

  // Hardware-rendering parameters (depth/stencil requested by the game core)
  bool m_hwDepth = false;
  bool m_hwStencil = false;
};
} // namespace RETRO
} // namespace KODI
