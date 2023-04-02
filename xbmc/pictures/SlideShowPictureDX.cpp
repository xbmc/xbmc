/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SlideShowPictureDX.h"

#include "guilib/TextureDX.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"

#include <DirectXMath.h>
using namespace DirectX;
using namespace Microsoft::WRL;

std::unique_ptr<CSlideShowPic> CSlideShowPic::CreateSlideShowPicture()
{
  return std::make_unique<CSlideShowPicDX>();
}

bool CSlideShowPicDX::UpdateVertexBuffer(Vertex* vertices)
{
  if (!m_vb) // create new
  {
    CD3D11_BUFFER_DESC desc(sizeof(Vertex) * 5, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC,
                            D3D11_CPU_ACCESS_WRITE);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    initData.SysMemPitch = sizeof(Vertex) * 5;
    if (SUCCEEDED(DX::DeviceResources::Get()->GetD3DDevice()->CreateBuffer(
            &desc, &initData, m_vb.ReleaseAndGetAddressOf())))
      return true;
  }
  else // update
  {
    ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();
    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pContext->Map(m_vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      memcpy(res.pData, vertices, sizeof(Vertex) * 5);
      pContext->Unmap(m_vb.Get(), 0);
      return true;
    }
  }

  return false;
}

void CSlideShowPicDX::Render(float* x, float* y, CTexture* pTexture, UTILS::COLOR::Color color)
{
  Vertex vertex[5];
  for (int i = 0; i < 4; i++)
  {
    vertex[i].pos = XMFLOAT3(x[i], y[i], 0);
    CD3DHelper::XMStoreColor(&vertex[i].color, color);
    vertex[i].texCoord = XMFLOAT2(0.0f, 0.0f);
    vertex[i].texCoord2 = XMFLOAT2(0.0f, 0.0f);
  }

  if (pTexture)
  {
    vertex[1].texCoord.x = vertex[2].texCoord.x =
        (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    vertex[2].texCoord.y = vertex[3].texCoord.y =
        (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }
  else
  {
    vertex[1].texCoord.x = vertex[2].texCoord.x = 1.0f;
    vertex[2].texCoord.y = vertex[3].texCoord.y = 1.0f;
  }
  vertex[4] = vertex[0]; // Not used when pTexture != NULL

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();
  pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_BLEND);

  // Set state to render the image
  if (pTexture)
  {
    pTexture->LoadToGPU();
    CDXTexture* dxTexture = reinterpret_cast<CDXTexture*>(pTexture);
    ID3D11ShaderResourceView* shaderRes = dxTexture->GetShaderResource();
    pGUIShader->SetShaderViews(1, &shaderRes);
    pGUIShader->DrawQuad(vertex[0], vertex[1], vertex[2], vertex[3]);
  }
  else
  {
    if (!UpdateVertexBuffer(vertex))
      return;

    ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();

    unsigned stride = sizeof(Vertex);
    unsigned offset = 0;
    pContext->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    pGUIShader->Draw(5, 0);
    pGUIShader->RestoreBuffers();
  }
}
