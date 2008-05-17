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
    pData = NULL;
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
  
    memset(alpha, 0, sizeof(alpha));
    memset(color, 0, sizeof(color));
    memset(stats, 0, sizeof(stats));
    memset(highlight_alpha, 0, sizeof(highlight_alpha));
    memset(highlight_color, 0, sizeof(highlight_color));
  }

  BYTE* pData; // rle data
  int pTFData; // pointer to top field picture data (needs rle parsing)
  int pBFData; // pointer to bottom field picture data (needs rle parsing)
  int x;
  int y;
  int width;
  int height;

  int stats[4]; // nr of pixels for each color found in the overlay.
  
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

  bool CanDisplayWithAlphas(int a[4])
  {
    return(
      a[0] * stats[0] > 0 ||
      a[1] * stats[1] > 0 ||
      a[2] * stats[2] > 0 ||
      a[3] * stats[3] > 0);
  }
};
