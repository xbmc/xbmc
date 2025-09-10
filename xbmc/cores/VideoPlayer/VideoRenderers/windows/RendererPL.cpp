/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererPL.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "VideoRenderers/BaseRenderer.h"
#include "VideoRenderers/HwDecRender/DXVAEnumeratorHD.h"
#include "WIN32Util.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "utils/memcpy_sse2.h"
#include "windowing/GraphicContext.h"
#include "PLHelper.h"
#include <ppl.h>


using namespace Microsoft::WRL;


CRendererBase* CRendererPL::Create(CVideoSettings& videoSettings)
{
  return new CRendererPL(videoSettings);
}

void CRendererPL::GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture)
{
  unsigned weight = 0;
  const AVPixelFormat av_pixel_format = picture.videoBuffer->GetFormat();

  // Support common YUV formats for libplacebo
  if (av_pixel_format == AV_PIX_FMT_YUV420P ||
    av_pixel_format == AV_PIX_FMT_YUV420P10 ||
    av_pixel_format == AV_PIX_FMT_YUV420P16 ||
    av_pixel_format == AV_PIX_FMT_NV12 ||
    av_pixel_format == AV_PIX_FMT_P010 ||
    av_pixel_format == AV_PIX_FMT_P016)
  {
    weight += 800; // High priority for libplacebo
  }
  else if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    weight += 700; // Still support hardware decoded content
  }

  if (weight > 0)
    weights[RENDER_LIBPLACEBO] = weight;
}

CRendererPL::CRendererPL(CVideoSettings& videoSettings) : CRendererHQ(videoSettings)
{
  m_renderMethodName = "LibPlacebo";
  m_colorSpace = {};
  m_chromaLocation = PL_CHROMA_UNKNOWN;
}

CRenderInfo CRendererPL::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  info.m_deintMethods.push_back(VS_INTERLACEMETHOD_AUTO);

  return  info;
}

bool CRendererPL::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (!__super::Configure(picture, fps, orientation))
    return false;

    //Log initiation
    

    PL::PLInstance::Get()->Init();
    // Set up color space based on picture metadata
    m_colorSpace = pl_color_space{
      .primaries = pl_primaries_from_av(picture.color_primaries),
      .transfer = pl_transfer_from_av(picture.color_transfer),
    };
    m_chromaLocation = pl_chroma_from_av(picture.chroma_position);
  
  return true;
}

void CRendererPL::CheckVideoParameters()
{
  __super::CheckVideoParameters();

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (buf)
  {
    // Check if color space parameters have changed
    if (buf->color_space != m_lastColorSpace ||
      buf->color_transfer != m_lastColorTransfer ||
      buf->primaries != m_lastPrimaries)
    {
      m_lastColorSpace = buf->color_space;
      m_lastColorTransfer = buf->color_transfer;
      m_lastPrimaries = buf->primaries;
      m_colorSpace = pl_color_space{
        .primaries = pl_primaries_from_av(buf->primaries),
        .transfer = pl_transfer_from_av(buf->color_transfer),
      };
    }
  }
  CreateIntermediateTarget(m_viewWidth, m_viewHeight, false, DXGI_FORMAT_R10G10B10A2_UNORM);
}

void CRendererPL::RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags)
{

  CRect dst = CRect(destPoints[0], destPoints[2]);
  CPoint rotated[4];

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (!buf || !buf->IsLoaded())
    return;

  CRenderBufferImpl* buffer = static_cast<CRenderBufferImpl*>(buf);

  pl_frame frameOut{};
  pl_frame frameIn{};

  pl_d3d11_wrap_params outputParams{};

  if (!buffer->GetLibplaceboFrame(frameIn))
    return;

  if (frameIn.color.primaries != PL_COLOR_SYSTEM_DOLBYVISION)
  {
    //if not dovi set the color space setted during config
    frameIn.color = m_colorSpace;
  }
  

  //Add icc profile
  //add rotate

  outputParams.w = target.GetWidth();
  outputParams.h = target.GetHeight();
  outputParams.fmt = target.GetFormat();
  outputParams.tex = target.Get();
  outputParams.array_slice = 1;
  
  pl_tex interTexture = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &outputParams);

  frameOut.num_planes = 1;
  frameOut.planes[0].texture = interTexture;
  frameOut.planes[0].components = 4;
  frameOut.planes[0].component_mapping[0] = PL_CHANNEL_R;
  frameOut.planes[0].component_mapping[1] = PL_CHANNEL_G;
  frameOut.planes[0].component_mapping[2] = PL_CHANNEL_B;
  frameOut.planes[0].component_mapping[3] = PL_CHANNEL_A;
  
  frameOut.crop.x1 = dst.Width();
  frameOut.crop.y1 = dst.Height();
  
  frameOut.color = frameIn.color;
  
  if (DX::DeviceResources::Get()->IsTransferPQ())
  {
    frameOut.color.transfer = PL_COLOR_TRC_PQ;
    frameOut.color.primaries = PL_COLOR_PRIM_BT_2020;
  }
  else
  {
    frameOut.color.primaries = PL_COLOR_PRIM_BT_709;
    frameOut.color.transfer = PL_COLOR_TRC_UNKNOWN;
  }
  frameOut.repr.sys = PL_COLOR_SYSTEM_RGB;

  pl_render_params params;
  params = pl_render_default_params;
  //Would need to make the target clearable first
  params.skip_target_clearing = true;
  pl_frame_set_chroma_location(&frameIn, m_chromaLocation);
  bool res = pl_render_image(PL::PLInstance::Get()->GetRenderer(), &frameIn, &frameOut, &params);

  sourceRect = dst;
}

