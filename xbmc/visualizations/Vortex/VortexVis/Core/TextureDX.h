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

#pragma once

#include <d3d11.h>

class TextureDX
{
public:
  TextureDX(ID3D11Texture2D* pTexture);
  virtual ~TextureDX();

  void AddRef()
  {
    m_iRefCount++;
  }

  void Release()
  {
    if (--m_iRefCount == 0)
      delete this;
  }

  ID3D11Texture2D* GetTexture() { return m_texture; }
  ID3D11RenderTargetView* GetRenderTarget();
  ID3D11ShaderResourceView* GetShaderView();
  bool LockRect(UINT subRes, D3D11_MAPPED_SUBRESOURCE* res);
  void UnlockRect(UINT subRes);

protected:
  int m_iRefCount;
  ID3D11Texture2D* m_texture;
  ID3D11RenderTargetView* m_renderTarget;
  ID3D11ShaderResourceView* m_shaderView;
};
