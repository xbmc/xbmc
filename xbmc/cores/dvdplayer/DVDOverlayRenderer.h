
#pragma once

#include "DVDCodecs\Video\DVDVideoCodec.h"
#include "..\VideoRenderers\XBoxRenderer.h" // YV12Image definition
#include "DVDOverlay.h"

class CDVDOverlayRenderer
{
public:
  static void Render(YV12Image* pImage, CDVDOverlay* pOverlay);
  
  static void RenderI420(DVDVideoPicture* pPicture, CDVDOverlaySpu* pOverlay, bool bCrop);
  static void Render_SPU_YUV(YV12Image* pImage, CDVDOverlaySpu* pOverlay, bool bCrop);
};
