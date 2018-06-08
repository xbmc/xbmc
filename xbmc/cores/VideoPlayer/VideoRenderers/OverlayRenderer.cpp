/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "OverlayRenderer.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayText.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "windowing/GraphicContext.h"
#include "guilib/GUIFontManager.h"
#include "Application.h"
#include "ServiceBroker.h"
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
}

COverlay::~COverlay() = default;

unsigned int CRenderer::m_textureid = 1;

CRenderer::CRenderer()
{
  m_font = "__subtitle__";
  m_fontBorder = "__subtitleborder__";
}

CRenderer::~CRenderer()
{
  Flush();
}

void CRenderer::AddOverlay(CDVDOverlay* o, double pts, int index)
{
  CSingleLock lock(m_section);

  SElement   e;
  e.pts = pts;
  e.overlay_dvd = o->Acquire();
  m_buffers[index].push_back(e);
}

void CRenderer::Release(std::vector<SElement>& list)
{
  std::vector<SElement> l = list;
  list.clear();

  for (auto &elem : l)
  {
    if (elem.overlay_dvd)
      elem.overlay_dvd->Release();
  }
}

void CRenderer::Flush()
{
  CSingleLock lock(m_section);

  for(int i = 0; i < NUM_BUFFERS; i++)
    Release(m_buffers[i]);

  ReleaseCache();

  g_fontManager.Unload(m_font);
  g_fontManager.Unload(m_fontBorder);
}

void CRenderer::Release(int idx)
{
  CSingleLock lock(m_section);
  Release(m_buffers[idx]);
}

void CRenderer::ReleaseCache()
{
  for (auto& overlay : m_textureCache)
  {
    delete overlay.second;
  }
  m_textureCache.clear();
  m_textureid++;
}

