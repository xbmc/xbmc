
#pragma once

#include "DVDVideoCodec.h"
#include "../DVDVideo.h"

class CDVDCodecUtils
{
public:
  static DVDVideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(DVDVideoPicture* pPicture);
  static bool CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc);
  static bool CopyPictureToOverlay(YUVOverlay* pOverlay, DVDVideoPicture *pSrc);
};

