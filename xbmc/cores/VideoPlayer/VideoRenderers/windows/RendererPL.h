/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "RendererHQ.h"
#include "VideoRenderers/HwDecRender/DXVAHD.h"

#include <map>

#include <d3d11_4.h>

#pragma comment(lib, "libplacebo-354.lib")

extern "C"
{
#include <libavutil/pixfmt.h>
}
#include "libplacebo/log.h"
#include "libplacebo/renderer.h"
#include "libplacebo/d3d11.h"
#include <libplacebo/options.h>
#include "libplacebo/utils/frame_queue.h"
#include "libplacebo/utils/upload.h"
#include "libplacebo/colorspace.h"
# define PL_LIBAV_IMPLEMENTATION 0
#include <libplacebo/utils/libav.h>
#if 0
/*Libplacebo*/
struct pl_color_space color;
struct pl_color_repr repr;
// Original values before Dolby Vision metadata mapping
enum pl_color_primaries primaries_orig;
enum pl_color_transfer transfer_orig;
enum pl_color_system sys_orig;
enum pl_chroma_location chroma_location;
AVBufferRef* dovi;

m_pPlHelper = new CPlHelper();
if (!m_pPlHelper->Init())
CLog::Log(LOGERROR, "CDVDVideoCodecFFmpegV2::Open() Failed initiating libplacebo!");

CPlHelper* m_pPlHelper = nullptr;
pl_tex plTextures[PL_MAX_PLANES] = {};

Microsoft::WRL::ComPtr<ID3D11Texture2D> pD3D11Texture2D;
pl_frame frameOut{};
pl_frame frameIn{};
pl_d3d11_wrap_params outputParams{};
pl_d3d11_wrap_params inParams{};

ComPtr<ID3D11Resource> pResource;
ComPtr<ID3D11Texture2D> surface;
m_videoBuffer->GetResource(&pResource);
if (SUCCEEDED(pResource.As(&surface)))
{

  inParams.w = m_videoBuffer->width;
  inParams.h = m_videoBuffer->height;
  inParams.fmt = m_videoBuffer->format;
  inParams.tex = surface.Get();
  pl_tex inTexture = pl_d3d11_wrap(m_pPlHelper->GetPLD3d11()->gpu, &inParams);
  struct pl_color_space csp {};
  struct pl_color_repr repr {};
  struct pl_dovi_metadata dovi {};
  pl_color_space colorspace{};
  colorspace.primaries = m_pPlHelper->GetColorPrimaries(avctx->color_primaries);
  colorspace.transfer = m_pPlHelper->GetColorTransfer(avctx->color_trc);

  frameIn.num_planes = 1;
  frameIn.planes[0].texture = inTexture;
  frameIn.planes[0].components = 3;
  frameIn.planes[0].component_mapping[0] = 0;
  frameIn.planes[0].component_mapping[1] = 1;
  frameIn.planes[0].component_mapping[2] = 2;
  frameIn.planes[0].component_mapping[3] = -1;
  frameIn.planes[0].flipped = false;
  frameIn.repr = repr;
  //If pre merged the output of the shaders is a rgb image
  frameIn.repr.sys = PL_COLOR_SYSTEM_RGB;
  frameIn.color = csp;


}


#endif

#define MAX_FRAME_PASSES 256
#define MAX_BLEND_PASSES 8
#define MAX_BLEND_FRAMES 8

enum RenderMethod;


class CRendererPL : public CRendererHQ
{
  class CRenderBufferImpl;
public:
  ~CRendererPL() = default;

  CRenderInfo GetRenderInfo() override;
  bool Supports(ESCALINGMETHOD method) const override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool WantsDoublePass() override { return true; }
  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);
  static DXGI_FORMAT GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt);
protected:
  explicit CRendererPL(CVideoSettings& videoSettings);

  void CheckVideoParameters() override;
  void RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags) override;
  CRenderBuffer* CreateBuffer() override;


private:
  
  // Color space conversion helpers
  enum pl_color_primaries GetLibplaceboPrimaries(AVColorPrimaries primaries);
  enum pl_color_transfer GetLibplaceboTransfer(AVColorTransferCharacteristic transfer);

  // Color space info
  pl_color_space m_colorSpace;
  pl_chroma_location m_chromaLocation;

  // Tracking for parameter changes
  AVColorSpace m_lastColorSpace = AVCOL_SPC_UNSPECIFIED;
  AVColorTransferCharacteristic m_lastColorTransfer = AVCOL_TRC_UNSPECIFIED;
  AVColorPrimaries m_lastPrimaries = AVCOL_PRI_UNSPECIFIED;

  AVPixelFormat m_format;
  std::vector<pl_color_primaries> m_testprimaries;
  std::vector<pl_color_transfer> m_testtransfer;
  std::vector<pl_color_system> m_testsystem;
};

class CRendererPL::CRenderBufferImpl : public CRenderBuffer
{
public:
  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  bool UploadBuffer() override;
  bool GetLibplaceboFrame(pl_frame& frame);

private:
  bool UploadToTexture();

  pl_bit_encoding plbits;
  pl_plane plplanes[3] = {};
  pl_tex pltex[3] = {};
};
