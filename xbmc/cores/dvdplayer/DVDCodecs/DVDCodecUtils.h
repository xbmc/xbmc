
#pragma once

#include "DVDVideoCodec.h"

class CDVDCodecUtils
{
public:
  static DVDVideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(DVDVideoPicture* pPicture);
  static bool CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc);
};

