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
#include "EvrSharedRender.h"
#include "Utils/Log.h"
#include "guilib/GUIWindowManager.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/VideoRenderers/RenderManager.h"

const DWORD D3DFVF_VID_FRAME_VERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;

struct VID_FRAME_VERTEX
{
  float x;
  float y;
  float z;
  float rhw;
  float u;
  float v;
};

HRESULT CEvrSharedRender::CreateFakeStaging(ID3D11Texture2D** ppTexture)
{
  D3D11_TEXTURE2D_DESC Desc;
  Desc.Width = 1;
  Desc.Height = 1;
  Desc.MipLevels = 1;
  Desc.ArraySize = 1;
  Desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  Desc.SampleDesc.Count = 1;
  Desc.SampleDesc.Quality = 0;
  Desc.Usage = D3D11_USAGE_STAGING;
  Desc.BindFlags = 0;
  Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  Desc.MiscFlags = 0;

  return m_pD3DDeviceKodi->CreateTexture2D(&Desc, NULL, ppTexture);
}

HRESULT CEvrSharedRender::ForceComplete()
{
  HRESULT hr = S_OK;
  D3D11_MAPPED_SUBRESOURCE region;
  D3D11_BOX UnitBox = { 0, 0, 0, 1, 1, 1 };
  ID3D11DeviceContext* pContext;

  m_pD3DDeviceKodi->GetImmediateContext(&pContext);
  pContext->CopySubresourceRegion(m_pKodiFakeStaging, 0, 0, 0, 0, m_pKodiUnderTexture, 0, &UnitBox);

  hr = pContext->Map(m_pKodiFakeStaging, 0, D3D11_MAP_READ, 0, &region);
  if (SUCCEEDED(hr))
  {
    pContext->Unmap(m_pKodiFakeStaging, 0);
    SAFE_RELEASE(pContext);
  }
  return hr;
}

CEvrSharedRender::CEvrSharedRender()
{
  color_t clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;
  CD3DHelper::XMStoreColor(m_fColor, clearColour);
  m_bUnderRender = false;
  m_bGuiVisible = false;
  m_bGuiVisibleOver = false;
}

CEvrSharedRender::~CEvrSharedRender()
{
  // release Evr resources
  SAFE_RELEASE(m_pEvrVertexBuffer);
  SAFE_RELEASE(m_pEvrUnderTexture);
  SAFE_RELEASE(m_pEvrOverTexture);

  // release Kodi resources
  SAFE_RELEASE(m_pKodiUnderTexture);
  SAFE_RELEASE(m_pKodiOverTexture);
  SAFE_RELEASE(m_pKodiFakeStaging);
}

HRESULT CEvrSharedRender::CreateSharedResource(IDirect3DTexture9** ppTexture9, ID3D11Texture2D** ppTexture11)
{
  HRESULT hr;
  HANDLE pSharedHandle = nullptr;

  if (FAILED(hr = m_pD3DDeviceEvr->CreateTexture(m_dwWidth, m_dwHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, ppTexture9, &pSharedHandle)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceKodi->OpenSharedResource(pSharedHandle, __uuidof(ID3D11Texture2D), (void**)(ppTexture11))))
    return hr;

  return hr;
}

HRESULT CEvrSharedRender::CreateTextures(ID3D11Device* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceEvr, int width, int height)
{
  HRESULT hr;
  m_pD3DDeviceKodi = pD3DDeviceKodi;
  m_pD3DDeviceEvr = pD3DDeviceEvr;
  m_dwWidth = width;
  m_dwHeight = height;

  CEvrCallback::Get()->Register(this);

  // Create VertexBuffer
  if (FAILED(hr = m_pD3DDeviceEvr->CreateVertexBuffer(sizeof(VID_FRAME_VERTEX) * 4, D3DUSAGE_WRITEONLY, D3DFVF_VID_FRAME_VERTEX, D3DPOOL_DEFAULT, &m_pEvrVertexBuffer, NULL)))
    CLog::Log(LOGDEBUG, "%s Failed to create EVR vertex buffer", __FUNCTION__);

  // Create Fake Staging Texture
  if (FAILED(hr = CreateFakeStaging(&m_pKodiFakeStaging)))
    CLog::Log(LOGDEBUG, "%s Failed to create fake staging texture", __FUNCTION__);

  // Create Under Shared Texture
  if (FAILED(hr = CreateSharedResource(&m_pEvrUnderTexture, &m_pKodiUnderTexture)))
    CLog::Log(LOGDEBUG, "%s Failed to create under shared texture", __FUNCTION__);

  // Create Over Shared Texture
  if (FAILED(hr = CreateSharedResource(&m_pEvrOverTexture, &m_pKodiOverTexture)))
    CLog::Log(LOGDEBUG, "%s Failed to create over shared texture", __FUNCTION__);

  return hr;
}

