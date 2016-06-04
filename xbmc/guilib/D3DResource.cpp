/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifdef HAS_DX

#include "D3DResource.h"
#include "system.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"

#pragma comment(lib, "d3dcompiler.lib")

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

CD3DTexture::CD3DTexture()
{
  m_width = 0;
  m_height = 0;
  m_mipLevels = 0;
  m_usage = D3D11_USAGE_DEFAULT;
  m_format = DXGI_FORMAT_B8G8R8A8_UNORM;
  m_texture = nullptr;
  m_textureView = nullptr;
  m_renderTarget = nullptr;
  m_data = nullptr;
  m_pitch = 0;
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

  if (!g_Windowing.IsFormatSupport(format, D3D11_FORMAT_SUPPORT_TEXTURE2D))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - unsupported texture format %d", format);
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
  if (D3D11_USAGE_DEFAULT == usage && g_Windowing.IsFormatSupport(format, D3D11_FORMAT_SUPPORT_RENDER_TARGET))
    m_bindFlags |= D3D11_BIND_RENDER_TARGET;
  if ( D3D11_USAGE_STAGING != m_usage 
    && ( g_Windowing.IsFormatSupport(format, D3D11_FORMAT_SUPPORT_SHADER_LOAD)
      || g_Windowing.IsFormatSupport(format, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)))
  {
    m_bindFlags |= D3D11_BIND_SHADER_RESOURCE;
  }

  if (!CreateInternal(pixels, srcPitch))
  {
    CLog::Log(LOGERROR, "%s - failed to create texture.", __FUNCTION__);
    return false;
  }

  g_Windowing.Register(this);
  return true;
}

bool CD3DTexture::CreateInternal(const void* pixels /* nullptr */, unsigned int srcPitch /* 0 */)
{
  ID3D11Device* pD3DDevice = g_Windowing.Get3D11Device();

  CD3D11_TEXTURE2D_DESC textureDesc(m_format, m_width, m_height, 1, m_mipLevels, m_bindFlags, m_usage, m_cpuFlags, 1, 0,
    (m_mipLevels > 1) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);

  D3D11_SUBRESOURCE_DATA initData = { 0 };
  initData.pSysMem = pixels;
  initData.SysMemPitch = srcPitch ? srcPitch : CD3DHelper::BitsPerPixel(m_format) * m_width / 8;
  initData.SysMemSlicePitch = 0;

  return SUCCEEDED(pD3DDevice->CreateTexture2D(&textureDesc, (pixels ? &initData : NULL), &m_texture));
}

ID3D11ShaderResourceView* CD3DTexture::GetShaderResource()
{
  if (!m_texture)
    return nullptr;

  if (!m_textureView) 
  {
    CD3D11_SHADER_RESOURCE_VIEW_DESC cSRVDesc(D3D11_SRV_DIMENSION_TEXTURE2D);
    HRESULT hr = g_Windowing.Get3D11Device()->CreateShaderResourceView(m_texture, &cSRVDesc, &m_textureView);

    if (FAILED(hr))
    {
      CLog::Log(LOGWARNING, __FUNCTION__ " - cannot create texture view.");
      SAFE_RELEASE(m_textureView);
    }
  }

  return m_textureView;
}

ID3D11RenderTargetView* CD3DTexture::GetRenderTarget()
{
  if (!m_texture)
    return nullptr;

  if (!m_renderTarget)
  {
    CD3D11_RENDER_TARGET_VIEW_DESC cRTVDesc(D3D11_RTV_DIMENSION_TEXTURE2D);
    if (FAILED(g_Windowing.Get3D11Device()->CreateRenderTargetView(m_texture, &cRTVDesc, &m_renderTarget)))
    {
      CLog::Log(LOGWARNING, __FUNCTION__ " - cannot create texture view.");
    }
  }

  return m_renderTarget;
}

void CD3DTexture::Release()
{
  g_Windowing.Unregister(this);
  SAFE_RELEASE(m_texture);
  SAFE_RELEASE(m_textureView);
  SAFE_RELEASE(m_renderTarget);
}

bool CD3DTexture::GetDesc(D3D11_TEXTURE2D_DESC *desc)
{
  if (m_texture)
  {
    m_texture->GetDesc(desc);
    return true;
  }
  return false;
}

