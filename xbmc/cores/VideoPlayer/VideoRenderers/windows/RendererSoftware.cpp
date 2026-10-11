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

#include <algorithm>
#include <cstring>

using namespace Microsoft::WRL;

namespace
{
using RowWriter = void (*)(const uint8_t* src, uint8_t* dst, unsigned width);

void CopyRow(const uint8_t* src, uint8_t* dst, unsigned width)
{
  std::memcpy(dst, src, width * sizeof(uint32_t));
}

// x2rgb10le has red in the high ten bits, R10G10B10A2 has it in the low ten
void SwapRedBlueRow(const uint8_t* src, uint8_t* dst, unsigned width)
{
  const uint32_t* in = reinterpret_cast<const uint32_t*>(src);
  uint32_t* out = reinterpret_cast<uint32_t*>(dst);
  for (unsigned x = 0; x < width; ++x)
  {
    const uint32_t pixel = in[x];
    out[x] = (pixel & 0xC00FFC00) | ((pixel >> 20) & 0x3FF) | ((pixel & 0x3FF) << 20);
  }
}

// A packed 32 bit RGB picture format and the texture format that holds it without conversion
struct DirectFormat
{
  AVPixelFormat picture;
  DXGI_FORMAT texture;
  RowWriter writeRow;
};

constexpr DirectFormat DIRECT_FORMATS[] = {
    {AV_PIX_FMT_BGR0, DXGI_FORMAT_B8G8R8X8_UNORM, CopyRow},
    {AV_PIX_FMT_X2RGB10LE, DXGI_FORMAT_R10G10B10A2_UNORM, SwapRedBlueRow},
};

const DirectFormat* FindDirectFormat(AVPixelFormat format)
{
  const auto it = std::ranges::find(DIRECT_FORMATS, format, &DirectFormat::picture);
  return it != std::end(DIRECT_FORMATS) ? &*it : nullptr;
}

void WriteDirect(const DirectFormat& format, CRenderBuffer& buf, const CD3DTexture& target)
{
  uint8_t* planes[YuvImage::MAX_PLANES];
  int strides[YuvImage::MAX_PLANES];
  buf.GetDataPlanes(planes, strides);

  D3D11_MAPPED_SUBRESOURCE mapping;
  if (!target.LockRect(0, &mapping, D3D11_MAP_WRITE_DISCARD))
  {
    CLog::LogF(LOGERROR, "failed to lock swtarget texture into memory.");
    return;
  }

  const uint8_t* src = planes[0];
  uint8_t* dst = static_cast<uint8_t*>(mapping.pData);
  const unsigned height = std::min(target.GetHeight(), buf.GetHeight());
  for (unsigned y = 0; y < height; ++y, src += strides[0], dst += mapping.RowPitch)
    format.writeRow(src, dst, buf.GetWidth());

  target.UnlockRect(0);
}
} // namespace

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

CRendererSoftware::CRendererSoftware(CVideoSettings& videoSettings) : CRendererBase(videoSettings)
{
  m_renderMethodName = "Software";
}

CRendererSoftware::~CRendererSoftware()
{
  if (m_srcFilter)
  {
    sws_freeFilter(m_srcFilter);
    m_srcFilter = nullptr;
  }

  if (m_sw_scale_ctx)
  {
    sws_freeContext(m_sw_scale_ctx);
    m_sw_scale_ctx = nullptr;
  }
  if (m_restoreMultithreadProtectedOff)
    DX::DeviceResources::Get()->SetMultithreadProtected(false);
}

