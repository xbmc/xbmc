
#pragma once
#include "DVDOverlay.h"

class CDVDOverlayImage : public CDVDOverlay
{
public:
  CDVDOverlayImage() : CDVDOverlay(DVDOVERLAY_TYPE_IMAGE)
  {
    data = NULL;
    palette = NULL;
    linesize = 0;
    x = 0;
    y = 0;
    width = 0;
    height = 0;
  }

  ~CDVDOverlayImage()
  {
    if(data) free(data);
    if(palette) free(palette);
  }

  BYTE*  data;
  int    linesize;

  DWORD* palette;
  int    palette_colors;

  int    x;
  int    y;
  int    width;
  int    height;
};
