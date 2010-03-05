/*
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "cores/VideoRenderers/RenderManager.h"

#include "WindowingFactory.h" //d3d device and d3d interface

#include "application.h"
#include "file.h"
#include "DSRenderer.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#if (D3DX_SDK_VERSION >= 42) //aug 2009 sdk and up there is no dxerr9 anymore
  #include <Dxerr.h>
#else
  #include <dxerr9.h>
  #define DXGetErrorString(hr)      DXGetErrorString9(hr)
  #define DXGetErrorDescription(hr) DXGetErrorDescription9(hr)
#endif


#include "util.h"

CDsRenderer::CDsRenderer()
: CUnknown(NAME("CDsRenderer"), NULL)
{
  m_nCurSurface = 0;
  m_nNbDXSurface = 1;
  m_nVMR9Surfaces = 0;
  m_iVMR9Surface = 0;

  g_renderManager.PreInit(true);
  g_Windowing.Register(this); 
}

CDsRenderer::~CDsRenderer()
{
  DeleteSurfaces();
  //vm9 unload current surface??
  g_Windowing.Unregister(this);
  g_renderManager.UnInit();
}

UINT CDsRenderer::GetAdapter(IDirect3D9* pD3D)
{
  HMONITOR hMonitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
  if(hMonitor == NULL) return D3DADAPTER_DEFAULT;

  for(UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
  {
    HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
    if(hAdpMon == hMonitor) return adp;
  }

  return D3DADAPTER_DEFAULT;
}

HRESULT CDsRenderer::CreateSurfaces(D3DFORMAT Format)
{
  HRESULT hr = S_OK;
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);

  for(int i = 0; i < m_nNbDXSurface+2; i++)
  {
    m_pVideoTexture[i] = NULL;
    m_pVideoSurface[i] = NULL;
  }

  m_SurfaceType = Format;

  int nTexturesNeeded = m_nNbDXSurface + 2;

  for (int i = 0; i < nTexturesNeeded; i++)
  {
    if(FAILED(hr = g_Windowing.Get3DDevice()->CreateTexture(
      m_iVideoWidth, m_iVideoHeight, 1, 
      D3DUSAGE_RENDERTARGET, Format/*D3DFMT_X8R8G8B8 D3DFMT_A8R8G8B8*/, 
      D3DPOOL_DEFAULT, &m_pVideoTexture[i], NULL)))
      return hr;

    if(FAILED(hr = m_pVideoTexture[i]->GetSurfaceLevel(0, &m_pVideoSurface[i])))
      return hr;
  }

  hr = g_Windowing.Get3DDevice()->ColorFill(m_pVideoSurface[m_nCurSurface], NULL, 0);
  
  return S_OK;
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