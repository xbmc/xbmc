/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "RendererBase.h"

#include <map>
extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

class CRenderBufferSoftware : public CRenderBufferBase
{
public:
  explicit CRenderBufferSoftware(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferSoftware();

  void AppendPicture(const VideoPicture& picture) override;
  bool GetDataPlanes(uint8_t*(& planes)[3], int(& strides)[3]) override;

  void ReleasePicture() override;
  bool IsLoaded() override;
  bool UploadBuffer() override;

private:
  D3D11_MAPPED_SUBRESOURCE m_msr{};
};

class CRendererSoftware : public CRendererBase
{
public:
  ~CRendererSoftware();

  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;
  bool Supports(ESCALINGMETHOD method) override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);

protected:
  explicit CRendererSoftware(CVideoSettings& videoSettings) : CRendererBase(videoSettings) {}
  CRenderBufferBase* CreateBuffer() override;
  void RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags) override;
  void FinalOutput(CD3DTexture& source, CD3DTexture& target, const CRect& src, const CPoint(&destPoints)[4]) override;

private:
  SwsContext* m_sw_scale_ctx = nullptr;
};
