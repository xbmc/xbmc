/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPRendererOpenGLES.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
  class CRendererFactoryGBM : public IRendererFactory
  {
  public:
    // implementation of IRendererFactory
    std::string RenderSystemName() const override;
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools(CRenderContext &context) override;
  };

  class CRPRendererGBM : public CRPRendererOpenGLES
  {
  public:
    CRPRendererGBM(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererGBM() override;

  protected:
    // implementation of CRPRendererOpenGLES
    void Render(uint8_t alpha) override;
  };
}
}