void CRenderer::ReleaseUnused()
{
  for (auto it = m_textureCache.begin(); it != m_textureCache.end(); )
  {
    bool found = false;
    for (auto& buffer : m_buffers)
    {
      for (auto& dvdoverlay : buffer)
      {
        if (dvdoverlay.overlay_dvd && dvdoverlay.overlay_dvd->m_textureid == it->first)
        {
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
    if (!found)
    {
      delete it->second;
      it = m_textureCache.erase(it);
    }
    else
      ++it;
  }
}

void CRenderer::Render(int idx)
{
  CSingleLock lock(m_section);

  std::vector<COverlay*> render;
  std::vector<SElement>& list = m_buffers[idx];
  for(std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    COverlay* o = NULL;

    if(it->overlay_dvd)
      o = Convert(it->overlay_dvd, it->pts);

    if(!o)
      continue;

    render.push_back(o);
  }

  float total_height = 0.0f;
  float cur_height = 0.0f;
  int subalign = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_SUBTITLES_ALIGN);
  for (std::vector<COverlay*>::iterator it = render.begin(); it != render.end(); ++it)
  {
    COverlay* o = nullptr;
    COverlayText *text = dynamic_cast<COverlayText*>(*it);
    if (text)
    {
      text->PrepareRender(CServiceBroker::GetSettings().GetString(CSettings::SETTING_SUBTITLES_FONT),
                          CServiceBroker::GetSettings().GetInt(CSettings::SETTING_SUBTITLES_COLOR),
                          CServiceBroker::GetSettings().GetInt(CSettings::SETTING_SUBTITLES_HEIGHT),
                          CServiceBroker::GetSettings().GetInt(CSettings::SETTING_SUBTITLES_STYLE),
                          m_font, m_fontBorder);
      o = text;
    }
    else
    {
      o = *it;
      o->PrepareRender();
    }
    total_height += o->m_height;
  }

  for (std::vector<COverlay*>::iterator it = render.begin(); it != render.end(); ++it)
  {
    COverlay* o = *it;

    float adjust_height = 0.0f;

    if (o->m_type == COverlay::TYPE_GUITEXT)
    {
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
    }

    Render(o, adjust_height);
  }

  ReleaseUnused();
}

void CRenderer::Render(COverlay* o, float adjust_height)
{
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
      scale_x = (float)m_rv.Width();
      scale_y = (float)m_rv.Height();
    }

    if(align == COverlay::ALIGN_VIDEO)
    {
      scale_x = m_rs.Width();
      scale_y = m_rs.Height();
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
        RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
        state.x += m_rv.x1 + m_rv.Width() * 0.5f;
        state.y += m_rv.y1  + (res.iSubtitles - res.Overscan.top);
      }
      else
      {
        state.x += m_rv.x1;
        state.y += m_rv.y1;
      }
    }

    if(align == COverlay::ALIGN_VIDEO)
    {
      float scale_x = m_rd.Width() / m_rs.Width();
      float scale_y = m_rd.Height() / m_rs.Height();

      state.x      *= scale_x;
      state.y      *= scale_y;
      state.width  *= scale_x;
      state.height *= scale_y;

      state.x      += m_rd.x1;
      state.y      += m_rd.y1;
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

  std::vector<SElement>& list = m_buffers[idx];
  for(std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    if (it->overlay_dvd)
    {
      hasOverlay = true;
      break;
    }
  }
  return hasOverlay;
}

void CRenderer::SetVideoRect(CRect &source, CRect &dest, CRect &view)
{
  m_rs = source;
  m_rd = dest;
  m_rv = view;
}

COverlay* CRenderer::Convert(CDVDOverlaySSA* o, double pts)
{
  // libass render in a target area which named as frame. the frame size may bigger than video size,
  // and including margins between video to frame edge. libass allow to render subtitles into the margins.
  // this has been used to show subtitles in the top or bottom "black bar" between video to frame border.
  int videoWidth = MathUtils::round_int(m_rd.Width());
  int videoHeight = MathUtils::round_int(m_rd.Height());
  int targetWidth = MathUtils::round_int(m_rv.Width());
  int targetHeight = MathUtils::round_int(m_rv.Height());
  int useMargin;

  int subalign = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_SUBTITLES_ALIGN);
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
    res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
    position = 100.0 - (res.iSubtitles - res.Overscan.top) * 100 / res.iHeight;
  }
  else
    position = 0.0;
  int changes = 0;
  ASS_Image* images = o->m_libass->RenderImage(targetWidth, targetHeight, videoWidth, videoHeight, pts, useMargin, position, &changes);

  if(o->m_textureid)
  {
    if(changes == 0)
    {
      std::map<unsigned int, COverlay*>::iterator it = m_textureCache.find(o->m_textureid);
      if (it != m_textureCache.end())
        return it->second;
    }
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
  m_textureCache[m_textureid] = overlay;
  o->m_textureid = m_textureid;
  m_textureid++;
  return overlay;
}


COverlay* CRenderer::Convert(CDVDOverlay* o, double pts)
{
  COverlay* r = NULL;

  if(o->IsOverlayType(DVDOVERLAY_TYPE_SSA))
    r = Convert(static_cast<CDVDOverlaySSA*>(o), pts);
  else if(o->m_textureid)
  {
    std::map<unsigned int, COverlay*>::iterator it = m_textureCache.find(o->m_textureid);
    if (it != m_textureCache.end())
      r = it->second;
  }

  if (r)
  {
    return r;
  }

#if defined(HAS_GL) || defined(HAS_GLES)
  if (o->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
    r = new COverlayTextureGL(static_cast<CDVDOverlayImage*>(o));
  else if(o->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    r = new COverlayTextureGL(static_cast<CDVDOverlaySpu*>(o));
#elif defined(HAS_DX)
  if (o->IsOverlayType(DVDOVERLAY_TYPE_IMAGE))
    r = new COverlayImageDX(static_cast<CDVDOverlayImage*>(o));
  else if(o->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    r = new COverlayImageDX(static_cast<CDVDOverlaySpu*>(o));
#endif

  if(!r && o->IsOverlayType(DVDOVERLAY_TYPE_TEXT))
    r = new COverlayText(static_cast<CDVDOverlayText*>(o));

  m_textureCache[m_textureid] = r;
  o->m_textureid = m_textureid;
  m_textureid++;

  return r;
}
