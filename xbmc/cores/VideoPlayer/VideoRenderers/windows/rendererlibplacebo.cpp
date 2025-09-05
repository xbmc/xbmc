/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererLibplacebo.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "VideoRenderers/BaseRenderer.h"
#include "WIN32Util.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "utils/memcpy_sse2.h"
#include "windowing/GraphicContext.h"

#include <ppl.h>
#include <libplacebo/d3d11.h>
#include <libplacebo/renderer.h>
#include <libplacebo/utils/upload.h>
#include "plhelper.h"

# define PL_LIBAV_IMPLEMENTATION 0
#include <libplacebo/utils/libav.h>

using namespace Microsoft::WRL;

CRendererBase* CRendererLibplacebo::Create(CVideoSettings& videoSettings)
{
  return new CRendererLibplacebo(videoSettings);
}

void CRendererLibplacebo::GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture)
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

CRendererLibplacebo::CRendererLibplacebo(CVideoSettings& videoSettings)
  : CRendererHQ(videoSettings)
{
  m_renderMethodName = "libplacebo";
}

CRendererLibplacebo::~CRendererLibplacebo()
{
  UnInit();
}

CRenderInfo CRendererLibplacebo::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  // libplacebo handles deinterlacing internally
  info.m_deintMethods.push_back(VS_INTERLACEMETHOD_AUTO);

  return info;
}

bool CRendererLibplacebo::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  if (!__super::Configure(picture, fps, orientation))
    return false;

  m_format = picture.videoBuffer->GetFormat();

  if (!PL::PLInstance::Get()->Init())
  {
    CLog::LogF(LOGERROR, "Failed to initialize libplacebo");
    return false;
  }

  // Set up color space information
  UpdateColorspace(picture);

  return true;
}



void CRendererLibplacebo::UnInit()
{
  //This is a problem if we release this we cannot free the texture afterward
  //PL::PLInstance::Get()->Reset();
}

void CRendererLibplacebo::UpdateColorspace(const VideoPicture& picture)
{
  // Set up color space based on picture metadata
  m_colorSpace = pl_color_space{
    .primaries = GetLibplaceboPrimaries(picture.color_primaries),
    .transfer = GetLibplaceboTransfer(picture.color_transfer),
  };
  m_chromaLocation = pl_chroma_from_av(picture.chroma_position);
}

enum pl_color_primaries CRendererLibplacebo::GetLibplaceboPrimaries(AVColorPrimaries primaries)
{
  switch (primaries)
  {
  case AVCOL_PRI_BT709:        return PL_COLOR_PRIM_BT_709;
  case AVCOL_PRI_BT2020:       return PL_COLOR_PRIM_BT_2020;
  case AVCOL_PRI_SMPTE170M:    return PL_COLOR_PRIM_BT_601_525;
  case AVCOL_PRI_BT470BG:      return PL_COLOR_PRIM_BT_601_625;
  //case AVCOL_PRI_SMPTE240M:    return PL_COLOR_PRIM_SMPTE_240M;
  case AVCOL_PRI_SMPTE431:     return PL_COLOR_PRIM_DCI_P3;
  case AVCOL_PRI_SMPTE432:     return PL_COLOR_PRIM_DISPLAY_P3;
  default:                     return PL_COLOR_PRIM_UNKNOWN;
  }
}

enum pl_color_transfer CRendererLibplacebo::GetLibplaceboTransfer(AVColorTransferCharacteristic transfer)
{
  switch (transfer)
  {
  case AVCOL_TRC_BT709:        return PL_COLOR_TRC_BT_1886;
  case AVCOL_TRC_SMPTE2084:    return PL_COLOR_TRC_PQ;
  case AVCOL_TRC_ARIB_STD_B67: return PL_COLOR_TRC_HLG;
  case AVCOL_TRC_GAMMA22:      return PL_COLOR_TRC_GAMMA22;
  case AVCOL_TRC_GAMMA28:      return PL_COLOR_TRC_GAMMA28;
  //case AVCOL_TRC_SMPTE240M:    return PL_COLOR_TRC_SMPTE_240M;
  default:                     return PL_COLOR_TRC_UNKNOWN;
  }
}

void CRendererLibplacebo::CheckVideoParameters()
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

      // Update color space
      VideoPicture tempPicture = {};
      tempPicture.color_space = buf->color_space;
      tempPicture.color_transfer = buf->color_transfer;
      tempPicture.color_primaries = buf->primaries;
      UpdateColorspace(tempPicture);
    }
  }
  CreateIntermediateTarget(m_sourceWidth, m_sourceHeight, false);
}

