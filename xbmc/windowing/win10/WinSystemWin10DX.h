/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

