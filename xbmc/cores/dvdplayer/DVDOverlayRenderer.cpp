
#include "stdafx.h"
#include "DVDOverlayRenderer.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"


void CDVDOverlayRenderer::Render(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlay)
{
  if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_SPU))
  {
    // display subtitle, if bForced is true, it's a menu overlay and we should crop it
    Render_SPU_YUV(pPicture, pOverlay, pOverlay->bForced);
  }
  else if (false && pOverlay->IsOverlayType(DVDOVERLAY_TYPE_TEXT))
  {
    CDVDOverlayText* pOverlayText = (CDVDOverlayText*)pOverlay;
    
    //CLog::Log(LOGDEBUG, " - s: %i, e: %i", (int)(pOverlayText->iPTSStartTime / 1000), (int)(pOverlayText->iPTSStopTime / 1000));
    
    CDVDOverlayText::CElement* e = pOverlayText->m_pHead;
    while (e)
    {
      if (e->IsElementType(CDVDOverlayText::ELEMENT_TYPE_TEXT))
      {
        CDVDOverlayText::CElementText* t = (CDVDOverlayText::CElementText*)e;
        CLog::Log(LOGDEBUG, " - %s", t->m_text);
      }
      e = e->pNext;
    }
  }
}

// render the parsed sub (parsed rle) onto the yuv image
void CDVDOverlayRenderer::Render_SPU_YUV(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlaySpu, bool bCrop)
{
  CDVDOverlaySpu* pOverlay = (CDVDOverlaySpu*)pOverlaySpu;
  
  unsigned __int8*  p_destptr = NULL;
  unsigned __int16* p_source = (unsigned __int16*)pOverlay->pData;
  unsigned __int8*  p_dest[3];

  int i_x, i_y;
  int rp_len, i_color, pixels_to_draw;
  unsigned __int16 i_colprecomp, i_destalpha;
  
  int btn_x_start = pOverlay->crop_i_x_start;
  int btn_x_end   = pOverlay->crop_i_x_end;
  int btn_y_start = pOverlay->crop_i_y_start;
  int btn_y_end   = pOverlay->crop_i_y_end;
  
  int *p_color;
  int p_alpha;

  p_dest[0] = pPicture->data[0] + pPicture->stride[0] * pOverlay->y;
  p_dest[1] = pPicture->data[1] + pPicture->stride[1] * (pOverlay->y >> 1);
  p_dest[2] = pPicture->data[2] + pPicture->stride[2] * (pOverlay->y >> 1);

  /* Draw until we reach the bottom of the subtitle */
  for (i_y = pOverlay->y; i_y < pOverlay->y + pOverlay->height; i_y++)
  {
    /* Draw until we reach the end of the line */
    for (i_x = pOverlay->x; i_x < pOverlay->x + pOverlay->width ; i_x += rp_len)
    {
      /* Get the RLE part, then draw the line */
      i_color = *p_source & 0x3;
      rp_len = *p_source++ >> 2;

      while( rp_len > 0 )
      {      
        pixels_to_draw = rp_len;

        p_color = pOverlay->color[i_color];
        p_alpha = pOverlay->alpha[i_color];

        if (bCrop)
        {
          if (i_y > btn_y_start && i_y < btn_y_end)
          {            
            if (i_x < btn_x_start && i_x + rp_len >= btn_x_start) // starts outside
              pixels_to_draw = btn_x_start - i_x;
            else if( i_x >= btn_x_start && i_x <= btn_x_end ) // starts inside
            {
              p_color = pOverlay->highlight_color[i_color];
              p_alpha = pOverlay->highlight_alpha[i_color];              
              pixels_to_draw = btn_x_end - i_x + 1; // don't draw part that is outside
            }
          }
          /* make sure we are not requested to draw to far */
          /* that part will be taken care of in next pass */
          if( pixels_to_draw > rp_len ) 
            pixels_to_draw = rp_len;
        }
        
        switch (p_alpha)
        {
        case 0x00:
          break;

        case 0x0f:
          memset(p_dest[0] + i_x, p_color[0], pixels_to_draw);
          if (!(i_y & 1)) // Only draw even lines
          {
            memset(p_dest[1] + (i_x >> 1), p_color[2], pixels_to_draw >> 1);
            memset(p_dest[2] + (i_x >> 1), p_color[1], pixels_to_draw >> 1);
          }
          break;

        default:
          /* To be able to divide by 16 (>>4) we add 1 to the alpha.
            * This means Alpha 0 won't be completely transparent, but
            * that's handled in a special case above anyway. */ 
          // First we deal with Y
          i_colprecomp = (unsigned __int16)p_color[0]
                        * (unsigned __int16)(p_alpha + 1);
          i_destalpha = 15 - p_alpha;
          
          for (p_destptr = p_dest[0] + i_x; p_destptr < p_dest[0] + i_x + pixels_to_draw; p_destptr++)
          {
            *p_destptr = (( i_colprecomp + (unsigned __int16) * p_destptr * i_destalpha ) >> 4) & 0xFF;
          }
          
          if (!(i_y & 1)) // Only draw even lines
          {
            // now U
            i_colprecomp = (unsigned __int16)p_color[2]
                          * (unsigned __int16)(p_alpha + 1);
            for ( p_destptr = p_dest[1] + (i_x >> 1); p_destptr < p_dest[1] + ((i_x + pixels_to_draw) >> 1); p_destptr++)
            {
              *p_destptr = (( i_colprecomp + (unsigned __int16) * p_destptr * i_destalpha ) >> 4) & 0xFF;
            }
            // and finally V
            i_colprecomp = (unsigned __int16)p_color[1]
                          * (unsigned __int16)(p_alpha + 1);
            for ( p_destptr = p_dest[2] + (i_x >> 1); p_destptr < p_dest[2] + ((i_x + pixels_to_draw) >> 1); p_destptr++)
            {
              *p_destptr = (( i_colprecomp + (unsigned __int16) * p_destptr * i_destalpha ) >> 4) & 0xFF;
            }
          }
          break;
        }

        /* add/subtract what we just drew */
        rp_len -= pixels_to_draw;
        i_x += pixels_to_draw;
      }
    }

    p_dest[0] += pPicture->stride[0];
    if (i_y & 1)
      continue;
    p_dest[1] += pPicture->stride[1];
    p_dest[2] += pPicture->stride[2];
  }
}