bool CD3DTexture::LockRect(UINT subresource, D3D11_MAPPED_SUBRESOURCE *res, D3D11_MAP mapType)
{
  if (m_texture)
  {
    if (m_usage == D3D11_USAGE_DEFAULT)
      return false;
    if ((mapType == D3D11_MAP_READ || mapType == D3D11_MAP_READ_WRITE) && m_usage == D3D11_USAGE_DYNAMIC)
      return false;

    return (S_OK == g_Windowing.GetImmediateContext()->Map(m_texture, subresource, mapType, 0, res));
  }
  return false;
}

bool CD3DTexture::UnlockRect(UINT subresource)
{
  if (m_texture)
  {
    g_Windowing.GetImmediateContext()->Unmap(m_texture, subresource);
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

    ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();

    D3D11_TEXTURE2D_DESC textureDesc;
    m_texture->GetDesc(&textureDesc);

    ID3D11Texture2D* texture = nullptr;
    if (textureDesc.Usage != D3D11_USAGE_STAGING || 0 == (textureDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ))
    {
      // create texture which can be readed by CPU - D3D11_USAGE_STAGING
      CD3D11_TEXTURE2D_DESC stagingDesc(textureDesc);
      stagingDesc.Usage = D3D11_USAGE_STAGING;
      stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      stagingDesc.BindFlags = 0;

      if (FAILED(g_Windowing.Get3D11Device()->CreateTexture2D(&stagingDesc, NULL, &texture)))
        return;

      // copy contents to new texture
      pContext->CopyResource(texture, m_texture);
    }
    else 
      texture = m_texture;

    // read data from texture
    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pContext->Map(texture, 0, D3D11_MAP_READ, 0, &res)))
    {
      m_pitch = res.RowPitch;
      unsigned int memUsage = GetMemoryUsage(res.RowPitch);
      m_data = new unsigned char[memUsage];
      memcpy(m_data, res.pData, memUsage);
      pContext->Unmap(texture, 0);
    }
    else
      CLog::Log(LOGERROR, "%s - Failed to store resource.", __FUNCTION__);

    if (texture != m_texture)
      SAFE_RELEASE(texture);
  }
}

void CD3DTexture::OnDestroyDevice()
{
  SaveTexture();
  SAFE_RELEASE(m_texture);
  SAFE_RELEASE(m_textureView);
  SAFE_RELEASE(m_renderTarget);
}

void CD3DTexture::OnLostDevice()
{
}

