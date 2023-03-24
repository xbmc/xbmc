/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "D3DResource.h"

#include "GUIShaderDX.h"
#include "filesystem/File.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#ifdef TARGET_WINDOWS_DESKTOP
#pragma comment(lib, "d3dcompiler.lib")
#endif

size_t CD3DHelper::BitsPerPixel(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
  case DXGI_FORMAT_R32G32B32A32_UINT:
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return 128;

  case DXGI_FORMAT_R32G32B32_TYPELESS:
  case DXGI_FORMAT_R32G32B32_FLOAT:
  case DXGI_FORMAT_R32G32B32_UINT:
  case DXGI_FORMAT_R32G32B32_SINT:
    return 96;

  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
  case DXGI_FORMAT_R16G16B16A16_UNORM:
  case DXGI_FORMAT_R16G16B16A16_UINT:
  case DXGI_FORMAT_R16G16B16A16_SNORM:
  case DXGI_FORMAT_R16G16B16A16_SINT:
  case DXGI_FORMAT_R32G32_TYPELESS:
  case DXGI_FORMAT_R32G32_FLOAT:
  case DXGI_FORMAT_R32G32_UINT:
  case DXGI_FORMAT_R32G32_SINT:
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return 64;

  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
  case DXGI_FORMAT_R10G10B10A2_UNORM:
  case DXGI_FORMAT_R10G10B10A2_UINT:
  case DXGI_FORMAT_R11G11B10_FLOAT:
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
  case DXGI_FORMAT_R8G8B8A8_UINT:
  case DXGI_FORMAT_R8G8B8A8_SNORM:
  case DXGI_FORMAT_R8G8B8A8_SINT:
  case DXGI_FORMAT_R16G16_TYPELESS:
  case DXGI_FORMAT_R16G16_FLOAT:
  case DXGI_FORMAT_R16G16_UNORM:
  case DXGI_FORMAT_R16G16_UINT:
  case DXGI_FORMAT_R16G16_SNORM:
  case DXGI_FORMAT_R16G16_SINT:
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32_FLOAT:
  case DXGI_FORMAT_R32_UINT:
  case DXGI_FORMAT_R32_SINT:
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
  case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
  case DXGI_FORMAT_R8G8_B8G8_UNORM:
  case DXGI_FORMAT_G8R8_G8B8_UNORM:
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_B8G8R8X8_UNORM:
  case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    return 32;

  case DXGI_FORMAT_R8G8_TYPELESS:
  case DXGI_FORMAT_R8G8_UNORM:
  case DXGI_FORMAT_R8G8_UINT:
  case DXGI_FORMAT_R8G8_SNORM:
  case DXGI_FORMAT_R8G8_SINT:
  case DXGI_FORMAT_R16_TYPELESS:
  case DXGI_FORMAT_R16_FLOAT:
  case DXGI_FORMAT_D16_UNORM:
  case DXGI_FORMAT_R16_UNORM:
  case DXGI_FORMAT_R16_UINT:
  case DXGI_FORMAT_R16_SNORM:
  case DXGI_FORMAT_R16_SINT:
  case DXGI_FORMAT_B5G6R5_UNORM:
  case DXGI_FORMAT_B5G5R5A1_UNORM:
  case DXGI_FORMAT_B4G4R4A4_UNORM:
    return 16;

  case DXGI_FORMAT_R8_TYPELESS:
  case DXGI_FORMAT_R8_UNORM:
  case DXGI_FORMAT_R8_UINT:
  case DXGI_FORMAT_R8_SNORM:
  case DXGI_FORMAT_R8_SINT:
  case DXGI_FORMAT_A8_UNORM:
    return 8;

  case DXGI_FORMAT_R1_UNORM:
    return 1;

  case DXGI_FORMAT_BC1_TYPELESS:
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC4_TYPELESS:
  case DXGI_FORMAT_BC4_UNORM:
  case DXGI_FORMAT_BC4_SNORM:
    return 4;

  case DXGI_FORMAT_BC2_TYPELESS:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC3_TYPELESS:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC5_TYPELESS:
  case DXGI_FORMAT_BC5_UNORM:
  case DXGI_FORMAT_BC5_SNORM:
  case DXGI_FORMAT_BC6H_TYPELESS:
  case DXGI_FORMAT_BC6H_UF16:
  case DXGI_FORMAT_BC6H_SF16:
  case DXGI_FORMAT_BC7_TYPELESS:
  case DXGI_FORMAT_BC7_UNORM:
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return 8;

  default:
    return 0;
  }
}

