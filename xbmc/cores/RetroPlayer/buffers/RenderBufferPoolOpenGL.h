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

  // implementation of IRenderBufferPool via CBaseRenderBufferPool
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

protected:
  // implementation of CBaseRenderBufferPool
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
  bool ConfigureInternal() override;

private:
  // Configuration parameters
  GLuint m_pixeltype = 0;
  GLuint m_internalformat = 0;
  GLuint m_pixelformat = 0;
  GLuint m_bpp = 0;
};
} // namespace RETRO
} // namespace KODI
