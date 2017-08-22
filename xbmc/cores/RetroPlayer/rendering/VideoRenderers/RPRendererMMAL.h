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
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "threads/CriticalSection.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
  class CMMALProcess;
  class CMMALRenderer;
  class CRenderContext;
  class CRenderSettings;
  class IRenderBuffer;
  class IRenderBufferPool;

  class CRendererFactoryMMAL : public IRendererFactory
  {
  public:
    ~CRendererFactoryMMAL() override = default;

    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools() override;
  };

  class CRPRendererMMAL : public CRPBaseRenderer
  {
  public:
    CRPRendererMMAL(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererMMAL() override;

    // public implementation of CRPBaseRenderer
    bool Supports(ERENDERFEATURE feature) const override;
    ESCALINGMETHOD GetDefaultScalingMethod() const override;
    void Deinitialize() override;

    static bool SupportsScalingMethod(ESCALINGMETHOD method);

  protected:
    // protected implementation of CRPBaseRenderer
    bool ConfigureInternal() override;
    void RenderInternal(bool clear, uint8_t alpha) override;
    void FlushInternal() override;
    void ManageRenderArea() override;

  private:
    // MMAL properties
    std::unique_ptr<CMMALProcess> m_process;
    std::unique_ptr<CMMALRenderer> m_renderer;

    // Synchronization
    CCriticalSection m_mutex;
  };
}
}
