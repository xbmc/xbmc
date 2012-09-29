/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AmbiPiGrid.h"
#include "utils/log.h"

using namespace PERIPHERALS;

CAmbiPiGrid::~CAmbiPiGrid()
{
  free(m_tiles);
  free(m_tileData.stream);
}

#define SIZE_OF_X_AND_Y_COORDINATE 2
CAmbiPiGrid::CAmbiPiGrid(unsigned int width, unsigned int height) :
  m_width(width),
  m_height(height)
{  
  
  m_numTiles = (m_width * 2) + ((m_height - 2) * 2);
  unsigned int allocSize = m_numTiles * sizeof(Tile);
  m_tiles = (Tile*)malloc(allocSize);
  ZeroMemory(m_tiles, allocSize);


  m_tileData.streamLength = sizeof(m_tileData.streamLength) + (m_numTiles * (SIZE_OF_X_AND_Y_COORDINATE + sizeof(YUV)));
  m_tileData.stream = (BYTE *)malloc(m_tileData.streamLength);

  UpdateTileCoordinates(width, height);
  DumpCoordinates();
}

TileData *CAmbiPiGrid::GetTileData(void)
{
  BYTE *pStream = m_tileData.stream;

  ZeroMemory(pStream, m_tileData.streamLength);

  unsigned int tileIndex = 0;
  Tile* pTile;

  *pStream++ = (BYTE) (m_tileData.streamLength >> 24 & 0xFF);
  *pStream++ = (BYTE) (m_tileData.streamLength >> 16 & 0xFF);
  *pStream++ = (BYTE) (m_tileData.streamLength >> 8 & 0xFF);
  *pStream++ = (BYTE) (m_tileData.streamLength >> 0 & 0xFF);

  while (tileIndex < m_numTiles) {
    pTile = m_tiles + tileIndex;

    *pStream++ = (BYTE)pTile->m_x;
    *pStream++ = (BYTE)pTile->m_y;

    *pStream++ = pTile->m_yuv.y;
    *pStream++ = pTile->m_yuv.u;
    *pStream++ = pTile->m_yuv.v;

    tileIndex++;
  }  

  unsigned int bytesAddedToStream = pStream - m_tileData.stream;

  return &m_tileData;
}

void CAmbiPiGrid::UpdateTileCoordinates(unsigned int width, unsigned int height)
{
  unsigned int start_position_x = width / 2;

  unsigned int zeroBasedY;
  unsigned int zeroBasedX;

  unsigned int x;
  unsigned int y;

  unsigned int tileIndex = 0;

  for (y = 1; y <= height; y++)
  {
    zeroBasedY = y - 1;
    if (y == 1) 
    {
      // ProcessTopEdgeFromCenterToLeft
      for (x = start_position_x; x > 0; x--)
      {
        zeroBasedX = x - 1;
        UpdateSingleTileCoordinates(tileIndex++, zeroBasedX, zeroBasedY);
      }
    } 
    else if (y == height)
    {
      // ProcessBottomEdge
      for (x = 1; x < width; x++)
      {
        zeroBasedX = x - 1;
        UpdateSingleTileCoordinates(tileIndex++, zeroBasedX, zeroBasedY);
      }
    }
    else
    {
      // ProcessLeftEdgeOnly
      zeroBasedX = 0;
      UpdateSingleTileCoordinates(tileIndex++, zeroBasedX, zeroBasedY);
    }
  }

  for (y = height; y >= 1; y--)
  {
    zeroBasedY = y - 1;
    if (y == 1)
    {
      // ProcessTopEdgeFromRightToCenter
      for (x = width; x > start_position_x; x--)
      {
        zeroBasedX = x - 1;
        UpdateSingleTileCoordinates(tileIndex++, zeroBasedX, zeroBasedY);
      }
    }
    else
    {
      // ProcessRightEdgeOnly
      zeroBasedX = width - 1;
      UpdateSingleTileCoordinates(tileIndex++, zeroBasedX, zeroBasedY);
    }
  }

  CLog::Log(LOGDEBUG, "%s - leds positions processed: %d", __FUNCTION__, tileIndex);  
}

void CAmbiPiGrid::UpdateSingleTileCoordinates(unsigned int tileIndex, unsigned int x, unsigned int y)
{
  Tile* pTile = m_tiles + tileIndex;
  pTile->m_x = x;
  pTile->m_y = y;
}

void CAmbiPiGrid::DumpCoordinates(void)
{
  unsigned int tileIndex = 0;
  Tile* pTile;

  while (tileIndex < m_numTiles) {

    pTile = m_tiles + tileIndex;
    CLog::Log(LOGDEBUG, "%s - Coordinates, index: %d, x: %d, y: %d", __FUNCTION__, tileIndex, pTile->m_x, pTile->m_y);  

    tileIndex++;
  }  
}