void ID3DResource::Register()
{
  if (!m_bRegistered)
    DX::Windowing()->Register(this);
  m_bRegistered = true;
}

void ID3DResource::Unregister()
{
  if (m_bRegistered)
    DX::Windowing()->Unregister(this);
  m_bRegistered = false;
}

CD3DTexture::CD3DTexture()
{
  m_width = 0;
  m_height = 0;
  m_mipLevels = 0;
  m_usage = D3D11_USAGE_DEFAULT;
  m_format = DXGI_FORMAT_B8G8R8A8_UNORM;
  m_texture = nullptr;
  m_renderTargets[0] = nullptr;
  m_renderTargets[1] = nullptr;
  m_data = nullptr;
  m_pitch = 0;
  m_bindFlags = 0;
  m_cpuFlags = 0;
  m_viewIdx = 0;
  m_views.clear();
}

CD3DTexture::~CD3DTexture()
{
  Release();
  delete[] m_data;
}

bool CD3DTexture::Create(UINT width, UINT height, UINT mipLevels, D3D11_USAGE usage, DXGI_FORMAT format, const void* pixels /* nullptr */, unsigned int srcPitch /* 0 */)
{
  m_width = width;
  m_height = height;
  m_mipLevels = mipLevels;
  // create the texture
  Release();

  if (format == DXGI_FORMAT_UNKNOWN)
    format = DXGI_FORMAT_B8G8R8A8_UNORM; // DXGI_FORMAT_UNKNOWN

  if (!DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_TEXTURE2D))
  {
    CLog::LogF(LOGERROR, "unsupported texture format {}", DX::DXGIFormatToString(format));
    return false;
  }

  m_cpuFlags = 0;
  if (usage == D3D11_USAGE_DYNAMIC || usage == D3D11_USAGE_STAGING)
  {
    m_cpuFlags |= D3D11_CPU_ACCESS_WRITE;
    if (usage == D3D11_USAGE_STAGING)
      m_cpuFlags |= D3D11_CPU_ACCESS_READ;
  }

  m_format = format;
  m_usage = usage;

  m_bindFlags = 0; // D3D11_BIND_SHADER_RESOURCE;
  if (D3D11_USAGE_DEFAULT == usage && DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_RENDER_TARGET))
    m_bindFlags |= D3D11_BIND_RENDER_TARGET;
  if ( D3D11_USAGE_STAGING != m_usage )
  {
    if (DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_SHADER_LOAD)
      || DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
    {
      m_bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_DECODER_OUTPUT))
    {
      m_bindFlags |= D3D11_BIND_DECODER;
    }
  }

  if (!CreateInternal(pixels, srcPitch))
  {
    CLog::LogF(LOGERROR, "failed to create texture.");
    return false;
  }

  Register();

  return true;
}

bool CD3DTexture::CreateInternal(const void* pixels /* nullptr */, unsigned int srcPitch /* 0 */)
{
  ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
  ComPtr<ID3D11DeviceContext> pD3D11Context = DX::DeviceResources::Get()->GetD3DContext();

  UINT miscFlags = 0;
  bool autogenmm = false;
  if (m_mipLevels == 0 && DX::Windowing()->IsFormatSupport(m_format, D3D11_FORMAT_SUPPORT_MIP_AUTOGEN))
  {
    autogenmm = pixels != nullptr;
    miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
  }
  else
    m_mipLevels = 1;

  CD3D11_TEXTURE2D_DESC textureDesc(m_format, m_width, m_height, 1, m_mipLevels, m_bindFlags, m_usage, m_cpuFlags, 1, 0, miscFlags);
  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = pixels;
  initData.SysMemPitch = srcPitch ? srcPitch : CD3DHelper::BitsPerPixel(m_format) * m_width / 8;
  initData.SysMemSlicePitch = 0;

  HRESULT hr = pD3DDevice->CreateTexture2D(&textureDesc, (!autogenmm && pixels) ? &initData : nullptr, m_texture.ReleaseAndGetAddressOf());
  if (SUCCEEDED(hr) && autogenmm)
  {
    pD3D11Context->UpdateSubresource(m_texture.Get(), 0, nullptr, pixels,
      (srcPitch ? srcPitch : CD3DHelper::BitsPerPixel(m_format) * m_width / 8), 0);
  }

  if (autogenmm)
    GenerateMipmaps();

  return SUCCEEDED(hr);
}

