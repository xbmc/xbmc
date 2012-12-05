#ifndef COMBO_RENDERER
#define COMBO_RENDERER

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

//const DWORD FVF_YV12VERTEX = D3DFVF_XYZRHW|D3DFVF_TEX3;

class CComboRenderer : public CXBoxRenderer
{
public:
  CComboRenderer(LPDIRECT3DDEVICE8 pDevice);
  //~CComboRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot();
  virtual void FlipPage(int source);

  // Functions called from mplayer
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual unsigned int PreInit();
  virtual void UnInit();

protected:
  virtual void Render(DWORD flags);
  virtual void ManageDisplay();
  virtual void ManageTextures();
  bool CreateYUY2Texture(int index);
  void DeleteYUY2Texture(int index);
  void ClearYUY2Texture(int index);
  void YV12toYUY2();
  void CheckScreenSaver();

  LONG YUV2RGB(BYTE y, BYTE u, BYTE v);

  DWORD m_hPixelShader;

  // RGB/YUY2 texture target(s)
  LPDIRECT3DSURFACE8  m_RGBSurface[2];
  LPDIRECT3DTEXTURE8  m_YUY2Texture[2];

  static const DWORD FVF_YUYVVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX4;
  static const DWORD FVF_RGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;

  int m_iYUY2RenderBuffer;
  int m_iYUY2Buffers;
  int m_iScreenWidth;
  int m_iScreenHeight;
  // screensaver stuff
  bool m_bHasDimView;
};

#endif
