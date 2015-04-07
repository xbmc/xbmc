/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "utils/log.h"
#include "DVDOverlayRenderer.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDCodecs/Overlay/DVDOverlayImage.h"
#include "DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoRenderers/OverlayRendererUtil.h"

#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))


void CDVDOverlayRenderer::Render(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlay, double pts)
{
  if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_SPU))
  {
    // display subtitle, if bForced is true, it's a menu overlay and we should crop it
    Render_SPU_YUV(pPicture, pOverlay, pOverlay->bForced);
  }
  else if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
  {
    Render(pPicture, (CDVDOverlayImage*)pOverlay);
  }
  else if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_SSA))
  {
    Render(pPicture, (CDVDOverlaySSA*)pOverlay, pts);
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
        CLog::Log(LOGDEBUG, " - %s", t->GetTextPtr());
      }
      e = e->pNext;
    }
  }
}


void CDVDOverlayRenderer::Render(DVDPictureRenderer* pPicture, CDVDOverlaySSA* pOverlay, double pts)
{

  int height, width;
  height = pPicture->height;
  width = pPicture->width;

  ASS_Image* img = pOverlay->m_libass->RenderImage(width, height, width, height, pts);

  int depth = OVERLAY::GetStereoscopicDepth();

  while(img)
  {
    unsigned int color = img->color;
    uint8_t alpha = (uint8_t)(color &0xff);

    // fully transparent or width or height is 0 -> not displayed
    if(alpha == 255 || img->w == 0 || img->h == 0)
    {
      img = img->next;
      continue;
    }

    //ASS_Image colors are RGBA
    double r = ((color >> 24) & 0xff) / 255.0;
    double g = ((color >> 16) & 0xff) / 255.0;
    double b = ((color >> 8 ) & 0xff) / 255.0;

    uint8_t luma  = (uint8_t)(        255 * CLAMP( 0.299 * r + 0.587 * g + 0.114 * b,  0.0, 1.0));
    uint8_t v     = (uint8_t)(127.5 + 255 * CLAMP( 0.500 * r - 0.419 * g - 0.081 * b, -0.5, 0.5));
    uint8_t u     = (uint8_t)(127.5 + 255 * CLAMP(-0.169 * r - 0.331 * g + 0.500 * b, -0.5, 0.5));

    int y = std::max(0,std::min(img->dst_y, pPicture->height-img->h));
    int x = std::max(0,std::min(img->dst_x + depth, pPicture->width-img->w));

    for(int i=0; i<img->h; i++)
    {
      if(y + i >= pPicture->height)
        break;

      uint8_t* line = img->bitmap + img->stride*i;

      uint8_t* target[3];
      target[0] = pPicture->data[0] + pPicture->stride[0]*(i + y) + x;
      target[1] = pPicture->data[1] + pPicture->stride[1]*((i + y)>>1) + (x>>1);
      target[2] = pPicture->data[2] + pPicture->stride[2]*((i + y)>>1) + (x>>1);

      for(int j=0; j<img->w; j++)
      {
        if(x + j >= pPicture->width)
          break;

        unsigned char index, opacity, k;
        index = line[j];

        //Blend the image with the underlying picture
        opacity = 255 - alpha;
        k = (unsigned char)index * opacity / 255;

        target[0][j]    = (k*luma + (255-k)*target[0][j])/255;
        target[1][j>>1] = (k*u    + (255-k)*target[1][j>>1])/255;
        target[2][j>>1] = (k*v    + (255-k)*target[2][j>>1])/255;
      }
    }
    img = img->next;
  }
}

void CDVDOverlayRenderer::Render(DVDPictureRenderer* pPicture, CDVDOverlayImage* pOverlay)
{
  uint8_t* palette[4];
  for(int i=0;i<4;i++)
    palette[i] = (uint8_t*)calloc(1, pOverlay->palette_colors);

  for(int i=0;i<pOverlay->palette_colors;i++)
  {
    uint32_t color = pOverlay->palette[i];

    palette[3][i] = (uint8_t)((color >> 24) & 0xff);

    double r = ((color >> 16) & 0xff) / 255.0;
    double g = ((color >> 8 ) & 0xff) / 255.0;
    double b = ((color >> 0 ) & 0xff) / 255.0;

    palette[0][i] = (uint8_t)(255 * CLAMP(0.299 * r + 0.587 * g + 0.114 * b, 0.0, 1.0));
    palette[1][i] = (uint8_t)(127.5 + 255 * CLAMP( 0.500 * r - 0.419 * g - 0.081 * b, -0.5, 0.5));
    palette[2][i] = (uint8_t)(127.5 + 255 * CLAMP(-0.169 * r - 0.331 * g + 0.500 * b, -0.5, 0.5));
  }

  // we try o fit it in if it's outside the image
  int y = std::max(0,std::min(pOverlay->y, pPicture->height-pOverlay->height));
  int x = std::max(0,std::min(pOverlay->x, pPicture->width-pOverlay->width));

  for(int i=0;i<pOverlay->height;i++)
  {
    if(y + i >= pPicture->height)
      break;

    uint8_t* line = pOverlay->data + pOverlay->linesize*i;

    uint8_t* target[3];
    target[0] = pPicture->data[0] + pPicture->stride[0]*(i + y) + x;
    target[1] = pPicture->data[1] + pPicture->stride[1]*((i + y)>>1) + (x>>1);
    target[2] = pPicture->data[2] + pPicture->stride[2]*((i + y)>>1) + (x>>1);

    for(int j=0;j<pOverlay->width;j++)
    {
      if(x + j >= pPicture->width)
        break;

      unsigned char index = line[j];
      if(index > pOverlay->palette_colors)
      {
        CLog::Log(LOGWARNING, "%s - out of range color index %u", __FUNCTION__, index);
        continue;
      }

      if(palette[3][index] == 0)
        continue;

      int s_blend = palette[3][index] + 1;
      int t_blend = 256 - s_blend;

      target[0][j] = (target[0][j] * t_blend + palette[0][index] * s_blend) >> 8;
      if(!(1&(i|j)))
      {
        target[1][j>>1] = (target[1][j>>1] * t_blend + palette[1][index] * s_blend) >> 8;
        target[2][j>>1] = (target[2][j>>1] * t_blend + palette[2][index] * s_blend) >> 8;
      }
    }

  }
  for(int i=0;i<4;i++)
    free(palette[i]);
}

// render the parsed sub (parsed rle) onto the yuv image
void CDVDOverlayRenderer::Render_SPU_YUV(DVDPictureRenderer* pPicture, CDVDOverlay* pOverlaySpu, bool bCrop)
{
  CDVDOverlaySpu* pOverlay = (CDVDOverlaySpu*)pOverlaySpu;

  unsigned __int8*  p_destptr = NULL;
  unsigned __int16* p_source = (unsigned __int16*)pOverlay->result;
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
          if (i_y >= btn_y_start && i_y <= btn_y_end)
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
