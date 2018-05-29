/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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

#include <ppl.h>
#include <ppltasks.h>

#include "WinRenderBuffer.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/WinRenderer.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#if defined(HAVE_SSE2)
#include "platform/win32/utils/gpu_memcpy_sse4.h"
#endif
#include "platform/win32/utils/memcpy_sse2.h"
#include "utils/CPUInfo.h"

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2
#define PLANE_UV 1
#define PLANE_D3D11 0

static DXGI_FORMAT plane_formats[][2] =
{
  { DXGI_FORMAT_R8_UNORM,  DXGI_FORMAT_R8G8_UNORM },   // NV12
  { DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16G16_UNORM }, // P010
  { DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16G16_UNORM }  // P016
};

using namespace Microsoft::WRL;

CRenderBuffer::CRenderBuffer()
  : loaded(false)
  , frameIdx(0)
  , format(BUFFER_FMT_NONE)
  , videoBuffer(nullptr)
  , primaries(AVCOL_PRI_UNSPECIFIED)
  , color_space(AVCOL_SPC_BT709)
  , full_range(false)
  , bits(8)
  , texBits(8)
  , m_locked(false)
  , m_bPending(false)
  , m_soft(false)
  , m_width(0)
  , m_height(0)
  , m_widthTex(0)
  , m_heightTex(0)
  , m_activePlanes(0)
  , m_mapType(D3D11_MAP_WRITE_DISCARD)
  , m_staging(nullptr)
  , m_rects()
{
}

CRenderBuffer::~CRenderBuffer()
{
  Release();
}

void CRenderBuffer::Release()
{
  loaded = false;
  if (videoBuffer)
  {
    videoBuffer->Release();
    videoBuffer = nullptr;
  }
  m_staging = nullptr;
  for (unsigned i = 0; i < m_activePlanes; i++)
  {
    // unlock before release
    if (m_locked && m_textures[i].Get() && m_rects[i].pData)
      m_textures[i].UnlockRect(0);

    m_textures[i].Release();
    memset(&m_rects[i], 0, sizeof(D3D11_MAPPED_SUBRESOURCE));
  }
  m_activePlanes = 0;
  texBits = 8;
  bits = 8;

  m_planes[0] = nullptr;
  m_planes[1] = nullptr;
}

void CRenderBuffer::Lock()
{
  if (m_locked)
    return;

  m_locked = true;

  for (unsigned i = 0; i < m_activePlanes; i++)
  {
    if (!m_textures[i].Get())
      continue;

    if (m_textures[i].LockRect(0, &m_rects[i], m_mapType) == false)
    {
      memset(&m_rects[i], 0, sizeof(D3D11_MAPPED_SUBRESOURCE));
      CLog::Log(LOGERROR, "%s - failed to lock texture %d into memory", __FUNCTION__, i);
      m_locked = false;
    }
  }
}

void CRenderBuffer::Unlock()
{
  if (!m_locked)
    return;

  m_locked = false;

  for (unsigned i = 0; i < m_activePlanes; i++)
  {
    if (m_textures[i].Get() && m_rects[i].pData)
      if (!m_textures[i].UnlockRect(0))
        CLog::Log(LOGERROR, "% - failed to unlock texture %d", __FUNCTION__, i);

    memset(&m_rects[i], 0, sizeof(D3D11_MAPPED_SUBRESOURCE));
  }
}

