/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererSoftware.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

using namespace Microsoft::WRL;

CRendererBase* CRendererSoftware::Create(CVideoSettings& videoSettings)
{
  return new CRendererSoftware(videoSettings);
}

void CRendererSoftware::GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture)
{
  unsigned weight = 0;
  const AVPixelFormat av_pixel_format = picture.videoBuffer->GetFormat();

  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    const AVPixelFormat sw_format = GetAVFormat(GetDXGIFormat(picture));
    if (sws_isSupportedInput(sw_format))
      // double copying (GPU->CPU->(converting)->GPU)
      weight += 100;
  }
  else if (sws_isSupportedInput(av_pixel_format))
    weight += 200;

  if (weight > 0)
    weights[RENDER_SW] = weight;
}

CRendererSoftware::~CRendererSoftware()
{
  if (m_sw_scale_ctx)
  {
    sws_freeContext(m_sw_scale_ctx);
    m_sw_scale_ctx = nullptr;
  }
}

bool CRendererSoftware::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (__super::Configure(picture, fps, orientation))
  {
    if (!CreateIntermediateTarget(m_sourceWidth, m_sourceHeight, true))
      return false;

    m_format = picture.videoBuffer->GetFormat();
    if (m_format == AV_PIX_FMT_D3D11VA_VLD)
      m_format = GetAVFormat(GetDXGIFormat(picture));

    return true;
  }
  return false;
}

bool CRendererSoftware::Supports(ESCALINGMETHOD method)
{
  return method == VS_SCALINGMETHOD_AUTO
    || method == VS_SCALINGMETHOD_LINEAR;
}

void CRendererSoftware::RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags)
{
  // if creation failed
  if (!m_outputShader)
    return;

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];

  // 1. convert yuv to rgb
  m_sw_scale_ctx = sws_getCachedContext(m_sw_scale_ctx,
    buf->GetWidth(), buf->GetHeight(), buf->av_format,
    buf->GetWidth(), buf->GetHeight(), AV_PIX_FMT_BGRA,
    SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  if (!m_sw_scale_ctx)
    return;

  sws_setColorspaceDetails(m_sw_scale_ctx,
    sws_getCoefficients(buf->color_space), buf->full_range,
    sws_getCoefficients(AVCOL_SPC_BT709),  buf->full_range,
    0, 1 << 16, 1 << 16);

  uint8_t* src[YuvImage::MAX_PLANES];
  int srcStride[YuvImage::MAX_PLANES];
  buf->GetDataPlanes(src, srcStride);

  D3D11_MAPPED_SUBRESOURCE mapping;
  if (target.LockRect(0, &mapping, D3D11_MAP_WRITE_DISCARD))
  {
    uint8_t *dst[] = { static_cast<uint8_t*>(mapping.pData), nullptr, nullptr };
    int dstStride[] = { static_cast<int>(mapping.RowPitch), 0, 0 };

    sws_scale(m_sw_scale_ctx, src, srcStride, 0, std::min(target.GetHeight(), buf->GetHeight()), dst, dstStride);

    if (!target.UnlockRect(0))
      CLog::LogF(LOGERROR, "failed to unlock swtarget texture.");
  }
  else
    CLog::LogF(LOGERROR, "failed to lock swtarget texture into memory.");

  // rotate initial rect
  ReorderDrawPoints(CRect(destPoints[0], destPoints[2]), destPoints);
}

void CRendererSoftware::FinalOutput(CD3DTexture& source, CD3DTexture& target, const CRect& src, const CPoint(&destPoints)[4])
{
  m_outputShader->Render(source, src, destPoints, target, 
    DX::Windowing()->UseLimitedColor(), 
    m_videoSettings.m_Contrast * 0.01f,
    m_videoSettings.m_Brightness * 0.01f);
}

CRenderBuffer* CRendererSoftware::CreateBuffer()
{
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

CRendererSoftware::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{
}

CRendererSoftware::CRenderBufferImpl::~CRenderBufferImpl()
{
  CRenderBufferImpl::ReleasePicture();
}

void CRendererSoftware::CRenderBufferImpl::AppendPicture(const VideoPicture& picture)
{
  __super::AppendPicture(picture);

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    QueueCopyFromGPU();
    m_widthTex = m_sDesc.Width;
    m_heightTex = m_sDesc.Height;
  }
}

bool CRendererSoftware::CRenderBufferImpl::GetDataPlanes(uint8_t*(&planes)[3], int(&strides)[3])
{
  if (!videoBuffer)
    return false;

  switch (videoBuffer->GetFormat())
  {
  case AV_PIX_FMT_D3D11VA_VLD:
    planes[0] = reinterpret_cast<uint8_t*>(m_msr.pData);
    planes[1] = reinterpret_cast<uint8_t*>(m_msr.pData) + m_msr.RowPitch * m_sDesc.Height;
    strides[0] = strides[1] = m_msr.RowPitch;
    break;
  default:
    videoBuffer->GetPlanes(planes);
    videoBuffer->GetStrides(strides);
  }

  return true;
}

void CRendererSoftware::CRenderBufferImpl::ReleasePicture()
{
  if (m_staging && m_msr.pData != nullptr)
  {
    DX::DeviceResources::Get()->GetImmediateContext()->Unmap(m_staging.Get(), 0);
    m_msr = {};
  }
  __super::ReleasePicture();
}

bool CRendererSoftware::CRenderBufferImpl::UploadBuffer()
{
  if (!videoBuffer)
    return false;

  if (videoBuffer->GetFormat() != AV_PIX_FMT_D3D11VA_VLD)
  {
    m_bLoaded = true;
    return true;
  }

  if (!m_staging)
    return false;

  if (m_msr.pData == nullptr)
  {
    // map will finish copying data from GPU to CPU
    m_bLoaded = SUCCEEDED(DX::DeviceResources::Get()->GetImmediateContext()->Map(
        m_staging.Get(), 0, D3D11_MAP_READ, 0, &m_msr));
  }

  return m_bLoaded;
}
