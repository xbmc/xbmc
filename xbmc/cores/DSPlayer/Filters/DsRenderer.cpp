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
#include "dxerr.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxVobsub.h"
#include "util.h"

CDsRenderer::CDsRenderer()
: CUnknown(NAME("CDsRenderer"), NULL),
m_CurrentSubtitle(STREAM_SUBTITLE),
m_dsPlayerSubtitle(&m_overlayContainer)
{
  m_nCurSurface = 0;
  g_renderManager.PreInit(true);
  g_Windowing.Register(this); 
}

CDsRenderer::~CDsRenderer()
{
  g_renderManager.UnInit();
  g_Windowing.Unregister(this);
}

void CDsRenderer::AddSubtitleStream()
{

}

bool CDsRenderer::AddSubtitleFile(const std::string& filename)
{
  std::string ext = CUtil::GetExtension(filename);
  if(ext == ".idx")
  {
    CDVDDemuxVobsub v;
    if(!v.Open(filename))
      return false;

    m_SelectionStreams.Update(NULL, &v);
    return true;
  }
  if(ext == ".sub")
  {
    CStdString strReplace;
    CUtil::ReplaceExtension(filename,".idx",strReplace);
    if (XFILE::CFile::Exists(strReplace))
      return false;
  }
  SelectionStream s;
  s.source   = m_SelectionStreams.Source(STREAM_SOURCE_TEXT, filename);
  s.type     = STREAM_SUBTITLE;
  s.id       = 0;
  s.filename = filename;
  s.name     = CUtil::GetFileName(filename);
  m_SelectionStreams.Update(s);
  return true;
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
  m_SurfaceType = Format;
  bool hrb;

  for( int i = 0; i < DS_MAX_3D_SURFACE; ++i ) 
  {
    m_pVideoTexture[i] = new CD3DTexture();
    m_pVideoSurface[i] = NULL;
  }
     

  for (int i = 0; i < DS_NBR_3D_SURFACE; i++)
  {
    hrb = m_pVideoTexture[i]->Create(m_iVideoWidth,  
                                    m_iVideoHeight,
                                    1,               /* Levels */
                                    D3DUSAGE_RENDERTARGET,
                                    m_SurfaceType,        /* D3D_FORMAT */
                                    D3DPOOL_DEFAULT);
    if (!hrb)
    {
      CLog::Log(LOGERROR,"%s Failed to create texture number %i",__FUNCTION__,i);
      break;
    }
    else
    {
      g_Windowing.Register(m_pVideoTexture[i]);
      hr = m_pVideoTexture[i]->GetSurfaceLevel(0, &m_pVideoSurface[i]);
    }
  }
  return hr;
}

void CDsRenderer::DeleteSurfaces()
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);

  for( int i = 0; i < DS_NBR_3D_SURFACE; ++i ) 
  {
    if (m_pVideoTexture[i] != NULL)
      m_pVideoTexture[i]->Release();
    m_pVideoSurface[i] = NULL;
  }

}

STDMETHODIMP CDsRenderer::RenderPresent(CD3DTexture* videoTexture,IDirect3DSurface9* videoSurface)
{

  
  if (!g_renderManager.IsConfigured())
    return E_FAIL;
  
  
  g_renderManager.PaintVideoTexture(videoTexture,videoSurface);
  
  g_application.NewFrame();
  g_application.WaitFrame(100);
  return S_OK;

}