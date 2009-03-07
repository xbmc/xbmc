/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUITextureSDL.h"
#include "GraphicContext.h"

using namespace std;

#ifdef HAS_SDL_2D

CGUITextureSDL::CGUITextureSDL(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureSDL::Allocate()
{
  // allocate our cached textures
  for (unsigned int i = 0; i < m_texture.size(); i++)
    m_cachedTextures.push_back(CCachedTexture());
}

void CGUITextureSDL::Free()
{
  // free our cached textures
  for( unsigned int i=0;i<m_cachedTextures.size();++i )
  {
    if (m_cachedTextures[i].surface)
      SDL_FreeSurface(m_cachedTextures[i].surface);
  }
  m_cachedTextures.clear();
}

void CGUITextureSDL::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, DWORD color, int orientation)
{
  SDL_Surface* surface = m_texture.m_textures[m_currentFrame]; 
  float u[2] = { texture.x1, texture.x2 };
  float v[2] = { texture.y1, texture.y2 };
  
  // cache texture based on:
  // 1.  Bounding box
  // 2.  Diffuse color
  int b[4];
  CalcBoundingBox(x, y, 4, b);
  CCachedTexture &cached = m_cachedTextures[m_currentFrame];
  if (!cached.surface || cached.width != b[2] || cached.height != b[3] || color != cached.diffuseColor)
  { // need to re-render the surface
    RenderWithEffects(surface, x, y, u, v, &color, m_diffuse.size() ? m_diffuse.m_textures[0] : NULL, m_diffuseScaleU, m_diffuseScaleV, cached);
  }
  if (cached.surface)
  {
    SDL_Rect dst = { (Sint16)b[0], (Sint16)b[1], 0, 0 };
    g_graphicsContext.BlitToScreen(cached.surface, NULL, &dst);
  }
}

void CGUITextureSDL::CalcBoundingBox(float *x, float *y, int n, int *b)
{
  b[0] = (int)x[0], b[2] = 1;
  b[1] = (int)y[0], b[3] = 1;
  for (int i = 1; i < 4; ++i)
  {
    if (x[i] < b[0]) b[0] = (int)x[i];
    if (y[i] < b[1]) b[1] = (int)y[i];
    if (x[i]+1 > b[0] + b[2]) b[2] = (int)x[i]+1 - b[0];
    if (y[i]+1 > b[1] + b[3]) b[3] = (int)y[i]+1 - b[1];
  }
}

#define CLAMP(a,b,c) (a < b) ? b : ((a > c) ? c : a)

void CGUITextureSDL::GetTexel(float tu, float tv, SDL_Surface *src, BYTE *texel)
{
  int pu1 = (int)floor(tu);
  int pv1 = (int)floor(tv);
  int pu2 = pu1+1, pv2 = pv1+1;
  float du = tu - pu1;
  float dv = tv - pv1;
  pu1 = CLAMP(pu1,0,src->w-1);
  pu2 = CLAMP(pu2,0,src->w-1);
  pv1 = CLAMP(pv1,0,src->h-1);
  pv2 = CLAMP(pv2,0,src->h-1);
  BYTE *tex1 = (BYTE *)src->pixels + pv1 * src->pitch + pu1 * 4;
  BYTE *tex2 = (BYTE *)src->pixels + pv1 * src->pitch + pu2 * 4;
  BYTE *tex3 = (BYTE *)src->pixels + pv2 * src->pitch + pu1 * 4;
  BYTE *tex4 = (BYTE *)src->pixels + pv2 * src->pitch + pu2 * 4;
  // average these bytes
  texel[0] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
  texel[1] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
  texel[2] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
  texel[3] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
}

#define MODULATE(a,b) (b ? ((int)a*(b+1)) >> 8 : 0)
#define ALPHA_BLEND(a,b,c) (c ? ((int)a*(c+1) + b*(255-c)) >> 8 : b)

