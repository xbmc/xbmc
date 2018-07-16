/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextureD3D.h"

#include "D3DResource.h"
#include "GUIShaderDX.h"
#include "rendering/dx/RenderContext.h"
#include "Texture.h"

#include <DirectXMath.h>

using namespace DirectX;

CGUITextureD3D::CGUITextureD3D(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

CGUITextureD3D::~CGUITextureD3D()
{
}

void CGUITextureD3D::Begin(UTILS::Color color)
{
  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  texture->LoadToGPU();

  if (m_diffuse.size())
	  m_diffuse.m_textures[0]->LoadToGPU();

  m_col = color;

  DX::Windowing()->SetAlphaBlendEnable(true);
}

void CGUITextureD3D::End()
{
}

void CGUITextureD3D::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation)
{
  XMFLOAT4 xcolor;
  CD3DHelper::XMStoreColor(&xcolor, m_col);

  Vertex verts[4];
  verts[0].pos.x = x[0]; verts[0].pos.y = y[0]; verts[0].pos.z = z[0];
  verts[0].texCoord.x = texture.x1;   verts[0].texCoord.y = texture.y1;
  verts[0].texCoord2.x = diffuse.x1;  verts[0].texCoord2.y = diffuse.y1;
  verts[0].color = xcolor;

  verts[1].pos.x = x[1]; verts[1].pos.y = y[1]; verts[1].pos.z = z[1];
  if (orientation & 4)
  {
    verts[1].texCoord.x = texture.x1;
    verts[1].texCoord.y = texture.y2;
  }
  else
  {
    verts[1].texCoord.x = texture.x2;
    verts[1].texCoord.y = texture.y1;
  }
  if (m_info.orientation & 4)
  {
    verts[1].texCoord2.x = diffuse.x1;
    verts[1].texCoord2.y = diffuse.y2;
  }
  else
  {
    verts[1].texCoord2.x = diffuse.x2;
    verts[1].texCoord2.y = diffuse.y1;
  }
  verts[1].color = xcolor;

  verts[2].pos.x = x[2]; verts[2].pos.y = y[2]; verts[2].pos.z = z[2];
  verts[2].texCoord.x = texture.x2;   verts[2].texCoord.y = texture.y2;
  verts[2].texCoord2.x = diffuse.x2;  verts[2].texCoord2.y = diffuse.y2;
  verts[2].color = xcolor;

  verts[3].pos.x = x[3]; verts[3].pos.y = y[3]; verts[3].pos.z = z[3];
  if (orientation & 4)
  {
    verts[3].texCoord.x = texture.x2;
    verts[3].texCoord.y = texture.y1;
  }
  else
  {
    verts[3].texCoord.x = texture.x1;
    verts[3].texCoord.y = texture.y2;
  }
  if (m_info.orientation & 4)
  {
    verts[3].texCoord2.x = diffuse.x2;
    verts[3].texCoord2.y = diffuse.y1;
  }
  else
  {
    verts[3].texCoord2.x = diffuse.x1;
    verts[3].texCoord2.y = diffuse.y2;
  }
  verts[3].color = xcolor;

  CDXTexture* tex = (CDXTexture *)m_texture.m_textures[m_currentFrame];
  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();

  pGUIShader->Begin(m_diffuse.size() ? SHADER_METHOD_RENDER_MULTI_TEXTURE_BLEND : SHADER_METHOD_RENDER_TEXTURE_BLEND);

  if (m_diffuse.size())
  {
    CDXTexture* diff = (CDXTexture *)m_diffuse.m_textures[0];
    ID3D11ShaderResourceView* resource[] = { tex->GetShaderResource(), diff->GetShaderResource() };
    pGUIShader->SetShaderViews(ARRAYSIZE(resource), resource);
  }
  else
  {
    ID3D11ShaderResourceView* resource = tex->GetShaderResource();
    pGUIShader->SetShaderViews(1, &resource);
  }
  pGUIShader->DrawQuad(verts[0], verts[1], verts[2], verts[3]);
}

void CGUITextureD3D::DrawQuad(const CRect &rect, UTILS::Color color, CBaseTexture *texture, const CRect *texCoords)
{
  unsigned numViews = 0;
  ID3D11ShaderResourceView* views = nullptr;

  if (texture)
  {
    texture->LoadToGPU();
    numViews = 1;
    views = ((CDXTexture *)texture)->GetShaderResource();
  }

  CD3DTexture::DrawQuad(rect, color, numViews, &views, texCoords, texture ? SHADER_METHOD_RENDER_TEXTURE_BLEND : SHADER_METHOD_RENDER_DEFAULT);
}
