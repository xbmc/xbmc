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
#include "DSRenderer.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

CDsRenderer::CDsRenderer()
: CUnknown(NAME("CDsRenderer"), NULL)
{
  m_D3D = g_Windowing.Get3DObject();
  m_D3DDev = g_Windowing.Get3DDevice();
  
  g_renderManager.PreInit(true);
  
}

CDsRenderer::~CDsRenderer()
{
  //release id3dresource
  CSingleLock lock(m_critsection);
  
  

  g_renderManager.UnInit();
}

void CDsRenderer::InitClock()
{
  if (m_VideoClock.ThreadHandle() == NULL)
  {
    m_VideoClock.Create();
    //we have to wait for the clock to start otherwise alsa can cause trouble
    if (!m_VideoClock.WaitStarted(2000))
      CLog::Log(LOGDEBUG, "m_VideoClock didn't start in time");
  }

}
HRESULT CDsRenderer::AllocSurface(D3DFORMAT Format)
{
  EnterCriticalSection(&m_critPrensent);
  HRESULT hr;
  m_pVideoTexture = NULL;
  m_pVideoSurface = NULL;
  m_SurfaceType = Format;
  D3DDISPLAYMODE dm;
  hr = m_D3DDev->GetDisplayMode(NULL, &dm);
  
  if (SUCCEEDED(hr))
    m_D3DDev->CreateTexture(m_iVideoWidth,  m_iVideoHeight,
                            1,
                            D3DUSAGE_RENDERTARGET,
                            dm.Format,//m_SurfaceType, // default is D3DFMT_A8R8G8B8
                            D3DPOOL_DEFAULT,
                            &m_pVideoTexture,
                            NULL);
  if (SUCCEEDED(hr))
    m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface);

  LeaveCriticalSection(&m_critPrensent);
  return hr;
}


STDMETHODIMP CDsRenderer::RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface)
{

  HRESULT hr;
  if (!g_renderManager.IsConfigured())
    return E_FAIL;
  
  CSingleLock lock(m_critsection);
  g_renderManager.PaintVideoTexture(videoTexture,videoSurface);
  g_application.NewFrame();
  g_application.WaitFrame(100);
  return S_OK;

}