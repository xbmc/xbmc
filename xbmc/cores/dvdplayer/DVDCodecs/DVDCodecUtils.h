#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Video/DVDVideoCodec.h"
#include "cores/VideoRenderers/RenderFormats.h"

struct YV12Image;

class CDVDCodecUtils
{
public:
  static DVDVideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(DVDVideoPicture* pPicture);
  static bool CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc);
  static bool CopyPicture(YV12Image* pDst, DVDVideoPicture *pSrc);
  
  static DVDVideoPicture* ConvertToNV12Picture(DVDVideoPicture *pSrc);
  static DVDVideoPicture* ConvertToYUV422PackedPicture(DVDVideoPicture *pSrc, ERenderFormat format);
  static bool CopyNV12Picture(YV12Image* pImage, DVDVideoPicture *pSrc);
  static bool CopyYUV422PackedPicture(YV12Image* pImage, DVDVideoPicture *pSrc);
  static bool CopyDXVA2Picture(YV12Image* pImage, DVDVideoPicture *pSrc);

  static bool IsVP3CompatibleWidth(int width);

  static double NormalizeFrameduration(double frameduration);

  static ERenderFormat EFormatFromPixfmt(int fmt);
  static int           PixfmtFromEFormat(ERenderFormat format);
};

