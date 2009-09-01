/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *      Initial code sponsored by: Voddler Inc (voddler.com)
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
#include "stdafx.h"
#include "OverlayRenderer.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "../../Settings.h"
#ifdef HAS_GL
#include "OverlayRendererGL.h"
#endif


using namespace OVERLAY;


COverlay::COverlay()
{
  m_x      = 0.0f;
  m_y      = 0.0f;
  m_width  = 0.0;
  m_height = 0.0;
  m_type   = TYPE_NONE;
  m_align  = ALIGN_SCREEN;
  m_pos    = POSITION_RELATIVE;
  m_references = 1;
}

COverlay::~COverlay()
{
}

COverlay* COverlay::Acquire()
{
  InterlockedIncrement(&m_references);
  return this;
}

long COverlay::Release()
{
  long count = InterlockedDecrement(&m_references);
  if (count == 0)
    delete this;

  return count;
}

CRenderer::CRenderer()
{
  m_render = 0;
  m_decode = (m_render + 1) % 2;
}

CRenderer::~CRenderer()
{
  for(int i = 0; i < 2; i++)
    Release(m_buffers[i]);
}

void CRenderer::AddOverlay(CDVDOverlay* o)
{
  CSingleLock lock(m_section);

  SElement   e;
  if(o->m_overlay)
    e.overlay     = o->m_overlay->Acquire();
  else
    e.overlay_dvd = o->Acquire();
  m_buffers[m_decode].push_back(e);
}

void CRenderer::AddOverlay(COverlay* o)
{
  CSingleLock lock(m_section);

  SElement   e;
  e.overlay = o->Acquire();
  m_buffers[m_decode].push_back(e);
}

void CRenderer::AddCleanup(COverlay* o)
{
  CSingleLock lock(m_section);
  m_cleanup.push_back(o->Acquire());
}

void CRenderer::Release(SElementV& list)
{
  SElementV l = list;
  list.clear();

  for(SElementV::iterator it = l.begin(); it != l.end(); it++)
  {
    if(it->overlay)
      it->overlay->Release();
    if(it->overlay_dvd)
      it->overlay_dvd->Release();
  }
}

void CRenderer::Release(COverlayV& list)
{
  COverlayV l = list;
  list.clear();

  for(COverlayV::iterator it = l.begin(); it != l.end(); it++)
    (*it)->Release();
}

void CRenderer::Flush()
{
  CSingleLock lock(m_section);

  for(int i = 0; i < 2; i++)
    Release(m_buffers[i]);

  Release(m_cleanup);
}

void CRenderer::Flip()
{
  CSingleLock lock(m_section);

  m_render = m_decode;
  m_decode =(m_decode + 1) % 2;

  Release(m_buffers[m_decode]);
}

void CRenderer::Render()
{
  CSingleLock lock(m_section);

  Release(m_cleanup);

  SElementV& list = m_buffers[m_render];
  for(SElementV::iterator it = list.begin(); it != list.end(); it++)
  {
    COverlay*& o = it->overlay;

    if(!o && it->overlay_dvd)
      o = Convert(it->overlay_dvd);

    if(!o)
      continue;

    Render(o);
  }
}

void CRenderer::Render(COverlay* o)
{
  RECT rs, rd, rv;
  RESOLUTION_INFO res;
  g_renderManager.GetVideoRect(rs, rd);
  rv  = g_graphicsContext.GetViewWindow();
  res = g_settings.m_ResInfo[g_renderManager.GetResolution()];

  SRenderState state;
  state.x       = o->m_x;
  state.y       = o->m_y;
  state.width   = o->m_width;
  state.height  = o->m_height;

  COverlay::EPosition pos   = o->m_pos;
  COverlay::EAlign    align = o->m_align;

  if(pos == COverlay::POSITION_RELATIVE)
  {
    float scale_x = 1.0;
    float scale_y = 1.0;

    if(align == COverlay::ALIGN_SCREEN
    || align == COverlay::ALIGN_SUBTITLE)
    {
      scale_x = (float)res.iWidth;
      scale_y = (float)res.iHeight;
    }

    if(align == COverlay::ALIGN_VIDEO)
    {
      scale_x = (float)(rs.right  - rs.left);
      scale_y = (float)(rs.bottom - rs.top);
    }

    state.x      *= scale_x;
    state.y      *= scale_y;
    state.width  *= scale_x;
    state.height *= scale_y;

    pos = COverlay::POSITION_ABSOLUTE;
  }

  if(pos == COverlay::POSITION_ABSOLUTE)
  {
    if(align == COverlay::ALIGN_SCREEN
    || align == COverlay::ALIGN_SUBTITLE)
    {
      float scale_x = (float)(rv.right  - rv.left) / res.iWidth;
      float scale_y = (float)(rv.bottom - rv.top)  / res.iHeight;

      state.x      *= scale_x;
      state.y      *= scale_y;
      state.width  *= scale_x;
      state.height *= scale_y;

      if(align == COverlay::ALIGN_SUBTITLE)
      {
        state.x += rv.left + (rv.right - rv.left) * 0.5;
        state.y += rv.top  + (res.iSubtitles - res.Overscan.top) * scale_y;
      }
      else
      {
        state.x += rv.left;
        state.y += rv.top;
      }
    }

    if(align == COverlay::ALIGN_VIDEO)
    {
      float scale_x = (float)(rd.right  - rd.left) / (rs.right  - rs.left);
      float scale_y = (float)(rd.bottom - rd.top)  / (rs.bottom - rs.top);

      state.x      -= rs.left;
      state.y      -= rs.top;

      state.x      *= scale_x;
      state.y      *= scale_y;
      state.width  *= scale_x;
      state.height *= scale_y;

      state.x      += rd.left;
      state.y      += rd.top;
    }

  }

  o->Render(state);
}

COverlay* CRenderer::Convert(CDVDOverlay* o)
{
  COverlay* r = o->m_overlay;
  if(r)
    return r->Acquire();

#ifdef HAS_GL
  if     (o->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
    r = new COverlayTextureGL((CDVDOverlayImage*)o);
  else if(o->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    r = new COverlayTextureGL((CDVDOverlaySpu*)o);
#endif

  if(r)
    o->m_overlay = r->Acquire();
  return r;
}
