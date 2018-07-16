/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
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
  bool      convert_quad(ASS_Image* images, SQuads& quads, int max_x);
  int       GetStereoscopicDepth();

}