void CD3DTexture::RestoreTexture()
{
  // yay, we're back - make a new copy of the texture
  if (!m_texture && m_data)
  {
    if (!CreateInternal(m_data, m_pitch))
    {
      CLog::Log(LOGERROR, "%s: failed restore texture", __FUNCTION__);
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

void CD3DTexture::OnResetDevice()
{
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

// static methods
void CD3DTexture::DrawQuad(const CPoint points[4], color_t color, CD3DTexture *texture, const CRect *texCoords, SHADER_METHOD options)
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

void CD3DTexture::DrawQuad(const CRect &rect, color_t color, CD3DTexture *texture, const CRect *texCoords, SHADER_METHOD options)
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

void CD3DTexture::DrawQuad(const CPoint points[4], color_t color, unsigned numViews, ID3D11ShaderResourceView **view, const CRect *texCoords, SHADER_METHOD options)
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

  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(view && numViews > 0 ? options : SHADER_METHOD_RENDER_DEFAULT);
  if (view && numViews > 0)
    pGUIShader->SetShaderViews(numViews, view);
  pGUIShader->DrawQuad(verts[0], verts[1], verts[2], verts[3]);
}

void CD3DTexture::DrawQuad(const CRect &rect, color_t color, unsigned numViews, ID3D11ShaderResourceView **view, const CRect *texCoords, SHADER_METHOD options)
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
    g_Windowing.Register(this);
    return true;
  }
  return false;
}

void CD3DEffect::Release()
{
  g_Windowing.Unregister(this);
  OnDestroyDevice();
}

void CD3DEffect::OnDestroyDevice()
{
  SAFE_RELEASE(m_effect);
  m_techniquie = nullptr;
  m_currentPass = nullptr;
}

void CD3DEffect::OnCreateDevice()
{
  CreateEffect();
}

bool CD3DEffect::SetFloatArray(LPCSTR handle, const float* val, unsigned int count)
{
  if (m_effect)
  {
    return S_OK == m_effect->GetVariableByName(handle)->SetRawValue(val, 0, sizeof(float) * count);
  }
  return false;
}

bool CD3DEffect::SetMatrix(LPCSTR handle, const XMFLOAT4X4* mat)
{
  if (m_effect)
  {
    return S_OK == m_effect->GetVariableByName(handle)->AsMatrix()->SetMatrix((float *)mat);
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
    return (S_OK == m_currentPass->Apply(0, g_Windowing.Get3D11Context()));
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

  for( DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it )
	{
    D3D_SHADER_MACRO m;
		m.Name = it->first.c_str();
    if (it->second.empty())
      m.Definition = nullptr;
    else
		  m.Definition = it->second.c_str();
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

  hr = D3DX11CompileEffectFromMemory(m_effectString.c_str(), m_effectString.length(), "", &definemacros[0], nullptr,
                                     dwShaderFlags, 0, g_Windowing.Get3D11Device(), &m_effect, &pError);

  if(hr == S_OK)
    return true;
  else if(pError)
  {
    std::string error;
    error.assign((const char*)pError->GetBufferPointer(), pError->GetBufferSize());
    CLog::Log(LOGERROR, "CD3DEffect::CreateEffect(): %s", error.c_str());
  }
  else
    CLog::Log(LOGERROR, "CD3DEffect::CreateEffect(): call to D3DXCreateEffect() failed with %" PRId32, hr);
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
    g_Windowing.Register(this);
    return true;
  }
  return false;
}

void CD3DBuffer::Release()
{
  g_Windowing.Unregister(this);
  SAFE_RELEASE(m_buffer);
}

bool CD3DBuffer::Map(void **data)
{
  if (m_buffer)
  {
    D3D11_MAPPED_SUBRESOURCE resource;
    if (SUCCEEDED(g_Windowing.Get3D11Context()->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
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
    g_Windowing.Get3D11Context()->Unmap(m_buffer, 0);
    return true;
  }
  return false;
}

void CD3DBuffer::OnDestroyDevice()
{
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();

  if (!pDevice || !pContext || !m_buffer)
    return;

  D3D11_BUFFER_DESC srcDesc;
  m_buffer->GetDesc(&srcDesc);

  ID3D11Buffer *buffer = nullptr;
  if (srcDesc.Usage != D3D11_USAGE_STAGING || 0 == (srcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ))
  {
    CD3D11_BUFFER_DESC trgDesc(srcDesc);
    trgDesc.Usage = D3D11_USAGE_STAGING;
    trgDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    trgDesc.BindFlags = 0;

    if (FAILED(pDevice->CreateBuffer(&trgDesc, NULL, &buffer)))
      return;

    pContext->CopyResource(buffer, m_buffer);
  }
  else
    buffer = m_buffer;

  D3D11_MAPPED_SUBRESOURCE res;
  if (SUCCEEDED(pContext->Map(buffer, 0, D3D11_MAP_READ, 0, &res)))
  {
    m_data = new unsigned char[srcDesc.ByteWidth];
    memcpy(m_data, res.pData, srcDesc.ByteWidth);
    pContext->Unmap(buffer, 0);
  }

  if (buffer != m_buffer)
    SAFE_RELEASE(buffer);
  SAFE_RELEASE(m_buffer);
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
  return (S_OK == g_Windowing.Get3D11Device()->CreateBuffer(&bDesc, (pData ? &initData : nullptr), &m_buffer));
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
  g_Windowing.Unregister(this);
  ReleaseShader();
  SAFE_RELEASE(m_VSBuffer);
  SAFE_DELETE_ARRAY(m_vertexLayout);
}

void CD3DVertexShader::ReleaseShader()
{
  UnbindShader();

  SAFE_RELEASE(m_VS);
  SAFE_RELEASE(m_inputLayout);
  m_inited = false;
}

bool CD3DVertexShader::Create(const std::wstring& vertexFile, D3D11_INPUT_ELEMENT_DESC* vertexLayout, unsigned int vertexLayoutSize)
{
  ReleaseShader();

  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  if (!pDevice)
    return false;

  if (FAILED(D3DReadFileToBlob(vertexFile.c_str(), &m_VSBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to load the vertex shader.");
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
    g_Windowing.Register(this);

  return m_inited;
}

bool CD3DVertexShader::Create(const void* code, size_t codeLength, D3D11_INPUT_ELEMENT_DESC* vertexLayout, unsigned int vertexLayoutSize)
{
  ReleaseShader();

  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  if (!pDevice)
    return false;

  // trick to load bytecode into ID3DBlob
  if (FAILED(D3DStripShader(code, codeLength, D3DCOMPILER_STRIP_REFLECTION_DATA, &m_VSBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to load the vertex shader.");
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
    g_Windowing.Register(this);

  return m_inited;
}

bool CD3DVertexShader::CreateInternal()
{
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  CLog::Log(LOGDEBUG, __FUNCTION__ " - Create the vertex shader.");

  // Create the vertex shader
  if (FAILED(pDevice->CreateVertexShader(m_VSBuffer->GetBufferPointer(), m_VSBuffer->GetBufferSize(), nullptr, &m_VS)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to Create the vertex shader.");
    SAFE_RELEASE(m_VSBuffer);
    return false;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__ " - create the input layout.");

  if (FAILED(pDevice->CreateInputLayout(m_vertexLayout, m_vertexLayoutSize, m_VSBuffer->GetBufferPointer(), m_VSBuffer->GetBufferSize(), &m_inputLayout)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to create the input layout.");
    return false;
  }

  return true;
}

void CD3DVertexShader::BindShader()
{
  if (!m_inited)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  if (!pContext)
    return;

  pContext->IASetInputLayout(m_inputLayout);
  pContext->VSSetShader(m_VS, nullptr, 0);
}

void CD3DVertexShader::UnbindShader()
{
  if (!m_inited)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  pContext->IASetInputLayout(nullptr);
  pContext->VSSetShader(nullptr, nullptr, 0);
}

void CD3DVertexShader::OnCreateDevice()
{
  if (m_VSBuffer && !m_VS)
    m_inited = CreateInternal();
}

void CD3DVertexShader::OnDestroyDevice()
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
  g_Windowing.Unregister(this);
  ReleaseShader();
  SAFE_RELEASE(m_PSBuffer);
}

void CD3DPixelShader::ReleaseShader()
{
  SAFE_RELEASE(m_PS);
  m_inited = false;
}

bool CD3DPixelShader::Create(const std::wstring& wstrFile)
{
  ReleaseShader();

  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  if (!pDevice)
    return false;

  if (FAILED(D3DReadFileToBlob(wstrFile.c_str(), &m_PSBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to load the vertex shader.");
    return false;
  }

  m_inited = CreateInternal();

  if (m_inited)
    g_Windowing.Register(this);

  return m_inited;
}

bool CD3DPixelShader::Create(const void* code, size_t codeLength)
{
  ReleaseShader();

  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  if (!pDevice)
    return false;

  // trick to load bytecode into ID3DBlob
  if (FAILED(D3DStripShader(code, codeLength, D3DCOMPILER_STRIP_REFLECTION_DATA, &m_PSBuffer)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to load the vertex shader.");
    return false;
  }

  m_inited = CreateInternal();

  if (m_inited)
    g_Windowing.Register(this);

  return m_inited;
}

bool CD3DPixelShader::CreateInternal()
{
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  CLog::Log(LOGDEBUG, __FUNCTION__ " - Create the pixel shader.");

  // Create the vertex shader
  if (FAILED(pDevice->CreatePixelShader(m_PSBuffer->GetBufferPointer(), m_PSBuffer->GetBufferSize(), nullptr, &m_PS)))
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to Create the pixel shader.");
    SAFE_RELEASE(m_PSBuffer);
    return false;
  }

  return true;
}

void CD3DPixelShader::BindShader()
{
  if (!m_inited)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  if (!pContext)
    return;

  pContext->PSSetShader(m_PS, nullptr, 0);
}

void CD3DPixelShader::UnbindShader()
{
  if (!m_inited)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  pContext->IASetInputLayout(nullptr);
  pContext->VSSetShader(nullptr, nullptr, 0);
}

void CD3DPixelShader::OnCreateDevice()
{
  if (m_PSBuffer && !m_PS)
    m_inited = CreateInternal();
}

void CD3DPixelShader::OnDestroyDevice()
{
  ReleaseShader();
}

#endif
