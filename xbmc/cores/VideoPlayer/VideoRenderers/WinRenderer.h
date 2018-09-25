/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BaseRenderer.h"
#include "ColorManager.h"
#include "guilib/D3DResource.h"
#include "HwDecRender/DXVAHD.h"
#include "RenderCapture.h"
#include "WinRenderBuffer.h"

#include <wrl/client.h>

#define AUTOSOURCE -1

class CYUV2RGBShader;
class CConvolutionShader;
class COutputShader;
struct VideoPicture;
enum EBufferFormat;

enum RenderMethod
{
  RENDER_INVALID = 0x00,
  RENDER_PS      = 0x01,
  RENDER_SW      = 0x02,
  RENDER_DXVA    = 0x03,
};

class CWinRenderer : public CBaseRenderer
{
public:
  CWinRenderer();
  ~CWinRenderer();

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  void Update() override;
  bool RenderCapture(CRenderCapture* capture) override;

  // Player functions
  bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;
  void AddVideoPicture(const VideoPicture &picture, int index) override;
  void UnInit() override;
  bool IsConfigured() override { return m_bConfigured; }
  bool Flush(bool saveBuffers) override;
  CRenderInfo GetRenderInfo() override;
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  void SetBufferSize(int numBuffers) override { m_neededBuffers = numBuffers; }
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;

  // Feature support
  bool SupportsMultiPassRendering() override { return false; }
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

  bool WantsDoublePass() override;
  bool ConfigChanged(const VideoPicture& picture) override;

protected:
  void PreInit();
  virtual void Render(DWORD flags, CD3DTexture* target);
  void RenderSW(CD3DTexture* target);
  void RenderHW(DWORD flags, CD3DTexture* target);
  void RenderPS(CD3DTexture* target);
  void RenderHQ(CD3DTexture* target);
  void ManageTextures();
  void DeleteRenderBuffer(int index);
  bool CreateRenderBuffer(int index);
  int NextBuffer() const;
  void SelectRenderMethod();
  void UpdateVideoFilter();
  void SelectSWVideoFilter();
  void SelectPSVideoFilter();
  void UpdatePSVideoFilter();
  void ColorManagmentUpdate();
  bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height, bool dynamic);
  EBufferFormat SelectBufferFormat(AVPixelFormat format, const RenderMethod method) const;
  AVColorPrimaries GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned int width, unsigned int height) const;

  bool LoadCLUT();

  bool m_bConfigured;
  bool m_bUseHQScaler;
  bool m_bFilterInitialized;
  bool m_cmsOn;
  bool m_clutLoaded;
  bool m_useDithering;
  bool m_toneMapping;

  unsigned int m_destWidth;
  unsigned int m_destHeight;
  unsigned int m_frameIdx;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;
  int m_neededBuffers;
  int m_iRequestedMethod;
  int m_cmsToken{ -1 };
  int m_CLUTSize{ 0 };
  int m_ditherDepth;

  DXGI_FORMAT m_dxva_format;
  RenderMethod m_renderMethod;
  EBufferFormat m_bufferFormat;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;
  CRenderBuffer m_renderBuffers[NUM_BUFFERS];

  struct SwsContext *m_sw_scale_ctx;
  CRenderCapture* m_capture;
  std::unique_ptr<DXVA::CProcessorHD> m_processor;
  std::unique_ptr<CYUV2RGBShader> m_colorShader;
  std::unique_ptr<CConvolutionShader> m_scalerShader;
  std::unique_ptr<COutputShader> m_outputShader;
  std::unique_ptr<CColorManager> m_colorManager;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pCLUTView;

  CD3DTexture m_IntermediateTarget;
  AVColorPrimaries m_srcPrimaries;
};
