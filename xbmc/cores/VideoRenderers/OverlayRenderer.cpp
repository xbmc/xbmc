/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#include "OverlayRenderer.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/MathUtils.h"
#if defined(HAS_GL) || defined(HAS_GLES)
#include "OverlayRendererGL.h"
#elif defined(HAS_DX)
#include "OverlayRendererDX.h"
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
  AtomicIncrement(&m_references);
  return this;
}

long COverlay::Release()
{
  long count = AtomicDecrement(&m_references);
  if (count == 0)
    delete this;

  return count;
}

long COverlayMainThread::Release()
{
  long count = AtomicDecrement(&m_references);
  if (count == 0)
  {
    if (g_application.IsCurrentThread())
      delete this;
    else
      g_renderManager.AddCleanup(this);
  }
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

void CRenderer::AddOverlay(CDVDOverlay* o, double pts)
{
  CSingleLock lock(m_section);

  SElement   e;
  e.pts = pts;
  e.overlay_dvd = o->Acquire();
  m_buffers[m_decode].push_back(e);
}

void CRenderer::AddOverlay(COverlay* o, double pts)
{
  CSingleLock lock(m_section);

  SElement   e;
  e.pts = pts;
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
    COverlay* o = NULL;

    if(it->overlay)
      o = it->overlay->Acquire();
    else if(it->overlay_dvd)
      o = Convert(it->overlay_dvd, it->pts);

    if(!o)
      continue;

    Render(o);

    o->Release();
  }
}

void CRenderer::Render(COverlay* o)
{
  CRect rs, rd, rv;
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
      scale_x = rs.Width();
      scale_y = rs.Height();
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
      float scale_x = rv.Width() / res.iWidth;
      float scale_y = rv.Height()  / res.iHeight;

      state.x      *= scale_x;
      state.y      *= scale_y;
      state.width  *= scale_x;
      state.height *= scale_y;

      if(align == COverlay::ALIGN_SUBTITLE)
      {
        state.x += rv.x1 + rv.Width() * 0.5f;
        state.y += rv.y1  + (res.iSubtitles - res.Overscan.top) * scale_y;
      }
      else
      {
        state.x += rv.x1;
        state.y += rv.y1;
      }
    }

    if(align == COverlay::ALIGN_VIDEO)
    {
      float scale_x = rd.Width() / rs.Width();
      float scale_y = rd.Height() / rs.Height();

      state.x      -= rs.x1;
      state.y      -= rs.y1;

      state.x      *= scale_x;
      state.y      *= scale_y;
      state.width  *= scale_x;
      state.height *= scale_y;

      state.x      += rd.x1;
      state.y      += rd.y1;
    }

  }

  o->Render(state);
}

COverlay* CRenderer::Convert(CDVDOverlaySSA* o, double pts)
{
  CRect src, dst;
  g_renderManager.GetVideoRect(src, dst);

  int width  = MathUtils::round_int(dst.Width());
  int height = MathUtils::round_int(dst.Height());

  int changes = 0;
  ASS_Image* images = o->m_libass->RenderImage(width, height, pts, &changes);

  if(o->m_overlay)
  {
    if(changes == 0)
      return o->m_overlay->Acquire();
  }

#if defined(HAS_GL) || defined(HAS_GLES)
  return new COverlayGlyphGL(images, width, height);
#elif defined(HAS_DX)
  return new COverlayQuadsDX(images, width, height);
#endif
  return NULL;
}


COverlay* CRenderer::Convert(CDVDOverlay* o, double pts)
{
  COverlay* r = NULL;

  if(o->IsOverlayType(DVDOVERLAY_TYPE_SSA))
    r = Convert((CDVDOverlaySSA*)o, pts);
  else if(o->m_overlay)
    r = o->m_overlay->Acquire();

  if(r)
  {
    if(o->m_overlay)
      o->m_overlay->Release();
    o->m_overlay = r->Acquire();
    return r;
  }

#if defined(HAS_GL) || defined(HAS_GLES)
  if     (o->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
    r = new COverlayTextureGL((CDVDOverlayImage*)o);
  else if(o->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    r = new COverlayTextureGL((CDVDOverlaySpu*)o);
#elif defined(HAS_DX)
  if     (o->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
    r = new COverlayImageDX((CDVDOverlayImage*)o);
  else if(o->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    r = new COverlayImageDX((CDVDOverlaySpu*)o);
#endif

  if(r)
    o->m_overlay = r->Acquire();
  return r;
}

