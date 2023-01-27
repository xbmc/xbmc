/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPBaseRenderer.h"

#include "cores/RetroPlayer/buffers/IRenderBuffer.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

// Consider renderer visible until this many frames have passed without rendering
#define VISIBLE_DURATION_FRAME_COUNT 1

CRPBaseRenderer::CRPBaseRenderer(const CRenderSettings& renderSettings,
                                 CRenderContext& context,
                                 std::shared_ptr<IRenderBufferPool> bufferPool)
  : m_context(context), m_bufferPool(std::move(bufferPool)), m_renderSettings(renderSettings)
{
  m_bufferPool->RegisterRenderer(this);
}

CRPBaseRenderer::~CRPBaseRenderer()
{
  SetBuffer(nullptr);

  m_bufferPool->UnregisterRenderer(this);
}

bool CRPBaseRenderer::IsCompatible(const CRenderVideoSettings& settings) const
{
  return m_bufferPool->IsCompatible(settings);
}

bool CRPBaseRenderer::Configure(AVPixelFormat format)
{
  m_format = format;

  if (!m_bufferPool->IsConfigured())
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Configuring buffer pool");

    if (!m_bufferPool->Configure(format))
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to configure buffer pool");
      return false;
    }
  }

  if (ConfigureInternal())
    m_bConfigured = true;

  return m_bConfigured;
}

void CRPBaseRenderer::FrameMove()
{
  m_renderFrameCount++;
}

bool CRPBaseRenderer::IsVisible() const
{
  return m_renderFrameCount <= m_lastRender + VISIBLE_DURATION_FRAME_COUNT;
}

IRenderBuffer* CRPBaseRenderer::GetRenderBuffer() const
{
  if (m_renderBuffer != nullptr)
  {
    m_renderBuffer->Acquire();
    return m_renderBuffer;
  }

  return nullptr;
}

void CRPBaseRenderer::SetBuffer(IRenderBuffer* buffer)
{
  if (m_renderBuffer != buffer)
  {
    if (m_renderBuffer != nullptr)
      m_renderBuffer->Release();

    m_renderBuffer = buffer;

    if (m_renderBuffer != nullptr)
      m_renderBuffer->Acquire();
  }
}

void CRPBaseRenderer::RenderFrame(bool clear, uint8_t alpha)
{
  m_lastRender = m_renderFrameCount;

  if (!m_bConfigured || m_renderBuffer == nullptr)
    return;

  ManageRenderArea(*m_renderBuffer);

  RenderInternal(clear, alpha);
  PostRender();

  m_renderBuffer->SetRendered(true);
}

void CRPBaseRenderer::Flush()
{
  SetBuffer(nullptr);
  FlushInternal();
}

void CRPBaseRenderer::SetScalingMethod(SCALINGMETHOD method)
{
  m_renderSettings.VideoSettings().SetScalingMethod(method);
}

void CRPBaseRenderer::SetStretchMode(STRETCHMODE stretchMode)
{
  m_renderSettings.VideoSettings().SetRenderStretchMode(stretchMode);
}

void CRPBaseRenderer::SetRenderRotation(unsigned int rotationDegCCW)
{
  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}

void CRPBaseRenderer::SetPixels(const std::string& pixelPath)
{
  m_renderSettings.VideoSettings().SetPixels(pixelPath);
}

