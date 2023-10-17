/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererShaders.h"

#include "DVDCodecs/Video/DXVA.h"
#include "rendering/dx/RenderContext.h"
#include "utils/CPUInfo.h"
#ifndef _M_ARM
  #include "utils/gpu_memcpy_sse4.h"
#endif
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <ppl.h>

using namespace Microsoft::WRL;
static DXGI_FORMAT plane_formats[][2] =
{
  { DXGI_FORMAT_R8_UNORM,  DXGI_FORMAT_R8G8_UNORM },   // NV12
  { DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16G16_UNORM }, // P010
  { DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16G16_UNORM }  // P016
};

CRendererBase* CRendererShaders::Create(CVideoSettings& videoSettings)
{
  return new CRendererShaders(videoSettings);
}

void CRendererShaders::GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture)
{
  unsigned weight = 0;
  const AVPixelFormat av_pixel_format = picture.videoBuffer->GetFormat();

  switch (av_pixel_format)
  {
    case AV_PIX_FMT_D3D11VA_VLD:
      if (IsHWPicSupported(picture))
        weight += 1000; // support natively
      else
        weight += 200; // double copying (GPU->CPU->GPU)
      break;
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_NV12:
      weight += 500; // single copying
      break;
    case AV_PIX_FMT_YUV420P10:
    case AV_PIX_FMT_YUV420P16:
      if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R16_UNORM, D3D11_FORMAT_SUPPORT_TEXTURE2D))
        weight += 500; // single copying
      else
        CLog::LogF(LOGWARNING, "Texture format DXGI_FORMAT_R16_UNORM is not supported.");
      break;
    case AV_PIX_FMT_P010:
    case AV_PIX_FMT_P016:
      if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R16_UNORM, D3D11_FORMAT_SUPPORT_TEXTURE2D) &&
          DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R16G16_UNORM,
                                           D3D11_FORMAT_SUPPORT_TEXTURE2D))
        weight += 500; // single copying
      else
        CLog::LogF(LOGWARNING, "Texture format R16_UNORM / R16G16_UNORM is not supported.");
      break;
  }

  if (weight > 0)
    weights[RENDER_PS] = weight;
}

CRendererShaders::CRendererShaders(CVideoSettings& videoSettings) : CRendererHQ(videoSettings)
{
  m_renderMethodName = "Pixel Shaders";
}

bool CRendererShaders::Supports(ESCALINGMETHOD method) const
{
  if (method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return __super::Supports(method);
}

bool CRendererShaders::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (__super::Configure(picture, fps, orientation))
  {
    m_format = picture.videoBuffer->GetFormat();
    if (m_format == AV_PIX_FMT_D3D11VA_VLD)
    {
      const DXGI_FORMAT dxgi_format = GetDXGIFormat(picture);

      // if decoded texture isn't supported in shaders
      // then change format to supported via copying
      if (!IsHWPicSupported(picture))
        m_format = GetAVFormat(dxgi_format);
    }
    m_srcPrimaries = picture.color_primaries;

    CreateIntermediateTarget(m_sourceWidth, m_sourceHeight, false, CalcIntermediateTargetFormat());
    return true;
  }
  return false;
}

void CRendererShaders::RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags)
{
  if (!m_colorShader)
    return;

  // reset scissors and viewport
  CD3D11_VIEWPORT viewPort(0.0f, 0.0f,
    static_cast<float>(target.GetWidth()),
    static_cast<float>(target.GetHeight()));
  DX::DeviceResources::Get()->GetD3DContext()->RSSetViewports(1, &viewPort);
  DX::Windowing()->ResetScissors();

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];

  CPoint srcPoints[4];
  sourceRect.GetQuad(srcPoints);

  m_colorShader->SetParams(m_videoSettings.m_Contrast, m_videoSettings.m_Brightness, 
                           DX::Windowing()->UseLimitedColor());
  m_colorShader->SetColParams(buf->color_space, buf->bits, !buf->full_range, buf->texBits);
  m_colorShader->Render(sourceRect, srcPoints, buf, target);

  if (!HasHQScaler())
    ReorderDrawPoints(CRect(destPoints[0], destPoints[2]), destPoints);
}

