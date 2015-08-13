/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc
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

#include "MadvrCallback.h"
#include "SurfaceQueueLib/SurfaceQueue.h"

class CMadvrSharedRender
{
public:
  CMadvrSharedRender();
  virtual ~CMadvrSharedRender();

  HRESULT CreateTextures(ID3D11Device* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceMadVR, int width, int height);
  HRESULT Render(MADVR_RENDER_LAYER layer);  HRESULT RenderToTexture(MADVR_RENDER_LAYER layer);
  void Flush(MADVR_RENDER_LAYER layer);
  
private:
  HRESULT CreateSharedResource(IDirect3DTexture9** ppTextureMadvr, IDirect3DTexture9** ppTextureKodi, IDirect3DSurface9** ppSurfaceKodi);
  void RenderKodi(MADVR_RENDER_LAYER layer);
  HRESULT RenderMadvr(MADVR_RENDER_LAYER layer);  HRESULT RenderTexture(MADVR_RENDER_LAYER layer);
  HRESULT SetupVertex();
  HRESULT StoreMadDeviceState();
  HRESULT SetupMadDeviceState();
  HRESULT RestoreMadDeviceState();

  ID3D11Device*             m_pD3DDeviceKodi = nullptr;
  IDirect3DDevice9Ex*       m_pD3DDeviceMadVR = nullptr;

  IDirect3DVertexBuffer9*   m_pMadvrVertexBuffer = nullptr;
  ID3D11Texture2D*          m_pKodiTexture = nullptr;
  ID3D11RenderTargetView*   m_pKodiSurface = nullptr;
  IDirect3DTexture9*        m_pMadvrTexture = nullptr;
  
  ISurfaceQueue*            m_pD3D11Queue = nullptr;
  ISurfaceQueue*            m_pD3D9Queue = nullptr;
  ISurfaceProducer*         m_pD3D11Producer = nullptr;
  ISurfaceConsumer*         m_pD3D11Consumer = nullptr;
  ISurfaceProducer*         m_pD3D9Producer = nullptr;
  ISurfaceConsumer*         m_pD3D9Consumer = nullptr;

  DWORD m_dwWidth = 0;
  DWORD m_dwHeight = 0;
  float m_fColor[4];
  bool bUnderRender;

  // stored madVR device state
  IDirect3DVertexShader9* m_pOldVS = nullptr;
  IDirect3DVertexBuffer9* m_pOldStreamData = nullptr;
  IDirect3DBaseTexture9* m_pOldTexture = nullptr;

  DWORD m_dwOldFVF = 0;
  UINT  m_nOldOffsetInBytes = 0;
  UINT  m_nOldStride = 0;
  RECT  m_oldScissorRect;

  DWORD m_D3DRS_CULLMODE = 0;
  DWORD m_D3DRS_LIGHTING = 0;
  DWORD m_D3DRS_ZENABLE = 0;
  DWORD m_D3DRS_ALPHABLENDENABLE = 0;
  DWORD m_D3DRS_SRCBLEND = 0;
  DWORD m_D3DRS_DESTBLEND = 0;

  IDirect3DPixelShader9* m_pPix = nullptr;
};
