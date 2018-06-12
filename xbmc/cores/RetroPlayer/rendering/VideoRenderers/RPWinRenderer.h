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
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <dxgi.h>
#include <memory>
#include <stdint.h>
#include <vector>

class CD3DTexture;
struct SwsContext;

namespace KODI
{
namespace RETRO
{
  class CRenderContext;
  class CRPWinOutputShader;

  class CWinRendererFactory : public IRendererFactory
  {
  public:
    virtual ~CWinRendererFactory() = default;

    // implementation of IRendererFactory
    std::string RenderSystemName() const override;
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools(CRenderContext &context) override;
  };

  class CWinRenderBuffer : public CRenderBufferSysMem
  {
  public:
    CWinRenderBuffer(AVPixelFormat pixFormat, DXGI_FORMAT dxFormat, unsigned int width, unsigned int height);
    ~CWinRenderBuffer() override = default;

    // implementation of IRenderBuffer via CRenderBufferSysMem
    bool UploadTexture() override;

    CD3DTexture *GetTarget() { return m_intermediateTarget.get(); }

  private:
    bool CreateTexture();
    bool GetTexture(uint8_t*& data, unsigned int& stride);
    bool ReleaseTexture();

    bool CreateScalingContext();
    void ScalePixels(uint8_t *source, unsigned int sourceStride, uint8_t *target, unsigned int targetStride);

    static AVPixelFormat GetPixFormat(DXGI_FORMAT dxFormat);

    // Construction parameters
    const AVPixelFormat m_pixFormat;
    const DXGI_FORMAT m_targetDxFormat;
    const unsigned int m_width;
    const unsigned int m_height;

    AVPixelFormat m_targetPixFormat;
    std::unique_ptr<CD3DTexture> m_intermediateTarget;

    SwsContext *m_swsContext = nullptr;
  };

  class CWinRenderBufferPool : public CBaseRenderBufferPool
  {
  public:
    CWinRenderBufferPool();
    ~CWinRenderBufferPool() override = default;

    // implementation of IRenderBufferPool via CRenderBufferPoolSysMem
    bool IsCompatible(const CRenderVideoSettings &renderSettings) const override;

    // implementation of CBaseRenderBufferPool via CRenderBufferPoolSysMem
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;

    // DirectX interface
    bool ConfigureDX(DXGI_FORMAT dxFormat);
    CRPWinOutputShader *GetShader(ESCALINGMETHOD scalingMethod) const;

  private:
    static const std::vector<ESCALINGMETHOD> &GetScalingMethods();

    void CompileOutputShaders();

    DXGI_FORMAT m_targetDxFormat = DXGI_FORMAT_UNKNOWN;
    std::map<ESCALINGMETHOD, std::unique_ptr<CRPWinOutputShader>> m_outputShaders;
  };

  class CRPWinRenderer : public CRPBaseRenderer
  {
  public:
    CRPWinRenderer(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPWinRenderer() override;

    // implementation of CRPBaseRenderer
    bool Supports(ERENDERFEATURE feature) const override;
    ESCALINGMETHOD GetDefaultScalingMethod() const override { return DEFAULT_SCALING_METHOD; }

    static bool SupportsScalingMethod(ESCALINGMETHOD method);

    /*!
     * \brief The default scaling method of the renderer
     */
    static const ESCALINGMETHOD DEFAULT_SCALING_METHOD = VS_SCALINGMETHOD_NEAREST;

  protected:
    // implementation of CRPBaseRenderer
    bool ConfigureInternal() override;
    void RenderInternal(bool clear, uint8_t alpha) override;

  private:
    void Render(CD3DTexture *target);
  };
}
}