ID3D11ShaderResourceView* CD3DTexture::GetShaderResource(DXGI_FORMAT format /* = DXGI_FORMAT_UNKNOWN */)
{
  if (!m_texture)
    return nullptr;

  if (format == DXGI_FORMAT_UNKNOWN)
    format = m_format;

  if (!DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
    && !DX::Windowing()->IsFormatSupport(format, D3D11_FORMAT_SUPPORT_SHADER_LOAD))
    return nullptr;

  if (!m_views[format])
  {
    ComPtr<ID3D11ShaderResourceView> view;
    CD3D11_SHADER_RESOURCE_VIEW_DESC cSRVDesc(D3D11_SRV_DIMENSION_TEXTURE2D, format);
    HRESULT hr = DX::DeviceResources::Get()->GetD3DDevice()->CreateShaderResourceView(m_texture.Get(), &cSRVDesc, view.GetAddressOf());
    if (SUCCEEDED(hr))
      m_views.insert_or_assign(format, view);
    else
      CLog::LogF(LOGWARNING, "cannot create texture view.");
  }

  return m_views[format].Get();
}

ID3D11ShaderResourceView** CD3DTexture::GetAddressOfSRV(DXGI_FORMAT format)
{
  if (format == DXGI_FORMAT_UNKNOWN)
    format = m_format;

  if (!m_views[format])
    GetShaderResource(format);

  return m_views[format].GetAddressOf();
}

ID3D11RenderTargetView* CD3DTexture::GetRenderTarget()
{
  return GetRenderTargetInternal(m_viewIdx);
}

ID3D11RenderTargetView** CD3DTexture::GetAddressOfRTV()
{
  if (!m_renderTargets[m_viewIdx])
    GetRenderTargetInternal(m_viewIdx);
  return m_renderTargets[m_viewIdx].GetAddressOf();
}

void CD3DTexture::Release()
{
  Unregister();

  m_views.clear();
  m_renderTargets[0] = nullptr;
  m_renderTargets[1] = nullptr;
  m_texture = nullptr;
}

bool CD3DTexture::GetDesc(D3D11_TEXTURE2D_DESC *desc) const
{
  if (m_texture)
  {
    m_texture->GetDesc(desc);
    return true;
  }
  return false;
}

bool CD3DTexture::LockRect(UINT subresource, D3D11_MAPPED_SUBRESOURCE *res, D3D11_MAP mapType) const
{
  if (m_texture)
  {
    if (m_usage == D3D11_USAGE_DEFAULT)
      return false;
    if ((mapType == D3D11_MAP_READ || mapType == D3D11_MAP_READ_WRITE) && m_usage == D3D11_USAGE_DYNAMIC)
      return false;

    return (S_OK == DX::DeviceResources::Get()->GetImmediateContext()->Map(m_texture.Get(), subresource, mapType, 0, res));
  }
  return false;
}

bool CD3DTexture::UnlockRect(UINT subresource) const
{
  if (m_texture)
  {
    DX::DeviceResources::Get()->GetImmediateContext()->Unmap(m_texture.Get(), subresource);
    return true;
  }
  return false;
}

void CD3DTexture::SaveTexture()
{
  if (m_texture)
  {
    delete[] m_data;
    m_data = nullptr;

    ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetImmediateContext();

    D3D11_TEXTURE2D_DESC textureDesc;
    m_texture->GetDesc(&textureDesc);

    ComPtr<ID3D11Texture2D> texture = nullptr;
    if (textureDesc.Usage != D3D11_USAGE_STAGING || 0 == (textureDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ))
    {
      // create texture which can be readed by CPU - D3D11_USAGE_STAGING
      CD3D11_TEXTURE2D_DESC stagingDesc(textureDesc);
      stagingDesc.Usage = D3D11_USAGE_STAGING;
      stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      stagingDesc.BindFlags = 0;

      if (FAILED(DX::DeviceResources::Get()->GetD3DDevice()->CreateTexture2D(&stagingDesc, NULL, &texture)))
        return;

      // copy contents to new texture
      pContext->CopyResource(texture.Get(), m_texture.Get());
    }
    else
      texture = m_texture;

    // read data from texture
    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pContext->Map(texture.Get(), 0, D3D11_MAP_READ, 0, &res)))
    {
      m_pitch = res.RowPitch;
      unsigned int memUsage = GetMemoryUsage(res.RowPitch);
      m_data = new unsigned char[memUsage];
      memcpy(m_data, res.pData, memUsage);
      pContext->Unmap(texture.Get(), 0);
    }
    else
      CLog::LogF(LOGERROR, "Failed to store resource.");
  }
}

