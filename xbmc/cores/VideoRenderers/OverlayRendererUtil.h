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
#pragma once

#include <stdlib.h>

class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

namespace OVERLAY {

  struct SQuad
  {
     int           u, v;
     unsigned char r, g, b, a;
     int           x, y;
     int           w, h;
  };

  struct SQuads
  {
    SQuads()
    {
      data = NULL;
      quad = NULL;
      size_x = 0;
      size_y = 0;
      count  = 0;
    }
   ~SQuads()
    {
      free(data);
      free(quad);
    }
    int      size_x;
    int      size_y;
    int      count;
    uint8_t* data;
    SQuad*   quad;
  };

  uint32_t* convert_rgba(CDVDOverlayImage* o, bool mergealpha);
  uint32_t* convert_rgba(CDVDOverlaySpu*   o, bool mergealpha
                       , int& min_x, int& max_x
                       , int& min_y, int& max_y);
  bool      convert_quad(ASS_Image* images, SQuads& quads);

}