bool CRendererSoftware::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (__super::Configure(picture, fps, orientation))
  {
    DXGI_FORMAT targetFormat = DXGI_FORMAT_UNKNOWN;
    const DirectFormat* direct = FindDirectFormat(picture.videoBuffer->GetFormat());
    if (direct && DX::Windowing()->IsFormatSupport(direct->texture, D3D11_FORMAT_SUPPORT_TEXTURE2D))
      targetFormat = direct->texture;

    if (!CreateIntermediateTarget(m_sourceWidth, m_sourceHeight, true, targetFormat))
      return false;

    m_format = picture.videoBuffer->GetFormat();
    if (m_format == AV_PIX_FMT_D3D11VA_VLD)
    {
      m_format = GetAVFormat(GetDXGIFormat(picture));
      if (!DX::DeviceResources::Get()->SetMultithreadProtected(true))
        m_restoreMultithreadProtectedOff = true;
    }
    return true;
  }
  return false;
}

bool CRendererSoftware::Supports(ESCALINGMETHOD method) const
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

  const DirectFormat* direct = FindDirectFormat(buf->av_format);
  if (direct && direct->texture == target.GetFormat())
  {
    WriteDirect(*direct, *buf, target);
    ReorderDrawPoints(CRect(destPoints[0], destPoints[2]), destPoints);
    return;
  }

  const AVPixelFormat dstFormat = (target.GetFormat() == DXGI_FORMAT_R10G10B10A2_UNORM)
                                      ? AV_PIX_FMT_X2BGR10LE
                                      : AV_PIX_FMT_BGRA;

  const int swsFlags = SWS_BILINEAR | SWS_FULL_CHR_H_INT | SWS_PRINT_INFO;

  // Known chroma interpolation problems of libswscale as of 7.1.100 / ffmpeg 6.0.1:
  // - 8 bit source and destination: swscale uses a "special converter" limited to nearest
  // neighbor for chroma. It's used when source size = dest size and filter lengths are 1.
  // - AV_PIX_FMT_X2BGR10 destination: full chroma interpolation is not available and there is no
  //  swscale API to retrieve the presence of support.
  //
  // - 10 bit source / 8 bit dest: "normal" scaler is used by default, with good results.
  // For simplicity assume that av_format cannot change 8 > 10 bit mid-stream (perf impact not measurable)

  if (!m_srcFilter && dstFormat == AV_PIX_FMT_BGRA &&
      (buf->av_format == AV_PIX_FMT_YUV420P || buf->av_format == AV_PIX_FMT_NV12))
  {
    m_srcFilter = sws_getDefaultFilter(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);

    sws_freeVec(m_srcFilter->lumH);
    SwsVector* vec = sws_allocVec(3);
    vec->coeff[0] = 0.0;
    vec->coeff[1] = 1.0;
    vec->coeff[2] = 0.0;
    m_srcFilter->lumH = vec;
  }

  // 1. convert yuv to rgb
  m_sw_scale_ctx = sws_getCachedContext(m_sw_scale_ctx, buf->GetWidth(), buf->GetHeight(),
                                        buf->av_format, buf->GetWidth(), buf->GetHeight(),
                                        dstFormat, swsFlags, m_srcFilter, nullptr, nullptr);

  if (!m_sw_scale_ctx)
    return;

  // swscale takes an RGB source through YUV and back, so both ways must use the same coefficients
  const bool rgb = (av_pix_fmt_desc_get(buf->av_format)->flags & AV_PIX_FMT_FLAG_RGB) != 0;

  sws_setColorspaceDetails(m_sw_scale_ctx,
    sws_getCoefficients(rgb ? AVCOL_SPC_BT709 : buf->color_space), buf->full_range,
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
  float contrast = m_videoSettings.m_Contrast * 0.01f;
  float brightness = m_videoSettings.m_Brightness * 0.01f;

  // swscale leaves the range of an RGB source as it is, so expand it here
  const CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (!buf->full_range && (av_pix_fmt_desc_get(buf->av_format)->flags & AV_PIX_FMT_FLAG_RGB))
  {
    brightness -= contrast * 2.0f * 16.0f / 219.0f;
    contrast *= 255.0f / 219.0f;
  }

  m_outputShader->Render(source, src, destPoints, target, DX::Windowing()->UseLimitedColor(),
                         contrast, brightness);
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
