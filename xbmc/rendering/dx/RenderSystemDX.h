/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef RENDER_SYSTEM_DX_H
#define RENDER_SYSTEM_DX_H

#ifdef HAS_DX

#pragma once

#include <vector>
#include "rendering/RenderSystem.h"
#include "guilib/GUIShaderDX.h"
#include "threads/CriticalSection.h"

enum PCI_Vendors
{
  PCIV_ATI    = 0x1002,
  PCIV_nVidia = 0x10DE,
  PCIV_Intel  = 0x8086
};

class ID3DResource;

class CRenderSystemDX : public CRenderSystemBase
{
public:
  CRenderSystemDX();
  virtual ~CRenderSystemDX();

  // CRenderBase
  virtual bool InitRenderSystem();
  virtual bool DestroyRenderSystem();
  virtual bool ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate);
  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool ClearBuffers(color_t color);
  virtual bool IsExtSupported(const char* extension);
  virtual bool IsFormatSupport(DXGI_FORMAT format, unsigned int usage);
  virtual void SetVSync(bool vsync);
  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);
  virtual void RestoreViewPort();
  virtual CRect ClipRectToScissorRect(const CRect &rect);
  virtual bool ScissorsCanEffectClipping();
  virtual void SetScissors(const CRect &rect);
  virtual void ResetScissors();
  virtual void CaptureStateBlock();
  virtual void ApplyStateBlock();
  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.f);
  virtual void ApplyHardwareTransform(const TransformMatrix &matrix);
  virtual void RestoreHardwareTransform();
  virtual void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view);
  virtual bool SupportsStereo(RENDER_STEREO_MODE mode) const;
  virtual bool TestRender();
  virtual void Project(float &x, float &y, float &z);
  virtual CRect GetBackBufferRect() { return CRect(0.f, 0.f, static_cast<float>(m_nBackBufferWidth), static_cast<float>(m_nBackBufferHeight)); }

  IDXGIOutput* GetCurrentOutput(void) { return m_pOutput; }
  void GetDisplayMode(DXGI_MODE_DESC *mode, bool useCached = false);
  void FinishCommandList(bool bExecute = true);
  void FlushGPU();

  ID3D11Device*           Get3D11Device()      { return m_pD3DDev; }
  ID3D11DeviceContext*    Get3D11Context()     { return m_pContext; }
  ID3D11DeviceContext*    GetImmediateContext(){ return m_pImdContext; }
  CGUIShaderDX*           GetGUIShader()       { return m_pGUIShader; }
  unsigned                GetFeatureLevel()    { return m_featureLevel; }
  D3D11_USAGE             DefaultD3DUsage()    { return m_defaultD3DUsage; }
  DXGI_ADAPTER_DESC       GetAIdentifier()     { return m_adapterDesc; }
  bool                    Interlaced()         { return m_interlaced; }
  int                     GetBackbufferCount() const { return 2; }
  void                    SetAlphaBlendEnable(bool enable);

  static std::string GetErrorDescription(HRESULT hr);

