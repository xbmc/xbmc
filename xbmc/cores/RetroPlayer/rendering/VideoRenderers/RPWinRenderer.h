/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <memory>
#include <stdint.h>
#include <vector>

#include <dxgi.h>

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
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
};

class CWinRenderBuffer : public CRenderBufferSysMem
{
public:
  CWinRenderBuffer(AVPixelFormat pixFormat, DXGI_FORMAT dxFormat);
  ~CWinRenderBuffer() override;

  // implementation of IRenderBuffer via CRenderBufferSysMem
  bool UploadTexture() override;

  CD3DTexture* GetTarget() { return m_intermediateTarget.get(); }

private:
  bool CreateTexture();
  bool GetTexture(uint8_t*& data, unsigned int& stride);
  bool ReleaseTexture();

  bool CreateScalingContext();
  void ScalePixels(const uint8_t* source,
                   unsigned int sourceStride,
                   uint8_t* target,
                   unsigned int targetStride);

  static AVPixelFormat GetPixFormat();

  // Construction parameters
  const AVPixelFormat m_pixFormat;
  const DXGI_FORMAT m_targetDxFormat;

  AVPixelFormat m_targetPixFormat;
  std::unique_ptr<CD3DTexture> m_intermediateTarget;

  SwsContext* m_swsContext = nullptr;
};

class CWinRenderBufferPool : public CBaseRenderBufferPool
{
public:
  CWinRenderBufferPool();
  ~CWinRenderBufferPool() override = default;

  // implementation of IRenderBufferPool via CRenderBufferPoolSysMem
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

  // implementation of CBaseRenderBufferPool via CRenderBufferPoolSysMem
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;

  // DirectX interface
  bool ConfigureDX();
  CRPWinOutputShader* GetShader(SCALINGMETHOD scalingMethod) const;

private:
  static const std::vector<SCALINGMETHOD>& GetScalingMethods();

  void CompileOutputShaders();

  DXGI_FORMAT m_targetDxFormat = DXGI_FORMAT_UNKNOWN;
  std::map<SCALINGMETHOD, std::unique_ptr<CRPWinOutputShader>> m_outputShaders;
};

class CRPWinRenderer : public CRPBaseRenderer
{
public:
  CRPWinRenderer(const CRenderSettings& renderSettings,
                 CRenderContext& context,
                 std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPWinRenderer() override = default;

  // implementation of CRPBaseRenderer
  bool Supports(RENDERFEATURE feature) const override;
  SCALINGMETHOD GetDefaultScalingMethod() const override { return DEFAULT_SCALING_METHOD; }

  static bool SupportsScalingMethod(SCALINGMETHOD method);

  /*!
   * \brief The default scaling method of the renderer
   */
  static const SCALINGMETHOD DEFAULT_SCALING_METHOD = SCALINGMETHOD::NEAREST;

protected:
  // implementation of CRPBaseRenderer
  bool ConfigureInternal() override;
  void RenderInternal(bool clear, uint8_t alpha) override;

private:
  void Render(CD3DTexture& target);
};
} // namespace RETRO
} // namespace KODI
