/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPRendererOpenGL.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
class CRendererFactoryDMAOpenGL : public IRendererFactory
{
public:
  ~CRendererFactoryDMAOpenGL() override = default;

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
 */
class CRPRendererDMAOpenGL : public CRPRendererOpenGL
{
public:
  CRPRendererDMAOpenGL(const CRenderSettings& renderSettings,
                       CRenderContext& context,
                       std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPRendererDMAOpenGL() override = default;

protected:
  // Implementation of CRPRendererOpenGL
  void Render(uint8_t alpha) override;
};
} // namespace RETRO
} // namespace KODI
