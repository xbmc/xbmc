/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayRendererUtil.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>

namespace OVERLAY
{

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

#define clamp(x) (x) > 255.0 ? 255 : ((x) < 0.0 ? 0 : (int)(x + 0.5))
static uint32_t build_rgba(const int yuv[3], int alpha, bool mergealpha)
{
  int    a = alpha + ( (alpha << 4) & 0xff );
  double r = 1.164 * (yuv[0] - 16)                          + 1.596 * (yuv[2] - 128);
  double g = 1.164 * (yuv[0] - 16) - 0.391 * (yuv[1] - 128) - 0.813 * (yuv[2] - 128);
  double b = 1.164 * (yuv[0] - 16) + 2.018 * (yuv[1] - 128);
  return build_rgba(a, clamp(r), clamp(g), clamp(b), mergealpha);
}
#undef clamp

void convert_rgba(const CDVDOverlayImage& o,
                  bool mergealpha,
                  std::vector<uint32_t>& rgba,
                  const std::vector<uint32_t>* paletteOverride)
{
  const std::vector<uint32_t>& srcPalette = paletteOverride ? *paletteOverride : o.palette;

  uint32_t palette[256] = {};
  for (size_t i = 0; i < srcPalette.size(); i++)
    palette[i] = build_rgba(
        (srcPalette[i] >> PIXEL_ASHIFT) & 0xff, (srcPalette[i] >> PIXEL_RSHIFT) & 0xff,
        (srcPalette[i] >> PIXEL_GSHIFT) & 0xff, (srcPalette[i] >> PIXEL_BSHIFT) & 0xff, mergealpha);

  for (int row = 0; row < o.height; row++)
    for (int col = 0; col < o.width; col++)
      rgba[row * o.width + col] = palette[o.pixels[row * o.linesize + col]];
}

void convert_rgba(const CDVDOverlaySpu& o,
                  bool mergealpha,
                  int& min_x,
                  int& max_x,
                  int& min_y,
                  int& max_y,
                  std::vector<uint32_t>& rgba)
{
  uint32_t palette[8];
  for (int i = 0; i < 4; i++)
  {
    palette[i] = build_rgba(o.color[i], o.alpha[i], mergealpha);
    palette[i + 4] = build_rgba(o.highlight_color[i], o.highlight_alpha[i], mergealpha);
  }

  uint32_t  color;
  uint32_t* trg;
  uint16_t* src;

  int len, idx, draw;

  int btn_x_start = 0
    , btn_x_end   = 0
    , btn_y_start = 0
    , btn_y_end   = 0;

  if (o.bForced)
  {
    btn_x_start = o.crop_i_x_start - o.x;
    btn_x_end = o.crop_i_x_end - o.x;
    btn_y_start = o.crop_i_y_start - o.y;
    btn_y_end = o.crop_i_y_end - o.y;
  }

  min_x = o.width;
  max_x = 0;
  min_y = o.height;
  max_y = 0;

  trg = rgba.data();
  src = (uint16_t*)o.result;

  for (int y = 0; y < o.height; y++)
  {
    for (int x = 0; x < o.width; x += len)
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
    trg += o.width;
  }

  /* if nothing visible, just output a dummy pixel */
  if(max_x <= min_x
  || max_y <= min_y)
  {
    max_y = max_x = 1;
    min_y = min_x = 0;
  }
}

bool convert_quad(ASS_Image* images, SQuads& quads, int max_x)
{
  ASS_Image* img;
  int count = 0;

  if (!images)
    return false;

  // first calculate how many glyph we have and the total x length

  for(img = images; img; img = img->next)
  {
    // fully transparent or width or height is 0 -> not displayed
    if((img->color & 0xff) == 0xff || img->w == 0 || img->h == 0)
      continue;

    quads.size_x += img->w + 1;
    count++;
  }

  if (count == 0)
    return false;

  if (quads.size_x > max_x)
    quads.size_x = max_x;

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
  quads.quad.resize(count);
  quads.texture.resize(quads.size_x * quads.size_y);

  SQuad* v = quads.quad.data();
  uint8_t* data = quads.texture.data();

  int y = 0;

  curr_x = 0;
  curr_y = 0;

  for (img = images; img; img = img->next)
  {
    if ((img->color & 0xff) == 0xff || img->w == 0 || img->h == 0)
      continue;

    unsigned int color = img->color;
    unsigned int alpha = (color & 0xff);

    if (curr_x + img->w >= quads.size_x)
    {
      curr_y += y + 1;
      curr_x = 0;
      y = 0;
      data = quads.texture.data() + curr_y * quads.size_x;
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

    for (int i = 0; i < img->h; i++)
      memcpy(data + quads.size_x * i, img->bitmap + img->stride * i, img->w);

    if (img->h > y)
      y = img->h;

    curr_x += img->w + 1;
    data   += img->w + 1;
  }
  return true;
}

namespace
{
constexpr float ST2084_m1 = 2610.0f / (4096.0f * 4.0f);
constexpr float ST2084_m2 = (2523.0f / 4096.0f) * 128.0f;
constexpr float ST2084_c1 = 3424.0f / 4096.0f;
constexpr float ST2084_c2 = (2413.0f / 4096.0f) * 32.0f;
constexpr float ST2084_c3 = (2392.0f / 4096.0f) * 32.0f;

// PQ code value (0-1) -> linear (1.0 == 10000 nits).
float DecodePQ(float x)
{
  x = std::clamp(x, 0.0f, 1.0f);
  const float p = std::pow(x, 1.0f / ST2084_m2);
  const float num = std::max(p - ST2084_c1, 0.0f);
  const float den = std::max(ST2084_c2 - ST2084_c3 * p, 1e-6f);
  return std::pow(num / den, 1.0f / ST2084_m1);
}

float LinearToSrgbComponent(float c)
{
  c = std::clamp(c, 0.0f, 1.0f);
  return (c <= 0.0031308f) ? (12.92f * c) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
}
} // namespace

void ConvertPgsPaletteToSdr(std::vector<uint32_t>& palette, float hdrWhiteNits, float sdrWhiteNits)
{
  // Linear BT.2020 -> linear BT.709/sRGB primaries, D65 both ends;
  // numerically confirmed to map (1,1,1) to (1,1,1).
  static constexpr float BT2020_TO_709[3][3] = {
      {1.660491002f, -0.587641139f, -0.072849863f},
      {-0.124550474f, 1.132899897f, -0.008349423f},
      {-0.018150763f, -0.100578897f, 1.118729660f},
  };

  const float whiteScale = std::min(10000.0f, 5000.0f + hdrWhiteNits) / sdrWhiteNits;

  for (uint32_t& entry : palette)
  {
    const int a = (entry >> PIXEL_ASHIFT) & 0xff;
    const float linearPQ[3] = {
        DecodePQ(((entry >> PIXEL_RSHIFT) & 0xff) / 255.0f) * whiteScale,
        DecodePQ(((entry >> PIXEL_GSHIFT) & 0xff) / 255.0f) * whiteScale,
        DecodePQ(((entry >> PIXEL_BSHIFT) & 0xff) / 255.0f) * whiteScale,
    };

    float linear709[3];
    for (int i = 0; i < 3; i++)
      linear709[i] =
          std::max(BT2020_TO_709[i][0] * linearPQ[0] + BT2020_TO_709[i][1] * linearPQ[1] +
                       BT2020_TO_709[i][2] * linearPQ[2],
                   0.0f);

    // Scale toward white by the single largest channel instead of clipping
    // each channel independently, so an out-of-gamut or above-white colour
    // keeps its hue - e.g. orange stays orange rather than shifting toward
    // yellow/white as one or two channels clip before the others.
    const float maxChannel = std::max({linear709[0], linear709[1], linear709[2]});
    if (maxChannel > 1.0f)
      for (float& c : linear709)
        c /= maxChannel;

    const int r = static_cast<int>(LinearToSrgbComponent(linear709[0]) * 255.0f + 0.5f);
    const int g = static_cast<int>(LinearToSrgbComponent(linear709[1]) * 255.0f + 0.5f);
    const int b = static_cast<int>(LinearToSrgbComponent(linear709[2]) * 255.0f + 0.5f);

    entry = (a << PIXEL_ASHIFT) | (r << PIXEL_RSHIFT) | (g << PIXEL_GSHIFT) | (b << PIXEL_BSHIFT);
  }
}

int GetStereoscopicDepth()
{
  int depth = 0;

  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() != RenderStereoMode::MONO &&
      CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() != RenderStereoMode::OFF)
  {
    depth  = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH);
    depth *=
        (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoView() == RenderStereoView::LEFT
             ? 1
             : -1);
  }

  return depth;
}

}