void CD3DTexture::OnDestroyDevice(bool fatal)
{
  if (!fatal)
    SaveTexture();
  m_views.clear();
  m_renderTargets[0] = nullptr;
  m_renderTargets[1] = nullptr;
  m_texture = nullptr;
}

void CD3DTexture::RestoreTexture()
{
  // yay, we're back - make a new copy of the texture
  if (!m_texture && m_data)
  {
    if (!CreateInternal(m_data, m_pitch))
    {
      CLog::LogF(LOGERROR, "failed restore texture");
    }

    delete[] m_data;
    m_data = NULL;
    m_pitch = 0;
  }
}

void CD3DTexture::OnCreateDevice()
{
  RestoreTexture();
}

ID3D11RenderTargetView* CD3DTexture::GetRenderTargetInternal(unsigned idx)
{
  if (idx > 1)
    return nullptr;

  if (!m_texture)
    return nullptr;

  if (!DX::Windowing()->IsFormatSupport(m_format, D3D11_FORMAT_SUPPORT_RENDER_TARGET))
    return nullptr;

  if (!m_renderTargets[idx])
  {
    CD3D11_RENDER_TARGET_VIEW_DESC cRTVDesc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_UNKNOWN, 0, idx, 1);
    if (FAILED(DX::DeviceResources::Get()->GetD3DDevice()->CreateRenderTargetView(m_texture.Get(), &cRTVDesc, m_renderTargets[idx].ReleaseAndGetAddressOf())))
    {
      CLog::LogF(LOGWARNING, "cannot create texture view.");
    }
  }

  return m_renderTargets[idx].Get();
}

unsigned int CD3DTexture::GetMemoryUsage(unsigned int pitch) const
{
  switch (m_format)
  {
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC3_UNORM:
    return pitch * m_height / 4;
  default:
    return pitch * m_height;
  }
}

void CD3DTexture::GenerateMipmaps()
{
  if (m_mipLevels == 0)
  {
    ID3D11ShaderResourceView* pSRView = GetShaderResource();
    if (pSRView != nullptr)
      DX::DeviceResources::Get()->GetD3DContext()->GenerateMips(pSRView);
  }
}

// static methods
void CD3DTexture::DrawQuad(const CPoint points[4],
                           UTILS::COLOR::Color color,
                           CD3DTexture* texture,
                           const CRect* texCoords,
                           SHADER_METHOD options)
{
  unsigned numViews = 0;
  ID3D11ShaderResourceView* views = nullptr;

  if (texture)
  {
    numViews = 1;
    views = texture->GetShaderResource();
  }

  DrawQuad(points, color, numViews, &views, texCoords, options);
}

void CD3DTexture::DrawQuad(const CRect& rect,
                           UTILS::COLOR::Color color,
                           CD3DTexture* texture,
                           const CRect* texCoords,
                           SHADER_METHOD options)
{
  CPoint points[] =
  {
    { rect.x1, rect.y1 },
    { rect.x2, rect.y1 },
    { rect.x2, rect.y2 },
    { rect.x1, rect.y2 },
  };
  DrawQuad(points, color, texture, texCoords, options);
}

void CD3DTexture::DrawQuad(const CPoint points[4],
                           UTILS::COLOR::Color color,
                           unsigned numViews,
                           ID3D11ShaderResourceView** view,
                           const CRect* texCoords,
                           SHADER_METHOD options)
{
  XMFLOAT4 xcolor;
  CD3DHelper::XMStoreColor(&xcolor, color);
  CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);

  Vertex verts[4] = {
    { XMFLOAT3(points[0].x, points[0].y, 0), xcolor, XMFLOAT2(coords.x1, coords.y1), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(points[1].x, points[1].y, 0), xcolor, XMFLOAT2(coords.x2, coords.y1), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(points[2].x, points[2].y, 0), xcolor, XMFLOAT2(coords.x2, coords.y2), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(points[3].x, points[3].y, 0), xcolor, XMFLOAT2(coords.x1, coords.y2), XMFLOAT2(0.0f, 0.0f) },
  };

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();

  pGUIShader->Begin(view && numViews > 0 ? options : SHADER_METHOD_RENDER_DEFAULT);
  if (view && numViews > 0)
    pGUIShader->SetShaderViews(numViews, view);
  pGUIShader->DrawQuad(verts[0], verts[1], verts[2], verts[3]);
}

