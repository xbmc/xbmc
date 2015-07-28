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
#include "MadvrSharedRender.h"
#include "mvrInterfaces.h"
#include "Application.h"
#include "Utils/Log.h"
#include "guilib/GUIWindowManager.h"
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

CMadvrSharedRender::CMadvrSharedRender()
{
}

CMadvrSharedRender::~CMadvrSharedRender()
{
  m_pMadvrVertexBuffer = nullptr;
  m_pMadvrUnderTexture = nullptr;
  m_pKodiUnderTexture = nullptr;
  m_pMadvrOverTexture = nullptr;
  m_pKodiOverTexture = nullptr;
}

HRESULT CMadvrSharedRender::CreateTextures(IDirect3DDevice9Ex* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceMadVR, int width, int height)
{
  // Create Shared Texture
  m_pD3DDeviceKodi = pD3DDeviceKodi;
  m_pD3DDeviceMadVR = pD3DDeviceMadVR;

  HRESULT hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->CreateVertexBuffer(sizeof(VID_FRAME_VERTEX) * 4, D3DUSAGE_WRITEONLY, D3DFVF_VID_FRAME_VERTEX, D3DPOOL_DEFAULT, &m_pMadvrVertexBuffer, NULL)))
    CLog::Log(LOGDEBUG, "%s Failed to create madVR vertex buffer", __FUNCTION__);

  //Under Textures
  if (FAILED(hr = m_pD3DDeviceKodi->CreateTexture(width, height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pKodiUnderTexture, &m_pSharedUnderHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create kodi shared texture", __FUNCTION__);

  if (FAILED(hr = m_pD3DDeviceMadVR->CreateTexture(width, height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pMadvrUnderTexture, &m_pSharedUnderHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create madVR shared texture", __FUNCTION__);

  //Over Textures
  if (FAILED(hr = m_pD3DDeviceKodi->CreateTexture(width, height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pKodiOverTexture, &m_pSharedOverHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create kodi shared texture", __FUNCTION__);

  if (FAILED(hr = m_pD3DDeviceMadVR->CreateTexture(width, height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pMadvrOverTexture, &m_pSharedOverHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create madVR shared texture", __FUNCTION__);

  return hr;
}

HRESULT CMadvrSharedRender::RenderMadvr(MADVR_RENDER_LAYER layer, int width, int height)
{
  HRESULT hr = CALLBACK_INFO_DISPLAY;

  if (!CMadvrCallback::Get()->GetRenderOnMadvr())
    return hr;

  // Reset render count to notice when the GUI it's active or deactive
  CMadvrCallback::Get()->ResetRenderCount();

  m_dwWidth = width;
  m_dwHeight = height;

  // if the context it's kodi menu manage the layer
  if (!g_graphicsContext.IsFullScreenVideo())
  {
    if (layer == RENDER_LAYER_OVER && !CMadvrCallback::Get()->IsVideoLayer())
    {
      // re-check if there is a videolayer before render over
      CMadvrCallback::Get()->SetRenderLayer(RENDER_LAYER_CHECK);
      g_windowManager.Render();

      // if there is not a video layer render all over the video
      if (!CMadvrCallback::Get()->IsVideoLayer())
        layer = RENDER_LAYER_ALL;
    }

    //set initial render variables
    CMadvrCallback::Get()->SetVideoLayer(false);
    CMadvrCallback::Get()->SetRenderLayer(layer);
  }

  // Store madVR States
  if (FAILED(StoreMadDeviceState()))
    return hr;

  // Render Kodi Gui
  RenderToTexture(layer);
  m_pD3DDeviceKodi->BeginScene();
  (layer == RENDER_LAYER_UNDER) ? g_windowManager.Render() : g_application.RenderNoPresent();
  g_renderManager.NewFrame();
  m_pD3DDeviceKodi->EndScene();
  m_pD3DDeviceKodi->Present(NULL, NULL, NULL, NULL);

  //Setup madVR Device
  if (FAILED(SetupMadDeviceState()))
    return hr;

  // Setup Vertex Buffer
  if (FAILED(SetupVertex()))
    return hr;

  // Draw Kodi shared texture on madVR
  if (FAILED(RenderTexture(layer)))
    return hr;

  // Restore madVR states
  if (FAILED(RestoreMadDeviceState()))
    return hr;

  // return an hresult for madVR renderOSD latency mode
  return CMadvrCallback::Get()->IsGuiActive() ? CALLBACK_USER_INTERFACE : CALLBACK_INFO_DISPLAY;
}

HRESULT CMadvrSharedRender::RenderToTexture(MADVR_RENDER_LAYER layer)
{
  HRESULT hr = E_UNEXPECTED;

  Com::SmartPtr<IDirect3DTexture9> pTexture;
  Com::SmartPtr<IDirect3DSurface9> pSurface;
  layer == RENDER_LAYER_UNDER ? pTexture = m_pKodiUnderTexture : pTexture = m_pKodiOverTexture;

  if (FAILED(hr = pTexture->GetSurfaceLevel(0, &pSurface)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceKodi->SetRenderTarget(0, pSurface)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceKodi->Clear(0, NULL, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.0f, 0)))
    return hr;

  return hr;
}

HRESULT CMadvrSharedRender::RenderTexture(MADVR_RENDER_LAYER layer)
{
  Com::SmartPtr<IDirect3DTexture9> pTexture;
  layer == RENDER_LAYER_UNDER ? pTexture = m_pMadvrUnderTexture : pTexture = m_pMadvrOverTexture;

  HRESULT hr = m_pD3DDeviceMadVR->SetStreamSource(0, m_pMadvrVertexBuffer, 0, sizeof(VID_FRAME_VERTEX));
  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceMadVR->SetTexture(0, pTexture);
  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceMadVR->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
  if (FAILED(hr))
    return hr;

  return S_OK;
}

HRESULT CMadvrSharedRender::SetupVertex()
{
  VID_FRAME_VERTEX* vertices = nullptr;

  // Lock the vertex buffer
  HRESULT hr = m_pMadvrVertexBuffer->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD);

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

    hr = m_pMadvrVertexBuffer->Unlock();
    if (FAILED(hr))
      return hr;
  }

  return hr;
}

HRESULT CMadvrSharedRender::StoreMadDeviceState()
{
  HRESULT hr = E_UNEXPECTED;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetScissorRect(&m_oldScissorRect)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetVertexShader(&m_pOldVS)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetFVF(&m_dwOldFVF)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetTexture(0, &m_pOldTexture)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetStreamSource(0, &m_pOldStreamData, &m_nOldOffsetInBytes, &m_nOldStride)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetRenderState(D3DRS_CULLMODE, &m_D3DRS_CULLMODE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetRenderState(D3DRS_LIGHTING, &m_D3DRS_LIGHTING)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetRenderState(D3DRS_ZENABLE, &m_D3DRS_ZENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_D3DRS_ALPHABLENDENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetRenderState(D3DRS_SRCBLEND, &m_D3DRS_SRCBLEND)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetRenderState(D3DRS_DESTBLEND, &m_D3DRS_DESTBLEND)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->GetPixelShader(&m_pPix)))
    return hr;

  return hr;
}

HRESULT CMadvrSharedRender::SetupMadDeviceState()
{
  HRESULT hr = E_UNEXPECTED;

  RECT newScissorRect;
  newScissorRect.bottom = m_dwHeight;
  newScissorRect.top = 0;
  newScissorRect.left = 0;
  newScissorRect.right = m_dwWidth;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetScissorRect(&newScissorRect)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetVertexShader(NULL)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetFVF(D3DFVF_VID_FRAME_VERTEX)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetPixelShader(NULL)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_LIGHTING, FALSE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_ZENABLE, FALSE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA)))
    return hr;

  return hr;
}

HRESULT CMadvrSharedRender::RestoreMadDeviceState()
{
  HRESULT hr = S_FALSE;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetScissorRect(&m_oldScissorRect)))
    return hr;

  hr = m_pD3DDeviceMadVR->SetTexture(0, m_pOldTexture);

  if (m_pOldTexture)
    m_pOldTexture->Release();

  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceMadVR->SetVertexShader(m_pOldVS);

  if (m_pOldVS)
    m_pOldVS->Release();

  if (FAILED(hr))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetFVF(m_dwOldFVF)))
    return hr;

  hr = m_pD3DDeviceMadVR->SetStreamSource(0, m_pOldStreamData, m_nOldOffsetInBytes, m_nOldStride);

  if (m_pOldStreamData)
    m_pOldStreamData->Release();

  if (FAILED(hr))
    return hr;

  hr = m_pD3DDeviceMadVR->SetPixelShader(m_pPix);
  if (m_pPix)
    m_pPix->Release();

  if (FAILED(hr))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_CULLMODE, m_D3DRS_CULLMODE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_LIGHTING, m_D3DRS_LIGHTING)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_ZENABLE, m_D3DRS_ZENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_ALPHABLENDENABLE, m_D3DRS_ALPHABLENDENABLE)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_SRCBLEND, m_D3DRS_SRCBLEND)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceMadVR->SetRenderState(D3DRS_DESTBLEND, m_D3DRS_DESTBLEND)))
    return hr;

  return hr;
}