void CRenderBuffer::Clear() const
{
  // Set Y to 0 and U,V to 128 (RGB 0,0,0) to avoid visual artifacts
  switch (format)
  {
  case BUFFER_FMT_YUV420P:
    memset(m_rects[PLANE_Y].pData,    0, m_rects[PLANE_Y].RowPitch * m_heightTex);
    memset(m_rects[PLANE_U].pData, 0x80, m_rects[PLANE_U].RowPitch * (m_heightTex >> 1));
    memset(m_rects[PLANE_V].pData, 0x80, m_rects[PLANE_V].RowPitch * (m_heightTex >> 1));
    break;
  case BUFFER_FMT_YUV420P10:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData),     0, m_rects[PLANE_Y].RowPitch * m_heightTex >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_U].pData), 0x200, m_rects[PLANE_U].RowPitch * (m_heightTex >> 1) >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_V].pData), 0x200, m_rects[PLANE_V].RowPitch * (m_heightTex >> 1) >> 1);
    break;
  case BUFFER_FMT_YUV420P16:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData),      0, m_rects[PLANE_Y].RowPitch * m_heightTex >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_U].pData), 0x8000, m_rects[PLANE_U].RowPitch * (m_heightTex >> 1) >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_V].pData), 0x8000, m_rects[PLANE_V].RowPitch * (m_heightTex >> 1) >> 1);
    break;
  case BUFFER_FMT_NV12:
    memset(m_rects[PLANE_Y].pData,     0, m_rects[PLANE_Y].RowPitch * m_heightTex);
    memset(m_rects[PLANE_UV].pData, 0x80, m_rects[PLANE_UV].RowPitch * (m_heightTex >> 1));
    break;
  case BUFFER_FMT_D3D11_BYPASS:
    break;
  case BUFFER_FMT_D3D11_NV12:
  {
    uint8_t* uvData = static_cast<uint8_t*>(m_rects[PLANE_D3D11].pData) + m_rects[PLANE_D3D11].RowPitch * m_heightTex;
    memset(m_rects[PLANE_D3D11].pData, 0, m_rects[PLANE_D3D11].RowPitch * m_heightTex);
    memset(uvData, 0x80, m_rects[PLANE_D3D11].RowPitch * (m_heightTex >> 1));
    break;
  }
  case BUFFER_FMT_D3D11_P010:
  {
    wchar_t* uvData = static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData) + m_rects[PLANE_D3D11].RowPitch * (m_heightTex >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData), 0, m_rects[PLANE_D3D11].RowPitch * m_heightTex >> 1);
    wmemset(uvData, 0x200, m_rects[PLANE_D3D11].RowPitch * (m_heightTex >> 1) >> 1);
    break;
  }
  case BUFFER_FMT_D3D11_P016:
  {
    wchar_t* uvData = static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData) + m_rects[PLANE_D3D11].RowPitch * (m_heightTex >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData), 0, m_rects[PLANE_D3D11].RowPitch * m_heightTex >> 1);
    wmemset(uvData, 0x8000, m_rects[PLANE_D3D11].RowPitch * (m_heightTex >> 1) >> 1);
    break;
  }
  case BUFFER_FMT_UYVY422:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData), 0x0080, m_rects[PLANE_Y].RowPitch * (m_heightTex >> 1));
    break;
  case BUFFER_FMT_YUYV422:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData), 0x8000, m_rects[PLANE_Y].RowPitch * (m_heightTex >> 1));
    break;
  default:
    break;
  }
}