void CD3DTexture::DrawQuad(const CRect& rect,
                           UTILS::COLOR::Color color,
                           unsigned numViews,
                           ID3D11ShaderResourceView** view,
                           const CRect* texCoords,
                           SHADER_METHOD options)
{
  CPoint points[] =
  {
    { rect.x1, rect.y1 },
    { rect.x2, rect.y1 },
    { rect.x2, rect.y2 },
    { rect.x1, rect.y2 },
  };
  DrawQuad(points, color, numViews, view, texCoords, options);
}

CD3DEffect::CD3DEffect()
{
  m_effect = nullptr;
  m_techniquie = nullptr;
  m_currentPass = nullptr;
}

CD3DEffect::~CD3DEffect()
{
  Release();
}

bool CD3DEffect::Create(const std::string &effectString, DefinesMap* defines)
{
  Release();
  m_effectString = effectString;
  m_defines.clear();
  if (defines != nullptr)
    m_defines = *defines; //FIXME: is this a copy of all members?
  if (CreateEffect())
  {
    Register();
    return true;
  }
  return false;
}

void CD3DEffect::Release()
{
  Unregister();
  OnDestroyDevice(false);
}

void CD3DEffect::OnDestroyDevice(bool fatal)
{
  m_effect = nullptr;
  m_techniquie = nullptr;
  m_currentPass = nullptr;
}

void CD3DEffect::OnCreateDevice()
{
  CreateEffect();
}

HRESULT CD3DEffect::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
  XFILE::CFile includeFile;

  std::string fileName("special://xbmc/system/shaders/");
  fileName.append(pFileName);

  if (!includeFile.Open(fileName))
  {
    CLog::LogF(LOGERROR, "Could not open 3DLUT file: {}", fileName);
    return E_FAIL;
  }

  int64_t length = includeFile.GetLength();
  void *pData = malloc(length);
  if (includeFile.Read(pData, length) != length)
  {
    free(pData);
    return E_FAIL;
  }
  *ppData = pData;
  *pBytes = length;

  return S_OK;
}

HRESULT CD3DEffect::Close(LPCVOID pData)
{
  free((void*)pData);
  return S_OK;
}

bool CD3DEffect::SetFloatArray(LPCSTR handle, const float* val, unsigned int count)
{
  if (m_effect)
  {
    return S_OK == m_effect->GetVariableByName(handle)->SetRawValue(val, 0, sizeof(float) * count);
  }
  return false;
}

bool CD3DEffect::SetMatrix(LPCSTR handle, const float* mat)
{
  if (m_effect)
  {
    return S_OK == m_effect->GetVariableByName(handle)->AsMatrix()->SetMatrix(mat);
  }
  return false;
}

bool CD3DEffect::SetTechnique(LPCSTR handle)
{
  if (m_effect)
  {
    m_techniquie = m_effect->GetTechniqueByName(handle);
    if (!m_techniquie->IsValid())
      m_techniquie = nullptr;

    return nullptr != m_techniquie;
  }
  return false;
}

bool CD3DEffect::SetTexture(LPCSTR handle, CD3DTexture &texture)
{
  if (m_effect)
  {
    ID3DX11EffectShaderResourceVariable* var = m_effect->GetVariableByName(handle)->AsShaderResource();
    if (var->IsValid())
      return SUCCEEDED(var->SetResource(texture.GetShaderResource()));
  }
  return false;
}

bool CD3DEffect::SetResources(LPCSTR handle, ID3D11ShaderResourceView** ppSRViews, size_t count)
{
  if (m_effect)
  {
    ID3DX11EffectShaderResourceVariable* var = m_effect->GetVariableByName(handle)->AsShaderResource();
    if (var->IsValid())
      return SUCCEEDED(var->SetResourceArray(ppSRViews, 0, count));
  }
  return false;
}

bool CD3DEffect::SetConstantBuffer(LPCSTR handle, ID3D11Buffer *buffer)
{
  if (m_effect)
  {
    ID3DX11EffectConstantBuffer* effectbuffer = m_effect->GetConstantBufferByName(handle);
    if (effectbuffer->IsValid())
      return (S_OK == effectbuffer->SetConstantBuffer(buffer));
  }
  return false;
}

bool CD3DEffect::SetScalar(LPCSTR handle, float value)
{
  if (m_effect)
  {
    ID3DX11EffectScalarVariable* scalar = m_effect->GetVariableByName(handle)->AsScalar();
    if (scalar->IsValid())
      return (S_OK == scalar->SetFloat(value));
  }

  return false;
}

