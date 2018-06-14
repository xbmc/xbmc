/*
 *      Copyright (C) 2018 Team Kodi
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

#include "RenderUtils.h"
#include "utils/MathUtils.h"

#include <cmath>

using namespace KODI;
using namespace RETRO;

void CRenderUtils::CalculateViewMode(VIEWMODE viewMode, unsigned int rotationDegCCW, unsigned int sourceWidth, unsigned int sourceHeight, float screenWidth, float screenHeight, float &pixelRatio, float &zoomAmount)
{
  const float sourceFrameRatio = static_cast<float>(sourceWidth) / static_cast<float>(sourceHeight);

  switch (viewMode)
  {
  case VIEWMODE::Normal:
  {
    switch (rotationDegCCW)
    {
      case 90:
      case 270:
      {
        pixelRatio = 1.0f / (sourceFrameRatio * sourceFrameRatio);
        break;
      }
      default:
        pixelRatio = 1.0f;
        break;
    }
    zoomAmount = 1.0f;

    break;
  }
  case VIEWMODE::Stretch4x3:
  {
    // Stretch to 4:3 ratio
    pixelRatio = (4.0f / 3.0f) / sourceFrameRatio;
    zoomAmount = 1.0f;

    break;
  }
  case VIEWMODE::Fullscreen:
  {
    // Stretch to the limits of the screen
    pixelRatio = (screenWidth / screenHeight) / sourceFrameRatio;
    zoomAmount = 1.0f;

    break;
  }
  case VIEWMODE::Original:
  {
    switch (rotationDegCCW)
    {
      case 90:
      case 270:
      {
        pixelRatio = 1.0f / (sourceFrameRatio * sourceFrameRatio);
        break;
      }
      default:
        pixelRatio = 1.0f;
        break;
    }

    // Calculate the correct zoom amount
    // First zoom to full width
    float newHeight = screenWidth / pixelRatio;
    if (newHeight > screenHeight)
    {
      // Zoom to full height
      newHeight = screenHeight;
    }

    // Now work out the zoom amount so that no zoom is done
    zoomAmount = sourceHeight / newHeight;

    switch (rotationDegCCW)
    {
      case 90:
      case 270:
      {
        zoomAmount *= sourceFrameRatio;
        break;
      }
      default:
        break;
    }

    break;
  }
  default:
    break;
  }
}

void CRenderUtils::CalcNormalRenderRect(const CRect &viewRect, float outputFrameRatio, float zoomAmount, CRect &destRect)
{
  const float offsetX = viewRect.x1;
  const float offsetY = viewRect.y1;
  const float width = viewRect.Width();
  const float height = viewRect.Height();

  // If view window is empty, set empty destination
  if (height == 0 || width == 0)
  {
    destRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

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
  if (std::fabs(newWidth - width) < 1.0f)
    newWidth = width;
  if (std::fabs(newHeight - height) < 1.0f)
    newHeight = height;

  // Center the game
  float posY = (height - newHeight) / 2;
  float posX = (width - newWidth) / 2;

  destRect.x1 = static_cast<float>(MathUtils::round_int(posX + offsetX));
  destRect.x2 = destRect.x1 + MathUtils::round_int(newWidth);
  destRect.y1 = static_cast<float>(MathUtils::round_int(posY + offsetY));
  destRect.y2 = destRect.y1 + MathUtils::round_int(newHeight);
}

void CRenderUtils::ClipRect(const CRect &viewRect, CRect &sourceRect, CRect &destRect)
{
  const float offsetX = viewRect.x1;
  const float offsetY = viewRect.y1;
  const float width = viewRect.Width();
  const float height = viewRect.Height();

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

std::array<CPoint, 4> CRenderUtils::ReorderDrawPoints(const CRect &destRect, unsigned int orientationDegCCW, float aspectRatio)
{
  std::array<CPoint, 4> rotatedDestCoords{};

  switch (orientationDegCCW)
  {
  case 0:
  {
    rotatedDestCoords[0] = CPoint{ destRect.x1, destRect.y1 }; // Top left
    rotatedDestCoords[1] = CPoint{ destRect.x2, destRect.y1 }; // Top right
    rotatedDestCoords[2] = CPoint{ destRect.x2, destRect.y2 }; // Bottom right
    rotatedDestCoords[3] = CPoint{ destRect.x1, destRect.y2 }; // Bottom left
    break;
  }
  case 90:
  {
    rotatedDestCoords[0] = CPoint{ destRect.x1, destRect.y2 }; // Bottom left
    rotatedDestCoords[1] = CPoint{ destRect.x1, destRect.y1 }; // Top left
    rotatedDestCoords[2] = CPoint{ destRect.x2, destRect.y1 }; // Top right
    rotatedDestCoords[3] = CPoint{ destRect.x2, destRect.y2 }; // Bottom right
    break;
  }
  case 180:
  {
    rotatedDestCoords[0] = CPoint{ destRect.x2, destRect.y2 }; // Bottom right
    rotatedDestCoords[1] = CPoint{ destRect.x1, destRect.y2 }; // Bottom left
    rotatedDestCoords[2] = CPoint{ destRect.x1, destRect.y1 }; // Top left
    rotatedDestCoords[3] = CPoint{ destRect.x2, destRect.y1 }; // Top right
    break;
  }
  case 270:
  {
    rotatedDestCoords[0] = CPoint{ destRect.x2, destRect.y1 }; // Top right
    rotatedDestCoords[1] = CPoint{ destRect.x2, destRect.y2 }; // Bottom right
    rotatedDestCoords[2] = CPoint{ destRect.x1, destRect.y2 }; // Bottom left
    rotatedDestCoords[3] = CPoint{ destRect.x1, destRect.y1 }; // Top left
    break;
  }
  default:
    break;
  }

  return rotatedDestCoords;
}
