/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RendererHQ.h"
#include "VideoRenderers/RenderFlags.h"

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <libplacebo/d3d11.h>
#include <libplacebo/renderer.h>
#include <libplacebo/colorspace.h>

class CRendererLibplacebo : public CRendererHQ
{
  class CRenderBufferImpl;

public:
  CRendererLibplacebo(CVideoSettings& videoSettings);
  ~CRendererLibplacebo() override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);

  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;
  CRenderInfo GetRenderInfo() override;

protected:
  void CheckVideoParameters() override;
  void RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags) override;
  CRenderBuffer* CreateBuffer() override;

private:
  
  void UnInit();
  void UpdateColorspace(const VideoPicture& picture);
  bool CreateTargetTexture(CD3DTexture& target, pl_tex& plTex);

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
};

class CRendererLibplacebo::CRenderBufferImpl : public CRenderBuffer
{
public:
  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  bool UploadBuffer() override;
  bool GetLibplaceboFrame(pl_frame& frame);

private:
  bool UploadPlanes();
  bool UploadYUV420P(uint8_t* src[3], int srcStrides[3]);
  bool UploadNV12(uint8_t* src[3], int srcStrides[3]);
  bool UploadYUV420P_HighBit(uint8_t* src[3], int srcStrides[3], int bitDepth);
  bool UploadP010_P016(uint8_t* src[3], int srcStrides[3], int bitDepth);

  pl_tex m_plTextures[3]; // Y, U, V or Y, UV planes
};