bool CD3DEffect::Begin(UINT *passes, DWORD flags)
{
  if (m_effect && m_techniquie)
  {
    D3DX11_TECHNIQUE_DESC desc = {};
    HRESULT hr = m_techniquie->GetDesc(&desc);
    *passes = desc.Passes;
    return S_OK == hr;
  }
  return false;
}

bool CD3DEffect::BeginPass(UINT pass)
{
  if (m_effect && m_techniquie)
  {
    m_currentPass = m_techniquie->GetPassByIndex(pass);
    if (!m_currentPass || !m_currentPass->IsValid())
    {
      m_currentPass = nullptr;
      return false;
    }
    return (S_OK == m_currentPass->Apply(0, DX::DeviceResources::Get()->GetD3DContext()));
  }
  return false;
}

bool CD3DEffect::EndPass()
{
  if (m_effect && m_currentPass)
  {
    m_currentPass = nullptr;
    return true;
  }
  return false;
}

bool CD3DEffect::End()
{
  if (m_effect && m_techniquie)
  {
    m_techniquie = nullptr;
    return true;
  }
  return false;
}

bool CD3DEffect::CreateEffect()
{
  HRESULT hr;
  ID3DBlob* pError = nullptr;

  std::vector<D3D_SHADER_MACRO> definemacros;

  for (const auto& it : m_defines)
  {
    D3D_SHADER_MACRO m;
    m.Name = it.first.c_str();
    if (it.second.empty())
      m.Definition = nullptr;
    else
      m.Definition = it.second.c_str();
                definemacros.push_back( m );
	}

  definemacros.push_back(D3D_SHADER_MACRO());
  definemacros.back().Name = nullptr;
  definemacros.back().Definition = nullptr;

  UINT dwShaderFlags = 0;

#ifdef _DEBUG
  //dwShaderFlags |= D3DCOMPILE_DEBUG;
  // Disable optimizations to further improve shader debugging
  //dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  hr = D3DX11CompileEffectFromMemory(m_effectString.c_str(), m_effectString.length(), "", &definemacros[0], this,
                                     dwShaderFlags, 0, DX::DeviceResources::Get()->GetD3DDevice(), m_effect.ReleaseAndGetAddressOf(), &pError);

  if(hr == S_OK)
    return true;
  else if(pError)
  {
    std::string error;
    error.assign((const char*)pError->GetBufferPointer(), pError->GetBufferSize());
    CLog::Log(LOGERROR, "CD3DEffect::CreateEffect(): {}", error);
  }
  else
    CLog::Log(LOGERROR, "CD3DEffect::CreateEffect(): call to D3DXCreateEffect() failed with {}",
              hr);
  return false;
}

CD3DBuffer::CD3DBuffer()
{
  m_length = 0;
  m_stride = 0;
  m_usage  = D3D11_USAGE_DEFAULT;
  m_format = DXGI_FORMAT_UNKNOWN;
  m_buffer = nullptr;
  m_data   = nullptr;
}

CD3DBuffer::~CD3DBuffer()
{
  Release();
  delete[] m_data;
}

bool CD3DBuffer::Create(D3D11_BIND_FLAG type, UINT count, UINT stride, DXGI_FORMAT format, D3D11_USAGE usage, const void* data)
{
  m_type = type;
  m_stride = stride ? stride : CD3DHelper::BitsPerPixel(format);
  m_format = format;
  m_length = count * m_stride;
  m_usage = usage;
  m_cpuFlags = 0;

  if (m_usage == D3D11_USAGE_DYNAMIC || m_usage == D3D11_USAGE_STAGING)
  {
    m_cpuFlags |= D3D11_CPU_ACCESS_WRITE;
    if (m_usage == D3D11_USAGE_STAGING)
      m_cpuFlags |= D3D11_CPU_ACCESS_READ;
  }

  Release();

  // create the vertex buffer
  if (CreateBuffer(data))
  {
    Register();
    return true;
  }
  return false;
}

void CD3DBuffer::Release()
{
  Unregister();
  m_buffer = nullptr;
}

