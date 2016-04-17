/*
 *      Copyright (C) 2005-2016 Team XBMC
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


#include "DebugRenderer.h"
#include "OverlayRendererGUI.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayText.h"

using namespace OVERLAY;

CDebugRenderer::CDebugRenderer()
{
  for (int i=0; i<4; i++)
  {
    m_overlay[i] = nullptr;
    m_strDebug[i] = " ";
  }
}

CDebugRenderer::~CDebugRenderer()
{
  for (int i=0; i<4; i++)
  {
    if (m_overlay[i])
      m_overlay[i]->Release();
  }
}

void CDebugRenderer::SetInfo(std::string &info1, std::string &info2, std::string &info3, std::string &info4)
{
  m_overlayRenderer.Release(0);

  if (info1 != m_strDebug[0])
  {
    m_strDebug[0] = info1;
    if (m_overlay[0])
      m_overlay[0]->Release();
    m_overlay[0] = new CDVDOverlayText();
    m_overlay[0]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[0]));
  }
  if (info2 != m_strDebug[1])
  {
    m_strDebug[1] = info2;
    if (m_overlay[1])
      m_overlay[1]->Release();
    m_overlay[1] = new CDVDOverlayText();
    m_overlay[1]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[1]));
  }
  if (info3 != m_strDebug[2])
  {
    m_strDebug[2] = info3;
    if (m_overlay[2])
      m_overlay[2]->Release();
    m_overlay[2] = new CDVDOverlayText();
    m_overlay[2]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[2]));
  }
  if (info4 != m_strDebug[3])
  {
    m_strDebug[3] = info4;
    if (m_overlay[3])
      m_overlay[3]->Release();
    m_overlay[3] = new CDVDOverlayText();
    m_overlay[3]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[3]));
  }

  m_overlayRenderer.AddOverlay(m_overlay[0], 0, 0);
  m_overlayRenderer.AddOverlay(m_overlay[1], 0, 0);
  m_overlayRenderer.AddOverlay(m_overlay[2], 0, 0);
  m_overlayRenderer.AddOverlay(m_overlay[3], 0, 0);
}

void CDebugRenderer::Render(CRect &src, CRect &dst, CRect &view)
{
  m_overlayRenderer.SetVideoRect(src, dst, view);
  m_overlayRenderer.Render(0);
}

void CDebugRenderer::Flush()
{
  m_overlayRenderer.Flush();
}

CDebugRenderer::CRenderer::CRenderer() : OVERLAY::CRenderer()
{
  m_font = "__debug__";
  m_fontBorder = "__debugborder__";
}

void CDebugRenderer::CRenderer::Render(int idx)
{
  std::vector<COverlay*> render;
  std::vector<SElement>& list = m_buffers[idx];
  int posY = 0;
  for (std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    COverlay* o = nullptr;

    if (it->overlay_dvd)
      o = Convert(it->overlay_dvd, it->pts);

    if (!o)
      continue;

    COverlayText *text = dynamic_cast<COverlayText*>(o);
    if (text)
      text->PrepareRender("arial.ttf", 1, 16, 0, m_font, m_fontBorder);

    o->m_pos = COverlay::POSITION_ABSOLUTE;
    o->m_align = COverlay::ALIGN_SCREEN;
    o->m_x = 10 + o->m_width / 2;
    o->m_y = posY + o->m_height;
    OVERLAY::CRenderer::Render(o, 0);

    posY = o->m_y;
  }

  ReleaseUnused();
}