void CRPBaseRenderer::ManageRenderArea(const IRenderBuffer& renderBuffer)
{
  // Get texture parameters
  const unsigned int sourceWidth = renderBuffer.GetWidth();
  const unsigned int sourceHeight = renderBuffer.GetHeight();
  const unsigned int sourceRotationDegCCW = renderBuffer.GetRotation();
  const float sourceAspectRatio =
      static_cast<float>(sourceWidth) / static_cast<float>(sourceHeight);

  const SCALINGMETHOD scaleMode = m_renderSettings.VideoSettings().GetScalingMethod();
  const STRETCHMODE stretchMode = m_renderSettings.VideoSettings().GetRenderStretchMode();
  const unsigned int rotationDegCCW =
      (sourceRotationDegCCW + m_renderSettings.VideoSettings().GetRenderRotation()) % 360;

  // Get screen parameters
  float screenWidth;
  float screenHeight;
  //! @Todo screenPixelRatio unused - Possibly due to display integer scaling according to Garbear
  float screenPixelRatio;

  if (scaleMode == SCALINGMETHOD::NEAREST && stretchMode == STRETCHMODE::Original &&
      m_context.DisplayHardwareScalingEnabled())
  {
    screenWidth = sourceWidth;
    screenHeight = sourceHeight;
    screenPixelRatio = 1.0;
  }
  else
  {
    GetScreenDimensions(screenWidth, screenHeight, screenPixelRatio);
  }

  // Entire target rendering area for the video (including black bars)
  const CRect viewRect = m_context.GetViewWindow();

  // Calculate pixel ratio and zoom amount
  float pixelRatio = 1.0f;
  float zoomAmount = 1.0f;
  CRenderUtils::CalculateStretchMode(stretchMode, rotationDegCCW, sourceWidth, sourceHeight,
                                     screenWidth, screenHeight, pixelRatio, zoomAmount);

  // Calculate destination dimensions
  CRect destRect;
  CRenderUtils::CalcNormalRenderRect(viewRect, sourceAspectRatio * pixelRatio, zoomAmount,
                                     destRect);

  m_sourceRect.x1 = 0.0f;
  m_sourceRect.y1 = 0.0f;
  m_sourceRect.x2 = static_cast<float>(sourceWidth);
  m_sourceRect.y2 = static_cast<float>(sourceHeight);

  // Clip as needed
  if (!(m_context.IsFullScreenVideo() || m_context.IsCalibrating()))
    CRenderUtils::ClipRect(viewRect, m_sourceRect, destRect);

  if (stretchMode == STRETCHMODE::Zoom)
  {
    // Crop for zoom mode
    CRenderUtils::CropSource(m_sourceRect, rotationDegCCW, viewRect.Width(), viewRect.Height(),
                             static_cast<float>(sourceWidth), static_cast<float>(sourceHeight),
                             destRect.Width(), destRect.Height());
    destRect = viewRect;
  }

  // Adapt the drawing rect points if we have to rotate
  m_rotatedDestCoords = CRenderUtils::ReorderDrawPoints(destRect, rotationDegCCW);
}

void CRPBaseRenderer::MarkDirty()
{
  // CServiceBroker::GetGUI()->GetWindowManager().MarkDirty(m_dimensions); //! @todo
}

void CRPBaseRenderer::PreRender(bool clear)
{
  if (!m_bConfigured)
    return;

  // Clear screen
  if (clear)
    m_context.Clear(m_context.UseLimitedColor() ? UTILS::COLOR::LIMITED_BLACK
                                                : UTILS::COLOR::BLACK);
}

void CRPBaseRenderer::PostRender()
{
  m_context.ApplyStateBlock();
}

void CRPBaseRenderer::GetScreenDimensions(float& screenWidth,
                                          float& screenHeight,
                                          float& screenPixelRatio)
{
  // Get our calibrated full screen resolution
  RESOLUTION_INFO info = m_context.GetResInfo();

  screenWidth = static_cast<float>(info.Overscan.right - info.Overscan.left);
  screenHeight = static_cast<float>(info.Overscan.bottom - info.Overscan.top);

  // Splitres scaling factor
  float xscale = static_cast<float>(info.iScreenWidth) / static_cast<float>(info.iWidth);
  float yscale = static_cast<float>(info.iScreenHeight) / static_cast<float>(info.iHeight);

  screenWidth *= xscale;
  screenHeight *= yscale;

  screenPixelRatio = info.fPixelRatio;
}