bool CD3DBuffer::Map(void **data)
{
  if (m_buffer)
  {
    D3D11_MAPPED_SUBRESOURCE resource;
    if (SUCCEEDED(DX::DeviceResources::Get()->GetD3DContext()->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
    {
      *data = resource.pData;
      return true;
    }
  }
  return false;
}

bool CD3DBuffer::Unmap()
{
  if (m_buffer)
  {
    DX::DeviceResources::Get()->GetD3DContext()->Unmap(m_buffer.Get(), 0);
    return true;
  }
  return false;
}

void CD3DBuffer::OnDestroyDevice(bool fatal)
{
  if (fatal)
  {
    m_buffer = nullptr;
    return;
  }

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();

  if (!pDevice || !pContext || !m_buffer)
    return;

  D3D11_BUFFER_DESC srcDesc;
  m_buffer->GetDesc(&srcDesc);

  ComPtr<ID3D11Buffer> buffer;
  if (srcDesc.Usage != D3D11_USAGE_STAGING || 0 == (srcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ))
  {
    CD3D11_BUFFER_DESC trgDesc(srcDesc);
    trgDesc.Usage = D3D11_USAGE_STAGING;
    trgDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    trgDesc.BindFlags = 0;

    if (SUCCEEDED(pDevice->CreateBuffer(&trgDesc, NULL, &buffer)))
      pContext->CopyResource(buffer.Get(), m_buffer.Get());
  }
  else
    buffer = m_buffer;

  if (buffer != nullptr)
  {
    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pContext->Map(buffer.Get(), 0, D3D11_MAP_READ, 0, &res)))
    {
      m_data = new unsigned char[srcDesc.ByteWidth];
      memcpy(m_data, res.pData, srcDesc.ByteWidth);
      pContext->Unmap(buffer.Get(), 0);
    }
  }
  m_buffer = nullptr;
}

void CD3DBuffer::OnCreateDevice()
{
  // yay, we're back - make a new copy of the vertices
  if (!m_buffer && m_data)
  {
    CreateBuffer(m_data);
    delete[] m_data;
    m_data = nullptr;
  }
}

bool CD3DBuffer::CreateBuffer(const void* pData)
{
  CD3D11_BUFFER_DESC bDesc(m_length, m_type, m_usage, m_cpuFlags);
  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = pData;
  return (S_OK == DX::DeviceResources::Get()->GetD3DDevice()->CreateBuffer(&bDesc, (pData ? &initData : nullptr), m_buffer.ReleaseAndGetAddressOf()));
}

/****************************************************/
/*          D3D Vertex Shader Class                 */
/****************************************************/
CD3DVertexShader::CD3DVertexShader()
{
  m_VS = nullptr;
  m_vertexLayout = nullptr;
  m_VSBuffer = nullptr;
  m_inputLayout = nullptr;
  m_vertexLayoutSize = 0;
  m_inited = false;
}

CD3DVertexShader::~CD3DVertexShader()
{
  Release();
}

void CD3DVertexShader::Release()
{
  Unregister();
  ReleaseShader();
  m_VSBuffer = nullptr;
  if (m_vertexLayout)
  {
    delete[] m_vertexLayout;
    m_vertexLayout = nullptr;
  }
}

void CD3DVertexShader::ReleaseShader()
{
  UnbindShader();

  m_VS = nullptr;
  m_inputLayout = nullptr;
  m_inited = false;
}

