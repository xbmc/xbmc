#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFormats.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

struct YV12Image;

class CDVDCodecUtils
{
public:
  static VideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(VideoPicture* pPicture);
  static bool CopyPicture(VideoPicture* pDst, VideoPicture* pSrc);
  static bool CopyPicture(YV12Image* pDst, VideoPicture *pSrc);
  
  static VideoPicture* ConvertToNV12Picture(VideoPicture *pSrc);
  static VideoPicture* ConvertToYUV422PackedPicture(VideoPicture *pSrc, ERenderFormat format);
  static bool CopyNV12Picture(YV12Image* pImage, VideoPicture *pSrc);
  static bool CopyYUV422PackedPicture(YV12Image* pImage, VideoPicture *pSrc);

  static bool IsVP3CompatibleWidth(int width);

  static double NormalizeFrameduration(double frameduration, bool *match = NULL);

  static ERenderFormat EFormatFromPixfmt(int fmt);
  static AVPixelFormat PixfmtFromEFormat(ERenderFormat format);
};

