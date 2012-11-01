#ifndef RGBV2_RENDERER
#define RGBV2_RENDERER

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBoxRenderer.h"

class CRGBRendererV2 : public CXBoxRenderer
{
public:
  CRGBRendererV2(LPDIRECT3DDEVICE8 pDevice);
  //~CRGBRendererV2();

  // Functions called from mplayer
  // virtual void     WaitForFlip();
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual unsigned int PreInit();
  virtual void UnInit();
  virtual void FlipPage(int source);

protected:
  virtual void Render(DWORD flags);
  virtual void ManageTextures();

  bool Create444PTexture(bool full, bool field);
  void Delete444PTexture();
  void Clear444PTexture(bool full, bool field);

  bool CreateLookupTextures(const YUVCOEF &coef, const YUVRANGE &range);
  void DeleteLookupTextures();

  void InterleaveYUVto444P(
      YUVPLANES          pSources,
      LPDIRECT3DTEXTURE8 pAlpha,
      LPDIRECT3DSURFACE8 pTarget,
      RECT &source, RECT &sourcealpha, RECT &target,
      unsigned cshift_x,  unsigned cshift_y,
      float    offset_x,  float    offset_y,
      float    coffset_x, float    coffset_y);

  void RenderYUVtoRGB(
      D3DBaseTexture* pSource,
      RECT &source, RECT &target,
      float offset_x, float offset_y);

  // YUV interleaved texture
  LPDIRECT3DTEXTURE8 m_444PTextureFull;
  LPDIRECT3DTEXTURE8 m_444PTextureField;

  bool m_444GeneratedFull;
  int m_444RenderBuffer;


  // textures for YUV->RGB lookup
  LPDIRECT3DTEXTURE8 m_UVLookup;
  LPDIRECT3DTEXTURE8 m_YLookup;
  LPDIRECT3DTEXTURE8 m_UVErrorLookup;
  YUVRANGE m_yuvrange_last;
  YUVCOEF  m_yuvcoef_last;


  // Pixel shaders
  DWORD m_hInterleavingShader;
  DWORD m_hInterleavingShaderAlpha;
  DWORD m_hYUVtoRGBLookup;

  BYTE m_motionpass;

  // Vertex types
  static const DWORD FVF_YUVRGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX4;
  static const DWORD FVF_RGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;
};

#endif
