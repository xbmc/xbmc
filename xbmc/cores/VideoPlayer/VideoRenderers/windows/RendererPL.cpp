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
  
}

CRenderInfo CRendererPL::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  info.m_deintMethods.push_back(VS_INTERLACEMETHOD_AUTO);

  return  info;
}

void fill_all_tests(std::vector<pl_color_primaries>& prim, std::vector<pl_color_transfer>& trans, std::vector<pl_color_system>& sys)
{


  // Toutes les primaires connues
  const pl_color_primaries primaries_list[] = {
      PL_COLOR_PRIM_UNKNOWN,
      // Standard gamut:
      PL_COLOR_PRIM_BT_601_525,   // ITU-R Rec. BT.601 (525-line = NTSC, SMPTE-C)
      PL_COLOR_PRIM_BT_601_625,   // ITU-R Rec. BT.601 (625-line = PAL, SECAM)
      PL_COLOR_PRIM_BT_709,       // ITU-R Rec. BT.709 (HD), also sRGB
      PL_COLOR_PRIM_BT_470M,      // ITU-R Rec. BT.470 M
      PL_COLOR_PRIM_EBU_3213,     // EBU Tech. 3213-E / JEDEC P22 phosphors
      // Wide gamut:
      PL_COLOR_PRIM_BT_2020,      // ITU-R Rec. BT.2020 (UltraHD)
      PL_COLOR_PRIM_APPLE,        // Apple RGB
      PL_COLOR_PRIM_ADOBE,        // Adobe RGB (1998)
      PL_COLOR_PRIM_PRO_PHOTO,    // ProPhoto RGB (ROMM)
      PL_COLOR_PRIM_CIE_1931,     // CIE 1931 RGB primaries
      PL_COLOR_PRIM_DCI_P3,       // DCI-P3 (Digital Cinema)
      PL_COLOR_PRIM_DISPLAY_P3,   // DCI-P3 (Digital Cinema) with D65 white point
      PL_COLOR_PRIM_V_GAMUT,      // Panasonic V-Gamut (VARICAM)
      PL_COLOR_PRIM_S_GAMUT,      // Sony S-Gamut
      PL_COLOR_PRIM_FILM_C,       // Traditional film primaries with Illuminant C
      PL_COLOR_PRIM_ACES_AP0,     // ACES Primaries #0 (ultra wide)
      PL_COLOR_PRIM_ACES_AP1,     // ACES Primaries #1
      PL_COLOR_PRIM_COUNT
  };

  // Toutes les transferts connus
  const pl_color_transfer transfer_list[] = {
      PL_COLOR_TRC_UNKNOWN,
      // Standard dynamic range:
      PL_COLOR_TRC_BT_1886,       // ITU-R Rec. BT.1886 (CRT emulation + OOTF)
      PL_COLOR_TRC_SRGB,          // IEC 61966-2-4 sRGB (CRT emulation)
      PL_COLOR_TRC_LINEAR,        // Linear light content
      PL_COLOR_TRC_GAMMA18,       // Pure power gamma 1.8
      PL_COLOR_TRC_GAMMA20,       // Pure power gamma 2.0
      PL_COLOR_TRC_GAMMA22,       // Pure power gamma 2.2
      PL_COLOR_TRC_GAMMA24,       // Pure power gamma 2.4
      PL_COLOR_TRC_GAMMA26,       // Pure power gamma 2.6
      PL_COLOR_TRC_GAMMA28,       // Pure power gamma 2.8
      PL_COLOR_TRC_PRO_PHOTO,     // ProPhoto RGB (ROMM)
      PL_COLOR_TRC_ST428,         // Digital Cinema Distribution Master (XYZ)
      // High dynamic range:
      PL_COLOR_TRC_PQ,            // ITU-R BT.2100 PQ (perceptual quantizer), aka SMPTE ST2048
      PL_COLOR_TRC_HLG,           // ITU-R BT.2100 HLG (hybrid log-gamma), aka ARIB STD-B67
      PL_COLOR_TRC_V_LOG,         // Panasonic V-Log (VARICAM)
      PL_COLOR_TRC_S_LOG1,        // Sony S-Log1
      PL_COLOR_TRC_S_LOG2,        // Sony S-Log2
      PL_COLOR_TRC_COUNT
  };

  // Toutes les matrices YUV→RGB
  const pl_color_system matrix_list[] = {
      PL_COLOR_SYSTEM_UNKNOWN,      // YCbCr-like color systems:
      PL_COLOR_SYSTEM_BT_601,      // ITU-R Rec. BT.601 (SD)
      PL_COLOR_SYSTEM_BT_709,      // ITU-R Rec. BT.709 (HD)
      PL_COLOR_SYSTEM_SMPTE_240M,  // SMPTE-240M
      PL_COLOR_SYSTEM_BT_2020_NC,  // ITU-R Rec. BT.2020 (non-constant luminance)
      PL_COLOR_SYSTEM_BT_2020_C,   // ITU-R Rec. BT.2020 (constant luminance)
      PL_COLOR_SYSTEM_BT_2100_PQ,  // ITU-R Rec. BT.2100 ICtCp PQ variant
      PL_COLOR_SYSTEM_BT_2100_HLG, // ITU-R Rec. BT.2100 ICtCp HLG variant
      PL_COLOR_SYSTEM_DOLBYVISION, // Dolby Vision (see pl_dovi_metadata)
      PL_COLOR_SYSTEM_YCGCO,       // YCgCo (derived from RGB)
      // Other color systems:
      PL_COLOR_SYSTEM_RGB,         // Red, Green and Blue
      PL_COLOR_SYSTEM_XYZ,         // Digital Cinema Distribution Master (XYZ)
      PL_COLOR_SYSTEM_COUNT
  };


  // Boucle brute sur toutes les combinaisons
  for (int mi = 0; mi < PL_COLOR_SYSTEM_COUNT; mi++)
  {
    sys.push_back(matrix_list[mi]);
  }
  for (int ti = 0; ti < PL_COLOR_TRC_COUNT; ti++)
  {
    trans.push_back(transfer_list[ti]);
  }
  for (int pi = 0; pi < PL_COLOR_PRIM_COUNT; pi++)
  {
    prim.push_back(primaries_list[pi]);
  }
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

    if (m_testsystem.size() == 0)
      fill_all_tests(m_testprimaries, m_testtransfer, m_testsystem);
  
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
  CRenderBufferImpl* plbuf = (CRenderBufferImpl*)buf;

  if (!buffer->GetLibplaceboFrame(frameIn))
    return;
  frameIn.repr.sys = PL_COLOR_SYSTEM_BT_709;
  frameIn.color = m_colorSpace;
  //todo
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
  frameOut.repr = frameIn.repr;
  frameOut.color = frameIn.color;
  frameOut.color.primaries = m_testprimaries.at(PL::PLInstance::Get()->CurrentPrim);
  frameOut.color.transfer = m_testtransfer.at(PL::PLInstance::Get()->Currenttransfer);
  frameOut.repr.sys = m_testsystem.at(PL::PLInstance::Get()->CurrentMatrix);

  pl_render_params params;
  params = pl_render_default_params;
  //Would need to make the target clearable first
  params.skip_target_clearing = true;
  pl_frame_set_chroma_location(&frameIn, m_chromaLocation);
  bool res = pl_render_image(PL::PLInstance::Get()->GetRenderer(), &frameIn, &frameOut, &params);

  sourceRect = dst;
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

bool CRendererPL::CRenderBufferImpl::GetLibplaceboFrame(pl_frame& frame)
{
  if (!m_bLoaded)
    return false;
  pl_color_repr crpr{};

  crpr.sys = PL_COLOR_SYSTEM_BT_709;
  crpr.levels = PL_COLOR_LEVELS_LIMITED;
  crpr.bits.bit_shift = 0;

  crpr.bits.color_depth = 16;
  crpr.bits.sample_depth = 16;

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

  struct pl_bit_encoding bits;

  int out_map[4];

  const AVPixFmtDescriptor* fmtdesc = av_pix_fmt_desc_get(buffer_format);
  videoBuffer->GetPlanes(src);
  videoBuffer->GetStrides(srcStrides);
  pl_plane_data pdata[4] = { };
  
  for (int n = 0; n < pl_plane_data_from_pixfmt(pdata, &bits, videoBuffer->GetFormat()); n++)
  {
    pdata[n].pixels = src[n];
    pdata[n].row_stride = srcStrides[n];
    //pdata[n].pixel_stride = 2;
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
