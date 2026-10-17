/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "RendererHQ.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"

#include <map>

#include <d3d11_4.h>

extern "C"
{
#include <libavutil/pixfmt.h>
}

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2
#define PLANE_UV 1

enum RenderMethod;
class CDolbyVisionEffect;

class CRendererShaders : public CRendererHQ
{
  class CRenderBufferImpl;
public:
  ~CRendererShaders() override;

  bool Supports(ESCALINGMETHOD method) const override;
  bool Configure(const VideoPicture& picture, float fps, unsigned orientation) override;

  static CRendererBase* Create(CVideoSettings& videoSettings);
  static void GetWeight(std::map<RenderMethod, int>& weights, const VideoPicture& picture);

protected:
  explicit CRendererShaders(CVideoSettings& videoSettings);
  void RenderImpl(CD3DTexture& target, CRect& sourceRect, CPoint(&destPoints)[4], uint32_t flags) override;
  void CheckVideoParameters() override;
  void UpdateVideoFilters() override;
  CRenderBuffer* CreateBuffer() override;
  static bool IsHWPicSupported(const VideoPicture& picture);

private:
  static AVColorPrimaries GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned int width, unsigned int height);
  DXGI_FORMAT CalcIntermediateTargetFormat() const;
#if defined(TARGET_WINDOWS_STORE)
  bool RenderDolbyVision(CD3DTexture& target, CRenderBuffer* buf);
  void DisableDolbyVisionEffect(const char* reason);
  bool UploadDolbyVisionFrame(CRenderBuffer* buf);

  std::unique_ptr<CDolbyVisionEffect> m_dvEffect;
  ID3D11Texture2D* m_dvOutput = nullptr;
  uint64_t m_dvFrameIdx = UINT64_MAX; // last frame fed to the effect
  bool m_dvEffectDolbyVisionOutput = false; // m_DolbyVisionOutput when the effect was opened
  CD3DTexture m_dvUpload; // P010 copy of a software decoded frame
#endif

  AVColorPrimaries m_srcPrimaries = AVCOL_PRI_BT709;
  std::unique_ptr<CYUV2RGBShader> m_colorShader;
};

class CRendererShaders::CRenderBufferImpl : public CRenderBuffer
{
public:
  explicit CRenderBufferImpl(AVPixelFormat av_pix_format, unsigned width, unsigned height);
  ~CRenderBufferImpl();

  void AppendPicture(const VideoPicture& picture) override;
  bool UploadBuffer() override;
  unsigned GetViewCount() const override;
  ID3D11View* GetView(unsigned viewIdx) override;
  void ReleasePicture() override;

private:
  bool UploadFromGPU();
  bool UploadFromBuffer() const;

  unsigned m_viewCount = 0;
  CD3DTexture m_textures[YuvImage::MAX_PLANES];
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planes[2];
};
