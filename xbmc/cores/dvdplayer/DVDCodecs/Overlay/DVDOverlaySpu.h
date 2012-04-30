#pragma once

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

#include "DVDOverlay.h"

class CDVDOverlaySpu : public CDVDOverlay
{
public:
  CDVDOverlaySpu() : CDVDOverlay(DVDOVERLAY_TYPE_SPU)
  {
    pTFData = 0;
    pBFData = 0;
    x = 0;
    y = 0;
    width = 0;
    height = 0;

    crop_i_x_end = 0;
    crop_i_y_end = 0;
    crop_i_x_start = 0;
    crop_i_y_start = 0;

    bHasColor = false;
    bHasAlpha = false;

    memset(result, 0, sizeof(result));
    memset(alpha, 0, sizeof(alpha));
    memset(color, 0, sizeof(color));
    memset(highlight_alpha, 0, sizeof(highlight_alpha));
    memset(highlight_color, 0, sizeof(highlight_color));
  }

  CDVDOverlaySpu(const CDVDOverlaySpu& src)
    : CDVDOverlay(src)
  {
    pTFData = src.pTFData;
    pBFData = src.pBFData;
    x       = src.x;
    y       = src.y;
    width   = src.width;
    height  = src.height;

    crop_i_x_end   = src.crop_i_x_end;
    crop_i_y_end   = src.crop_i_y_end;
    crop_i_x_start = src.crop_i_x_start;
    crop_i_y_start = src.crop_i_y_start;

    bHasColor = src.bHasColor;
    bHasAlpha = src.bHasAlpha;

    memcpy(result         , src.result         , sizeof(result));
    memcpy(alpha          , src.alpha          , sizeof(alpha));
    memcpy(color          , src.color          , sizeof(color));
    memcpy(highlight_alpha, src.highlight_alpha, sizeof(highlight_alpha));
    memcpy(highlight_color, src.highlight_color, sizeof(highlight_color));
  }

  BYTE result[2*65536 + 20]; // rle data
  int pTFData; // pointer to top field picture data (needs rle parsing)
  int pBFData; // pointer to bottom field picture data (needs rle parsing)
  int x;
  int y;
  int width;
  int height;

  // the four contrasts, [0] = background
  int alpha[4];
  bool bHasAlpha;

  // the four yuv colors, containing [][0] = Y, [][1] = Cr, [][2] = Cb
  // [0][] = background, [1][] = pattern, [2][] = emphasis1, [3][] = emphasis2
  int color[4][3];
  bool bHasColor;

  // used for cropping overlays
  int crop_i_x_end;
  int crop_i_y_end;
  int crop_i_x_start;
  int crop_i_y_start;

  // provided by the navigator engine
  // should be used on the highlighted areas
  int highlight_color[4][3];
  int highlight_alpha[4];
};