bool CRenderBuffer::CreateBuffer(EBufferFormat fmt, unsigned width, unsigned height, bool software)
{
  format = fmt;
  m_soft = software;
  m_width = width;
  m_height = height;
  m_widthTex = width;
  m_heightTex = height;

  m_mapType = D3D11_MAP_WRITE_DISCARD;
  D3D11_USAGE usage = D3D11_USAGE_DYNAMIC;

  if (software)
  {
    m_mapType = D3D11_MAP_WRITE;
    usage = D3D11_USAGE_STAGING;
  }

  DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;

  switch (format)
  {
  case BUFFER_FMT_YUV420P10:
  case BUFFER_FMT_YUV420P16:
  {
    if ( !m_textures[PLANE_Y].Create(m_widthTex,      m_heightTex,      1, usage, DXGI_FORMAT_R16_UNORM)
      || !m_textures[PLANE_U].Create(m_widthTex >> 1, m_heightTex >> 1, 1, usage, DXGI_FORMAT_R16_UNORM)
      || !m_textures[PLANE_V].Create(m_widthTex >> 1, m_heightTex >> 1, 1, usage, DXGI_FORMAT_R16_UNORM))
      return false;
    m_activePlanes = 3;
    texBits = (format == BUFFER_FMT_YUV420P10) ? 10 : 16;
    break;
  }
  case BUFFER_FMT_YUV420P:
  {
    if ( !m_textures[PLANE_Y].Create(m_widthTex,      m_heightTex,      1, usage, DXGI_FORMAT_R8_UNORM)
      || !m_textures[PLANE_U].Create(m_widthTex >> 1, m_heightTex >> 1, 1, usage, DXGI_FORMAT_R8_UNORM)
      || !m_textures[PLANE_V].Create(m_widthTex >> 1, m_heightTex >> 1, 1, usage, DXGI_FORMAT_R8_UNORM))
      return false;
    m_activePlanes = 3;
    break;
  }
  case BUFFER_FMT_NV12:
  {
    DXGI_FORMAT uvFormat = DXGI_FORMAT_R8G8_UNORM;
    // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
    if (!DX::Windowing()->IsFormatSupport(uvFormat, D3D11_FORMAT_SUPPORT_TEXTURE2D))
      uvFormat = DXGI_FORMAT_R8G8_SNORM;
    if ( !m_textures[PLANE_Y].Create(m_widthTex,       m_heightTex,      1, usage, DXGI_FORMAT_R8_UNORM)
      || !m_textures[PLANE_UV].Create(m_widthTex >> 1, m_heightTex >> 1, 1, usage, uvFormat))
      return false;

    m_activePlanes = 2;
    break;
  }
  case BUFFER_FMT_D3D11_BYPASS:
  {
    m_activePlanes = 2;
    break;
  }
  case BUFFER_FMT_D3D11_NV12:
  case BUFFER_FMT_D3D11_P010:
  case BUFFER_FMT_D3D11_P016:
  {
    // some drivers don't allow not aligned decoder textures.
    m_widthTex = FFALIGN(width, 32);
    m_heightTex = FFALIGN(height, 32);
    if (format == BUFFER_FMT_D3D11_NV12)
      dxgi_format = DXGI_FORMAT_NV12;
    else if (format == BUFFER_FMT_D3D11_P010)
      dxgi_format = DXGI_FORMAT_P010;
    else if (format == BUFFER_FMT_D3D11_P016)
      dxgi_format = DXGI_FORMAT_P016;

    if (!m_textures[PLANE_D3D11].Create(m_widthTex, m_heightTex, 1, usage, dxgi_format))
      return false;

    m_activePlanes = 2;
    break;
  }
  case BUFFER_FMT_UYVY422:
  {
    if (!m_textures[PLANE_Y].Create(m_widthTex >> 1, m_heightTex, 1, usage, DXGI_FORMAT_B8G8R8A8_UNORM))
      return false;

    m_activePlanes = 1;
    break;
  }
  case BUFFER_FMT_YUYV422:
  {
    if (!m_textures[PLANE_Y].Create(m_widthTex >> 1, m_heightTex, 1, usage, DXGI_FORMAT_B8G8R8A8_UNORM))
      return false;

    m_activePlanes = 1;
    break;
  }
  default:
    ;
  }
  return true;
}

bool CRenderBuffer::UploadBuffer()
{
  if (!videoBuffer)
    return false;

  Lock();

  bool ret = false;
  switch (format)
  {
  case BUFFER_FMT_D3D11_BYPASS:
  {
    const auto buf = static_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
    // rewrite dimension to actual values for proper usage in shaders
    m_widthTex = buf->width;
    m_heightTex = buf->height;
    ret = true;
    break;
  }
  case BUFFER_FMT_D3D11_NV12:
  case BUFFER_FMT_D3D11_P010:
  case BUFFER_FMT_D3D11_P016:
  {
    ret = CopyToD3D11();
    break;
  }
  case BUFFER_FMT_NV12:
  case BUFFER_FMT_YUV420P:
  case BUFFER_FMT_YUV420P10:
  case BUFFER_FMT_YUV420P16:
  case BUFFER_FMT_UYVY422:
  case BUFFER_FMT_YUYV422:
  {
    ret = CopyBuffer();
    break;
  }
  default:
    break;
  }

  Unlock();

  loaded = ret;
  return loaded;
}

void CRenderBuffer::AppendPicture(const VideoPicture & picture)
{
  videoBuffer = picture.videoBuffer;
  videoBuffer->Acquire();

  primaries = static_cast<AVColorPrimaries>(picture.color_primaries);
  color_space = static_cast<AVColorSpace>(picture.color_space);
  color_transfer = static_cast<AVColorTransferCharacteristic>(picture.color_transfer);
  full_range = picture.color_range == 1;
  bits = picture.colorBits;

  hasDisplayMetadata = picture.hasDisplayMetadata;
  displayMetadata = picture.displayMetadata;
  lightMetadata = picture.lightMetadata;
  if (picture.hasLightMetadata && picture.lightMetadata.MaxCLL)
    hasLightMetadata = true;

  if (picture.videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
    QueueCopyBuffer();
  loaded = false;
}

void CRenderBuffer::ReleasePicture()
{
  if (videoBuffer)
    videoBuffer->Release();
  videoBuffer = nullptr;

  m_planes[0] = nullptr;
  m_planes[1] = nullptr;
}

HRESULT CRenderBuffer::GetResource(ID3D11Resource** ppResource, unsigned* index)
{
  if (!ppResource)
    return E_POINTER;

  if (format == BUFFER_FMT_D3D11_BYPASS)
  {
    unsigned arrayIdx = 0;
    HRESULT hr = GetDXVAResource(ppResource, &arrayIdx);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
    }
    else if (index)
    {
      *index = arrayIdx;
    }
    return hr;
  }
  else
  {
    ComPtr<ID3D11Resource> pResource = m_textures[0].Get();
    *ppResource = pResource.Detach();
    if (index)
      *index = 0;
  }
  return S_OK;
}