void CRendererShaders::CheckVideoParameters()
{
  __super::CheckVideoParameters();

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (buf->primaries != m_srcPrimaries)
  {
    // source params is changed, reset shader
    m_srcPrimaries = buf->primaries;
    m_colorShader.reset();
  }
}

void CRendererShaders::UpdateVideoFilters()
{
  __super::UpdateVideoFilters();

  if (!m_colorShader)
  {
    m_colorShader = std::make_unique<CYUV2RGBShader>();

    AVColorPrimaries dstPrimaries = AVCOL_PRI_BT709;

    if (DX::Windowing()->IsHDROutput() &&
        (m_srcPrimaries == AVCOL_PRI_BT709 || m_srcPrimaries == AVCOL_PRI_BT2020))
      dstPrimaries = m_srcPrimaries;

    if (!m_colorShader->Create(m_format, dstPrimaries, m_srcPrimaries))
    {
      // we are in a big trouble
      CLog::LogF(LOGERROR, "unable to create YUV->RGB shader, rendering is not possible");
      m_colorShader.reset();
    }
  }
}

bool CRendererShaders::IsHWPicSupported(const VideoPicture& picture)
{
  // checking support of decoder texture in shaders
  const DXGI_FORMAT dxgi_format = GetDXGIFormat(picture);
  if (dxgi_format != DXGI_FORMAT_UNKNOWN)
  {
    CD3D11_TEXTURE2D_DESC texDesc(
      dxgi_format,
      FFALIGN(picture.iWidth, 32),
      FFALIGN(picture.iHeight, 32),
      1, 1,
      D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE,
      D3D11_USAGE_DEFAULT
    );

    ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
    return SUCCEEDED(pDevice->CreateTexture2D(&texDesc, nullptr, nullptr));
  }
  return false;
}

DXGI_FORMAT CRendererShaders::CalcIntermediateTargetFormat() const
{
  // Default value: same as the back buffer
  DXGI_FORMAT format{DX::Windowing()->GetBackBuffer().GetFormat()};

  // High precision setting not enabled: use back buffer format, 8 or 10 bits
  // enabled: look for higher quality format
  if (!DX::Windowing()->IsHighPrecisionProcessingSettingEnabled())
    return format;

  UINT reqSupport{D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_RENDER_TARGET};

  // Preferred: float16 as the yuv-rgb conversion takes place in float
  // => avoids a conversion / quantization round-trip, but uses more memory / bandwidth
  const std::array hdrformats{DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM,
                              DXGI_FORMAT_R32G32B32A32_FLOAT};

  const auto it =
      std::find_if(hdrformats.cbegin(), hdrformats.cend(), [&](DXGI_FORMAT outputFormat) {
        return DX::Windowing()->IsFormatSupport(outputFormat, reqSupport);
      });

  if (it != hdrformats.cend())
    format = *it;
  else
    CLog::LogF(LOGDEBUG, "no compatible high precision format found.");

  return format;
}

