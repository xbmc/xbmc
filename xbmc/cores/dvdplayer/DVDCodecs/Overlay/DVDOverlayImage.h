
#pragma once
#include "DVDOverlay.h"

class CDVDOverlayImage : public CDVDOverlay
{
public:
  CDVDOverlayImage() : CDVDOverlay(DVDOVERLAY_TYPE_IMAGE)
  {
    data = NULL;
    palette = NULL;
    linesize = NULL;
    x = NULL;
    y = NULL;
    width = NULL;
    height = NULL;
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
