/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayText.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "guilib/GraphicContext.h"
#include "Application.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/MathUtils.h"
#include "OverlayRendererUtil.h"
#include "OverlayRendererGUI.h"
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
}

CRenderer::~CRenderer()
{
  for(int i = 0; i < NUM_BUFFERS; i++)
    Release(m_buffers[i]);
}

void CRenderer::AddOverlay(CDVDOverlay* o, double pts, int index)
{
  CSingleLock lock(m_section);

  SElement   e;
  e.pts = pts;
  e.overlay_dvd = o->Acquire();
  m_buffers[index].push_back(e);
}

void CRenderer::AddOverlay(COverlay* o, double pts, int index)
{
  CSingleLock lock(m_section);

  SElement   e;
  e.pts = pts;
  e.overlay = o->Acquire();
  m_buffers[index].push_back(e);
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

  for(SElementV::iterator it = l.begin(); it != l.end(); ++it)
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

  for(COverlayV::iterator it = l.begin(); it != l.end(); ++it)
    (*it)->Release();
}

void CRenderer::Flush()
{
  CSingleLock lock(m_section);

  for(int i = 0; i < NUM_BUFFERS; i++)
    Release(m_buffers[i]);

  Release(m_cleanup);
}

void CRenderer::Release(int idx)
{
  CSingleLock lock(m_section);
  Release(m_buffers[idx]);
}

void CRenderer::Render(int idx)
{
  CSingleLock lock(m_section);

  Release(m_cleanup);

  std::vector<COverlay*> render;
  SElementV& list = m_buffers[idx];
  for(SElementV::iterator it = list.begin(); it != list.end(); ++it)
  {
    COverlay* o = NULL;

    if(it->overlay)
      o = it->overlay->Acquire();
    else if(it->overlay_dvd)
      o = Convert(it->overlay_dvd, it->pts);

    if(!o)
      continue;
 
    render.push_back(o);
  }

  float total_height = 0.0f;
  float cur_height = 0.0f;
  int subalign = CSettings::Get().GetInt("subtitles.align");
  for (std::vector<COverlay*>::iterator it = render.begin(); it != render.end(); ++it)
  {
    COverlay* o = *it;
    o->PrepareRender();
    total_height += o->m_height;
  }

  for (std::vector<COverlay*>::iterator it = render.begin(); it != render.end(); ++it)
  {
    COverlay* o = *it;

    float adjust_height = 0.0f;

    if(subalign == SUBTITLE_ALIGN_TOP_INSIDE ||
       subalign == SUBTITLE_ALIGN_TOP_OUTSIDE)
    {
      adjust_height = cur_height;
      cur_height += o->m_height;
    }
    else
    {
      total_height -= o->m_height;
      adjust_height = -total_height;
    }

    Render(o, adjust_height);

    o->Release();
  }
}

void CRenderer::Render(COverlay* o, float adjust_height)
{
  CRect rs, rd, rv;
  g_renderManager.GetVideoRect(rs, rd, rv);

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
      scale_x = (float)rv.Width();
      scale_y = (float)rv.Height();
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
      if(align == COverlay::ALIGN_SUBTITLE)
      {
        RESOLUTION_INFO res = g_graphicsContext.GetResInfo(g_renderManager.GetResolution());
        state.x += rv.x1 + rv.Width() * 0.5f;
        state.y += rv.y1  + (res.iSubtitles - res.Overscan.top);
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

      state.x      *= scale_x;
      state.y      *= scale_y;
      state.width  *= scale_x;
      state.height *= scale_y;

      state.x      += rd.x1;
      state.y      += rd.y1;
    }

  }

  state.x += GetStereoscopicDepth();
  state.y += adjust_height;

  o->Render(state);
}

bool CRenderer::HasOverlay(int idx)
{
  bool hasOverlay = false;

  CSingleLock lock(m_section);

  SElementV& list = m_buffers[idx];
  for(SElementV::iterator it = list.begin(); it != list.end(); ++it)
  {
    if (it->overlay || it->overlay_dvd)
    {
      hasOverlay = true;
      break;
    }
  }
  return hasOverlay;
}

COverlay* CRenderer::Convert(CDVDOverlaySSA* o, double pts)
{
  // libass render in a target area which named as frame. the frame size may bigger than video size,
  // and including margins between video to frame edge. libass allow to render subtitles into the margins.
  // this has been used to show subtitles in the top or bottom "black bar" between video to frame border.
  CRect src, dst, target;
  g_renderManager.GetVideoRect(src, dst, target);
  int videoWidth = MathUtils::round_int(dst.Width());
  int videoHeight = MathUtils::round_int(dst.Height());
  int targetWidth = MathUtils::round_int(target.Width());
  int targetHeight = MathUtils::round_int(target.Height());
  int useMargin;

  int subalign = CSettings::Get().GetInt("subtitles.align");
  if(subalign == SUBTITLE_ALIGN_BOTTOM_OUTSIDE
  || subalign == SUBTITLE_ALIGN_TOP_OUTSIDE
  ||(subalign == SUBTITLE_ALIGN_MANUAL && g_advancedSettings.m_videoAssFixedWorks))
    useMargin = 1;
  else
    useMargin = 0;
  double position;
  // position used to call ass_set_line_position, it's vertical line position of subtitles in percent.
  // value is 0-100: 0 = on the bottom (default), 100 = on top.
  if(subalign == SUBTITLE_ALIGN_TOP_INSIDE
  || subalign == SUBTITLE_ALIGN_TOP_OUTSIDE)
    position = 100.0;
  else if (subalign == SUBTITLE_ALIGN_MANUAL && g_advancedSettings.m_videoAssFixedWorks)
  {
    RESOLUTION_INFO res;
    res = g_graphicsContext.GetResInfo(g_renderManager.GetResolution());
    position = 100.0 - (res.iSubtitles - res.Overscan.top) * 100 / res.iHeight;
  }
  else
    position = 0.0;
  int changes = 0;
  ASS_Image* images = o->m_libass->RenderImage(targetWidth, targetHeight, videoWidth, videoHeight, pts, useMargin, position, &changes);

  if(o->m_overlay)
  {
    if(changes == 0)
      return o->m_overlay->Acquire();
  }

  COverlay *overlay = NULL;
#if defined(HAS_GL) || defined(HAS_GLES)
  overlay = new COverlayGlyphGL(images, targetWidth, targetHeight);
#elif defined(HAS_DX)
  overlay = new COverlayQuadsDX(images, targetWidth, targetHeight);
#endif
  // scale to video dimensions
  if (overlay)
  {
    overlay->m_width = (float)targetWidth / videoWidth;
    overlay->m_height = (float)targetHeight / videoHeight;
    overlay->m_x = ((float)videoWidth - targetWidth) / 2 / videoWidth;
    overlay->m_y = ((float)videoHeight - targetHeight) / 2 / videoHeight;
  }
  return overlay;
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

  if(!r && o->IsOverlayType(DVDOVERLAY_TYPE_TEXT))
    r = new COverlayText((CDVDOverlayText*)o);

  if(r)
    o->m_overlay = r->Acquire();
  return r;
}

