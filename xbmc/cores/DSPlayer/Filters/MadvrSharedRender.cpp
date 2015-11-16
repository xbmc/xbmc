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
#include "Utils/Log.h"
#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"

CMadvrSharedRender::CMadvrSharedRender()
{
}

CMadvrSharedRender::~CMadvrSharedRender()
{
}

HRESULT CMadvrSharedRender::CreateTextures(ID3D11Device* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceDS, int width, int height)
{
  HRESULT hr = __super::CreateTextures(pD3DDeviceKodi, pD3DDeviceDS, width, height);
  CDSRendererCallback::Get()->Register(this);

  return hr;
}


HRESULT CMadvrSharedRender::Render(DS_RENDER_LAYER layer)
{
  // Lock madVR thread while kodi rendering
  CAutoLock lock(&m_dsLock);

  if (!CDSRendererCallback::Get()->GetRenderOnDS() || (g_graphicsContext.IsFullScreenVideo() && layer == RENDER_LAYER_UNDER))
    return CALLBACK_INFO_DISPLAY;

  // Render the GUI on madVR
  RenderInternal(layer);

  // Pull the trigger on the wait for the main Kodi application thread
  if (layer == RENDER_LAYER_OVER)
    m_kodiWait.Unlock();

  // Return to madVR if we rendered something
  return (m_bGuiVisible) ? CALLBACK_USER_INTERFACE : CALLBACK_INFO_DISPLAY;
}

void CMadvrSharedRender::RenderToUnderTexture()
{
  // Wait that madVR complete the rendering
  m_kodiWait.Wait(100);

  {
    // Lock madVR thread while kodi rendering
    CAutoLock lock(&m_dsLock);
    m_dsLock.Lock();

    CDSRendererCallback::Get()->SetCurrentVideoLayer(RENDER_LAYER_UNDER);
    CDSRendererCallback::Get()->ResetRenderCount();

    ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
    ID3D11RenderTargetView* pSurface11;

    m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiUnderTexture, NULL, &pSurface11);
    pContext->OMSetRenderTargets(1, &pSurface11, 0);
    pContext->ClearRenderTargetView(pSurface11, m_fColor);
    pSurface11->Release();
  }
}

void CMadvrSharedRender::RenderToOverTexture()
{
  CDSRendererCallback::Get()->SetCurrentVideoLayer(RENDER_LAYER_OVER);

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* pSurface11;

  m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiOverTexture, NULL, &pSurface11);
  pContext->OMSetRenderTargets(1, &pSurface11, 0);
  pContext->ClearRenderTargetView(pSurface11, m_fColor);
  pSurface11->Release();
}

void CMadvrSharedRender::EndRender()
{
  // Force to complete the rendering on Kodi device
  g_Windowing.FinishCommandList();
  ForceComplete();

  m_bGuiVisible = CDSRendererCallback::Get()->GuiVisible();
  m_bGuiVisibleOver = CDSRendererCallback::Get()->GuiVisible(RENDER_LAYER_OVER);

  // Unlock madVR rendering
  m_dsLock.Unlock();
}