CRenderBuffer* CRendererShaders::CreateBuffer()
{
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

CRendererShaders::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{
  DXGI_FORMAT view_formats[YuvImage::MAX_PLANES] = {};

  switch (av_format)
  {
  case AV_PIX_FMT_D3D11VA_VLD:
    m_viewCount = 2;
    break;
  case AV_PIX_FMT_NV12:
  {
    view_formats[0] = DXGI_FORMAT_R8_UNORM;
    view_formats[1] = DXGI_FORMAT_R8G8_UNORM;
    // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
    if (!DX::Windowing()->IsFormatSupport(view_formats[1], D3D11_FORMAT_SUPPORT_TEXTURE2D))
      view_formats[1] = DXGI_FORMAT_R8G8_SNORM;
    m_viewCount = 2;
    break;
  }
  case AV_PIX_FMT_P010:
  case AV_PIX_FMT_P016:
  {
    view_formats[0] = DXGI_FORMAT_R16_UNORM;
    view_formats[1] = DXGI_FORMAT_R16G16_UNORM;
    m_viewCount = 2;
    break;
  }
  case AV_PIX_FMT_YUV420P:
  {
    view_formats[0] = view_formats[1] = view_formats[2] = DXGI_FORMAT_R8_UNORM;
    m_viewCount = 3;
    break;
  }
  case AV_PIX_FMT_YUV420P10:
  case AV_PIX_FMT_YUV420P16:
  {
    view_formats[0] = view_formats[1] = view_formats[2] = DXGI_FORMAT_R16_UNORM;
    m_viewCount = 3;
    texBits = av_format == AV_PIX_FMT_YUV420P10 ? 10 : 16;
    break;
  }
  default:
    // unsupported format
    return;
  }

  if (av_format != AV_PIX_FMT_D3D11VA_VLD)
  {
    for (size_t i = 0; i < m_viewCount; i++)
    {
      const auto w = i ? m_width >> 1 : m_width;
      const auto h = i ? m_height >> 1 : m_height;

      if (!m_textures[i].Create(w, h, 1, D3D11_USAGE_DYNAMIC, view_formats[i]))
        break;

      // clear plane
      D3D11_MAPPED_SUBRESOURCE mapping = {};
      if (m_textures[i].LockRect(0, &mapping, D3D11_MAP_WRITE_DISCARD))
      {
        if (view_formats[i] == DXGI_FORMAT_R8_UNORM ||
          view_formats[i] == DXGI_FORMAT_R8G8_UNORM ||
          view_formats[i] == DXGI_FORMAT_R8G8_SNORM)
          memset(mapping.pData, i ? 0x80 : 0, mapping.RowPitch * h);
        else
          wmemset(static_cast<wchar_t*>(mapping.pData), i ? 0x8000 : 0, mapping.RowPitch * h >> 1);

        if (m_textures[i].UnlockRect(0)) {}
      }
    }
  }
}

CRendererShaders::CRenderBufferImpl::~CRenderBufferImpl()
{
  CRenderBufferImpl::ReleasePicture();
}

void CRendererShaders::CRenderBufferImpl::AppendPicture(const VideoPicture& picture)
{
  __super::AppendPicture(picture);

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    if (AV_PIX_FMT_D3D11VA_VLD != av_format)
      QueueCopyFromGPU();

    const auto hw = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer);
    m_widthTex = hw->width;
    m_heightTex = hw->height;
  }
}

bool CRendererShaders::CRenderBufferImpl::UploadBuffer()
{
  if (!videoBuffer)
    return false;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    if (AV_PIX_FMT_D3D11VA_VLD == av_format)
      m_bLoaded = true;
    else
      m_bLoaded = UploadFromGPU();
  }
  else
    m_bLoaded = UploadFromBuffer();

  return m_bLoaded;
}

unsigned CRendererShaders::CRenderBufferImpl::GetViewCount() const
{
  return m_viewCount;
}

ID3D11View* CRendererShaders::CRenderBufferImpl::GetView(unsigned viewIdx)
{
  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD &&
    AV_PIX_FMT_D3D11VA_VLD == av_format)
  {
    if (m_planes[viewIdx])
      return m_planes[viewIdx].Get();

    unsigned arrayIdx;
    ComPtr<ID3D11Resource> pResource;
    if (FAILED(GetResource(&pResource, &arrayIdx)))
    {
      CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
      return nullptr;
    }

    const auto dxva_format = CRendererBase::GetDXGIFormat(videoBuffer);
    // impossible but we check
    if (dxva_format < DXGI_FORMAT_NV12 || dxva_format > DXGI_FORMAT_P016)
      return nullptr;

    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(
      D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
      plane_formats[dxva_format - DXGI_FORMAT_NV12][viewIdx],
      0, 1, arrayIdx, 1
    );

    ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
    if (FAILED(pD3DDevice->CreateShaderResourceView(pResource.Get(), &srvDesc, &m_planes[viewIdx])))
    {
      CLog::LogF(LOGERROR, "unable to create shader target for decoder texture.");
      return nullptr;
    }

    return m_planes[viewIdx].Get();
  }

  return m_textures[viewIdx].GetShaderResource();
}