ID3D11View* CRenderBuffer::GetView(unsigned idx)
{
  switch (format)
  {
  case BUFFER_FMT_D3D11_BYPASS:
  {
    if (m_planes[idx])
      return m_planes[idx].Get();

    unsigned arrayIdx;
    ComPtr<ID3D11Resource> pResource;
    ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();

    HRESULT hr = GetDXVAResource(pResource.GetAddressOf(), &arrayIdx);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
      return nullptr;
    }
    auto dxva = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
    if (!dxva)
      return nullptr;

    DXGI_FORMAT plane_format = plane_formats[dxva->format - DXGI_FORMAT_NV12][idx];
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2DARRAY, plane_format,
                                             0, 1, dxva->GetIdx(), 1);
    hr = pD3DDevice->CreateShaderResourceView(pResource.Get(), &srvDesc, m_planes[idx].ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "unable to create SRV for decoder surface (%d)", plane_format);
      return nullptr;
    }

    return m_planes[idx].Get();
  }
  case BUFFER_FMT_D3D11_NV12:
  case BUFFER_FMT_D3D11_P010:
  case BUFFER_FMT_D3D11_P016:
  {
    if (format == BUFFER_FMT_D3D11_NV12)
      return m_textures[PLANE_D3D11].GetShaderResource(idx ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8_UNORM);
    return m_textures[PLANE_D3D11].GetShaderResource(idx ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R16_UNORM);

  }
  case BUFFER_FMT_NV12:
  case BUFFER_FMT_YUV420P:
  case BUFFER_FMT_YUV420P10:
  case BUFFER_FMT_YUV420P16:
  case BUFFER_FMT_UYVY422:
  case BUFFER_FMT_YUYV422:
  default:
  {
    return m_textures[idx].GetShaderResource();
  }
  }
}

void CRenderBuffer::GetDataPtr(unsigned idx, void** pData, int* pStride) const
{
  if (pData)
    *pData = m_rects[idx].pData;
  if (pStride)
    *pStride = m_rects[idx].RowPitch;
}

bool CRenderBuffer::MapPlane(unsigned idx, void** pData, int* pStride) const
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (!m_textures[idx].LockRect(0, &res, D3D11_MAP_READ))
  {
    CLog::Log(LOGERROR, "%s - failed to lock buffer textures into memory.", __FUNCTION__);
    *pData = nullptr;
    *pStride = 0;
    return false;
  }

  *pData = res.pData;
  *pStride = res.RowPitch;
  return true;
}

bool CRenderBuffer::UnmapPlane(unsigned idx) const
{
  if (!m_textures[idx].UnlockRect(0))
  {
    CLog::Log(LOGERROR, "%s - failed to unlock buffer texture.", __FUNCTION__);
    return false;
  }
  return true;
}

bool CRenderBuffer::HasPic() const
{
  const auto dxva_buffer = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
  return dxva_buffer || m_textures[0].Get();
}

void CRenderBuffer::QueueCopyBuffer()
{
  if (!videoBuffer)
    return;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD && format < BUFFER_FMT_D3D11_BYPASS)
  {
    CopyToStaging();
  }
}

