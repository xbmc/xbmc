/*
*      Copyright (C) 2005-2015 Team XBMC
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

#pragma once

#include <stdint.h>
#include <string>

typedef int DisplayMode;
#define DM_WINDOWED     -1
#define DM_FULLSCREEN1   0
#define DM_FULLSCREEN2   1
// DM_FULLSCREEN3        2
// etc.

enum RESOLUTION {
  RES_INVALID        = -1,
  RES_HDTV_1080i     =  0,
  RES_HDTV_720pSBS   =  1,
  RES_HDTV_720pTB    =  2,
  RES_HDTV_1080pSBS  =  3,
  RES_HDTV_1080pTB   =  4,
  RES_HDTV_720p      =  5,
  RES_HDTV_480p_4x3  =  6,
  RES_HDTV_480p_16x9 =  7,
  RES_NTSC_4x3       =  8,
  RES_NTSC_16x9      =  9,
  RES_PAL_4x3        = 10,
  RES_PAL_16x9       = 11,
  RES_PAL60_4x3      = 12,
  RES_PAL60_16x9     = 13,
  RES_AUTORES        = 14,
  RES_WINDOW         = 15,
  RES_DESKTOP        = 16,          // Desktop resolution for primary screen
  RES_CUSTOM         = 16 + 1,      // Desktop resolution for screen #2
//                     ...
//                     12 + N - 1   // Desktop resolution for screen N
//                     12 + N       // First additional resolution, in a N screen configuration.
//                     12 + N + ... // Other resolutions, in any order
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
  int iBlanking; /**< number of pixels of padding between stereoscopic frames */
  int iScreenWidth;
  int iScreenHeight;
  int iSubtitles;
  uint32_t dwFlags;
  float fPixelRatio;
  float fRefreshRate;
  std::string strMode;
  std::string strOutput;
  std::string strId;
public:
  RESOLUTION_INFO(int width = 1280, int height = 720, float aspect = 0, const std::string &mode = "");
  float DisplayRatio() const;
  RESOLUTION_INFO(const RESOLUTION_INFO& res);
};

class CResolutionUtils
{
public:
  static RESOLUTION ChooseBestResolution(float fps, int width, bool is3D);
protected:
  static bool FindResolutionFromOverride(float fps, int width, bool is3D, RESOLUTION &resolution, float& weight, bool fallback);
  static void FindResolutionFromFpsMatch(float fps, int width, bool is3D, RESOLUTION &resolution, float& weight);
  static RESOLUTION FindClosestResolution(float fps, int width, bool is3D, float multiplier, RESOLUTION current, float& weight);
  static float RefreshWeight(float refresh, float fps);
};
