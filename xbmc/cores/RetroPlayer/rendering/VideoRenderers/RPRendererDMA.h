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
class CRendererFactoryDMA : public IRendererFactory
{
public:
  ~CRendererFactoryDMA() override = default;

  // implementation of IRendererFactory
  std::string RenderSystemName() const override;
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
  };

  /**
   * @brief Special CRPBaseRenderer implementation to handle Direct Memory
   *        Access (DMA) buffer types. For specific use with
   *        CRenderBufferPoolDMA and CRenderBufferDMA. A windowing system
   *        must register use of this renderer and register at least one
   *        CBufferObject types.
   *
   */
  class CRPRendererDMA : public CRPRendererOpenGLES
  {
  public:
    CRPRendererDMA(const CRenderSettings& renderSettings,
                   CRenderContext& context,
                   std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererDMA() override = default;

  protected:
    // implementation of CRPRendererOpenGLES
    void Render(uint8_t alpha) override;
  };
}
}
