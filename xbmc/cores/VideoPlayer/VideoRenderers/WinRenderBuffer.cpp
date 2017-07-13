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
#include "DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"
#include "utils/win32/gpu_memcpy_sse4.h"
#include "utils/CPUInfo.h"
#include "utils/win32/memcpy_sse2.h"
#include "windowing/windows/WinSystemWin32DX.h"
#include "WinRenderer.h"

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2
#define PLANE_UV 1
#define PLANE_D3D11 0

CRenderBuffer::CRenderBuffer()
  : loaded(false)
  , frameIdx(0)
  , flags(0)
  , format(BUFFER_FMT_NONE)
  , videoBuffer(nullptr)
  , m_locked(false)
  , m_bPending(false)
  , m_soft(false)
  , m_width(0)
  , m_height(0)
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
  SAFE_RELEASE(videoBuffer);
  SAFE_RELEASE(m_staging);
  for (unsigned i = 0; i < m_activePlanes; i++)
  {
    // unlock before release
    if (m_locked && m_textures[i].Get() && m_rects[i].pData)
      m_textures[i].UnlockRect(0);

    m_textures[i].Release();
    memset(&m_rects[i], 0, sizeof(D3D11_MAPPED_SUBRESOURCE));
  }
  m_activePlanes = 0;
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
    memset(m_rects[PLANE_Y].pData,    0, m_rects[PLANE_Y].RowPitch * m_height);
    memset(m_rects[PLANE_U].pData, 0x80, m_rects[PLANE_U].RowPitch * (m_height >> 1));
    memset(m_rects[PLANE_V].pData, 0x80, m_rects[PLANE_V].RowPitch * (m_height >> 1));
    break;
  case BUFFER_FMT_YUV420P10:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData),     0, m_rects[PLANE_Y].RowPitch * m_height >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_U].pData), 0x200, m_rects[PLANE_U].RowPitch * (m_height >> 1) >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_V].pData), 0x200, m_rects[PLANE_V].RowPitch * (m_height >> 1) >> 1);
    break;
  case BUFFER_FMT_YUV420P16: 
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData),      0, m_rects[PLANE_Y].RowPitch * m_height >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_U].pData), 0x8000, m_rects[PLANE_U].RowPitch * (m_height >> 1) >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_V].pData), 0x8000, m_rects[PLANE_V].RowPitch * (m_height >> 1) >> 1);
    break;
  case BUFFER_FMT_NV12:
    memset(m_rects[PLANE_Y].pData,     0, m_rects[PLANE_Y].RowPitch * m_height);
    memset(m_rects[PLANE_UV].pData, 0x80, m_rects[PLANE_UV].RowPitch * (m_height >> 1));
    break;
  case BUFFER_FMT_D3D11_BYPASS:
    break;
  case BUFFER_FMT_D3D11_NV12:
  {
    uint8_t* uvData = static_cast<uint8_t*>(m_rects[PLANE_D3D11].pData) + m_rects[PLANE_D3D11].RowPitch * m_height;
    memset(m_rects[PLANE_D3D11].pData, 0, m_rects[PLANE_D3D11].RowPitch * m_height);
    memset(uvData, 0x80, m_rects[PLANE_D3D11].RowPitch * (m_height >> 1));
    break;
  }
  case BUFFER_FMT_D3D11_P010:
  {
    wchar_t* uvData = static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData) + m_rects[PLANE_D3D11].RowPitch * (m_height >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData), 0, m_rects[PLANE_D3D11].RowPitch * m_height >> 1);
    wmemset(uvData, 0x200, m_rects[PLANE_D3D11].RowPitch * (m_height >> 1) >> 1);
    break;
  }
  case BUFFER_FMT_D3D11_P016: 
  {
    wchar_t* uvData = static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData) + m_rects[PLANE_D3D11].RowPitch * (m_height >> 1);
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_D3D11].pData), 0, m_rects[PLANE_D3D11].RowPitch * m_height >> 1);
    wmemset(uvData, 0x8000, m_rects[PLANE_D3D11].RowPitch * (m_height >> 1) >> 1);
    break;
  }
  case BUFFER_FMT_UYVY422:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData), 0x0080, m_rects[PLANE_Y].RowPitch * (m_height >> 1));
    break;
  case BUFFER_FMT_YUYV422:
    wmemset(static_cast<wchar_t*>(m_rects[PLANE_Y].pData), 0x8000, m_rects[PLANE_Y].RowPitch * (m_height >> 1));
    break;
  default:
    break;
  }
}