void CAmbiPiGrid::UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight)
{
  unsigned int tileIndex = 0;
  Tile* pTile;

  unsigned int sampleWidth = imageWidth / m_width;
  unsigned int sampleHeight = imageHeight / m_height;

  CLog::Log(LOGDEBUG, "%s - Updating grid sample rectangles for image, width: %d, height: %d, sampleWidth: %d, sampleHeight: %d", __FUNCTION__, imageWidth, imageHeight, sampleWidth, sampleHeight);  

  while (tileIndex < m_numTiles) {

    pTile = m_tiles + tileIndex;

    pTile->m_sampleRect.SetRect(
      pTile->m_x * sampleWidth,
      pTile->m_y * sampleHeight,
      std::min(imageWidth, (pTile->m_x * sampleWidth) + sampleWidth),
      std::min(imageHeight, (pTile->m_y * sampleHeight) + sampleHeight)
    );

    CLog::Log(LOGDEBUG, "%s - updated rectangle, x: %d, y: %d, rect: (left: %f, top: %f, right: %f, bottom: %f)",
      __FUNCTION__, 
      pTile->m_x,
      pTile->m_y,
      pTile->m_sampleRect.x1,
      pTile->m_sampleRect.y1,
      pTile->m_sampleRect.x2,
      pTile->m_sampleRect.y2
    );  

    tileIndex++;
  }
}

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

void CAmbiPiGrid::UpdateTilesFromImage(const YV12Image* pImage)
{
  unsigned int tileIndex = 0;
  Tile* pTile;

  while (tileIndex < m_numTiles) {

    pTile = m_tiles + tileIndex;
    CalculateAverageColorForTile(pImage, pTile);
    tileIndex++;
  }  
}

void CAmbiPiGrid::CalculateAverageColorForTile(const YV12Image* pImage, Tile *pTile) {

  if (pImage->bpp != 1)
  {
    ZeroMemory(&pTile->m_yuv, sizeof(YUV));
    return; // TODO implement support for 2bpp images
  }

  BYTE *pY, *pU, *pV;

  RGB rgb;
  YUV yuv;

  AverageRGB averageRgb = { 0, 0, 0 };
  AverageYUV averageYuv = { 0, 0, 0 };

  unsigned long int pixelsInTile = (pTile->m_sampleRect.y2 - pTile->m_sampleRect.y1) * (pTile->m_sampleRect.x2 - pTile->m_sampleRect.x1);

  BYTE *pLineY = pImage->plane[PLANE_Y] + int((pImage->stride[PLANE_Y]/2) * pTile->m_sampleRect.y1);
  BYTE *pLineU = pImage->plane[PLANE_U] + int((pImage->stride[PLANE_U]/2) * pTile->m_sampleRect.y1);
  BYTE *pLineV = pImage->plane[PLANE_V] + int((pImage->stride[PLANE_V]/2) * pTile->m_sampleRect.y1);


  for (int y = pTile->m_sampleRect.y1; y < pTile->m_sampleRect.y2; y++) 
  {
    pY = pLineY + (int)pTile->m_sampleRect.x1;
    pU = pLineU + (int)pTile->m_sampleRect.x1;
    pV = pLineV + (int)pTile->m_sampleRect.x1;

    for (int x = pTile->m_sampleRect.x1; x < pTile->m_sampleRect.x2; x++) 
    {
      yuv.y = *(pY++);
      yuv.u = *(pU++);
      yuv.v = *(pV++);

      CImageConversion::ConvertYuvToRgb(&yuv, &rgb);

      UpdateAverageYuv(&yuv, pixelsInTile, &averageYuv);
      UpdateAverageRgb(&rgb, pixelsInTile, &averageRgb);
    }
    pLineY += (pImage->stride[PLANE_Y]/2);
    pLineU += (pImage->stride[PLANE_U]/2);
    pLineV += (pImage->stride[PLANE_V]/2);
  }
  UpdateAverageColorForTile(pTile, &averageYuv);
 
  /*
  CLog::Log(LOGDEBUG, "%s - average color for tile, x: %d, y: %d, RGB: #%02x%02x%02x, YUV: #%02x%02x%02x",
    __FUNCTION__, 
    pTile->m_x, 
    pTile->m_y, 
    (BYTE)averageRgb.r, 
    (BYTE)averageRgb.g,
    (BYTE)averageRgb.b,
    (BYTE)averageYuv.y, 
    (BYTE)averageYuv.u,
    (BYTE)averageYuv.v
  );
  */
}

void CAmbiPiGrid::UpdateAverageYuv(const YUV *pYuv, unsigned long int numPixels, AverageYUV *pAverageYuv)
{
  pAverageYuv->y += ((float)pYuv->y / numPixels);
  pAverageYuv->u += ((float)pYuv->u / numPixels);
  pAverageYuv->v += ((float)pYuv->v / numPixels);
}

void CAmbiPiGrid::UpdateAverageRgb(const RGB *pRgb, unsigned long int numPixels, AverageRGB *pAverageRgb)
{
  pAverageRgb->r += ((float)pRgb->r / numPixels);
  pAverageRgb->g += ((float)pRgb->g / numPixels);
  pAverageRgb->b += ((float)pRgb->b / numPixels);
}

void CAmbiPiGrid::UpdateAverageColorForTile(Tile *pTile, const AverageYUV *pAverageYuv)
{
  pTile->m_yuv.y = pAverageYuv->y;
  pTile->m_yuv.u = pAverageYuv->u;
  pTile->m_yuv.v = pAverageYuv->v;
}

void CImageConversion::ConvertYuvToRgb(const YUV *pYuv, RGB *pRgb)
{
  pRgb->r = pYuv->y + 1.4075 * (pYuv->v - 128);
  pRgb->g = pYuv->y - 0.3455 * (pYuv->u - 128) - (0.7169 * (pYuv->v - 128));
  pRgb->b = pYuv->y + 1.7790 * (pYuv->u - 128);
}
