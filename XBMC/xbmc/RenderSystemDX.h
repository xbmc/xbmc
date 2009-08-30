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

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>
#include <dxdiag.h>
#include "RenderSystem.h"

#ifdef HAS_DX

class CRenderSystemDX : public CRenderSystemBase
{
public:
  CRenderSystemDX();
  ~CRenderSystemDX();

  virtual bool Create();
  virtual bool Destroy();
  virtual bool AttachWindow(CWinSystem* winSystem);
  LPDIRECT3D9 GetD3D() { return m_pD3D; }
  LPDIRECT3DDEVICE9 GetD3DDevice() { return m_pD3DDevice; }
  D3DPRESENT_PARAMETERS* GetD3DParams() { return &m_D3DPP; }

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool Present();

  virtual bool ClearBuffers(DWORD color);
  virtual bool ClearBuffers(float r, float g, float b, float a);

  virtual bool Test();

protected:
  bool CreateResources();
  bool DeleteResources();
  bool CreateDevice();

private:
  LPDIRECT3D9 m_pD3D;
  LPDIRECT3DDEVICE9 m_pD3DDevice;
  D3DPRESENT_PARAMETERS m_D3DPP;
};

extern CRenderSystemDX g_RenderSystem;

#endif

#endif // RENDER_SYSTEM_H