bool CRenderBuffer::CreateBuffer(EBufferFormat fmt, unsigned width, unsigned height, bool software)
{
  format = fmt;
  m_width = width;
  m_height = height;
  m_soft = software;

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
    if ( !m_textures[PLANE_Y].Create(m_width,      m_height,      1, usage, DXGI_FORMAT_R16_UNORM)
      || !m_textures[PLANE_U].Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R16_UNORM)
      || !m_textures[PLANE_V].Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R16_UNORM))
      return false;
    m_activePlanes = 3;
    break;
  }
  case BUFFER_FMT_YUV420P:
  {
    if ( !m_textures[PLANE_Y].Create(m_width,      m_height,      1, usage, DXGI_FORMAT_R8_UNORM)
      || !m_textures[PLANE_U].Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R8_UNORM)
      || !m_textures[PLANE_V].Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R8_UNORM))
      return false;
    m_activePlanes = 3;
    break;
  }
  case BUFFER_FMT_NV12:
  {
    DXGI_FORMAT uvFormat = DXGI_FORMAT_R8G8_UNORM;
    // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
    if (!g_Windowing.IsFormatSupport(uvFormat, D3D11_FORMAT_SUPPORT_TEXTURE2D))
      uvFormat = DXGI_FORMAT_R8G8_SNORM;
    if ( !m_textures[PLANE_Y].Create(m_width,       m_height,      1, usage, DXGI_FORMAT_R8_UNORM)
      || !m_textures[PLANE_UV].Create(m_width >> 1, m_height >> 1, 1, usage, uvFormat))
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
    m_width = FFALIGN(width, 32);
    m_height = FFALIGN(height, 32);
    if (format == BUFFER_FMT_D3D11_NV12)
      dxgi_format = DXGI_FORMAT_NV12;
    else if (format == BUFFER_FMT_D3D11_P010)
      dxgi_format = DXGI_FORMAT_P010;
    else if (format == BUFFER_FMT_D3D11_P016)
      dxgi_format = DXGI_FORMAT_P016;

    if (!m_textures[PLANE_D3D11].Create(m_width, m_height, 1, usage, dxgi_format))
      return false;

    m_activePlanes = 2;
    break;
  }
  case BUFFER_FMT_UYVY422:
  {
    if (!m_textures[PLANE_Y].Create(m_width >> 1, m_height, 1, usage, DXGI_FORMAT_B8G8R8A8_UNORM))
      return false;

    m_activePlanes = 1;
    break;
  }
  case BUFFER_FMT_YUYV422:
  {
    if (!m_textures[PLANE_Y].Create(m_width >> 1, m_height, 1, usage, DXGI_FORMAT_B8G8R8A8_UNORM))
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
    auto buf = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
    // rewrite dimension to actual values for proper usage in shaders
    m_width = buf->width;
    m_height = buf->height;
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

ID3D11View* CRenderBuffer::GetView(unsigned idx)
{
  switch (format)
  {
  case BUFFER_FMT_D3D11_BYPASS:
  {
    auto buf = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
    return buf ? buf->GetSRV(idx) : nullptr;
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

ID3D11View* CRenderBuffer::GetHWView() const
{
  auto buf = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
  return buf ? buf->view : nullptr;
}

ID3D11Resource* CRenderBuffer::GetResource(unsigned idx) const
{
  return m_textures[idx].Get();
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
  auto dxva_buffer = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
  return dxva_buffer || m_textures[0].Get();
}

void CRenderBuffer::QueueCopyBuffer()
{
  if (!videoBuffer)
    return;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD && format < BUFFER_FMT_D3D11_BYPASS)
  {
    DXVA::CDXVAOutputBuffer *buf = static_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
    CopyToStaging(reinterpret_cast<ID3D11VideoDecoderOutputView*>(buf->view));
  }
}

bool CRenderBuffer::CopyToD3D11()
{
  if (!m_locked || !m_rects[PLANE_D3D11].pData)
    return false;

  // destination
  D3D11_MAPPED_SUBRESOURCE rect = m_rects[PLANE_D3D11];
  uint8_t* pData = static_cast<uint8_t*>(rect.pData);
  uint8_t* dst[] = {pData, pData + m_textures[PLANE_D3D11].GetHeight() * rect.RowPitch};
  int dstStride[] = {rect.RowPitch, rect.RowPitch};
  // source
  uint8_t* src[3]; 
  videoBuffer->GetPlanes(src);
  int srcStrides[3];
  videoBuffer->GetStrides(srcStrides);
  
  unsigned width = m_width;
  unsigned height = m_height;

  auto buffer_format = videoBuffer->GetFormat();
  // copy to texture
  if ( buffer_format == AV_PIX_FMT_NV12
    || buffer_format == AV_PIX_FMT_P010
    || buffer_format == AV_PIX_FMT_P016 )
  {
    Concurrency::parallel_invoke([&]() {
        // copy Y
        copy_plane(src, srcStrides, height, width, dst, dstStride);
      }, [&]() {
        // copy UV
        copy_plane(&src[1], &srcStrides[1], height >> 1, width, &dst[1], &dstStride[1]);
      });
    // copy cache size of UV line again to fix Intel cache issue 
    copy_plane(&src[1], &srcStrides[1], 1, 32, &dst[1], &dstStride[1]);
  }
  // convert 8bit
  else if ( buffer_format == AV_PIX_FMT_YUV420P )
  {
    Concurrency::parallel_invoke([&]() {
        // copy Y
        copy_plane(src, srcStrides, height, width, dst, dstStride);
      }, [&]() {
        // convert U+V -> UV
        convert_yuv420_nv12_chrome(src, srcStrides, height, width, dst, dstStride);
      });
    // copy cache size of UV line again to fix Intel cache issue 
    // height and width multiplied by two because they will be divided by func
    convert_yuv420_nv12_chrome(src, srcStrides, 2, 64, dst, dstStride);
  }
  // convert 10/16bit
  else if ( buffer_format == AV_PIX_FMT_YUV420P10
         || buffer_format == AV_PIX_FMT_YUV420P16 )
  {
    uint8_t bpp = buffer_format == AV_PIX_FMT_YUV420P10 ? 10 : 16;
    Concurrency::parallel_invoke([&]() {
        // copy Y
        copy_plane(src, srcStrides, height, width, dst, dstStride, bpp);
      }, [&]() {
        // convert U+V -> UV
        convert_yuv420_p01x_chrome(src, srcStrides, height, width, dst, dstStride, bpp);
      });
    // copy cache size of UV line again to fix Intel cache issue 
    // height multiplied by two because it will be divided by func
    convert_yuv420_p01x_chrome(src, srcStrides, 2, 32, dst, dstStride, bpp);
  }
  return true;
}

bool CRenderBuffer::CopyToStaging(ID3D11View* view)
{
  HRESULT hr = S_OK;

  if (!view)
    return false;

  ID3D11VideoDecoderOutputView* pView = reinterpret_cast<ID3D11VideoDecoderOutputView*>(view);
  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vpivd;
  pView->GetDesc(&vpivd);
  ID3D11Resource* resource = nullptr;
  pView->GetResource(&resource);

  if (!m_staging)
  {
    // create staging texture
    ID3D11Texture2D* surface = nullptr;
    hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&surface));
    if (SUCCEEDED(hr))
    {
      D3D11_TEXTURE2D_DESC tDesc;
      surface->GetDesc(&tDesc);
      SAFE_RELEASE(surface);

      CD3D11_TEXTURE2D_DESC sDesc(tDesc);
      sDesc.ArraySize = 1;
      sDesc.Usage = D3D11_USAGE_STAGING;
      sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      sDesc.BindFlags = 0;

      hr = g_Windowing.Get3D11Device()->CreateTexture2D(&sDesc, nullptr, &m_staging);
      if (SUCCEEDED(hr))
        m_sDesc = sDesc;
    }
  }

  if (m_staging)
  {
    ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();
    // queue copying content from decoder texture to temporary texture.
    // actual data copying will be performed before rendering
    pContext->CopySubresourceRegion(m_staging,
                                    D3D11CalcSubresource(0, 0, 1),
                                    0, 0, 0,
                                    resource,
                                    D3D11CalcSubresource(0, vpivd.Texture2D.ArraySlice, 1),
                                    nullptr);
    m_bPending = true;
  }
  SAFE_RELEASE(resource);

  return SUCCEEDED(hr);
}

void CRenderBuffer::CopyFromStaging() const
{
  if (!m_locked)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();
  D3D11_MAPPED_SUBRESOURCE rectangle;
  if (SUCCEEDED(pContext->Map(m_staging, 0, D3D11_MAP_READ, 0, &rectangle)))
  {
    void* (*copy_func)(void* d, const void* s, size_t size) =
      ((g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_SSE4) != 0) ? gpu_memcpy : memcpy;

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
    pContext->Unmap(m_staging, 0);
  }
}

bool CRenderBuffer::CopyBuffer()
{
  AVPixelFormat format = videoBuffer->GetFormat();
  if (format == AV_PIX_FMT_D3D11VA_VLD)
  {
    if (m_bPending)
    {
      CopyFromStaging();
      m_bPending = false;
    }
    return true;
  }

  if ( format == AV_PIX_FMT_YUV420P
    || format == AV_PIX_FMT_YUV420P10
    || format == AV_PIX_FMT_YUV420P16
    || format == AV_PIX_FMT_NV12 )
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

    when_all(tasks.begin(), tasks.end()).wait();//.then([this]() { StartRender(); });
    return true;
  }
  return false;
}
