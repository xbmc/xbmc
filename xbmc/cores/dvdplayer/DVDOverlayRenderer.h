
#pragma once

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"

#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif

class CDVDOverlayImage;

typedef struct stDVDPictureRenderer
{
  BYTE* data[4];
  int stride[4];
}
DVDPictureRenderer;

class CDVDOverlayRenderer
{
public:
  static void Render(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlay);
  static void Render(DVDPictureRenderer* pPicture, CDVDOverlayImage* pOverlay);

  static void Render(YV12Image* pImage, CDVDOverlay* pOverlay)
  {
    DVDPictureRenderer p;
    
    p.data[0] = pImage->plane[0];
    p.data[1] = pImage->plane[1];
    p.data[2] = pImage->plane[2];
    p.data[3] = NULL;
    
    p.stride[0] = pImage->stride[0];
    p.stride[1] = pImage->stride[1];
    p.stride[2] = pImage->stride[2];
    p.stride[3] = 0;
    
    Render(&p, pOverlay);
  }
  
  static void Render(DVDVideoPicture* pPicture, CDVDOverlay* pOverlay)
  {
    DVDPictureRenderer p;
    
    p.data[0] = pPicture->data[0];
    p.data[1] = pPicture->data[1];
    p.data[2] = pPicture->data[2];
    p.data[3] = pPicture->data[3];
    
    p.stride[0] = pPicture->iLineSize[0];
    p.stride[1] = pPicture->iLineSize[1];
    p.stride[2] = pPicture->iLineSize[2];
    p.stride[3] = pPicture->iLineSize[3];
    
    Render(&p, pOverlay);
  }

private:

  static void Render_SPU_YUV(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlaySpu, bool bCrop);
};
