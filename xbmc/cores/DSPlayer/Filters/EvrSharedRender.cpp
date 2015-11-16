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
#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"
#include "cores/VideoRenderers/RenderManager.h"

CEvrSharedRender::CEvrSharedRender()
{
}

CEvrSharedRender::~CEvrSharedRender()
{
}

HRESULT CEvrSharedRender::CreateTextures(ID3D11Device* pD3DDeviceKodi, IDirect3DDevice9Ex* pD3DDeviceDS, int width, int height)
{
  HRESULT hr = __super::CreateTextures(pD3DDeviceKodi, pD3DDeviceDS, width, height);
  CDSRendererCallback::Get()->Register(this);

  return hr;
}

HRESULT CEvrSharedRender::Render(DS_RENDER_LAYER layer)
{
  if (!CDSRendererCallback::Get()->GetRenderOnDS() || (g_graphicsContext.IsFullScreenVideo() && layer == RENDER_LAYER_UNDER))
    return S_FALSE;

  // Render the GUI on EVR
  RenderInternal(layer);

  return S_OK;
}

void CEvrSharedRender::RenderToUnderTexture()
{
  CDSRendererCallback::Get()->SetCurrentVideoLayer(RENDER_LAYER_UNDER);
  CDSRendererCallback::Get()->ResetRenderCount();

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* pSurface11;

  m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiUnderTexture, NULL, &pSurface11);
  pContext->OMSetRenderTargets(1, &pSurface11, 0);
  pContext->ClearRenderTargetView(pSurface11, m_fColor);
  pSurface11->Release();
}

void CEvrSharedRender::RenderToOverTexture()
{
  CDSRendererCallback::Get()->SetCurrentVideoLayer(RENDER_LAYER_OVER);

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* pSurface11;

  m_pD3DDeviceKodi->CreateRenderTargetView(m_pKodiOverTexture, NULL, &pSurface11);
  pContext->OMSetRenderTargets(1, &pSurface11, 0);
  pContext->ClearRenderTargetView(pSurface11, m_fColor);
  pSurface11->Release();
}

void CEvrSharedRender::EndRender()
{
  m_bGuiVisible = CDSRendererCallback::Get()->GuiVisible();
  m_bGuiVisibleOver = CDSRendererCallback::Get()->GuiVisible(RENDER_LAYER_OVER);

  if (!m_bGuiVisibleOver && !g_graphicsContext.IsFullScreenVideo())
    g_renderManager.Render(true, 0, 255);

  // Force to complete the rendering on Kodi device
  g_Windowing.FinishCommandList();
  ForceComplete();

  g_renderManager.OnAfterPresent(); // We need to do some stuff after Present
}
