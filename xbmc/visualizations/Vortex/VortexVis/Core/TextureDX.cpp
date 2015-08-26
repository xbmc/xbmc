/*
*  Copyright © 2010-2015 Team Kodi
*  http://kodi.tv
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "TextureDX.h"
#include "Renderer.h"

TextureDX::TextureDX(ID3D11Texture2D* pTexture)
  : m_iRefCount(1),
    m_texture(pTexture),
    m_renderTarget(NULL),
    m_shaderView(NULL)
{
}

TextureDX::~TextureDX()
{
  SAFE_RELEASE( m_shaderView );
  SAFE_RELEASE( m_renderTarget );
  SAFE_RELEASE( m_texture );
}

ID3D11RenderTargetView* TextureDX::GetRenderTarget()
{
  if (!m_renderTarget)
    Renderer::CreateRenderTarget(m_texture, &m_renderTarget);

  return m_renderTarget;
}

ID3D11ShaderResourceView* TextureDX::GetShaderView()
{
  if (!m_shaderView)
    Renderer::CreateShaderView(m_texture, &m_shaderView);

  return m_shaderView;
}

bool TextureDX::LockRect(UINT subRes, D3D11_MAPPED_SUBRESOURCE* res)
{
  if (!m_texture)
    return false;

  return S_OK == Renderer::GetContext()->Map(m_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, res);
}

void TextureDX::UnlockRect(UINT subRes)
{
  if (!m_texture)
    return;

  Renderer::GetContext()->Unmap(m_texture, 0);
}
