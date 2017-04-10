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

struct YuvImage;

class CDVDCodecUtils
{
public:
  static bool CopyPicture(YuvImage* pDst, VideoPicture *pSrc);
  static bool CopyNV12Picture(YuvImage* pImage, VideoPicture *pSrc);
  static bool CopyYUV422PackedPicture(YuvImage* pImage, VideoPicture *pSrc);

  static bool IsVP3CompatibleWidth(int width);

  static double NormalizeFrameduration(double frameduration, bool *match = NULL);

  static ERenderFormat EFormatFromPixfmt(int fmt);
  static AVPixelFormat PixfmtFromEFormat(ERenderFormat format);
};

