
#pragma once

#include "DVDVideoCodec.h"
#include "../../VideoRenderers/XBoxRenderer.h" // for YV12Image definition

class CDVDCodecUtils
{
public:
  static DVDVideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(DVDVideoPicture* pPicture);
  static bool CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc);
  static bool CopyPictureToOverlay(YV12Image* pImage, DVDVideoPicture *pSrc);
};

