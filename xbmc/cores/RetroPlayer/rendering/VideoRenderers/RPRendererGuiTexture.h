/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/process/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/IPlayer.h"

namespace KODI
{
namespace RETRO
{
  class CRendererFactoryGuiTexture : public IRendererFactory
  {
  public:
    virtual ~CRendererFactoryGuiTexture() = default;

    // implementation of IRendererFactory
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools() override;
  };

  class CRenderBufferPoolGuiTexture : public CBaseRenderBufferPool
  {
  public:
    CRenderBufferPoolGuiTexture(ESCALINGMETHOD scalingMethod);
    ~CRenderBufferPoolGuiTexture() override = default;

    // implementation of IRenderBufferPool via CBaseRenderBufferPool
    bool IsCompatible(const CRenderVideoSettings &renderSettings) const override;

    // implementation of CBaseRenderBufferPool
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;

  private:
    ESCALINGMETHOD m_scalingMethod;
  };

  class CRPRendererGuiTexture : public CRPBaseRenderer
  {
  public:
    CRPRendererGuiTexture(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererGuiTexture() override = default;

    // public implementation of CRPBaseRenderer
    bool Supports(ERENDERFEATURE feature) const override;
    ESCALINGMETHOD GetDefaultScalingMethod() const override { return VS_SCALINGMETHOD_NEAREST; }

  protected:
    // protected implementation of CRPBaseRenderer
    void RenderInternal(bool clear, uint8_t alpha) override;
  };
}
}