HRESULT CEvrSharedRender::Render(EVR_RENDER_LAYER layer)
{
  if (!CEvrCallback::Get()->GetRenderOnEvr() || (g_graphicsContext.IsFullScreenVideo() && layer == EVR_LAYER_UNDER))
    return S_FALSE;

  // Render the GUI on EVR
  RenderEvr(layer);

  return S_OK;
}

HRESULT CEvrSharedRender::RenderEvr(EVR_RENDER_LAYER layer)
{
  HRESULT hr = E_UNEXPECTED;

  // If the over layer it's empty skip the rendering of the under layer and drawn everything over EVR
  if (layer == EVR_LAYER_UNDER && !m_bGuiVisibleOver)
    return hr;

  if (layer == EVR_LAYER_OVER)
    m_bGuiVisibleOver ? layer = EVR_LAYER_OVER : layer = EVR_LAYER_UNDER;

  // Store EVR States
  if (FAILED(hr = StoreEvrDeviceState()))
    return hr;

  // Setup EVR Device
  if (FAILED(hr = SetupEvrDeviceState()))
    return hr;

  // Setup Vertex Buffer
  if (FAILED(hr = SetupVertex()))
    return hr;

  // Draw Kodi shared texture on EVR D3D9 device
  if (FAILED(hr = RenderTexture(layer)))
    return hr;

  // Restore EVR states
  if (FAILED(hr = RestoreEvrDeviceState()))
    return hr;

  return hr;
}

void CEvrSharedRender::RenderToUnderTexture()
{
  CEvrCallback::Get()->SetCurrentVideoLayer(EVR_LAYER_UNDER);
  CEvrCallback::Get()->ResetRenderCount();

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* pSurface11;

  m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiUnderTexture, NULL, &pSurface11);
  pContext->OMSetRenderTargets(1, &pSurface11, 0);
  pContext->ClearRenderTargetView(pSurface11, m_fColor);
  pSurface11->Release();
}

void CEvrSharedRender::RenderToOverTexture()
{
  CEvrCallback::Get()->SetCurrentVideoLayer(EVR_LAYER_OVER);

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* pSurface11;

  m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiOverTexture, NULL, &pSurface11);
  pContext->OMSetRenderTargets(1, &pSurface11, 0);
  pContext->ClearRenderTargetView(pSurface11, m_fColor);
  pSurface11->Release();
}

void CEvrSharedRender::EndRender()
{
  // Force to complete the rendering on Kodi device
  g_Windowing.FinishCommandList();
  ForceComplete();

  g_renderManager.OnAfterPresent(); // We need to do some stuff after Present

  m_bGuiVisible = CEvrCallback::Get()->GuiVisible();
  m_bGuiVisibleOver = CEvrCallback::Get()->GuiVisible(EVR_LAYER_OVER);
}

HRESULT CEvrSharedRender::RenderTexture(EVR_RENDER_LAYER layer)
{
  IDirect3DTexture9* pTexture9;

  layer == EVR_LAYER_UNDER ? pTexture9 = m_pEvrUnderTexture : pTexture9 = m_pEvrOverTexture;

  HRESULT hr = m_pD3DDeviceEvr->SetStreamSource(0, m_pEvrVertexBuffer, 0, sizeof(VID_FRAME_VERTEX));
  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceEvr->SetTexture(0, pTexture9);
  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceEvr->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
  if (FAILED(hr))
    return hr;

  return hr;
}

