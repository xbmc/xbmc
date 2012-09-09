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

#include "DVDOverlay.h"
#include <string.h>
#include <stdlib.h>

class CDVDOverlayImage : public CDVDOverlay
{
public:
  CDVDOverlayImage() : CDVDOverlay(DVDOVERLAY_TYPE_IMAGE)
  {
    data = NULL;
    palette = NULL;
    palette_colors = 0;
    linesize = 0;
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    source_width  = 0;
    source_height = 0;
  }

  CDVDOverlayImage(const CDVDOverlayImage& src)
    : CDVDOverlay(src)
  {
    data    = (BYTE*)malloc(src.linesize * src.height);
    memcpy(data, src.data, src.linesize * src.height);

    palette = (uint32_t*)malloc(src.palette_colors * 4);
    memcpy(palette, src.palette, src.palette_colors * 4);

    palette_colors = src.palette_colors;
    linesize       = src.linesize;
    x              = src.x;
    y              = src.y;
    width          = src.width;
    height         = src.height;
    source_width   = src.source_width;
    source_height  = src.source_height;

  }

  ~CDVDOverlayImage()
  {
    if(data) free(data);
    if(palette) free(palette);
  }

  BYTE*  data;
  int    linesize;

  uint32_t* palette;
  int    palette_colors;

  int    x;
  int    y;
  int    width;
  int    height;
  int    source_width;
  int    source_height;
};
