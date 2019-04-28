/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemWin10.h"
#include "rendering/dx/RenderSystemDX.h"

class CWinSystemWin10DX : public CWinSystemWin10, public CRenderSystemDX
{
public:
  CWinSystemWin10DX();
  ~CWinSystemWin10DX();

  // Implementation of CWinSystemBase via CWinSystemWin10
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void PresentRenderImpl(bool rendered) override;
  bool DPIChanged(WORD dpi, RECT windowRect) const override;
  bool DestroyRenderSystem() override;
  void* GetHWContext() override { return m_deviceResources->GetD3DContext(); }

  void UninitHooks();
  void InitHooks(IDXGIOutput* pOutput);

  void OnMove(int x, int y) override;
  void OnResize(int width, int height);
  winrt::Windows::Foundation::Size GetOutputSize() const { return m_deviceResources->GetOutputSize(); }
  void TrimDevice() const { m_deviceResources->Trim(); }

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

  void Register(IDispResource *resource) override { CWinSystemWin10::Register(resource); };
  void Unregister(IDispResource *resource) override { CWinSystemWin10::Unregister(resource); };

  void ShowSplash(const std::string& message) override;

protected:
  void SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res) override;
  void ReleaseBackBuffer() override;
  void CreateBackBuffer() override;
  void ResizeDeviceBuffers() override;
  bool IsStereoEnabled() override;
};