void CRendererShaders::CRenderBufferImpl::ReleasePicture()
{
  __super::ReleasePicture();

  m_planes[0] = nullptr;
  m_planes[1] = nullptr;
}

bool CRendererShaders::CRenderBufferImpl::UploadFromGPU()
{
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
  D3D11_MAPPED_SUBRESOURCE mapGPU;
  D3D11_MAPPED_SUBRESOURCE mappings[2];

  if (FAILED(pContext->Map(m_staging.Get(), 0, D3D11_MAP_READ, 0, &mapGPU)))
    return false;

  if (!m_textures[PLANE_Y].LockRect(0, &mappings[PLANE_Y], D3D11_MAP_WRITE_DISCARD) ||
    !m_textures[PLANE_UV].LockRect(0, &mappings[PLANE_UV], D3D11_MAP_WRITE_DISCARD))
  {
    pContext->Unmap(m_staging.Get(), 0);
    return false;
  }

  void* (*copy_func)(void* d, const void* s, size_t size) =
#if defined(HAVE_SSE2)
      ((CServiceBroker::GetCPUInfo()->GetCPUFeatures() & CPU_FEATURE_SSE4) != 0) ? gpu_memcpy :
#endif
                                                                                 memcpy;

  auto* s_y = static_cast<uint8_t*>(mapGPU.pData);
  auto* s_uv = static_cast<uint8_t*>(mapGPU.pData) + m_sDesc.Height * mapGPU.RowPitch;
  auto* d_y = static_cast<uint8_t*>(mappings[PLANE_Y].pData);
  auto* d_uv = static_cast<uint8_t*>(mappings[PLANE_UV].pData);

  if (mappings[PLANE_Y].RowPitch == mapGPU.RowPitch
    && mappings[PLANE_UV].RowPitch == mapGPU.RowPitch)
  {
    Concurrency::parallel_invoke([&]() {
      // copy Y
      copy_func(d_y, s_y, mapGPU.RowPitch * m_height);
    }, [&]() {
      // copy UV
      copy_func(d_uv, s_uv, mapGPU.RowPitch * m_height >> 1);
    });
  }
  else
  {
    Concurrency::parallel_invoke([&]() {
      // copy Y
      for (unsigned y = 0; y < m_height; ++y)
      {
        copy_func(d_y, s_y, mappings[PLANE_Y].RowPitch);
        s_y += mapGPU.RowPitch;
        d_y += mappings[PLANE_Y].RowPitch;
      }
    }, [&]() {
      // copy UV
      for (unsigned y = 0; y < m_height >> 1; ++y)
      {
        copy_func(d_uv, s_uv, mappings[PLANE_UV].RowPitch);
        s_uv += mapGPU.RowPitch;
        d_uv += mappings[PLANE_UV].RowPitch;
      }
    });
  }
  pContext->Unmap(m_staging.Get(), 0);

  return m_textures[PLANE_Y].UnlockRect(0) &&
    m_textures[PLANE_UV].UnlockRect(0);
}

bool CRendererShaders::CRenderBufferImpl::UploadFromBuffer() const
{
  uint8_t* bufData[3];
  int srcLines[3];
  videoBuffer->GetPlanes(bufData);
  videoBuffer->GetStrides(srcLines);

  for (unsigned plane = 0; plane < m_viewCount; ++plane)
  {
    D3D11_MAPPED_SUBRESOURCE mapping = {};
    if (!m_textures[plane].LockRect(0, &mapping, D3D11_MAP_WRITE_DISCARD))
      break;

    auto* dst = static_cast<uint8_t*>(mapping.pData);
    auto* src = bufData[plane];
    int srcLine = srcLines[plane];
    int dstLine = mapping.RowPitch;
    int height = plane ? m_height >> 1 : m_height;

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
  }

  for (unsigned plane = 0; plane < m_viewCount; ++plane)
    if (!m_textures[plane].UnlockRect(0)) {}

  return true;
}
