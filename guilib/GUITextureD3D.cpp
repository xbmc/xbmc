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
#include "GUITextureD3D.h"
#include "GraphicContext.h"

#ifndef HAS_SDL

CGUITextureD3D::CGUITextureD3D(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureD3D::Begin()
{
  LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
  // Set state to render the image
#ifdef HAS_XBOX_D3D
  if (!m_textures[m_currentFrame].m_texCoordsArePixels)
    p3DDevice->SetPalette( 0, m_textures[m_currentFrame].m_palette);
  if (m_diffuse.m_palette)
    p3DDevice->SetPalette( 1, m_diffuse.m_palette);
#endif
  p3DDevice->SetTexture( 0, m_textures[m_currentFrame].m_texture );
  p3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  p3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  if (m_diffuse.m_texture)
  {
    p3DDevice->SetTexture( 1, m_diffuse.m_texture );
    p3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    p3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
    p3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    p3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  }

  p3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
  p3DDevice->SetRenderState( D3DRS_ALPHAREF, 0 );
  p3DDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
  p3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  p3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  p3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  p3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  p3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  p3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  p3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  p3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);
#ifdef HAS_XBOX_D3D
  p3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE);
#endif
  p3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 );
}

void CGUITextureD3D::End()
{
  LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
#ifdef HAS_XBOX_D3D
  if (g_graphicsContext.RectIsAngled(m_vertex.x1, m_vertex.y1, m_vertex.x2, m_vertex.y2))
  {
    p3DDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE );
    p3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS, TRUE );
    p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
    float u = m_frameWidth, v = m_frameHeight;
    if (!m_textures[m_currentFrame].m_texCoordsArePixels)
    {
      u *= m_texCoordsScaleU;
      v *= m_texCoordsScaleV;
    }
    Render(m_vertex.x1, m_vertex.y1, m_vertex.x2, m_vertex.y2, 0, 0, u, v, u, v);
    p3DDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
    p3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS, FALSE );
    p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  }
  p3DDevice->SetPalette( 0, NULL);
  if (m_diffuse.m_palette)
    p3DDevice->SetPalette( 1, NULL);
#endif
  // unset the texture and palette or the texture caching crashes because the runtime still has a reference
  g_graphicsContext.Get3DDevice()->SetTexture( 0, NULL );
  if (m_diffuse.m_texture)
    g_graphicsContext.Get3DDevice()->SetTexture( 1, NULL );
}

void CGUITextureD3D::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, DWORD color, int orientation)
{
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  // D3D aligns to half pixel boundaries
  for (int i = 0; i < 4; i++)
  {
    x[i] -= 0.5f;
    y[i] -= 0.5f;
    z[i] -= 0.5f;
  };

  CUSTOMVERTEX verts[4];
  verts[0].x = x[0]; verts[0].y = y[0]; verts[0].z = z[0];
  verts[0].tu = texture.x1;   verts[0].tv = texture.y1;
  verts[0].tu2 = diffuse.x1;  verts[0].tv2 = diffuse.y1;
  verts[0].color = color;

  verts[1].x = x[1]; verts[1].y = y[1]; verts[1].z = z[1];
  if (orientation & 4)
  {
    verts[1].tu = texture.x1;
    verts[1].tv = texture.y2;
  }
  else
  {
    verts[1].tu = texture.x2;
    verts[1].tv = texture.y1;
  }
  if (m_info.orientation & 4)
  {
    verts[1].tu2 = diffuse.x1;
    verts[1].tv2 = diffuse.y2;
  }
  else
  {
    verts[1].tu2 = diffuse.x2;
    verts[1].tv2 = diffuse.y1;
  }
  verts[1].color = color;

  verts[2].x = x[2]; verts[2].y = y[2]; verts[2].z = z[2];
  verts[2].tu = texture.x2;   verts[2].tv = texture.y2;
  verts[2].tu2 = diffuse.x2;  verts[2].tv2 = diffuse.y2;
  verts[2].color = color;

  verts[3].x = x[3]; verts[3].y = y[3]; verts[3].z = z[3];
  if (orientation & 4)
  {
    verts[3].tu = texture.x2;
    verts[3].tv = texture.y1;
  }
  else
  {
    verts[3].tu = texture.x1;
    verts[3].tv = texture.y2;
  }
  if (m_info.orientation & 4)
  {
    verts[3].tu2 = diffuse.x2;
    verts[3].tv2 = diffuse.y1;
  }
  else
  {
    verts[3].tu2 = diffuse.x1;
    verts[3].tv2 = diffuse.y2;
  }
  verts[3].color = color;

  g_graphicsContext.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
}

#endif
