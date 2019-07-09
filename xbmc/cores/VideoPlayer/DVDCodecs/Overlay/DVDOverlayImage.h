/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlay.h"

#include <stdlib.h>
#include <string.h>

#include "PlatformDefs.h"

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
    data    = (uint8_t*)malloc(src.linesize * src.height);
    memcpy(data, src.data, src.linesize * src.height);

    if(src.palette)
    {
      palette = (uint32_t*)malloc(src.palette_colors * 4);
      memcpy(palette, src.palette, src.palette_colors * 4);
    }
    else
      palette = NULL;

    palette_colors = src.palette_colors;
    linesize       = src.linesize;
    x              = src.x;
    y              = src.y;
    width          = src.width;
    height         = src.height;
    source_width   = src.source_width;
    source_height  = src.source_height;

  }

  CDVDOverlayImage(const CDVDOverlayImage& src, int sub_x, int sub_y, int sub_w, int sub_h)
  : CDVDOverlay(src)
  {
    int bpp;
    if(src.palette)
    {
      bpp = 1;
      palette = (uint32_t*)malloc(src.palette_colors * 4);
      memcpy(palette, src.palette, src.palette_colors * 4);
    }
    else
    {
      bpp = 4;
      palette = NULL;
    }

    palette_colors = src.palette_colors;
    linesize       = sub_w * bpp;
    x              = sub_x;
    y              = sub_y;
    width          = sub_w;
    height         = sub_h;
    source_width   = src.source_width;
    source_height  = src.source_height;

    data = (uint8_t*)malloc(sub_h * linesize);

    uint8_t* s = src.data_at(sub_x, sub_y);
    uint8_t* t = data;

    for(int row = 0;row < sub_h; ++row)
    {
      memcpy(t, s, linesize);
      s += src.linesize;
      t += linesize;
    }

    m_textureid = 0;
  }

  ~CDVDOverlayImage() override
  {
    if(data) free(data);
    if(palette) free(palette);
  }

  CDVDOverlayImage* Clone() override
  {
    return new CDVDOverlayImage(*this);
  }

  uint8_t* data_at(int sub_x, int sub_y) const
  {
    int bpp;
    if(palette)
      bpp = 1;
    else
      bpp = 4;
    return &data[(sub_y - y)*linesize +
                 (sub_x - x)*bpp];
  }

  uint8_t*  data;
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
