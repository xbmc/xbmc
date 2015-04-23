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

#include "system.h"
#include "OverlayRendererUtil.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"
#include "settings/Settings.h"

namespace OVERLAY {

static uint32_t build_rgba(int a, int r, int g, int b, bool mergealpha)
{
  if(mergealpha)
    return      a        << PIXEL_ASHIFT
         | (r * a / 255) << PIXEL_RSHIFT
         | (g * a / 255) << PIXEL_GSHIFT
         | (b * a / 255) << PIXEL_BSHIFT;
  else
    return a << PIXEL_ASHIFT
         | r << PIXEL_RSHIFT
         | g << PIXEL_GSHIFT
         | b << PIXEL_BSHIFT;
}

#define clamp(x) (x) > 255.0 ? 255 : ((x) < 0.0 ? 0 : (int)(x+0.5f))
static uint32_t build_rgba(int yuv[3], int alpha, bool mergealpha)
{
  int    a = alpha + ( (alpha << 4) & 0xff );
  double r = 1.164 * (yuv[0] - 16)                          + 1.596 * (yuv[2] - 128);
  double g = 1.164 * (yuv[0] - 16) - 0.391 * (yuv[1] - 128) - 0.813 * (yuv[2] - 128);
  double b = 1.164 * (yuv[0] - 16) + 2.018 * (yuv[1] - 128);
  return build_rgba(a, clamp(r), clamp(g), clamp(b), mergealpha);
}
#undef clamp

uint32_t* convert_rgba(CDVDOverlayImage* o, bool mergealpha)
{
  uint32_t* rgba = (uint32_t*)malloc(o->width * o->height * sizeof(uint32_t));

  if(!rgba)
    return NULL;

  uint32_t palette[256];
  memset(palette, 0, 256 * sizeof(palette[0]));
  for(int i = 0; i < o->palette_colors; i++)
    palette[i] = build_rgba((o->palette[i] >> PIXEL_ASHIFT) & 0xff
                          , (o->palette[i] >> PIXEL_RSHIFT) & 0xff
                          , (o->palette[i] >> PIXEL_GSHIFT) & 0xff
                          , (o->palette[i] >> PIXEL_BSHIFT) & 0xff
                          , mergealpha);

  for(int row = 0; row < o->height; row++)
    for(int col = 0; col < o->width; col++)
      rgba[row * o->width + col] = palette[ o->data[row * o->linesize + col] ];

  return rgba;
}

uint32_t* convert_rgba(CDVDOverlaySpu* o, bool mergealpha
                              , int& min_x, int& max_x
                              , int& min_y, int& max_y)
{
  uint32_t* rgba = (uint32_t*)malloc(o->width * o->height * sizeof(uint32_t));

  if(!rgba)
    return NULL;

  uint32_t palette[8];
  for(int i = 0; i < 4; i++)
  {
    palette[i]   = build_rgba(o->color[i]          , o->alpha[i]          , mergealpha);
    palette[i+4] = build_rgba(o->highlight_color[i], o->highlight_alpha[i], mergealpha);
  }

  uint32_t  color;
  uint32_t* trg;
  uint16_t* src;

  int len, idx, draw;

  int btn_x_start = 0
    , btn_x_end   = 0
    , btn_y_start = 0
    , btn_y_end   = 0;

  if(o->bForced)
  {
    btn_x_start = o->crop_i_x_start - o->x;
    btn_x_end   = o->crop_i_x_end   - o->x;
    btn_y_start = o->crop_i_y_start - o->y;
    btn_y_end   = o->crop_i_y_end   - o->y;
  }

  min_x = o->width;
  max_x = 0;
  min_y = o->height;
  max_y = 0;

  trg = rgba;
  src = (uint16_t*)o->result;

  for (int y = 0; y < o->height; y++)
  {
    for (int x = 0; x < o->width ; x += len)
    {
      /* Get the RLE part, then draw the line */
      idx = *src & 0x3;
      len = *src++ >> 2;

      while( len > 0 )
      {
        draw  = len;
        color = palette[idx];

        if (y >= btn_y_start && y <= btn_y_end)
        {
          if     ( x <  btn_x_start && x + len >= btn_x_start) // starts outside
            draw = btn_x_start - x;
          else if( x >= btn_x_start && x       <= btn_x_end)   // starts inside
          {
            color = palette[idx + 4];
            draw  = btn_x_end - x + 1;
          }
        }
        /* make sure we are not requested to draw to far */
        /* that part will be taken care of in next pass */
        if( draw > len )
          draw = len;

        /* calculate cropping */
        if(color & 0xff000000)
        {
          if(x < min_x)
            min_x = x;
          if(y < min_y)
            min_y = y;
          if(x + draw > max_x)
            max_x = x + draw;
          if(y + 1    > max_y)
            max_y = y + 1;
        }

        for(int i = 0; i < draw; i++)
          trg[x + i] = color;

        len -= draw;
        x   += draw;
      }
    }
    trg += o->width;
  }

  /* if nothing visible, just output a dummy pixel */
  if(max_x <= min_x
  || max_y <= min_y)
  {
    max_y = max_x = 1;
    min_y = min_x = 0;
  }

  return rgba;
}

bool convert_quad(ASS_Image* images, SQuads& quads)
{
  ASS_Image* img;

  if (!images)
    return false;

  // first calculate how many glyph we have and the total x length

  for(img = images; img; img = img->next)
  {
    // fully transparent or width or height is 0 -> not displayed
    if((img->color & 0xff) == 0xff || img->w == 0 || img->h == 0)
      continue;

    quads.size_x += img->w + 1;
    quads.count++;
  }

  if (quads.count == 0)
    return false;

  if (quads.size_x > (int)g_Windowing.GetMaxTextureSize())
    quads.size_x = g_Windowing.GetMaxTextureSize();

  int curr_x = 0;
  int curr_y = 0;

  // calculate the y size of the texture

  for(img = images; img; img = img->next)
  {
    if((img->color & 0xff) == 0xff || img->w == 0 || img->h == 0)
      continue;

    // check if we need to split to new line
    if (curr_x + img->w >= quads.size_x)
    {
      quads.size_y += curr_y + 1;
      curr_x = 0;
      curr_y = 0;
    }

    curr_x += img->w + 1;

    if (img->h > curr_y)
      curr_y = img->h;
  }

  quads.size_y += curr_y + 1;

  // allocate space for the glyph positions and texturedata

  quads.quad = (SQuad*)  calloc(quads.count, sizeof(SQuad));
  quads.data = (uint8_t*)calloc(quads.size_x * quads.size_y, 1);

  SQuad*   v    = quads.quad;
  uint8_t* data = quads.data;

  int y = 0;

  curr_x = 0;
  curr_y = 0;

  for(img = images; img; img = img->next)
  {
    if((img->color & 0xff) == 0xff || img->w == 0 || img->h == 0)
      continue;

    unsigned int color = img->color;
    unsigned int alpha = (color & 0xff);

    if (curr_x + img->w >= quads.size_x)
    {
      curr_y += y + 1;
      curr_x  = 0;
      y       = 0;
      data    = quads.data + curr_y * quads.size_x;
    }

    unsigned int r = ((color >> 24) & 0xff);
    unsigned int g = ((color >> 16) & 0xff);
    unsigned int b = ((color >> 8 ) & 0xff);

    v->a = 255 - alpha;
    v->r = r;
    v->g = g;
    v->b = b;

    v->u = curr_x;
    v->v = curr_y;

    v->x = img->dst_x;
    v->y = img->dst_y;

    v->w = img->w;
    v->h = img->h;

    v++;

    for(int i=0; i<img->h; i++)
      memcpy(data        + quads.size_x * i
           , img->bitmap + img->stride  * i
           , img->w);

    if (img->h > y)
      y = img->h;

    curr_x += img->w + 1;
    data   += img->w + 1;
  }
  return true;
}

int GetStereoscopicDepth()
{
  int depth = 0;

  if(g_graphicsContext.GetStereoMode() != RENDER_STEREO_MODE_MONO
  && g_graphicsContext.GetStereoMode() != RENDER_STEREO_MODE_OFF)
  {
    depth  = CSettings::Get().GetInt("subtitles.stereoscopicdepth");
    depth *= (g_graphicsContext.GetStereoView() == RENDER_STEREO_VIEW_LEFT ? 1 : -1);
  }

  return depth;
}

}
