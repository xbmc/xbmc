/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "DVDCodecUtils.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"

// allocate a new picture (PIX_FMT_YUV420P)
DVDVideoPicture* CDVDCodecUtils::AllocatePicture(int iWidth, int iHeight)
{
  DVDVideoPicture* pPicture = new DVDVideoPicture;
  if (pPicture)
  {
    pPicture->iWidth = iWidth;
    pPicture->iHeight = iHeight;

    int w = iWidth / 2;
    int h = iHeight / 2;
    int size = w * h;
    int totalsize = (iWidth * iHeight) + size * 2;
    BYTE* data = new BYTE[totalsize];
    if (data)
    {
      pPicture->data[0] = data;
      pPicture->data[1] = pPicture->data[0] + (iWidth * iHeight);
      pPicture->data[2] = pPicture->data[1] + size;
      pPicture->data[3] = NULL;
      pPicture->iLineSize[0] = iWidth;
      pPicture->iLineSize[1] = w;
      pPicture->iLineSize[2] = w;
      pPicture->iLineSize[3] = 0;
    }
    else
    {
      CLog::Log(LOGFATAL, "CDVDCodecUtils::AllocatePicture, unable to allocate new video picture, out of memory.");
      delete pPicture;
      pPicture = NULL;
    }
  }
  return pPicture;
}

void CDVDCodecUtils::FreePicture(DVDVideoPicture* pPicture)
{
  delete[] pPicture->data[0];
  delete pPicture;
}

bool CDVDCodecUtils::CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc)
{
  BYTE *s, *d;
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;

  s = pSrc->data[0];
  d = pDst->data[0];

  for (int y = 0; y < h; y++)
  {
    fast_memcpy(d, s, w);
    s += pSrc->iLineSize[0];
    d += pDst->iLineSize[0];
  }

  w >>= 1;
  h >>= 1;

  s = pSrc->data[1];
  d = pDst->data[1];
  for (int y = 0; y < h; y++)
  {
    fast_memcpy(d, s, w);
    s += pSrc->iLineSize[1];
    d += pDst->iLineSize[1];
  }

  s = pSrc->data[2];
  d = pDst->data[2];
  for (int y = 0; y < h; y++)
  {
    fast_memcpy(d, s, w);
    s += pSrc->iLineSize[2];
    d += pDst->iLineSize[2];
  }
  return true;
}

bool CDVDCodecUtils::CopyPicture(YV12Image* pImage, DVDVideoPicture *pSrc)
{
  BYTE *s = pSrc->data[0];
  BYTE *d = pImage->plane[0];
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;
  if ((w == pSrc->iLineSize[0]) && ((unsigned int) pSrc->iLineSize[0] == pImage->stride[0]))
  {
    fast_memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      fast_memcpy(d, s, w);
      s += pSrc->iLineSize[0];
      d += pImage->stride[0];
    }
  }
  s = pSrc->data[1];
  d = pImage->plane[1];
  w = pSrc->iWidth >> 1;
  h = pSrc->iHeight >> 1;
  if ((w==pSrc->iLineSize[1]) && ((unsigned int) pSrc->iLineSize[1]==pImage->stride[1]))
  {
    fast_memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      fast_memcpy(d, s, w);
      s += pSrc->iLineSize[1];
      d += pImage->stride[1];
    }
  }
  s = pSrc->data[2];
  d = pImage->plane[2];
  if ((w==pSrc->iLineSize[2]) && ((unsigned int) pSrc->iLineSize[2]==pImage->stride[2]))
  {
    fast_memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      fast_memcpy(d, s, w);
      s += pSrc->iLineSize[2];
      d += pImage->stride[2];
    }
  }
  return true;
}

DVDVideoPicture* CDVDCodecUtils::ConvertToNV12Picture(DVDVideoPicture *pSrc)
{
  // Clone a YV12 picture to new NV12 picture.
  DVDVideoPicture* pPicture = new DVDVideoPicture;
  if (pPicture)
  {
    *pPicture = *pSrc;

    int w = pPicture->iWidth / 2;
    int h = pPicture->iHeight / 2;
    int size = w * h;
    int totalsize = (pPicture->iWidth * pPicture->iHeight) + size * 2;
    BYTE* data = new BYTE[totalsize];
    if (data)
    {
      pPicture->data[0] = data;
      pPicture->data[1] = pPicture->data[0] + (pPicture->iWidth * pPicture->iHeight);
      pPicture->data[2] = NULL;
      pPicture->data[3] = NULL;
      pPicture->iLineSize[0] = pPicture->iWidth;
      pPicture->iLineSize[1] = pPicture->iWidth;
      pPicture->iLineSize[2] = 0;
      pPicture->iLineSize[3] = 0;
      pPicture->format = DVDVideoPicture::FMT_NV12;
      
      // copy luma
      uint8_t *s = pSrc->data[0];
      uint8_t *d = pPicture->data[0];
      for (int y = 0; y < pSrc->iHeight; y++)
      {
        fast_memcpy(d, s, pSrc->iWidth);
        s += pSrc->iLineSize[0];
        d += pPicture->iLineSize[0];
      }

      //copy chroma
      uint8_t *s_u, *s_v, *d_uv;
      for (int y = 0; y < pSrc->iHeight/2; y++) {
        s_u = pSrc->data[1] + (y * pSrc->iLineSize[1]);
        s_v = pSrc->data[2] + (y * pSrc->iLineSize[2]);
        d_uv = pPicture->data[1] + (y * pPicture->iLineSize[1]);
        for (int x = 0; x < pSrc->iWidth/2; x++) {
          *d_uv++ = *s_u++;
          *d_uv++ = *s_v++;
        }
      }
      
    }
    else
    {
      CLog::Log(LOGFATAL, "CDVDCodecUtils::AllocateNV12Picture, unable to allocate new video picture, out of memory.");
      delete pPicture;
      pPicture = NULL;
    }
  }
  return pPicture;
}

bool CDVDCodecUtils::CopyNV12Picture(YV12Image* pImage, DVDVideoPicture *pSrc)
{
  BYTE *s = pSrc->data[0];
  BYTE *d = pImage->plane[0];
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;
  // Copy Y
  if ((w == pSrc->iLineSize[0]) && ((unsigned int) pSrc->iLineSize[0] == pImage->stride[0]))
  {
    fast_memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      fast_memcpy(d, s, w);
      s += pSrc->iLineSize[0];
      d += pImage->stride[0];
    }
  }
  
  s = pSrc->data[1];
  d = pImage->plane[1];
  w = pSrc->iWidth;
  h = pSrc->iHeight >> 1;
  // Copy packed UV (width is same as for Y as it's both U and V components)
  if ((w==pSrc->iLineSize[1]) && ((unsigned int) pSrc->iLineSize[1]==pImage->stride[1]))
  {
    fast_memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      fast_memcpy(d, s, w);
      s += pSrc->iLineSize[1];
      d += pImage->stride[1];
    }
  }

  return true;
}
