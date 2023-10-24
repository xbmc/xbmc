/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererDXVA.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "VideoRenderers/BaseRenderer.h"
#include "VideoRenderers/HwDecRender/DXVAEnumeratorHD.h"
#include "WIN32Util.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "utils/memcpy_sse2.h"
#include "windowing/GraphicContext.h"

#include <ppl.h>

using namespace DXVA;
using namespace Microsoft::WRL;

CRendererBase* CRendererDXVA::Create(CVideoSettings& videoSettings)
{
  return new CRendererDXVA(videoSettings);
}

void CRendererDXVA::GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture)
{
  unsigned weight = 0;
  const AVPixelFormat av_pixel_format = picture.videoBuffer->GetFormat();
  const DXGI_FORMAT dxgi_format = GetDXGIFormat(av_pixel_format, __super::GetDXGIFormat(picture));

  if (dxgi_format == DXGI_FORMAT_UNKNOWN)
  {
    CLog::LogF(LOGWARNING, "Unknown texture format is not supported.",
               DX::DXGIFormatToString(dxgi_format));
    return;
  }

  if (av_pixel_format != AV_PIX_FMT_D3D11VA_VLD)
  {
    // check format for buffer
    CD3D11_TEXTURE2D_DESC texDesc(dxgi_format, FFALIGN(picture.iWidth, 32),
                                  FFALIGN(picture.iHeight, 32), 1, 1, D3D11_BIND_DECODER,
                                  D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
    if (FAILED(pDevice->CreateTexture2D(&texDesc, nullptr, nullptr)))
    {
      CLog::LogF(LOGWARNING, "Texture format {} is not supported.",
                 DX::DXGIFormatToString(dxgi_format));
      return;
    }
  }

  CEnumeratorHD enumerator;
  enumerator.Open(picture.iWidth, picture.iHeight, dxgi_format);

  if (enumerator.SupportedConversions({picture, IntendToRenderAsHDR(picture)}).empty())
  {
    CLog::LogF(LOGWARNING, "DXVA will not be used.");
    return;
  }

  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    weight += 1000;
  }
  else
  {
    if (av_pixel_format == AV_PIX_FMT_NV12 ||
        av_pixel_format == AV_PIX_FMT_P010 ||
        av_pixel_format == AV_PIX_FMT_P016)
      weight += 500; // single copying

    else if (av_pixel_format == AV_PIX_FMT_YUV420P ||
        av_pixel_format == AV_PIX_FMT_YUV420P10 ||
        av_pixel_format == AV_PIX_FMT_YUV420P16)
      weight += 400; // single copying + convert
  }

  // prefer DXVA method for interlaced HW decoded material
  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD && 
    picture.iFlags & DVP_FLAG_INTERLACED)
    weight += 1000;

  if (weight > 0)
    weights[RENDER_DXVA] = weight;
}

CRendererDXVA::CRendererDXVA(CVideoSettings& videoSettings) : CRendererHQ(videoSettings)
{
  m_renderMethodName = "DXVA";
}

CRenderInfo CRendererDXVA::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  info.m_deintMethods.push_back(VS_INTERLACEMETHOD_DXVA_AUTO);

  return  info;
}

bool CRendererDXVA::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (__super::Configure(picture, fps, orientation))
  {
    m_format = picture.videoBuffer->GetFormat();
    const DXGI_FORMAT dxgi_format = GetDXGIFormat(m_format, __super::GetDXGIFormat(picture));

    if (DX::Windowing()->IsVideoSuperResolutionSettingEnabled() &&
        CProcessorHD::IsSuperResolutionSuitable(picture))
    {
      m_tryVSR = true;
    }

    m_enumerator = std::make_shared<DXVA::CEnumeratorHD>();
    if (m_enumerator->Open(picture.iWidth, picture.iHeight, dxgi_format))
    {
      if (CServiceBroker::GetLogging().IsLogLevelLogged(LOGDEBUG) &&
          CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
      {
        m_enumerator->LogSupportedConversions(
            dxgi_format, CEnumeratorHD::AvToDxgiColorSpace(DXVA::DXGIColorSpaceArgs(picture)));
      }

      m_conversionsArgs = SupportedConversionsArgs{picture, IntendToRenderAsHDR(picture)};

      const ProcessorConversions conversions =
          m_enumerator->SupportedConversions(m_conversionsArgs);
      if (!conversions.empty())
      {
        m_conversion = ChooseConversion(conversions);

        CLog::LogF(LOGINFO, "chosen conversion: {}", m_conversion.ToString());

        // create processor
        m_processor = std::make_unique<DXVA::CProcessorHD>();
        if (m_processor->Open(picture, m_enumerator) && m_processor->SetConversion(m_conversion))
        {
          if (m_tryVSR)
            m_processor->TryEnableVideoSuperResolution();

          return true;
        }
      }
    }
    CLog::LogF(LOGERROR, "unable to create DXVA processor");
    m_processor.reset();
    m_enumerator.reset();
  }
  return false;
}