void CRendererLibplacebo::RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags)
{

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (!buf || !buf->IsLoaded())
    return;

  CRenderBufferImpl* buffer = static_cast<CRenderBufferImpl*>(buf);

  // Upload planes to libplacebo
  pl_frame frame = {};
  if (!buffer->GetLibplaceboFrame(frame))
  {
    CLog::LogF(LOGERROR, "Failed to get libplacebo frame");
    return;
  }

  frame.color = m_colorSpace;

  // Set up target
  pl_tex targetTex = nullptr;
  if (!CreateTargetTexture(target, targetTex))
  {
    CLog::LogF(LOGERROR, "Failed to create target texture");
    return;
  }
  pl_d3d11_wrap_params outputParams{};
  pl_frame frameOut = {};
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
  frameOut.planes[0].flipped = false;
  frameOut.crop.x1 = target.GetWidth();
  frameOut.crop.y1 = target.GetHeight();
  
  frameOut.color = frame.color;
  frameOut.repr.sys = PL_COLOR_SYSTEM_BT_709;
  frameOut.repr.levels = PL_COLOR_LEVELS_FULL;


  // Set up render parameters
  pl_render_params renderParams = pl_render_default_params;

  // Apply crop and destination rectangle
  CRect src = sourceRect;
  CRect dst(destPoints[0], destPoints[2]);


  frame.crop.x1 = buffer->GetWidth();
  frame.crop.y1 = buffer->GetHeight();

  frameOut.crop.x1 = target.GetWidth();
  frameOut.crop.y1 = target.GetHeight();
  pl_frame_set_chroma_location(&frame, m_chromaLocation);
  
  // Render the frame
  if (!pl_render_image(PL::PLInstance::Get()->GetRenderer(), &frame, &frameOut, &renderParams))
  {
    CLog::LogF(LOGERROR, "Failed to render frame with libplacebo");
  }
}

bool CRendererLibplacebo::CreateTargetTexture(CD3DTexture& target, pl_tex& plTex)
{
  ComPtr<ID3D11Texture2D> d3dTex;
  if (FAILED(target.Get()->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&d3dTex)))
    return false;
  pl_d3d11_wrap_params params = {};
  params.tex = d3dTex.Get();
  params.array_slice = 0;
  plTex = pl_d3d11_wrap(PL::PLInstance::Get()->GetGpu(), &params);

  return plTex != nullptr;
}

bool CRendererLibplacebo::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_BRIGHTNESS ||
    feature == RENDERFEATURE_CONTRAST ||
    feature == RENDERFEATURE_ROTATION ||
    feature == RENDERFEATURE_STRETCH ||
    feature == RENDERFEATURE_ZOOM)
  {
    return true; // libplacebo supports these through renderer params
  }

  return CRendererBase::Supports(feature);
}

bool CRendererLibplacebo::Supports(ESCALINGMETHOD method) const
{
  // libplacebo has excellent scaling support
  return true;
}

CRenderBuffer* CRendererLibplacebo::CreateBuffer()
{
  return new CRenderBufferImpl(m_format, m_sourceWidth, m_sourceHeight);
}

// Render Buffer Implementation
CRendererLibplacebo::CRenderBufferImpl::CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : CRenderBuffer(av_pix_format, width, height)
{
  memset(m_plTextures, 0, sizeof(m_plTextures));
}

CRendererLibplacebo::CRenderBufferImpl::~CRenderBufferImpl()
{
  ReleasePicture();

  for (int i = 0; i < 3; i++)
  {
    if (m_plTextures[i])
    {
      pl_tex_destroy(PL::PLInstance::Get()->GetGpu(), &m_plTextures[i]);
    }
  }
}

bool CRendererLibplacebo::CRenderBufferImpl::UploadBuffer()
{
  if (!videoBuffer || !PL::PLInstance::Get()->GetGpu())
    return false;

  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
  {
    // For hardware decoded content, we might need to copy to system memory first
    // or handle it differently - this is a simplified approach
    m_bLoaded = true;
    return true;
  }

  return UploadPlanes();
}