void CRendererPL::ProcessHDR(CRenderBuffer* rb)
{
  if (m_AutoSwitchHDR && rb->primaries == AVCOL_PRI_BT2020 &&
    (rb->color_transfer == AVCOL_TRC_SMPTE2084 || rb->color_transfer == AVCOL_TRC_ARIB_STD_B67) &&
    !DX::Windowing()->IsHDROutput())
  {
    DX::Windowing()->ToggleHDR(); // Toggle display HDR ON
  }

  if (!DX::Windowing()->IsHDROutput())
  {
    if (m_HdrType != HDR_TYPE::HDR_NONE_SDR)
    {
      m_HdrType = HDR_TYPE::HDR_NONE_SDR;
      m_lastHdr10 = {};
    }
    return;
  }
  CRenderBufferImpl* rbpl = static_cast<CRenderBufferImpl*>(rb);
  
  pl_swapchain_colorspace_hint(PL::PLInstance::Get()->GetSwapchain(), &rbpl->hdrColorSpace);

}

bool CRendererPL::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_BRIGHTNESS || feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_ROTATION)
  {
#if TODO
#endif

    return false;
  }

  return CRendererBase::Supports(feature);
}

bool CRendererPL::Supports(ESCALINGMETHOD method) const
{
#if TODO
#endif

  return __super::Supports(method);
}

CRenderBuffer* CRendererPL::CreateBuffer()
{
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

CRendererPL::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{

  m_widthTex = FFALIGN(width, 32);
  m_heightTex = FFALIGN(height, 32);
  
}

CRendererPL::CRenderBufferImpl::~CRenderBufferImpl()
{
  CRenderBufferImpl::ReleasePicture();
}


void CRendererPL::CRenderBufferImpl::AppendPicture(const VideoPicture& picture)
{
  __super::AppendPicture(picture);
  hdrDoviRpu = picture.hdrDoviRpu;
  hdrMetadata = picture.hdrMetadata;
  doviMetadata = picture.doviMetadata;
  hdrColorSpace = picture.doviColorSpace;
  doviColorRepr = picture.doviColorRepr;
  doviPlMetadata = picture.doviPlMetadata;
  hdrDoviRpu = picture.hdrDoviRpu;
  hasDoviMetadata = picture.hasDoviMetadata;
  hasDoviRpuMetadata = picture.hasDoviRpuMetadata;
  hasHDR10PlusMetadata = picture.hasHDR10PlusMetadata;
}

bool CRendererPL::CRenderBufferImpl::GetLibplaceboFrame(pl_frame& frame)
{
  if (!m_bLoaded)
    return false;
  pl_color_repr crpr{};

  if (hasDoviMetadata)
  {
    crpr = doviColorRepr;
    frame.color = hdrColorSpace;
  }

  if (hasHDR10PlusMetadata)
  {
    pl_av_hdr_metadata metadata = {};
    pl_hdr_metadata out = {};
    metadata.clm = &lightMetadata;
    metadata.mdm = &displayMetadata;
    metadata.dhp = &hdrMetadata;
    
    pl_map_hdr_metadata(&hdrColorSpace.hdr, &metadata);
    frame.color.hdr = hdrColorSpace.hdr;

    
  }
  //set sample dep and others
  crpr.bits = plbits;
  frame.repr = crpr;
  frame.num_planes = 3;
  frame.planes[0] = plplanes[0];
  frame.planes[1] = plplanes[1];
  frame.planes[2] = plplanes[2];
  
  return true;
}
bool CRendererPL::CRenderBufferImpl::UploadBuffer()
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

bool CRendererPL::CRenderBufferImpl::UploadToTexture()
{

  // source
  uint8_t* src[3];
  int srcStrides[3];

  const AVPixelFormat buffer_format = videoBuffer->GetFormat();

  //AV_PIX_FMT_YUV420P10LE

  int out_map[4];

  const AVPixFmtDescriptor* fmtdesc = av_pix_fmt_desc_get(buffer_format);
  videoBuffer->GetPlanes(src);
  videoBuffer->GetStrides(srcStrides);
  pl_plane_data pdata[4] = { };
  
  for (int n = 0; n < pl_plane_data_from_pixfmt(pdata, &plbits, buffer_format); n++)
  {
    pdata[n].pixels = src[n];
    pdata[n].row_stride = srcStrides[n];
    pdata[n].width = n > 0 ? m_width >> 1: m_width;
    pdata[n].height = n > 0 ? m_height >> 1 : m_height;

    
    if (!pl_upload_plane(PL::PLInstance::Get()->GetGpu(), &plplanes[n], &pltex[n], &pdata[n]))
    {
      CLog::Log(LOGERROR, "pl_upload_plane failed");

    }
  }
  
  m_bLoaded = true;
  //m_bLoaded = m_texture.UnlockRect(0);
  return m_bLoaded;
}
