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

#include "DSRendererCallback.h"
#include "threads/Event.h"

class CEvrSharedRender: public IDSRendererPaintCallback
{

public:
  CEvrSharedRender();
  virtual ~CEvrSharedRender();

  // IEvrPaintCallback
  virtual void RenderToUnderTexture();
  virtual void RenderToOverTexture();
  virtual void EndRender();
  
  HRESULT CreateTextures(ID3D11Device* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceEvr, int width, int height);
  HRESULT Render(DS_RENDER_LAYER layer);  

private:
  HRESULT CreateSharedResource(IDirect3DTexture9** ppTexture9, ID3D11Texture2D** ppTexture11);
  HRESULT CreateFakeStaging(ID3D11Texture2D** ppTexture);
  HRESULT ForceComplete();
  
  HRESULT RenderEvr(DS_RENDER_LAYER layer);  
  HRESULT RenderTexture(DS_RENDER_LAYER layer);
  HRESULT SetupVertex();
  HRESULT StoreEvrDeviceState();
  HRESULT SetupEvrDeviceState();
  HRESULT RestoreEvrDeviceState();

  ID3D11Device*             m_pD3DDeviceKodi = nullptr;
  IDirect3DDevice9Ex*       m_pD3DDeviceEvr = nullptr;

  IDirect3DVertexBuffer9*   m_pEvrVertexBuffer = nullptr;
  IDirect3DTexture9*        m_pEvrUnderTexture = nullptr;
  IDirect3DTexture9*        m_pEvrOverTexture = nullptr;

  ID3D11Texture2D*          m_pKodiUnderTexture = nullptr;
  ID3D11Texture2D*          m_pKodiOverTexture = nullptr;
  ID3D11Texture2D*          m_pKodiFakeStaging = nullptr;

  DWORD m_dwWidth = 0;
  DWORD m_dwHeight = 0;
  float m_fColor[4];
  bool m_bUnderRender;
  bool m_bGuiVisible;
  bool m_bGuiVisibleOver;

  // stored Evr device state
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
