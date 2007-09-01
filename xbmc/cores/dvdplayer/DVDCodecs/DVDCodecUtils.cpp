
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
  if (pDst->iWidth != pSrc->iWidth || pDst->iHeight != pSrc->iHeight) return false;

  int size_y = (pSrc->iWidth * pSrc->iHeight);
  int size_u = pSrc->iWidth / 2 * pSrc->iHeight / 2;
  int size_v = size_u;

  memcpy(pDst->data[0], pSrc->data[0], size_y);
  memcpy(pDst->data[1], pSrc->data[1], size_u);
  memcpy(pDst->data[2], pSrc->data[2], size_v);

  return true;
}

bool CDVDCodecUtils::CopyPictureToOverlay(YV12Image* pImage, DVDVideoPicture *pSrc)
{
  BYTE *s = pSrc->data[0];
  BYTE *d = pImage->plane[0];
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[0];
    d += pImage->stride[0];
  }
  s = pSrc->data[1];
  d = pImage->plane[1];
  w = pSrc->iWidth >> 1;
  h = pSrc->iHeight >> 1;
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[1];
    d += pImage->stride[1];
  }
  s = pSrc->data[2];
  d = pImage->plane[2];
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[2];
    d += pImage->stride[2];
  }
  return true;
}
