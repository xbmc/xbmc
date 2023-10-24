/*
 *  Copyright (C) 2017-2019 Team Kodi
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

extern "C"
{
#include <libavutil/pixfmt.h>
}

enum RenderMethod;

class CRendererDXVA : public CRendererHQ
{
  class CRenderBufferImpl;
public:
  ~CRendererDXVA() = default;

  CRenderInfo GetRenderInfo() override;
  bool Supports(ESCALINGMETHOD method) const override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool WantsDoublePass() override { return true; }
  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;
  bool NeedBuffer(int idx) override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);
  static DXGI_FORMAT GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt);

protected:
  explicit CRendererDXVA(CVideoSettings& videoSettings);

  void CheckVideoParameters() override;
  void RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags) override;
  CRenderBuffer* CreateBuffer() override;
  virtual std::string GetRenderMethodDebugInfo() const;

private:
  void FillBuffersSet(CRenderBuffer* (&buffers)[8]);
  CRect ApplyTransforms(const CRect& destRect) const;
  /*!
   * \brief Choose the best available conversion for the given source and output constraints
   * \param conversions list of supported conversions
   * \return best match
   */
  DXVA::ProcessorConversion ChooseConversion(const DXVA::ProcessorConversions& conversions) const;

  std::unique_ptr<DXVA::CProcessorHD> m_processor;
  std::shared_ptr<DXVA::CEnumeratorHD> m_enumerator;
  DXVA::ProcessorConversion m_conversion;
  DXVA::SupportedConversionsArgs m_conversionsArgs;
  bool m_tryVSR{false};
};

class CRendererDXVA::CRenderBufferImpl : public CRenderBuffer
{
public:
  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  bool UploadBuffer() override;
  HRESULT GetResource(ID3D11Resource** ppResource, unsigned* index) const override;

  static DXGI_FORMAT GetDXGIFormat(AVPixelFormat format, DXGI_FORMAT default_fmt = DXGI_FORMAT_UNKNOWN);

private:
  bool UploadToTexture();

  CD3DTexture m_texture;
};