HRESULT CEvrSharedRender::SetupVertex()
{
  VID_FRAME_VERTEX* vertices = nullptr;

  // Lock the vertex buffer
  HRESULT hr = m_pEvrVertexBuffer->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD);

  if (SUCCEEDED(hr))
  {
    RECT rDest;
    rDest.bottom = m_dwHeight;
    rDest.left = 0;
    rDest.right = m_dwWidth;
    rDest.top = 0;

    vertices[0].x = (float)rDest.left - 0.5f;
    vertices[0].y = (float)rDest.top - 0.5f;
    vertices[0].z = 0.0f;
    vertices[0].rhw = 1.0f;
    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;

    vertices[1].x = (float)rDest.right - 0.5f;
    vertices[1].y = (float)rDest.top - 0.5f;
    vertices[1].z = 0.0f;
    vertices[1].rhw = 1.0f;
    vertices[1].u = 1.0f;
    vertices[1].v = 0.0f;

    vertices[2].x = (float)rDest.right - 0.5f;
    vertices[2].y = (float)rDest.bottom - 0.5f;
    vertices[2].z = 0.0f;
    vertices[2].rhw = 1.0f;
    vertices[2].u = 1.0f;
    vertices[2].v = 1.0f;

    vertices[3].x = (float)rDest.left - 0.5f;
    vertices[3].y = (float)rDest.bottom - 0.5f;
    vertices[3].z = 0.0f;
    vertices[3].rhw = 1.0f;
    vertices[3].u = 0.0f;
    vertices[3].v = 1.0f;

    hr = m_pEvrVertexBuffer->Unlock();
    if (FAILED(hr))
      return hr;
  }

  return hr;
}

HRESULT CEvrSharedRender::StoreEvrDeviceState()
{
  HRESULT hr = E_UNEXPECTED;

  if (FAILED(hr = m_pD3DDeviceEvr->GetScissorRect(&m_oldScissorRect)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetVertexShader(&m_pOldVS)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetFVF(&m_dwOldFVF)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetTexture(0, &m_pOldTexture)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetStreamSource(0, &m_pOldStreamData, &m_nOldOffsetInBytes, &m_nOldStride)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetRenderState(D3DRS_CULLMODE, &m_D3DRS_CULLMODE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetRenderState(D3DRS_LIGHTING, &m_D3DRS_LIGHTING)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetRenderState(D3DRS_ZENABLE, &m_D3DRS_ZENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_D3DRS_ALPHABLENDENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetRenderState(D3DRS_SRCBLEND, &m_D3DRS_SRCBLEND)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetRenderState(D3DRS_DESTBLEND, &m_D3DRS_DESTBLEND)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->GetPixelShader(&m_pPix)))
    return hr;

  return hr;
}

HRESULT CEvrSharedRender::SetupEvrDeviceState()
{
  HRESULT hr = E_UNEXPECTED;

  RECT newScissorRect;
  newScissorRect.bottom = m_dwHeight;
  newScissorRect.top = 0;
  newScissorRect.left = 0;
  newScissorRect.right = m_dwWidth;

  if (FAILED(hr = m_pD3DDeviceEvr->SetScissorRect(&newScissorRect)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetVertexShader(NULL)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetFVF(D3DFVF_VID_FRAME_VERTEX)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetPixelShader(NULL)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_LIGHTING, FALSE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_ZENABLE, FALSE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA)))
    return hr;

  return hr;
}

HRESULT CEvrSharedRender::RestoreEvrDeviceState()
{
  HRESULT hr = S_FALSE;

  if (FAILED(hr = m_pD3DDeviceEvr->SetScissorRect(&m_oldScissorRect)))
    return hr;

  hr = m_pD3DDeviceEvr->SetTexture(0, m_pOldTexture);

  if (m_pOldTexture)
    m_pOldTexture->Release();

  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceEvr->SetVertexShader(m_pOldVS);

  if (m_pOldVS)
    m_pOldVS->Release();

  if (FAILED(hr))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetFVF(m_dwOldFVF)))
    return hr;

  hr = m_pD3DDeviceEvr->SetStreamSource(0, m_pOldStreamData, m_nOldOffsetInBytes, m_nOldStride);

  if (m_pOldStreamData)
    m_pOldStreamData->Release();

  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceEvr->SetPixelShader(m_pPix);
  if (m_pPix)
    m_pPix->Release();

  if (FAILED(hr))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_CULLMODE, m_D3DRS_CULLMODE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_LIGHTING, m_D3DRS_LIGHTING)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_ZENABLE, m_D3DRS_ZENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_ALPHABLENDENABLE, m_D3DRS_ALPHABLENDENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_SRCBLEND, m_D3DRS_SRCBLEND)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceEvr->SetRenderState(D3DRS_DESTBLEND, m_D3DRS_DESTBLEND)))
    return hr;

  return hr;
}