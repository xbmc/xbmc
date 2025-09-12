/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once


#include "PLD3D11Texture.h"
#include "PlHelper.h"
#include <wrl/client.h>
#include <rendering/dx/DeviceResources.h>
using namespace Microsoft::WRL;
namespace PL
{


  CPLD3D11Texture::CPLD3D11Texture(ID3D11Texture2D* tex, UINT plane)
    : m_tex(nullptr)
  {
    if (!tex)
      return;

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);

    // Wrap the plane of the D3D11 texture
    pl_d3d11_wrap_params params = {};
    params.tex = tex;
    params.w = desc.Width;
    params.h = desc.Height;
    params.fmt = desc.Format;
    params.array_slice = 1;

    m_tex[0] = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &params);
  }

  CPLD3D11Texture::CPLD3D11Texture(ID3D11Resource* res, UINT plane)
    : m_tex(nullptr)
  {
    if (!res)
      return;
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);

    ComPtr<ID3D11ShaderResourceView> srvY;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16_UNORM; // single channel
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2DArray.FirstArraySlice = 0; // 0 = Y plane
    srvDesc.Texture2DArray.ArraySize = 1;
    srvDesc.Texture2DArray.MipLevels = 1;
    
    hr = DX::DeviceResources::Get()->GetD3DDevice()->CreateShaderResourceView(tex, &srvDesc, &srvY);

    // UV plane view
    ComPtr<ID3D11ShaderResourceView> srvUV;
    srvDesc.Format = DXGI_FORMAT_R16G16_UNORM; // interleaved UV
    srvDesc.Texture2DArray.FirstArraySlice = 1; // 1 = UV
    srvDesc.Texture2DArray.ArraySize = 1;
    srvDesc.Texture2DArray.MipLevels = 1;
    hr = DX::DeviceResources::Get()->GetD3DDevice()->CreateShaderResourceView(tex, &srvDesc, &srvUV);

    // Wrap the plane of the D3D11 texture
    pl_d3d11_wrap_params params = {};
    params.tex = tex;
    params.w = desc.Width;
    params.h = desc.Height;
    params.fmt = DXGI_FORMAT_R16_UNORM;
    params.array_slice = plane;

    m_tex[0] = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &params);
    params.w = desc.Width/2;
    params.h = desc.Height/2;
    params.fmt = DXGI_FORMAT_R16G16_UNORM;
    params.array_slice = plane;

    m_tex[1] = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &params);
  }

  CPLD3D11Texture::~CPLD3D11Texture()
  {
    if (m_tex[0])
      pl_tex_destroy(PL::PLInstance::Get()->GetGpu(), &m_tex[0]);
  }




  void CPLD3D11Texture::cleanup()
  {
    if (m_tex[0])
    {
      pl_tex_destroy(PL::PLInstance::Get()->GetGpu(), &m_tex[0]);
      m_tex[0] = nullptr;
    }
  }


};