bool CRendererDXVA::NeedBuffer(int idx)
{
  if (m_renderBuffers[idx]->IsLoaded() && m_renderBuffers[idx]->pictureFlags & DVP_FLAG_INTERLACED)
  {
    if (m_renderBuffers[idx]->frameIdx + (m_processor->PastRefs() * 2u) >=
        m_renderBuffers[m_iBufferIndex]->frameIdx)
      return true;
  }

  return false;
}

void CRendererDXVA::CheckVideoParameters()
{
  __super::CheckVideoParameters();

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (m_enumerator)
  {
    const SupportedConversionsArgs args{buf->primaries, buf->color_space, buf->color_transfer,
                                        buf->full_range, ActualRenderAsHDR()};

    if (m_conversionsArgs != args)
    {
      CLog::LogF(LOGINFO, "source format change detected");

      const ProcessorConversions conversions = m_enumerator->SupportedConversions(args);
      // TODO case no supported conversion: add support in WinRenderer to fallback to a render method with support
      // For now, keep using the current conversion. Results won't be ideal but a black screen is avoided
      const ProcessorConversion conversion =
          conversions.empty() ? m_conversion : ChooseConversion(conversions);

      if (m_conversion != conversion)
      {
        CLog::LogF(LOGINFO, "new conversion: {}", conversion.ToString());

        m_processor->SetConversion(conversion);
        m_conversion = conversion;
      }
      m_conversionsArgs = args;
    }
  }

  CreateIntermediateTarget(HasHQScaler() ? m_sourceWidth : m_viewWidth,
                           HasHQScaler() ? m_sourceHeight : m_viewHeight, false,
                           m_conversion.m_outputFormat);
}

void CRendererDXVA::RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags)
{
  CRect src = sourceRect;
  CRect dst = HasHQScaler() ? sourceRect : ApplyTransforms(CRect(destPoints[0], destPoints[2]));
  const CRect trg(0.0f, 0.0f, static_cast<float>(target.GetWidth()), static_cast<float>(target.GetHeight()));

  CWIN32Util::CropSource(src, dst, trg, m_renderOrientation);

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  CRenderBuffer* views[8] = {};
  FillBuffersSet(views);

  m_processor->Render(src, dst, target.Get(), views,
                      flags, buf->frameIdx % UINT32_MAX, m_renderOrientation,
                      m_videoSettings.m_Contrast, 
                      m_videoSettings.m_Brightness);

  if (!HasHQScaler())
  {
    // change src and dest in case of dxva scale
    dst.GetQuad(destPoints);
    sourceRect = dst;
  }
}

CRect CRendererDXVA::ApplyTransforms(const CRect& destRect) const
{
  CRect result;
  CPoint rotated[4];
  ReorderDrawPoints(destRect, rotated);

  switch (m_renderOrientation)
  {
  case 90:
    result = { rotated[3], rotated[1] };
    break;
  case 180:
    result = destRect;
    break;
  case 270:
    result = { rotated[1], rotated[3] };
    break;
  default:
    result = CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(destRect);
    break;
  }

  return result;
}

void CRendererDXVA::FillBuffersSet(CRenderBuffer* (&buffers)[8])
{
  int past = 0;
  int future = 0;

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  buffers[2] = buf;

  // set future frames
  while (future < 2)
  {
    bool found = false;
    for (int i = 0; i < m_iNumBuffers; i++)
    {
      if (m_renderBuffers[i]->frameIdx == buf->frameIdx + (future * 2 + 2))
      {
        // a future frame may not be loaded yet
        if (m_renderBuffers[i]->IsLoaded() || m_renderBuffers[i]->UploadBuffer())
        {
          buffers[1 - future++] = m_renderBuffers[i];
          found = true;
          break;
        }
      }
    }
    if (!found)
      break;
  }

  // set past frames
  while (past < 4)
  {
    bool found = false;
    for (int i = 0; i < m_iNumBuffers; i++)
    {
      if (m_renderBuffers[i]->frameIdx == buf->frameIdx - (past * 2 + 2))
      {
        if (m_renderBuffers[i]->IsLoaded())
        {
          buffers[3 + past++] = m_renderBuffers[i];
          found = true;
          break;
        }
      }
    }
    if (!found)
      break;
  }
}

bool CRendererDXVA::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_BRIGHTNESS || feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_ROTATION)
  {
    if (m_processor)
      return m_processor->Supports(feature);

    return false;
  }

  return CRendererBase::Supports(feature);
}

bool CRendererDXVA::Supports(ESCALINGMETHOD method) const
{
  if (method == VS_SCALINGMETHOD_DXVA_HARDWARE)
    return true;

  return __super::Supports(method);
}

