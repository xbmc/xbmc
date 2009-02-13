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

 
#include "stdafx.h"
#include "DVDCodecUtils.h"
#include "cores/VideoRenderers/RenderManager.h"

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
    memcpy(d, s, w);
    s += pSrc->iLineSize[0];
    d += pDst->iLineSize[0];
  }

  w >>= 1;
  h >>= 1;

  s = pSrc->data[1];
  d = pDst->data[1];
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[1];
    d += pDst->iLineSize[1];
  }

  s = pSrc->data[2];
  d = pDst->data[2];
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
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
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
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
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[1];
      d += pImage->stride[1];
    }
  }
  s = pSrc->data[2];
  d = pImage->plane[2];
  if ((w==pSrc->iLineSize[2]) && ((unsigned int) pSrc->iLineSize[2]==pImage->stride[2]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[2];
      d += pImage->stride[2];
    }
  }
  return true;
}