bool CRenderBuffer::CopyToD3D11()
{
  if (!m_locked || !m_rects[PLANE_D3D11].pData)
    return false;

  // destination
  D3D11_MAPPED_SUBRESOURCE rect = m_rects[PLANE_D3D11];
  uint8_t* pData = static_cast<uint8_t*>(rect.pData);
  uint8_t* dst[] = {pData, pData + m_heightTex * rect.RowPitch};
  int dstStride[] = {rect.RowPitch, rect.RowPitch};
  // source
  uint8_t* src[3];
  videoBuffer->GetPlanes(src);
  int srcStrides[3];
  videoBuffer->GetStrides(srcStrides);

  const unsigned width = m_width;
  const unsigned height = m_height;

  const AVPixelFormat buffer_format = videoBuffer->GetFormat();
  // copy to texture
  if ( buffer_format == AV_PIX_FMT_NV12
    || buffer_format == AV_PIX_FMT_P010
    || buffer_format == AV_PIX_FMT_P016 )
  {
    Concurrency::parallel_invoke([&]() {
        // copy Y
        copy_plane(src[0], srcStrides[0], height, width, dst[0], dstStride[0]);
      }, [&]() {
        // copy UV
        copy_plane(src[1], srcStrides[1], height >> 1, width, dst[1], dstStride[1]);
      });
    // copy cache size of UV line again to fix Intel cache issue
    copy_plane(src[1], srcStrides[1], 1, 32, dst[1], dstStride[1]);
  }
  // convert 8bit
  else if ( buffer_format == AV_PIX_FMT_YUV420P )
  {
    Concurrency::parallel_invoke([&]() {
        // copy Y
        copy_plane(src[0], srcStrides[0], height, width, dst[0], dstStride[0]);
      }, [&]() {
        // convert U+V -> UV
        convert_yuv420_nv12_chrome(&src[1], &srcStrides[1], height, width, dst[1], dstStride[1]);
      });
    // copy cache size of UV line again to fix Intel cache issue
    // height and width multiplied by two because they will be divided by func
    convert_yuv420_nv12_chrome(&src[1], &srcStrides[1], 2, 64, dst[1], dstStride[1]);
  }
  // convert 10/16bit
  else if ( buffer_format == AV_PIX_FMT_YUV420P10
         || buffer_format == AV_PIX_FMT_YUV420P16 )
  {
    const uint8_t bpp = buffer_format == AV_PIX_FMT_YUV420P10 ? 10 : 16;
    Concurrency::parallel_invoke([&]() {
        // copy Y
        copy_plane(src[0], srcStrides[0], height, width, dst[0], dstStride[0], bpp);
      }, [&]() {
        // convert U+V -> UV
        convert_yuv420_p01x_chrome(&src[1], &srcStrides[1], height, width, dst[1], dstStride[1], bpp);
      });
    // copy cache size of UV line again to fix Intel cache issue
    // height multiplied by two because it will be divided by func
    convert_yuv420_p01x_chrome(&src[1], &srcStrides[1], 2, 32, dst[1], dstStride[1], bpp);
  }
  return true;
}

bool CRenderBuffer::CopyToStaging()
{
  unsigned index;
  ComPtr<ID3D11Resource> pResource;
  HRESULT hr = GetDXVAResource(pResource.GetAddressOf(), &index);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
    return false;
  }

  if (!m_staging)
  {
    // create staging texture
    ComPtr<ID3D11Texture2D> surface;
    if (SUCCEEDED(pResource.As(&surface)))
    {
      D3D11_TEXTURE2D_DESC tDesc;
      surface->GetDesc(&tDesc);

      CD3D11_TEXTURE2D_DESC sDesc(tDesc);
      sDesc.ArraySize = 1;
      sDesc.Usage = D3D11_USAGE_STAGING;
      sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      sDesc.BindFlags = 0;

      if (SUCCEEDED(DX::DeviceResources::Get()->GetD3DDevice()->CreateTexture2D(&sDesc, nullptr, m_staging.GetAddressOf())))
        m_sDesc = sDesc;
    }
  }

  if (m_staging)
  {
    ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
    // queue copying content from decoder texture to temporary texture.
    // actual data copying will be performed before rendering
    pContext->CopySubresourceRegion(m_staging.Get(),
                                    D3D11CalcSubresource(0, 0, 1),
                                    0, 0, 0,
                                    pResource.Get(),
                                    D3D11CalcSubresource(0, index, 1),
                                    nullptr);
    m_bPending = true;
  }

  return m_staging != nullptr;
}

