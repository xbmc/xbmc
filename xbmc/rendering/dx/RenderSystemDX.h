/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DeviceResources.h"
#include "rendering/RenderSystem.h"
#include "threads/Condition.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "utils/ColorUtils.h"

#include <wrl/client.h>

class ID3DResource;
class CGUIShaderDX;
enum AVPixelFormat;
enum AVPixelFormat;

class CRenderSystemDX : public CRenderSystemBase, DX::IDeviceNotify
{
public:
  CRenderSystemDX();
  virtual ~CRenderSystemDX();

  // CRenderBase overrides
  bool InitRenderSystem() override;
  bool DestroyRenderSystem() override;
  bool BeginRender() override;
  bool EndRender() override;
  void PresentRender(bool rendered, bool videoLayer) override;
  bool ClearBuffers(UTILS::COLOR::Color color) override;
  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;
  void RestoreViewPort() override;
  CRect ClipRectToScissorRect(const CRect &rect) override;
  bool ScissorsCanEffectClipping() override;
  void SetScissors(const CRect &rect) override;
  void ResetScissors() override;
  void CaptureStateBlock() override;
  void ApplyStateBlock() override;
  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.f) override;
  void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view) override;
  bool SupportsStereo(RENDER_STEREO_MODE mode) const override;
  void Project(float &x, float &y, float &z) override;
  bool SupportsNPOT(bool dxt) const override;

  // IDeviceNotify overrides
  void OnDXDeviceLost() override;
  void OnDXDeviceRestored() override;

  // CRenderSystemDX methods
  CGUIShaderDX* GetGUIShader() const { return m_pGUIShader; }
  bool IsFormatSupport(DXGI_FORMAT format, unsigned int usage) const;
  CRect GetBackBufferRect();
  CD3DTexture& GetBackBuffer();

  void FlushGPU() const;
  void RequestDecodingTime();
  void ReleaseDecodingTime();
  void SetAlphaBlendEnable(bool enable);

  // empty overrides
  bool IsExtSupported(const char* extension) const override { return false; }
  bool ResetRenderSystem(int width, int height) override { return true; }

protected:
  virtual void PresentRenderImpl(bool rendered) = 0;

  bool CreateStates();
  bool InitGUIShader();
  void OnResize();
  void CheckInterlacedStereoView(void);
  void CheckDeviceCaps();

  CCriticalSection m_resourceSection;
  CCriticalSection m_decoderSection;

  // our adapter could change as we go
  bool m_inScene{ false }; ///< True if we're in a BeginScene()/EndScene() block
  bool m_BlendEnabled{ false };
  bool m_ScissorsEnabled{ false };
  D3D11_VIEWPORT m_viewPort;
  CRect m_scissor;
  CGUIShaderDX* m_pGUIShader{ nullptr };
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> m_BlendEnableState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> m_BlendDisableState;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_RSScissorDisable;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_RSScissorEnable;
  // stereo interlaced/checkerboard intermediate target
  CD3DTexture m_rightEyeTex;

  XbmcThreads::EndTime<> m_decodingTimer;
  XbmcThreads::ConditionVariable m_decodingEvent;

  std::shared_ptr<DX::DeviceResources> m_deviceResources;
};

