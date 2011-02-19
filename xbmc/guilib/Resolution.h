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

#pragma once

#include <stdint.h>
#include "utils/StdString.h"

typedef int DisplayMode;
#define DM_WINDOWED     -1
#define DM_FULLSCREEN1   0
#define DM_FULLSCREEN2   1
// DM_FULLSCREEN3        2
// etc.

enum RESOLUTION {
  RES_INVALID        = -1,
  RES_HDTV_1080i     =  0,
  RES_HDTV_720p      =  1,
  RES_HDTV_480p_4x3  =  2,
  RES_HDTV_480p_16x9 =  3,
  RES_NTSC_4x3       =  4,
  RES_NTSC_16x9      =  5,
  RES_PAL_4x3        =  6,
  RES_PAL_16x9       =  7,
  RES_PAL60_4x3      =  8,
  RES_PAL60_16x9     =  9,
  RES_AUTORES        = 10,
  RES_WINDOW         = 11,
  RES_DESKTOP        = 12,          // Desktop resolution for primary screen
  RES_CUSTOM         = 12 + 1,      // Desktop resolution for screen #2
//                     ...
//                     12 + N - 1   // Desktop resolution for screen N
//                     12 + N       // First additional resolution, in a N screen configuration.
//                     12 + N + ... // Other resolutions, in any order
};

enum VSYNC {
  VSYNC_DISABLED     =  0,
  VSYNC_VIDEO        =  1,
  VSYNC_ALWAYS       =  2,
  VSYNC_DRIVER       =  3
};

struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
public:
  OVERSCAN()
  {
    left = top = right = bottom = 0;
  }
  OVERSCAN(const OVERSCAN& os)
  {
    left = os.left; top = os.top;
    right = os.right; bottom = os.bottom;
  }
};

struct RESOLUTION_INFO
{
  OVERSCAN Overscan;
  bool bFullScreen;
  int iScreen;
  int iWidth;
  int iHeight;
  int iSubtitles;
  uint32_t dwFlags;
  float fPixelRatio;
  float fRefreshRate;
  CStdString strMode;
  CStdString strOutput;
  CStdString strId;
public:
  RESOLUTION_INFO(int width = 1280, int height = 720, float aspect = 0, const CStdString &mode = "")
  {
    iWidth = width;
    iHeight = height;
    fPixelRatio = aspect ? ((float)width)/height / aspect : 1.0f;
    strMode = mode;
    bFullScreen = true;
    fRefreshRate = 0;
    dwFlags = iSubtitles = iScreen = 0;
  }
  float DisplayRatio() const
  {
    return iWidth * fPixelRatio / iHeight;
  }
  RESOLUTION_INFO(const RESOLUTION_INFO& res)
  {
    Overscan = res.Overscan; bFullScreen = res.bFullScreen;
    iScreen = res.iScreen; iWidth = res.iWidth; iHeight = res.iHeight;
    iSubtitles = res.iSubtitles; dwFlags = res.dwFlags;
    fPixelRatio = res.fPixelRatio; fRefreshRate = res.fRefreshRate;
    strMode = res.strMode; strOutput = res.strOutput; strId = res.strId;
  }
};
