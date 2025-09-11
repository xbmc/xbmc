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
  void ProcessHDR(CRenderBuffer* rb) override;

private:
  // Color space info
  pl_color_space m_colorSpace;
  pl_chroma_location m_chromaLocation;

  // Tracking for parameter changes
  AVColorSpace m_lastColorSpace = AVCOL_SPC_UNSPECIFIED;
  AVColorTransferCharacteristic m_lastColorTransfer = AVCOL_TRC_UNSPECIFIED;
  AVColorPrimaries m_lastPrimaries = AVCOL_PRI_UNSPECIFIED;

  AVPixelFormat m_format;
};

class CRendererPL::CRenderBufferImpl : public CRenderBuffer
{
public:
  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  void AppendPicture(const VideoPicture& picture) override;
  bool UploadBuffer() override;
  bool GetLibplaceboFrame(pl_frame& frame);
  bool HasHdrData();
  pl_color_space hdrColorSpace; //< pl_color_space
  pl_color_repr doviColorRepr;
  pl_dovi_metadata doviPlMetadata;
  pl_hdr_metadata hdrDoviRpu; //< pl_hdr_metadata
private:
  bool UploadToTexture();
  bool hasHDR10PlusMetadata = false;
  bool hasDoviMetadata = false;
  bool hasDoviRpuMetadata = false;
  AVDynamicHDRPlus hdrMetadata;
  AVDOVIMetadata doviMetadata;
  
  
  
  pl_bit_encoding plbits;
  pl_plane plplanes[3] = {};
  pl_tex pltex[3] = {};
};
