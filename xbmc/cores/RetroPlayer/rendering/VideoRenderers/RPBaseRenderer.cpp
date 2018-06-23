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

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/buffers/IRenderBuffer.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

// Consider renderer visible until this many frames have passed without rendering
#define VISIBLE_DURATION_FRAME_COUNT  1

CRPBaseRenderer::CRPBaseRenderer(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  m_context(context),
  m_bufferPool(std::move(bufferPool)),
  m_renderSettings(renderSettings)
{
  m_bufferPool->RegisterRenderer(this);
}

CRPBaseRenderer::~CRPBaseRenderer()
{
  SetBuffer(nullptr);

  m_bufferPool->UnregisterRenderer(this);
}

bool CRPBaseRenderer::IsCompatible(const CRenderVideoSettings &settings) const
{
  if (!m_bufferPool->IsCompatible(settings))
    return false;

  return true;
}

bool CRPBaseRenderer::Configure(AVPixelFormat format, unsigned int width, unsigned int height)
{
  m_format = format;
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = 0; //! @todo

  if (!m_bufferPool->IsConfigured())
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Configuring buffer pool");

    if (!m_bufferPool->Configure(format))
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to configure buffer pool");
      return false;
    }
  }

  ManageRenderArea();

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
  if (m_renderFrameCount <= m_lastRender + VISIBLE_DURATION_FRAME_COUNT)
    return true;

  return false;
}

void CRPBaseRenderer::SetBuffer(IRenderBuffer *buffer)
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

  RenderInternal(clear, alpha);
  PostRender();

  m_renderBuffer->SetRendered(true);
}

void CRPBaseRenderer::Flush()
{
  SetBuffer(nullptr);
  FlushInternal();
}

float CRPBaseRenderer::GetAspectRatio() const
{
  return static_cast<float>(m_sourceWidth) / static_cast<float>(m_sourceHeight);
}

unsigned int CRPBaseRenderer::GetRotationDegCCW() const
{
  unsigned int renderOrientation = m_renderSettings.VideoSettings().GetRenderRotation();
  return (renderOrientation + m_renderOrientation) % 360;
}

void CRPBaseRenderer::SetScalingMethod(SCALINGMETHOD method)
{
  m_renderSettings.VideoSettings().SetScalingMethod(method);
}

void CRPBaseRenderer::SetViewMode(VIEWMODE viewMode)
{
  m_renderSettings.VideoSettings().SetRenderViewMode(viewMode);
}

void CRPBaseRenderer::SetRenderRotation(unsigned int rotationDegCCW)
{
  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}

void CRPBaseRenderer::ManageRenderArea()
{
  const VIEWMODE viewMode = m_renderSettings.VideoSettings().GetRenderViewMode();
  const unsigned int rotationDegCCW = GetRotationDegCCW();

  // Get screen parameters
  float screenWidth;
  float screenHeight;
  float screenPixelRatio;
  GetScreenDimensions(screenWidth, screenHeight, screenPixelRatio);

  // Entire target rendering area for the video (including black bars)
  const CRect viewRect = m_context.GetViewWindow();

  // Calculate pixel ratio and zoom amount
  float pixelRatio = 1.0f;
  float zoomAmount = 1.0f;
  CRenderUtils::CalculateViewMode(viewMode, rotationDegCCW, m_sourceWidth, m_sourceHeight, screenWidth, screenHeight, pixelRatio, zoomAmount);

  // Calculate destination dimensions
  CRect destRect;
  CRenderUtils::CalcNormalRenderRect(viewRect, GetAspectRatio() * pixelRatio, zoomAmount, destRect);

  m_sourceRect.x1 = 0.0f;
  m_sourceRect.y1 = 0.0f;
  m_sourceRect.x2 = static_cast<float>(m_sourceWidth);
  m_sourceRect.y2 = static_cast<float>(m_sourceHeight);

  // Clip as needed
  if (!(m_context.IsFullScreenVideo() || m_context.IsCalibrating()))
    CRenderUtils::ClipRect(viewRect, m_sourceRect, destRect);

  // Adapt the drawing rect points if we have to rotate
  m_rotatedDestCoords = CRenderUtils::ReorderDrawPoints(destRect, rotationDegCCW, GetAspectRatio());
}

void CRPBaseRenderer::MarkDirty()
{
  //CServiceBroker::GetGUI()->GetWindowManager().MarkDirty(m_dimensions); //! @todo
}

void CRPBaseRenderer::PreRender(bool clear)
{
  if (!m_bConfigured)
    return;

  // Clear screen
  if (clear)
    m_context.Clear(m_context.UseLimitedColor() ? 0x101010 : 0);

  ManageRenderArea();
}

void CRPBaseRenderer::PostRender()
{
  m_context.ApplyStateBlock();
}

void CRPBaseRenderer::GetScreenDimensions(float &screenWidth, float &screenHeight, float &screenPixelRatio)
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
