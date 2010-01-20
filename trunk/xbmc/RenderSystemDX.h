/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#ifndef RENDER_SYSTEM_DX_H
#define RENDER_SYSTEM_DX_H

#pragma once

#include <vector>
#include "RenderSystem.h"
#include "CriticalSection.h"

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
  virtual bool PresentRender();
  virtual bool ClearBuffers(color_t color);
  virtual bool ClearBuffers(float r, float g, float b, float a);
  virtual bool IsExtSupported(const char* extension);

  virtual void SetVSync(bool vsync);

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);

  virtual void CaptureStateBlock();
  virtual void ApplyStateBlock();

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight);

  virtual void ApplyHardwareTransform(const TransformMatrix &matrix);
  virtual void RestoreHardwareTransform();

  virtual bool TestRender();

  LPDIRECT3DDEVICE9 Get3DDevice() { return m_pD3DDevice; }
  int GetBackbufferCount() const { return m_D3DPP.BackBufferCount; }

  /*!
   \brief Register as a dependent of the DirectX Render System
   Resources should call this on construction if they're dependent on the Render System
   for survival. Any resources that registers will get callbacks on loss and reset of
   device, where resources that are in the D3DPOOL_DEFAULT pool should be handled.
   In addition, callbacks for destruction and creation of the device are also called,
   where any resources dependent on the DirectX device should be destroyed and recreated.
   \sa Unregister, ID3DResource
  */
  void Register(ID3DResource *resource);

  /*!
   \brief Unregister as a dependent of the DirectX Render System
   Resources should call this on destruction if they're a dependent on the Render System
   \sa Register, ID3DResource
  */
  void Unregister(ID3DResource *resource);

protected:
  bool CreateDevice();
  void DeleteDevice();
  void OnDeviceLost();
  void OnDeviceReset();
  bool PresentRenderImpl();

  void SetFocusWnd(HWND wnd) { m_hFocusWnd = wnd; }
  void SetDeviceWnd(HWND wnd) { m_hDeviceWnd = wnd; }
  void SetMonitor(HMONITOR monitor);
  void SetRenderParams(unsigned int width, unsigned int height, bool fullScreen, float refreshRate);
  void BuildPresentParameters();
  virtual void UpdateMonitor() {};

  LPDIRECT3D9 m_pD3D;

  // our adapter could change as we go
  bool m_needNewDevice;
  unsigned int m_adapter;
  LPDIRECT3DDEVICE9 m_pD3DDevice;
  unsigned int m_screenHeight;

  D3DPRESENT_PARAMETERS m_D3DPP;
  HWND m_hFocusWnd;
  HWND m_hDeviceWnd;
  unsigned int m_nBackBufferWidth;
  unsigned int m_nBackBufferHeight;
  bool m_bFullScreenDevice;
  float m_refreshRate;
  HRESULT m_nDeviceStatus;
  IDirect3DStateBlock9* m_stateBlock;
  int64_t m_systemFreq;

  CCriticalSection m_resourceSection;
  std::vector<ID3DResource*>  m_resources;

  bool m_inScene; ///< True if we're in a BeginScene()/EndScene() block
};

#endif // RENDER_SYSTEM_DX
