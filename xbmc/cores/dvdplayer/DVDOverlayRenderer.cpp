
#include "../../stdafx.h"
#include "DVDOverlayRenderer.h"

void CDVDOverlayRenderer::Render(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlay)
{
  if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_SPU))
  {
    // display subtitle, if bForced is true, it's a menu overlay and we should crop it
    Render_SPU_YUV(pPicture, (CDVDOverlaySpu*)pOverlay, pOverlay->bForced);
  }
  else if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_TEXT))
  {
    CDVDOverlayText* pOverlayText = (CDVDOverlayText*)pOverlay;
    
    //CLog::DebugLog(" - s: %i, e: %i", (int)(pOverlayText->iPTSStartTime / 1000), (int)(pOverlayText->iPTSStopTime / 1000));
    
    CDVDOverlayText::CElement* e = pOverlayText->m_pHead;
    while (e)
    {
      if (e->IsElementType(CDVDOverlayText::ELEMENT_TYPE_TEXT))
      {
        CDVDOverlayText::CElementText* t = (CDVDOverlayText::CElementText*)e;
        CLog::DebugLog(" - %S", t->m_wszText);
      }
      e = e->pNext;
    }
    CLog::DebugLog("");
  }
}

// render the parsed sub (parsed rle) onto the yuv image
void CDVDOverlayRenderer::Render_SPU_YUV(DVDPictureRenderer* pPicture, CDVDOverlaySpu* pOverlay, bool bCrop)
{
  unsigned __int8* p_dest[3];
  unsigned __int8* p_destptr;
  unsigned __int16* p_source = (unsigned __int16*)pOverlay->pData;

  int i_x, i_y;
  int i_len, i_color;
  unsigned __int16 i_colprecomp, i_destalpha;

  int button_i_x_start = pOverlay->crop_i_x_start;
  int button_i_x_end = pOverlay->crop_i_x_end;
  int button_i_y_start = pOverlay->crop_i_y_start;
  int button_i_y_end = pOverlay->crop_i_y_end;
  
  p_dest[0] = pPicture->data[0] + pOverlay->y * pPicture->stride[0];
  p_dest[1] = pPicture->data[1] + (pOverlay->y >> 1) * pPicture->stride[1];
  p_dest[2] = pPicture->data[2] + (pOverlay->y >> 1) * pPicture->stride[2];;

  /* Draw until we reach the bottom of the subtitle */
  for ( i_y = pOverlay->y; i_y < pOverlay->y + pOverlay->height; i_y ++ )
  {
    /* Draw until we reach the end of the line */
    for (i_x = pOverlay->x; i_x < pOverlay->x + pOverlay->width ; i_x += i_len)
    {
      /* Get the RLE part, then draw the line */
      i_color = *p_source & 0x3;
      i_len = *p_source++ >> 2;

      // skip drawing if we are outside the crop region
      if (bCrop && ( i_x < button_i_x_start || i_x > button_i_x_end
                    || i_y < button_i_y_start || i_y > button_i_y_end ) )
      {
        continue;
      }

      switch (pOverlay->alpha[i_color])
      {
      case 0x00:
        break;

      case 0x0f:
        memset(p_dest[0] + i_x, pOverlay->color[i_color][0], i_len);
        if (i_y & 1) continue; // Only draw even lines
        memset(p_dest[1] + (i_x >> 1), pOverlay->color[i_color][2], i_len >> 1);
        memset(p_dest[2] + (i_x >> 1), pOverlay->color[i_color][1], i_len >> 1);
        break;

      default:
        /* To be able to divide by 16 (>>4) we add 1 to the alpha.
          * This means Alpha 0 won't be completely transparent, but
          * that's handled in a special case above anyway. */ 
        // First we deal with Y
        i_colprecomp = (unsigned __int16)pOverlay->color[i_color][0]
                      * (unsigned __int16)(pOverlay->alpha[i_color] + 1);
        i_destalpha = 15 - pOverlay->alpha[i_color];
        for ( p_destptr = p_dest[0] + i_x; p_destptr < p_dest[0] + i_x + i_len; p_destptr++)
        {
          *p_destptr = (( i_colprecomp + (unsigned __int16) * p_destptr * i_destalpha ) >> 4) & 0xFF;
        }
        if (i_y & 1) continue; // Only draw even lines
        // now U
        i_colprecomp = (unsigned __int16)pOverlay->color[i_color][2]
                      * (unsigned __int16)(pOverlay->alpha[i_color] + 1);
        for ( p_destptr = p_dest[1] + (i_x >> 1); p_destptr < p_dest[1] + ((i_x + i_len) >> 1); p_destptr++)
        {
          *p_destptr = (( i_colprecomp + (unsigned __int16) * p_destptr * i_destalpha ) >> 4) & 0xFF;
        }
        // and finally V
        i_colprecomp = (unsigned __int16)pOverlay->color[i_color][1]
                      * (unsigned __int16)(pOverlay->alpha[i_color] + 1);
        for ( p_destptr = p_dest[2] + (i_x >> 1); p_destptr < p_dest[2] + ((i_x + i_len) >> 1); p_destptr++)
        {
          *p_destptr = (( i_colprecomp + (unsigned __int16) * p_destptr * i_destalpha ) >> 4) & 0xFF;
        }
        break;
      }
    }
    p_dest[0] += pPicture->stride[0];
    if (i_y & 1)
      continue;
    p_dest[1] += pPicture->stride[1];
    p_dest[2] += pPicture->stride[2];
  }
}