bool CRendererLibplacebo::CRenderBufferImpl::UploadPlanes()
{
  uint8_t* src[3];
  int srcStrides[3];
  videoBuffer->GetPlanes(src);
  videoBuffer->GetStrides(srcStrides);

  const AVPixelFormat format = videoBuffer->GetFormat();

  // Clean up existing textures
  for (int i = 0; i < 3; i++)
  {
    if (m_plTextures[i])
    {
      //pl_tex_destroy(PL::PLInstance::Get()->GetGpu(), &m_plTextures[i]);
      //m_plTextures[i] = nullptr;
    }
  }

  bool success = false;

  if (format == AV_PIX_FMT_YUV420P)
  {
    success = UploadYUV420P(src, srcStrides);
  }
  else if (format == AV_PIX_FMT_NV12)
  {
    success = UploadNV12(src, srcStrides);
  }
  else if (format == AV_PIX_FMT_YUV420P10 || format == AV_PIX_FMT_YUV420P16 || format == AV_PIX_FMT_YUV420P10LE)
  {
    success = UploadYUV420P_HighBit(src, srcStrides, format == AV_PIX_FMT_YUV420P16 ? 16 : 10);
  }
  else if (format == AV_PIX_FMT_P010 || format == AV_PIX_FMT_P016)
  {
    success = UploadP010_P016(src, srcStrides, format == AV_PIX_FMT_P016 ? 16 : 10);
  }

  m_bLoaded = success;
  return success;
}

bool CRendererLibplacebo::CRenderBufferImpl::UploadYUV420P(uint8_t* src[3], int srcStrides[3])
{

  pl_tex_params paramy = pl_tex_params{
        .w = static_cast<int>(m_width),
    .h = static_cast<int>(m_height),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r8"),
    .sampleable = true,
    .host_writable = true,
  };
    pl_tex_params paramu= pl_tex_params{
    .w = static_cast<int>(m_width / 2),
    .h = static_cast<int>(m_height / 2),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r8"),
    .sampleable = true,
    .host_writable = true,
    };
  pl_tex_params paramv = pl_tex_params{
    .w = static_cast<int>(m_width / 2),
    .h = static_cast<int>(m_height / 2),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r8"),
    .sampleable = true,
    .host_writable = true,
  };
  if (!m_plTextures[0] || !m_plTextures[1] || !m_plTextures[2])
  {
    // Y plane
    m_plTextures[0] = pl_tex_create(PL::PLInstance::Get()->GetGpu(), &paramy);
    // U plane
    m_plTextures[1] = pl_tex_create(PL::PLInstance::Get()->GetGpu(), &paramu);
    // V plane
    m_plTextures[2] = pl_tex_create(PL::PLInstance::Get()->GetGpu(), &paramv);
  }


  pl_tex_transfer_params tex[3];
  for (int x = 0; x < 3; x++)
  {
    tex[x] = pl_tex_transfer_params{
    .tex = m_plTextures[x],
    .row_pitch = (size_t)srcStrides[x],
    .ptr = src[x],
    };
    // Upload data
    pl_tex_upload(PL::PLInstance::Get()->GetGpu(), &tex[x]);
  }
  return true;
}


// Placeholder implementations for high bit depth formats
bool CRendererLibplacebo::CRenderBufferImpl::UploadYUV420P_HighBit(uint8_t* src[3], int srcStrides[3], int bitDepth)
{
  // Y plane
  pl_tex_params paramy = pl_tex_params{
        .w = static_cast<int>(m_width),
    .h = static_cast<int>(m_height),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r16"),
    .sampleable = true,
    .host_writable = true,
  };
  // U plane
  pl_tex_params paramu = pl_tex_params{
  .w = static_cast<int>(m_width / 2),
  .h = static_cast<int>(m_height / 2),
  .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r16"),
  .sampleable = true,
  .host_writable = true,
  };
  // V plane
  pl_tex_params paramv = pl_tex_params{
    .w = static_cast<int>(m_width / 2),
    .h = static_cast<int>(m_height / 2),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r16"),
    .sampleable = true,
    .host_writable = true,
  };

  
    // Y plane
     pl_tex_recreate(PL::PLInstance::Get()->GetGpu(), &m_plTextures[0],&paramy);
    // U plane
     pl_tex_recreate(PL::PLInstance::Get()->GetGpu(), &m_plTextures[1],&paramu);
    // V plane
     pl_tex_recreate(PL::PLInstance::Get()->GetGpu(), &m_plTextures[2],&paramv);
  
  
  if (!m_plTextures[0] || !m_plTextures[1] || !m_plTextures[2])
    return false;
  pl_tex_transfer_params tex[3];
  tex[0] = pl_tex_transfer_params{
    .tex = m_plTextures[0],
            .rc = {},
    .row_pitch = (size_t)srcStrides[0],
    .ptr = src[0],
  };
  pl_tex_upload(PL::PLInstance::Get()->GetGpu(), &tex[0]);
  // Upload data

  for (int x = 1; x < 3; x++)
  {
    tex[x] = pl_tex_transfer_params{
    .tex = m_plTextures[x],
        .rc = {},
    .row_pitch = (size_t)srcStrides[x],
    .ptr = src[x],

    };
    // Upload data
    pl_tex_upload(PL::PLInstance::Get()->GetGpu(), &tex[x]);
  }
  return true;
}

