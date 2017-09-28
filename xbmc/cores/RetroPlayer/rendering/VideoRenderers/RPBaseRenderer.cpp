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
#include "cores/RetroPlayer/process/IRenderBuffer.h"
#include "cores/RetroPlayer/process/IRenderBufferPool.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "settings/Settings.h"
#include "utils/MathUtils.h"
#include "ServiceBroker.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>

using namespace KODI;
using namespace RETRO;

// Consider renderer visible until this many frames have passed without rendering
#define VISIBLE_DURATION_FRAME_COUNT  1

CRPBaseRenderer::CRPBaseRenderer(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  m_context(context),
  m_bufferPool(std::move(bufferPool)),
  m_renderSettings(renderSettings)
{
  m_oldDestRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);

  for(int i=0; i < 4; i++)
  {
    m_rotatedDestCoords[i].x = 0;
    m_rotatedDestCoords[i].y = 0;
  }

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

bool CRPBaseRenderer::Configure(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int orientation)
{
  m_format = format;
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_sourceFrameRatio = static_cast<float>(width) / static_cast<float>(height);
  m_renderOrientation = orientation;

  if (!m_bufferPool->IsConfigured())
    m_bufferPool->Configure(format, width, height);

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

void CRPBaseRenderer::GetVideoRect(CRect &source, CRect &dest, CRect &view) const
{
  source = m_sourceRect;
  dest = m_renderSettings.Geometry().Dimensions();
  view = m_viewRect;
}

float CRPBaseRenderer::GetAspectRatio() const
{
  return m_sourceFrameRatio;
}

void CRPBaseRenderer::SetScalingMethod(ESCALINGMETHOD method)
{
  m_renderSettings.VideoSettings().SetScalingMethod(method);
}

void CRPBaseRenderer::SetViewMode(ViewMode viewMode)
{
  m_renderSettings.VideoSettings().SetRenderViewMode(viewMode);
  CalculateViewMode();
}

void CRPBaseRenderer::CalculateViewMode()
{
  const ViewMode viewMode = m_renderSettings.VideoSettings().GetRenderViewMode();

  // Parameters to determine
  float &pixelRatio = m_pixelRatio;
  float &zoomAmount = m_zoomAmount;
  bool &bNonLinearStretch = m_bNonLinearStretch;

  // Get our calibrated full screen resolution
  RESOLUTION res = m_context.GetVideoResolution();
  RESOLUTION_INFO info = m_context.GetResInfo();

  float screenWidth = static_cast<float>(info.Overscan.right - info.Overscan.left);
  float screenHeight = static_cast<float>(info.Overscan.bottom - info.Overscan.top);

  // And the source frame ratio
  float sourceFrameRatio = GetAspectRatio();

  // Splitres scaling factor
  float xscale = static_cast<float>(info.iScreenWidth) / static_cast<float>(info.iWidth);
  float yscale = static_cast<float>(info.iScreenHeight) / static_cast<float>(info.iHeight);

  screenWidth *= xscale;
  screenHeight *= yscale;

  bNonLinearStretch = false;

  switch (viewMode)
  {
  case ViewModeZoom:
  {
    // Zoom image so no black bars
    pixelRatio = 1.0f;

    // Calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * pixelRatio / info.fPixelRatio;

    // Now calculate the correct zoom amount
    // First zoom to full height
    float newHeight = screenHeight;
    float newWidth = newHeight * outputFrameRatio;

    zoomAmount = newWidth / screenWidth;

    if (newWidth < screenWidth)
    {
      // Zoom to full width
      newWidth = screenWidth;
      newHeight = newWidth / outputFrameRatio;
      zoomAmount = newHeight / screenHeight;
    }

    break;
  }
  case ViewModeStretch4x3:
  {
    // Stretch image to 4:3 ratio
    zoomAmount = 1.0f;

    if (res == RES_PAL_4x3 || res == RES_PAL60_4x3 || res == RES_NTSC_4x3 || res == RES_HDTV_480p_4x3)
    {
      // Stretch to the limits of the 4:3 screen
      // Incorrect behaviour, but it's what the users want, so...
      pixelRatio = (screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio;
    }
    else
    {
      // Now we need to set pixelRatio so that fOutputFrameRatio = 4:3
      pixelRatio = (4.0f / 3.0f) / sourceFrameRatio;
    }

    break;
  }
  case ViewModeWideZoom:
  {
    // Super zoom
    float stretchAmount = (screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio;

    pixelRatio = pow(stretchAmount, float(2.0 / 3.0));
    zoomAmount = pow(stretchAmount, float((stretchAmount < 1.0) ? -1.0 / 3.0 : 1.0 / 3.0));

    bNonLinearStretch = true;

    break;
  }
  case ViewModeStretch16x9:
  case ViewModeStretch16x9Nonlin:
  {
    // Stretch image to 16:9 ratio
    zoomAmount = 1.0f;

    if (res == RES_PAL_4x3 || res == RES_PAL60_4x3 || res == RES_NTSC_4x3 || res == RES_HDTV_480p_4x3)
    {
      // Now we need to set pixelRatio so that outputFrameRatio = 16:9.
      pixelRatio = (16.0f / 9.0f) / sourceFrameRatio;
    }
    else
    {
      // Stretch to the limits of the 16:9 screen
      // Incorrect behaviour, but it's what the users want, so...
      pixelRatio = (screenWidth / screenHeight) * info.fPixelRatio / sourceFrameRatio;
    }

    bNonLinearStretch = (viewMode == ViewModeStretch16x9Nonlin);

    break;
  }
  case ViewModeOriginal:
  {
    // Zoom image so that the height is the original size
    pixelRatio = 1.0f;

    // Get the size of the media file
    // Calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * pixelRatio / info.fPixelRatio;

    // Now calculate the correct zoom amount.  First zoom to full width.
    float newHeight = screenWidth / outputFrameRatio;
    if (newHeight > screenHeight)
    {
      // Zoom to full height
      newHeight = screenHeight;
    }

    // Now work out the zoom amount so that no zoom is done
    zoomAmount = m_sourceHeight / newHeight;

    break;
  }
  case ViewModeNormal:
  {
    pixelRatio = 1.0f;
    zoomAmount = 1.0f;
    break;
  }
  default:
    break;
  }
}

inline void CRPBaseRenderer::ReorderDrawPoints()
{
  const CRect &destRect = m_renderSettings.Geometry().Dimensions();

  // 0 - top left, 1 - top right, 2 - bottom right, 3 - bottom left
  float origMat[4][2] =
    {
      { destRect.x1, destRect.y1 },
      { destRect.x2, destRect.y1 },
      { destRect.x2, destRect.y2 },
      { destRect.x1, destRect.y2 }
    };

  bool changeAspect = false;
  int pointOffset = 0;

  switch (m_renderOrientation)
  {
  case 90:
    pointOffset = 1;
    changeAspect = true;
    break;
  case 180:
    pointOffset = 2;
    break;
  case 270:
    pointOffset = 3;
    changeAspect = true;
    break;
  }

  // If renderer doesn't support rotation, treat orientation as 0 degree so
  // that ffmpeg might handle it
  if (!Supports(RENDERFEATURE_ROTATION))
  {
    pointOffset = 0;
    changeAspect = false;
  }

  float diffX = 0.0f;
  float diffY = 0.0f;
  float centerX = 0.0f;
  float centerY = 0.0f;

  if (changeAspect) // We are either rotating by 90 or 270 degrees which inverts aspect ratio
  {
    float newWidth = destRect.Height(); // New width is old height
    float newHeight = destRect.Width(); // New height is old width
    float diffWidth = newWidth - destRect.Width(); // Difference between old and new width
    float diffHeight = newHeight - destRect.Height(); // Difference between old and new height

    // If the new width is bigger then the old or the new height is bigger
    // then the old, we need to scale down
    if (diffWidth > 0.0f || diffHeight > 0.0f)
    {
      float aspectRatio = GetAspectRatio();

      // Scale to fit screen width because the difference in width is bigger
      // then the difference in height
      if (diffWidth > diffHeight)
      {
        // Clamp to the width of the old dest rect
        newWidth = destRect.Width();
        newHeight *= aspectRatio;
      }
      else // Scale to fit screen height
      {
        // Clamp to the height of the old dest rect
        newHeight = destRect.Height();
        newWidth /= aspectRatio;
      }
    }

    // Calculate the center point of the view
    centerX = m_viewRect.x1 + m_viewRect.Width() / 2.0f;
    centerY = m_viewRect.y1 + m_viewRect.Height() / 2.0f;

    // Calculate the number of pixels we need to go in each x direction from
    // the center point
    diffX = newWidth / 2;
    // Calculate the number of pixels we need to go in each y direction from
    // the center point
    diffY = newHeight / 2;
  }

  for (int destIdx = 0, srcIdx = pointOffset; destIdx < 4; destIdx++)
  {
    m_rotatedDestCoords[destIdx].x = origMat[srcIdx][0];
    m_rotatedDestCoords[destIdx].y = origMat[srcIdx][1];

    if (changeAspect)
    {
      switch (srcIdx)
      {
      case 0:// top left
        m_rotatedDestCoords[destIdx].x = centerX - diffX;
        m_rotatedDestCoords[destIdx].y = centerY - diffY;
        break;
      case 1:// top right
        m_rotatedDestCoords[destIdx].x = centerX + diffX;
        m_rotatedDestCoords[destIdx].y = centerY - diffY;
        break;
      case 2:// bottom right
        m_rotatedDestCoords[destIdx].x = centerX + diffX;
        m_rotatedDestCoords[destIdx].y = centerY + diffY;
        break;
      case 3:// bottom left
        m_rotatedDestCoords[destIdx].x = centerX - diffX;
        m_rotatedDestCoords[destIdx].y = centerY + diffY;
        break;
      }
    }
    srcIdx++;
    srcIdx = srcIdx % 4;
  }
}

void CRPBaseRenderer::CalcNormalRenderRect(float offsetX, float offsetY, float width, float height, float inputFrameRatio, float zoomAmount)
{
  CRect &sourceRect = m_sourceRect;
  CRect &destRect = m_renderSettings.Geometry().Dimensions();

  // If view window is empty, set empty destination
  if (height == 0 || width == 0)
  {
    destRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  // Scale up image as much as possible and keep the aspect ratio (introduces
  // with black bars)
  // Calculate the correct output frame ratio (using the users pixel ratio
  // setting and the output pixel ratio setting)
  float outputFrameRatio = inputFrameRatio / m_context.GetResInfo().fPixelRatio;

  // Allow a certain error to maximize size of render area
  float fCorrection = width / height / outputFrameRatio - 1.0f;
  float fAllowed = GetAllowedErrorInAspect();

  if (fCorrection > fAllowed)
    fCorrection = fAllowed;

  if (fCorrection < -fAllowed)
    fCorrection = -fAllowed;

  outputFrameRatio *= 1.0f + fCorrection;

  // Maximize the game width
  float newWidth = width;
  float newHeight = newWidth / outputFrameRatio;

  if (newHeight > height)
  {
    newHeight = height;
    newWidth = newHeight * outputFrameRatio;
  }

  // Scale the game up by set zoom amount
  newWidth *= zoomAmount;
  newHeight *= zoomAmount;

  // If we are less than one pixel off use the complete screen instead
  if (std::abs(newWidth - width) < 1.0f)
    newWidth = width;
  if (std::abs(newHeight - height) < 1.0f)
    newHeight = height;

  // Center the game
  float posY = (height - newHeight) / 2;
  float posX = (width - newWidth) / 2;

  const float verticalShift = 0.0f; //! @todo

  // Vertical shift range -1 to 1 shifts within the top and bottom black bars
  // If there are no top and bottom black bars, this range does nothing
  float blackBarSize = std::max((height - newHeight) / 2.0f, 0.0f);
  posY += blackBarSize * std::max(std::min(verticalShift, 1.0f), -1.0f);

  // Vertical shift ranges -2 to -1 and 1 to 2 will shift the image out of the screen
  // If vertical shift is -2 it will be completely shifted out the top,
  // if it's 2 it will be completely shifted out the bottom
  float shiftRange = std::min(newHeight, newHeight - (newHeight - height) / 2.0f);
  if (verticalShift > 1.0f)
    posY += shiftRange * (verticalShift - 1.0f);
  else if (verticalShift < -1.0f)
    posY += shiftRange * (verticalShift + 1.0f);

  destRect.x1 = static_cast<float>(MathUtils::round_int(posX + offsetX));
  destRect.x2 = destRect.x1 + MathUtils::round_int(newWidth);
  destRect.y1 = static_cast<float>(MathUtils::round_int(posY + offsetY));
  destRect.y2 = destRect.y1 + MathUtils::round_int(newHeight);

  // Clip as needed
  if (!(m_context.IsFullScreenVideo() || m_context.IsCalibrating()))
  {
    CRect original(destRect);
    destRect.Intersect(CRect(offsetX, offsetY, offsetX + width, offsetY + height));
    if (destRect != original)
    {
      float scaleX = sourceRect.Width() / original.Width();
      float scaleY = sourceRect.Height() / original.Height();
      sourceRect.x1 += (destRect.x1 - original.x1) * scaleX;
      sourceRect.y1 += (destRect.y1 - original.y1) * scaleY;
      sourceRect.x2 += (destRect.x2 - original.x2) * scaleX;
      sourceRect.y2 += (destRect.y2 - original.y2) * scaleY;
    }
  }

  UpdateDrawPoints(destRect);
}

void CRPBaseRenderer::UpdateDrawPoints(const CRect &destRect)
{
  if (m_oldDestRect != destRect || m_oldRenderOrientation != m_renderOrientation)
  {
    // Adapt the drawing rect points if we have to rotate and either destrect
    // or orientation changed
    ReorderDrawPoints();
    m_oldDestRect = destRect;
    m_oldRenderOrientation = m_renderOrientation;
  }
}

void CRPBaseRenderer::ManageRenderArea()
{
  m_viewRect = m_context.GetViewWindow();

  m_sourceRect.x1 = 0.0f;
  m_sourceRect.y1 = 0.0f;
  m_sourceRect.x2 = static_cast<float>(m_sourceWidth);
  m_sourceRect.y2 = static_cast<float>(m_sourceHeight);

  CalcNormalRenderRect(m_viewRect.x1, m_viewRect.y1, m_viewRect.Width(), m_viewRect.Height(), GetAspectRatio() * m_pixelRatio, m_zoomAmount);
  CalculateViewMode();
}

void CRPBaseRenderer::MarkDirty()
{
  //g_windowManager.MarkDirty(m_renderSettings.Geometry().Dimensions()); //! @todo
}

float CRPBaseRenderer::GetAllowedErrorInAspect() const
{
  return CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT) * 0.01f;
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
