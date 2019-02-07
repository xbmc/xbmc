/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <concrt.h>
#if defined(TARGET_WINDOWS_STORE)
#include <dxgi1_3.h>
#else
#include <dxgi1_2.h>
#include <easyhook/easyhook.h>
#endif
#include <functional>
#include <memory>

#include "DirectXHelper.h"
#include "guilib/D3DResource.h"

struct RESOLUTION_INFO;

namespace DX
{
  interface IDeviceNotify
  {
    virtual void OnDXDeviceLost() = 0;
    virtual void OnDXDeviceRestored() = 0;
  };

  // Controls all the DirectX device resources.
  class DeviceResources
  {
  public:
    static std::shared_ptr<DX::DeviceResources> Get();

    DeviceResources();
    virtual ~DeviceResources();
    void Release();

    void ValidateDevice();
    void HandleDeviceLost(bool removed);
    bool Begin();
    void Present();

    // The size of the render target, in pixels.
    winrt::Windows::Foundation::Size GetOutputSize() const { return m_outputSize; }
    // The size of the render target, in dips.
    winrt::Windows::Foundation::Size GetLogicalSize() const { return m_logicalSize; }
    void SetLogicalSize(float width, float height);
    float GetDpi() const { return m_effectiveDpi; }
    void SetDpi(float dpi);

    // D3D Accessors.
    bool HasValidDevice() const { return m_bDeviceCreated; }
    ID3D11Device1* GetD3DDevice() const { return m_d3dDevice.Get(); }
    ID3D11DeviceContext1* GetD3DContext() const { return m_deferrContext.Get(); }
    ID3D11DeviceContext1* GetImmediateContext() const { return m_d3dContext.Get(); }
    IDXGISwapChain1* GetSwapChain() const { return m_swapChain.Get(); }
    IDXGIFactory2* GetIDXGIFactory2() const { return m_dxgiFactory.Get(); }
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }
    ID3D11RenderTargetView* GetBackBufferRTV();
    ID3D11DepthStencilView* GetDSV() const { return m_d3dDepthStencilView.Get(); }
    D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
    CD3DTexture* GetBackBuffer() { return &m_backBufferTex; }

    void GetOutput(IDXGIOutput** ppOutput) const;
    void GetAdapterDesc(DXGI_ADAPTER_DESC *desc) const;
    void GetDisplayMode(DXGI_MODE_DESC *mode) const;

    D3D11_VIEWPORT GetScreenViewport() const { return m_screenViewport; }
    void SetViewPort(D3D11_VIEWPORT& viewPort) const;

    void ReleaseBackBuffer();
    void CreateBackBuffer();
    void ResizeBuffers();

    bool SetFullScreen(bool fullscreen, RESOLUTION_INFO& res);

    // DX resources registration
    void Register(ID3DResource *resource);
    void Unregister(ID3DResource *resource);

    void FinishCommandList(bool bExecute = true) const;
    void ClearDepthStencil() const;
    void ClearRenderTarget(ID3D11RenderTargetView* pRTView, float color[4]) const;
    void RegisterDeviceNotify(IDeviceNotify* deviceNotify);

    bool IsStereoAvailable() const;
    bool IsStereoEnabled() const { return m_stereoEnabled; }
    void SetStereoIdx(byte idx) { m_backBufferTex.SetViewIdx(idx); }

    void SetMonitor(HMONITOR monitor);
    HMONITOR GetMonitor() const;
#if defined(TARGET_WINDOWS_DESKTOP)
    void SetWindow(HWND window);
#elif defined(TARGET_WINDOWS_STORE)
    void Trim() const;
    void SetWindow(const winrt::Windows::UI::Core::CoreWindow& window);
    void SetWindowPos(winrt::Windows::Foundation::Rect rect);
#endif // TARGET_WINDOWS_STORE
    bool DoesTextureSharingWork();

  private:
    class CBackBuffer : public CD3DTexture
    {
    public:
      CBackBuffer() : CD3DTexture() {}
      void SetViewIdx(unsigned idx) { m_viewIdx = idx; }
      bool Acquire(ID3D11Texture2D* pTexture);
    };

    HRESULT CreateSwapChain(DXGI_SWAP_CHAIN_DESC1 &desc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fsDesc, IDXGISwapChain1 **ppSwapChain) const;
    void CreateDeviceIndependentResources();
    void CreateDeviceResources();
    void CreateWindowSizeDependentResources();
    void UpdateRenderTargetSize();
    void OnDeviceLost(bool removed);
    void OnDeviceRestored();
    void HandleOutputChange(const std::function<bool(DXGI_OUTPUT_DESC)>& cmpFunc);
    bool CreateFactory();

    HWND m_window{ nullptr };
#if defined(TARGET_WINDOWS_STORE)
    winrt::Windows::UI::Core::CoreWindow m_coreWindow = nullptr;
#endif
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
    Microsoft::WRL::ComPtr<IDXGIOutput1> m_output;

    Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_deferrContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D11Debug> m_d3dDebug;
#endif

    CBackBuffer m_backBufferTex;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
    D3D11_VIEWPORT m_screenViewport;

    // Cached device properties.
    D3D_FEATURE_LEVEL m_d3dFeatureLevel;
    winrt::Windows::Foundation::Size m_outputSize;
    winrt::Windows::Foundation::Size m_logicalSize;
    float m_dpi;

    // This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
    float m_effectiveDpi;
    // The IDeviceNotify can be held directly as it owns the DeviceResources.
    IDeviceNotify* m_deviceNotify;

    // scritical section
    Concurrency::critical_section m_criticalSection;
    Concurrency::critical_section m_resourceSection;
    std::vector<ID3DResource*> m_resources;
    bool m_stereoEnabled;
    bool m_bDeviceCreated;
  };
}
