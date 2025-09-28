﻿/*
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
  if (av_pixel_format == AV_PIX_FMT_YUV420P || av_pixel_format == AV_PIX_FMT_YUV420P10 ||
      av_pixel_format == AV_PIX_FMT_YUV420P16 || av_pixel_format == AV_PIX_FMT_NV12 ||
      av_pixel_format == AV_PIX_FMT_P010 || av_pixel_format == AV_PIX_FMT_P016)
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

CRendererPL::CRendererPL(CVideoSettings& videoSettings) : CRendererBase(videoSettings)
{
  m_renderMethodName = "LibPlacebo";
  m_colorSpace = {};
  m_chromaLocation = PL_CHROMA_UNKNOWN;
}

CRendererPL::~CRendererPL()
{
  PL::PLInstance::Get()->Reset();
}

CRenderInfo CRendererPL::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  info.m_deintMethods.push_back(VS_INTERLACEMETHOD_AUTO);

  return info;
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
  m_format = picture.videoBuffer->GetFormat();
  return true;
}

DEBUG_INFO_VIDEO CRendererPL::GetDebugInfo(int idx)
{
  

  CRenderBuffer* rb = m_renderBuffers[idx];
  CRenderBufferImpl* plbuffer = static_cast<CRenderBufferImpl*>(rb);

  DEBUG_INFO_VIDEO info;
  pl_hdr_metadata hdr = plbuffer->GetHdrMetadata();

  info.videoSource =
      StringUtils::Format("Output: Format: {} Levels: full, ColorMatrix:rgb",
                          DX::DXGIFormatToShortString(m_IntermediateTarget.GetFormat()));

  info.videoSource +=
      StringUtils::Format(" Transfer: {} Primaries: {}", PL::PLInstance::Get()->pl_color_transfer_short_name(m_displayTransfer),
                          PL::PLInstance::Get()->pl_color_primaries_short_name(m_displayPrimaries));

  info.metaLight = StringUtils::Format(
      "Input: Matrix:{} Primaries:{} Transfer:{}", PL::PLInstance::Get()->pl_color_primaries_short_name(m_colorSpace.primaries),
      PL::PLInstance::Get()->pl_color_transfer_short_name(m_colorSpace.transfer), PL::PLInstance::Get()->pl_color_system_short_name(m_videoMatrix));

  //If we have metadata and we are sending it to the swapchain
  if (plbuffer->hasDisplayMetadata && m_bTargetColorspaceHint)
  {
    info.shader = "Primaries (meta): ";
    info.shader += StringUtils::Format(
        "R({:.3f} {:.3f}), G({:.3f} {:.3f}), B({:.3f} {:.3f}), WP({:.3f} {:.3f})", hdr.prim.red.x,
        hdr.prim.red.y, hdr.prim.green.x, hdr.prim.green.y, hdr.prim.blue.x, hdr.prim.blue.y,
        hdr.prim.white.x, hdr.prim.white.y);

    info.render = StringUtils::Format("HDR light (meta): max ML: {:.0f}, min ML: {:.4f}",
                                      hdr.max_luma, hdr.min_luma);
    info.render += StringUtils::Format(", max CLL: {}, max FALL: {}", hdr.max_cll, hdr.max_fall);
  }
  //line 1 std::string videoSource;
  //2 std::string metaPrim;
  //3 std::string metaLight;
  //4 std::string shader;
  //5 std::string render;
  return info;
}

void CRendererPL::CheckVideoParameters()
{
  __super::CheckVideoParameters();

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (buf)
  {
    // Check if color space parameters have changed
    if (buf->color_space != m_lastColorSpace || buf->color_transfer != m_lastColorTransfer ||
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
  //Create the intermediate target so far we use what the backbuffer is
  //Would we use DXGI_FORMAT_R10G10B10A2_UNORM if IsHighPrecisionProcessingSettingEnabled is on with a 8 bits backbuffer?
  CreateIntermediateTarget(m_viewWidth, m_viewHeight, false);

  PL::PLInstance::Get()->fill_d3d_format(&m_plOutputFormat, m_IntermediateTarget.GetFormat());
  
  m_plRenderParams = pl_render_default_params;
  PL::pl_tone_mapping method;
  switch (m_videoSettings.m_ToneMapMethod)
  {
    case VS_TONEMAPMETHOD_OFF:
      method = PL::TONE_MAPPING_AUTO;
      break;
    case VS_TONEMAPMETHOD_REINHARD:
      method = PL::TONE_MAPPING_REINHARD;
      break;
    case VS_TONEMAPMETHOD_ACES:
      method = PL::TONE_MAPPING_SPLINE;
      break;
    case VS_TONEMAPMETHOD_HABLE:
    default:
      method = PL::TONE_MAPPING_AUTO;
      break;

  }
  //m_videoSettings.m_ToneMapParam
  //This one was deprecated and should modify tone_constants
  //it consist of 11 float settings 
  pl_color_map_params params = pl_color_map_high_quality_params;
  params = {
    //const struct pl_gamut_map_function *gamut_mapping;
    //struct pl_gamut_map_constants gamut_constants;
    //int lut3d_size[3];
    //bool lut3d_tricubic;
    //bool gamut_expansion;
    .tone_mapping_function = PL::PLInstance::Get()->GetToneMappingFunction(method),
    //struct pl_tone_map_constants tone_constants;
    //bool inverse_tone_mapping;
    //enum pl_hdr_metadata_type metadata;
    //int lut_size;
    //float contrast_recovery;
    //float contrast_smoothness;
    //bool force_tone_mapping_lut;
    //bool visualize_lut;
    //pl_rect2df visualize_rect;
    //float visualize_hue;    // useful range [-pi, pi]
    //float visualize_theta;  // useful range [0, pi/2]
    //bool show_clipping;
  };
  m_plRenderParams.color_map_params = &params;

  //To avoid spam on the debug log
  m_plRenderParams.border = PL_CLEAR_SKIP;
  
  

}

void CRendererPL::RenderImpl(CD3DTexture& target,
                             CRect& sourceRect,
                             CPoint (&destPoints)[4],
                             uint32_t flags)
{

  CRect dst = CRect(destPoints[0], destPoints[2]);
  CPoint rotated[4];
  pl_frame frameOut{};
  pl_frame frameIn{};
  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (!buf || !buf->IsLoaded())
    return;

  CRenderBufferImpl* buffer = static_cast<CRenderBufferImpl*>(buf);
  if (!buffer->GetLibplaceboFrame(frameIn))
    return;

  //Dovi color space is set when compiling hdr data
  if (frameIn.repr.sys != PL_COLOR_SYSTEM_DOLBYVISION)
  {
    frameIn.repr.levels = buf->full_range ? PL_COLOR_LEVELS_FULL : PL_COLOR_LEVELS_LIMITED;
    frameIn.repr.sys = pl_system_from_av(buf->color_space);
    if (frameIn.repr.sys == PL_COLOR_SYSTEM_UNKNOWN)
    {
      if (buf->primaries == AVCOL_PRI_BT470BG || buf->primaries == AVCOL_PRI_SMPTE170M)
        frameIn.repr.sys = PL_COLOR_SYSTEM_BT_601;
      else if (buf->primaries == AVCOL_PRI_BT709)
        frameIn.repr.sys = PL_COLOR_SYSTEM_BT_709;
      else if (buf->primaries == AVCOL_PRI_BT2020 &&
               (buf->color_transfer == AVCOL_TRC_SMPTEST2084 ||
                buf->color_transfer == AVCOL_TRC_ARIB_STD_B67))
        frameIn.repr.sys = PL_COLOR_SYSTEM_BT_2020_NC;
      else
        frameIn.repr.sys = PL_COLOR_SYSTEM_BT_709;
    }
    frameIn.color = m_colorSpace;
  }
  else
    m_colorSpace = frameIn.color;

  //TODO
  //Add icc profile
  //add rotate
  //add cache for saving time during the compiling of glsl shaders

  //wrap the intermediate texture onthe output frame
  pl_d3d11_wrap_params d3dparams = {.tex = target.Get(),
                                    .array_slice = 1,
                                    .fmt = target.GetFormat(),
                                    .w = (int)target.GetWidth(),
                                    .h = (int)target.GetHeight()};
  
  
  frameOut.num_planes = m_plOutputFormat.num_planes;
  frameOut.planes[0].texture = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &d3dparams);
  frameOut.planes[0].components = m_plOutputFormat.components[0];
  frameOut.planes[0].component_mapping[0] = m_plOutputFormat.component_mapping[0][0];
  frameOut.planes[0].component_mapping[1] = m_plOutputFormat.component_mapping[0][1];
  frameOut.planes[0].component_mapping[2] = m_plOutputFormat.component_mapping[0][2];
  frameOut.planes[0].component_mapping[3] = m_plOutputFormat.component_mapping[0][3];

  frameOut.crop.x0 = dst.x1;
  frameOut.crop.x1 = dst.x2;
  frameOut.crop.y0 = dst.y1;
  frameOut.crop.y1 = dst.y2;
  //We skip rendererbase process hdr so its important to set it if its not valid
  if (m_HdrType == HDR_TYPE::HDR_INVALID)
  {
    if (buf->hasDisplayMetadata)
      m_HdrType = HDR_TYPE::HDR_HDR10;
  }

  if (ActualRenderAsHDR() && m_bTargetColorspaceHint)
  {
    frameOut.color.primaries = PL_COLOR_PRIM_BT_2020;
      // temporary until libplacebo output goes directly to back buffer
      // and doesn't get processed by the output shader.
      if (buf->color_transfer == AVCOL_TRC_ARIB_STD_B67)
      frameOut.color.transfer = PL_COLOR_TRC_HLG;
    else
      frameOut.color.transfer = PL_COLOR_TRC_PQ;
  }
  else
  {
    frameOut.color.primaries = PL_COLOR_PRIM_BT_709;
    frameOut.color.transfer = PL_COLOR_TRC_BT_1886;
  }
  //! @todo does frameOut.color.hdr make any difference in the rendering?
  //! @todo copy frameIn.color.hdr or make up something from the display EDID?
  frameOut.repr.sys = PL_COLOR_SYSTEM_RGB;
  frameOut.repr.levels =
    DX::Windowing()->UseLimitedColor() ? PL_COLOR_LEVELS_LIMITED : PL_COLOR_LEVELS_FULL;



  //Without this recent version of libplacebo would spam the debug log like crazy
  //And its also set on an info level
  
  //this data is used for the video debug renderer
  m_displayTransfer = frameOut.color.transfer;
  m_displayPrimaries = frameOut.color.primaries;
  m_videoMatrix = frameIn.repr.sys;
  pl_frame_set_chroma_location(&frameIn, m_chromaLocation);

  bool res = pl_render_image(PL::PLInstance::Get()->GetRenderer(), &frameIn, &frameOut, &m_plRenderParams);

  sourceRect = dst;
}

bool CRendererPL::Supports(ERENDERFEATURE feature) const
{
  //3d lut
  //use 10bit for sdr in the menu
  // dithering
  //TODO
  //Rotation is not done by libplacebo it expect a non rotated texture
  //RENDERFEATURE_TONEMAP
  if (feature == RENDERFEATURE_BRIGHTNESS || feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_GAMMA || feature == RENDERFEATURE_NONLINSTRETCH ||
      feature == RENDERFEATURE_ROTATION || feature == RENDERFEATURE_NOISE || 
      feature ==RENDERFEATURE_STRETCH || feature == RENDERFEATURE_PIXEL_RATIO || 
      feature == RENDERFEATURE_VERTICAL_SHIFT || feature == RENDERFEATURE_ZOOM )
  {

    return false;
  }
  else
    return true;
}

bool CRendererPL::Supports(ESCALINGMETHOD method) const
{
  //The scaling in kodi is only one libplacebo 
  if (method == VS_SCALINGMETHOD_AUTO)
    return true;
  return true;
}

CRenderBuffer* CRendererPL::CreateBuffer()
{
  //can we get the dxva format at this moment?
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

CRendererPL::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format,
                                                  unsigned width,
                                                  unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{
  //Is this needed??
  m_widthTex = FFALIGN(width, 32);
  m_heightTex = FFALIGN(height, 32);
  //will be set on first upload
  m_plFormat.num_planes = -1;
}

CRendererPL::CRenderBufferImpl::~CRenderBufferImpl()
{
  //do we release the ref to plplanes and pltex here???
  CRenderBufferImpl::ReleasePicture();
}

void CRendererPL::CRenderBufferImpl::AppendPicture(const VideoPicture& picture)
{
  __super::AppendPicture(picture);
  m_plColorSpace = picture.plColorSpace;
  m_plColorRepr = picture.plColorRepr;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    const auto hw = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer);
    m_widthTex = hw->width;
    m_heightTex = hw->height;
  }
}

bool CRendererPL::CRenderBufferImpl::GetLibplaceboFrame(pl_frame& frame)
{
  if (!m_bLoaded)
    return false;

  //hdr data is in the frame color space
  frame.color = m_plColorSpace;
  //set sample dep and others
  m_plColorRepr.bits = m_plFormat.bits;

  frame.repr = m_plColorRepr;

  frame.num_planes = m_plFormat.num_planes;
  frame.planes[0] = m_plplanes[0];
  frame.planes[1] = m_plplanes[1];
  frame.planes[2] = m_plplanes[2];

  return true;
}
bool CRendererPL::CRenderBufferImpl::UploadBuffer()
{
  if (!videoBuffer)
    return false;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    return UploadWrapPlanes();
  }
  else
  {
    return UploadPlanes();
  }
}

bool CRendererPL::CRenderBufferImpl::UploadPlanes()
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
  pl_plane_data pdata[4] = {};

  m_plFormat.num_planes = pl_plane_data_from_pixfmt(pdata, &m_plFormat.bits, buffer_format);

  for (int n = 0; n < m_plFormat.num_planes; n++)
  {
    pdata[n].pixels = src[n];
    pdata[n].row_stride = srcStrides[n];
    pdata[n].width = n > 0 ? m_width >> 1 : m_width;
    pdata[n].height = n > 0 ? m_height >> 1 : m_height;

    if (!pl_upload_plane(PL::PLInstance::Get()->GetGpu(), &m_plplanes[n], &m_pltex[n], &pdata[n]))
    {
      CLog::Log(LOGERROR, "pl_upload_plane failed");
    }
  }
  
  m_bLoaded = true;
  return m_bLoaded;
}

bool CRendererPL::CRenderBufferImpl::UploadWrapPlanes()
{
  ComPtr<ID3D11Resource> pResource;
  ComPtr<ID3D11Texture2D> pTexture;
  ComPtr<ID3D11ShaderResourceView> srvY;
  D3D11_TEXTURE2D_DESC desc;
  HRESULT hr;
  unsigned arrayIdx;

  if (FAILED(GetResource(&pResource, &arrayIdx)))
  {
    CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
    return false;
  }

  if (m_plFormat.num_planes == -1)
  {
    //fill the plane data information needed for the conversion only once
    const auto dxva_buf = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer);
    DXGI_FORMAT fmt = dxva_buf->format;
    PL::PLInstance::Get()->fill_d3d_format(&m_plFormat, fmt);
  }

  hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
  pTexture->GetDesc(&desc);

  // Wrap the plane of the D3D11 texture
  // TODO maybe reuse the srv
  for (int i = 0; i < m_plFormat.num_planes; i++)
  {
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2DARRAY, m_plFormat.planes[i],
                                             0, 1, arrayIdx, 1);
    hr = DX::DeviceResources::Get()->GetD3DDevice()->CreateShaderResourceView(pTexture.Get(),
                                                                              &srvDesc, &srvY);
    pl_d3d11_wrap_params params = {};
    params.tex = pTexture.Get();
    params.w = desc.Width / m_plFormat.width_div[i];
    params.h = desc.Height / m_plFormat.height_div[i];
    params.fmt = m_plFormat.planes[i];
    params.array_slice = arrayIdx;
    m_pltex[i] = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &params);

    if (!m_pltex[i])
      return false;
    m_plplanes[i].texture = m_pltex[i];
    //number of components per plane example uv is 2 in d3d the alpha is always a component but not with libplacebo
    m_plplanes[i].components = m_plFormat.components[i];
    //mapping yuv planes to rgba channels
    for (int j = 0; j < 4; j++)
      m_plplanes[i].component_mapping[j] = m_plFormat.component_mapping[i][j];
  }
  m_bLoaded = true;
  return m_bLoaded;
}

bool is_memzero(void* ptr, size_t size)
{
  static const char zeros[1] = {0};
  return memcmp(ptr, zeros, size) == 0;
}

bool CRendererPL::CRenderBufferImpl::HasHdrData()
{
  if (!is_memzero(&m_plColorRepr.dovi, sizeof(m_plColorRepr.dovi)))
    return true;
  if (!is_memzero(&m_plColorSpace.hdr, sizeof(m_plColorSpace.hdr)))
    return true;
  // still all zeros
  return false;
}