protected:
  bool CreateDevice();
  void DeleteDevice();
  void OnDeviceLost();
  void OnDeviceReset();
  void PresentRenderImpl(bool rendered);

  void SetFocusWnd(HWND wnd) { m_hFocusWnd = wnd; }
  void SetDeviceWnd(HWND wnd) { m_hDeviceWnd = wnd; }
  void SetMonitor(HMONITOR monitor);
  void SetRenderParams(unsigned int width, unsigned int height, bool fullScreen, float refreshRate);
  bool CreateWindowSizeDependentResources();
  bool CreateStates();
  bool InitGUIShader();
  void OnMove();
  void OnResize(unsigned int width, unsigned int height);
  void SetFullScreenInternal();
  void GetClosestDisplayModeToCurrent(IDXGIOutput* output, DXGI_MODE_DESC* outCurrentDisplayMode, bool useCached = false);
  void CheckInterlasedStereoView(void);
  void SetMaximumFrameLatency(uint8_t latency = -1);

  bool GetStereoEnabled() const;
  bool GetDisplayStereoEnabled() const;
  void SetDisplayStereoEnabled(bool enable);
  void UpdateDisplayStereoStatus(bool isfirst = false);

  virtual void Register(ID3DResource *resource);
  virtual void Unregister(ID3DResource *resource);
  virtual void UpdateMonitor() {};
  virtual void OnDisplayLost() {};
  virtual void OnDisplayReset() {};
  virtual void OnDisplayBack() {};

  // our adapter could change as we go
  bool                        m_needNewDevice{false};
  bool                        m_needNewViews;
  bool                        m_resizeInProgress{false};
  unsigned int                m_screenHeight{0};
  HWND                        m_hFocusWnd{nullptr};
  HWND                        m_hDeviceWnd{nullptr};
  unsigned int                m_nBackBufferWidth{0};
  unsigned int                m_nBackBufferHeight{0};
  bool                        m_bFullScreenDevice{false};
  float                       m_refreshRate;
  bool                        m_interlaced;
  HRESULT                     m_nDeviceStatus{S_OK};
  int64_t                     m_systemFreq;
  D3D11_USAGE                 m_defaultD3DUsage{D3D11_USAGE_DEFAULT};
  bool                        m_useWindowedDX;
  CCriticalSection            m_resourceSection;
  std::vector<ID3DResource*>  m_resources;
  bool                        m_inScene{false}; ///< True if we're in a BeginScene()/EndScene() block
  D3D_DRIVER_TYPE             m_driverType{D3D_DRIVER_TYPE_HARDWARE};
  D3D_FEATURE_LEVEL           m_featureLevel{D3D_FEATURE_LEVEL_11_1};
  IDXGIFactory1*              m_dxgiFactory{nullptr};
  ID3D11Device*               m_pD3DDev{nullptr};
  IDXGIAdapter1*              m_adapter{nullptr};
  IDXGIOutput*                m_pOutput{nullptr};
  ID3D11DeviceContext*        m_pContext{nullptr};
  ID3D11DeviceContext*        m_pImdContext{nullptr};
  IDXGISwapChain*             m_pSwapChain{nullptr};
  IDXGISwapChain1*            m_pSwapChain1{nullptr};
  ID3D11RenderTargetView*     m_pRenderTargetView{nullptr};
  ID3D11DepthStencilState*    m_depthStencilState{nullptr};
  ID3D11DepthStencilView*     m_depthStencilView{nullptr};
  D3D11_VIEWPORT              m_viewPort;
  CRect                       m_scissor;
  CGUIShaderDX*               m_pGUIShader{nullptr};
  ID3D11BlendState*           m_BlendEnableState{nullptr};
  ID3D11BlendState*           m_BlendDisableState{nullptr};
  bool                        m_BlendEnabled{false};
  ID3D11RasterizerState*      m_RSScissorDisable{nullptr};
  ID3D11RasterizerState*      m_RSScissorEnable{nullptr};
  bool                        m_ScissorsEnabled{false};
  DXGI_ADAPTER_DESC           m_adapterDesc;
  // stereo interlaced/checkerboard intermediate target
  ID3D11Texture2D*            m_pTextureRight{nullptr};
  ID3D11RenderTargetView*     m_pRenderTargetViewRight{nullptr};
  ID3D11ShaderResourceView*   m_pShaderResourceViewRight{nullptr};
  bool                        m_bResizeRequred{false};
  bool                        m_bHWStereoEnabled{false};
  // improve get current mode
  DXGI_MODE_DESC              m_cachedMode;
#ifdef _DEBUG
  ID3D11Debug*                m_d3dDebug{nullptr};
#endif
  bool                        m_bDefaultStereoEnabled{false};
  bool                        m_bStereoEnabled{false};
};

#endif

#endif // RENDER_SYSTEM_DX
