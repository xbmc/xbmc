/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/GameSettings.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

namespace KODI
{
namespace RETRO
{
  class CRendererFactoryGuiTexture : public IRendererFactory
  {
  public:
    ~CRendererFactoryGuiTexture() override = default;

    // implementation of IRendererFactory
    std::string RenderSystemName() const override;
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools(CRenderContext &context) override;
  };

  class CRenderBufferPoolGuiTexture : public CBaseRenderBufferPool
  {
  public:
    CRenderBufferPoolGuiTexture(SCALINGMETHOD scalingMethod);
    ~CRenderBufferPoolGuiTexture() override = default;

    // implementation of IRenderBufferPool via CBaseRenderBufferPool
    bool IsCompatible(const CRenderVideoSettings &renderSettings) const override;

    // implementation of CBaseRenderBufferPool
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;

  private:
    SCALINGMETHOD m_scalingMethod;
  };

  class CRPRendererGuiTexture : public CRPBaseRenderer
  {
  public:
    CRPRendererGuiTexture(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererGuiTexture() override = default;

    // public implementation of CRPBaseRenderer
    bool Supports(RENDERFEATURE feature) const override;
    SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

  protected:
    // protected implementation of CRPBaseRenderer
    void RenderInternal(bool clear, uint8_t alpha) override;
  };
}
}
