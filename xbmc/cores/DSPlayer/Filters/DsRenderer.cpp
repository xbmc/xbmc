/*
 *      Copyright (C) 2005-2010 Team XBMC
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


#include "cores/VideoRenderers/RenderManager.h"

#include "WindowingFactory.h" //d3d device and d3d interface

#include "application.h"
#include "file.h"
#include "DSRenderer.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "dxerr.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxVobsub.h"
#include "util.h"

CDsRenderer::CDsRenderer()
: CUnknown(NAME("CDsRenderer"), NULL)
{
  m_nCurSurface = 0;
  g_renderManager.PreInit(RENDERER_DSHOW_VMR9);
  g_Windowing.Register(this); 
}

CDsRenderer::~CDsRenderer()
{
  g_renderManager.UnInit();
  g_Windowing.Unregister(this);
}

UINT CDsRenderer::GetAdapter(IDirect3D9* pD3D)
{
  return g_Windowing.GetCurrentAdapter();
  /*HMONITOR hMonitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
	if(hMonitor == NULL) return D3DADAPTER_DEFAULT;

	for(UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
	{
		HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
		if(hAdpMon == hMonitor) 
		  return adp;
	}

	return D3DADAPTER_DEFAULT;*/
}

HRESULT CDsRenderer::CreateSurfaces(D3DFORMAT Format)
{
  HRESULT hr = S_OK;
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);
  m_SurfaceType = Format;
  bool hrb;

  for( int i = 0; i < DS_MAX_3D_SURFACE; ++i ) 
  {
    m_pVideoTexture[i] = NULL;
    m_pVideoSurface[i] = NULL;
  }
     
  for (int i = 0; i < DS_NBR_3D_SURFACE; i++)
  {
    if(FAILED(hr = g_Windowing.Get3DDevice()->CreateTexture(
      m_iVideoWidth, m_iVideoHeight, 1, 
      D3DUSAGE_RENDERTARGET, m_SurfaceType/*D3DFMT_X8R8G8B8 D3DFMT_A8R8G8B8*/, 
      D3DPOOL_DEFAULT, &m_pVideoTexture[i], NULL)))
      return hr;
    if(FAILED(hr = m_pVideoTexture[i]->GetSurfaceLevel(0, &m_pVideoSurface[i])))
      return hr;
  }
  return hr;
}

void CDsRenderer::DeleteSurfaces()
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);

  for( int i = 0; i < DS_NBR_3D_SURFACE; ++i ) 
  {
    m_pVideoTexture[i] = NULL;
    m_pVideoSurface[i] = NULL;
  }

}

STDMETHODIMP CDsRenderer::RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface)
{  
  if (!g_renderManager.IsConfigured())
    return E_FAIL;  
  
  g_renderManager.PaintVideoTexture(videoTexture,videoSurface);
  
  g_application.NewFrame();
  g_application.WaitFrame(100);
  return S_OK;

}