bool CD3DVertexShader::Create(const std::wstring& vertexFile, D3D11_INPUT_ELEMENT_DESC* vertexLayout, unsigned int vertexLayoutSize)
{
  ReleaseShader();

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (!pDevice)
    return false;

  if (FAILED(D3DReadFileToBlob(vertexFile.c_str(), m_VSBuffer.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "failed to load the vertex shader.");
    return false;
  }

  if (vertexLayout && vertexLayoutSize)
  {
    m_vertexLayoutSize = vertexLayoutSize;
    m_vertexLayout = new D3D11_INPUT_ELEMENT_DESC[vertexLayoutSize];
    for (unsigned int i = 0; i < vertexLayoutSize; ++i)
      m_vertexLayout[i] = vertexLayout[i];
  }
  else
    return false;

  m_inited = CreateInternal();

  if (m_inited)
    Register();

  return m_inited;
}

bool CD3DVertexShader::Create(const void* code, size_t codeLength, D3D11_INPUT_ELEMENT_DESC* vertexLayout, unsigned int vertexLayoutSize)
{
  ReleaseShader();

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (!pDevice)
    return false;

  // trick to load bytecode into ID3DBlob
  if (FAILED(D3DStripShader(code, codeLength, D3DCOMPILER_STRIP_REFLECTION_DATA, m_VSBuffer.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "failed to load the vertex shader.");
    return false;
  }

  if (vertexLayout && vertexLayoutSize)
  {
    m_vertexLayoutSize = vertexLayoutSize;
    m_vertexLayout = new D3D11_INPUT_ELEMENT_DESC[vertexLayoutSize];
    for (unsigned int i = 0; i < vertexLayoutSize; ++i)
      m_vertexLayout[i] = vertexLayout[i];
  }
  else
    return false;

  m_inited = CreateInternal();

  if (m_inited)
    Register();

  return m_inited;
}

bool CD3DVertexShader::CreateInternal()
{
  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  CLog::LogF(LOGDEBUG, "creating vertex shader.");

  // Create the vertex shader
  if (FAILED(pDevice->CreateVertexShader(m_VSBuffer->GetBufferPointer(), m_VSBuffer->GetBufferSize(), nullptr, m_VS.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "failed to Create the vertex shader.");
    m_VSBuffer = nullptr;
    return false;
  }

  CLog::LogF(LOGDEBUG, "creating input layout.");

  if (FAILED(pDevice->CreateInputLayout(m_vertexLayout, m_vertexLayoutSize, m_VSBuffer->GetBufferPointer(), m_VSBuffer->GetBufferSize(), m_inputLayout.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "failed to create the input layout.");
    return false;
  }

  return true;
}

void CD3DVertexShader::BindShader()
{
  if (!m_inited)
    return;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  if (!pContext)
    return;

  pContext->IASetInputLayout(m_inputLayout.Get());
  pContext->VSSetShader(m_VS.Get(), nullptr, 0);
}

void CD3DVertexShader::UnbindShader()
{
  if (!m_inited)
    return;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  pContext->IASetInputLayout(nullptr);
  pContext->VSSetShader(nullptr, nullptr, 0);
}

void CD3DVertexShader::OnCreateDevice()
{
  if (m_VSBuffer && !m_VS)
    m_inited = CreateInternal();
}

void CD3DVertexShader::OnDestroyDevice(bool fatal)
{
  ReleaseShader();
}

/****************************************************/
/*           D3D Pixel Shader Class                 */
/****************************************************/
CD3DPixelShader::CD3DPixelShader()
{
  m_PS = nullptr;
  m_PSBuffer = nullptr;
  m_inited = false;
}

CD3DPixelShader::~CD3DPixelShader()
{
  Release();
}

void CD3DPixelShader::Release()
{
  Unregister();
  ReleaseShader();
  m_PSBuffer = nullptr;
}

void CD3DPixelShader::ReleaseShader()
{
  m_inited = false;
  m_PS = nullptr;
}

bool CD3DPixelShader::Create(const std::wstring& wstrFile)
{
  ReleaseShader();

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (!pDevice)
    return false;

  if (FAILED(D3DReadFileToBlob(wstrFile.c_str(), m_PSBuffer.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "Failed to load the vertex shader.");
    return false;
  }

  m_inited = CreateInternal();

  if (m_inited)
    Register();

  return m_inited;
}

bool CD3DPixelShader::Create(const void* code, size_t codeLength)
{
  ReleaseShader();

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (!pDevice)
    return false;

  // trick to load bytecode into ID3DBlob
  if (FAILED(D3DStripShader(code, codeLength, D3DCOMPILER_STRIP_REFLECTION_DATA, m_PSBuffer.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "Failed to load the vertex shader.");
    return false;
  }

  m_inited = CreateInternal();

  if (m_inited)
    Register();

  return m_inited;
}

bool CD3DPixelShader::CreateInternal()
{
  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  CLog::LogF(LOGDEBUG, "Create the pixel shader.");

  // Create the vertex shader
  if (FAILED(pDevice->CreatePixelShader(m_PSBuffer->GetBufferPointer(), m_PSBuffer->GetBufferSize(), nullptr, m_PS.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGERROR, "Failed to Create the pixel shader.");
    m_PSBuffer = nullptr;
    return false;
  }

  return true;
}

void CD3DPixelShader::BindShader()
{
  if (!m_inited)
    return;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  if (!pContext)
    return;

  pContext->PSSetShader(m_PS.Get(), nullptr, 0);
}

void CD3DPixelShader::UnbindShader()
{
  if (!m_inited)
    return;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  pContext->IASetInputLayout(nullptr);
  pContext->VSSetShader(nullptr, nullptr, 0);
}

void CD3DPixelShader::OnCreateDevice()
{
  if (m_PSBuffer && !m_PS)
    m_inited = CreateInternal();
}

void CD3DPixelShader::OnDestroyDevice(bool fatal)
{
  ReleaseShader();
}
