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

#include "RenderSystem.h"
#include <vector>

class CRenderSystemDX : public CRenderSystemBase
{
public:
  CRenderSystemDX();
  virtual ~CRenderSystemDX();

  // CRenderBase
  virtual bool InitRenderSystem();
  virtual bool DestroyRenderSystem();
  virtual bool ResetRenderSystem(int width, int height);  

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool PresentRender();
  virtual bool ClearBuffers(color_t color);
  virtual bool ClearBuffers(float r, float g, float b, float a);
  virtual bool IsExtSupported(const char* extension);

  virtual void SetVSync(bool vsync);
  virtual bool GetVSync() { return m_bVSync; }

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);
  
  virtual bool NeedPower2Texture() { return m_NeedPower2Texture; }
  virtual bool SupportsCompressedTextures();

  virtual void CaptureStateBlock();
  virtual void ApplyStateBlock();

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight);

  virtual void ApplyHardwareTransform(const TransformMatrix &matrix);
  virtual void RestoreHardwareTransform();

  virtual bool TestRender();

  // CRenderSystemDX
  virtual void SetFocusWnd(HWND wnd) { m_hFocusWnd = wnd; }
  virtual void SetDeviceWnd(HWND wnd) { m_hDeviceWnd = wnd; }
  virtual void SetBackBufferSize(unsigned int width, unsigned int height) { m_nBackBufferWidth = width; m_nBackBufferHeight = height; };
  virtual void SetDeviceFullScreen(bool bFullScreen){ m_bFullScreenDevice = bFullScreen; };
  virtual LPDIRECT3DDEVICE9 Get3DDevice() { return m_pD3DDevice; }
  int GetBackbufferCount() const { return m_D3DPP.BackBufferCount; }
  bool IsDeviceReady() const { return m_nDeviceStatus == S_OK; }
  virtual bool CreateEffect(CStdString& name, ID3DXEffect** pEffect);
  virtual void ReleaseEffect(ID3DXEffect* pEffect);

protected:
  virtual bool CreateResources();
  virtual void DeleteResources();
  virtual void OnDeviceLost();
  virtual void OnDeviceReset();
  virtual void SetVSyncImpl(bool enable){};
  virtual bool PresentRenderImpl();
  void CalculateMaxTexturesize();
  virtual bool CreateDevice();
  
  LPDIRECT3D9 m_pD3D;
  LPDIRECT3DDEVICE9 m_pD3DDevice;
  D3DPRESENT_PARAMETERS m_D3DPP;
  HWND m_hFocusWnd;
  HWND m_hDeviceWnd;
  unsigned int m_nBackBufferWidth;
  unsigned int m_nBackBufferHeight;
  bool m_bFullScreenDevice;
  HRESULT m_nDeviceStatus;
  IDirect3DStateBlock9* m_stateBlock;

  std::vector<ID3DXEffect*> m_vecEffects;

  int        m_iVSyncMode;
};

#endif // RENDER_SYSTEM_DX
