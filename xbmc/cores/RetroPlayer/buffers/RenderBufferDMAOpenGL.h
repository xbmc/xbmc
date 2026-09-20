/*
 *  Copyright (C) 2017-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderBufferDMA.h"

namespace KODI
{
namespace RETRO
{
class CRenderBufferDMAOpenGL : public CRenderBufferDMA
{
public:
  explicit CRenderBufferDMAOpenGL(int fourcc);
  ~CRenderBufferDMAOpenGL() override = default;

protected:
  // Implementation of CRenderBufferDMA
  bool UploadFromMemory() override;
  void ConfigureTexture() override;
};
} // namespace RETRO
} // namespace KODI