bool CRendererLibplacebo::CRenderBufferImpl::UploadNV12(uint8_t* src[3], int srcStrides[3])
{
  /*
  // Y plane
  m_plTextures[0] = pl_tex_create(PL::PLInstance::Get()->GetGpu(), pl_tex_params{
    .w = static_cast<int>(m_width),
    .h = static_cast<int>(m_height),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "r8"),
    .sampleable = true,
    .host_writable = true,
    });

  // UV plane
  m_plTextures[1] = pl_tex_create(PL::PLInstance::Get()->GetGpu(), pl_tex_params{
    .w = static_cast<int>(m_width / 2),
    .h = static_cast<int>(m_height / 2),
    .format = pl_find_named_fmt(PL::PLInstance::Get()->GetGpu(), "rg8"),
    .sampleable = true,
    .host_writable = true,
    });

  if (!m_plTextures[0] || !m_plTextures[1])
    return false;

  // Upload data
  pl_tex_transfer_params y_transfer = {
    .tex = m_plTextures[0],
    .ptr = src[0],
    .row_pitch = srcStrides[0],
  };
  pl_tex_upload(PL::PLInstance::Get()->GetGpu(), &y_transfer);

  pl_tex_transfer_params uv_transfer = {
    .tex = m_plTextures[1],
    .ptr = src[1],
    .row_pitch = srcStrides[1],
  };
  pl_tex_upload(PL::PLInstance::Get()->GetGpu(), &uv_transfer);
  */
  return true;
}

bool CRendererLibplacebo::CRenderBufferImpl::GetLibplaceboFrame(pl_frame& frame)
{
  if (!m_bLoaded)
    return false;

  const AVPixelFormat format = videoBuffer->GetFormat();

  if (format == AV_PIX_FMT_YUV420P)
  {
    frame.num_planes = 3;
    frame.repr.sys = PL_COLOR_SYSTEM_BT_709;
    frame.repr.levels = PL_COLOR_LEVELS_LIMITED;

    frame.planes[0] = { .texture = m_plTextures[0], .components = 1, .component_mapping = {0} };
    frame.planes[1] = { .texture = m_plTextures[1], .components = 1, .component_mapping = {1} };
    frame.planes[2] = { .texture = m_plTextures[2], .components = 1, .component_mapping = {2} };
  }
  else if (format == AV_PIX_FMT_YUV420P10LE || format == AV_PIX_FMT_YUV420P16LE)
  {
    frame.num_planes = 3;
    frame.repr.sys = PL_COLOR_SYSTEM_BT_709;
    frame.repr.levels = PL_COLOR_LEVELS_LIMITED;
    frame.repr.bits.sample_depth = (format == AV_PIX_FMT_YUV420P16LE) ? 16 : 10;
    frame.repr.bits.color_depth = (format == AV_PIX_FMT_YUV420P16LE) ? 16 : 10;

    frame.planes[0] = { .texture = m_plTextures[0], .components = 1, .component_mapping = {0} };
    frame.planes[1] = { .texture = m_plTextures[1], .components = 1, .component_mapping = {1} };
    frame.planes[2] = { .texture = m_plTextures[2], .components = 1, .component_mapping = {2} };
  }
  else if (format == AV_PIX_FMT_NV12)
  {
    frame.num_planes = 2;
    frame.repr.sys = PL_COLOR_SYSTEM_BT_709;
    frame.repr.levels = PL_COLOR_LEVELS_LIMITED;

    frame.planes[0] = { .texture = m_plTextures[0], .components = 1, .component_mapping = {0} };
    frame.planes[1] = { .texture = m_plTextures[1], .components = 2, .component_mapping = {1, 2} };
  }

  return true;
}


bool CRendererLibplacebo::CRenderBufferImpl::UploadP010_P016(uint8_t* src[3], int srcStrides[3], int bitDepth)
{
  // Implementation for P010/P016 formats
  // Similar to UploadNV12 but with 16-bit format
  return false; // Placeholder
}