void CRenderBuffer::CopyFromStaging() const
{
  if (!m_locked)
    return;

  ComPtr<ID3D11DeviceContext> pContext(DX::DeviceResources::Get()->GetImmediateContext());
  D3D11_MAPPED_SUBRESOURCE rectangle;
  if (SUCCEEDED(pContext->Map(m_staging.Get(), 0, D3D11_MAP_READ, 0, &rectangle)))
  {
    void* (*copy_func)(void* d, const void* s, size_t size) =
#if defined(HAVE_SSE2)
      ((g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_SSE4) != 0) ? gpu_memcpy :
#endif
      memcpy;

    uint8_t* s_y = static_cast<uint8_t*>(rectangle.pData);
    uint8_t* s_uv = static_cast<uint8_t*>(rectangle.pData) + m_sDesc.Height * rectangle.RowPitch;
    uint8_t* d_y = static_cast<uint8_t*>(m_rects[PLANE_Y].pData);
    uint8_t* d_uv = static_cast<uint8_t*>(m_rects[PLANE_UV].pData);

    if ( m_rects[PLANE_Y].RowPitch == rectangle.RowPitch
      && m_rects[PLANE_UV].RowPitch == rectangle.RowPitch)
    {
      Concurrency::parallel_invoke([&]() {
          // copy Y
          copy_func(d_y, s_y, rectangle.RowPitch * m_height);
        }, [&]() {
          // copy UV
          copy_func(d_uv, s_uv, rectangle.RowPitch * m_height >> 1);
        });
    }
    else
    {
      Concurrency::parallel_invoke([&]() {
          // copy Y
          for (unsigned y = 0; y < m_height; ++y)
          {
            copy_func(d_y, s_y, m_rects[PLANE_Y].RowPitch);
            s_y += rectangle.RowPitch;
            d_y += m_rects[PLANE_Y].RowPitch;
          }
        }, [&]() {
          // copy UV
          for (unsigned y = 0; y < m_height >> 1; ++y)
          {
            copy_func(d_uv, s_uv, m_rects[PLANE_UV].RowPitch);
            s_uv += rectangle.RowPitch;
            d_uv += m_rects[PLANE_UV].RowPitch;
          }
        });
    }
    pContext->Unmap(m_staging.Get(), 0);
  }
}

bool CRenderBuffer::CopyBuffer()
{
  const AVPixelFormat buffer_format = videoBuffer->GetFormat();
  if (buffer_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    if (m_bPending)
    {
      CopyFromStaging();
      m_bPending = false;
    }
    return true;
  }

  if ( buffer_format == AV_PIX_FMT_YUV420P
    || buffer_format == AV_PIX_FMT_YUV420P10
    || buffer_format == AV_PIX_FMT_YUV420P16
    || buffer_format == AV_PIX_FMT_NV12 )
  {
    uint8_t* bufData[3];
    int srcLines[3];
    videoBuffer->GetPlanes(bufData);
    videoBuffer->GetStrides(srcLines);
    std::vector<Concurrency::task<void>> tasks;

    for (unsigned plane = 0; plane < m_activePlanes; ++plane)
    {
      uint8_t* dst = static_cast<uint8_t*>(m_rects[plane].pData);
      uint8_t* src = bufData[plane];
      int srcLine = srcLines[plane];
      int dstLine = m_rects[plane].RowPitch;
      int height = plane == 0 ? m_height : m_height >> 1;

      auto task = Concurrency::create_task([src, dst, srcLine, dstLine, height]()
      {
        if (srcLine == dstLine)
        {
          memcpy(dst, src, srcLine * height);
        }
        else
        {
          uint8_t* s = src;
          uint8_t* d = dst;
          for (int i = 0; i < height; ++i)
          {
            memcpy(d, s, std::min(srcLine, dstLine));
            d += dstLine;
            s += srcLine;
          }
        }
      });
      tasks.push_back(task);
    }

    // event based await is required on WinRT because
    // blocking WinRT STA threads with task.wait() isn't allowed
    auto sync = std::make_shared<Concurrency::event>();
    when_all(tasks.begin(), tasks.end()).then([&sync]() {
      sync->set();
    });
    sync->wait();
    return true;
  }
  return false;
}

HRESULT CRenderBuffer::GetDXVAResource(ID3D11Resource** ppResource, unsigned* arrayIdx)
{
  if (!ppResource)
    return E_POINTER;
  if (!arrayIdx)
    return E_POINTER;

  auto dxva = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
  if (!dxva)
    return E_UNEXPECTED;

  ComPtr<ID3D11Resource> pResource;
  HRESULT hr;
  if (dxva->shared)
  {
    HANDLE sharedHandle = dxva->GetHandle();
    if (sharedHandle == INVALID_HANDLE_VALUE)
      return E_HANDLE;

    ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
    hr = pD3DDevice->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), reinterpret_cast<void**>(pResource.GetAddressOf()));
  }
  else
  {
    if (dxva->view)
    {
      dxva->view->GetResource(&pResource);
      hr = S_OK;
    }
    else
      hr = E_UNEXPECTED;
  }

  if (SUCCEEDED(hr))
  {
    *ppResource = pResource.Detach();
    *arrayIdx = dxva->GetIdx();
  }

  return hr;
}
