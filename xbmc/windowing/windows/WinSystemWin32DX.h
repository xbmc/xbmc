/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "HDRStatus.h"
#include "rendering/dx/RenderSystemDX.h"
#include "windowing/windows/WinSystemWin32.h"

struct D3D10DDIARG_CREATERESOURCE;

class CWinSystemWin32DX : public CWinSystemWin32, public CRenderSystemDX
{
  friend interface DX::IDeviceNotify;
public:
  CWinSystemWin32DX();
  ~CWinSystemWin32DX();

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Implementation of CWinSystemBase via CWinSystemWin32
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void PresentRenderImpl(bool rendered) override;
  bool DPIChanged(WORD dpi, RECT windowRect) const override;
  void SetWindow(HWND hWnd) const;
  bool DestroyRenderSystem() override;
  void* GetHWContext() override { return m_deviceResources->GetD3DContext(); }

  void UninitHooks();
  void InitHooks(IDXGIOutput* pOutput);

  void OnMove(int x, int y) override;
  void OnResize(int width, int height);

  /*!
   \brief Register as a dependent of the DirectX Render System
   Resources should call this on construction if they're dependent on the Render System
   for survival. Any resources that registers will get callbacks on loss and reset of
   device. In addition, callbacks for destruction and creation of the device are also called,
   where any resources dependent on the DirectX device should be destroyed and recreated.
   \sa Unregister, ID3DResource
  */
  void Register(ID3DResource *resource) const
  {
    m_deviceResources->Register(resource);
  };
  /*!
   \brief Unregister as a dependent of the DirectX Render System
   Resources should call this on destruction if they're a dependent on the Render System
   \sa Register, ID3DResource
  */
  void Unregister(ID3DResource *resource) const
  {
    m_deviceResources->Unregister(resource);
  };

  void Register(IDispResource* resource) override { CWinSystemWin32::Register(resource); }
  void Unregister(IDispResource* resource) override { CWinSystemWin32::Unregister(resource); }

  void FixRefreshRateIfNecessary(const D3D10DDIARG_CREATERESOURCE* pResource) const;

  // HDR OS/display override
  bool IsHDRDisplay() override;
  HDR_STATUS ToggleHDR() override;
  HDR_STATUS GetOSHDRStatus() override;

  // HDR support
  bool IsHDROutput() const;
  bool IsTransferPQ() const;
  void SetHdrMetaData(DXGI_HDR_METADATA_HDR10& hdr10) const;
  void SetHdrColorSpace(const DXGI_COLOR_SPACE_TYPE colorSpace) const;

  // Get debug info from swapchain
  DEBUG_INFO_RENDER GetDebugInfo() override;

  bool SupportsVideoSuperResolution() override;

protected:
  void SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res) override;
  void ReleaseBackBuffer() override;
  void CreateBackBuffer() override;
  void ResizeDeviceBuffers() override;
  bool IsStereoEnabled() override;
  void OnScreenChange(HMONITOR monitor) override;
  bool ChangeResolution(const RESOLUTION_INFO& res, bool forceChange = false) override;

  HMODULE m_hDriverModule;
};

