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
#include "windowing/WindowingFactory.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "settings/AdvancedSettings.h"

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
  color_t clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;
  CD3DHelper::XMStoreColor(m_fColor, clearColour);
}

CMadvrSharedRender::~CMadvrSharedRender()
{
  Release(m_pMadvrVertexBuffer);
  Release(m_pMadvrUnderTexture);
  Release(m_pMadvrOverTexture);  
  Release(m_pKodiUnderSurface);
  Release(m_pKodiOverSurface);
  Release(m_pKodiUnderTexture);
  Release(m_pKodiOverTexture);
}

void CMadvrSharedRender::Release(IUnknown* pUnknown)
{
  if (pUnknown)
  {
    pUnknown->Release();
    pUnknown = nullptr;
  }
}

HRESULT CMadvrSharedRender::GetSharedHandle(IUnknown* pUnknown, HANDLE* pHandle)
{
  ASSERT(pUnknown);
  ASSERT(pHandle);

  HRESULT hr = S_OK;

  *pHandle = NULL;
  IDXGIResource* pSurface;

  if (FAILED(hr = pUnknown->QueryInterface(__uuidof(IDXGIResource), (void**)&pSurface)))
  {
    return hr;
  }

  hr = pSurface->GetSharedHandle(pHandle);
  pSurface->Release();

  return hr;
}

HRESULT CMadvrSharedRender::CreateTextureDX11( ID3D11Device* m_pDevice, UINT Width, UINT Height, DXGI_FORMAT format, ID3D11Texture2D** ppTexture, HANDLE* pHandle)
{
  ASSERT(m_pDevice);
  ASSERT(ppTexture);
  ASSERT(pHandle);

  HRESULT hr;

  D3D11_TEXTURE2D_DESC Desc;
  Desc.Width = Width;
  Desc.Height = Height;
  Desc.MipLevels = 1;
  Desc.ArraySize = 1;
  Desc.Format = format;
  Desc.SampleDesc.Count = 1;
  Desc.SampleDesc.Quality = 0;
  Desc.Usage = D3D11_USAGE_DEFAULT;
  Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  Desc.CPUAccessFlags = 0;
  Desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

  hr = m_pDevice->CreateTexture2D(&Desc, NULL, ppTexture);

  if (SUCCEEDED(hr))
  {
    if (FAILED(GetSharedHandle(*ppTexture, pHandle)))
    {
      (*ppTexture)->Release();
      (*ppTexture) = NULL;
    }
  }
  return hr;
}

HRESULT CMadvrSharedRender::CreateRenderTargetView()
{
  HRESULT hr;

  CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2D);

  if (FAILED(hr = m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiUnderTexture, &rtDesc, &m_pKodiUnderSurface)))
    return hr;

  if (FAILED(hr = m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiOverTexture, &rtDesc, &m_pKodiOverSurface)))
    return hr;

  return hr;
}

HRESULT CMadvrSharedRender::CreateTextures(ID3D11Device* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceMadVR, int width, int height)
{
  HRESULT hr;
  m_pD3DDeviceKodi = pD3DDeviceKodi;
  m_pD3DDeviceMadVR = pD3DDeviceMadVR;
  m_dwWidth = width;
  m_dwHeight = height;

  // Create VertexBuffer
  if (FAILED(hr = m_pD3DDeviceMadVR->CreateVertexBuffer(sizeof(VID_FRAME_VERTEX) * 4, D3DUSAGE_WRITEONLY, D3DFVF_VID_FRAME_VERTEX, D3DPOOL_DEFAULT, &m_pMadvrVertexBuffer, NULL)))
    CLog::Log(LOGDEBUG, "%s Failed to create madVR vertex buffer", __FUNCTION__);

  // Under Shared Texture
  if (FAILED(hr = CreateTextureDX11(m_pD3DDeviceKodi, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, &m_pKodiUnderTexture, &m_pSharedUnderHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create kodi shared under texture", __FUNCTION__);

  if (FAILED(hr = m_pD3DDeviceMadVR->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pMadvrUnderTexture, &m_pSharedUnderHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create madVR shared under texture", __FUNCTION__, m_pSharedUnderHandle);

  // Over Shared Texture
  if (FAILED(hr = CreateTextureDX11(m_pD3DDeviceKodi, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, &m_pKodiOverTexture, &m_pSharedOverHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create kodi shared over texture", __FUNCTION__);

  if (FAILED(hr = m_pD3DDeviceMadVR->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pMadvrOverTexture, &m_pSharedOverHandle)))
    CLog::Log(LOGDEBUG, "%s Failed to create madVR shared over texture", __FUNCTION__);

  // Create RrenderTargetView
  if (FAILED(hr = CreateRenderTargetView()))
    CLog::Log(LOGDEBUG, "%s Failed to create textures render target view", __FUNCTION__);

  return hr;
}

HRESULT CMadvrSharedRender::RenderMadvr(MADVR_RENDER_LAYER layer)
{
  HRESULT hr = CALLBACK_INFO_DISPLAY;

  if (!CMadvrCallback::Get()->GetRenderOnMadvr())
    return hr;

  // Reset render count to notice when the GUI it's active or deactive
  CMadvrCallback::Get()->ResetRenderCount();

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

  // Begin render Kodi Gui
  g_Windowing.BeginRender();

  // Render to Shared texture Surface
  RenderToTexture(layer);

  // Call the render from madVR thread
  (layer == RENDER_LAYER_UNDER) ? g_windowManager.Render() : g_application.RenderNoPresent();

  // Present the frame from madVR thread
  CDirtyRegionList dirtyRegions = g_windowManager.GetDirty();
  g_graphicsContext.Flip(dirtyRegions);

  // Pull the trigger on Applicaiton.Render() FrameWait();
  g_renderManager.NewFrame();

  // End Render Kodi Gui
  g_Windowing.EndRender();

  // Return without render in madVR if the Kodi Gui isn't visible or if there is a resize in progress
  if (!CMadvrCallback::Get()->IsGuiActive() || g_Windowing.GetResizeInProgress())
    return hr;

  // Setup madVR Device
  if (FAILED(SetupMadDeviceState()))
    return hr;

  // Setup Vertex Buffer
  if (FAILED(SetupVertex()))
    return hr;

  // Draw Kodi shared texture on madVR D3D9 device
  if (FAILED(RenderTexture(layer)))
    return hr;

  // Restore madVR states
  if (FAILED(RestoreMadDeviceState()))
    return hr;

  // Return to madVR that we rendered something
  return CALLBACK_USER_INTERFACE;
}

HRESULT CMadvrSharedRender::RenderToTexture(MADVR_RENDER_LAYER layer)
{
  HRESULT hr = S_OK;
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* pRenderTargetView;

  (layer == RENDER_LAYER_UNDER) ? pRenderTargetView = m_pKodiUnderSurface : pRenderTargetView = m_pKodiOverSurface;

  pContext->OMSetRenderTargets(1, &pRenderTargetView, 0);
  pContext->ClearRenderTargetView(pRenderTargetView, m_fColor);

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