void CGUITextureSDL::RenderWithEffects(SDL_Surface *src, float *x, float *y, float *u, float *v, DWORD *c, SDL_Surface *diffuse, float diffuseScaleU, float diffuseScaleV, CCachedTexture &dst)
{
  // renders the surface from u[0],v[0] -> u[1],v[1] into the parallelogram defined by x[0],y[0]...x[3],y[3]
  // we create a new surface of the appropriate size, and render into it the resized and rotated texture,
  // with diffuse modulation etc. as necessary.
  
  // first create our surface
  if (dst.surface)
    SDL_FreeSurface(dst.surface);
  // calculate the bounding box
  int b[4];
  CalcBoundingBox(x, y, 4, b);
  dst.width = b[2]; dst.height = b[3];
  dst.diffuseColor = c[0];
  
  // create a new texture this size
  dst.surface = SDL_CreateRGBSurface(SDL_HWSURFACE, dst.width+1, dst.height+1, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  if (!dst.surface)
    return; // can't create surface

  // vectors of parallelogram in screenspace
  float ax = x[1] - x[0];
  float ay = y[1] - y[0];
  float bx = x[3] - x[0];
  float by = y[3] - y[0];

  // as we're mapping to a parallelogram, don't assume that the vectors are orthogonal (allows for skewed textures later, and no more computation)
  // we want the coordinate vector wrt the basis vectors [ax,ay] and [bx,by], so invert the matrix [ax bx;ay by]
  u[0] *= src->w;
  v[0] *= src->h;
  u[1] *= src->w;
  v[1] *= src->h;
  
  float det_m = ax * by - ay * bx;
  float m[2][2];
  m[0][0] = by / det_m * (u[1] - u[0]);
  m[0][1] = -bx / det_m * (u[1] - u[0]);
  m[1][0] = -ay / det_m * (v[1] - v[0]);
  m[1][1] = ax / det_m * (v[1] - v[0]);
  
  // we render from these values in our texture.  Note that u[0] may be bigger than u[1] when flipping
  const float minU = min(u[0], u[1]) - 0.5f;
  const float maxU = max(u[0], u[1]) + 0.5f;
  const float minV = min(v[0], v[1]) - 0.5f;
  const float maxV = max(v[0], v[1]) + 0.5f;
  
  SDL_LockSurface(dst.surface);
  SDL_LockSurface(src);
  if (diffuse)
  {
    SDL_LockSurface(diffuse);
    diffuseScaleU *= diffuse->w / src->w;
    diffuseScaleV *= diffuse->h / src->h;
  }
  
  // for speed, find the bounding box of our x, y
  // for every pixel in the bounding box, find the corresponding texel
  for (int sy = 0; sy <= dst.height; ++sy)
  {
    for (int sx = 0; sx <= dst.width; ++sx)
    {
      // find where this pixel corresponds in our texture
      float tu = m[0][0] * (sx + b[0] - x[0] - 0.5f) + m[0][1] * (sy + b[1] - y[0] - 0.5f) + u[0];
      float tv = m[1][0] * (sx + b[0] - x[0] - 0.5f) + m[1][1] * (sy + b[1] - y[0] - 0.5f) + v[0];
      if (tu > minU && tu < maxU && tv > minV && tv < maxV)
      { // in the texture - render it to screen
        BYTE tex[4];
        GetTexel(tu, tv, src, tex);
        if (diffuse)
        {
          BYTE diff[4];
          GetTexel(tu * diffuseScaleU, tv * diffuseScaleV, diffuse, diff);
          tex[0] = MODULATE(tex[0], diff[0]);
          tex[1] = MODULATE(tex[1], diff[1]);
          tex[2] = MODULATE(tex[2], diff[2]);
          tex[3] = MODULATE(tex[3], diff[3]);
        }
        // currently we just use a single color
        BYTE *diffuse = (BYTE *)c;
        BYTE *screen = (BYTE *)dst.surface->pixels + sy * dst.surface->pitch + sx*4;
        screen[0] = MODULATE(tex[0], diffuse[0]);
        screen[1] = MODULATE(tex[1], diffuse[1]);
        screen[2] = MODULATE(tex[2], diffuse[2]);
        screen[3] = MODULATE(tex[3], diffuse[3]);
      }
    }
  }
  SDL_UnlockSurface(src);
  SDL_UnlockSurface(dst.surface);
  if (diffuse)
    SDL_UnlockSurface(diffuse);
}

#endif
