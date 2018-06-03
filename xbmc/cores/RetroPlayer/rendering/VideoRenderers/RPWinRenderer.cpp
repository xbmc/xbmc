/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
#include "libswscale/swscale.h"
}

#include <cstring>

using namespace KODI;
using namespace RETRO;

// --- CWinRendererFactory -----------------------------------------------------

std::string CWinRendererFactory::RenderSystemName() const
{
  return "DirectX";
}

CRPBaseRenderer *CWinRendererFactory::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPWinRenderer(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CWinRendererFactory::CreateBufferPools(CRenderContext &context)
{
  return { std::make_shared<CWinRenderBufferPool>() };
}

// --- CWinRenderBuffer --------------------------------------------------------

CWinRenderBuffer::CWinRenderBuffer(AVPixelFormat pixFormat, DXGI_FORMAT dxFormat, unsigned int width, unsigned int height) :
  m_pixFormat(pixFormat),
  m_targetDxFormat(dxFormat),
  m_width(width),
  m_height(height),
  m_targetPixFormat(GetPixFormat(dxFormat))
{
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

uint8_t *CWinRenderBuffer::GetTexture()
{
  // Scale and upload texture
  D3D11_MAPPED_SUBRESOURCE destlr;
  if (!m_intermediateTarget->LockRect(0, &destlr, D3D11_MAP_WRITE_DISCARD))
  {
    CLog::Log(LOGERROR, "WinRenderer: Failed to lock swtarget texture into memory");
    return nullptr;
  }

  return static_cast<uint8_t*>(destlr.pData);
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

  const size_t destSize = m_width * m_height * (CD3DHelper::BitsPerPixel(m_targetDxFormat) >> 3);
  if (destSize == 0)
  {
    CLog::Log(LOGERROR, "WinRenderer: Unknown bits per pixel for DX format %d", m_targetDxFormat);
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

  uint8_t *destData = GetTexture();
  if (destData == nullptr)
    return false;

  ScalePixels(m_data.data(), m_data.size(), destData, destSize);

  if (!ReleaseTexture())
    return false;

  return true;
}

bool CWinRenderBuffer::CreateScalingContext()
{
  m_swsContext = sws_getContext(m_width, m_height, m_pixFormat, m_width, m_height, m_targetPixFormat,
    SWS_FAST_BILINEAR, NULL, NULL, NULL);

  if (m_swsContext == nullptr)
  {
    CLog::Log(LOGERROR, "WinRenderer: Failed to create swscale context");
    return false;
  }

  return true;
}

void CWinRenderBuffer::ScalePixels(uint8_t *source, size_t sourceSize, uint8_t *target, size_t targetSize)
{
  const int sourceStride = static_cast<int>(sourceSize / m_height);
  const int targetStride = static_cast<int>(targetSize / m_height);

  uint8_t* src[] =       { source,        nullptr,   nullptr,   nullptr };
  int      srcStride[] = { sourceStride,  0,         0,         0       };
  uint8_t *dst[] =       { target,        nullptr,   nullptr,   nullptr };
  int      dstStride[] = { targetStride,  0,         0,         0       };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);
}

AVPixelFormat CWinRenderBuffer::GetPixFormat(DXGI_FORMAT dxFormat)
{
  return AV_PIX_FMT_BGRA; //! @todo
}

// --- CWinRenderBufferPool ----------------------------------------------------

CWinRenderBufferPool::CWinRenderBufferPool()
{
  CompileOutputShaders();
}

bool CWinRenderBufferPool::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  return GetShader(renderSettings.GetScalingMethod()) != nullptr;
}

IRenderBuffer *CWinRenderBufferPool::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CWinRenderBuffer(m_format, m_targetDxFormat, m_width, m_height);
}

bool CWinRenderBufferPool::ConfigureDX(DXGI_FORMAT dxFormat)
{
  if (m_targetDxFormat != DXGI_FORMAT_UNKNOWN)
    return false; // Already configured

  m_targetDxFormat = dxFormat;

  return true;
}

CRPWinOutputShader *CWinRenderBufferPool::GetShader(ESCALINGMETHOD scalingMethod) const
{
  auto it = m_outputShaders.find(scalingMethod);

  if (it != m_outputShaders.end())
    return it->second.get();

  return nullptr;
}

const std::vector<ESCALINGMETHOD> &CWinRenderBufferPool::GetScalingMethods()
{
  static std::vector<ESCALINGMETHOD> scalingMethods = {
    VS_SCALINGMETHOD_NEAREST,
    VS_SCALINGMETHOD_LINEAR,
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
      CLog::Log(LOGERROR, "RPWinRenderer: Unable to create output shader (%s)",
        CRenderTranslator::TranslateScalingMethod(scalingMethod));
  }
}

// --- CRPWinRenderer ----------------------------------------------------------

CRPWinRenderer::CRPWinRenderer(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

CRPWinRenderer::~CRPWinRenderer()
{
  Deinitialize();
}

bool CRPWinRenderer::ConfigureInternal()
{
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  DXGI_FORMAT targetDxFormat = renderingDx->GetBackBuffer()->GetFormat();

  static_cast<CWinRenderBufferPool*>(m_bufferPool.get())->ConfigureDX(targetDxFormat);

  return true;
}

void CRPWinRenderer::RenderInternal(bool clear, uint8_t alpha)
{
  CRenderSystemDX *renderingDx = static_cast<CRenderSystemDX*>(m_context.Rendering());

  // Set alpha blend state
  renderingDx->SetAlphaBlendEnable(alpha < 0xFF);

  Render(renderingDx->GetBackBuffer());
}

bool CRPWinRenderer::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_STRETCH ||
      //feature == RENDERFEATURE_NONLINSTRETCH ||
      feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_VERTICAL_SHIFT ||
      feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

bool CRPWinRenderer::SupportsScalingMethod(ESCALINGMETHOD method)
{
  if (method == VS_SCALINGMETHOD_LINEAR ||
      method == VS_SCALINGMETHOD_NEAREST)
    return true;

  return false;
}

void CRPWinRenderer::Render(CD3DTexture *target)
{
  if (m_renderBuffer != nullptr)
  {
    CD3DTexture *intermediateTarget = static_cast<CWinRenderBuffer*>(m_renderBuffer)->GetTarget();
    if (intermediateTarget != nullptr)
    {
      CRect viewPort;
      m_context.GetViewPort(viewPort);

      // Pick appropriate output shader depending on the scaling method of the renderer
      ESCALINGMETHOD scalingMethod = m_renderSettings.VideoSettings().GetScalingMethod();

      CWinRenderBufferPool *bufferPool = static_cast<CWinRenderBufferPool*>(m_bufferPool.get());
      CRPWinOutputShader *outputShader = bufferPool->GetShader(scalingMethod);

      // Use the picked output shader to render to the target
      if (outputShader != nullptr)
      {
        outputShader->Render(*intermediateTarget, m_sourceWidth, m_sourceHeight,
          m_sourceRect, m_rotatedDestCoords, viewPort, target,
          m_context.UseLimitedColor() ? 1 : 0);
      }
    }
  }
}
