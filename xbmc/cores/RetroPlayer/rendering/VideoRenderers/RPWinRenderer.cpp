/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPWinRenderer.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/rendering/VideoShaders/windows/RPWinOutputShader.h"
#include "guilib/D3DResource.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"

extern "C"
{
#include <libswscale/swscale.h>
}

#include <cstring>

using namespace KODI;
using namespace RETRO;

// --- CWinRendererFactory -----------------------------------------------------

std::string CWinRendererFactory::RenderSystemName() const
{
  return "DirectX";
}

CRPBaseRenderer* CWinRendererFactory::CreateRenderer(const CRenderSettings& settings,
                                                     CRenderContext& context,
                                                     std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPWinRenderer(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CWinRendererFactory::CreateBufferPools(CRenderContext& context)
{
  return {std::make_shared<CWinRenderBufferPool>()};
}

// --- CWinRenderBuffer --------------------------------------------------------

CWinRenderBuffer::CWinRenderBuffer(AVPixelFormat pixFormat, DXGI_FORMAT dxFormat)
  : m_pixFormat(pixFormat), m_targetDxFormat(dxFormat), m_targetPixFormat(GetPixFormat())
{
}

CWinRenderBuffer::~CWinRenderBuffer()
{
  if (m_swsContext != nullptr)
    sws_freeContext(m_swsContext);
}

bool CWinRenderBuffer::CreateTexture()
{
  if (!m_intermediateTarget->Create(m_width, m_height, 1, D3D11_USAGE_DYNAMIC, m_targetDxFormat))
  {
    CLog::Log(LOGERROR, "WinRenderer: Intermediate render target creation failed");
    return false;
  }

  return true;
}

bool CWinRenderBuffer::GetTexture(uint8_t*& data, unsigned int& stride)
{
  // Scale and upload texture
  D3D11_MAPPED_SUBRESOURCE destlr;
  if (!m_intermediateTarget->LockRect(0, &destlr, D3D11_MAP_WRITE_DISCARD))
  {
    CLog::Log(LOGERROR, "WinRenderer: Failed to lock swtarget texture into memory");
    return false;
  }

  data = static_cast<uint8_t*>(destlr.pData);
  stride = destlr.RowPitch;

  return true;
}

bool CWinRenderBuffer::ReleaseTexture()
{
  if (!m_intermediateTarget->UnlockRect(0))
  {
    CLog::Log(LOGERROR, "WinRenderer: Failed to unlock swtarget texture");
    return false;
  }

  return true;
}

bool CWinRenderBuffer::UploadTexture()
{
  if (m_targetDxFormat == DXGI_FORMAT_UNKNOWN)
  {
    CLog::Log(LOGERROR, "WinRenderer: Invalid DX texture format");
    return false;
  }

  if (!CreateScalingContext())
    return false;

  // Create intermediate texture
  if (!m_intermediateTarget)
  {
    m_intermediateTarget.reset(new CD3DTexture);
    if (!CreateTexture())
    {
      m_intermediateTarget.reset();
      return false;
    }
  }

  uint8_t* destData = nullptr;
  unsigned int destStride = 0;
  if (!GetTexture(destData, destStride))
    return false;

  const unsigned int sourceStride = static_cast<unsigned int>(m_data.size() / m_height);
  ScalePixels(m_data.data(), sourceStride, destData, destStride);

  if (!ReleaseTexture())
    return false;

  return true;
}

bool CWinRenderBuffer::CreateScalingContext()
{
  if (m_swsContext == nullptr)
  {
    m_swsContext = sws_getContext(m_width, m_height, m_pixFormat, m_width, m_height,
                                  m_targetPixFormat, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    if (m_swsContext == nullptr)
    {
      CLog::Log(LOGERROR, "WinRenderer: Failed to create swscale context");
      return false;
    }
  }

  return true;
}

void CWinRenderBuffer::ScalePixels(const uint8_t* source,
                                   unsigned int sourceStride,
                                   uint8_t* target,
                                   unsigned int targetStride)
{
  uint8_t* src[] = {const_cast<uint8_t*>(source), nullptr, nullptr, nullptr};
  int srcStride[] = {static_cast<int>(sourceStride), 0, 0, 0};
  uint8_t* dst[] = {target, nullptr, nullptr, nullptr};
  int dstStride[] = {static_cast<int>(targetStride), 0, 0, 0};

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);
}

AVPixelFormat CWinRenderBuffer::GetPixFormat()
{
  return AV_PIX_FMT_BGRA;
}

// --- CWinRenderBufferPool ----------------------------------------------------

CWinRenderBufferPool::CWinRenderBufferPool()
{
  CompileOutputShaders();
}

bool CWinRenderBufferPool::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return GetShader(renderSettings.GetScalingMethod()) != nullptr;
}

IRenderBuffer* CWinRenderBufferPool::CreateRenderBuffer(void* header /* = nullptr */)
{
  return new CWinRenderBuffer(m_format, m_targetDxFormat);
}

bool CWinRenderBufferPool::ConfigureDX()
{
  if (m_targetDxFormat != DXGI_FORMAT_UNKNOWN)
    return false; // Already configured

  // There are three pixel formats used by libretro: 0RGB32, RGB565 and
  // RGB555. DirectX support for these varies, so always use BGRA32 as the
  // intermediate format.
  m_targetDxFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

  return true;
}

CRPWinOutputShader* CWinRenderBufferPool::GetShader(SCALINGMETHOD scalingMethod) const
{
  auto it = m_outputShaders.find(scalingMethod);

  if (it != m_outputShaders.end())
    return it->second.get();

  return nullptr;
}

const std::vector<SCALINGMETHOD>& CWinRenderBufferPool::GetScalingMethods()
{
  static std::vector<SCALINGMETHOD> scalingMethods = {
      SCALINGMETHOD::NEAREST,
      SCALINGMETHOD::LINEAR,
  };

  return scalingMethods;
}

void CWinRenderBufferPool::CompileOutputShaders()
{
  for (auto scalingMethod : GetScalingMethods())
  {
    std::unique_ptr<CRPWinOutputShader> outputShader(new CRPWinOutputShader);
    if (outputShader->Create(scalingMethod))
      m_outputShaders[scalingMethod] = std::move(outputShader);
    else
      CLog::Log(LOGERROR, "RPWinRenderer: Unable to create output shader ({})",
                CRenderTranslator::TranslateScalingMethod(scalingMethod));
  }
}

// --- CRPWinRenderer ----------------------------------------------------------

CRPWinRenderer::CRPWinRenderer(const CRenderSettings& renderSettings,
                               CRenderContext& context,
                               std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

bool CRPWinRenderer::ConfigureInternal()
{
  CRenderSystemDX* renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  static_cast<CWinRenderBufferPool*>(m_bufferPool.get())->ConfigureDX();

  return true;
}

void CRPWinRenderer::RenderInternal(bool clear, uint8_t alpha)
{
  CRenderSystemDX* renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  // Set alpha blend state
  renderingDx->SetAlphaBlendEnable(alpha < 0xFF);

  Render(renderingDx->GetBackBuffer());
}

bool CRPWinRenderer::Supports(RENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
      feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION)
    return true;

  return false;
}

bool CRPWinRenderer::SupportsScalingMethod(SCALINGMETHOD method)
{
  if (method == SCALINGMETHOD::LINEAR || method == SCALINGMETHOD::NEAREST)
    return true;

  return false;
}

void CRPWinRenderer::Render(CD3DTexture& target)
{
  const CPoint destPoints[4] = {m_rotatedDestCoords[0], m_rotatedDestCoords[1],
                                m_rotatedDestCoords[2], m_rotatedDestCoords[3]};

  if (m_renderBuffer != nullptr)
  {
    CD3DTexture* intermediateTarget = static_cast<CWinRenderBuffer*>(m_renderBuffer)->GetTarget();
    if (intermediateTarget != nullptr)
    {
      CRect viewPort;
      m_context.GetViewPort(viewPort);

      // Pick appropriate output shader depending on the scaling method of the renderer
      SCALINGMETHOD scalingMethod = m_renderSettings.VideoSettings().GetScalingMethod();

      CWinRenderBufferPool* bufferPool = static_cast<CWinRenderBufferPool*>(m_bufferPool.get());
      CRPWinOutputShader* outputShader = bufferPool->GetShader(scalingMethod);

      // Use the picked output shader to render to the target
      if (outputShader != nullptr)
      {
        outputShader->Render(*intermediateTarget, m_sourceRect, destPoints, viewPort, &target,
                             m_context.UseLimitedColor() ? 1 : 0);
      }
    }
  }
}
