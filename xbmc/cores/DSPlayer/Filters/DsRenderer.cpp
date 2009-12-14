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

STDMETHODIMP CDsRenderer::RenderPresent(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface)
{

  
  if (!g_renderManager.IsConfigured())
    return E_FAIL;
  
  CSingleLock lock(m_critsection);
  g_renderManager.PaintVideoTexture(videoTexture,videoSurface);
  g_application.NewFrame();
  g_application.WaitFrame(100);
  return S_OK;

}