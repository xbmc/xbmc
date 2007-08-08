
#pragma once

#include "Video/DVDVideoCodec.h"

struct YV12Image;

class CDVDCodecUtils
{
public:
  static DVDVideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(DVDVideoPicture* pPicture);
  static bool CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc);
  static bool CopyPictureToOverlay(YV12Image* pImage, DVDVideoPicture *pSrc);
};