CRenderBuffer* CRendererDXVA::CreateBuffer()
{
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

std::string CRendererDXVA::GetRenderMethodDebugInfo() const
{
  if (m_processor && DX::Windowing()->SupportsVideoSuperResolution())
  {
    return StringUtils::Format("Video Super Resolution: {}",
                               m_processor->IsVideoSuperResolutionEnabled() ? "requested" : "OFF");
  }
  return {};
}

DXGI_FORMAT CRendererDXVA::GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt)
{
  return CRenderBufferImpl::GetDXGIFormat(format, default_fmt);
}

CRendererDXVA::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{
  const auto dxgi_format = GetDXGIFormat(av_pix_format);
  if (dxgi_format == DXGI_FORMAT_UNKNOWN)
    return;

  m_widthTex = FFALIGN(width, 32);
  m_heightTex = FFALIGN(height, 32);

  m_texture.Create(m_widthTex, m_heightTex, 1, D3D11_USAGE_DYNAMIC, dxgi_format);
}

CRendererDXVA::CRenderBufferImpl::~CRenderBufferImpl()
{
  CRenderBufferImpl::ReleasePicture();
}

bool CRendererDXVA::CRenderBufferImpl::UploadBuffer()
{
  if (!videoBuffer)
    return false;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    m_bLoaded = true;
    return true;
  }

  return UploadToTexture();
}

HRESULT CRendererDXVA::CRenderBufferImpl::GetResource(ID3D11Resource** ppResource, unsigned* index) const
{
  if (!ppResource)
    return E_POINTER;
  if (!index)
    return E_POINTER;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
    return __super::GetResource(ppResource, index);

  ComPtr<ID3D11Resource> pResource = m_texture.Get();
  *ppResource = pResource.Detach();
  *index = 0;

  return S_OK;
}

DXGI_FORMAT CRendererDXVA::CRenderBufferImpl::GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt)
{
  switch (format)
  {
  case AV_PIX_FMT_NV12:
  case AV_PIX_FMT_YUV420P:
    return DXGI_FORMAT_NV12;
  case AV_PIX_FMT_P010:
  case AV_PIX_FMT_YUV420P10:
    return DXGI_FORMAT_P010;
  case AV_PIX_FMT_P016:
  case AV_PIX_FMT_YUV420P16:
    return DXGI_FORMAT_P016;
  default:
    return default_fmt;
  }
}

bool CRendererDXVA::CRenderBufferImpl::UploadToTexture()
{
  D3D11_MAPPED_SUBRESOURCE rect;
  if (!m_texture.LockRect(0, &rect, D3D11_MAP_WRITE_DISCARD))
    return false;

  // destination
  uint8_t* pData = static_cast<uint8_t*>(rect.pData);
  uint8_t* dst[] = { pData, pData + m_texture.GetHeight() * rect.RowPitch };
  int dstStride[] = { static_cast<int>(rect.RowPitch), static_cast<int>(rect.RowPitch) };

  // source
  uint8_t* src[3];
  int srcStrides[3];
  videoBuffer->GetPlanes(src);
  videoBuffer->GetStrides(srcStrides);

  const unsigned width = m_width;
  const unsigned height = m_height;

  const AVPixelFormat buffer_format = videoBuffer->GetFormat();
  // copy to texture
  if (buffer_format == AV_PIX_FMT_NV12 ||
    buffer_format == AV_PIX_FMT_P010 ||
    buffer_format == AV_PIX_FMT_P016)
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
  else if (buffer_format == AV_PIX_FMT_YUV420P)
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
  else if (buffer_format == AV_PIX_FMT_YUV420P10 ||
    buffer_format == AV_PIX_FMT_YUV420P16)
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

  m_bLoaded = m_texture.UnlockRect(0);
  return m_bLoaded;
}

ProcessorConversion CRendererDXVA::ChooseConversion(const ProcessorConversions& conversions) const
{
  assert(conversions.size() > 0);

  // Try HQ except when:
  // - trying VSR scaling, which requires RGB8 processor output format
  // - the user opted out of high quality and the swap chain is 8 bits.
  if (!m_tryVSR && (DX::Windowing()->IsHighPrecisionProcessingSettingEnabled() ||
                    DX::Windowing()->GetBackBuffer().GetFormat() == DXGI_FORMAT_R10G10B10A2_UNORM))
  {
    const auto it =
        std::find_if(conversions.cbegin(), conversions.cend(), [](const ProcessorConversion& c) {
          return c.m_outputFormat == DXGI_FORMAT_R10G10B10A2_UNORM;
        });

    if (it != conversions.end())
      return *it;
    else
      CLog::LogF(LOGDEBUG, "no compatible high precision format found.");
  }

  const auto it =
      std::find_if(conversions.cbegin(), conversions.cend(), [](const ProcessorConversion& c) {
        return c.m_outputFormat == DXGI_FORMAT_B8G8R8A8_UNORM;
      });

  if (it != conversions.end())
    return *it;

  // bad situation, nothing matching our needs found, return the first conversion available
  CLog::LogF(LOGWARNING, "no conversion to wanted formats found, defaulting to first conversion.");
  return conversions.front();
}
